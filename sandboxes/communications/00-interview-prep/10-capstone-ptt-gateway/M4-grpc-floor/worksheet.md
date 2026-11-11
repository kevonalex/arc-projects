# Capstone M4 — gRPC control plane and floor control

**Tier:** Capstone, milestone 4 of 4. **Time:** 5–7 h. **Code:** `code/` (stubs `include/gateway_core.hpp`, reference `solutions/gateway_core.hpp`, tests `tests/test_gateway.cpp`; optional **untested** gRPC skeleton in `code/grpc/`).
**Safety/legal:** synthetic or recorded-with-permission audio only. No live public-safety reception, ever (in the UK that also runs into the Wireless Telegraphy Act 2006). The gRPC skeleton binds to `127.0.0.1` with insecure credentials: local development only.

---

## §1 Why this milestone exists

M1–M3 built the **media plane**: bits on the air and voice in RTP. A real gateway also needs a **control plane**: who is calling on which talkgroup, who is allowed to talk right now, and how healthy is the link. That is this milestone. Pipeline position:

```
WAV in -> 20 ms frames -> [M1] frame+FEC -> [M3] 4FSK -> AWGN+CFO -> demod/decode -> [M1] parse
       -> [M2] mu-law RTP over UDP multicast -> jitter buffer -> WAV out
                      ^                                   |
   [M4] control plane: StartCall / EndCall, floor control (who talks), streaming stats (BER, loss, jitter)
```

gRPC carries **control and telemetry only** — never the voice. Voice stays on RTP/UDP because it needs deadlines, not retransmission (M2 Q13).

The design pattern is the lesson: all the interesting logic (validation, idempotency, arbitration, stats, retry rules) lives in a **transport-independent, single-threaded, fully unit-tested core** (`gateway_core.hpp`). The gRPC server is a thin adapter you can read in five minutes. That separation is what you describe in an interview when asked "how do you test a gRPC service?".

By the end you can: implement a PTT **floor controller** (priority queue, preemption, talk-burst timer), a call table with **idempotent** `StartCall` and correct status codes, derived statistics, **exponential backoff** and a retry policy, and explain deadlines, streaming and thread safety for a gRPC service.

## §2 Prerequisites

* Capstone M1–M3 (what BER, loss and jitter mean). Basic C++ containers; a mental model of RPC (client calls a function on a server).
* Build: `cmake -S code -B build -DCMAKE_CXX_FLAGS="-Wall -Wextra -fsanitize=address,undefined"`, `cmake --build build`, `ctest --test-dir build`. `student.*` fail until implemented; `solution.*` pass. The gRPC part is optional (`-DWITH_GRPC=ON`, needs gRPC + protobuf installed; **untested skeleton, VERIFY names**).

## §3 Concept build-up

### §3.1 Floor control, the rules (W 3.1)

**Push-to-talk is half-duplex:** on one talkgroup exactly one person transmits at a time. If two radios key up at once, a gateway that forwards both produces overlapping garbage (or the receivers hear a mixture they can't decode). **Floor control** is the referee: a user *requests* the floor; the controller grants, queues, preempts or denies.

Concrete rules (the tests pin each):

| Situation | Result | Events |
|---|---|---|
| empty user, or priority outside 0..7 | `Denied` | none |
| floor idle | `Granted` | `Granted(user)` |
| requester already holds it | `AlreadyHolder` | none |
| priority **strictly higher** than the holder's | `Preempted` | `Revoked(old)`, `Granted(new)`; old holder **not** re-queued; if the requester was waiting it leaves the queue |
| already queued (equal or lower priority) | `Queued` | none (no duplicate entry) |
| queue full | `Denied` | none |
| otherwise | `Queued` | `Queued(user)` |

Priority 0 is normal, 7 is emergency. Equal priority never preempts (including emergency vs emergency): otherwise two emergencies would steal the floor from each other forever.

**Grant order** is *highest priority first, oldest first within a priority* (stable sort on arrival order). Example: holder at priority 7; requests n1(0), n2(0), p3(3), n3(0), p3b(3) → order `[p3, p3b, n1, n2, n3]`.

### §3.2 Release, withdrawal and idle (W 3.2)

* **Holder releases** → event `Released(user)`, then the *next waiter* is granted (`Granted`) or, if nobody waits, `Idle`.
* **A waiter releases** = withdraws from the queue: silently removed, no events.
* A user who neither holds nor waits → returns `false` (the service maps this to `FailedPrecondition`).

### §3.3 The talk-burst timer (W 3.3)

A stuck or forgotten PTT (a handset in a bag, a crashed console) must not hold the group forever. `tick(ms)` adds elapsed time to the holder's total; once `held ≥ max_talk_ms` the holder is **timed out** (`TimedOut(user)`) and the next waiter is granted exactly as for a release. Details the tests pin:

* the limit is `≥` (1000 ms limit: 600 + 400 hits it);
* a new grant resets the timer to zero (the next talker gets the full allowance);
* with no holder, `tick` does nothing (an idle floor has no timer).

Why the controller does not own a clock: time is *injected* via `tick`. Tests then run in zero real time and are deterministic, and the adapter chooses the tick source (a timer thread every 100 ms).

> **Misconception box.** *"The timer revokes the floor, so the talker is cut off immediately."* The controller only changes *state* and emits an event; the adapter must tell the talker (`TimedOut` message) and stop forwarding their audio. *"Preemption means the old talker queues up again."* No — they're revoked and must ask again; re-queuing would make emergency preemption a no-op under churn.
> **Known limitation to be able to discuss:** a queued user who re-requests at a *higher* priority (but still not above the holder) is *not* reprioritised — the request returns `Queued` and the original entry stands.

### §3.4 The call table: validation, idempotency, capacity (W 3.4)

`GatewayService` keeps a map `call_id → {CallStats, FloorController}`.

`start_call(call_id, talkgroup, caller)`:

1. **Validate in this order:** `call_id` 1–64 chars; talkgroup 1–65535 (16-bit, matching M2's multicast mapping); caller non-empty → `InvalidArgument`.
2. **Idempotency:** if `call_id` exists with the *identical* parameters → `Ok` (the retry-safe case); with different parameters → `AlreadyExists`.
3. **Capacity:** a *new* call when the table is full → `ResourceExhausted`. (An existing identical call is not "new", so it is still `Ok` when full.)

`end_call` → `Ok` or `NotFound`.

Why idempotent: the network can lose the *response* after the server did the work. The client then retries; if `StartCall` weren't idempotent the retry would fail with "already exists" and the client could not tell whether *its* call was created. The **client-chosen `call_id` is an idempotency key**: same key + same parameters = same call.

**Status codes** (numbers equal gRPC's, so the adapter is a `static_cast`; VERIFY against `grpc/status_code_enum.h`): `Ok 0`, `InvalidArgument 3`, `DeadlineExceeded 4`, `NotFound 5`, `AlreadyExists 6`, `ResourceExhausted 8`, `FailedPrecondition 9`, `Unavailable 14`.

Choosing a code is an API-design decision: `InvalidArgument` = "your request is wrong regardless of server state" (don't retry); `FailedPrecondition` = "valid, but the system state doesn't allow it now" (e.g. releasing a floor you don't hold); `NotFound` = unknown id; `AlreadyExists` = conflict; `ResourceExhausted` = quota/capacity; `Unavailable` = transient, retry with backoff.

### §3.5 Stats: counters stored, ratios derived (W 3.5)

`record_frame(call, received, bits_checked, bit_errors, jitter_sample_ms)`:

* received: `packets++`, `bits_checked += bits`, `bit_errors += errs`, `jitter += (sample − jitter)/16` (the RFC 3550 smoothing from M2);
* lost: `lost++` only (a lost frame has no jitter sample and no bits).

`snapshot()` returns a copy with **derived** fields computed on the spot: `loss_pct = 100 × lost / (packets + lost)`, `ber = bit_errors / bits_checked` (0 when nothing checked: no division by zero). Never store a derived value next to its inputs: they drift apart. Store counters (monotonic, mergeable), derive ratios when asked.

Worked example (the test): frames: (rx, 1000 bits, 10 errors, jitter 16) → jitter `0 + 16/16 = 1.0`; (rx, 1000, 0, 0.0) → `1.0 − 1/16 = 0.9375`; (lost) → jitter untouched; (rx, 1000, 5, 0.0) → `0.9375 × 15/16 = 0.87890625`. Totals: packets 3, lost 1 → loss **25 %**; bits 3000, errors 15 → BER **0.005**.

### §3.6 Resilience: deadlines, backoff, retries (W 3.6)

* **Deadline:** every client call carries one. Without it, a dead server hangs the caller forever (and, with a thread per call, exhausts the client). The deadline is propagated to the server so it can stop working for a caller who has given up.
* **Exponential backoff:** wait `min(cap, base × 2^attempt)` ms: with base 100 ms, cap 5000 ms the waits are 100, 200, 400, 800, 1600, 3200, **5000** (6400 capped), then 5000 forever; 7 retries wait 11.3 s in total. It must not overflow for attempt = 1000 (the test): stop doubling once the cap is reached. In production add **jitter** (randomise each wait) so thousands of clients that failed together don't retry together (retry storm / thundering herd).
* **What may be retried:** `Unavailable` → yes (the request wasn't processed); `DeadlineExceeded` → only if the RPC is **idempotent** (the server may have done the work); everything else is a verdict, not a hiccup: no.

> **Misconception box.** *"Retries make a system reliable."* Unbounded, unjittered retries turn a small outage into a big one. Bound attempts, back off, jitter, and retry only what is safe. *"gRPC gives exactly-once."* It gives at-most-once per attempt; exactly-once effects need idempotency (keys), as in §3.4.

### §3.7 gRPC in one page (skeleton under `code/grpc/`, untested)

* **Four shapes:** unary (`StartCall`, `EndCall`), server streaming (`WatchStats`: one `CallStats` every `interval_ms` until the call ends or the client cancels), client streaming (unused here), bidirectional streaming (`FloorChannel`: client sends `REQUEST/RELEASE`, server pushes `GRANTED/QUEUED/REVOKED/TIMED_OUT/IDLE/DENIED`).
* **Why gRPC for control:** typed contracts (`.proto`), HTTP/2 multiplexing, built-in deadlines, cancellation, status codes, streaming with flow control, generated stubs in many languages. **Why not for audio:** retransmission/ordering semantics and head-of-line blocking (M2).
* **proto3 evolution:** field numbers are the wire identity — never reuse or renumber; add new fields with new numbers; unknown fields are ignored. Note the skeleton's `CallStats` message omits `talkgroup`, `caller`, `bits_checked`, `bit_errors` (the core has them) — a deliberate "what would you add?" exercise.
* **Threading:** gRPC invokes handlers from a thread pool. The core is intentionally single-threaded; the adapter holds **one mutex** around core calls. Rule: never hold the lock while doing blocking I/O (writing to a stream); copy the data out under the lock, then write.
* **Security:** insecure credentials = localhost only; beyond that TLS/mTLS, per-call authentication and authorisation (who may request the floor on which talkgroup).

### §3.8 Misconceptions summarised

| Claim | Reality |
|---|---|
| Floor control is just a mutex | It has priority, preemption, queueing, timeouts and notifications: a protocol, not a lock. |
| Higher priority always wins | Strictly higher only; equal never preempts; and starvation of low priority must be considered. |
| Time belongs inside the controller | Injected time (`tick`) makes it deterministic and testable. |
| Idempotent = "ignore duplicates" | Idempotent = repeating the *same request* has the same *effect and answer*; conflicting reuse must still fail. |
| One status code per failure is enough | Clients decide on retry/fix/report from the code: choose precisely. |
| Derived stats can be stored | Store counters; derive ratios. |
| Retry on every error | Only transient errors, with backoff + jitter; DeadlineExceeded only if idempotent. |

## §4 Worked examples

**(a) A floor sequence.** Limit 30 s, queue 4.

| Step | Call | Result | Events drained |
|---|---|---|---|
| 1 | `request(alice, 0)` | Granted | `Granted(alice)` |
| 2 | `request(alice, 0)` | AlreadyHolder | – |
| 3 | `request(bob, 0)` twice | Queued, Queued | `Queued(bob)` once |
| 4 | `release(alice)` | true | `Released(alice)`, `Granted(bob)` |
| 5 | `release(bob)` | true | `Released(bob)`, `Idle` |
| 6 | `release(bob)` | false | – |

**(b) Preemption.** `a(1)` holds; `b(1)` queues (equal does not preempt); `c(7)` → **Preempted**: events `Revoked(a)`, `Granted(c)`; queue `[b]` (a gone); `d(7)` → **Queued** (emergency does not preempt emergency).

**(c) Timer.** max 1000 ms; `a` holds, `b` waits. `tick(600)`: nothing. `tick(400)`: `TimedOut(a)`, `Granted(b)`. `tick(999)`: nothing (b's timer restarted). `tick(1)`: `TimedOut(b)`, `Idle`. `tick(100000)` on the idle floor: nothing.

**(d) Service glue (`GatewayService(8, 500, 2)`).** floor request for an unknown call → `NotFound`; start call `tg`; `a` granted, `b` queued; `floor_release(tg, "zzz")` → `FailedPrecondition`; `tick(500)` → holder becomes `b`; `floor_request(tg, "", 0)` → `FloorResult::Denied`, mapped to `ResourceExhausted` (a mapping you'll critique in Q11).

## §5 Questions

Do §5 on paper first. **Story** questions are written answers of 150–250 words that become interview stories: *context → decision → alternatives rejected → numbers → what I'd change*.

**Q1 — Draw.** Sketch the whole capstone gateway with the **media plane** (M1–M3 + RTP) and the **control plane** (gRPC). Mark which messages travel over each, which protocol/transport, and where the floor controller, call table and stats live. Why isn't voice carried over gRPC?
*Context: W 1.*

**Q2 — Trace.** For limit 30 s and queue 4, trace: `request(a,0)`, `request(a,0)`, `request(b,0)`, `request(b,0)`, `release(a)`, `release(b)`, `release(b)`. Give the result of each call and the events drained at the end.
*Context: W 3.1–3.2.*

**Q3 — Predict.** Holder `h` at priority 7; then `n1(0)`, `n2(0)`, `p3(3)`, `n3(0)`, `p3b(3)` request. Give `queue_order()`, then the events and new holder after `release(h)`, and who holds after `release(p3)`.
*Context: W 3.1.*

**Q4 — Trace.** `a(1)` holds, `b(1)` requests, `c(7)` requests, `d(7)` requests. Give each result, the events, the queue and the holder. Why is `a` not re-queued?
*Context: W 3.1.*

**Q5 — Timer.** Limit 1000 ms. `a` holds, `b` waits. Trace `tick(600)`, `tick(400)`, `tick(999)`, `tick(1)`, `tick(100000)`. What changes if the adapter ticks every 100 ms but the limit is 1050 ms? Why is time injected rather than read from a clock?
*Context: W 3.3.*

**Q6 — Classify.** Give the status code for each: `start_call("", 100, "alice")`; talkgroup 0; talkgroup 65536; empty caller; the same call again with identical arguments; same id with talkgroup 101; same id with caller "mallory"; a new call when the table (size 2) is full; the same *existing* call again when the table is full; `end_call` twice. Why this validation order?
*Context: W 3.4.*

**Q7 — Explain.** A client sends `StartCall(c1, 100, alice)`; the server creates the call but the response is lost; the client times out and retries. Walk through what happens (a) with an idempotent server, (b) with a non-idempotent one. Why is the call id supplied by the *client*?
*Context: W 3.4.*

**Q8 — Calculate.** Reproduce the stats test: after the four frames (rx 1000/10/16.0, rx 1000/0/0.0, lost, rx 1000/5/0.0) give packets, lost, loss %, bits checked, errors, BER and each intermediate jitter value. Why does a lost frame not touch the jitter estimate, and why are `loss_pct` and `ber` not stored?
*Context: W 3.5.*

**Q9 — Calculate.** Backoff with base 100 ms, cap 5000 ms for attempts 0–8 and the cumulative wait after 7 retries. Explain how the implementation avoids overflow at attempt 1000 and what "cap below base" (cap 50) returns. Why add random jitter?
*Context: W 3.6.*

**Q10 — Decide.** For each code and each of `idempotent = true/false` say whether `is_retryable` should be true: `Ok`, `InvalidArgument`, `NotFound`, `AlreadyExists`, `ResourceExhausted`, `FailedPrecondition`, `DeadlineExceeded`, `Unavailable`. Justify `DeadlineExceeded` and `ResourceExhausted` in one sentence each.
*Context: W 3.6.*

**Q11 — Critique.** The service maps `FloorResult::Denied` to `ResourceExhausted`, but `Denied` covers two different causes (invalid priority/empty user; queue full). Propose a better mapping, say what the client should do in each case, and describe how you'd change the core without breaking existing tests.
*Context: W 4(d).*

**Q12 — Debug.** The adapter handles `FloorChannel` streams on gRPC threads while a timer thread calls `tick()`. Describe a data race that would occur without synchronisation (name the variables), a deadlock you could create by holding the lock during a blocking stream write, and the correct pattern. Which sanitizer finds the race (hint: not ASan/UBSan)?
*Context: W 3.7.*

**Q13 — Story (design).** *Why floor control, and what happens without it?* Cover: half-duplex PTT semantics, collision of simultaneous talkers at the gateway (two RTP streams to one group), who decides in a conventional radio system vs an IP gateway, mapping to 3GPP MCPTT floor control (TS 24.380; VERIFY the details) and to LMR-style channel access, and why the arbiter must be centralised per talkgroup.

**Q14 — Story (design).** *Defend the priority, preemption and timer policy.* Cover: strictly-higher-preempts, emergency not preempting emergency, revoked talker not re-queued (and notified), starvation of low priority and how to bound it (ageing), the talk-burst timer for stuck PTT, priority validation range, and the known limitation about re-prioritising queued users.

**Q15 — Story (design).** *Idempotent StartCall and the choice of status codes.* Cover: client idempotency key, the lost-response scenario, AlreadyExists vs Ok, capacity (`ResourceExhausted`) semantics, what each code tells the client to do, and API evolution (adding fields to the proto without breaking old clients).

**Q16 — Story (design).** *Deadlines, retries and backoff.* Cover: why every call has a deadline and how deadlines propagate, `Unavailable` vs `DeadlineExceeded` retry rules, the numbers (100 ms … 5 s cap, 11.3 s over 7 retries), jitter, retry budgets/circuit breakers to prevent retry storms, and the interplay with idempotency.

**Q17 — Story (design).** *Why a single-threaded hermetic core with a thin adapter?* Cover: testability (17 tests, no network), determinism (injected time), the lock granularity choice (one mutex vs per-call vs an actor/strand), never doing blocking I/O under the lock, TSan in CI, what the untested adapter risks (and how you'd close the gap: an integration test with an in-process server).

**Q18 — Etherstack / integration.** Narrate one PTT press end to end, naming every layer: radio frame (M1) → modem (M3) → RTP + jitter buffer (M2) → gateway call table → floor request/grant → audio forwarded → stats stream (`loss_pct`, `ber`, `jitter_ms`) → release. List what you would add before shipping (TLS/mTLS and per-call authorisation, metrics export, persistence/recovery of calls, a timer thread with multi-subscriber fan-out, rate limiting) and one sentence on why the CI uses only synthetic traffic and never live public-safety reception.

## §6 Self-assessment

You are done when you can, without notes:

* [ ] Recite the floor decision table and the grant order rule, and trace a preemption.
* [ ] Explain the talk-burst timer's `≥`, the reset on grant and the idle no-op.
* [ ] Give the status code for each validation case and justify idempotent `StartCall`.
* [ ] Compute loss %, BER and the smoothed jitter by hand (25 %, 0.005, 0.87890625).
* [ ] Compute the backoff series (100 … 3200, 5000) and state the retry rules.
* [ ] Tell the "why a hermetic core" and "why idempotency" stories in 90 seconds with numbers.
* [ ] `ctest` shows all `solution.*` green, ASan/UBSan clean, `student.*` red until you implement.

**VERIFY list (not tested here):**

1. The gRPC adapter (`code/grpc/`) was **never compiled** here: package names, CMake targets, and API usage are unverified.
2. gRPC status-code numbers (the core pins 3, 4, 5, 6, 8, 9, 14; confirm against `grpc/status_code_enum.h`) and the recommended handling of `RESOURCE_EXHAUSTED` in gRPC retry guidance.
3. 3GPP MCPTT floor-control behaviour (TS 24.380) and priority/override semantics versus this simplified controller.
4. Default gRPC deadline/keepalive/flow-control behaviour in your version.
5. The retry/backoff constants (100 ms base, 5 s cap) are teaching values, not recommendations; production values come from measurement.
6. TSan/ASan cannot be combined in one binary; run them separately.
