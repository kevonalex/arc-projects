# Pack 04 - Answers

Format: **Reasoning -> Answer -> Common wrong answers**. Reference code: `code/solutions/es_containers/`.

---

## Prerequisites

**P1.** `size()` is `size_t` (unsigned); `0 - 1` wraps to 18,446,744,073,709,551,615 (on 64-bit), so a loop `i < v.size() - 1` iterates "forever". (Pack 02 §3.3.)
**P2.** The vector copies instead of moving on reallocation (`move_if_noexcept`), so growth costs deep copies.
**P3.** A pointer/reference/view to an object whose lifetime ended, e.g. returning a reference to a local, or a `string_view` of a temporary `std::string`.
**P4.** `[begin,end)` includes `begin`, excludes `end`. Empty range = `begin == end`; length = `end - begin`; adjacent ranges share a boundary with no overlap.
**P5.** `heap-buffer-overflow` (READ/WRITE of size 1, "0 bytes after N-byte region") - provided the library has ASan container annotations or the access leaves the allocation; with spare capacity it can be a silent in-capacity overrun (`container-overflow` appears only if libstdc++ vector annotations are enabled).

---

## 5.1 Explain why

**Q1.** *Reasoning:* a reallocation relocates every current element. If capacity grows by a constant, the number of relocations per push stays proportional to n, so n pushes cost O(n^2). If capacity multiplies by a factor > 1, reallocations become rarer as the vector grows; with doubling, relocations occur at sizes 1,2,4,...,512 for 1000 pushes: 1+2+...+512 = **1,023** relocations, about one extra copy per element -> O(1) amortised. With +1 growth: 0+1+...+999 = **499,500**. *Wrong:* "doubling wastes memory so +1 is better" - tempting for embedded; the right answer for tight memory is `reserve` (exact size) not +1 growth. "amortised O(1) means every push is O(1)" - individual pushes can be O(n).

**Q2.** *Reasoning:* `const std::string&` accepts only `std::string`; a literal creates a temporary `std::string` (possible heap allocation). `string_view` is `(ptr,len)`: accepts literals, `std::string`, other views with no allocation. Traps: (1) a view is non-owning, so it dangles if the owner dies/reallocates (`string_view sv = std::string("x");`); (2) it is not null-terminated, so `sv.data()` is not a valid C string. *Wrong:* "string_view copies the characters" - the opposite; "it's always safe because it's const" - const says nothing about lifetime.

**Q3.** *Reasoning:* sentinels (`-1`, `0xFFFF`, `""`) overload a legal value as "absent"; a 16-bit field where all values are legal has none to spare, and callers can forget to test. `optional<T>` makes absence part of the type. A `union` shares storage but doesn't record which member is active and does not run constructors/destructors of non-trivial members; `variant` stores an index and manages construction/destruction of the active alternative, and `get`/`get_if` check against it. *Wrong:* "optional is for pointers" - it holds values inline; "variant is just an enum plus union" - the *management of lifetimes* is the big difference.

**Q4.** *Reasoning:* a template is a recipe; the compiler creates ("instantiates") a function only when it sees a use with concrete types *and* has the body available at that point. If the body is in a `.cpp`, other translation units only see the declaration, emit a call to e.g. `clamp_val<int>(int,int,int)`, and no TU generated that instantiation -> linker `undefined reference`. So definitions go in headers (or you explicitly instantiate the few types in the `.cpp`: `template int clamp_val<int>(int,int,int);`). *Wrong:* "templates can't be in .cpp because of the preprocessor" - it's about instantiation visibility.

---

## 5.2 Calculate

**Q5.** (a) Doubling: capacities 1,2,...,1024 -> **1,023** relocations (1+2+...+512), **11** allocations. (b) +1: relocations 0+1+...+999 = **499,500**; allocations **1000** (one per push). (c) as above. (d) `reserve(1000)`: **1** allocation, **0** relocations. *Wrong:* 1000 relocations for doubling (confusing relocations with pushes); 2048 (summing capacities).

**Q6.** `optional<uint8_t>` 2 (1 + 1 flag); `optional<uint16_t>` 4 (2 + flag, padded to alignment 2); `optional<uint32_t>` 8; `optional<uint64_t>` 16 (alignment 8 pads the flag out to 8). `array<uint16_t,5>` 10 (no overhead). `span<const uint8_t>` 16 (pointer + size); `span<const uint8_t,4>` 8 (the size is in the type). `variant<monostate,int64_t,string_view>`: payload max(1,8,16) = 16; index 1 byte padded to alignment 8 -> 24. (Typical for common ABIs; VERIFY with `static_assert(sizeof(...) == ...)` on your target.) Lesson: for large arrays of optionals use parallel `bool`/bitmask arrays - the flag costs a whole word.

**Q7.** `ceil_div(37, 8)`: 37/8 = 4 rem 5 -> **5**. `1000/8 = 125` rem 0 -> **125**. `1001` -> **126**. Overflow: `4,294,967,295 + 2 - 1 = 4,294,967,296 = 2^32` which wraps to **0** in `uint32_t`; `0/2 = 0`, but the true answer is 2,147,483,648 (`0x80000000`). `a/b + (a % b != 0)` never forms a sum bigger than `a`. *Wrong:* "use `(a + b - 1) / b` in `size_t` so it's fine" - `size_t` can still wrap near its max (rare, but it is a trust-boundary bug if `a` comes off the wire).

---

## 5.3 Predict

**Q8.** (a) `v[0] = 9;` - `p` stays valid (no reallocation, same element). (b) `push_back(4)` with `size == capacity == 3` -> **reallocation: `p` dangling** (ASan `heap-use-after-free` on use). (c) `erase(begin())`: elements shift down; `p` still points into live storage but now at the old element 2 -> invalid per the standard (iterators/references at or after the erase point), and it "works" by accident. (d) `clear()`: elements are destroyed; capacity unchanged; reading `*p` is formally UB although ASan sees allocated memory and stays quiet - a silent bug. (e) `reserve(2)` when capacity is already 3: **no-op**, `p` valid. *Lesson:* ASan finds (b) but not (c)/(d); don't rely on tools for invalidation bugs. *Wrong:* "(d) is fine because the memory is still there".

**Q9.** `a`: OK (string literal has static storage). `b`: **dangling** - the temporary `std::string` dies at the `;`. `c`: dangling after `s += ...` if the append reallocates (here "hello" + 20 more chars exceeds the small-string buffer of 15, so it reallocates); even if it doesn't, mutating the owner invalidates views formally. `d`: **dangling** - `std::string::substr` returns a *temporary std::string*. `e`: OK - `string_view::substr` returns a view into `s`; valid while `s` is alive and unmodified. *Wrong:* "d is fine because substr returns a view" - that is the *view's* substr.

**Q10.** Default-constructed holds the first alternative: `v.index() == 0`, `std::get<int>(v) == 0`. After `v = std::string("x")`: `index() == 1`; `std::get_if<int>(&v) == nullptr`; `std::get<int>(v)` throws `std::bad_variant_access`. *Wrong:* "get<int> returns garbage/0 after the change" - it throws.

---

## 5.4 Trace by hand

**Q11.** `s = "a,,b"` (indices 0:`a`, 1:`,`, 2:`,`, 3:`b`). start=0 -> `find(',',0)=1` -> push `substr(0,1)="a"`, start=2. `find(',',2)=2` -> push `substr(2,0)=""`, start=3. `find(',',3)=npos` -> push `substr(3)="b"`, return. **size 3**, fields `"a"`, `""`, `"b"`.

**Q12.** (a) size 5 >= 3; marker ok; n=2; size == 5 ok; xor `0A ^ 0B = 01` == frame[4] -> view = offset 2, length 2. (b) n=3, size 6 ok; xor `01^02^03 = 00` == frame[5] -> valid (a zero checksum is legitimate!), view offset 2, length 3. (c) checksum `01 != 02` -> `nullopt`. (d) n=2 so `size` must be 5 but is 6 -> the **length check** fails -> `nullopt` (trailing byte). *Wrong:* (b) invalid "because checksum is 0" - tempting from treating 0 as "missing"; wrong.

---

## 5.5 Find the bug

**Q13.** `std::remove` only moves keepers forward and returns the new logical end, which is *discarded*. Size stays 5; e.g. `{2,3,1,3,1}` (first two are the keepers, the tail is valid but unspecified). Fix: `v.erase(std::remove(v.begin(), v.end(), x), v.end());` (or `std::erase(v, x)` in C++20). *Wrong:* "it leaves {2,3}" - tempting because of the name.

**Q14.** After `v.erase(it)`, `it` is invalid and `++it` is UB; also the element that slid into `*it` is skipped. If the last element was erased, `it` ends up beyond `end()` and the loop runs past the vector (ASan may report `container-overflow`/`heap-buffer-overflow`, or nothing). Fixes: (1) `for (auto it = v.begin(); it != v.end();) { if (*it == 0) it = v.erase(it); else ++it; }`; (2) `v.erase(std::remove(v.begin(), v.end(), 0), v.end());` (O(n) instead of O(n^2) for many removals).

**Q15.** The ternary yields a temporary `std::string`; it is converted to `string_view` and the temporary is destroyed at the end of the return statement -> returns a dangling view. Fix: `return p == 1 ? "DMR" : "P25";` (literals have static storage) or return `std::string`/`const char*`. *Wrong:* "string_view extends the temporary's lifetime" - only a *reference* bound directly to a temporary does, and not through a conversion to a view.

**Q16.** The parameter is `int*` (array decay); `sizeof(table)` is the pointer size 8, `/ sizeof(int)` 4 = **2** on 64-bit (on a 32-bit target: 4/4 = 1). GCC/Clang warn with `-Wsizeof-array-argument` (on by default). Fix: `template<size_t N> size_t count(const int (&)[N]) { return N; }`, or `std::array`/`std::span`. *Wrong:* "returns 10 because the parameter says [10]" - the 10 is ignored by the language.

---

## 5.6 Implement

**Q17.** `be_bytes`: shifts of a `uint32_t` (no promotion issue) with `static_cast<uint8_t>` each. `erase_value`: `erase(remove(...), end)`, count = `end - new_end` computed *before* erasing. `dedupe_sorted`: `erase(unique(...), end)`. Wrong: computing the removed count as `old_size - v.size()` after forgetting to erase; using `unique` on an unsorted vector (it removes only *adjacent* duplicates).

**Q18.** `frame_payload`: check `size >= 3` **before** reading `frame[0]`/`[1]`; `size == n+3` (exact); build `subspan(2,n)`; xor; compare with `frame[2+n]`. `sub_span`: `offset <= size`, then `count <= size - offset`. Wrong: `offset + count <= size` (wraps; test `NoSizeTOverflow`); `frame.size() >= n + 3` (accepts trailing garbage); copying the payload into a vector (works, but defeats the zero-copy test).

**Q19.** `split`: loop with `find(sep, start)`; the field count is `separators + 1`; use `substr`. `parse_u16`: accumulate in `uint32_t`, check each step, reject empty/non-digit. Wrong: `std::stoi` (throws, accepts `" 12"`, `"12abc"`, `"+1"`), accumulating in `uint16_t` and checking at the end (wraps), skipping the empty-input check (returns 0 for `""`).

**Q20.** `parse_field`: empty -> monostate; optional leading `-`; 1..18 digits (18 digits always fit int64); any non-digit -> return the *original* view as text. `describe`: `holds_alternative`/`get_if`. Wrong: `std::stoll` + exceptions for control flow; treating `"-"` as an integer; 19-digit numbers overflowing `int64` (UBSan `signed integer overflow`).

**Q21.** `clamp_val`: only `operator<` (`v < lo ? lo : (hi < v ? hi : v)`). `array_size`: `return N;` (deduced from `T(&)[N]`). `ceil_div`: `a/b + (a%b != 0)`. `sum_all`: `(0LL + ... + static_cast<long long>(xs))`. `find_if_simple`: loop. Wrong: `std::max/min` on mismatched types in clamp (deduction conflict); `sizeof(a)/sizeof(a[0])` inside a function taking a pointer; `(xs + ...)` for the empty pack (ill-formed for `+`; unary folds only allow `&&`, `||`, `,` when empty).

**Q22.** `push`: reject if `size_ == N`, store `items_[size_++]`. `pop`: reject if empty, return `std::move(items_[--size_])`. Because `std::array<T,N> items_{}` default-constructs all N elements, T must be default constructible (a limitation fixed in pack 07 with raw storage + placement new). Wrong: `items_[size_--]` (off by one - `size_` is a count, last valid index is `size_-1`); returning a reference to the popped slot.

---

## 5.7 Connect

**Q23.** `frame_decode` (pack 01) copies N payload bytes into caller-owned memory: O(N) time, the caller owns the copy, which stays valid after the receive buffer is reused. `frame_payload` is O(1) with zero allocation, but the view is valid only while the frame bytes are alive and unchanged. Prefer the copy when the payload must outlive the receive slot (queued to another thread; ring-buffer slot recycled); prefer the view when parsing synchronously inside the receive callback. Both reject the same malformed inputs - the test lists are deliberately parallel.

**Q24.** Lifetime: `push_back` can reallocate, so the span dangles. Race: unsynchronised reader and writer on a `vector` is a data race (UB), even if the writer only appends. Design: a fixed pool/ring of frame slots (pack 07 `RingBuffer`/`SpscQueue`): the receiver fills slot k and publishes the index; the decoder views slot k, which is untouched until it releases it. `push_back` on a hot path: may call `malloc` (unbounded latency, possibly a lock inside the allocator, fragmentation) and relocates all elements. *Wrong:* "wrap the vector in a mutex" - fixes the race but not the dangling view or the latency.

**Q25.** `s.data() + 1` is almost certainly misaligned for `uint32_t` (UBSan: `load of misaligned address`), and reading a `uint8_t` array through a `uint32_t*` violates strict aliasing (UB; compiler may reorder/elide). Write `std::uint32_t v; std::memcpy(&v, s.data() + 1, sizeof v);` (compilers turn it into one load) and then convert endianness (`read_be32` from pack 02 does both at once portably).

---

## 5.8 Interview-style

**Q26.** `std::array` for compile-time-fixed sizes (headers, IDs; no heap, constexpr). `std::vector` for runtime-sized *owned* data when allocation timing is acceptable (setup, non-real-time paths), `reserve` once. `std::span` for *non-owning function parameters* that accept all of the above. On real-time paths: array or fixed-capacity containers (pack 07), spans into pre-allocated storage, never growth. State the ownership in the type.

**Q27.** `optional`: simple "value or nothing", loses the reason. Error enum / `std::expected` (C++23) / `variant<T, Error>`: carries *why* - best for parsers that must distinguish bad checksum from truncation. Out-param + bool: legacy, easy to ignore. Exceptions: clean call sites but unbounded cost on throw, need unwinding tables, often disabled in embedded/real-time builds (`-fno-exceptions`); acceptable for truly exceptional conditions in non-RT code. Use `[[nodiscard]]` on result types. *Wrong:* "exceptions are always bad / always good" - say when.

**Q28.** Instantiation: the compiler substitutes concrete types into the template and compiles the result; errors are discovered *after* substitution, deep in library internals, so the message shows the instantiation chain. `static_assert(is_unsigned_v<T>, "...")` gives a custom message at the point of use; C++20 concepts (`template<std::unsigned_integral T>`) reject bad types at overload resolution with a short, readable "constraint not satisfied". Also mention code bloat and header-only definitions.

**Q29.** Pitfalls: non-owning (dangling from temporaries, reallocated owners), not null-terminated (C APIs), `substr` on `std::string` yields a std::string, mutation of the owner invalidates, don't store views in long-lived objects without a lifetime story. Use `std::string` when you need ownership, null termination, a long lifetime, or to store beyond the source's life; use `string_view` for read-only parameters and transient parsing.
