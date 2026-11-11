# 09 · Binary Search — Answers & Write-ups

Reference code: `code/solutions/binary_search_solutions.cpp`; tests `code/tests/test_binary_search.cpp` (8 tests, ASan+UBSan clean; 33 and 153 are checked exhaustively over **every** rotation of arrays of length 1..12).

---

## P1 · 704 Binary Search

1. **Clarify.** Strictly ascending `int`s; return the index of `target` or `−1`. Empty input → `−1`.
2. **Brute.** Linear scan: O(n) time, O(1) space.
3. **Optimise.** Closed interval `[lo, hi]`. **Invariant:** if `target` is in the array it lies in `[lo, hi]`. Probe `mid = lo + (hi−lo)/2`: equal ⇒ done; `nums[mid] < target` ⇒ target is right of `mid` ⇒ `lo = mid+1`; else `hi = mid−1`. Each step discards `mid` and at least half the interval. Loop `while (lo <= hi)`; exit with an empty interval ⇒ absent.
4. **Code.** see solution.
5. **Trace `[-1,0,3,5,9,12], 9`:**
```
lo=0 hi=5 mid=2 (3)  3<9  -> lo=3
lo=3 hi=5 mid=4 (9)  hit  -> return 4 ✓
target 2:
lo=0 hi=5 mid=2 (3)  3>2  -> hi=1
lo=0 hi=1 mid=0 (-1) -1<2 -> lo=1
lo=1 hi=1 mid=1 (0)  0<2  -> lo=2
lo=2 hi=1  -> empty -> -1 ✓
```
   Edges: empty → `hi = -1`, loop skipped → `-1`. `{5}` target 5 → `0`. `{1,3}` target 0: `mid=0`, `1>0` → `hi=-1` → `-1`. Values `{INT_MIN, 0, INT_MAX}` target `INT_MAX`: `lo+hi = 2` fine here, but with `lo=hi_index` huge arrays `lo+hi` could overflow — the `lo + (hi−lo)/2` form never does.
6. **Final.** O(log n) time, O(1) space.

## P2 · 33 Search in Rotated Sorted Array

1. **Clarify.** Distinct values; array was ascending then rotated left by some `r ∈ [0, n)`; return index or `−1`.
2. **Brute.** Linear O(n). Or find the pivot (153) in O(log n) then binary-search the correct side: O(log n) with two passes.
3. **Optimise.** One pass using "one half is always sorted" (A4 proof). At each `mid`:
   * if `nums[mid] == target` return `mid`;
   * if `nums[lo] <= nums[mid]` the left half `[lo, mid]` is sorted: if `nums[lo] ≤ target < nums[mid]` go left (`hi = mid−1`) else right (`lo = mid+1`);
   * else the right half `[mid, hi]` is sorted: if `nums[mid] < target ≤ nums[hi]` go right else left.
   **Invariant:** if the target exists it is in `[lo, hi]`. `<=` in `nums[lo] <= nums[mid]` handles `lo == mid` (a one- or two-element range): then the "left half" is just `[mid]`, trivially sorted, and the range test `nums[lo] ≤ target < nums[mid]` is empty, so we move right correctly. With `<` the `lo == mid` case would be misclassified as "right half sorted" and can discard the target (e.g. `[3,1]`, target 1).
4. **Code.** see solution; a lambda `at(i)` removes the `size_t` casts.
5. **Trace `[4,5,6,7,0,1,2], target 0`:**
```
lo=0 hi=6 mid=3 (7)  nums[0]=4 <= 7 -> left sorted [4..7]; 4<=0<7? no -> lo=4
lo=4 hi=6 mid=5 (1)  nums[4]=0 <= 1 -> left sorted [0..1]; 0<=0<1 yes -> hi=4
lo=4 hi=4 mid=4 (0)  hit -> 4 ✓
```
   Edge `[3,1], 1`: `lo=0,hi=1,mid=0 (3)`; `nums[0]<=nums[0]` left "sorted" `[3]`; `3<=1<3` no → `lo=1`; `mid=1 (1)` hit → `1` ✓. Edge `[1,2,3,4], 4` (not rotated) behaves like 704. Edge absent target `[4,5,6,7,0,1,2], 3`: ends with empty interval → `−1` ✓. `{}` → `−1`.
6. **Final.** O(log n) time, O(1) space.

## P3 · 153 Find Minimum in Rotated Sorted Array

1. **Clarify.** Distinct; non-empty; rotation possibly 0 (then the min is `nums[0]`).
2. **Brute.** Linear `min_element`: O(n).
3. **Optimise.** Boundary search with predicate `P(i) = nums[i] ≤ nums[n−1]` (monotone, A4); minimum = first true. With `[lo, hi]`, test `nums[mid]` vs `nums[hi]`:
   * `nums[mid] > nums[hi]` ⇒ the drop (and the minimum) is strictly right of `mid` ⇒ `lo = mid+1`;
   * else `nums[mid] ≤ nums[hi]` ⇒ `[mid, hi]` is sorted so the minimum is at `mid` or left ⇒ `hi = mid` (keep `mid`!).
   Loop `while (lo < hi)`; at exit `lo == hi` is the minimum. **Invariant:** the minimum's index is in `[lo, hi]`. Using `hi = mid − 1` would exclude a candidate that may be the answer; comparing with `nums[lo]` is useless because for a sorted subrange `nums[mid] ≥ nums[lo]` doesn't reveal which side holds the drop.
4. **Code.** see solution.
5. **Trace `[3,4,5,1,2]`:**
```
lo=0 hi=4 mid=2 (5)  5 > nums[4]=2 -> lo=3
lo=3 hi=4 mid=3 (1)  1 <= 2        -> hi=3
lo=3 == hi=3 -> answer nums[3] = 1 ✓
```
   `[11,13,15,17]` (not rotated): `mid=1 (13) <= 17` → `hi=1`; `mid=0 (11) <= 13` → `hi=0` → 11 ✓. Edge size 1 → loop skipped → that element. Size 2 `[2,1]`: `mid=0 (2) > 1` → `lo=1` → 1 ✓; `[1,2]`: `1 <= 2` → `hi=0` → 1 ✓.
6. **Final.** O(log n) time, O(1) space.

---

## Part C answers
1. **False — monotone predicate is enough** (sorted is the usual special case). Rotated arrays and "answer-space" searches show this.
2. **False.** `lo + hi` can overflow `int`; use `lo + (hi-lo)/2`.
3. **False.** `lo < hi` pairs with `hi = mid` (boundary style); `hi = mid-1` pairs with `lo <= hi` (exact-match style). Mixed styles skip candidates or loop forever.
4. **False.** Use `<=`; `<` breaks `lo == mid`.
5. **False.** One pass suffices.
6. **False.** `nums[lo]` can't tell whether the range is already sorted; `nums[hi]` vs `nums[mid]` encodes the predicate.
7. **False.** With duplicates `nums[lo] == nums[mid] == nums[hi]` is ambiguous: worst case O(n) (LC 81/154).

## Part D answers
* Per loop: "target (if present) is in `[lo,hi]`" / "min index is in `[lo,hi]`".
* `⌈log₂(10⁹)⌉ = 30`.
* `[3,1], 1` with `<`: `nums[0] < nums[0]` false → treated as "right half sorted" → `nums[mid]=3 < 1`? no → `hi = mid-1 = -1` → returns `-1`, wrong.
* Predicate `feasible(capacity)` = "schedule fits within capacity"; monotone (more capacity never hurts); search the smallest capacity with `true` over `[max_item, sum_all]`.
