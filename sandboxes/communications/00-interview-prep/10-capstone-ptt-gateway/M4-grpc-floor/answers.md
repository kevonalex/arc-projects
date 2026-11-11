# Capstone M4 — Answers (gRPC control plane, floor control)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic traffic only. **VERIFY** = check against the standard/library before quoting. The gRPC adapter is an **untested skeleton**.

For the **Story** questions (Q13–Q17) there is no single right answer: a *model answer* lists the points a strong answer must hit. Rewrite it in your own words with your own numbers.

---

## Q1 — Control plane vs media plane

**Answer.**

| Plane | Carries | Transport | Where it lives |
|---|---|---|---|
| Media | 20 ms μ-law RTP voice (M2), radio frames (M1/M3) | UDP multicast (RTP) on the IP side; 4FSK on the air side | modem → frame parser → RTP packetiser → jitter buffer |
| Control | `StartCall`/`EndCall`, floor `REQUEST/RELEASE` and `GRANTED/QUEUED/REVOKED/TIMED_OUT/IDLE`, `WatchStats` stream (`loss_pct`, `ber`, `jitter_ms`) | gRPC over HTTP/2 + TCP (TLS beyond localhost) | `GatewayService` (call table + a `FloorController` per call + stats) |

Voice is not carried over gRPC: TCP retransmission and in-order delivery add head-of-line blocking and latency, and a late voice packet is worthless (M2 Q13). Control messages are small, must be reliable and ordered, and benefit from typed contracts, deadlines and status codes.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Send audio as a gRPC stream, it's simpler" | One protocol. | Reliable-ordered transport harms real-time voice; no multicast. |
| "Floor control belongs in the radio" | Radios have PTT. | On an IP gateway several radios/consoles contend; a central arbiter per talkgroup is needed. |

## Q2 — Floor trace

**Answer.** `request(a,0)` → **Granted**; `request(a,0)` → **AlreadyHolder**; `request(b,0)` → **Queued**; `request(b,0)` → **Queued** (idempotent, no new event); `release(a)` → **true** (Released a, Granted b); `release(b)` → **true** (Released b, then Idle); `release(b)` → **false**. Events drained at the end: `Granted(a)`, `Queued(b)`, `Released(a)`, `Granted(b)`, `Released(b)`, `Idle`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| A second `Queued(b)` event | Second call = second event. | The duplicate request is a no-op (idempotent). |
| `release(b)` third time returns true | "Release is harmless". | b no longer holds or waits: `false` (→ `FailedPrecondition`). |

## Q3 — Priority order

**Answer.** `queue_order() = [p3, p3b, n1, n2, n3]` (priority 3 before 0; within a priority, arrival order). `release(h)` → events `Released(h)`, `Granted(p3)`; holder `p3`. `release(p3)` → holder **`p3b`** (next highest priority, even though n1 arrived earlier).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `[n1, n2, p3, n3, p3b]` | Plain FIFO. | Priority outranks arrival order. |
| `[p3b, p3, …]` | Newest first. | Within a priority the *oldest* waits longest and goes first. |

## Q4 — Preemption

**Answer.** `request(b,1)` → **Queued** (equal priority doesn't preempt); `request(c,7)` → **Preempted** (7 > 1): events `Revoked(a)`, `Granted(c)`; queue `[b]`; holder `c`; `request(d,7)` → **Queued** (7 is not strictly higher than 7). `a` is not re-queued: re-queueing would hand the floor back to `a` as soon as the emergency ends, and under churn an emergency would always wait behind the people it displaced; `a` is *told* (`Revoked`) and must ask again.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `d` preempts `c` | Both emergencies. | Strictly higher only; otherwise emergencies steal from each other. |
| `a` is queued first | "Fair". | Revoked users re-request; the queue holds only people who asked. |

## Q5 — Timer

**Answer.** `tick(600)`: held 600, nothing. `tick(400)`: held 1000 ≥ 1000 → `TimedOut(a)`, `Granted(b)`, b's timer reset to 0. `tick(999)`: nothing. `tick(1)`: 1000 → `TimedOut(b)`, `Idle`. `tick(100000)` with no holder: nothing. With a 100 ms tick and a limit of 1050 ms the holder is cut at the first tick ≥ 1050, i.e. **1100 ms**: timer granularity adds up to one tick of overrun, so choose the tick small relative to the allowance. Injected time makes the controller deterministic, so tests run instantly with exact numbers and can't be flaky; the adapter decides how ticks are produced.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Timeout at 1001 ms ("> limit") | Natural reading of "exceeds". | The pinned rule is `≥` (limit reached). |
| New talker inherits the elapsed time | Shared counter. | A grant resets `held_ms` to 0. |

## Q6 — Status codes

**Answer.**

| Request | Code |
|---|---|
| empty `call_id` | InvalidArgument |
| talkgroup 0 / 65536 | InvalidArgument |
| empty caller | InvalidArgument |
| identical repeat | Ok |
| same id, talkgroup 101 | AlreadyExists |
| same id, caller "mallory" | AlreadyExists |
| new call, table (2) full | ResourceExhausted |
| identical existing call, table full | Ok |
| `end_call` twice | Ok, then NotFound |

Order: argument checks first (they depend only on the request, so an invalid request is `InvalidArgument` whatever the server's state), then the state-dependent checks (existing id → Ok/AlreadyExists, then capacity). This gives clients a stable contract: fix the request vs. back off.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Full table rejects the identical repeat | "Table is full." | The call already exists; rejecting would make retries after a lost response fail. |
| `FailedPrecondition` for the conflicting id | State-related. | `AlreadyExists` is the precise code for a conflict on creation. |

## Q7 — Lost response

**Answer.** (a) Idempotent: the retry (same id, same parameters) → `Ok`; the client now knows its call exists; state unchanged, no duplicate. (b) Non-idempotent: retry → `AlreadyExists`; the client can't tell whether *its* earlier attempt created the call or someone else did; it must issue a `GetCall`/list or abandon and leak the call. The id is client-chosen because it's an **idempotency key**: the client can reuse it across retries (a server-generated id would be unknown to the client after the lost response). The conflict rule (`AlreadyExists` when parameters differ) stops two different clients colliding silently.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Server generates ids | Avoids collisions. | Lost response ⇒ the client can't retry safely. |
| "gRPC retries are exactly-once" | Marketing. | Delivery is at-most-once per attempt; effects need idempotency. |

## Q8 — Stats arithmetic

**Answer.** Frames: rx(1000, 10, 16.0) → packets 1, jitter `0 + (16 − 0)/16 = 1.0`; rx(1000, 0, 0.0) → packets 2, jitter `1.0 − 1.0/16 = 0.9375`; lost → lost 1, jitter unchanged (0.9375); rx(1000, 5, 0.0) → packets 3, jitter `0.9375 × 15/16 = 0.87890625`. Totals: packets **3**, lost **1**, `loss_pct = 100 × 1/4 = 25 %`, bits **3000**, errors **15**, BER **0.005**. A lost frame has no arrival, so it has no jitter sample; feeding 0 would bias the estimate. Derived values are computed in `snapshot()` so they cannot disagree with counters (and the zero-denominator case is handled once).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| loss = 1/3 = 33 % | Lost / received. | Denominator = expected = received + lost. |
| BER 0.015 | Dividing by 1000. | Divide by *all checked bits* (3000), and lost frames add none. |

## Q9 — Backoff

**Answer.** `min(5000, 100 × 2^n)`: attempts 0–8 → **100, 200, 400, 800, 1600, 3200, 5000, 5000, 5000**. Cumulative wait for the first 7 retries (attempts 0–6): `100 + 200 + 400 + 800 + 1600 + 3200 + 5000 = 11 300 ms`. Overflow: double in a 64-bit variable only while below the cap, so the loop stops after a few steps (attempt 1000 still returns 5000). Cap below base (cap 50, base 100): the loop never doubles; the result is clamped → **50**. Add random jitter (e.g. uniform in [0, delay] or ±50 %) so clients that failed together don't retry in lockstep.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `100 << attempt` | Simplest. | Overflows (UB) for attempt ≥ 32/64. |
| Cap applies only to the sum | Total-time thinking. | The cap bounds each wait. |

## Q10 — Retry matrix

**Answer.** `Unavailable`: true/true. `DeadlineExceeded`: false (non-idempotent) / true (idempotent). `Ok`, `InvalidArgument`, `NotFound`, `AlreadyExists`, `ResourceExhausted`, `FailedPrecondition`: false/false. `DeadlineExceeded`: the server may have performed the work before the deadline hit, so only idempotent RPCs may be repeated. `ResourceExhausted` here: the server said "full" — an immediate retry adds load; the client should surface it or retry with its own long backoff (policy choice; some gRPC guidance retries it with backoff — VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Retry everything but InvalidArgument" | Maximises success. | Retrying `AlreadyExists`/`FailedPrecondition` can't succeed; retrying non-idempotent timeouts duplicates work. |

## Q11 — Mapping `Denied`

**Answer.** `Denied` conflates (1) a malformed request (empty user, priority outside 0..7) and (2) queue capacity. Better: (1) → `InvalidArgument` (client bug: fix, don't retry); (2) → `ResourceExhausted` (busy: retry later with backoff, or tell the user "channel busy"). Core change without breaking the tests: keep `request()` returning `FloorResult::Denied` (tests pin that) and add `request_ex()` (or `last_denial()`) returning `{result, reason}` with `reason ∈ {InvalidRequest, QueueFull}`; have the service use the richer API. Adding a second `Denied*` enumerator would break the tests that compare against `Denied` for the queue-full case.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Always `PermissionDenied`" | Name matches. | That code means authorisation failure, not capacity or bad input. |

## Q12 — Races, deadlocks, TSan

**Answer.** *Race:* a gRPC thread inside `floor_request` pushes onto `queue_`/`events_` (std::vectors) while the timer thread runs `tick()` which reads/erases `queue_`, writes `holder_`/`held_ms_` → data race, vector corruption, UB; also `calls_` (a `std::map`) inserted by `start_call` while `tick()` iterates it (iterator invalidation). *Deadlock/stall:* holding the mutex across `stream->Write()` — if a client is slow, the write blocks on HTTP/2 flow control; every other RPC and the timer thread wait for the lock; if the write completion needs the lock it's a true deadlock. *Pattern:* lock → copy the events/stats you need → unlock → write. TSan (`-fsanitize=thread`) finds the race; ASan/UBSan don't, and TSan can't be combined with ASan in one binary: run separately.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "std::vector is thread-safe for push_back" | Containers feel safe. | No standard container allows concurrent mutation. |
| "Use a lock per method" | Fine-grained. | The invariants span several fields (holder, queue, events), so per-field locks permit torn state. |

## Q13 — Story: why floor control

**Model answer — hits:** (1) PTT is half-duplex: one talker per talkgroup; with two simultaneous talkers a gateway produces overlapping RTP streams that receivers can't separate, and in public safety the *important* transmission (emergency) can be lost. (2) In conventional radio the channel itself is the arbiter (capture effect, listening before talk); an IP gateway has no shared medium, so arbitration must be explicit, central per talkgroup, and low-latency (the grant must reach the radio in tens of ms). (3) 3GPP MCPTT has a standardised floor-control protocol (TS 24.380; VERIFY) with queueing, priority and override; this controller models the same concepts in miniature (request/grant/queue/revoke, emergency priority). (4) Requirements: deterministic arbitration (no races), notifications to everyone affected, timeout protection from stuck PTT, auditability (event log). *What I'd change:* persistence/recovery after a gateway restart, ageing for starvation, per-user authorisation.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "A mutex per talkgroup is enough" | One owner at a time. | No queueing, priority, notification or timeout: those are the requirements. |

## Q14 — Story: priority, preemption, timer

**Model answer.** *Strictly higher preempts:* otherwise equal priorities (especially two emergencies) would steal from each other. *Revoked, not re-queued, notified:* the displaced talker must know to stop (`Revoked`) and re-request; silent loss of the floor = the user keeps talking into the void. *Starvation:* strict priority can starve low priority under steady high-priority load; mitigations: ageing (raise priority with waiting time), per-priority quotas, talk-burst limits (already present). *Timer:* stuck PTT (handset in a bag) would otherwise block the whole talkgroup; the max-talk time bounds this; `≥` semantics and reset on grant; time injected for determinism. *Validation:* priorities 0–7 only, empty users denied. *Limitation to admit:* a queued user re-requesting at higher (but not preempting) priority isn't reprioritised; fix: update the waiter's priority in place and re-sort.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Preemption is always good in an emergency" | Safety. | Without limits, a stuck emergency PTT blocks everyone; the timer applies to emergencies too. |

## Q15 — Story: idempotent StartCall and status codes

**Model answer.** *Problem:* the network can lose the response after the server acted; a client that can't tell "created" from "already exists by someone else" can't recover. *Design:* client-chosen `call_id` is the idempotency key; identical repeats return `Ok`; conflicting reuse is `AlreadyExists`; capacity applies only to *new* calls (so an identical repeat succeeds even when full). *Status codes as a contract:* `InvalidArgument` (fix the request) < `FailedPrecondition` (state forbids it now) < `AlreadyExists` (conflict) < `ResourceExhausted` (capacity; back off) < `Unavailable` (transient; retry); clients can implement generic handling from the code alone. *Evolution:* new optional proto fields with new numbers, never reuse numbers; unknown fields ignored. *What I'd add:* idempotency-key expiry and a `GetCall` RPC.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Return AlreadyExists for any repeat" | Seems clear. | Breaks safe retry. |

## Q16 — Story: deadlines, retries, backoff

**Model answer.** *Deadlines:* every call has one (otherwise a hung server hangs the caller); deadlines propagate so servers abandon work for callers that have gone; choose them from the latency budget (control calls ≈ 100s of ms; streams use per-message liveness). *Retry rules:* `Unavailable` is safe to retry (not processed); `DeadlineExceeded` only for idempotent RPCs (`StartCall` is, thanks to §3.4); everything else is a verdict. *Numbers:* 100 ms base, 5 s cap: 100 … 3200, 5000; 7 retries ≈ 11.3 s; the cap bounds each wait. *Pitfalls:* retry storms (add jitter and a retry budget/circuit breaker), retries multiplying across layers (3 layers × 3 retries = 27 attempts), retrying non-idempotent RPCs. *What I'd change:* configure retries via gRPC service config rather than hand-rolled loops (VERIFY for your version).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Retry immediately, it's faster" | Low latency. | Hammering a struggling server prolongs the outage. |

## Q17 — Story: hermetic core, thin adapter

**Model answer.** The core has no sockets, threads or clocks: 17 tests run in milliseconds, deterministically, under ASan/UBSan; behavioural rules (floor, idempotency, retry policy) are specified by tests, not by whichever gRPC version is installed. The adapter only translates messages to calls and `Status` to `grpc::Status` (numeric codes match, so a cast); it is small enough to review by eye. *Threading:* single-threaded core + one mutex in the adapter = simple correctness; alternatives: per-call locks (more concurrency, harder invariants), or an actor/strand serialising everything (no locks, queue latency). Never block under the lock. Run TSan on the adapter. *Risk:* the adapter is untested here; close it with an in-process server integration test (start a server on an ephemeral port, call it from a client, assert on status codes and stream contents), plus a fuzz of the proto messages.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Mock gRPC and test the logic through it" | Tests the wiring too. | Couples logic tests to a framework; slower and flakier than testing the core directly. |

## Q18 — Whole-gateway narrative

**Answer (outline).** Handset keys up → 4FSK burst → gateway modem finds sync, estimates CFO, demodulates (M3) → M1 parse repairs bits (`payload_corrections`) or reports `CrcFail` → frame becomes 160 B μ-law → RTP packet (marker on the first packet of the talk-spurt) to `239.192.(tg>>8).(tg&255)` (M2) → jitter buffer on the receiving side → playout. Control: the call exists via `StartCall`; the talker's `FloorChannel` sends `REQUEST`; the controller returns `GRANTED` (or `QUEUED`/`REVOKED`); only the holder's audio is forwarded; `WatchStats` streams `loss_pct`, `ber` (from M1 corrections + M3), `jitter_ms` (M2); release/timeout → next grant or `Idle`. Before shipping: TLS/mTLS and per-call authorisation, metrics export, call-state recovery, timer thread + multi-subscriber fan-out, rate limiting. CI is synthetic-only (seeded channel simulator, deterministic arrivals, injected time): live public-safety capture is legally restricted (UK Wireless Telegraphy Act 2006), unnecessary and not reproducible.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Forward everyone's audio, let the listeners sort it out" | Simplest gateway. | Defeats half-duplex semantics and wastes emergency priority. |
