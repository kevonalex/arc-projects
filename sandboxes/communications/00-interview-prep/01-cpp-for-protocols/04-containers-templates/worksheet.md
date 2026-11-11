# Pack 04 - Containers and templates: array, vector, span, string_view, optional, variant, templates

**Time:** 2-4 hours. **Code:** `code/` (needs C++20 for `std::span`; stubs in `include/es_containers/`, reference in `solutions/`).
**C++17 note:** replace `std::span<const T>` with `gsl::span<const T>` (Guidelines Support Library) or a 10-line `struct { const T* p; size_t n; }`. Everything else here is C++17.

---

## 1. Why this matters

Protocol stacks are made of *sequences of bytes of varying length* and *values that may be absent or one-of-several-kinds*:

- A received frame is `N` bytes, `N` unknown until you read the header. You need a type that carries the pointer *and* the length, so you cannot get them out of sync.
- Parsing a frame must not copy it five times on the way up the stack (cost) or run `malloc` per layer (latency spikes on a real-time path).
- A decoder field may be "not present in this PDU": `-1`? `0`? When all 65,536 values of a 16-bit field are valid, no spare value remains for "absent".
- A gateway config/control message is "an int, or a string, or empty".
- Fixed-capacity pieces (queues, stacks, buffers) must be parametrised by *type and size* so the compiler allocates them statically - the topic of templates.

Failure modes: out-of-bounds `buf[len]` from a length field you trusted; a `string_view` pointing at a destroyed `std::string`; pointers into a `vector` that reallocated; a "not found" sentinel that collides with a legal value.

> **Safety:** all frames and strings here are synthetic. Never connect these exercises to live public-safety traffic.

---

## 2. Prerequisites check

- **P1.** Why is `v.size() - 1` dangerous on an empty `std::vector`, and what does it evaluate to? *(Pack 02 `../02-types-basics/` §3.3.)*
- **P2.** What happens to a `std::vector<Buffer>` on reallocation if `Buffer`'s move constructor isn't `noexcept`? *(Pack 03 `../03-lifetime-move/` §3.4.)*
- **P3.** What is a dangling reference/pointer? Give one way to create one. *(Pack 02 §3.6; pack 03 Q16.)*
- **P4.** What does the range `[begin, end)` mean and why is it half-open? *(Pack 02 §3.6.)*
- **P5.** What will ASan print if you write `v[v.size()]` on a `std::vector<uint8_t>`? *(Pack 01 §3.4.)*

---

## 3. Concept build-up

### 3.1 `std::array` and C arrays

**Concrete first.**

```cpp
void f(std::uint8_t buf[4]) { std::printf("%zu\n", sizeof(buf)); }   // prints 8 on a 64-bit machine!
std::uint8_t a[4]; f(a);
```

**Derive.** In C a function parameter `T buf[4]` is *rewritten* to `T* buf`: arrays *decay* to a pointer to the first element and lose their size (C chose this so passing an array would not copy it). `std::array<T, N>` is a thin wrapper: a real aggregate containing `T[N]`, so it keeps its size in the *type*, can be copied/returned by value, compared with `==`, used in `constexpr`, and has `.at(i)` (bounds-checked, throws) and `.size()`. It has zero overhead: `sizeof(std::array<std::uint8_t,4>) == 4`, no heap.

**Use when** the size is a compile-time constant (a 4-byte ID, a 20-byte header). **Failure if ignored:** `sizeof(buf)/sizeof(buf[0])` on a decayed parameter silently gives the pointer-size ratio (Q16).

> **Common misconceptions**
> - *"`std::array` is slower than a C array."* Same machine code.
> - *"`array<T,N>::operator[]` checks bounds."* No (only `.at()` does).
> - *"A C array parameter keeps its size."* It is a pointer.

### 3.2 `std::vector`

**Concrete first.** A vector is three pointers: `begin`, `end` (one past last element), `end_of_storage`. `size() = end - begin`, `capacity() = end_of_storage - begin`. Pushing when `size == capacity` allocates a bigger block, relocates the elements, frees the old block.

**Derive the growth rule.** If we grew by +1 each time, pushing n elements relocates 1 + 2 + ... + (n-1) = n(n-1)/2 elements - for n = 1000 that is **499,500** element relocations. If we *double* the capacity, relocations happen at sizes 1, 2, 4, ..., 512 and cost 1 + 2 + ... + 512 = **1,023** relocations for 1000 pushes - about one extra copy per element. The geometric sum 1 + 2 + 4 + ... + 2^k < 2^(k+1) is why `push_back` is called *amortised O(1)*. (libstdc++ doubles; MSVC grows by 1.5x - any factor > 1 gives amortised O(1).)

**`reserve(n)`** allocates once up front. On a real-time path you reserve at start-up and never exceed - growth means `malloc` on the hot path (pack 06).

**Invalidation rules (memorise by deriving):** anything that reallocates moves every element, so all pointers/references/iterators into the vector become dangling. `push_back` within capacity does not reallocate, but the `end()` iterator changes. `erase(it)` shifts later elements down: iterators/references at or after `it` are invalid. Failure in code: holding `int* p = &v[0]` then `v.push_back(...)` (Q8, Q14).

**`operator[]` does not check.** `v[v.size()]` is UB (ASan reports `heap-buffer-overflow`); `v.at(i)` throws `std::out_of_range` (not usable if exceptions are disabled - §3.5 of pack 06).

**Erase-remove idiom.** Algorithms (`std::remove`) work on *iterator pairs*; they cannot change a container's size. `std::remove(first,last,x)` moves the keepers to the front, preserves their order, and returns the new logical end; elements past it are in a valid-but-unspecified state. Only the container's `erase(new_end, end)` changes `size()`. Hence: `v.erase(std::remove(v.begin(), v.end(), x), v.end());` (Q13). (C++20: `std::erase(v, x)`.)

> **Common misconceptions**
> - *"`remove` removes."* It reorders; the size is unchanged.
> - *"A pointer into a vector is stable."* Only until the next reallocation.
> - *"`clear()` frees memory."* It destroys elements; `capacity()` is unchanged.
> - *"`vector<bool>` is a normal vector."* It is bit-packed and returns proxies; avoid for bytes.

### 3.3 `std::span`

**Concrete first.** Instead of the pair `(const uint8_t* p, size_t n)`:

```cpp
using ByteView = std::span<const std::uint8_t>;
std::optional<ByteView> frame_payload(ByteView frame);      // one parameter; length cannot be wrong
std::vector<std::uint8_t> v = ...;  ByteView all(v);       // views a vector without copying
ByteView head = all.first(4);  ByteView mid = all.subspan(2, 3);
```

**Derive.** A span is a *non-owning view* of a contiguous range: a pointer plus a length (16 bytes for dynamic extent; `span<T,N>` with compile-time `N` stores just the pointer). Because the pair travels together, a function cannot be handed a mismatched length; `subspan`/`first`/`last` create sub-views in O(1) with no copy and no allocation. `std::span<const T>` is the read-only form - the const-correctness of pack 02 applied to ranges. Spans convert implicitly from C arrays, `std::array`, `std::vector`.

**What it does *not* do.** It does not own: if the vector it views reallocates or dies, the span dangles (Q9, Q15). `operator[]` is unchecked (UB out of range; `subspan` with bad arguments is UB). *Therefore* write checked helpers (`sub_span` in Q18) at trust boundaries - where the numbers come off the wire.

**Overflow-safe range checks.** Validate `offset <= size` first, then `count <= size - offset`. The tempting `offset + count <= size` can wrap around when `count` is huge (pack 02: unsigned wrap): `2 + SIZE_MAX == 1` -> "1 <= 4" passes the check -> a wild view.

> **Common misconceptions**
> - *"span is a smart pointer."* It owns nothing.
> - *"span is bounds-checked."* Not by default.
> - *"`span<const T>` and `const span<T>` are the same."* The first has const *elements*; the second only a const *view object*.

### 3.4 `std::string_view`

**Concrete first.** Splitting `"17,42,x"` without allocating a single string: each field is a `(pointer, length)` pointing *into* the original text.

**Derive.** `string_view` is `span<const char>` plus text-flavoured operations (`find`, `substr`, `starts_with`, comparison). `substr` is O(1) (adjust pointer/length) - unlike `std::string::substr`, which allocates a *new std::string*. A function taking `const std::string&` forces callers holding a literal or a view to build a temporary `std::string` (heap allocation); `string_view` accepts all of them.

**Traps (each = lifetime bug, pack 03):**
1. **Not null-terminated.** A view of `"abc,def"`'s first field is `"abc"` but the byte after it is `','`. Passing `sv.data()` to a C function expecting a terminated string reads on until a `\0` - copy into a `std::string` first.
2. **Dangling:** `std::string_view sv = std::string("temp");` - the temporary dies at the semicolon; `sv` points at freed memory (Q9). Literals are fine (static storage).
3. **`std::string::substr` returns a std::string**; assigning that to a view dangles. Slice a *view*: `std::string_view(s).substr(...)`.
4. A `std::string` that is appended to may reallocate: views into it dangle.

> **Common misconceptions**
> - *"string_view is a safer `const string&`."* It is *less* safe about lifetime; it is cheaper and more flexible.
> - *"`sv.data()` is a C string."* Not terminated.
> - *"Returning string_view is fine."* Only if it views something that outlives the call.

### 3.5 `std::optional`

**Concrete first.**

```cpp
std::optional<std::uint16_t> parse_u16(std::string_view s);   // "65535" -> 65535, "x" -> nullopt
if (auto v = parse_u16(txt)) use(*v);
std::uint16_t d = parse_u16(txt).value_or(0);
```

**Derive.** A function that may fail to produce a value needs a way to say so. Options: (a) sentinel (`-1`, `0xFFFF`) - collides with legal values (all 65,536 `uint16_t` values are valid talkgroups?), invites unchecked use; (b) out-parameter + bool - clumsy; (c) exceptions - often disabled or banned on real-time/embedded builds (pack 06 §3.5); (d) `optional<T>` - the *type* says "maybe", callers must unpack. Layout: a `T` plus a `bool` flag, stored inline - **no heap**: `sizeof(optional<uint16_t>) == 4` (2 + 1 + padding), `optional<uint32_t> == 8`, `optional<uint64_t> == 16` (alignment doubles the cost; VERIFY on your ABI).

**Access:** `*o` / `o->` are unchecked (UB when empty); `o.value()` throws `std::bad_optional_access`; `o.value_or(x)` supplies a default; `if (o)` / `o.has_value()` test. Comparison `o == 5` is false for empty.

> **Common misconceptions**
> - *"optional is a nullable pointer."* No indirection, no heap, holds a copy.
> - *"`*o` is checked."* No.
> - *"optional<T&> exists."* Not in the standard; use `T*`.

### 3.6 `std::variant`

**Concrete first.** `using Field = std::variant<std::monostate, std::int64_t, std::string_view>;` holds *exactly one* of: nothing, an integer, a text view.

**Derive.** A C `union` overlaps its members' storage but does not remember which one is active - you need a separate tag, and non-trivial members (strings) are not constructed/destroyed correctly. `std::variant` is the *tagged union done right*: storage big enough for the largest alternative plus an index; it constructs and destroys the active alternative properly. Size here: max(8, 16) = 16 for the payload + 8 for index with alignment = 24 bytes on 64-bit (typical; VERIFY). The default-constructed variant holds the *first* alternative - if that type is not default-constructible, put `std::monostate` first.

**Access.** `std::holds_alternative<T>(v)`; `std::get<T>(v)` throws `bad_variant_access` on mismatch; `std::get_if<T>(&v)` returns a pointer or `nullptr`; `v.index()` is the active alternative number. `std::visit` (pack 07) applies a callable to whatever is held and is the preferred way to guarantee all alternatives are handled.

> **Common misconceptions**
> - *"variant is a union."* It tracks and manages the active member.
> - *"variant can hold references."* Not directly (use `reference_wrapper` or pointers).
> - *"Accessing the wrong type returns garbage."* It throws (with `get`) or returns null (`get_if`).

### 3.7 Templates

**Concrete first.** Without templates you'd write `max_int`, `max_double`, ...:

```cpp
template <class T> constexpr T clamp_val(T v, T lo, T hi) { return v < lo ? lo : (hi < v ? hi : v); }
clamp_val(5, 0, 3);          // compiler writes clamp_val<int>
clamp_val(1.5, 0.0, 1.0);    // and clamp_val<double>
```

**Derive.** A template is not code; it is a *recipe the compiler follows at compile time*. When it sees a use with concrete types it **instantiates**: substitutes the types and compiles that version - as if you had hand-written it. Consequences:
- **Zero run-time cost** (no virtual dispatch), each instantiation is separate machine code (*code bloat* if overused).
- **The definition must be visible where it is used**, so templates live in headers (otherwise: `undefined reference` at link time - the compiler never generated the instantiation).
- **Errors appear at instantiation** and read as walls of text: the compiler explains how substituting your type broke the recipe. C++20 *concepts* (`requires std::unsigned_integral<T>`) state requirements up front; C++17 uses `static_assert(std::is_unsigned_v<T>)` or `enable_if`.
- **Deduction:** `clamp_val(5, 0, 3)`: every `T` position must deduce the *same* type; `clamp_val(1, 0, 3.0)` fails (int vs double conflict).
- **Non-type parameters:** `template <class T, std::size_t N>` - `N` is a compile-time constant; it is how `std::array<T,N>` and our `StaticStack<T,N>` get their storage *inside the object* (no heap). `array_size(const T (&)[N])` deduces `N` from the array *type* (Q21).
- **Variadic templates and fold expressions (C++17):** `(0LL + ... + static_cast<long long>(xs))` expands to `0 + x1 + x2 + ...` (Q21).
- **Pitfall: returning a reference to a temporary:** `const T& max_ref(const T& a, const T& b)` is fine in `std::cout << max_ref(1,2);` but `const int& r = max_ref(1, 2);` leaves `r` dangling at the end of the statement (pack 03).

> **Common misconceptions**
> - *"Templates are slower to run."* They are as fast as hand-written code; they are slower to *compile*.
> - *"I can put the template body in a .cpp."* Only with explicit instantiation.
> - *"`T&&` always means rvalue reference."* In a deduced `template<class T> f(T&&)` it is a forwarding reference.
> - *"`typename` and `class` differ in template parameter lists."* They are equivalent there.

---

## 4. Fully worked example: a talkgroup list from text

Input text (synthetic): `"17,42,x,70000,,5"`. Goal: valid talkgroups as `uint16_t`, plus the bad fields, with *no per-field allocation*.

**Step 1 - split into views** (`split(s, ',')`): positions of `,` are indices 2, 5, 7, 13, 14. Fields: `[0,2)="17"`, `[3,5)="42"`, `[6,7)="x"`, `[8,13)="70000"`, `[14,14)=""`, `[15,16)="5"` - **6 fields** (5 commas + 1). Each `string_view` is `(ptr into s, length)`; no characters copied.

**Step 2 - parse each with `parse_u16`:**

| field | accumulate | result |
|---|---|---|
| `"17"` | 0 -> 1 -> 17 | `17` |
| `"42"` | 4 -> 42 | `42` |
| `"x"` | `'x'` not a digit | `nullopt` |
| `"70000"` | 7 -> 70 -> 700 -> 7000 -> **70000 > 65535 -> stop** | `nullopt` |
| `""` | empty | `nullopt` |
| `"5"` | 5 | `5` |

Why check *each step*: if we accumulated into `uint16_t` and checked only at the end, `"65537"` would wrap to 1 and look valid. We accumulate in `uint32_t` (max before multiply 65,535*10+9 = 655,359 - no overflow) and compare against 65,535 after every digit.

**Step 3 - collect.** `good = {17, 42, 5}` (`vector<uint16_t>`, `reserve(fields.size())` to avoid regrowth); `bad = {"x", "70000", ""}` stored as views (still valid because `s` outlives them). Total heap allocations: the two vectors only.

**Step 4 - what could go wrong with lifetimes.** If `s` were a temporary (`split(make_text(), ',')`) the returned views would dangle immediately. Rule: *a view is valid only while its owner is*; keep the owning `std::string` alive in the same scope or return owned strings from the API.

---

## 5. Questions

### 5.1 Explain why

**Q1.** *Context: a gateway collects 1000 call records into a vector in a loop.* Why does `std::vector::push_back` double (or scale) its capacity rather than grow by one, and why does that give amortised O(1)? Include the arithmetic for 1000 elements.

**Q2.** *Context: a config parser takes `const std::string&` and is called with string literals thousands of times.* Why does `std::string_view` help, and what two lifetime/representation traps does it introduce?

**Q3.** *Context: a decoder returns a field that may be absent, an int, or a text label.* Why prefer `optional`/`variant` over sentinels (`-1`, `""`) and a raw `union`? What does `variant` track that a union doesn't?

**Q4.** *Context: a developer moved a template function body into a `.cpp` and the build fails at link time.* Explain instantiation and why template definitions normally live in headers.

### 5.2 Calculate

**Q5.** *Context: reserve vs not.* Pushing 1000 elements: (a) total relocations with capacity doubling from 1; (b) with +1 growth; (c) allocations in each case; (d) allocations with `reserve(1000)`.

**Q6.** *Context: budgeting memory on a radio.* Give typical 64-bit `sizeof` for `std::optional<std::uint8_t>`, `optional<uint16_t>`, `optional<uint32_t>`, `optional<uint64_t>`, `std::array<std::uint16_t,5>`, `std::span<const uint8_t>` (dynamic), `std::span<const uint8_t,4>`, `std::variant<std::monostate, std::int64_t, std::string_view>`. Explain each.

**Q7.** *Context: sizing buffers from bit counts.* Compute `ceil_div` for (37 bits, 8) and (1000, 8), (1001, 8). Then show numerically why `(a + b - 1) / b` fails for `uint32_t` a = 4,294,967,295, b = 2.

### 5.3 Predict

**Q8.** *Context: a recorder keeps `int* p = &v[0]` while appending.* For `std::vector<int> v = {1,2,3}; v.reserve(3);` predict whether `p` stays valid after each independent operation: (a) `v[0] = 9;` (b) `v.push_back(4);` (c) `v.erase(v.begin());` then using `p` (d) `v.clear();` then reading `*p` (e) `v.reserve(2);`.

**Q9.** *Context: reviewing text-handling code.* Which of the following leave `sv` dangling? Explain each.
```cpp
std::string_view a = "literal";
std::string_view b = std::string("temp");
std::string s = "hello"; std::string_view c = s;     // then: s += " world, this is long";
std::string_view d = s.substr(1, 2);
std::string_view e = std::string_view(s).substr(1, 2);
```

**Q10.** *Context: a message type `std::variant<int, std::string> v;`.* Predict `v.index()`, `std::get<int>(v)`, then after `v = std::string("x")`: `v.index()`, `std::get_if<int>(&v)`, and what `std::get<int>(v)` does.

### 5.4 Trace by hand

**Q11.** *Context: debugging the splitter.* Trace `split("a,,b", ',')` showing `start`, `pos`, and each pushed view; give the final `size()`.

**Q12.** *Context: validating wire frames without copying.* Trace `frame_payload` on (a) `7E 02 0A 0B 01`, (b) `7E 03 01 02 03 00`, (c) `7E 02 0A 0B 02`, (d) `7E 02 0A 0B 01 00`. For each: which check decides, and what is returned (view start offset/length)?

### 5.5 Find the bug

**Q13.** *Context: cleaning a capture buffer.* Find the bug and say what the vector contains afterwards for `{1,2,1,3,1}`:
```cpp
void drop(std::vector<std::uint8_t>& v, std::uint8_t x) { std::remove(v.begin(), v.end(), x); }
```

**Q14.** *Context: pruning dead sessions.* Find the bug (and what ASan reports):
```cpp
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it == 0) v.erase(it);
```
Give two correct versions.

**Q15.** *Context: naming helper for logs.* Find the bug:
```cpp
std::string_view proto_name(int p) { return p == 1 ? std::string("DMR") : std::string("P25"); }
```

**Q16.** *Context: a helper to count entries in a table.* Find the bug and say what it returns on a 64-bit machine:
```cpp
std::size_t count(int table[10]) { return sizeof(table) / sizeof(table[0]); }
```

### 5.6 Implement

(`code/include/es_containers/`; tests per file.)

**Q17.** *Context: small container utilities.* Implement `be_bytes`, `erase_value`, `dedupe_sorted` in `views.hpp`.

**Q18.** *Context: zero-copy frame parsing.* Implement `frame_payload` and the overflow-safe `sub_span` in `views.hpp`.

**Q19.** *Context: parsing control strings without allocations.* Implement `split` and `parse_u16` in `text.hpp`.

**Q20.** *Context: a field that is empty, numeric or text.* Implement `parse_field` and `describe` in `text.hpp`.

**Q21.** *Context: generic helpers used by fixed-capacity containers.* Implement `clamp_val`, `array_size`, `ceil_div`, `sum_all`, `find_if_simple` in `templates.hpp`.

**Q22.** *Context: a no-heap stack for a call-handling path.* Implement `StaticStack<T,N>::push`/`pop` in `templates.hpp`.

### 5.7 Connect

**Q23.** *Context: pack 01 (`../01-toolchain/`) had `frame_decode(in, in_len, out, out_cap)` that copies the payload.* Compare it with `frame_payload`: what does each cost, who owns the bytes, and when must you prefer the copying version?

**Q24.** *Context: pack 03 (`../03-lifetime-move/`) and pack 06 (`../06-concurrency-rt/`).* A decoder thread parses a `span` over a `vector<uint8_t>` that the receive thread may push to. Name the two bugs (lifetime and race) and one design that avoids them without copying every frame. Why is `vector::push_back` on a hot real-time path a problem?

**Q25.** *Context: pack 05 (`../05-ub-layout-bit/`).* You have a `span<const uint8_t>` and want a `uint32_t` from bytes 1..4. Why is `*reinterpret_cast<const uint32_t*>(s.data() + 1)` wrong, and what do you write instead? (Preview; pack 05 derives it.)

### 5.8 Interview-style

**Q26.** "`std::array`, `std::vector`, `std::span`: when do you use each in an embedded protocol stack?"

**Q27.** "A parser can fail. Compare returning `optional`, an error enum/`expected`-style result, out-parameter+bool, and exceptions - particularly when exceptions are disabled."

**Q28.** "What is template instantiation? Why are template error messages long and how do concepts or `static_assert` help?"

**Q29.** "What are the pitfalls of `std::string_view`, and when would you still choose `std::string`?"

---

## 6. Self-assessment

- [ ] I can derive amortised O(1) growth with numbers (1,023 vs 499,500).
- [ ] I know which vector operations invalidate pointers/iterators and why.
- [ ] I can explain the erase-remove idiom (why `remove` can't shrink).
- [ ] I can write overflow-safe range checks for views.
- [ ] I can list the lifetime traps of `string_view` and `span`.
- [ ] I can choose between sentinel, `optional`, `variant`, exceptions for absent/failed values.
- [ ] I can explain template instantiation, deduction, non-type params, and fold expressions.
- [ ] All tests pass with sanitizers on.
- [ ] I can answer Q26-Q29 aloud.
