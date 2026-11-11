# Pack 03 cheat sheet - every line points back to the worksheet

**Lifetime**
- automatic objects destroyed at scope end, reverse construction order - §3.1 (Q9)
- members construct in DECLARATION order; destroy in reverse; dtor body runs before members - §3.1
- using an object outside its lifetime = UB - §3.1 (Q14-Q16)

**RAII**
- acquire in constructor, release in destructor; runs on return, break AND exception unwinding - §3.2
- destructors must not throw (throw during unwinding -> `std::terminate`) - §3.2
- `ScopeExit` = run-at-scope-exit with `release()` to cancel (commit/rollback) - §3.2, Q19

**Rule of 3/5/0**
- compiler copy of an owning pointer copies the pointer -> double free - §3.3, §4
- need a dtor/copy => define all of dtor, copy ctor, copy assign (3); add move ctor/assign (5) - §3.3
- declaring dtor or copy ops suppresses implicit move => silent deep copies - §3.3
- prefer rule of 0: own via `vector`/`string`/`unique_ptr` members - §3.3
- copy assign: use copy-and-swap (self-assign safe + exception safe) - §3.3 (Q14)

**Move semantics**
- `std::move(x)` = cast to `T&&`; moves nothing; move ctor steals - §3.4 (Q4)
- move = copy handle (ptr,size), reset source; `std::exchange(x, v)` is the idiom - §3.4, §4
- inline data (`std::array`, small POD) is copied on "move" - §3.4 (Q7)
- moved-from: valid but unspecified (std types); define yours (empty/null) - §3.4 (Q11, Q17)
- mark moves `noexcept` -> vector moves on growth, else copies (`move_if_noexcept`) - §3.4 (Q3, Q10)
- `const` objects can't be moved from; `return std::move(local)` blocks elision - §3.4
- C++17 guaranteed elision of prvalues: zero copies/moves - §3.4

**Smart pointers**
- `unique_ptr`: exclusive, size of a raw pointer, move-only, custom deleter for C handles - §3.5, Q20
- `shared_ptr`: control block (strong+weak counts), atomic refcount, 16 bytes - §3.5 (Q6, Q7)
- `make_unique`/`make_shared`: exception-safe, `make_shared` = 1 allocation - §3.5
- `weak_ptr`: `lock()` then use; breaks `shared_ptr` cycles - §3.5 (Q13, Q21)
- shared_ptr bookkeeping is thread-safe, the pointee and a shared `shared_ptr` object are not - §3.5, Q22
- raw pointer/reference = non-owning observer only - §3.5
- never construct two smart pointers from one raw pointer - Q15
- callbacks must not capture a raw `this` that can die first - Q16

**Debug**
- double free / use-after-free show in ASan with allocation+free stacks - §3.3, Q24
- LeakSanitizer finds cycles and forgotten owners - §3.5, Q13
