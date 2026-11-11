# Pack 06 - Threads, atomics, memory ordering, real-time rules

**Time:** 2-4 hours. **Code:** `code/` (stubs in `include/es_rt/`, reference in `solutions/`, a deliberately racy demo in `labs/`).
**Tooling:** the tests spawn threads. They run under ASan/UBSan by default; run them **a second time** with ThreadSanitizer: `cmake -S . -B build-tsan -DENABLE_TSAN=ON` (GCC/Clang on Linux/macOS/WSL2 - there is no TSan for MinGW or MSVC). On x86 many wrong memory orderings still "pass" at run time; TSan is what catches them.
Build and run: `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`. Every test has a 60 s timeout, so a deadlocked implementation fails instead of hanging.

---

## 1. Why this matters

A radio gateway is several things happening at once: a thread reading the radio/DSP interface, a protocol thread running the call state machine, a network thread talking to the 4G/5G MCPTT side, a logging thread. They share data (call tables, queues of frames, counters). Voice adds a deadline: a vocoder frame arrives every **20 ms** (P25 IMBE; DMR/TETRA use other framings - VERIFY) and must be decoded and played before the next one, every time, for hours.

Real consequences:

- Two threads do `++frames_received`; the total is silently wrong (§3.1). Replace "counter" by "length of a retransmit queue" and you get corrupted lists, not just wrong numbers.
- Two functions lock two mutexes in opposite order; once a month the gateway freezes mid-call (§3.2).
- A "ready" flag is set after the data is filled in - in your source. The compiler and the CPU may make it visible *before* the data (§3.4). It passes every test on your x86 laptop and fails on the ARM board.
- A voice thread calls `malloc` once per frame. 99.9999% of the time it takes 100 ns; once in a million it takes milliseconds, and a syllable of audio is clipped (§3.5). Average speed is irrelevant for deadlines; **worst case** is all that counts.

This pack builds the rules from the machine up: what really happens on `++x`, why a condition variable needs a predicate, what acquire/release actually promise, and why real-time code forbids heap, locks and exceptions.

> **Safety:** all frames, counters and messages in this pack are synthetic; never connect these exercises to live public-safety traffic.

---

## 2. Prerequisites check

- **P1.** What is a *data race* in one sentence, and why is it undefined behaviour rather than just "a wrong value"? *(Pack 05 `../05-ub-layout-bit/` §3.1: UB lets the optimiser assume it never happens.)*
- **P2.** What does a `std::lock_guard`-style object do in its constructor and destructor, and why is that RAII? *(Pack 03 `../03-lifetime-move/` §3.1.)*
- **P3.** Why can `vector::push_back` invalidate pointers and allocate, and what does `reserve` change? *(Pack 04 `../04-containers-templates/` §3.1.)*
- **P4.** Which sanitizer flag did you use in pack 01, and what does it catch that a unit test usually does not? *(Pack 01 `../01-toolchain/` §3.4.)*
- **P5.** `uint32_t` loaded from an address that is not a multiple of 4: what can go wrong? *(Pack 05 §3.4: misalignment; this matters again for atomics.)*

---

## 3. Concept build-up

### 3.1 Threads and data races

**Concrete first.** `labs/lab_lost_update.cpp`: four threads each execute `counter = counter + 1` one million times on a shared `volatile int`. Expected total 4,000,000. Typical output (yours differs, and differs *every run*):

```
expected      : 4000000
racy (volatile): 1273518   (lost 2726482 updates)
atomic        : 4000000   (lost 0 updates)
```

Where did 2.7 million increments go?

**Derive it.** A CPU has no "add one to this memory cell" step you can rely on. `counter = counter + 1` is **three** machine steps: (1) *load* the value from memory into a register, (2) *add* 1 in the register, (3) *store* the register back. Two threads can interleave those steps:

| step | Thread 1 | Thread 2 | memory `counter` |
|---|---|---|---|
| 1 | load -> r1 = 5 | | 5 |
| 2 | | load -> r2 = 5 | 5 |
| 3 | add: r1 = 6 | add: r2 = 6 | 5 |
| 4 | store r1 | | 6 |
| 5 | | store r2 | 6 |

Two increments happened, the counter went up by one: Thread 2's load saw the old value, so its store overwrote Thread 1's work. This is a **lost update**. With more cores and a million iterations it happens constantly.

**The rule (C++ memory model).** A **data race** exists when two threads access the *same memory location*, at least one access is a *write*, at least one access is *not atomic*, and nothing orders them (no happens-before, §3.4). A data race is **undefined behaviour** (the whole program, not just that line - pack 05 §3.1). The reasons are the ones you already know: the compiler optimises each thread as if no other thread existed (it may keep `counter` in a register for the whole loop, or turn the loop into one addition), and the hardware may tear or reorder accesses.

**Why `volatile` does not help.** `volatile` tells the compiler "do not optimise away this load/store". It does not make the three steps indivisible and it orders nothing between threads. (It exists for memory-mapped hardware registers.) In the lab it is only used so the loads/stores actually exist. Never use `volatile` for thread communication.

**Thread lifetime.** `std::thread t(f);` starts running `f` immediately. Before `t` is destroyed you must call `t.join()` (wait for it) or `t.detach()` (let it run free). Destroying a *joinable* thread calls `std::terminate()` - the language refuses to guess which you meant. `std::jthread` (C++20) is the RAII fix: its destructor joins. An exception that escapes the thread function also calls `std::terminate`. Arguments are copied into the thread; to pass a reference you must write `std::ref(x)` - and then *you* guarantee `x` outlives the thread.

**Three ways to make shared data safe**, in order of preference:
1. **Don't share.** Give each thread its own data and pass *messages* (queues, §3.2/pack 07). Most of the code in a gateway should be like this.
2. **Protect with a mutex** (§3.2): only one thread at a time touches the data.
3. **Make the single access atomic** (§3.3): for flags, counters, indices.

> **Common misconceptions**
> - *"`counter++` is one instruction, so it is safe."* Even where the compiler emits one `add [mem], 1`, the hardware does a load-modify-store inside; without the `lock` prefix another core can interleave. And the standard calls it a race regardless.
> - *"The race only loses a count."* A race on a pointer or a container's size corrupts structure; and it is UB.
> - *"`volatile` makes it thread-safe."* No (see above).
> - *"It passed 1000 test runs."* Races depend on timing; the optimiser, core count and load change it. TSan finds races that did not happen to manifest.
> - *"Detaching is the easy way to avoid join."* A detached thread that touches a destroyed local variable is a use-after-free (Q19).

### 3.2 Mutexes, deadlock, and condition variables

**Concrete first.** A call table shared between the protocol thread (adds/removes calls) and the network thread (reads them). The simplest safe design: one `std::mutex` guarding the table.

```cpp
std::mutex m;
std::map<int, Call> calls;

void add_call(int id, Call c) {
    std::lock_guard<std::mutex> lock(m);   // constructor: m.lock(); destructor: m.unlock()
    calls[id] = std::move(c);
}                                          // unlock happens even if an exception is thrown (RAII)
```

**Why it works.** `m.lock()` blocks until no other thread holds `m`. Code between lock and unlock (the **critical section**) therefore runs one thread at a time, and `unlock` also *publishes* all writes done inside to the next thread that locks (this is the happens-before of §3.4, for free). `lock_guard` is RAII (pack 03): you cannot forget to unlock.

**The lock family.** `lock_guard` (simple); `unique_lock` (can unlock early/relock; **required** by condition variables); `scoped_lock` (C++17; locks several mutexes without deadlock, see below). `std::mutex` is **not recursive**: locking it twice from the same thread is undefined (typically deadlock) - Q16.

**Rules of thumb for critical sections.** Keep them short; never call unknown code (callbacks, virtual functions, I/O) while holding the lock - that code may try to take the same lock or take forever.

**Deadlock - derive it.** Thread 1 locks A then wants B. Thread 2 locks B then wants A:

| step | Thread 1 | Thread 2 |
|---|---|---|
| 1 | lock(A) ok | |
| 2 | | lock(B) ok |
| 3 | lock(B) **waits** for T2 | |
| 4 | | lock(A) **waits** for T1 |

Both wait for the other forever. Four conditions are needed (mutual exclusion, hold-and-wait, no pre-emption, circular wait); the cheapest to remove is circular wait: **always lock in one global order**, or take both at once with `std::scoped_lock lk(a, b);` which uses a deadlock-avoidance algorithm (it may try/back off internally).

**Condition variables - the problem they solve.** A consumer thread needs to sleep until the queue is non-empty. Spinning (`while (q.empty()) {}`) burns a core; sleeping with a timer adds latency. A `std::condition_variable` lets a thread **sleep until another thread says "something changed"**.

```cpp
std::unique_lock<std::mutex> lock(m);
cv.wait(lock, [&]{ return !q.empty(); });   // == while (!q.empty()) cv.wait(lock);
auto item = std::move(q.front()); q.pop_front();
```

`cv.wait(lock)` does *atomically* "unlock the mutex and go to sleep"; when woken, it re-locks the mutex before returning.

**Derive why the predicate loop is mandatory.** Three separate failure stories:
1. **Lost wake-up.** If the notifier calls `notify_one()` *before* the consumer starts waiting, the notification is gone (a condition variable has no memory). A `wait` without checking the *state* (`!q.empty()`) sleeps forever (Q14). The **state** lives in your data, under the mutex; the cv is only a doorbell.
2. **Spurious wake-ups.** The standard allows `wait` to return without any notify (it makes implementations simpler/faster on some OSes). If you proceed blindly, you read an empty queue.
3. **Stolen wake-ups.** Between "producer notifies" and "consumer re-acquires the mutex", another consumer may have taken the item. Only re-testing the predicate under the lock is correct (Q15).

**Why the state change must happen under the mutex** even if the flag is atomic: the consumer checks the predicate (false) *while holding the mutex*, then goes to sleep. If the notifier flips an atomic flag and notifies *without* the mutex, it can do both in the gap between the consumer's check and its sleep: the doorbell rings, nobody hears it. Taking the mutex to change state closes the gap.

**Notify inside or outside the lock?** Both are correct. Notifying after unlocking (as `solutions/bounded_queue.hpp` does) avoids waking a thread that immediately blocks on the mutex we still hold.

**Priority inversion (preview of §3.5).** A high-priority thread H needs a mutex held by low-priority L. A medium-priority thread M becomes runnable and pre-empts L (M doesn't need the lock). Now H waits for L, and L can't run because of M: H is effectively delayed by M, which has *lower* priority than H. The 1997 Mars Pathfinder lander reset itself repeatedly because of exactly this (VERIFY details); the fix was enabling **priority inheritance** (L temporarily runs at H's priority while it holds the lock). `std::mutex` offers no such control; RTOS/POSIX mutexes (`PTHREAD_PRIO_INHERIT`) do.

> **Common misconceptions**
> - *"`cv.wait` wakes only when notified."* It may return spuriously: always use a predicate.
> - *"`notify_one` before `wait` is remembered."* No.
> - *"A mutex protects the variable."* It protects nothing by itself: every access, everywhere, must take the same mutex. One forgotten unlocked read is a data race.
> - *"Lock-free is always faster than mutexes."* An uncontended mutex costs tens of nanoseconds; lock-free structures are subtle and are justified mainly by **bounded worst case** (§3.5), not average speed.
> - *"`std::mutex` is fair / recursive."* Neither.

### 3.3 Atomics

**Concrete first.** The lab's second loop: `std::atomic<int> g; g.fetch_add(1);` four threads, 4,000,000 total, every time.

**What an atomic is.** `std::atomic<T>` makes each operation on it **indivisible**: no other thread can observe or interleave a half-done operation, and it is never a data race. `fetch_add` is one atomic **read-modify-write** (RMW): the load, add and store of §3.1 become one step. On x86 this compiles to a `lock add` (the core owns the cache line for the duration); on ARM to a load-exclusive/store-exclusive retry loop or a single LSE atomic instruction.

**Operations:** `load`, `store`, `exchange`, `fetch_add/sub/and/or/xor`, `compare_exchange_strong/weak` (CAS: "if the value is still `expected`, replace it with `desired`, else tell me the current value" - the building block of every lock-free algorithm; used in a retry loop).

**Lock-free or not?** `std::atomic<T>::is_lock_free()` tells you whether the CPU can do it with real atomic instructions or the library silently uses a hidden mutex. Integers up to the word size usually are; a 64-byte struct is not. A "lock-free" atomic that actually takes a lock is **forbidden on a real-time path** (§3.5) - check `is_lock_free()` (or `is_always_lock_free`) for your target (VERIFY on every new platform; `AtomicFacts` test prints it).

**Why atomics need alignment.** A naturally aligned 4-byte value sits inside one cache line and the CPU can read/write it in one go; a misaligned one may straddle two lines, and the hardware can no longer do it atomically (pack 05 §3.4). `std::atomic<T>` guarantees the alignment for you; don't cast raw buffer addresses to `std::atomic<T>*`.

**Memory orders.** Besides being indivisible, every atomic operation takes a `memory_order` argument that says how it is ordered relative to the *other* memory accesses around it. The default, `memory_order_seq_cst`, is the strongest and safest. Weaker orders exist because ordering costs time (and, on ARM, energy). There are three you need:

| order | promise | typical use |
|---|---|---|
| `relaxed` | the operation itself is atomic. **No ordering** of other memory accesses. | statistics counters (`EventCounter`) |
| `release` (stores) / `acquire` (loads) | publish data / see published data (§3.4) | flags, indices, mailboxes, SPSC queues |
| `seq_cst` (default) | acquire+release *and* all seq_cst operations appear in one global order | when in doubt; Dekker-style "I wrote mine, did you write yours?" patterns |

**ABA (note, no proof).** A CAS that checks "value is still A" can be fooled if the value went A -> B -> A in between (e.g. a pointer reused after free). Real lock-free stacks need tags/hazard pointers. *Taken on trust here;* the structures in this course (SPSC) avoid it by having single writers.

> **Common misconceptions**
> - *"Atomic means ordered."* An atomic `relaxed` counter says nothing about other variables.
> - *"`atomic<int> x; x = x + 1;` is atomic."* It is two atomic operations (load, then store) - a lost update is again possible. Use `fetch_add`/`++x` (the `++` operator on `atomic` *is* an RMW).
> - *"`std::atomic<MyStruct>` is fast."* If `is_lock_free()` is false it is a mutex in disguise.
> - *"CAS loops can't fail."* They can retry unboundedly under contention: lock-free is not wait-free.

### 3.4 Acquire/release, with a concrete reordering

**Concrete first - the message-passing pattern.** The producer fills in a message, then sets a flag. The consumer waits for the flag, then reads the message:

```cpp
int data = 0;
std::atomic<bool> ready{false};

// Producer thread               // Consumer thread
data = 42;                       while (!ready.load(std::memory_order_relaxed)) {}
ready.store(true, std::memory_order_relaxed);   assert(data == 42);   // can this fire?
```

With `relaxed` operations on `ready` **the assert can fire**, and `data` is not even atomic, so this is also a data race. Why? Four independent mechanisms can make "`ready` visible, `data` not yet":

1. **The compiler** sees two stores to *different, unrelated* locations. Nothing in single-thread semantics says `data = 42` must be emitted first; it may swap them.
2. **The store buffer.** A core does not write straight into cache; stores wait in a small FIFO *store buffer* and drain later. A weakly ordered CPU (ARM, POWER) may drain `ready` to the cache before `data` (different cache lines, different readiness).
3. **Invalidation queues / cache lines arriving at different times.** The consumer core may receive the new line holding `ready` before the line holding `data` (which still shows the old 0 in its cache).
4. **Speculation.** The consumer's CPU may execute `load data` *before* `load ready` has resolved.

Which hardware does which (VERIFY for your exact core): **x86-64 (TSO)** keeps stores in order, loads in order, and only lets a later *load* pass an earlier *store* (that is the store buffer); so the message-passing bug cannot occur *in hardware* on x86 - only the compiler can cause it. **ARM/AArch64, POWER, RISC-V (weak)** may reorder almost everything unless told otherwise. The C++ memory model is written for the weakest machine so that one correct program runs everywhere. *That is why "it works on my laptop" proves nothing.*

**A timeline that fails (weak CPU, simplified).**

| t | Producer | Consumer | what the consumer's cache holds |
|---|---|---|---|
| 1 | `data = 42` -> *sits in store buffer* | | data=0, ready=0 |
| 2 | `ready = 1` -> *drains first* | | |
| 3 | | load ready -> **1** | ready=1 |
| 4 | | load data -> **0** (stale!) | data=0 |
| 5 | `data=42` finally reaches cache | | too late |

**The fix: release/acquire.**

```cpp
// Producer
data = 42;                                         // plain write
ready.store(true, std::memory_order_release);      // RELEASE: nothing above may move below this

// Consumer
while (!ready.load(std::memory_order_acquire)) {}  // ACQUIRE: nothing below may move above this
assert(data == 42);                                // guaranteed
```

**The contract (intuition, not the formal definition).** If an `acquire` load **reads the value written by** a `release` store to the *same atomic*, then everything the releasing thread did *before* the store is visible to the acquiring thread *after* the load. The standard says the store **synchronizes-with** the load, which creates a **happens-before** edge: all writes before the release *happen-before* all reads after the acquire. The compiler and CPU may then not reorder across the pair: they emit a barrier or a special instruction (`stlr`/`ldar` on AArch64; on x86 plain `mov` suffice - x86 already orders this - so it costs nothing but still stops the compiler's reordering).

**Key facts.**
- The pair is the contract: a `release` with no matching `acquire` on the other side orders nothing for anybody.
- It orders memory accesses *relative to this atomic*; it does not make the whole system sequentially consistent.
- Acquire/release is exactly "lock/unlock" as a one-way barrier: `mutex.unlock()` is a release, `mutex.lock()` an acquire. That is why data protected by a mutex needs no atomics.
- One-way barriers: code *before* a release may not sink below it, but code *after* it may move up above it (and symmetrically for acquire). Enough for publication; not enough for "I wrote mine; did you write yours?" (that needs `seq_cst`).

**Numerical-style demo of why `seq_cst` exists - the store-buffer litmus test.** Two atomics `x = y = 0`:

```
Thread 1:  x = 1;  r1 = y;       Thread 2:  y = 1;  r2 = x;
```

Which `(r1, r2)` outcomes are possible? Interleave the four operations (keeping each thread's order): (0,1), (1,0), (1,1) clearly. **(0,0)** needs both loads to execute before both stores became visible - exactly what a store buffer allows (each thread's store waits in its own buffer while its load reads the old value). With `seq_cst` (one global order) `(0,0)` is forbidden; with `relaxed`, `release/acquire` it is allowed - *even on x86*, because this is the one reordering TSO has. (Q9)

*What we take on trust:* the formal definition of happens-before and the proof that release/acquire pairs suffice for publication (see the "C++ concurrency memory model" literature); and the exact fence instructions each CPU emits. What you must be able to *use*: the pair, the contract, and the failure table above.

> **Common misconceptions**
> - *"Release/acquire makes the flag atomic."* The flag was already atomic. The orders control what happens to *other* memory around it.
> - *"If it works on x86 it's correct."* x86 hides most reorderings; ARM will not. The compiler may still reorder on x86.
> - *"`memory_order_relaxed` is a faster `seq_cst`, just use it."* Relaxed gives you no ordering; use it only for independent counters.
> - *"Acquire on the store side"* - `acquire` only applies to loads, `release` only to stores (RMWs can be `acq_rel`).
> - *"A `fence` is always needed."* Release/acquire on the atomic itself is usually clearer and cheaper.

### 3.5 Real-time rules: no heap, no locks, no exceptions on hot paths

**Concrete first.** An audio/vocoder thread must finish each 20 ms frame before the next arrives. Suppose the work takes: decode 3 ms + vocoder 6 ms + output 2 ms in the **worst case** = 11 ms. Utilisation 11/20 = 55%; **slack** = 9 ms. If *anything* in that thread can stall for more than 9 ms - even rarely - a frame is late and the listener hears a gap.

**Soft vs hard real time.** *Hard*: a missed deadline is a failure (flight control). *Soft*: occasional misses degrade quality (voice: a clipped syllable in a missed call can matter operationally, so treat it as *firm* - design for no misses). Either way the design target is the **worst-case execution time (WCET)**, not the average.

**The "hot path"** is code that runs under a deadline: the audio/vocoder callback, the radio-slot handler, the interrupt handler. The rules below apply there, not to startup or to the logging thread.

**Rule 1 - no heap (`new`, `malloc`, `vector::push_back`, `std::string` growth, `make_shared`, `std::function` with big captures).** Why? `malloc` has to find a free block: the search time depends on heap fragmentation (unbounded in general); the allocator usually takes an internal **lock** (so it inherits Rule 2); on a slow path it asks the OS for memory (`brk`/`mmap`), which can **page-fault** (the kernel zeroes a page: microseconds to milliseconds). Fast on average, unbounded in the worst case. *Numbers (illustrative, not measured):* a frame needs 20 allocations -> 50 frames/s x 20 = 1000 allocations/s; if one in a million hits a 12 ms slow path (> the 9 ms slack), the expected time until the first miss is 10^6 / 1000 = 1000 s ~ **17 minutes** (Q7). A test that "runs fine for 30 seconds" proves nothing.
*Instead:* allocate everything at startup (**preallocation**): `FixedPool<T,N>`, fixed-capacity containers (pack 07), `std::array`, `vector::reserve` once.

**Rule 2 - no locks (`std::mutex`, `lock_guard`, anything that can block).** Why? The hot path can be blocked for as long as the *other* thread holds the lock - and that thread may be pre-empted, descheduled or low priority (priority inversion, §3.2). Your WCET now includes someone else's WCET and the scheduler. *Instead:* single-producer/single-consumer lock-free structures (`Mailbox`, SPSC queue in pack 07), atomics, or `try_lock` with a defined fallback ("skip this statistics update").

**Rule 3 - no exceptions (`throw`).** Why? The first throw has to find a handler by walking unwind tables, running destructors on the way, usually allocating the exception object, and touching cold code and data (page faults / cache misses). Typical cost is thousands of times a normal call, and **unpredictable** (VERIFY by measuring on your target: the point is the variance, not one number). Many embedded builds use `-fno-exceptions` anyway (also saves code size). *Instead:* return codes/`std::optional`/`expected`-style results (pack 04: `parse_u16` returns `optional`), mark hot functions `noexcept` (an exception escaping a `noexcept` function calls `std::terminate` - a loud failure instead of silent latency).

**Rule 4 - no blocking system calls** (`printf`/`iostream` which lock and allocate, file I/O, `sleep`, `std::cout`, network). *Instead:* write a small record into a preallocated lock-free ring and let a non-real-time thread format and print it.

**Rule 5 - bounded work.** No unbounded loops or recursion (`while (!flag) {}` without a timeout, searching a growing `std::map`), no page faults (lock and pre-touch memory: `mlock`, `memset` at startup on POSIX).

**Also decide your scheduling.** On a real-time OS or Linux with `SCHED_FIFO`, priority decides who runs; a high-priority thread that busy-waits can starve the system. *(Config details: VERIFY on your platform.)*

**Checklist for any hot-path function:** heap? lock? throw? syscall/I-O? unbounded loop? calls something that does any of those?

> **Common misconceptions**
> - *"Real-time means fast."* It means *predictable*: a slow, bounded function is fine; a fast function that is occasionally 100x slower is not.
> - *"Lock-free means real-time safe."* A CAS loop can retry unboundedly; and a `std::atomic` of a big struct uses a lock.
> - *"`std::vector` after `reserve` never allocates."* True only while `size() <= capacity()`; an extra `push_back` still allocates.
> - *"Exceptions are free when not thrown."* The not-thrown path is cheap on modern ABIs, but the throw path is the problem on a hot path.
> - *"Our average latency is 50 microseconds, so we're fine."* Deadlines are broken by the tail.

---

## 4. Fully worked example: hand a voice frame from the RX thread to the vocoder thread

**Context.** The RX thread has just decoded a frame header and payload (a few words); the vocoder thread must pick it up. Constraints: **no heap, no locks, no blocking** on either side (§3.5). We use `Mailbox<Frame>` from `code/include/es_rt/mailbox.hpp` (the solution is below the stub in `solutions/`).

```cpp
struct Frame { std::uint32_t seq; std::uint32_t a; std::uint32_t b; };  // invariant: a == 2*seq, b == 3*seq
es::rt::Mailbox<Frame> mb;        // one slot + one std::atomic<bool> full_
```

**Step 1 - what must be guaranteed?**
(a) The consumer must never see a *half-written* frame (e.g. new `seq`, old `a`).
(b) The producer must not overwrite the slot while the consumer is still reading it.
(c) No locks and no allocation.

**Step 2 - the protocol (the `full_` flag is the baton).**
- `full_ == false`: the producer owns the slot. `full_ == true`: the consumer owns the slot. Ownership changes only by a store to `full_`.
- Producer: `if (full_.load(acquire)) return false;` `slot_ = frame;` `full_.store(true, release);`
- Consumer: `if (!full_.load(acquire)) return false;` `out = slot_;` `full_.store(false, release);`

**Step 3 - trace one round and name every happens-before edge.**

| # | Thread | Action | Why it is safe |
|---|---|---|---|
| P1 | Producer | `full_.load(acquire)` reads `false` (initial value) | slot is free |
| P2 | Producer | `slot_ = {7, 14, 21}` (three plain writes) | only the producer touches the slot now |
| P3 | Producer | `full_.store(true, release)` | **release:** P2 cannot sink below this |
| C1 | Consumer | `full_.load(acquire)` reads **`true` written by P3** | **acquire reads from the release: P3 synchronizes-with C1** |
| C2 | Consumer | `out = slot_` reads `{7,14,21}` | P2 *happens-before* C2, so all three writes are visible: no torn frame (a) |
| C3 | Consumer | `full_.store(false, release)` | C2 cannot sink below this |
| P1' | Producer | next `full_.load(acquire)` reads **`false` written by C3** | C3 synchronizes-with P1'; so C2 (the read) *happens-before* the next overwrite P2' (b) |

Both directions need the pair: P3->C1 protects the *payload*; C3->P1' protects the *slot reuse*. If C1 loaded `true` but the consumer saw old `slot_` contents, that is the failure table of §3.4.

**Step 4 - what goes wrong with `relaxed`.** Replace the two stores/loads by `relaxed`: P3 may become visible before P2's writes drain (weak CPU) or even be emitted *before* P2 (compiler). The consumer then reads e.g. `{7, 0, 0}`: `a != 2*seq`. The test `Mailbox.ProducerConsumerNoTornOrLostMessages` checks that invariant on 50,000 frames; on x86 the *hardware* often hides it, but **TSan reports a data race on `slot_`** because no happens-before exists.

**Step 5 - cost and limits.** Both calls are a few loads/stores: **wait-free** (no loop, no lock), O(1), no heap (tested by `AllocCounting.MailboxHotLoopMakesNoHeapAllocations`). Limits: exactly one producer and one consumer; one message in flight (a second `try_publish` returns false: the *caller* decides to drop or retry). To buffer more, use a ring - `SpscQueue` in pack 07 - same idea, two indices instead of one flag.

**Step 6 - sizing the buffer (Little's law).** Frames arrive at 50/s; the vocoder holds each for up to 70 ms: average frames in flight = 50 x 0.070 = **3.5**, so at least 4 slots; plus a 60 ms jitter burst (3 frames) -> 7 -> round up to a power of two (pack 07 explains why): **8**.

---

## 5. Questions

### 5.1 Explain why

**Q1.** *Context: the gateway's `frames_received` counter, incremented by the RX thread and the network thread, shows 1,273,518 instead of 4,000,000 in the lab.* Explain, step by step, why `counter = counter + 1` loses updates, and why making the variable `volatile` does not fix it.

**Q2.** *Context: a queue consumer thread sleeps until the protocol thread has pushed a decoded packet.* Why must `condition_variable::wait` be used with a predicate (or a `while` loop)? Name all three failure scenarios.

**Q3.** *Context: the RX thread fills a `Frame` and sets `ready`; the vocoder thread polls `ready`.* With `relaxed` on `ready`, why can the consumer see `ready == true` and still read old frame data? Give at least two different mechanisms, and explain why x86 hides one of them.

**Q4.** *Context: code review of a vocoder callback that takes a `std::mutex` and calls `malloc`.* Give two independent reasons each why locks and heap allocation are banned on a real-time path. Why is the *average* cost irrelevant?

**Q5.** *Context: a colleague wants to `throw` on a malformed frame inside the radio slot handler.* Why are exceptions a bad fit for hot paths, and what do you do instead?

### 5.2 Calculate

**Q6.** *Context: a 20 ms frame pipeline.* Worst-case stage times are 3 ms (RX decode), 6 ms (vocoder), 2 ms (output). Compute the CPU utilisation and the slack. What is the longest single blocking delay (lock wait, allocator stall) the thread can suffer without missing the deadline? A low-priority thread holds a shared mutex for up to 4 ms: is it safe to take it on this path? For up to 10 ms?

**Q7.** *Context: deciding whether "one malloc per frame" matters.* A frame does 20 allocations; frames arrive every 20 ms. One allocation in 10^6 takes the 12 ms slow path (slack 9 ms). (a) Expected time to the first deadline miss. (b) Expected misses in an 8-hour shift.

**Q8.** *Context: sizing a frame buffer between RX and vocoder.* Arrival rate 50 frames/s, each frame is held for 70 ms. (a) Average frames in flight (Little's law: L = lambda x W). (b) How many slots with a worst-case burst of 4 extra frames, rounded up to a power of two?

### 5.3 Predict

**Q9.** *Context: two threads signalling "I've started" to each other with atomics, `x = y = 0`.*

```
T1: x.store(1, ORDER); r1 = y.load(ORDER);       T2: y.store(1, ORDER); r2 = x.load(ORDER);
```

List the possible `(r1, r2)` pairs for ORDER = `seq_cst`; for ORDER = `relaxed`. Which one is the surprising outcome and which hardware feature explains it?

**Q10.** *Context: the call-table lock and the stats lock.*

```cpp
std::mutex a, b;
void f() { std::lock_guard<std::mutex> l1(a); std::this_thread::sleep_for(10ms); std::lock_guard<std::mutex> l2(b); }
void g() { std::lock_guard<std::mutex> l1(b); std::this_thread::sleep_for(10ms); std::lock_guard<std::mutex> l2(a); }
// thread 1 calls f(); thread 2 calls g()
```

What happens, and why does the `sleep_for` matter for *this demo* but not for the bug? Give two fixes.

**Q11.** *Context: a helper that starts a background task.*

```cpp
void start() { std::thread t([] { std::puts("hi"); }); }   // no join, no detach
int main() { start(); }
```

What happens? Does it matter if the thread already finished?

### 5.4 Trace by hand

**Q12.** *Context: lost update with three increments.* `counter = 10`. Thread 1 runs `counter = counter + 1` twice; Thread 2 runs it once. Interleaving: T1 load, T2 load, T1 add+store, T1 load, T1 add+store, T2 add+store. (Each "load" reads memory into a private register; "add+store" adds 1 to that register and writes it.) What is the final value? How many updates were lost?

**Q13.** *Context: `BoundedQueue` of capacity 1; a producer pushes 1, 2, 3 then calls `close()`; a consumer pops until `nullopt`.* Schedule: the producer runs first and gets as far as it can; then the consumer runs `pop`; then the producer runs again; and so on, alternating whenever a thread blocks. Write the timeline: queue contents, who is blocked on which condition variable, and the values the consumer receives. (Use `not_full_` and `not_empty_` as in `solutions/bounded_queue.hpp`.)

**Q14.** *Context: a start-up handshake with the bug removed from the predicate.*

```cpp
bool ready = false;
// Notifier:                                   // Waiter:
{ lock_guard l(m); ready = true; }             unique_lock l(m);
cv.notify_one();                               cv.wait(l);          // no predicate!
```

Trace an interleaving in which the waiter sleeps forever. Then show the one-line fix.

### 5.5 Find the bug

**Q15.** *Context: a consumer pop that "worked in testing".*

```cpp
std::unique_lock<std::mutex> lock(m);
if (q.empty()) cv.wait(lock);
Frame f = q.front();           // crash under load
q.pop_front();
```

Find the bug and describe a two-consumer interleaving that triggers it.

**Q16.** *Context: a call-table class.*

```cpp
class CallTable {
    std::mutex m_; std::map<int, Call> calls_;
public:
    void add(int id, Call c)  { std::lock_guard<std::mutex> l(m_); calls_[id] = c; }
    void replace(int id, Call c) { std::lock_guard<std::mutex> l(m_); remove(id); add(id, c); }
    void remove(int id) { std::lock_guard<std::mutex> l(m_); calls_.erase(id); }
};
```

What happens when `replace` is called? Give a fix that keeps all three functions public and thread-safe.

**Q17.** *Context: a shutdown flag.*

```cpp
bool stop = false;                               // plain bool
void worker() { while (!stop) { poll_radio(); } }
void shutdown() { stop = true; }                 // called from another thread
```

At `-O2` the worker never exits. Why? Why is `volatile bool` also wrong? Fix with the right type *and* memory orders.

**Q18.** *Context: a vocoder callback running in the real-time audio thread.* List every real-time violation and give the replacement for each.

```cpp
void on_frame(const int16_t* pcm, size_t n) {
    std::vector<int16_t> tmp(pcm, pcm + n);                 // (1)
    std::lock_guard<std::mutex> g(g_mutex);                 // (2)
    std::cout << "frame " << n << "\n";                     // (3)
    if (n != 160) throw std::runtime_error("bad length");   // (4)
    stats["frames"]++;                                      // (5) stats is std::map<std::string,int>
}
```

**Q19.** *Context: a "fire and forget" worker.*

```cpp
void compute() {
    int result = 0;
    std::thread t([&result] { result = heavy(); });
    t.detach();
    use(result);
}
```

List all the problems. Fix with `std::jthread` or `join`.

### 5.6 Implement

(All in `code/`; run tests with `ctest --test-dir build --output-on-failure`. Do **not** open `solutions/` until you have tried.)

**Q20.** `include/es_rt/bounded_queue.hpp` - blocking `BoundedQueue<T>` with `push`, `pop`, `close`. Predicate waits, notify after unlocking, close wakes both waiter groups. Tests: `test_queue.cpp`.

**Q21.** `include/es_rt/mailbox.hpp` - one-slot SPSC `Mailbox<T>` using only an atomic flag with the right acquire/release orders (§4). Tests: `test_atomics.cpp` (also run under TSan).

**Q22.** `include/es_rt/counters.hpp` - `EventCounter` (relaxed `fetch_add`) and `StopFlag` (release/acquire). For each memory order you choose, write one sentence of justification.

**Q23.** `include/es_rt/pool.hpp` - `FixedPool<T, N>`: no heap, `noexcept`, free-index stack, pointer-to-index validation, double-release detection. Tests: `test_pool.cpp` and `test_rt_alloc.cpp` (checks it never calls `operator new`).

### 5.7 Connect

**Q24.** *(Pack 03.)* `std::shared_ptr`'s reference count is atomic. Does that make `shared_ptr<Call>` safe to share between threads while both mutate the `Call`? What does it protect, and what doesn't it? Why is a `UniqueBox<Frame>` moved through a `BoundedQueue` a simpler ownership story?

**Q25.** *(Pack 05.)* Connect "data race is UB" to the strict-aliasing and alignment discussion: why can a racy read of a misaligned 32-bit field return a value that was *never written*? Why does `std::atomic<uint32_t>` require alignment?

**Q26.** *(Pack 07, preview.)* A ring buffer has `head` (written by the producer) and `tail` (written by the consumer). Which atomic loads/stores need acquire and which release, and why can each side read its *own* index with `relaxed`?

### 5.8 Interview-style

**Q27.** "When would you choose a mutex, an atomic, or a lock-free queue?" Give a decision rule with an Etherstack-style example for each.

**Q28.** "Explain acquire/release to a colleague who knows only mutexes." Use the mutex analogy and the message-passing pattern.

**Q29.** "Design the thread structure for a gateway: radio RX, protocol state machine, network, logging. What is shared, how, and which thread has real-time constraints?"

**Q30.** "What is priority inversion and how do you prevent it?" Include why `std::mutex` alone cannot solve it.

---

## 6. Self-assessment

- [ ] I can explain `++x` as load/add/store and show an interleaving that loses an update.
- [ ] I can state the definition of a data race and why it is UB.
- [ ] I always use a predicate with `condition_variable::wait`, and can name three reasons.
- [ ] I can produce a deadlock by hand and fix it with a lock order or `scoped_lock`.
- [ ] I know why `volatile` is not for threads.
- [ ] I can write the message-passing pattern with acquire/release and explain what each side guarantees.
- [ ] I can say which hardware reorderings exist on x86 vs ARM (and flag what to VERIFY).
- [ ] I can choose `relaxed`, `acquire/release` or `seq_cst` for a counter, a flag and a litmus test.
- [ ] I can list the real-time rules and give the worst-case reason for each.
- [ ] I have run the tests under ASan/UBSan **and** TSan, and the TSan run was clean.
- [ ] I implemented `BoundedQueue`, `Mailbox`, `EventCounter`/`StopFlag`, `FixedPool` before reading the solutions.
