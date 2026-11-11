# 09 · Binary Search — Pattern Worksheet

**Pattern:** if a yes/no predicate over an index range is **monotone** (`F F F … F T T … T`), you can find the boundary in O(log n) by repeatedly halving the range, keeping an **invariant** that the boundary is inside it.
**Why it matters at a radio/protocol shop:** channel plans and sorted tables, timestamp lookup in ring buffers that have wrapped, searching sorted frame-id windows, "find the smallest threshold that works" (SNR / power / retry count).
**Coded:** 704, 33, 153 — `code/headers/binary_search.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (sorted? duplicates? rotated? return index or value? empty?) → 2 **Brute force + complexity** → 3 **Optimise** (state the **predicate**, why it is monotone, the **interval invariant** and the **loop condition**) → 4 **Code** (overflow-safe `mid`) → 5 **Hand-trace** `(lo, hi, mid)` for a normal + edge input → 6 **Final complexity**.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the technique works

### A1. Monotone predicate + interval invariant
Let `P(i)` be monotone on `[0, n)`: false up to some boundary `b` and true from `b` on (`b` may be `n` = "never true"). Maintain `[lo, hi]` with:

> **Invariant:** the boundary `b` satisfies `lo ≤ b ≤ hi`.

Probe `mid`. If `P(mid)` is true then `b ≤ mid`, so set `hi = mid`; else `b > mid`, set `lo = mid + 1`. Either way the interval strictly shrinks (by at least one element, roughly half) and still contains `b`. When `lo == hi` we have found `b`. **Iterations:** the size roughly halves each step ⇒ `⌈log₂(n+1)⌉` ⇒ **O(log n) time, O(1) space**. Proof of progress: with `mid = lo + ⌊(hi − lo)/2⌋` we always have `lo ≤ mid < hi` while `lo < hi`, so both updates make the interval smaller.

### A2. Two styles — pick one and be consistent
* **Exact match on a closed interval `[lo, hi]`** (704): loop `while (lo <= hi)`, `mid` equal → return; `lo = mid+1` or `hi = mid-1`. *Invariant:* if `target` is present it lies in `[lo, hi]`. Ends when the interval is empty (`lo > hi`) ⇒ absent.
* **Boundary search on `[lo, hi]` with `hi = mid`** (153): loop `while (lo < hi)`; `hi = mid` keeps `mid` as a candidate. Ends with `lo == hi` = the answer.
Mixing the two styles (`hi = mid` with `lo <= hi`) is the classic infinite loop.

### A3. Overflow-safe mid
`(lo + hi) / 2` overflows `int` when `lo + hi > INT_MAX`. Use `lo + (hi − lo) / 2`. (Famously, the JDK's `Arrays.binarySearch` shipped with this overflow bug for years.)

### A4. Rotated sorted array (distinct values)
A rotation of a sorted array is **two ascending runs**: the high run `A` followed by the low run `B`, with *every element of A greater than every element of B* (single "drop" between them).
**Key fact (33):** for any `[lo, hi]` and `mid`, **at least one of `[lo, mid]` and `[mid, hi]` is sorted (contains no drop).** Proof: there is at most one drop. If `nums[lo] ≤ nums[mid]`, a drop inside `(lo, mid]` would force `nums[lo] ∈ A` and `nums[mid] ∈ B` hence `nums[lo] > nums[mid]` — contradiction; so `[lo, mid]` is sorted. Otherwise `nums[lo] > nums[mid]`, so the drop is in `(lo, mid]`, and `[mid, hi]` (to the right of the only drop) is sorted. In the sorted half a **range test** (`left_end ≤ target < right_end`) tells us whether the target is inside; if not it must be in the other half. Needs *distinct* values — with duplicates (`81`) `nums[lo] == nums[mid]` is ambiguous and worst case degrades to O(n).
**Key fact (153):** the predicate `P(i) = nums[i] ≤ nums[n−1]` is monotone (`F…F T…T`) on a rotated array (the high run `A` is all `> nums[n−1]`, the low run `B` is all `≤`). The minimum is the **first true**. Comparing `nums[mid]` with `nums[hi]` implements that test. Compare with `hi`, not `lo`, because `nums[lo]` doesn't discriminate when the range is already sorted.

### A5. Binary search on the answer
Whenever "is value `x` feasible?" is monotone in `x` (minimum capacity/power/threshold) you can binary-search `x` with a feasibility check. Same invariant, same proof.

---

## Part B — Problems

### P1 · LC 704 Binary Search — `search`
**Ctx:** look up a channel number in a sorted channel plan.
1. Clarify: sorted ascending, distinct; return index or `-1`; empty input.
2. Brute: linear scan → ____
3. Optimise: A1/A2/A3. State the invariant for the closed-interval style and the loop condition.
5. Trace: `[-1,0,3,5,9,12], 9` and `…, 2`. Edge: empty, size 1, target below/above range, `INT_MIN/INT_MAX` values.
6. Final: ____

### P2 · LC 33 Search in Rotated Sorted Array — `search_rotated`
**Ctx:** a ring buffer of sorted timestamps has wrapped; find a sample without un-rotating the buffer.
1. Clarify: distinct values; rotated by an unknown pivot (maybe 0); return index or `-1`.
2. Brute: linear scan → ____ ; find pivot then two binary searches → ____
3. Optimise: A4 (**prove at least one half is sorted**). Why is `nums[lo] <= nums[mid]` (with `<=`) the right test?
5. Trace: `[4,5,6,7,0,1,2], target 0` as `(lo,hi,mid, sorted half, decision)`. Edge: `[3,1], 1` (`lo == mid`), unrotated array, absent target.
6. Final: ____

### P3 · LC 153 Find Minimum in Rotated Sorted Array — `find_min`
**Ctx:** find the wrap point (oldest sample) of a ring buffer holding sorted data.
1. Clarify: distinct values; non-empty; rotation may be 0.
2. Brute: linear scan → ____
3. Optimise: A4 (predicate `nums[i] ≤ nums[n−1]`); use the boundary-search style. Why compare with `hi` and why `hi = mid` (not `mid−1`)?
5. Trace: `[3,4,5,1,2]` and `[11,13,15,17]` (not rotated). Edge: size 1, size 2 both orders.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "Binary search needs the array to be sorted."
2. "`(lo+hi)/2` is fine for all array sizes."
3. "`while (lo < hi)` with `hi = mid - 1` is a safe combination."
4. "In 33, I should use `nums[lo] < nums[mid]` to detect the sorted left half."
5. "Rotated-array search needs two passes (find pivot, then search)."
6. "In 153 comparing `nums[mid]` with `nums[lo]` works as well as with `nums[hi]`."
7. "Duplicates don't change the complexity of 33/153."

## Part D — Self-check
* Write the invariant for each of your loops in one sentence.
* How many probes does a 10⁹-element search need at most? (≈ 30)
* Give an input where `<` instead of `<=` in step 33 returns the wrong index.
* Turn "smallest capacity such that the schedule fits" into a binary search: what is the predicate?
