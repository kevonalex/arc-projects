# 01 · Bits — Cheatsheet

**Core idea:** an integer is 32 independent columns; `& | ^ ~` work per column; use **unsigned** types.

## Identities
| Trick | Expression |
|-------|-----------|
| clear lowest set bit | `n & (n-1)` |
| isolate lowest set bit | `n & -n` (unsigned: `n & (~n + 1)`) |
| is power of two (n>0) | `(n & (n-1)) == 0` |
| set / clear / toggle / test bit k | `n \| (1u<<k)` · `n & ~(1u<<k)` · `n ^ (1u<<k)` · `(n>>k)&1u` |
| low k bits mask | `(1u<<k) - 1` (k < 32!) |
| XOR swap | `a^=b; b^=a; a^=b;` (never on the same object) |
| Gray code / inverse | `i ^ (i>>1)` · `b = g; while (g >>= 1) b ^= g;` |
| negate | `~x + 1` |

## XOR laws → pair cancellation
`x^0=x`, `x^x=0`, commutative, associative ⇒ XOR of a multiset = XOR of values with **odd** count. (136, 268; 137 needs mod-3 counting.)

## Recipes
| LC | Move | Time / Space |
|----|------|--------------|
| 191 | `while(n){n&=n-1;++c;}` | O(popcount) / O(1) |
| 190 | `out=(out<<1)\|(n&1); n>>=1` ×32; or swap 16/8/4/2/1 blocks | O(32) / O(1) |
| 338 | `bits[i]=bits[i>>1]+(i&1)` | O(n) / O(n) |
| 136 | XOR all | O(n) / O(1) |
| 137 | `ones=(ones^x)&~twos; twos=(twos^x)&~ones;` | O(n) / O(1) |
| 268 | `acc=n; acc^=i^nums[i]` | O(n) / O(1) |
| 461 | `popcount(x^y)` | O(1)-ish |
| 477 | `Σ_bit k(n-k)` (k = #ones in column) | O(32n) / O(1) |
| 371 | `while(b){c=(a&b)<<1; a^=b; b=c;}` on `uint32_t` | ≤32 rounds / O(1) |
| 201 | shift L,R right until equal; shift back | O(32) / O(1) |
| 89 | `i^(i>>1)` | O(2ⁿ) |
| 393 | `remaining` counter; masks `0 / 110 / 1110 / 11110 / 10` | O(n) / O(1) |

## Gotchas
* Do bit work on `uint32_t`/`uint64_t`; signed overflow and shifts ≥ width are UB.
* `a & b == 0` ⇒ `a & (b==0)`. Parenthesise.
* `~uint8_t` promotes to `int`; cast back.
* `1 << 31` → write `1u << 31`.
* Mask with `& 0xFF` when an `int` carries a byte (393).
* Use 64-bit for products of counts (477).
* Prefer `std::popcount` (C++20, `<bit>`) in production; implement Kernighan in interviews.

## Say-it-out-loud script
"Clarify width/signedness → brute force is O(w) per element → the property I exploit is ___ → code with unsigned → trace `101` and an edge (`0`, all-ones) → O(n) time, O(1) space."
