# Pack 06 cheat sheet - every line points back to the worksheet

**Threads and races**
- `x = x + 1` = load, add, store; interleaving loses updates - §3.1 (Q1, Q12)
- data race = same location + >=1 write + not atomic + unordered -> UB - §3.1
- `volatile` is not atomic and orders nothing; never for threads - §3.1 (Q17)
- joinable `std::thread` destroyed -> `std::terminate`; use `join` or `std::jthread` - §3.1 (Q11, Q19)
- safety order: don't share (messages) > mutex > atomic - §3.1

**Mutex / condition variable**
- `lock_guard` simple, `unique_lock` for cv, `scoped_lock` for several mutexes - §3.2
- `std::mutex` is not recursive: relocking = UB/deadlock; split into `_locked` helpers - §3.2 (Q16)
- deadlock = circular wait; fix with one global lock order or `scoped_lock(a, b)` - §3.2 (Q10)
- never call unknown code (callbacks, I/O) under a lock - §3.2
- `cv.wait(lock, pred)` == `while (!pred()) wait`; cv is a doorbell with no memory - §3.2 (Q2, Q14, Q15)
- three failures without predicate: lost, spurious, stolen wake-up - §3.2 (Q2)
- change the state under the same mutex the waiter uses - §3.2
- `close()` must `notify_all` every waiter group - §3.2 (Q13, Q20)
- priority inversion: H waits for L, M pre-empts L; fix = priority inheritance - §3.2 (Q30)

**Atomics**
- atomic ops are indivisible; `fetch_add` is one read-modify-write - §3.3 (Q22)
- `atomic<int> x; x = x + 1` is two operations (still racy); use `fetch_add`/`++x` - §3.3
- CAS = "replace if still expected"; loops can retry (lock-free != wait-free) - §3.3
- check `is_lock_free()`; non-lock-free atomics hide a mutex - §3.3
- atomics need natural alignment (single access in one cache line) - §3.3 (Q25)
- ABA: A->B->A fools CAS; SPSC avoids it with single writers - §3.3
- `relaxed` = atomicity only (counters); `release/acquire` = publish/see; `seq_cst` = global order - §3.3

**Acquire/release**
- message passing: data then `store(release)`; `load(acquire)` then data - §3.4 (Q3, Q21)
- reordering sources: compiler, store buffer, cache-line timing, speculation - §3.4 (Q3)
- x86 TSO reorders only store->load; ARM/POWER/RISC-V reorder almost all (VERIFY per core) - §3.4
- release without a matching acquire orders nothing - §3.4
- `unlock` = release, `lock` = acquire - §3.4 (Q28)
- store-buffer litmus: (0,0) allowed by relaxed/acq-rel, even on x86; forbidden by `seq_cst` - §3.4 (Q9)
- mailbox needs the pair BOTH ways: payload publish and slot reuse - §4, Q21
- SPSC indices: read the other side's with acquire, publish yours with release, own index relaxed - §3.4, Q26
- x86 "passes" proves nothing: run TSan - §3.4, §4

**Real-time rules**
- design for worst case (WCET), not average; slack = period - sum of WCET - §3.5 (Q6)
- no heap: unbounded search, internal lock, OS calls, page faults; preallocate - §3.5 (Q7, Q23)
- rare slow path at 1000 allocs/s hits within minutes (10^6 / 1000 s ~ 17 min) - §3.5 (Q7)
- no locks: wait for holder + inversion; use SPSC / atomics / `try_lock` fallback - §3.5 (Q18)
- no exceptions: throw path slow and variable; return `optional`/codes, mark `noexcept` - §3.5 (Q5)
- no blocking syscalls or iostream; log via lock-free ring to a non-RT thread - §3.5 (Q18)
- bounded loops, no page faults (`mlock`, pre-touch) - §3.5
- hot-path checklist: heap? lock? throw? syscall? unbounded loop? - §3.5
- buffer sizing: Little's law `L = lambda * W`, add burst, round to power of two - §4, Q8

**Tools**
- tests under ASan/UBSan **and** TSan (`-DENABLE_TSAN=ON`, not combinable) - header, §3.4
- count heap use by replacing `operator new` in a separate test exe - §3.5, Q23
- `shared_ptr` count is atomic, the pointee is not - §3.1, Q24
