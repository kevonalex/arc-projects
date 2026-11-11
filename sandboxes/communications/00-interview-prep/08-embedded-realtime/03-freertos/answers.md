# Pack 08/03 — Answers (FreeRTOS concepts, priority inversion)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Tick-based simulation, not a real RTOS. **VERIFY** FreeRTOS API details for your version.

---

## Q1 — Explain: fixed-priority preemptive

**Answer.** Each task has a fixed priority; the scheduler always runs the **highest-priority ready** task; when a higher-priority task becomes ready (timer expiry, semaphore given, queue item arrives) it **preempts** the current one at once. Equal priorities: round-robin by tick (FreeRTOS time slicing, VERIFY); in this sim ties go to the lowest task index.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Higher priority tasks run to completion first" | Sounds like sorting. | They run only while *ready*; when they block, lower ones run. |
| "Lower number = higher priority" | Cortex-M IRQs. | FreeRTOS: higher number = more urgent. |

## Q2 — Trace: Delay then Run

**Answer.** Ticks 0–2: idle (`-1 -1 -1`) because the task sleeps; ticks 3–4 task runs (`0 0`); finish = 5.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Task runs at tick 0 then sleeps" | Order of steps. | The first step is `Delay 3`: it sleeps first. |

## Q3 — Explain: queue vs array vs pointer

**Answer.** For 160 int16 = 320 bytes: a queue of **pointers/indices to a pool buffer** (zero-copy, bounded, blocking, flow-controlled) — copying 320 B per item is acceptable on a fast MCU but wastes bandwidth at 50 frames/s × multiple stages. A raw shared array needs hand-written locking and no wake-up. Define ownership: exactly one task owns a buffer at a time; the queue transfers ownership.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Pass the pointer to a stack variable" | Cheap. | Stack frame may be gone; dangling pointer. |

## Q4 — Trace: BoundedQueue(2)

**Answer.** send 10 → true; send 20 → true; send 30 → **false** (full); receive → true (10); send 30 → true; receive → 20; receive → 30; receive → **false** (empty).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "send 30 overwrites 10" | Ring-buffer habit. | A queue never overwrites silently (xQueueOverwrite is a separate API for length-1). |

## Q5 — Explain: lost events

**Answer.** Binary semaphore: ISR gives twice before the task takes → second give returns false/is absorbed: the task wakes **once** though two events occurred. Safe only if the task drains *all* pending work (e.g. read everything from the FIFO until empty) per wake-up. A counting semaphore (max N) remembers up to N events; beyond N they're lost too. If every event carries data, use a queue.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Semaphores queue every give" | Counting concept. | Binary caps at 1; counting caps at max. |

## Q6 — Compare: mutex vs binary semaphore

**Answer.** (1) **Ownership**: mutex unlock only by owner; semaphore any. (2) **Priority inheritance**: mutex yes; semaphore no. (3) **Purpose/ISR use**: mutex protects, not from ISRs; semaphore signals, ISR-safe. Bug example: two tasks share an SPI bus protected by a *binary semaphore*; a low-priority holder is preempted by a medium task → priority inversion (no inheritance) → the high-priority task misses its deadline.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "They are the same" | Same counting mechanics. | Different semantics: owner + inheritance. |
| "Mutex = faster" | Simpler. | Mutex is slightly *more* work (owner tracking, inheritance). |

## Q7 — Trace: without inheritance

**Answer.** `L L M M M M M M L L H` (indices 2 2 1 1 1 1 1 1 2 2 0).
t0 L locks and runs; t1 H arrives, tries to lock → blocks, so L continues; t2 M arrives (2 > 1) → preempts L, runs 6 ticks (t2–7); t8–9 L finishes its 4 ticks and unlocks; t10 H runs 1 tick. Finish: **M 8, L 10, H 11**. H response = 11 − 1 = **10** (needs 1).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "H preempts at t=1" | H has top priority. | H is *blocked on the mutex*, so it isn't ready. |
| "H finishes at t=10" | Off by one. | Finish tick is the tick after its last running tick: it runs in tick 10, finishes at 11. |

## Q8 — Explain: bounded vs unbounded

**Answer.** Without M, H waits only for L's critical section (≤ 4 ticks: bounded by its length — ordinary blocking, "priority *inversion* of bounded duration"). With M ready, M beats L, so L can't progress until **M is done or blocks** — the delay is bounded only by M's run time (and any other medium tasks): effectively unbounded relative to H's design.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Inversion = H waits for L" | Literal reading. | That's normal mutual exclusion; the *unbounded* part is M interfering. |

## Q9 — Trace: with inheritance

**Answer.** `L L L L H M M M M M M`. At t1 H blocks on L's mutex → L is boosted to 3 → at t2 M (2) cannot preempt L (3). L runs t0–3, unlocks at t4 (boost ends); H runs t4 (finish 5, response **4**); M runs t5–10 (finish 11). The boost starts at **t = 1** and ends at **t = 4**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Inheritance makes H preempt L immediately" | H is higher. | H still needs the mutex: only L can release it, so L must run — at H's priority. |
| "M should have run first, it's ready" | Ready at t=2. | At t2 L's effective priority is 3 > 2. |

## Q10 — Calculate: effective priorities

**Answer.** Chain (T1 owns A; T2 owns B and is blocked on A; T3 is blocked on B): `effective(T2) = 3` (T3 waits on its mutex B) and `effective(T1) = 3` (T2, which waits on T1's mutex A, now has effective priority 3). Finished waiter (prio 7, done): owner stays **1**. Deadlock cycle T0⇄T1: result ≥ 2 and **terminates** (depth limit 16 or visited-set) — no infinite recursion. Without inheritance: base priorities.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "T1 stays 1: only T2 waits on it" | One hop only. | T2's *effective* priority is 3 because T3 waits on it: inheritance is transitive. |
| "Inheritance resolves the deadlock" | Priorities rise. | A cycle can't be broken by priority. |

## Q11 — Explain: Pathfinder in 60 s

**Answer.** "July 1997, Mars Pathfinder, VxWorks. A low-priority meteorological task took the information-bus mutex; the high-priority bus manager blocked on it; a medium-priority communications task preempted the low one, so the bus manager waited too long; a watchdog saw it miss its deadline and reset the computer — repeatedly, losing data. JPL reproduced it on a replica, found the mutex had priority inheritance disabled, and uploaded a patch to turn it on. Lesson: mutexes must use priority inheritance or ceilings, and keep critical sections tiny." (VERIFY details.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "A software bug in the camera code" | Stories vary. | Scheduling/synchronisation config, not application logic. |

## Q12 — Design: alternatives

| Option | Pros | Cons | In an audio pipeline |
|---|---|---|---|
| Priority inheritance | transparent, automatic | only bounds inversion; chain handling varies | good default |
| Priority ceiling | no deadlock by construction, simple bound | must know all users up front, priority inflation | fits static pipelines |
| Gatekeeper task + queue | no shared lock at all | extra latency, one more task | best for logger/flash: logger owns the resource, others queue messages |
| Disable preemption/IRQs briefly | trivial, tiny critical sections | blocks *everyone*, hurts latency | only for a few instructions |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Raise the logger's priority" | Quick fix. | Moves the problem; starves other tasks and still allows inversion. |

## Q13 — Design: which primitive

**Answer.** ISR→task: binary semaphore / task notification (or queue if each event carries data). Two tasks on SPI: mutex (inheritance) or a gatekeeper. DSP→protocol frames: queue of buffer pointers + pool. N free buffers: counting semaphore.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Binary semaphore for SPI" | Same count. | No owner, no inheritance. |

## Q14 — Explain: mutex in ISR

**Answer.** A mutex belongs to a task: the kernel records the owner (for inheritance and ownership checks) and may block the caller. An ISR isn't a task: it can't block and has no priority in the task sense. Use `…FromISR` semaphore/queue/notification APIs, defer work to a task, and protect shared data with a short critical section or SPSC ring.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use a mutex with zero timeout in the ISR" | Non-blocking. | Still no owner semantics; unsupported/UB in FreeRTOS (VERIFY). |

## Q15 — Etherstack: pipeline prediction

**Prediction (before running).** Logger locks the bus at t0 and has 5 ticks of work; codec (high) arrives at t2 and blocks; DSP (medium, 10 ticks) arrives at t3 and preempts the logger. Codec waits until DSP finishes (t13) + logger's remaining 2 ticks → finishes t17 → response **15 > 8: deadline missed**. With inheritance the logger runs at codec's priority from t2: finishes t5, codec runs t5–6, **response 5 ≤ 8**. (The `pipeline_trace` demo in pack 08/04 prints exactly this.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The DSP task delays only itself" | It doesn't lock anything. | It delays the lock *holder* and therefore the codec. |
