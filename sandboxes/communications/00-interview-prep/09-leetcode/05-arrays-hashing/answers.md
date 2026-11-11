# 05 · Arrays & Hashing — Answers & Write-ups

Reference code: `code/solutions/arrays_hashing_solutions.cpp`; tests `code/tests/test_arrays_hashing.cpp` (10 tests, ASan+UBSan clean).

---

## P1 · 1 Two Sum

1. **Clarify.** Exactly one pair; indices distinct (`[3,3]`, T=6 → `{0,1}` is fine, but the same index twice is not). Negatives allowed; return indices in any order.
2. **Brute.** Double loop O(n²) time, O(1) space.
3. **Optimise.** One-pass hash map (A1): `need = T − a[i]`; if `need ∈ map` return `{map[need], i}`; else `map[a[i]] = i`. Completeness/soundness per A1. Alternative: sort `(value,index)` pairs and use two pointers: O(n log n), O(n) space, no hashing risk — good when hash worst cases matter.
4. **Code.** Use `long long need` then range-check before looking up as `int`.
5. **Trace `[2,7,11,15], 9`:** i=0: need 7 not in `{}` → `{2:0}`; i=1: need 2 ∈ map → return `{0,1}` ✓. `[3,3], 6`: i=0 need 3 not in `{}` → `{3:0}`; i=1 need 3 ∈ map (index 0) → `{0,1}` ✓ — works *because* lookup precedes insert and the second 3 overwrites nothing yet. Edge `[INT_MAX, INT_MIN, 5], −1`: i=0: `need = −1 − INT_MAX = INT_MIN`, not seen → `{INT_MAX:0}`; i=1: `need = −1 − INT_MIN = INT_MAX` ∈ map → `{0,1}` ✓. These subtractions happen to fit in 32 bits, but `T − a` overflows `int` in general (e.g. `T = INT_MIN, a = 1`), hence the 64-bit computation.
6. **Final.** O(n) expected time, O(n) space.

## P2 · 49 Group Anagrams

1. **Clarify.** Lowercase `a–z`; output groups in any order; `""` forms its own group; duplicates such as `{"aab","aab"}` belong together.
2. **Brute.** For each string compare with each group's representative using sorted copies: O(n²·L log L).
3. **Optimise.** Canonical key per string (A2), `unordered_map<key, vector<string>>`:
   * sorted-string key: O(L log L) per string;
   * 26-count key: O(L) per string (`"1,0,0,1,…"` with separators). Total O(n·(L+26)) expected.
   Correctness: two strings are anagrams ⇔ same multiset of letters ⇔ same count vector ⇔ same key; the key is injective on count vectors thanks to the separator.
4. **Code.** counts → string with `,` terminators → map.
5. **Trace `["eat","tea","tan","ate","nat","bat"]`:** keys `eat→{a1,e1,t1}`, `tea→same`, `tan→{a1,n1,t1}`, `ate→{a1,e1,t1}`, `nat→{a1,n1,t1}`, `bat→{a1,b1,t1}` ⇒ groups `{eat,tea,ate}`, `{tan,nat}`, `{bat}` ✓. Edge `[""]` → key = 26 zeros → one group `{""}` ✓. Edge empty input → no groups.
6. **Final.** O(n·L) expected time (count key), O(n·L) space.

## P3 · 238 Product of Array Except Self

1. **Clarify.** No division; `n ≥ 2`; zeros allowed; answer fits 32-bit; O(1) extra space apart from the output.
2. **Brute.** For each `i` multiply all others: O(n²).
3. **Optimise.** `out[i] = P[i]·S[i]` (A3). Pass 1: `out[i] = prefix; prefix *= a[i]`. Pass 2 (right→left): `out[i] *= suffix; suffix *= a[i]`.
   *Why division fails:* total ÷ `a[i]` is undefined for `a[i]=0`; with exactly one zero, only the zero's own slot is non-zero but the formula can't produce it; with two zeros every answer is 0 but division by zero crashes.
4. **Code.** two loops; `size_t` reverse loop written as `for (i = n; i-- > 0;)`.
5. **Trace `[1,2,3,4]`:** after pass 1 `out = [1,1,2,6]` (prefixes 1,1,2,6); pass 2: i=3: `out=6·1=6`, suf=4; i=2: `2·4=8`, suf=12; i=1: `1·12=12`, suf=24; i=0: `1·24=24` → `[24,12,8,6]` ✓. Edge `[-1,1,0,-3,3]` → exclusive prefixes `P = [1,-1,-1,0,0]`, exclusive suffixes `S = [0,0,-9,3,1]`, `out = P·S = [0,0,9,0,0]` ✓ (only the zero's own slot is non-zero: `(-1)(1)(-3)(3) = 9`).
6. **Final.** O(n) time, O(1) extra space.

## P4 · 128 Longest Consecutive Sequence

1. **Clarify.** Unsorted ints (duplicates, negatives); length of the longest run of consecutive values; must be O(n); empty → 0; mind `x−1`/`x+1` overflow at `INT_MIN`/`INT_MAX`.
2. **Brute.** Sort and scan: O(n log n) (fine in practice but not O(n)); or for each `x` walk upward with set lookups: O(n²).
3. **Optimise.** Hash set; start only where `x−1 ∉ S`; extend while `x+1 ∈ S` (A4). **Proof of O(n):** maximal runs partition the set. For a run `r` of length `ℓ`, the inner loop runs `ℓ−1` times and exactly once (only at its start) ⇒ total `Σ ℓ = |S| ≤ n`. The outer loop is `|S|` iterations with an O(1) check. Total O(n) expected.
4. **Code.** Iterate over the *set* (not the vector) to skip duplicates; guard `x == INT_MIN` and `cur < INT_MAX`.
5. **Trace `[100,4,200,1,3,2]`:** set `{1,2,3,4,100,200}`. `100`: 99 absent → start, 101 absent → len 1. `4`: 3 present → skip. `200`: start, len 1. `1`: 0 absent → start; 2,3,4 present → len 4. `3`,`2` skip. Best 4 ✓. Edge `[1,1,1]` → set `{1}` → 1. Edge `[]` → 0. Edge `{INT_MAX−1, INT_MAX, INT_MIN, INT_MIN+1}` → 2 without overflow.
6. **Final.** O(n) expected time, O(n) space.

## P5 · 41 First Missing Positive

1. **Clarify.** Smallest missing positive integer; unsorted; duplicates/negatives/large values allowed; O(n) time and O(1) extra space ⇒ mutate the input.
2. **Brute.** Put all in a hash set, count up from 1: O(n)/O(n); or sort: O(n log n).
3. **Optimise.** Cyclic placement (A5): for each `i`, while `1 ≤ v=nums[i] ≤ n` **and** `nums[v−1] ≠ v`, swap `nums[i]` with `nums[v−1]`. `while` because the swapped-in value may also be placeable; the `nums[v−1] ≠ v` guard handles duplicates (otherwise `[1,1]` loops forever swapping a value with itself). **≤ n swaps:** each swap fixes one value at its home permanently; values at home are never moved again. Second pass: first `i` with `nums[i] ≠ i+1` ⇒ `i+1`; otherwise `n+1`.
4. **Code.** see solution (index casts only for `size_t`).
5. **Trace `[3,4,−1,1]` (n=4):**
```
i=0: v=3 -> home 2 holds -1 != 3 -> swap -> [-1,4,3,1]; now nums[0]=-1 out of range, stop
i=1: v=4 -> home 3 holds 1 != 4 -> swap -> [-1,1,3,4]; nums[1]=1 -> home 0 holds -1 != 1 -> swap -> [1,-1,3,4]; nums[1]=-1 stop
i=2: v=3 -> home 2 holds 3 == 3 stop
i=3: v=4 -> home 3 holds 4 stop
scan: [1,-1,3,4] -> index1: -1 != 2 -> answer 2 ✓
```
   Edge `[1,1]`: i=0: v=1, home 0 holds 1 → stop; i=1: v=1 home 0 holds 1 == v → stop (guard prevents infinite loop); scan: index1 holds 1 ≠ 2 → **2** ✓. Edge `[7,8,9,11,12]`: nothing in range → index0: `7 ≠ 1` → **1** ✓. Edge `[1,2,3]` → `4`.
6. **Final.** O(n) time, O(1) extra space.

---

## Part C answers
1. **False.** Expected O(1); worst O(n) with collisions.
2. **False.** Insert-first pairs an element with itself when `2a = T`: `[3,2,4], 6` would return `{0,0}`.
3. **False.** Count-signature keys avoid sorting.
4. **False.** Fails with zeros and the problem forbids it.
5. **False.** The inner loop runs only from run starts: O(n).
6. **False.** The array itself is the table (A5).
7. **False.** `if` leaves the swapped-in value unplaced (e.g. `[3,4,-1,1]` at `i=1`).

## Part D answers
* 1: value→index; 49: letter-count signature; 128: value (set); 41: index = value−1 (array slots); 238: no table.
* After step `j`: map = `{a_i → i : i ≤ j}`.
* `[3,2,4], 6` with insert-first.
* See P5.
