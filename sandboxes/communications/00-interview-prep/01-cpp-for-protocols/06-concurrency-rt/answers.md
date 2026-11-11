# Pack 06 answers - threads, atomics, memory ordering, real-time rules

Each answer: reasoning first, then **common wrong answers** (why tempting / why wrong). Try the question yourself before reading. Section references are to `worksheet.md`.

---

## 5.1 Explain why

### Q1 - lost updates, and why `volatile` does not fix it

**Answer.** `counter = counter + 1` is three steps: load into a register, add 1, store back (§3.1). If Thread 2 loads *before* Thread 1 stores, both compute `old + 1` and both store the same number: two increments, one visible. With four threads hammering the same cache line a million times, interleavings like this occur constantly, so the total is a random fraction of 4,000,000. `volatile` only forces the compiler to emit the load and the store each time; it does not make the three steps indivisible and gives no inter-thread ordering. (It is also still a data race, i.e. UB.)

**Common wrong answers.**
- *"The CPU executes increments too fast and some get skipped."* Tempting because it sounds like a hardware speed limit. Wrong: nothing is skipped by the hardware; each thread's stores overwrite the others' results because the *read* was stale.
- *"`volatile` makes the variable shared, so it's safe."* Tempting from Java's `volatile`. In C++ it has no threading meaning.
- *"On x86 `add [mem],1` is atomic."* Only with the `lock` prefix, which the compiler emits for `atomic::fetch_add`, not for a plain/volatile `=` + 1; and the standard calls it UB anyway.

### Q2 - why `cv.wait` needs a predicate

**Answer.** A condition variable is a doorbell with no memory; the *state* is in your data. Three failures without a predicate/loop (§3.2):
1. **Lost wake-up:** the notification happens before the wait starts; the waiter sleeps forever (Q14).
2. **Spurious wake-up:** the implementation may return from `wait` with no notify.
3. **Stolen wake-up:** after the notify, another thread takes the item before the woken thread re-acquires the mutex; the condition is false again (Q15).
`cv.wait(lock, pred)` is `while (!pred()) cv.wait(lock);`: the condition is re-tested under the lock on every wake-up.

**Common wrong answers.**
- *"Only for spurious wake-ups, which are rare."* Tempting because spurious wake-ups are the famous reason. Wrong: the lost-wakeup and stolen-wakeup cases are *deterministic* logic bugs, not OS quirks.
- *"If I use `notify_all`, I don't need the predicate."* `notify_all` wakes everyone but they still race for the lock/item; the loser must re-check.
- *"The mutex already makes it safe."* The mutex protects the data; it does not tell a sleeping thread when the condition became true.

### Q3 - `relaxed` flag and stale data

**Answer.** Relaxed makes only the flag access atomic; it does *not* order the neighbouring `data` accesses. Mechanisms: (1) **compiler reordering** - two unrelated stores may be emitted in either order; (2) **store buffer** - on weak CPUs `ready` may drain to cache before `data`; (3) different cache lines arriving at different times on the consumer core; (4) speculative execution of `load data` before `load ready` resolves. **x86 (TSO)** hides (2)-(4) for this pattern (stores stay in order, loads stay in order), leaving only (1) - which is why such bugs appear first on ARM. Fix: `store(release)` / `load(acquire)` (§3.4).

**Common wrong answers.**
- *"Atomic variables are always visible immediately, so `data` is too."* Tempting: "atomic" sounds like "globally synchronised". Wrong: an atomic orders *itself*; ordering other memory is the memory-order argument's job.
- *"Only the compiler is to blame, add a compiler barrier."* A compiler barrier does not stop a weak CPU reordering.
- *"x86 is safe so the code is correct."* The compiler can still reorder, and the code is a data race (UB).

### Q4 - locks and heap on a real-time path

**Answer.** *Locks:* (a) you wait as long as the holder holds it, and the holder may be pre-empted or low priority - your WCET includes its WCET; (b) priority inversion (§3.2) can stretch the wait unboundedly. *Heap:* (a) search time depends on fragmentation, unbounded in general; (b) the allocator takes an internal lock (so it inherits all lock problems) and on slow paths calls the OS, which can page-fault (microseconds-milliseconds). Average cost is irrelevant because the deadline is broken by the **tail**: at 1000 allocations/s a one-in-a-million slow path hits in ~17 minutes (Q7).

**Common wrong answers.**
- *"malloc is fast enough."* True on average, which is exactly the wrong metric.
- *"Use a mutex only briefly, then it's fine."* Brief for the holder, but the *scheduler* can pre-empt the holder inside its critical section.
- *"Lock-free fixes everything."* Lock-free removes blocking, but a CAS loop can retry unboundedly; wait-free or single-writer structures are what give bounds.

### Q5 - exceptions on hot paths

**Answer.** The throw path is expensive and *variable*: allocate the exception object, search unwind tables, run destructors frame by frame, touch cold code/data (cache misses, possible page faults). The not-thrown path is cheap, but you must plan for the throw. Embedded builds often use `-fno-exceptions`. Instead: return `std::optional`/error codes/`expected`-style results (pack 04), validate cheaply at the boundary, mark hot functions `noexcept` so a stray exception terminates loudly rather than adding hidden latency.

**Common wrong answers.**
- *"Exceptions are always slow."* The no-throw path is near-free on modern ABIs; the issue is the unpredictable throw path.
- *"Catch it and continue; the cost is paid only on malformed frames."* Malformed frames are exactly when you are under stress (noise, interference) and the deadline still applies.

---

## 5.2 Calculate

### Q6 - utilisation and slack
Worst-case sum = 3 + 6 + 2 = 11 ms. Utilisation = 11/20 = **55%**. Slack = 20 - 11 = **9 ms**. The thread can absorb at most **9 ms** of blocking. A 4 ms lock hold fits (11 + 4 = 15 < 20) - *if* the bound of 4 ms is truly guaranteed (priority inversion and scheduling could add more, so it is still risky); a 10 ms hold does not (11 + 10 = 21 > 20).

**Common wrong answers.**
- *"Utilisation 55% means plenty of room, anything goes."* Utilisation is average load; the deadline cares about the single worst frame.
- *"Slack = 20 - 6 = 14 ms (only the biggest stage)."* Stages run in sequence within the period; all three count.

### Q7 - how soon does malloc bite?
(a) 20 allocations/frame x 50 frames/s = 1000 allocations/s. One in 10^6 is slow, so on average one slow call every 10^6/1000 = **1000 s ~ 16.7 minutes**; a 12 ms stall exceeds the 9 ms slack, so that is a deadline miss. (b) 8 h = 28,800 s -> 28,800/1000 = **~29 misses** (expected value; actual count varies). (Numbers are illustrative: the point is that rare events become frequent at 24/7 rates.)

**Common wrong answers.** *"One in a million is basically never."* Tempting because tests last seconds; wrong because the device runs for months: 10^6 events happen in minutes. *"20 x 50 = 100"* - arithmetic slip: 1000.

### Q8 - sizing with Little's law
(a) L = lambda x W = 50 x 0.070 = **3.5 frames** in flight on average. (b) 3.5 + 4 = 7.5 -> at least 8 slots; 8 is already a power of two -> **8**. Reason for power of two: masks instead of modulo (pack 07).

**Common wrong answers.** *"3.5 -> 4 slots."* Ignores the burst. *"50 x 70 = 3500."* Unit slip: 70 ms is 0.070 s.

---

## 5.3 Predict

### Q9 - store-buffer litmus
`seq_cst`: **(0,1), (1,0), (1,1)**. `relaxed`: those three **plus (0,0)**. (0,0) is the surprise: each thread's store is still sitting in its own **store buffer** when its load runs, so each load reads the old 0. It is permitted by `relaxed`, by acquire/release, and even happens on **x86** hardware (the one reordering TSO allows: a later load passing an earlier store). Only `seq_cst` (which inserts a full fence after the stores, e.g. `mfence`/`xchg` on x86) forbids it.

**Common wrong answers.**
- *"(0,0) is impossible - one of them must have stored first."* Tempting: it assumes one global order, which is exactly what `seq_cst` provides and `relaxed` does not.
- *"Only ARM can do that."* x86 can for store->load.

### Q10 - lock-order deadlock
Both threads lock their first mutex, sleep 10 ms, then each waits for the mutex the other holds: **deadlock**; the program hangs. The `sleep_for` makes the demo deterministic: without it, one thread often finishes both locks before the other starts and the bug stays latent until a rare timing in the field. Fixes: (1) a global lock order (always `a` then `b`); (2) `std::scoped_lock lk(a, b);` in both, which acquires both with deadlock avoidance.

**Common wrong answers.**
- *"One thread wins and the other waits briefly."* That is only true if the first completes both locks; here each holds one lock and wants the other.
- *"`lock_guard` prevents deadlock because it unlocks automatically."* It unlocks at scope end, which never comes.

### Q11 - destroying a joinable thread
`std::terminate()` is called when `t` is destroyed at the end of `start()` - program aborts (typically printing "terminate called without an active exception"). It does **not** matter if the thread has finished: a finished-but-not-joined thread is still *joinable*. (`"hi"` may or may not appear first.) Fix: `join()`, `detach()` (with ownership care), or `std::jthread`.

**Common wrong answers.** *"The thread is finished, so nothing happens."* Tempting - nothing is left to wait for - but joinability is about the handle, not the running state. *"It detaches automatically."* The designers chose abort over guessing.

---

## 5.4 Trace by hand

### Q12 - three increments
Start 10. T1 load (r1 = 10); T2 load (r2 = 10); T1 add+store -> memory 11; T1 load (r1 = 11); T1 add+store -> 12; T2 add+store: r2 = 10 + 1 = 11 -> memory **11**. Expected 13: **2 updates lost** (T2's stale value overwrote both of T1's).

**Common wrong answers.** *"12"* (assuming T2's add applied to the current memory). *"13"* (assuming atomicity).

### Q13 - bounded queue, capacity 1
| step | event | queue | blocked |
|---|---|---|---|
| 1 | P `push(1)` ok | [1] | |
| 2 | P `push(2)`: full -> waits on `not_full_` | [1] | P (`not_full_`) |
| 3 | C `pop` -> 1; notify `not_full_`; C `pop` again: empty -> waits on `not_empty_` | [] | C (`not_empty_`); P runnable |
| 4 | P wakes, `push(2)` ok; notify `not_empty_`; `push(3)`: full -> waits | [2] | P (`not_full_`); C runnable |
| 5 | C wakes, `pop` -> 2; notify `not_full_`; `pop`: empty -> waits | [] | C (`not_empty_`) |
| 6 | P wakes, `push(3)` ok, notify, `close()`: `closed_ = true`, `notify_all` both | [3] | none |
| 7 | C wakes: pred true (non-empty), `pop` -> 3; next `pop`: empty and closed -> `nullopt` | [] | |

Consumer receives 1, 2, 3, then end. Note step 7: because `pop` tests `q_.empty()` *after* the wait, the item 3 enqueued before `close()` is still delivered (drain semantics).

**Common wrong answer.** *"close() discards the remaining item 3."* Tempting from "closed means nothing more happens". Our contract: closed stops *new pushes*; consumers drain.

### Q14 - lost wake-up
Interleaving: (1) Notifier takes the lock, sets `ready = true`, releases it; (2) Notifier calls `notify_one()` - nobody is waiting, the notification vanishes; (3) Waiter locks and calls `cv.wait(l)` - it never looks at `ready` - it sleeps; (4) nobody will ever notify again: hang. Fix: `cv.wait(l, [&]{ return ready; });` - the waiter sees `ready == true` and doesn't sleep at all.

**Common wrong answer.** *"notify is queued until a waiter arrives."* Tempting (semaphores behave like that). Condition variables don't count.

---

## 5.5 Find the bug

### Q15 - `if` instead of `while`
`if (q.empty()) cv.wait(lock);` does not re-check after waking. Interleaving: consumer A sleeps in `wait`. Producer pushes one item and notifies A; A is runnable but must re-lock the mutex. Before it does, consumer B enters `pop`, locks the free mutex, sees a non-empty queue and takes the item. A then gets the mutex, returns from `wait`, calls `q.front()` on an **empty deque: UB** (crash under load). The same crash occurs after a spurious wake-up. Fix: `cv.wait(lock, [&]{ return !q.empty(); });`.

**Common wrong answer.** *"The producer notified only after pushing, so the queue can't be empty when A wakes."* True at notify time, false by the time A holds the lock.

### Q16 - re-locking a non-recursive mutex
`replace` locks `m_`, calls `remove`, which tries to lock `m_` again in the same thread: undefined behaviour for `std::mutex`, in practice **self-deadlock**. Fix: private helpers `add_locked`/`remove_locked` that assume the caller holds the lock; each public method locks once and calls helpers. (`std::recursive_mutex` compiles but hides a design flaw - and invariants may be broken when the inner call is re-entered; avoid.)

**Common wrong answer.** *"lock_guard is recursive-safe because it's RAII."* RAII manages release, not re-entrancy.

### Q17 - plain bool as stop flag
The compiler optimises `worker` as single-threaded: `stop` is not modified in the loop, so it hoists the load out and the loop becomes `while (true)`. It is also a data race (UB). `volatile bool` forces re-reading but remains a data race, gives no ordering, and is not what the standard offers for threads. Fix: `std::atomic<bool> stop{false};` with `stop.store(true, std::memory_order_release)` and `while (!stop.load(std::memory_order_acquire))` - release/acquire so data written before the request is visible to the worker (our `StopFlag`). Plain `relaxed` would also stop the loop but publishes nothing.

**Common wrong answers.** *"Add `volatile`."* Tempting because it fixes the infinite loop on your machine; wrong for the reasons above. *"It would work in a debug build, so the compiler is broken."* The standard permits the optimisation.

### Q18 - real-time violations
1. `std::vector tmp(...)`: heap allocation per frame -> use a preallocated buffer (`std::array<int16_t, 160>` or a `FixedPool` slot); better, avoid the copy.
2. `lock_guard` on a shared mutex: can block (inversion) -> remove the lock: counters as atomics, data through `Mailbox`/SPSC.
3. `std::cout`: takes a lock, may allocate, makes a system call -> write a fixed-size record into a lock-free ring and let a non-RT logger thread print.
4. `throw`: unbounded latency -> return an error status, drop/mark the frame, bump an `EventCounter`.
5. `stats["frames"]++` on `map<string,int>`: map insertion allocates a node, constructing a `std::string` key may allocate, and the lookup cost grows -> a plain struct of `EventCounter` members.

**Common wrong answers.** *"Only (1) and (4) matter."* Tempting because they look like "C++ features"; (2) and (3) are equally unbounded. *"Replace the vector by `reserve`."* `reserve` still allocates the first time - do it at startup, not per frame.

### Q19 - detached thread capturing a local
Problems: (1) `result` is captured by reference, but `compute()` can return first - the thread then writes into a dead stack frame (dangling reference, UB); (2) `use(result)` runs concurrently with the thread's write: a data race (reads 0 or a stale value); (3) detach means nothing can wait for completion or learn about an exception (which would call `terminate`). Fix:
```cpp
int result = 0;
std::jthread t([&result] { result = heavy(); });
t.join();            // explicit join BEFORE reading result; jthread's destructor would join too late
use(result);
```
or `std::async`/`future` to return the value safely.

**Common wrong answer.** *"jthread joins automatically, so just delete `detach`."* It joins at *scope exit*, after `use(result)` ran.

---

## 5.6 Implement

### Q20 - `BoundedQueue`
See `solutions/es_rt/bounded_queue.hpp`. Key decisions:
- `push` waits on `not_full_` with predicate `closed_ || q_.size() < cap_`; if `closed_`, return false without enqueueing.
- `pop` waits on `not_empty_` with `closed_ || !q_.empty()`; **then tests `q_.empty()`** (not `closed_`) so remaining items are drained before `nullopt`.
- `close()` sets `closed_` **under the mutex**, then `notify_all` on both cvs (every waiter must re-evaluate; `notify_one` would leave some asleep).
- Notify after unlocking (`lock.unlock()` then `notify_one()`): avoids waking a thread that will block on our still-held mutex.
- Value is moved in/out so `unique_ptr` payloads work (test `MoveOnlyPayload`).

**Common wrong answers / bugs.**
- `if` instead of the predicate (Q15). Tempting because shorter.
- `close()` with `notify_one`: with two consumers, one stays asleep forever (test `CloseWakesBlockedConsumer` only catches one waiter - think about N).
- Setting `closed_` without the mutex: the lost-wake-up gap of §3.2.
- Returning `nullopt` as soon as `closed_` (dropping queued data): fails `CloseDrainsThenReportsEnd`.
- Holding the lock while notifying: correct but slower; not wrong.

### Q21 - `Mailbox`
See `solutions/es_rt/mailbox.hpp`; the protocol and every edge is §4.
- `try_publish`: `full_.load(acquire)`; if true return false; `slot_ = v`; `full_.store(true, release)`.
- `try_take`: `full_.load(acquire)`; if false return false; `out = slot_`; `full_.store(false, release)`.
Why acquire on the *first* load of each side, not only the consumer's: the producer's acquire load synchronizes with the consumer's release-store of `false`, ensuring the consumer's read of `slot_` finished before the producer overwrites it (the second happens-before edge in §4).

**Common wrong answers.**
- *All `relaxed`.* Passes on x86 often; TSan reports a race on `slot_`; breaks on ARM.
- *Producer reads `full_` with `relaxed`.* Tempting ("it only reads its own flag") - but the consumer writes it, and the slot reuse needs the edge.
- *Making `slot_` atomic.* Doesn't work for multi-word payloads (not lock-free, and still doesn't guarantee a consistent *set* of fields).
- *`seq_cst` everywhere.* Correct, a bit slower on ARM; fine to start with, but know the minimal orders.

### Q22 - `EventCounter`, `StopFlag`
- `EventCounter::add`: `fetch_add(n, relaxed)`. Justification: it is a statistic; no other data is published through it; atomicity alone prevents lost updates. `load`: `relaxed` - a snapshot is fine.
- `StopFlag::request`: `store(true, release)`; `requested`: `load(acquire)`. Justification: a worker that sees `true` may read configuration the controller wrote before; the pair publishes it.

**Common wrong answers.** *Counter with `seq_cst`:* correct, just slower - fine, but then you can't justify the cost. *Counter with `count++` on a plain `uint64_t`:* a race. *StopFlag with `relaxed`:* the loop stops, but data written before `request()` is not guaranteed visible.

### Q23 - `FixedPool`
See `solutions/es_rt/pool.hpp`.
- Constructor: `free_[k] = N-1-k`, `free_count_ = N` so index 0 is on top (first acquire = element 0).
- `acquire`: if `free_count_ == 0` return `nullptr` (exhaustion is a normal result, never an exception or allocation); else pop an index, mark in use, return `&items_[idx]`.
- `release`: convert pointer to byte offset from `items_.data()` (via `uintptr_t` to avoid comparing unrelated pointers), reject offsets that are negative (`addr < base`), not a multiple of `sizeof(T)`, `idx >= N`, or `!in_use_[idx]` (double release); else push the index.
- Everything `noexcept`, storage inside the object: **no heap** (`AllocCounting.PoolHotLoopMakesNoHeapAllocations`).
Limitations to know: single-threaded; objects are default-constructed once and *reused*, not re-constructed (pack 07's `FixedVector` shows placement new for that).

**Common wrong answers / bugs.**
- Comparing `p < items_.data()` directly: comparing pointers to different objects is unspecified; use integers (as above).
- Forgetting the double-release guard: the same index is pushed twice -> two threads/users get the same object.
- `std::vector<std::size_t> free_` : allocates (fails the allocation-count test and the whole point).
- Throwing on exhaustion: breaks the hot-path rule.

---

## 5.7 Connect

### Q24 - `shared_ptr` is not thread-safe data
The **control block** (reference counts) is atomic, so copying/destroying `shared_ptr`s across threads is safe and the object is destroyed exactly once. The **pointee** is not protected: two threads mutating the same `Call` through different `shared_ptr` copies still race; also a *single* `shared_ptr` object being reassigned by one thread while another reads it is a race (use `std::atomic<std::shared_ptr>` in C++20 or a mutex). Moving a `UniqueBox<Frame>` through a `BoundedQueue` transfers **sole ownership**: only one thread touches the frame at a time, the queue's mutex supplies the happens-before, and no locks on the frame are needed. (And no atomic reference counting cost.)

**Common wrong answers.** *"shared_ptr is thread-safe, so I can share the object."* The most common concurrency misconception in C++ code reviews: the safety is only for the count.

### Q25 - tearing and alignment
A data race is UB; a practical symptom of a *misaligned* (or wider-than-word) access is **tearing**: the CPU performs it as two smaller accesses (two bytes/words, possibly two cache lines). The writer's store can be split likewise; the reader can combine the new first half with the old second half: a value *never written* (e.g. 0x1234FFFF where 0x1234_0000 and 0x0000_FFFF were written). `std::atomic<uint32_t>` is aligned to 4 so the access is a single indivisible operation inside one cache line; casting a raw buffer address to `atomic<T>*` breaks that (and aliasing: pack 05 §3.3).

**Common wrong answer.** *"A race on a 32-bit value just gives old or new, never garbage."* True only for aligned word accesses on typical CPUs - and still UB in C++, so the compiler can do something else entirely.

### Q26 - SPSC ring indices (preview)
Producer: `tail` load = **acquire** (pairs with the consumer's release-store of `tail`: the consumer finished reading the slot being reused); write the element (plain); `head` store = **release** (publishes the element). Consumer: `head` load = **acquire** (sees the element the producer published); read the element; `tail` store = **release** (tells the producer the slot may be reused). Each side may read its **own** index with `relaxed`: only that thread ever writes it, so it always sees its latest write (program order) and nothing needs publishing to itself.

**Common wrong answer.** *"Use `seq_cst` everywhere or it's wrong."* Not wrong, only slower on weak CPUs. *"Relaxed on everything: it's just indices."* Then the element data isn't published.

---

## 5.8 Interview-style

### Q27 - mutex vs atomic vs lock-free queue
Decision rule: **start with a mutex**; it's simplest, correct, and cheap when uncontended. Use an **atomic** for a *single* independent word whose meaning is self-contained: a stop flag, a statistics counter, a state enum read by monitoring code. Use a **lock-free (SPSC/MPSC) queue** only where blocking is forbidden or the worst case must be bounded: the radio/vocoder thread handing frames to protocol threads. Etherstack-style examples: the call table (mutex), `frames_received` (relaxed atomic counter), RX interrupt -> protocol thread frames (SPSC ring). Mention the cost of subtlety: lock-free structures are hard to verify; test under TSan.

**Weak answers:** *"Lock-free is faster, so always use it"* (not on average, and risk of bugs). *"Atomics for everything"* (can't atomically update two related fields).

### Q28 - acquire/release for a mutex person
"`unlock()` is a release and `lock()` is an acquire: everything done before the unlock is visible to whoever locks next. An atomic `store(release)` / `load(acquire)` does the same one-way publication without the mutual exclusion. In the message-passing pattern the producer writes data, then does a release-store on the flag; the consumer does an acquire-load on the flag; if it sees `true`, it is guaranteed to see the data. Without the pair, the compiler or CPU may reorder and the consumer can see the flag but old data."

**Weak answers:** "Acquire/release makes it faster than `seq_cst`" (that's a cost, not the meaning); "it flushes caches" (hardware detail, wrong model).

### Q29 - thread structure of a gateway
Model: **threads own data; communicate with messages.** RX thread (real-time: owns the DSP/radio side, preallocated pools, hands frames over via SPSC/Mailbox; no locks/heap/exceptions); protocol thread (owns the call state machine - single-threaded logic, no locks inside; receives events from queues); network thread (MCPTT side; blocking I/O allowed; talks via `BoundedQueue`); logging thread (non-RT; receives records from a lock-free ring). Shared mutable state is minimal: counters (relaxed atomics), a shutdown flag (release/acquire), config published once. Only RX/vocoder is hard-deadline. Shutdown: `StopFlag` + `close()` the queues + `join`. Mention testing: TSan + stress tests; no live public-safety traffic in tests.

**Weak answers:** "A global mutex around everything" (kills real-time, hides ordering); "Use `std::async` per packet" (allocates a thread/state per call).

### Q30 - priority inversion
A high-priority task H blocks on a lock held by a low-priority task L; a medium-priority task M pre-empts L; H effectively waits for M. Prevention: **priority inheritance** (L inherits H's priority while holding the lock) or **priority ceiling** protocols (POSIX `PTHREAD_PRIO_INHERIT`/`PTHREAD_PRIO_PROTECT`, RTOS mutexes); better still, avoid sharing locks across priorities (lock-free hand-off, message passing, `try_lock` with fallback). `std::mutex` has no priority protocol (VERIFY your platform's implementation), so it can't solve it alone. Real example: Mars Pathfinder 1997 (VERIFY details).

**Weak answers:** "Raise everything to high priority" (starves the system); "Hold the lock for less time" (helps, but pre-emption inside the section still causes inversion).
