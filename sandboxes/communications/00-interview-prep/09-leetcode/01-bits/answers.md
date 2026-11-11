# 01 · Bits — Answers & Write-ups

All reference code is in `code/solutions/bits_solutions.cpp` and passes `code/tests/test_bits.cpp` (22 tests, ASan+UBSan clean).

---

## Part A answers (proofs)

### A2 — reduce `x ^ x ^ y ^ x ^ y ^ z`

```
x ^ x ^ y ^ x ^ y ^ z
= x ^ x ^ x ^ y ^ y ^ z      (commutative, associative: regroup)
= (x ^ x) ^ x ^ (y ^ y) ^ z  (associative)
= 0 ^ x ^ 0 ^ z              (self-inverse, twice)
= x ^ z                      (identity, twice)
```

### Why XOR cancels pairs — general statement

Let `S = a₁ ^ a₂ ^ … ^ a_m`. By commutativity + associativity, the value of `S` does not depend on order or bracketing, so group equal values together: a value that occurs `c` times contributes `x ^ x ^ … ^ x` (`c` copies) `= x` if `c` is odd and `0` if `c` is even (pair them off with `x ^ x = 0`, leftover is `x ^ 0 = x`). So **`S` = XOR of exactly those values whose count is odd.** If only one value has odd count, `S` is that value. (No sorting needed — the proof regroups *mathematically*; the machine never regroups.)

### Why `n & (n-1)` works — see worksheet A3. Loop invariant: *after `j` iterations `n` equals the original with its `j` lowest set bits removed.* Termination: `popcount` strictly decreases.

---

## P1 · 191 Number of 1 Bits

1. **Clarify.** `uint32_t n`. `0 → 0`, `0xFFFFFFFF → 32`. Result in `[0, 32]`.
2. **Brute.** `for i in 0..31: count += (n >> i) & 1` — O(32) = O(w) time, O(1) space.
3. **Optimise.** Kernighan `n &= n-1` removes one set bit per iteration (A3) → O(popcount) ≤ 32, same worst case but fewer iterations for sparse words. (Hardware `POPCNT` / `std::popcount` (C++20) is O(1); know it, but implement the loop in an interview.)
4. **Code.** `while (n) { n &= n-1; ++count; }`
5. **Trace.**
   * `n = 1011₂`: `1011 & 1010 = 1010` (1) → `1010 & 1001 = 1000` (2) → `1000 & 0111 = 0` (3). Answer 3 ✓.
   * edge `n = 0x80000000 = 1000…0₂`: `n-1 = 0111…1`; AND = 0; one iteration → 1 ✓. (Unsigned: no sign trouble.)
6. **Final.** Time O(popcount(n)) ≤ O(w); space O(1).

## P2 · 190 Reverse Bits

1. **Clarify.** Width is exactly 32; leading zeros become trailing zeros.
2. **Brute.** Convert to a 32-char string, reverse, parse — O(w) time, O(w) space.
3. **Optimise.** Shift loop: `out = (out << 1) | (n & 1); n >>= 1;` ×32 — O(w) time, O(1) space. Invariant: after `j` iterations `out` holds the lowest `j` bits of `n` in reversed order. Divide-and-conquer (O(log w) steps, all-parallel):
```cpp
n = (n >> 16) | (n << 16);
n = ((n & 0xFF00FF00u) >> 8) | ((n & 0x00FF00FFu) << 8);
n = ((n & 0xF0F0F0F0u) >> 4) | ((n & 0x0F0F0F0Fu) << 4);
n = ((n & 0xCCCCCCCCu) >> 2) | ((n & 0x33333333u) << 2);
n = ((n & 0xAAAAAAAAu) >> 1) | ((n & 0x55555555u) << 1);
```
   Correctness: reversing = swap halves, then reverse inside each half recursively; the five steps swap 16-, 8-, 4-, 2-, 1-bit blocks.
4. **Code.** Reference uses the loop (clearer; the swap version is a follow-up answer; for repeated use, a 256-entry byte table).
5. **Trace.** `n = 1`: iteration 1: `out = 0<<1 | 1 = 1`, `n = 0`; iterations 2–32 shift `out` left 31 times → `1 << 31 = 0x80000000` ✓. Edge `0xFFFFFFFF` → all ones ✓; `reverse(reverse(x)) == x` tested.
6. **Final.** O(w) time (32 steps), O(1) space (or O(log w) steps with the mask version).

## P3 · 338 Counting Bits

1. **Clarify.** Return `n+1` values `bits[0..n]`. `n = 0 → {0}`.
2. **Brute.** popcount each i: O(n · w) (or O(n · popcount)).
3. **Optimise.** `i = 2·(i>>1) + (i&1)` ⇒ `bits(i) = bits(i>>1) + (i&1)`. Since `i>>1 < i`, it is already computed — DP in increasing `i`. Alternative: `bits(i) = bits(i & (i-1)) + 1` (A3 clears one bit; `i&(i-1) < i`). This is the first "DP over a precomputed table" you meet — same idea as the 13-dp pack.
4. **Code.** `bits[i] = bits[i >> 1] + (i & 1)`.
5. **Trace `n=5`:** `0:0`, `1:bits[0]+1=1`, `2:bits[1]+0=1`, `3:bits[1]+1=2`, `4:bits[2]+0=1`, `5:bits[2]+1=2` → `[0,1,1,2,1,2]` ✓. Edge `n=0` → `[0]`.
6. **Final.** O(n) time, O(n) space (output).

## P4 · 136 Single Number

1. **Clarify.** Every element appears twice except one. Values may be negative (XOR on `int` is bitwise-safe). Non-empty. Need O(1) extra space for the "good" answer.
2. **Brute.** Hash map of counts: O(n) time / O(n) space; sort and scan pairs: O(n log n) / O(1).
3. **Optimise.** XOR everything. **Algebraic proof:** XOR is commutative, associative, `x^x=0`, `x^0=x` (worksheet A2). Hence `XOR(all)` equals XOR of values with odd multiplicity (proof above): paired values each contribute `x ^ x = 0`; the single contributes itself; `0 ^ s = s`. Order never matters because the group is abelian — so we do **not** need pairs to be adjacent.
4. **Code.** `acc = 0; for x: acc ^= x;`
5. **Trace `[4,1,2,1,2]`** (3-bit columns):
```
acc = 000
^ 100 (4) = 100
^ 001 (1) = 101
^ 010 (2) = 111
^ 001 (1) = 110
^ 010 (2) = 100  = 4  ✓
```
   Edge `[-1,5,-1]`: `-1 = 0xFFFFFFFF`; `0 ^ -1 ^ 5 ^ -1 = 5` ✓. Edge `[7]` → 7.
6. **Final.** O(n) time, O(1) space.

**Misconception refuted:** nothing requires sorted input; the regrouping is in the algebra, not the data.

## P5 · 137 Single Number II

1. **Clarify.** Every element 3×, one element 1×. `x^x^x = x`, so plain XOR returns the XOR of *all distinct values* — wrong.
2. **Brute.** Hash map O(n)/O(n).
3. **Optimise.**
   * *(a) Column counting.* For each bit position `b`, `count_b = #elements with bit b set`. Triples contribute `3k` (≡ 0 mod 3), the single contributes its bit. So bit `b` of the answer is `count_b mod 3`. 32·n work, O(1) space. Correct because columns are independent (A1/A5).
   * *(b) Two-bit counter per column, all 32 columns in parallel.* `ones[b], twos[b]` encode `count mod 3` as `00,01,10`. Update for input bit `x`:
     `ones' = (ones ^ x) & ~twos`, `twos' = (twos ^ x) & ~ones'`.
     Verification by cases (x=1): `00→01`, `01→10`, `10→00` ✓; (x=0): states unchanged ✓. After the whole array every triple has returned its columns to `00`, so `ones` = the single.
4. **Code.** in `bits_solutions.cpp` using `uint32_t` (also makes `INT_MIN` safe).
5. **Trace `[2,2,3,2]`** (2=10, 3=11), state shown as `(ones,twos)`:
```
start         ones=00 twos=00
x=10: ones=(00^10)&~00=10 ; twos=(00^10)&~10=00   -> 10,00
x=10: ones=(10^10)&~00=00 ; twos=(00^10)&~00=10   -> 00,10
x=11: ones=(00^11)&~10=01 ; twos=(10^11)&~01=00   -> 01,00
x=10: ones=(01^10)&~00=11 ; twos=(00^10)&~11=00   -> 11,00
answer ones = 11 = 3 ✓
```
   Edge `[INT_MIN, 7,7,7]` → `INT_MIN` (tests).
6. **Final.** O(n) time, O(1) space.

## P6 · 268 Missing Number

1. **Clarify.** `n` distinct values from `{0..n}` ⇒ exactly one missing. `[0] → 1`, `[1] → 0`.
2. **Brute.** Sort + scan O(n log n); hash-set O(n)/O(n); Gauss `n(n+1)/2 − Σ` O(n)/O(1) but `n(n+1)` overflows `int` once `n ≳ 46340`.
3. **Optimise (XOR).** Form the multiset `{0,…,n} ∪ nums`. Each *present* value appears exactly twice (once in the range, once in `nums`); the *missing* value appears exactly once (range only). By the odd-multiplicity lemma, `XOR(all of that multiset)` = the missing value. Implementation folds both sides in one loop: start `acc = n`, then `acc ^= i ^ nums[i]` for `i = 0..n-1` (that covers range values `0..n-1` via `i`, range value `n` via the seed, and all of `nums`). No overflow possible — XOR never leaves 32 bits.
4. **Code.** see solution.
5. **Trace `[3,0,1]`, n=3:** `acc=3`; i=0: `3^0^3=0`; i=1: `0^1^0=1`; i=2: `1^2^1=2` → 2 ✓. Edge `[0]`, n=1: `acc=1`; i=0: `1^0^0=1` → 1 (the missing value is `n` itself) ✓. Edge `[1]`: `acc=1; 1^0^1 = 0` ✓.
6. **Final.** O(n) time, O(1) space.

## P7 · 461 Hamming Distance

1. **Clarify.** Two non-negative ints. 2. **Brute.** loop over 31 bits comparing → O(w). 3. **Optimise.** `x ^ y` has a 1 exactly where they differ; popcount it (P1). 4. `popcount(x ^ y)`. 5. **Trace `1,4`:** `001 ^ 100 = 101` → 2 ✓; edge `0,0 → 0`. 6. **Final.** O(popcount) ≤ O(w) time, O(1) space.

## P8 · 477 Total Hamming Distance

1. **Clarify.** `n ≤ 10⁴`, values `< 2³¹`. Sum over all unordered pairs.
2. **Brute.** `n²/2` pairs × popcount: O(n²·w) — 10⁸·31 too slow.
3. **Optimise.** `Σ_{i<j} popcount(aᵢ^aⱼ) = Σ_{bit} #{pairs differing in that bit}` (swap the sums — A5). In a column with `k` ones and `n−k` zeros a pair differs iff it takes one of each: `k·(n−k)` pairs. Use `long long` for the product.
4. **Code.** 32 outer × n inner.
5. **Trace `[4,14,2]`:** `4=0100, 14=1110, 2=0010`.
   * bit0: ones=0 → 0
   * bit1: `14,2` → k=2 → 2·1 = 2
   * bit2: `4,14` → k=2 → 2
   * bit3: `14` → k=1 → 1·2 = 2
   Total 6 ✓. Edge `[5]` (n=1) → 0.
6. **Final.** O(w·n) = O(n) time, O(1) space.

## P9 · 371 Sum of Two Integers — full bit-by-bit derivation

1. **Clarify.** No `+`/`-`. Negatives allowed; result fits `int`.
2. **Brute.** `while (b--) ++a` — O(b) and wrong for negative `b`; "increment" itself needs `+`.
3. **Optimise — the half adder.** Add one column. Inputs `aᵢ, bᵢ`:
   * sum bit staying in column `i` = `aᵢ ^ bᵢ`
   * carry bit going to column `i+1` = `aᵢ & bᵢ`

   (1+1 = `10₂`: sum 0, carry 1 ✓; 0+1: sum 1, carry 0 ✓.) Doing all columns at once: `partial = a ^ b` and `carry = (a & b) << 1`; then `a + b = partial + carry` — but that is again an addition. So **repeat**: set `a = partial`, `b = carry`, loop until `b == 0`. Each round pushes every carry at least one column left, so after ≤ 32 rounds all carries have either been absorbed or shifted past the top bit.
   *Why negatives work:* two's complement wraps mod 2³²; unsigned addition modulo 2³² produces the correct *bit pattern* of the signed sum. Use `uint32_t`, then cast to `int` (well-defined modulo in C++20; implementation-defined but universal before).
4. **Code.** `while (y) { c = (x & y) << 1; x ^= y; y = c; }`
5. **Trace `13 + 7`** (5 columns shown, `a` is the running partial sum, `b` the carry):

| round | a (partial) | b (carry) | a ^ b | (a & b) << 1 |
|-------|-------------|-----------|-------|--------------|
| 1 | 01101 (13) | 00111 (7)  | 01010 | 01010 (10) |
| 2 | 01010 (10) | 01010 (10) | 00000 | 10100 (20) |
| 3 | 00000 (0)  | 10100 (20) | 10100 | 00000 → stop |

   Result `10100 = 20` ✓. Edge `-1 + 1` (`0xFFFFFFFF + 1`): round 1 `x^y = 0xFFFFFFFE`, carry `2`; round 2 `x=0xFFFFFFFC`, carry `4`; … the carry doubles each round (the "ripple") until round 32 where the carry bit `2³¹` shifts out of the word: `(x&y)<<1 = 0`, `x = 0` → `0` ✓ (this is the worst case: 32 rounds). Edge `INT_MIN + INT_MAX = 0x80000000 + 0x7FFFFFFF`: `x^y = 0xFFFFFFFF`, `x&y = 0` → 1 round → `-1` ✓.
6. **Final.** O(w) rounds = O(1) for fixed width; O(1) space.

## P10 · 201 Bitwise AND of Numbers Range

1. **Clarify.** `0 ≤ L ≤ R < 2³¹`. `L == R` → `L`.
2. **Brute.** AND all of `[L,R]` — O(R−L) up to 2³¹.
3. **Optimise.** Claim: result = common binary prefix of `L` and `R`, padded with zeros. *Proof.* If `L == R` trivial. Else let `p` be the highest bit where they differ; `L = P0x`, `R = P1y` (equal prefix `P`). The range contains `P01…1` (`≥ L`) and `P10…0` (`≤ R`) — consecutive integers; their AND = `P00…0` (below bit `p` the patterns are complementary). Every number in the range has prefix `P` (they lie between two numbers sharing `P`… in fact between `L` and `R`), so the total AND keeps `P` and is a subset of `P0…0` ⇒ exactly `P0…0`.
4. **Code.** shift both right until equal, count shifts, shift left back.
5. **Trace `5..7`:** `101,111` → `10,11` (s=1) → `1,1` (s=2) equal → `1<<2 = 4` ✓. `26..30`: `11010,11110` → `1101,1111` → `110,111` → `11,11` (s=3) → `11000 = 24` ✓. Edge `0..0 → 0`; `1..INT_MAX → 0`.
6. **Final.** O(w) time, O(1) space.

## P11 · 89 Gray Code

1. **Clarify.** `2ⁿ` values `0..2ⁿ−1` each once; consecutive (and last→first) differ in exactly 1 bit; start at 0.
2. **Brute.** DFS/backtracking flipping one bit at a time over unused numbers — exponential worst case.
3. **Optimise.** `g(i) = i ^ (i >> 1)`. *Proof it flips one bit:* `g(i) ^ g(i+1) = (i ^ (i+1)) ^ ((i ^ (i+1)) >> 1)`. If `i` has `k` trailing ones, `i ^ (i+1) = 2^{k+1} − 1` (`k+1` low ones). Let `m = 2^{k+1}−1`; `m ^ (m>>1) = (2^{k+1}−1) ^ (2^k −1) = 2^k` — a single bit ✓. Cyclic closure: `g(2ⁿ−1) = 2ⁿ⁻¹`, one bit from `g(0) = 0` ✓. Bijection: `g` is invertible (`i = g ⊕ g>>1 ⊕ g>>2 …`).
4. **Code.** loop `i` from 0 to `2ⁿ−1`.
5. **Trace `n=3`:** `i: 000→000, 001→001, 010→011, 011→010, 100→110, 101→111, 110→101, 111→100`: `0,1,3,2,6,7,5,4`; consecutive XORs `1,2,1,4,1,2,1`, wrap `4^0 = 4` — all powers of two ✓. Edge `n=1 → [0,1]`.
6. **Final.** O(2ⁿ) time and output space (output-bound).

## P12 · 393 UTF-8 Validation

1. **Clarify.** Each int uses only its low 8 bits. Char length 1–4. Continuation bytes are `10xxxxxx`. A leading `10xxxxxx` or `11111xxx` is invalid. Truncated sequence is invalid.
2. **Brute.** The natural algorithm is already one pass; the "brute" would be regex/string conversion — O(n·8).
3. **Optimise.** State = `remaining` continuation bytes. If `remaining == 0` classify header by shifting: `b>>7==0` (1 byte), `b>>5==0b110` (2), `b>>4==0b1110` (3), `b>>3==0b11110` (4), else invalid. Otherwise require `b>>6 == 0b10` and decrement. Accept iff `remaining == 0` at the end.
4. **Code.** as above.
5. **Trace `[197,130,1]`:** `197 = 11000101` → `>>5 = 110` → remaining 1; `130 = 10000010` → `>>6 = 10` ✓ → 0; `1 = 00000001` → `>>7 = 0` ok → end, remaining 0 → **true** ✓. `[235,140,4]`: `235 = 11101011` → `>>4 = 1110` → remaining 2; `140 = 10001100` ✓ → 1; `4 = 00000100` → `>>6 = 0 ≠ 10` → **false** ✓. Edges: `[145]` (`10010001`, orphan) false; `[197]` truncated false; `[248,…]` (`11111000`) false; `[256+65]` masked to `65` true.
6. **Final.** O(n) time, O(1) space.

---

## Part C answers

1. **False.** `>>` on negatives is arithmetic and rounds toward −∞: `-3 >> 1 == -2` whereas `-3 / 2 == -1`. (Also implementation-defined before C++20.)
2. **False.** `1 << 32` is UB for a 32-bit `int` (count ≥ width). `1 << 31` is UB in C/C++11, implementation-defined in C++14/17, defined in C++20; write `1u << 31`.
3. **False.** See P4.
4. **False.** `==` binds tighter than `&`: `a & b == 0` is `a & (b == 0)`. Parenthesise: `(a & b) == 0`.
5. **False.** `x ^ x ^ x = x`: plain XOR yields the XOR of the distinct values, not the single.
6. **False.** `~` promotes `uint8_t` to `int` first: `~uint8_t{0x0F}` is `int -16` (`0xFFFFFFF0`). Cast back: `static_cast<uint8_t>(~x)`.
7. **False.** Reversing `1` in a 32-bit word gives `0x80000000`.

## Part D answers

* XOR laws: identity, self-inverse, commutative, associative ⇒ abelian group of exponent 2, i.e. the vector space `(ℤ/2)³²` over GF(2).
* `n & (n-1)`: `n = P10ᵏ`, `n-1 = P01ᵏ`, AND `= P00ᵏ`.
* One column with carry-in `c`: `s = a^b^c`, `c_out = (a&b) | (c&(a^b))`. The loop never has a carry-in *inside* a round: the carry from column `i−1` is delivered as `b`'s bit in column `i` next round, so two-input half-adders suffice when repeated.
* Column-decomposable: 137, 477, 191, 338 (via recurrence on columns), 190 (per-column move), 201 (prefix). XOR-algebra problems: 136, 268, 461.
