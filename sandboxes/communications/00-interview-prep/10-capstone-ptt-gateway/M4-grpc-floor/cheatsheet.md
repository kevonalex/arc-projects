# Capstone M4 cheatsheet — gRPC control plane and floor control (every row cites the worksheet)

Synthetic traffic only. The gRPC adapter is an untested skeleton. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Control plane (gRPC: StartCall/EndCall/floor/stats) vs media plane (RTP voice); gRPC never carries voice | §1, Q1 |
| PTT is half-duplex: one talker per talkgroup; floor control arbitrates | §3.1, Q13 |
| Invalid user or priority outside 0..7 → Denied; idle → Granted; holder again → AlreadyHolder | §3.1 |
| Strictly higher priority → Preempted: Revoked(old), Granted(new); old not re-queued | §3.1, Q4 |
| Equal priority never preempts (incl. emergency vs emergency); already queued → Queued, no duplicate | §3.1, Q4 |
| Queue full → Denied; priorities: 0 normal, 7 emergency | §3.1 |
| Grant order = highest priority first, oldest first within a priority | §3.1, Q3 |
| Holder release: Released(user) then Granted(next) or Idle; waiter release = silent withdrawal | §3.2, Q2 |
| Release by a non-participant → false → FailedPrecondition | §3.2, §4(d) |
| Talk-burst timer: held ≥ max → TimedOut(holder), grant next; new grant resets; idle no-op | §3.3, Q5 |
| Time injected via tick() ⇒ deterministic tests | §3.3, Q5 |
| Known limitation: queued user re-requesting higher (not preempting) isn't re-prioritised | §3.3, Q14 |
| start_call validation: id 1–64 chars, talkgroup 1–65535, caller non-empty → InvalidArgument | §3.4, Q6 |
| Identical repeat → Ok (idempotent); same id, different params → AlreadyExists | §3.4, Q7 |
| Full table → ResourceExhausted only for NEW calls | §3.4, Q6 |
| end_call: Ok, then NotFound | §3.4 |
| Client-chosen call_id = idempotency key (lost-response retry) | §3.4, Q7 |
| Code numbers: Ok 0, InvalidArgument 3, DeadlineExceeded 4, NotFound 5, AlreadyExists 6, ResourceExhausted 8, FailedPrecondition 9, Unavailable 14 | §3.4 |
| Stats: store counters, derive loss_pct and ber in snapshot() | §3.5, Q8 |
| received: packets++, bits, errors, jitter += (sample − jitter)/16; lost: lost++ only | §3.5, Q8 |
| Test numbers: loss 25 %, BER 0.005, jitter 1.0 → 0.9375 → 0.87890625 | §3.5, Q8 |
| Backoff = min(cap, base·2^attempt): 100, 200, 400, 800, 1600, 3200, 5000 …; 7 retries = 11.3 s | §3.6, Q9 |
| No overflow at attempt 1000; cap below base clamps to the cap; add jitter | §3.6, Q9 |
| Retry: Unavailable yes; DeadlineExceeded only if idempotent; all else no | §3.6, Q10 |
| Every client call carries a deadline | §3.6, Q16 |
| gRPC shapes: unary, server stream (WatchStats), bidi (FloorChannel) | §3.7 |
| proto3: never reuse/renumber field numbers; add new fields with new numbers | §3.7, Q15 |
| Core single-threaded; adapter holds one mutex; never block (stream write) under the lock | §3.7, Q12 |
| Race: gRPC threads vs timer thread on holder_/queue_/events_/calls_; TSan finds it, ASan doesn't | Q12 |
| Denied → ResourceExhausted conflates invalid input and queue full (better: split) | §4(d), Q11 |
| Insecure credentials = localhost only; TLS/mTLS beyond | §3.7, Q18 |
