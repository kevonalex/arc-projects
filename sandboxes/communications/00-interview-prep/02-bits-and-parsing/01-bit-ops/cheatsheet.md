# Pack 01 — Cheatsheet (every line points to its derivation in `worksheet.md`)

Do **not** memorise lines — if one looks unfamiliar, open the cited section and re-derive it.

| Fact | Where derived |
|---|---|
| AND with 0 forces 0, AND with 1 keeps; OR with 1 forces 1, OR with 0 keeps; XOR with 1 flips | W §2 |
| Bit 0 = LSB (right). Check the spec's numbering before coding | W §2, §3.1 |
| Set: `x \| (1u<<p)` — OR "forces 1" only where mask is 1 | W §3.2 |
| Clear: `x & ~(1u<<p)` — AND needs an *inverted* mask (1 keeps, 0 forces 0) | W §3.2 |
| Toggle: `x ^ (1u<<p)`; test: `(x>>p) & 1u` | W §3.2 |
| Never use `+` to set a flag: carry corrupts the neighbour | W §3.2 |
| Extract: `(x >> pos) & ((1u<<len) - 1u)` — shift fixes bottom edge, mask fixes top edge | W §3.3.2, §3.3.3 |
| `(1u<<len) - 1` has `len` ones because subtraction borrows through `len` zeros | W §3.3.4 |
| `len == 32` → `1u<<32` is UB (hardware gives 1 → mask 0 → silent zero). Special-case it | W §3.3.6 |
| Parenthesise: `-` binds tighter than `<<`, which binds tighter than `&`, and `==` tighter than `&` | W §2, §3.3.7 (F1) |
| Signed `>>` fills with sign bit; promote `uint8_t` before `<<24`; use `1u` not `1` | W §3.3.7 (F2, F5, F6) |
| NAC/DUID of word `w`: `NAC=(w>>4)&0xFFF`, `DUID=w&0xF` (verify layout in TIA-102.BAAA-A) | W §3.3.1, §4 |
| Insert: `(x & ~field) \| ((v<<pos) & field)`, `field = low_mask(len)<<pos` | W §3.4 |
| Clip the value or it leaks into the neighbouring field | W §3.4 |
| Two's complement: `-x = ~x + 1` because `x + ~x = 111…1` (no carries), `+1` wraps to 0 | W §3.5 |
| Top bit 1 means value `v - 2ⁿ`; the most negative value is its own negative | W §3.5 |
| Sign-extend n bits: `m=1u<<(n-1); v&=mask(n); (v^m)-m` — proven for every 4-bit value | W §3.6 |
| Write `x = A 1 0ᵏ`; then `x-1 = A 0 1ᵏ` and `-x = ~A 1 0ᵏ` | W §3.7 |
| `x & (x-1)` clears the lowest set bit; `x & -x` (`x & (0u-x)`) isolates it | W §3.7 |
| Power of two: `x != 0 && (x & (x-1)) == 0` — the `x != 0` is mandatory | W §3.7, Q11 |
| Kernighan popcount loops *popcount* times because each pass deletes exactly one 1 | W §3.8 |
| Prefer `std::popcount` (C++20) / `__builtin_popcount` in production | W §3.8, Q15 |
| Hamming weight = popcount; parity = `popcount & 1`; distance = `popcount(a^b)` | W §3.8, Q14 |
| RMW on shared registers is not atomic; `volatile` ≠ atomic | W Q16 |

**Standards/VERIFY:** P25 NID layout & DUID table → TIA-102.BAAA-A (W §3.3.1). Modular unsigned→signed conversion → C++20 [conv.integral] (W §3.6).
