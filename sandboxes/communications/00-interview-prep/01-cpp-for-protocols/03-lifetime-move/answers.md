# Pack 03 - Answers

Format: **Reasoning -> Answer -> Common wrong answers**. Reference code: `code/solutions/es_lifetime/`.

---

## Prerequisites

**P1.** The program touched memory that was already freed. The report has three stacks: where the bad access happened, **where it was freed** ("freed by thread T0 here"), and where it was allocated.
**P2.** `const uint8_t*`: the pointee is read-only, the pointer can move. `uint8_t* const`: the pointer is fixed, the pointee writable.
**P3.** A reference is an alias bound once, never null in valid code; a pointer is a re-seatable, nullable address.
**P4.** `new T` allocates and constructs one object, `delete` destroys and frees; `new T[n]`/`delete[]` handle arrays (and know the count to destruct). Mixing them is UB (ASan: `alloc-dealloc-mismatch`).

---

## 5.1 Explain why

**Q1.** *Reasoning:* the only thing C++ guarantees on *every* way out of a scope (return, break, exception) is destruction of automatic objects, in reverse construction order. During an exception, *stack unwinding* destroys each fully constructed local as it is left. So if release lives in a destructor, all 6 paths release without a single line of path-specific code. Manual calls need 6 correct call sites and future edits keep adding paths; `finally` needs to be written at each use site and doesn't compose with early returns across functions. *Answer:* RAII = release logic written once, run by the language. Corollary: destructors must not throw (throwing during unwinding calls `std::terminate`).
*Wrong:* "exceptions skip destructors for speed" - tempting because exceptions feel like a jump; wrong, unwinding calls them. "garbage collection does the same" - GC is non-deterministic and does not release locks/channels promptly.

**Q2.** *Reasoning:* the implicit copy constructor copies each member: `data_` (a pointer) is copied by value, so both objects hold the same address. Each destructor does `delete[] data_`, so the block is freed twice (ASan: `attempting double-free`). The invariant "I am the sole owner" was broken by the copy. A class that needs a destructor to free something is an owner; ownership semantics must then be defined for copy (deep copy or forbid) and assignment (same issue) - hence the rule of three; with move added, rule of five. *Wrong:* "the destructor is wrong" - tempting because the crash occurs there; the destructor is right, the copy is wrong. "Use `delete` once and mark" - leaks or dangles the other owner.

**Q3.** *Reasoning:* `push_back` on a full vector promises the strong guarantee: if an exception is thrown, the vector is unchanged. If elements are moved and the k-th move throws, the first k elements are already moved-from in the old storage; the old state cannot be restored. So the standard library uses `std::move_if_noexcept`: moves only when the move constructor is `noexcept` (or the type is move-only so copy isn't possible), else copies, which leaves the old storage intact. *Answer:* a non-`noexcept` move => reallocation copies all 10,000 frames (deep copies!). Fix: mark the move operations `noexcept` (they only swap pointers) or `= default`. *Wrong:* "vector never moves, it always copies" - older C++03 behaviour; "noexcept makes it faster by itself" - it only unlocks the move path.

**Q4.** *Reasoning:* `std::move(x)` is `static_cast<remove_reference_t<T>&&>(x)`: pure type conversion, no code for the machine. It makes overload resolution pick `T(T&&)`/`operator=(T&&)` if they exist; the *move constructor* does the stealing. For `const T`, the cast produces `const T&&`, which cannot bind to `T&&`; it binds to `const T&` (the copy ctor) so a copy happens - the demo test `ConstObjectCannotBeMovedFrom`. *Wrong:* "std::move empties the source" - tempting from its name; wrong, nothing happens unless a move constructor runs (`StdMoveAloneMovesNothing`).

---

## 5.2 Calculate

**Q5.** Capacities: push 1: 0->1 (nothing to relocate); push 2: full -> cap 2, relocate 1; push 3: full -> cap 4, relocate 2; push 5: full -> cap 8, relocate 4. (a) **7** relocations. (b) copy-only: 7 x 1500 = **10,500 bytes** (plus the 8 x 1500 = 12,000 for pushing lvalues, if you did not `std::move` them in). (c) With `noexcept` move: **0** payload bytes; 7 x 16 = **112 bytes** of handles. (Growth factor is implementation-specific: libstdc++ doubles, MSVC grows 1.5x.)
*Wrong:* "8 relocations" (counting the last insertion; no relocation happens when capacity suffices); "moving copies 1500 bytes too" (confusing with `std::array`).

**Q6.**

| stmt | strong (`use_count`) | weak observers | notes |
|---|---|---|---|
| (1) | 1 | 0 | one allocation (control block + Session) |
| (2) | 2 | 0 | |
| (3) | 2 | 1 | `w` does not raise the strong count |
| (4) | 1 | 1 | |
| (5) | 0 | 1 | **Session destructor runs here**; storage (merged with control block via `make_shared`) stays until `w` dies |
| (6) | `s` empty | 1 | `lock()` on expired returns an empty `shared_ptr` |

*Wrong:* "object freed when `w` dies" - tempting for the *storage* question; the *object* dies at strong==0.

**Q7.** 64-bit typical: `Buffer` 16; `UniqueBox<T>` 8; `shared_ptr<T>` 16 (object pointer + control block pointer); `vector<uint8_t>` 24 (begin, end, end-of-capacity); `array<uint8_t,1500>` 1500. A move copies the handle: 16/8/16/24 bytes - cheap, independent of payload. The array holds data inline: move = 1500-byte copy. For an SPSC queue prefer small handles (`UniqueBox<Frame>`) or index into a pre-allocated pool (pack 06). (Sizes are implementation-typical; VERIFY with `sizeof` on your target.)

---

## 5.3 Predict

**Q8.** `Probe a;` ctor 1. `Probe b = a;` copy_ctor 1. `Probe c = std::move(a);` move_ctor 1. `b = c;` copy_assign 1. `c = Probe(5);`: temporary constructed (ctor 2), move-assigned into `c` (move_assign 1), temporary destroyed (dtor 1). *Answer at HERE:* `ctor=2, copy_ctor=1, move_ctor=1, copy_assign=1, move_assign=1, dtor=1`. *Wrong:* `copy_ctor=2` for `Probe c = std::move(a)` (if you forget the cast, yes; with it, no); `dtor=0` (forgetting the temporary).

**Q9.** Output:
```
ctor a
ctor m1
ctor m2
Owner ctor
ctor b
dtor b
Owner dtor
dtor m2
dtor m1
end
dtor a
```
(`a` lives in `main`'s scope, so it dies after `end` is printed.)
Members construct in declaration order *before* the constructor body; destruction runs the destructor body first, then members in reverse. Locals destroy in reverse order of declaration (`b`, then `o`). *Wrong:* "Owner ctor" before `ctor m1` (thinking body first); `dtor m1` before `dtor m2`.

**Q10.** `vector<Probe>`: `move_ctor = 1, copy_ctor = 0` (relocation of the single old element). `vector<SlowMoveProbe>`: `move_ctor = 0, copy_ctor = 1` (copy because move can throw). The new element itself is default-constructed in place via `emplace_back()`, so it adds `ctor` only. *Wrong:* "both move" - the whole point of Q3.

**Q11.** Typical output `5 0` (all mainstream implementations leave a moved-from `std::string` empty). The standard only guarantees `t == "hello"` and that `s` is valid (you may call `s.size()`, assign, destroy) but its value is *unspecified*. *Wrong:* "`5 5` - the string was copied" - tempting since small strings are stored inline (SSO), but the move constructor still resets the source; "UB" - no, valid-but-unspecified is not UB.

---

## 5.4 Trace by hand

**Q12.** Blocks: X (x's 2 bytes), Y (y's 3 bytes).

| after | x | y | z | live |
|---|---|---|---|---|
| start | `{X,2}` | `{Y,3}` | - | 2 |
| (1) `y = x` | `{X,2}` | `{Y',2}` (new block; old Y freed) | - | 2 |
| (2) `z(move x)` | `{null,0}` | `{Y',2}` | `{X,2}` | 2 |
| (3) `x = move y` | `{Y',2}` | `{null,0}` | `{X,2}` | 2 |
| (4) `swap(x,z)` | `{X,2}` | `{null,0}` | `{Y',2}` | 2 |

(4): `tmp = move(x)` -> tmp `{Y'}`, x null; `x = move(z)` -> x `{X}`, z null; `z = move(tmp)` -> z `{Y'}`; `tmp` (null) destroyed. Three moves, zero allocations. *Wrong:* live count 3 after (1) (forgetting that copy-and-swap frees the old block) or believing std::swap copies.

**Q13.** After the two `make_shared`: x:1, y:1. `x->peer = y` -> y:2. `y->peer = x` -> x:2. Scope exit destroys locals in reverse: `y` handle (y:2->1), `x` handle (x:2->1). Neither reaches 0 -> `dtor` is **never printed**; both nodes leak (LeakSanitizer reports them). Fix: break the cycle: `std::weak_ptr<Node> peer` in at least one direction (or let a parent own children by `unique_ptr` and children use a raw/weak back pointer). *Wrong:* "`dtor` printed twice since locals are destroyed" - destroying a *handle* is not destroying the *object*.

---

## 5.5 Find the bug

**Q14.** (1) **Self-assignment**: `a = a` deletes `data_` and then reads `o.data_` - the same freed block: ASan `heap-use-after-free ... READ of size N ... in memcpy`, "freed by ... operator=". (2) **Not exception-safe**: if `new` throws (OOM), `data_` is already deleted but still set and `size_` already changed; the destructor later double-deletes. (3) Doesn't use `alloc_bytes` (live count wrong) - style. Fix: copy-and-swap (`Buffer tmp(o); swap(tmp); return *this;`) - handles both. *Wrong:* "add `if (this == &o) return *this;`" - fixes (1) only; (2) remains.

**Q15.** Two owners of one pointer: both destructors delete it -> `attempting double-free`. Also if `driver_open` allocates with something other than `new`, `delete` is the wrong deallocation (`alloc-dealloc-mismatch`). Fix: one owner, and give it the right deleter: `std::unique_ptr<Channel, decltype(&driver_close)> ch(driver_open(), &driver_close);`. *Wrong:* "use shared_ptr for both" - two separate `shared_ptr`s constructed from the same raw pointer have *separate* control blocks: same double free.

**Q16.** The lambda captures a raw `this`; the session is destroyed at t=2s; at t=5s the timer calls `on_timeout()` on a dead object: use-after-free (ASan `heap-use-after-free`, but only if the test reaches the timer). Fixes: (1) the session owns a `TimerHandle` (RAII) whose destructor cancels the timer - the cancel must be race-free vs. a callback running on the timer thread; (2) make the session `enable_shared_from_this`, capture `weak_from_this()` and in the callback `if (auto self = weak.lock()) self->on_timeout();`. *Wrong:* "capture by value `[=]`" - in a member function `[=]` still captures `this` (deprecated in C++20); "make `on_timeout` static" - doesn't remove the data access.

**Q17.** Using a moved-from `frame`: after `push(std::move(frame))` with a `Buffer`, `frame.size()` is 0 and `data()` is `nullptr`, so `frame.data()[0]` is a null dereference (and the log says 0 bytes). Even for types where move == copy it is a latent bug. Fix: read what you need *before* moving, or don't use the object after the move except to assign/destroy it. *Wrong:* "std::move is a no-op for reads" - only if the move constructor isn't called; with `push(const T&)` the move is just a copy.

---

## 5.6 Implement

**Q18.** See `solutions/es_lifetime/buffer.hpp`. Key points: destructor `free_bytes`; copy ctor `alloc_bytes(other.size_)` + `memcpy` guarded by `size_ != 0` (`memcpy(nullptr, nullptr, 0)` is UB); move ctor `std::exchange` both members, `noexcept`; copy assign by copy-and-swap; move assign frees own block, guarded by `this != &other`, then steals. Wrong: move ctor that copies the pointer but doesn't null the source (double free at scope end); move assign forgetting to free the old block (leak: `MoveAssignFreesOldAndEmptiesSource`); forgetting `noexcept` (`static_assert` and vector growth fail).

**Q19.** `~ScopeExit(){ if (active_) f_(); }`, `release(){ active_ = false; }`. Why LIFO: C++ destroys locals in reverse order. Wrong: calling `f_()` unconditionally (release ignored); copyable guard (callback runs twice) - copy is deleted. Caution: callback must not throw.

**Q20.** Destructor `delete p_`; move ctor `p_(std::exchange(other.p_, nullptr))`; move assign `if (this != &other) reset(other.release())`; `release()` = `exchange(p_, nullptr)`; `reset(p)` = swap in `p` *then* delete the old one. Wrong: `reset` deleting before installing (if the deleter re-enters the box you see a dangling pointer); `release` that deletes ("release" means *give up*, not *destroy*).

**Q21.** `add`: store a `weak_ptr` (`entries_.push_back(l)` converts). `notify`: (1) prune expired, (2) lock each into a local `vector<shared_ptr>` snapshot, (3) call `on_event` on the snapshot, return its size. The snapshot makes it safe when a callback calls `add()` (which could reallocate `entries_` mid-iteration -> iterator invalidation, ASan `heap-use-after-free`) and when a callback drops another listener's last owner (the snapshot keeps that listener alive for this round). Wrong: iterate `entries_` with `for (auto& w : entries_) if (auto s = w.lock()) s->on_event(code);` - re-entrancy invalidates the range-for; store `shared_ptr` in the registry (leaks listeners/extends lifetime).

---

## 5.7 Connect

**Q22.** Thread-safe: the control block's counts - copying/destroying *different* `shared_ptr` objects that share ownership from several threads is safe. Not thread-safe: reading and writing the *same* `shared_ptr` object concurrently (needs a mutex or C++20 `std::atomic<shared_ptr<T>>`), and any access to the pointee. For hard real-time: atomic RMW on a shared cache line has unbounded contention latency; creation allocates (heap, pack 06 §3.4); and the thread that drops the *last* reference runs the destructor and `free` - arbitrary work on a deadline-bound thread. *Wrong:* "shared_ptr is fully thread-safe" - only its bookkeeping.

**Q23.** `FixedVector<T,N>` stores objects in raw aligned bytes, so *nothing* constructs or destroys elements automatically: it needs a **destructor** that calls `~T()` on each live element, and copy/move constructors/assignments that construct elements one by one (a move can't steal a heap pointer - there is none: moving a `FixedVector` costs O(size) element moves). It takes over the job `std::vector` does for you: tracking which slots hold live objects and ending their lifetimes. Without it: leaks of whatever the elements own. *Wrong:* "it needs no destructor because it doesn't allocate" - elements may own resources.

**Q24.** UB means the standard places *no* requirements: freed memory often still contains the old bytes until the allocator reuses it, so a stale pointer "works" in tests (quiet heap, same allocation pattern) and corrupts something else in the field (busy heap, different thread timing). ASan (pack 01) quarantines freed blocks and poisons them, so the very first access reports with the free stack. *Wrong:* "a crash tells you quickly" - not guaranteed; silent corruption is the typical outcome.

---

## 5.8 Interview-style

**Q25.** Model answer: Rule of three: a class that needs a user-defined destructor, copy ctor or copy assignment (because it owns a resource) needs all three. Rule of five: C++11 adds move ctor/assign; declaring any of the five suppresses implicit moves, so provide or `= default` them deliberately. Rule of zero: prefer to own resources through members that already implement the rules (`vector`, `string`, `unique_ptr`), so your class declares none; fewer bugs and automatically correct. Prefer zero; five only for low-level resource wrappers; always mention `noexcept` moves and self-assignment. *Weak answer:* reciting the numbers with no reasoning.

**Q26.** Default `unique_ptr` (exclusive, zero overhead, move-only). `shared_ptr` only when ownership is genuinely shared with unclear last owner; costs: 16-byte handle, control block (often +1 allocation), atomic refcount traffic, cycle risk, hides design. Raw pointers/references for non-owning observation where the owner clearly outlives. `weak_ptr` for non-owning observation of shared objects. *Wrong:* "shared_ptr to be safe".

**Q27.** `std::move` is an unconditional cast to rvalue reference; moving happens in the move ctor/assign. A moved-from standard-library object is valid but unspecified: you can destroy it or assign a new value; calling functions with no preconditions (`size()`) is legal but the result is unspecified. For your own types define it (e.g. empty). Use-after-move logic bugs are a common review catch.

**Q28.** Because the move constructor isn't `noexcept` (and the type is copyable): the vector uses `move_if_noexcept` to preserve the strong guarantee. Fix: declare moves `noexcept` (only legal if they truly can't throw - pointer swaps), or `= default` them (deduced noexcept if members' moves are); verify with `static_assert(std::is_nothrow_move_constructible_v<T>)` and `Probe`-style counters.

**Q29.** One sensible design: a *pool* of preallocated frame slots (pack 06): the driver acquires a slot (no heap on the hot path), fills it, and hands a `UniqueBox`-like handle (or a slot index) into an SPSC queue; the decoder thread becomes the sole owner on pop; it hands data to the logger either by *copy* of the few bytes needed (logger must not hold the frame hostage) or by moving the handle into a second queue; the last owner returns the slot to the pool. State "exactly one owner at each point" and who frees. *Wrong:* "shared_ptr everywhere" (atomic costs, allocation in the driver path, unclear last owner) - or "pass raw pointers and hope".
