# Pack 04 cheat sheet - every line points back to the worksheet

**Arrays**
- C array parameter decays to pointer; `sizeof` gives pointer size - §3.1 (Q16)
- `std::array<T,N>`: size in the type, no heap, copyable, `.at()` checks, `[]` doesn't - §3.1

**vector**
- layout: begin / end / end_of_storage; `size = end-begin`, `capacity = eos-begin` - §3.2
- geometric growth => amortised O(1): 1000 pushes = 1,023 relocations vs 499,500 for +1 - §3.2 (Q1, Q5)
- `reserve(n)` once; never grow on a real-time path - §3.2
- realloc invalidates ALL pointers/iterators; erase invalidates at/after the point - §3.2 (Q8, Q14)
- `v[i]` unchecked (UB), `at(i)` throws - §3.2
- `remove` reorders only; `erase(remove(...), end)` shrinks - §3.2 (Q13)
- `clear()` keeps capacity - §3.2

**span**
- non-owning (ptr,len); `span<const T>` read-only; `first/last/subspan` O(1), no copy - §3.3
- dangles when owner reallocates/dies; `[]` unchecked - §3.3 (Q24)
- validate `offset <= size` then `count <= size - offset` (never `offset+count`) - §3.3, Q18

**string_view**
- `(ptr,len)`, not null-terminated; `substr` O(1) - §3.4
- dangling: view of temporary string; `std::string::substr` returns a std::string - §3.4 (Q9, Q15)
- prefer for read-only text parameters; use `std::string` to own - §3.4 (Q29)

**optional / variant**
- sentinel collides with legal values; `optional` = value + flag inline, no heap - §3.5
- `optional<T>` size: T + flag padded to alignment (u16->4, u32->8, u64->16) - §3.5 (Q6)
- `*o` unchecked; `.value()` throws; `.value_or(d)` - §3.5
- variant = tagged union with managed lifetime; first alternative default; `monostate` for "none" - §3.6
- `holds_alternative` / `get_if` (null on mismatch) / `get` (throws) / `index()` - §3.6 (Q10)

**Templates**
- template = compile-time recipe; instantiation per type; zero run-time cost, code bloat - §3.7
- definition must be visible at use => put in headers - §3.7 (Q4)
- `T` must deduce consistently: `clamp_val(1, 0, 3.0)` fails - §3.7
- non-type param `N` => storage in the object, no heap (`StaticStack<T,N>`) - §3.7, Q22
- `T(&)[N]` deduces array length; fold `(0LL + ... + x)` - §3.7, Q21
- `a/b + (a%b != 0)` avoids `(a+b-1)` overflow - §3.7 (Q7)
- `static_assert` / concepts for readable errors - §3.7 (Q28)
