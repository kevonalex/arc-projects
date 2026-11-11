# Pack 08/03 — FreeRTOS concepts: tasks, queues, semaphores vs mutexes, priority inversion (Mars Pathfinder) and priority inheritance

> **SAFETY.** A deterministic **tick-based simulation** of a preemptive fixed-priority scheduler — no real RTOS, no hardware, no radio. FreeRTOS API names and behaviours below are from memory:
> **VERIFY** against the FreeRTOS documentation for your version before relying on a detail. Mars Pathfinder facts are from the public post-mortems (**VERIFY** details).

---

## 1. Why this matters (Etherstack context)

Radio and gateway firmware is a set of concurrent activities with different deadlines — an ISR hands samples to a DSP task, a protocol task builds frames, a logger writes flash. An RTOS is how you give each activity a **priority** and a
**synchronisation tool**. The classic interview trap: *"what's the difference between a mutex and a binary semaphore?"* and *"explain priority inversion"*. A candidate who can run the Mars Pathfinder timeline on a whiteboard stands out.

## 2. Prerequisites

Pack 08/01 (volatile ≠ atomic) and 08/02 (ISRs, timers). C++17/GoogleTest in WSL with sanitizers.

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
```

## 3. Concept build-up

### 3.1 Tasks, priorities and the tick — the model in this pack

A **task** is an independent thread of execution with its own stack and a **priority**. On a single core only one task runs at a time. FreeRTOS uses **fixed-priority preemptive scheduling**: *the highest-priority task that is ready always runs*; a
higher-priority task that becomes ready **preempts** the running one immediately. Tasks of equal priority share the CPU round-robin on each tick (`configUSE_TIME_SLICING`, VERIFY).
**FreeRTOS priority convention: higher number = more urgent** — the *opposite* of Cortex-M interrupt priorities (pack 08/02). Task states: **Running, Ready, Blocked** (waiting for a queue/semaphore/delay/mutex), **Suspended**.
Time advances in **ticks** (a timer interrupt every `1/configTICK_RATE_HZ`, e.g. 1 ms). In this pack's simulator a task is a script of steps — `Run n` (use the CPU for n ticks), `Lock m`, `Unlock m`, `Delay n` — and the engine (given) steps tick by tick, asking **your** policy:
`pick_next(now)` → which ready task runs (highest effective priority; ties → lowest index; `-1` = idle), where *ready* means: not done, released (`now ≥ start`), not sleeping (`now ≥ wake`), not blocked.
Example `Engine.DelayAndIdleTicks`: a task that `Delay 3` then `Run 2` produces the trace `-1 -1 -1 0 0` — three idle ticks, then two running — finish at tick 5.

### 3.2 Queues — the safe way to pass data

A **queue** is a bounded FIFO that **copies** items in and out (`xQueueSend`, `xQueueReceive`; `…FromISR` variants inside interrupts). Blocking behaviour: a sender facing a full queue (or a receiver of an empty one) can **block with a timeout** — the scheduler runs something else.
In the model, `send` returns `false` when full and `receive` returns `false` when empty (non-blocking: *where the real call would block*). `BoundedQueue<int>(2)`: send 10, 20 OK; send 30 fails; receive → 10; send 30 OK; receive → 20, 30; empty → fail.
*Why copy and not pass pointers?* The sender's variable may disappear (stack) or change; copying decouples lifetimes — at the cost of copy time (so for big payloads send a **pointer to a buffer from a pool** and define who frees it). A queue also gives you **flow control**: a full queue throttles a producer.

> **Misconception box — "a queue is just a shared array".** A queue is **thread-safe, blocking and ordered**; a shared array needs your own locking and gives no wake-up.

### 3.3 Semaphores — signalling, not protecting

A **binary semaphore** is a flag with *blocking*: **give** sets it, **take** waits for it and clears it. Classic use: an ISR gives, a task takes ("data ready"). A **counting semaphore** counts events (up to a max) or resources.
Properties tested in `Semaphore`: `Semaphore(0,1)`: `take` fails (nothing signalled); `give` OK; a **second `give` while still pending fails** — the binary semaphore *absorbed* the second event: **events can be lost**; `Semaphore(0,4)`: four gives succeed, the fifth fails; `count()` = 4.
**No ownership:** *any* task or ISR may give; the taker is not tied to the giver. That is exactly why it's good for **signalling** and bad for **mutual exclusion**.
(FreeRTOS also offers **task notifications**, a lighter binary-semaphore replacement — VERIFY.)

### 3.4 Mutexes — protecting a shared resource

A **mutex** (MUTual EXclusion) guards a critical section: **lock, use, unlock**. It differs from a semaphore in three ways:

| | Binary semaphore | Mutex |
|---|---|---|
| Purpose | **signal** an event | **protect** a resource |
| Ownership | none — anyone can give | **owner** = the task that locked; **only the owner may unlock** |
| Priority inheritance | no | **yes** (FreeRTOS mutexes implement it — VERIFY; binary semaphores don't) |
| Use from an ISR | yes (`GiveFromISR`) | **no** — an ISR can't "own" (it isn't a task) |

`Mutex`: lock(7) OK, `owner() == 7`; lock(8) fails (taken); lock(7) again **fails** (non-recursive; a *recursive* mutex is a separate type); `unlock(8)` fails (not the owner — a semaphore would let it); `unlock(7)` OK, owner back to −1.
*Why ownership matters:* it tells the scheduler **who is holding the lock**, which is what priority inheritance needs (§3.6) — the key fact that makes the mutex/semaphore distinction *real* rather than a naming convention.

> **Misconception box — "a mutex is a binary semaphore initialised to 1".** Same mechanics, different **semantics**: ownership and inheritance. Using a semaphore as a mutex gives you no inversion protection and lets a buggy task "unlock" someone else's lock.

### 3.5 Priority inversion — the Mars Pathfinder timeline

Setup (`pathfinder()` in the test): three tasks, mutex 0 = the **information bus** lock. Priorities: **H = 3, M = 2, L = 1**.
`L` (released t=0): lock, run 4, unlock. `H` (t=1): lock, run 1, unlock. `M` (t=2): run 6 (CPU-bound, no lock).

**Without inheritance** — trace (task per tick; `L=2, M=1, H=0` as indices): `L L M M M M M M L L H`:
| tick | 0 | 1 | 2 | 3–7 | 8–9 | 10 |
|---|---|---|---|---|---|---|
| runs | L | L | M | M | L | H |
| why | L locks the bus | H arrives, **tries to lock → blocks**, L continues | M arrives: higher than L → **preempts L** | M hogs the CPU | M ends; L finally finishes its critical section | L unlocks → H runs 1 tick |

Finish times: **M 8, L 10, H 11**. H needed **1 tick of CPU** but took `11 − 1 = 10` ticks end-to-end — and **M, lower priority, finished before H**. That is **priority inversion**: a high-priority task effectively runs at a *lower* priority than a task that has nothing to do with it.
*Why is that worse than ordinary blocking?* If H must wait for L's critical section, that wait is **bounded** (L's run time = 4). But M can *preempt L* and extend the wait for as long as M runs, and with several medium tasks the delay is **unbounded**.

**History (VERIFY):** July 1997, **Mars Pathfinder** (VxWorks RTOS). A low-priority meteorological task held the **information-bus mutex**; the high-priority bus-management task blocked on it; medium-priority communications work preempted the low task; the bus task missed its deadline; a **watchdog**
declared a fault and **reset** the computer — repeatedly. Engineers reproduced it on the ground and **uploaded a patch enabling priority inheritance** on that mutex (a flag in the OS's semaphore creation) — fixed from Earth.
In this pack the watchdog is `DeadlineOfSixTicks`: H must finish within **6 ticks of release**; without inheritance H takes 10 → **reset**; with inheritance 4 → fine.

### 3.6 Priority inheritance — the fix

**Rule:** while a task owns a mutex that a higher-priority task is blocked on, the owner **temporarily runs at the highest priority of its waiters**. When it unlocks, the boost ends. In the code: `effective_priority(sc, task)` = base priority, or if `sc.inheritance`, the **max** over all tasks **blocked on a mutex this task owns** (and not done) of *their* effective priority — **transitive** (recursion with a depth limit of 16).
With inheritance the trace is `L L L L H M M M M M M`: at t=1 H blocks, L is boosted to 3 and **cannot be preempted by M**; L finishes at 4, H runs at 4 (finish 5, response **4**), M runs last (finish 11). M waited — it was never entitled to run ahead of H.
**Transitive chain** (`TransitiveChain`): T3(3) waits on B owned by T2(2), which waits on A owned by T1(1): T1 must run at 3, otherwise T2 can't get A, T3 can't get B. **Finished waiters** don't boost (a task that's done isn't waiting). **Deadlock** (T0 waits B owned by T1; T1 waits A owned by T0): priority inheritance **does not fix it**, but the implementation must **terminate** (hence the depth limit) rather than recurse forever.
**Alternatives:** **priority ceiling** (the mutex has a ceiling = highest priority of any task that uses it; the owner runs at the ceiling immediately — prevents deadlock by construction, VERIFY on your RTOS); **don't share locks across priorities** (use a gatekeeper task and a queue); **disable preemption briefly** (cheap, blunt).

> **Misconception box — "inheritance prevents deadlock / makes the system real-time".** It bounds *priority inversion*; it does nothing for deadlock, long critical sections, or CPU overload.
> **Misconception box 2 — "just give H a higher priority".** H already had the highest priority — and that is the whole problem.

## 4. Worked examples

**(a) Compute the response without inheritance:** `finish(H) − start(H) = 11 − 1 = 10`; with inheritance: `5 − 1 = 4`; deadline 6 → reset vs safe.
**(b) `pick_next` with a tie:** tasks 1 and 2 both prio 5 → task 1 (lowest index). Block task 1 → task 2. Task 2 sleeping until tick 10: at `now = 3` the only ready is task 0; at `now = 10` task 2 is ready again.
**(c) Which primitive?** ISR → task "frame received": binary semaphore (or task notification). Two tasks write one SPI bus: mutex. Frame buffers from DSP to protocol: queue of pointers + buffer pool. Count of free DMA descriptors: counting semaphore.

## 5. Questions

1. **Q1 (Explain)** — What does "fixed-priority preemptive scheduling" mean? Which task runs among equal priorities? (W 3.1)
2. **Q2 (Trace)** — Task `T` (`Delay 3`, `Run 2`) starts at 0, alone: give the tick trace and finish time. (W 3.1)
3. **Q3 (Explain)** — Queue vs shared array vs pointer-passing: pick one for a 160-sample audio frame; justify. (W 3.2)
4. **Q4 (Trace)** — `BoundedQueue(2)`: show every call result for send 10, 20, 30, recv, send 30, recv, recv, recv. (W 3.2)
5. **Q5 (Explain)** — Binary vs counting semaphore: when can an event be lost? Show with an ISR firing twice before the task runs. (W 3.3)
6. **Q6 (Compare)** — Mutex vs binary semaphore: three differences and one scenario where using the wrong one causes a bug. (W 3.4)
7. **Q7 (Trace)** — Run the Pathfinder timeline **without** inheritance by hand; give the per-tick trace, each finish time and H's response. (W 3.5)
8. **Q8 (Explain)** — Why is priority inversion *unbounded* with M and only *bounded* without it? (W 3.5)
9. **Q9 (Trace)** — Same with inheritance; at which tick does the boost start and end? (W 3.6)
10. **Q10 (Calculate)** — `effective_priority` for the three-task transitive chain; for a finished waiter; for the deadlock cycle. (W 3.6)
11. **Q11 (Explain)** — Tell the Pathfinder story in 60 seconds, including why a watchdog reset occurred and how it was fixed. (W 3.5)
12. **Q12 (Design)** — Alternatives to priority inheritance: ceiling, gatekeeper task, disabling preemption — pros/cons for a radio-audio pipeline. (W 3.6)
13. **Q13 (Design)** — Which primitive for: ISR→task event; two tasks sharing SPI; DSP→protocol frame hand-off; N free buffers? (W 3.2–3.4)
14. **Q14 (Explain)** — Why can't an ISR take a mutex? What does FreeRTOS provide instead (`…FromISR`, task notification)? (W 3.4)
15. **Q15 (Etherstack)** — In the audio pipeline of pack 08/04 (codec_out 3, dsp_resample 2, logger 1 sharing a mutex): predict the deadline miss and the fix before you run it. (W 3.5–3.6)

## 6. Self-assessment

- [ ] Run the Pathfinder timeline on a whiteboard both ways.
- [ ] State mutex vs semaphore: ownership, inheritance, ISR use.
- [ ] Explain why inversion is unbounded because of the medium task.
- [ ] Say precisely what inheritance does *not* solve (deadlock).
- [ ] Remember: FreeRTOS task priority — higher number more urgent; Cortex-M IRQ — lower number more urgent.

**VERIFY list:** FreeRTOS time-slicing default; mutex priority inheritance behaviour (single vs chained inheritance) in your kernel version; recursive mutex API; task notification semantics; Pathfinder details (VxWorks, information bus, watchdog, remote patch, dates);
ceiling protocol availability on your RTOS; whether `xSemaphoreGiveFromISR` on a binary semaphore can coalesce events (yes: this pack models it).
