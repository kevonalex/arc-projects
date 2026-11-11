# Pack 08/03 cheatsheet — FreeRTOS concepts (every line cites the worksheet section)

Tick-based simulation. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Fixed-priority preemptive: highest-priority READY task runs; newly ready higher one preempts | §3.1, Q1 |
| FreeRTOS task priority: higher number = more urgent (Cortex-M IRQ: lower = more urgent) | §3.1, Q1 |
| Task states: Running, Ready, Blocked, Suspended | §3.1 |
| Ready = not done, released, not sleeping, not blocked; tie → lowest index (sim) | §3.1, Q1 |
| Queue: bounded FIFO, copy in/out, can block with timeout; full → send fails, empty → receive fails | §3.2, Q3, Q4 |
| Pass pointers into a buffer pool for big payloads; define ownership | §3.2, Q3 |
| Binary semaphore = signal flag; second give while pending is absorbed (event lost) | §3.3, Q5 |
| Counting semaphore remembers up to max; no owner | §3.3, Q5 |
| Mutex: owner only may unlock; non-recursive; inheritance | §3.4, Q6 |
| Mutex ≠ binary semaphore (ownership, inheritance, ISR use) | §3.4, Q6 |
| ISR cannot take a mutex; use FromISR semaphore/queue/notification | §3.4, Q14 |
| Priority inversion: H blocked on lock held by L; M preempts L | §3.5, Q7 |
| Pathfinder trace w/o inheritance: L L M M M M M M L L H; finish M 8, L 10, H 11; H response 10 | §3.5, Q7 |
| Inversion is unbounded because M can run as long as it likes | §3.5, Q8 |
| Mars Pathfinder 1997: VxWorks, info-bus mutex, watchdog reset, patch enabling inheritance (VERIFY) | §3.5, Q11 |
| Inheritance: owner runs at max priority of tasks blocked on its mutexes; transitive | §3.6, Q10 |
| With inheritance: L L L L H M…; boost t1→t4; H finish 5, response 4; M finish 11 | §3.6, Q9 |
| Finished waiters don't boost; deadlock cycle must terminate (depth limit 16) | §3.6, Q10 |
| Inheritance does not fix deadlock or long critical sections | §3.6, Q10 |
| Alternatives: priority ceiling, gatekeeper task + queue, brief preemption disable | §3.6, Q12 |
| Which primitive: ISR event → semaphore/notification; shared bus → mutex; data → queue; N buffers → counting | §4(c), Q13 |
| Audio pipeline: codec resp 15 (miss, deadline 8) w/o; 5 with inheritance | Q15 |
