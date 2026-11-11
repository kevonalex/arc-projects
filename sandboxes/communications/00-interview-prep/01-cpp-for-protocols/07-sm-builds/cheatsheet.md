# Pack 07 cheat sheet - every line points back to the worksheet

**State machines**
- state = summary of history; booleans give 2^n combos, enum gives only legal ones - §3.1 (Q1)
- `step(state, event) -> {next, action, handled}` is pure; the wrapper stores state - §3.1 (Q19, Q21)
- return actions as data; the caller performs them (testable without a radio) - §3.1
- outer `switch (state)`: NO `default`, `-Wswitch` flags a new state; inner `default` = "ignored" - §3.1 (Q2, Q14)
- `to_string`: switch over all enumerators + `return "?"` for out-of-range - §3.1 (Q18)
- count ignored events: a spike is a diagnostic - §3.1 (Q11)
- switch = readable/reviewable, table = compact/data-driven but loses exhaustiveness warnings - §3.1 (Q7)
- test properties by walking the graph: reachable, no trap state, Talking only via FloorGranted - §3.1 (Q34)
- hang time keeps the channel reserved for a reply (duration is system config, VERIFY) - §3.1

**variant / visit**
- variant = tagged union that keeps tag and storage in sync; size = largest alternative + index - §3.2 (Q6)
- default-constructed variant holds the FIRST alternative - §3.2
- `get_if` -> pointer/nullptr; `holds_alternative` -> bool; `std::get` throws `bad_variant_access` - §3.2 (Q9)
- `visit` is compile-time exhaustive; a catch-all lambda / `switch(index())` loses that - §3.2 (Q20)
- `overloaded` = inherit all lambdas + `using Ts::operator()...` + deduction guide; same return type - §3.2
- trivially copyable alternatives -> variant can go through the SPSC queue by copy - §3.2, §3.4
- enum for payload-free labels, variant for typed payloads; combine via `to_event` - §3.2 (Q32)

**Ring buffer**
- free-running counters: `size = head - tail`, full when `size == N`, no wasted slot - §3.3
- slot = `counter & (N-1)` ; `x mod 2^k` = low k bits (13 = 1101 -> 13 & 7 = 5) - §3.3 (Q3)
- power of two is a CORRECTNESS rule: wrap point 2^bits must be a multiple of N - §3.3 (Q33)
- N = 6, 8-bit counter: slots 2, 3, 0 -> slots 4, 5 skipped; N = 8: 6, 7, 0 - §3.3 (Q3, Q15)
- unsigned subtraction handles wrap: (3 - 250) mod 256 = 9 - §3.3 (Q5)
- narrow types: `static_cast<Index>(head - tail)` (integer promotion makes it `int`) - §3.3 (Q15, Q22)
- `push_overwrite`: if full, `++tail` (drop oldest); `[i]` = `(tail + i) & mask` - §3.3 (Q10, Q23)
- compile-time guards: `static_assert` power of two, N <= max(Index) - §3.3 (Q15)

**SPSC queue**
- one writer per variable: `head_` producer-only, `tail_` consumer-only - §3.4
- own index `relaxed`, other's `acquire`, publish own with `release` - §3.4 (Q4, Q24)
- element write happens-before `head_` release; read happens-before `tail_` release - §3.4, §4 (Q27)
- stale index is safe (only errs toward full/empty); never believe MORE room/data than exists - §3.4 (Q4)
- two producers = data race: use one queue per producer or CAS-based MPSC - §3.4 (Q16)
- `alignas(64)` head_/tail_/buf_ against false sharing (line size VERIFY) - §3.4 (Q16, Q29)
- full policy belongs to the caller: `try_push` returns false, never blocks - §3.4 (Q31)
- `size_approx`: load `tail_` before `head_`, clamp to N - §3.4

**FixedVector**
- storage and object lifetime are different: raw `alignas(T) std::byte[]` + `size_` live count - §3.5 (Q25)
- create = placement new `::new (addr) T(args...)`; destroy = `p->~T()`; neither allocates/frees - §3.5
- `emplace_back`: perfect-forward, `++size_` only AFTER construction succeeds - §3.5 (Q25)
- element access through `std::launder(reinterpret_cast<T*>(...))` - §3.5 (Q29)
- `p[i] = v` on unconstructed bytes calls `operator=` on garbage (string frees a junk pointer) - §3.5 (Q17)
- destructor/`clear` destroy exactly the LIVE `size_` elements, last to first - §3.5 (Q17, Q25)
- copy ctor: `try { copy each } catch (...) { clear(); throw; }` (dtor won't run for a failed ctor) - §3.5 (Q26)
- move ctor moves each element (inline, nothing to steal), then empties the source - §3.5 (Q26, Q30)
- return `nullptr`/`false` when full, no exceptions on hot paths - §3.5

**Pipeline and tools**
- radio thread -> `SpscQueue<Msg>` -> protocol thread -> `CallMachine` (no locks, no heap) - §4 (Q27)
- run ASan/UBSan and TSan (`-DENABLE_TSAN=ON`); narrow `Index` types force wrap-around tests - header, §3.3
- size the buffer: Little's law + burst, round up to a power of two - §4 (Q31)
