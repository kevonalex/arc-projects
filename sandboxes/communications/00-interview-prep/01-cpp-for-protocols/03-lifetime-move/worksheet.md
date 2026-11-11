# Pack 03 - Lifetime and move: RAII, rule of 0/3/5, move semantics, smart pointers

**Time:** 2-4 hours. **Code:** `code/` (stubs in `include/es_lifetime/`, reference in `solutions/`). Build as in `../README.md`.
Some tests (`test_move_demo.cpp`) pass from the start: they are *observations*; predict their numbers first (Q8-Q11).

---

## 1. Why this matters

A radio gateway runs for months. Everything it acquires - frame buffers, sockets, call contexts, DSP handles, timers - must be released exactly once, on every path, including error paths and exceptions. And it passes frames between components thousands of times per second, where an accidental 1500-byte copy per hop is a measurable cost.

Real failure modes from this pack's topics:

- **Leak on an early `return`:** one missing `delete` on the "bad CRC" path leaks 1500 bytes per corrupt frame; the gateway dies after a week.
- **Double free:** two objects believe they own the same buffer; the second `free` corrupts the allocator.
- **Use-after-free:** a timer callback fires after its session was destroyed.
- **Hidden copies:** a container reallocates and *copies* every element because a move constructor lacks `noexcept`.

C++'s answer to all of these is *deterministic destruction*: the language runs a function (the destructor) at a precisely defined moment. This pack builds the toolkit: RAII, the rules for copy/move, and smart pointers.

> **Safety:** all frames are synthetic; never connect any of this to live public-safety traffic.

---

## 2. Prerequisites check

- **P1.** What does an ASan report titled `heap-use-after-free` mean, and where in the report do you find *who freed it*? *(Pack 01 `../01-toolchain/` §3.4.)*
- **P2.** What is the difference between `const uint8_t*` and `uint8_t* const`? *(Pack 02 `../02-types-basics/` §3.6.)*
- **P3.** What is the difference between a reference and a pointer? Can a reference be null? *(Pack 02 §3.6.)*
- **P4.** What do `new T` / `delete p` and `new T[n]` / `delete[] p` do, and why must they be paired? *(If unsure, read §3.1 now; basics.)*

---

## 3. Concept build-up

### 3.1 Lifetime and storage

**Concrete first.**

```cpp
bool process(const std::uint8_t* wire, std::size_t n) {
    std::uint8_t* tmp = new std::uint8_t[n];
    copy(wire, wire + n, tmp);
    if (!crc_ok(tmp, n)) return false;     // LEAK: tmp never deleted on this path
    handle(tmp, n);                        // if this throws: LEAK again
    delete[] tmp;
    return true;
}
```

Every object has a **lifetime** that begins when its constructor finishes and ends when its destructor starts. Three storage kinds: **automatic** (locals; destroyed at end of the enclosing block, in *reverse order of construction*), **static** (whole program), **dynamic** (heap; lives until you `delete` it). For dynamic storage nobody ends the lifetime for you - that is the problem. Using an object outside its lifetime (before construction or after destruction) is UB (Q14-Q16).

Members are constructed in *declaration order*, then the constructor body runs; destruction is the exact reverse: destructor body, then members in reverse declaration order. (You will trace this in Q9.)

> **Common misconceptions**
> - *"Scope ends -> memory freed."* Only for automatic objects; a raw pointer going out of scope frees nothing.
> - *"Members are initialised in the order of the init list."* In declaration order (a classic warning `-Wreorder`).
> - *"Leaks only matter at shutdown."* In a long-running radio gateway a leak per frame is a crash.

### 3.2 RAII: resource acquisition is initialisation

**Concrete first.** The leaky function becomes:

```cpp
bool process(const std::uint8_t* wire, std::size_t n) {
    Buffer tmp(n);                   // constructor acquires
    std::memcpy(tmp.data(), wire, n);
    if (!crc_ok(tmp.data(), n)) return false;   // destructor of tmp runs here
    handle(tmp.data(), n);                      // ...and here if handle throws
    return true;                                // ...and here
}
```

**Derive.** Which language event is guaranteed to happen on *every* exit path - `return`, fall-off-the-end, `break`, **exception unwinding**? Only the destruction of automatic objects. So tie the release of the resource to the destruction of an object that owns it. Acquire in the constructor, release in the destructor; the language does the rest. Java/Python use garbage collection (non-deterministic timing; fine for memory, bad for sockets and locks) or `finally` (must be written at every use site); C++ chose deterministic destructors, which also work for *non-memory* resources: mutex locks (`std::lock_guard`, pack 06), file handles, hardware channel reservations.

`ScopeExit` (Q19) is the general form: "run this code whenever I leave the scope" with a `release()` to cancel on success - the *commit/rollback* idiom.

**Rule: destructors must not throw.** If a destructor throws during unwinding of another exception, the program calls `std::terminate`. Destructors are implicitly `noexcept`.

> **Common misconceptions**
> - *"RAII is only about memory."* Memory is the easy case; locks, handles, reservations are the point.
> - *"Exceptions make cleanup complicated."* With RAII, exceptions need *no* special cleanup code.
> - *"I'll just be careful to delete."* Humans miss paths; the compiler never does.

### 3.3 Rule of 3 / 5 / 0

**Concrete first - watch it fail.** Suppose `Buffer` owns a pointer and we write *only* a destructor:

```cpp
class Buffer { uint8_t* p_; public: Buffer(size_t n):p_(new uint8_t[n]){} ~Buffer(){ delete[] p_; } };
{
    Buffer a(8);
    Buffer b = a;     // compiler-generated copy: copies the POINTER VALUE
}                     // b's dtor frees the block; a's dtor frees the SAME block -> double free
```

ASan: `attempting double-free on 0x602000000010`.

**Derive.** A class that *owns* something has an invariant: "this pointer is mine alone". The compiler-generated copy copies each member bitwise - for a pointer that gives two owners. So if you write a destructor (you own something), the generated copy is almost certainly wrong. **Rule of three:** if you need a custom destructor, copy constructor or copy assignment, you almost certainly need all three. **Rule of five:** since C++11, add *move* constructor and *move* assignment: declaring a destructor or a copy operation *suppresses* the implicit move operations, so without them a "move" silently falls back to a deep copy.

| You declare... | Implicit copy ctor/assign | Implicit move ctor/assign |
|---|---|---|
| nothing | generated | generated |
| destructor | generated (deprecated!) | **not** generated -> moves become copies |
| copy ctor or copy assign | generated for the other copy op (deprecated) | **not** generated |
| move ctor or move assign | **deleted** | the other one is *not* generated |

**Rule of zero** is the goal: give each resource its own owning type (`std::vector`, `std::string`, `std::unique_ptr`, `Buffer`...), then your class composes them and declares *none* of the five - the generated ones are correct. Use `= default` to state it, `= delete` to forbid.

**Copy assignment pitfalls.** (1) *Self-assignment*: `a = a` with "free old, then copy from other" frees the source (Q14). (2) *Exception safety*: allocating the new block *before* releasing the old keeps the object valid if `new` throws. **Copy-and-swap** solves both: `Buffer tmp(other); swap(tmp); return *this;` - the temporary takes our old block and frees it on scope exit.

> **Common misconceptions**
> - *"The compiler's copy is fine, I only added a destructor."* It copies pointers, not what they point to.
> - *"`= default` for the destructor changes nothing."* It still suppresses implicit moves.
> - *"Rule of zero means no destructors anywhere."* It means *your* class doesn't write one; the member types do.

### 3.4 Move semantics: what is actually moved

**Concrete first.** A 1 MiB frame buffer `a` is returned from a decoder into `b = make_frame();`. A copy = allocate 1 MiB + `memcpy` 1 MiB. But the source is a temporary that dies on the next line; its block could simply be *handed over*: copy the 8-byte pointer and size, set the source's pointer to null. That is a move: O(1), independent of the size.

**Derive the mechanism.**

1. **Value categories (the minimum).** An *lvalue* has a name/identity (`a`); an *rvalue* is a temporary or something you declared expendable.
2. **Rvalue reference `T&&`** can bind to rvalues. This lets us write an overload `Buffer(Buffer&&)` that is chosen *only* when the source is expendable.
3. **`std::move(x)` is just `static_cast<T&&>(x)`.** It moves *nothing*; it says "I permit you to treat `x` as expendable". The actual stealing happens inside the move constructor/assignment *if one exists* (test `StdMoveAloneMovesNothing`).
4. **What the move constructor does:** copy the *handle* (pointer, size, capacity), then reset the source so its destructor won't free the stolen block.

**What is actually moved, by type:**

| Type | Move does | Cost |
|---|---|---|
| `Buffer`, `std::vector`, `std::string`(long) | copy pointer/size (2-3 words), null the source | ~16-32 bytes |
| `std::unique_ptr` | copy pointer, null source | 8 bytes |
| `int`, `double`, raw pointer | copy (nothing to steal) | size of the type |
| `std::array<uint8_t, 1500>` | element-wise move = copy of 1500 bytes (data is *inline*, there is no pointer to steal) | 1500 bytes |
| `const T` | **cannot** be moved from; falls back to copy (`ConstObjectCannotBeMovedFrom`) | copy |

**Moved-from state.** The standard requires moved-from standard-library objects to be *valid but unspecified*: you may destroy or assign to them, but must not assume contents. For your own types, define it - `Buffer` leaves `size()==0`, `data()==nullptr`.

**Why `noexcept` matters (the vector story).** `vector::push_back` on a full vector allocates new storage and relocates elements. The library promises the *strong guarantee*: if something throws, the vector is unchanged. If it *moved* elements and the k-th move threw, the earlier k-1 elements are already gone from the old storage - cannot roll back. So the library uses `std::move_if_noexcept`: **moves only if the move constructor is `noexcept`**, otherwise copies. Forgetting `noexcept` on your move turns every reallocation into deep copies (test `VectorGrowthMovesOnlyIfMoveIsNoexcept`).

**Copy elision.** `Buffer b = make();` with `return Buffer(...)` - since C++17 the prvalue is constructed directly in `b`: *zero* copies or moves (guaranteed). Never write `return std::move(local);` - it disables the optimisation and forces a move.

**Sink parameters.** For "I will keep this": `void set_payload(Buffer b) { payload_ = std::move(b); }` - callers pass an lvalue (one copy) or an rvalue (one move).

> **Common misconceptions**
> - *"`std::move` moves."* It casts (Q4).
> - *"After `std::move(x)`, `x` is destroyed/empty."* It is whatever the move constructor left; reading it is a logic bug.
> - *"Moving is always cheap."* Only for types that own indirection (`std::array` and small PODs: copy).
> - *"`const` makes things safer for return values."* `const Buffer f()` blocks moving from the returned value.
> - *"`T&&` in a template is an rvalue reference."* In `template<class T> void f(T&&)` it is a *forwarding* reference (pack 04).

### 3.5 Smart pointers

**Concrete first.**

```cpp
auto frame = std::make_unique<Frame>();         // sole owner; deleted at scope exit
auto shared = std::make_shared<Session>();      // reference counted
std::weak_ptr<Session> observer = shared;       // does not keep it alive
if (auto s = observer.lock()) { /* use s safely */ }
```

**`unique_ptr<T>`** = exclusive ownership. Size = one pointer (zero overhead, Q7); move-only (copy deleted - there can be only one owner); custom deleter for C handles (`unique_ptr<FILE, decltype(&fclose)>`). *Default choice.* You implement a minimal version in Q20.

**`shared_ptr<T>`** = shared ownership via a **control block** holding a strong count and a weak count (and the deleter). Copying increments the strong count *atomically*; the object is destroyed when it reaches 0. Costs: a second pointer per `shared_ptr` (16 bytes), atomic increments (contended cache line across threads), usually an extra allocation (`make_shared` merges the control block and object into **one** allocation - also prevents a leak if another argument's evaluation throws between `new` and the `shared_ptr` constructor). *The control block is thread-safe; the pointed-to object is not.*

**`weak_ptr<T>`** = non-owning observer of a shared object. `lock()` returns a `shared_ptr` (empty if expired), making "use if still alive" atomic. It also **breaks cycles**: if `A` holds `shared_ptr<B>` and `B` holds `shared_ptr<A>`, each count stays >= 1 forever -> leak (Q13).

**Raw pointers and references are fine as *non-owning* observers** when the owner provably outlives them. The rule is not "never use `T*`" but "never use `new`/`delete` to manage ownership by hand".

**When ownership really is shared** (e.g. several listeners keep a session alive) use `shared_ptr`; if you only need *notification without keeping alive*, use `weak_ptr` (Q21).

> **Common misconceptions**
> - *"`shared_ptr` is the safe default."* It hides ownership design, costs atomics, and enables cycles. Default to `unique_ptr`.
> - *"`shared_ptr` makes the object thread-safe."* Only the reference count is.
> - *"Passing `shared_ptr` by value everywhere is free."* Each copy is an atomic increment/decrement.
> - *"`weak_ptr` access is `ptr->`."* You must `lock()` first.
> - *"`unique_ptr` can't be put in containers."* It can - moved in (`push_back(std::move(p))`).

---

## 4. Fully worked example: following one resource through five operations

We watch `Buffer` (pointer + size). Blocks are named by who allocated them. `live` = number of heap blocks alive.

```cpp
Buffer a(4);               // (1)
Buffer b(a);               // (2) copy
Buffer c(std::move(a));    // (3) move construct
b = std::move(c);          // (4) move assign
a = b;                     // (5) copy assign into a moved-from object
```

| after | a | b | c | live | why |
|---|---|---|---|---|---|
| (1) | `{A,4}` | - | - | 1 | `alloc_bytes(4)` |
| (2) | `{A,4}` | `{B,4}` | - | 2 | copy ctor allocates its **own** block B and copies 4 bytes |
| (3) | `{null,0}` | `{B,4}` | `{A,4}` | 2 | move ctor: c takes A; a is reset. **No allocation, no byte copy** |
| (4) | `{null,0}` | `{A,4}` | `{null,0}` | 1 | move assign: b frees its old block B, takes A; c is reset |
| (5) | `{N,4}` | `{A,4}` | `{null,0}` | 2 | copy assign into empty a: allocate N, copy from A |
| end of scope | freed | freed | - | 0 | destructors run in reverse: c (nothing), b frees A, a frees N |

**What goes wrong with the compiler-generated copy at step (2):** `b = {A,4}`: both `a` and `b` hold block A; live stays 1; at scope end both destructors `delete[]` A -> ASan: `attempting double-free`. And with the "free old then copy" assignment at step (5) with `a = a`, the source is freed before being read (Q14).

**The same invariants as code** (key lines of `solutions/es_lifetime/buffer.hpp`):

```cpp
Buffer(Buffer&& o) noexcept : data_(std::exchange(o.data_, nullptr)), size_(std::exchange(o.size_, 0)) {}
Buffer& operator=(const Buffer& o) { Buffer tmp(o); swap(tmp); return *this; }   // copy-and-swap
```

`std::exchange(x, v)` returns the old `x` and assigns `v` - the idiom for "take and reset".

---

## 5. Questions

### 5.1 Explain why

**Q1.** *Context: a gateway function acquires a channel reservation and must release it on 5 different error paths and one exception.* Explain why tying release to a destructor is more reliable than calling `release()` manually or using `finally`, and what C++ guarantees about destructor timing during an exception.

**Q2.** *Context: a colleague added `~Buffer(){ delete[] data_; }` and nothing else; tests pass until a frame is queued.* Explain exactly why the implicit copy then causes a double free and why the "rule of three" follows.

**Q3.** *Context: `std::vector<Frame>` of 10,000 frames gets slow only after someone added a custom move constructor to `Frame`.* Explain why a move constructor without `noexcept` can make vector reallocation *copy*, and what guarantee the library is protecting.

**Q4.** *Context: code review comment: "you called `std::move` but nothing was moved".* Explain what `std::move` does and does not do, and why `std::move(const_obj)` does not move.

### 5.2 Calculate

**Q5.** *Context: sizing the cost of queue growth.* A `std::vector<Buffer>` starts empty and `push_back`s 8 elements, each holding 1500 bytes, capacity doubling (1, 2, 4, 8). (a) How many element *relocations* happen in total? (b) If `Buffer` has a copy constructor only (no move), how many payload bytes are `memcpy`d by relocation alone? (c) With a `noexcept` move constructor, how many payload bytes are copied, and how many bytes of "handle" (pointer+size, 16 B)?

**Q6.** *Context: tracing session lifetimes.* Give the strong count (`use_count()`) and the number of live `weak_ptr` observers after each statement (std does not expose the weak count; reason it out from the control-block description in §3.5), and say when the managed object is destroyed and when its *storage* is released:
```cpp
auto a = std::make_shared<Session>();   // (1)
auto b = a;                             // (2)
std::weak_ptr<Session> w = a;           // (3)
b.reset();                              // (4)
a.reset();                              // (5)
auto s = w.lock();                      // (6)
```

**Q7.** *Context: choosing a type to pass through an SPSC queue.* On a 64-bit platform give typical `sizeof` for: `Buffer` (ptr + size_t), `UniqueBox<T>`, `std::shared_ptr<T>`, `std::vector<uint8_t>`, `std::array<uint8_t,1500>`. Which are cheap to move and which are not, and why?

### 5.3 Predict

**Q8.** *Context: using `Probe` (see `probe.hpp`) to see what the compiler does.* After the line marked `// HERE`, give `ctor, copy_ctor, move_ctor, copy_assign, move_assign, dtor`:
```cpp
Probe::reset();
Probe a;
Probe b = a;
Probe c = std::move(a);
b = c;
c = Probe(5);     // HERE
```

**Q9.** *Context: object teardown order for a call context holding two sub-objects.* Predict the exact output:
```cpp
struct M { const char* n; M(const char* s):n(s){ std::printf("ctor %s\n", n);} ~M(){ std::printf("dtor %s\n", n);} };
struct Owner { M m1{"m1"}; M m2{"m2"}; Owner(){ std::puts("Owner ctor"); } ~Owner(){ std::puts("Owner dtor"); } };
int main() { M a{"a"}; { Owner o; M b{"b"}; } std::puts("end"); }
```

**Q10.** *Context: predicting container behaviour before measuring.* For `std::vector<Probe>` and `std::vector<SlowMoveProbe>`, each `reserve(1)`, `emplace_back()` once, reset counters, `emplace_back()` again: predict `move_ctor`, `copy_ctor`. Then run `MoveDemo.VectorGrowthMovesOnlyIfMoveIsNoexcept`.

**Q11.** *Context: reviewing logging code.* Predict (and say what the standard guarantees) for `std::string s = "hello"; std::string t = std::move(s); std::printf("%zu %zu\n", t.size(), s.size());`

### 5.4 Trace by hand

**Q12.** *Context: debugging a frame-queue wrapper.* Fill in a table like §4 for:
```cpp
Buffer x(2); Buffer y(3);
y = x;                 // (1)
Buffer z(std::move(x));// (2)
x = std::move(y);      // (3)
std::swap(x, z);       // (4)  (std::swap = move-construct, move-assign, move-assign)
```
Give `x, y, z` (block label & size) and `live` after each step.

**Q13.** *Context: a leak in a session manager.* Trace use counts and say what happens at scope exit; then fix it:
```cpp
struct Node { std::shared_ptr<Node> peer; ~Node(){ std::puts("dtor"); } };
{ auto x = std::make_shared<Node>(); auto y = std::make_shared<Node>();
  x->peer = y; y->peer = x; }
```

### 5.5 Find the bug

**Q14.** *Context: code review of a hand-written assignment operator.* Find all bugs; say what ASan prints for `a = a`:
```cpp
Buffer& Buffer::operator=(const Buffer& o) {
    delete[] data_;
    size_ = o.size_;
    data_ = new std::uint8_t[size_];
    std::memcpy(data_, o.data_, size_);
    return *this;
}
```

**Q15.** *Context: wrapping a pointer received from a C API in smart pointers.* Find the bug:
```cpp
Channel* raw = driver_open();
std::unique_ptr<Channel> a(raw);
std::unique_ptr<Channel> b(raw);
```

**Q16.** *Context: a session schedules a timeout on a shared timer service.* Find the bug and give two fixes:
```cpp
void Session::arm() { timer_.after(std::chrono::seconds(5), [this] { on_timeout(); }); }
// ... the session is destroyed after 2 seconds, the timer fires at 5.
```

**Q17.** *Context: a decoder hands frames to a worker queue.* Find the bug:
```cpp
queue.push(std::move(frame));
log("queued %zu bytes", frame.size());
if (frame.data()[0] == 0x7E) { /* ... */ }
```

### 5.6 Implement

(`code/include/es_lifetime/`; run `ctest`.)

**Q18.** *Context: frame buffers owned by value.* Implement the five special members of `Buffer` (`buffer.hpp`). Use `detail::alloc_bytes`/`free_bytes`. Moves must be `noexcept` and leave the source empty; copy assignment must be self-assignment safe.

**Q19.** *Context: commit/rollback of a channel reservation.* Implement `ScopeExit::~ScopeExit` and `release()` (`scope_exit.hpp`).

**Q20.** *Context: exclusive ownership of a call context.* Implement `~UniqueBox`, the move constructor, move assignment, `release()`, `reset()` (`unique_box.hpp`).

**Q21.** *Context: logger/recorder/billing components listen for call events; their owners decide when they die.* Implement `CallRegistry::add` and `notify` (`call_registry.hpp`) so it never extends a listener's lifetime, never calls a dead listener, and tolerates `add()` from inside a callback.

### 5.7 Connect

**Q22.** *Context: pack 06 (`../06-concurrency-rt/`) - a decoder thread and a UI thread both hold a `shared_ptr<Session>`.* Which parts of `shared_ptr` are thread-safe and which are not? Why is `shared_ptr` a poor fit for a hard-real-time audio path?

**Q23.** *Context: pack 07 (`../07-sm-builds/`) builds `FixedVector<T,N>` with placement new.* Which of the five special members does `FixedVector` need, and which RAII responsibility does it take on that `std::vector` normally handles for you?

**Q24.** *Context: pack 05 (`../05-ub-layout-bit/`).* Why is a use-after-free "undefined behaviour" rather than "a crash", and why can the same bug pass your tests but fail in the field? Which pack-01 tool finds it?

### 5.8 Interview-style

**Q25.** "Explain the rule of three, five and zero. Which do you prefer and why?"

**Q26.** "`unique_ptr`, `shared_ptr`, raw pointer: how do you decide? What's the cost of `shared_ptr`?"

**Q27.** "What does `std::move` do? What is the state of an object after it's been moved from?"

**Q28.** "Why might `std::vector` copy your elements instead of moving them on growth, and how do you fix it?"

**Q29.** "Design the ownership of a frame buffer that is filled by a radio interrupt/driver, handed to a decoder thread, then to a logger. Who owns it at each step?"

---

## 6. Self-assessment

- [ ] I can explain RAII and why destructor timing makes exception-safe cleanup automatic.
- [ ] I can show how a defaulted copy of an owning pointer double-frees, and derive the rule of three/five.
- [ ] I can state what `std::move` does (cast) and what the move constructor does (steal + reset).
- [ ] I can explain why `noexcept` moves matter for `std::vector`.
- [ ] I can write `Buffer` (rule of five) and `UniqueBox` from memory.
- [ ] I can choose between `unique_ptr`, `shared_ptr`, `weak_ptr`, raw observer, and explain cycle leaks.
- [ ] I can predict the Probe counts in Q8 and Q10 and confirm with the tests.
- [ ] All tests pass with sanitizers on (including leak detection).
- [ ] I can answer Q25-Q29 aloud.
