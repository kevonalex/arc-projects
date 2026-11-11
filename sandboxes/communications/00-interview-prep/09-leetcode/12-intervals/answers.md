# 12 · Intervals — Answers & Write-ups

Reference code: `code/solutions/intervals_solutions.cpp`; tests `code/tests/test_intervals.cpp` (6 tests, ASan+UBSan clean).

---

## P1 · 56 Merge Intervals

1. **Clarify.** Closed intervals; unsorted input; touching intervals merge (`[1,4],[4,5] → [1,5]`); nested ones are absorbed; empty input → empty output.
2. **Brute.** Repeatedly scan for any overlapping pair, merge, restart: O(n²)–O(n³).
3. **Optimise.** Sort by start (A2), then sweep keeping `out` disjoint and sorted. For `iv`: `if (!out.empty() && iv.start <= out.back().end) out.back().end = max(out.back().end, iv.end); else out.push_back(iv);`. **Why only the last:** `out[i].end < out[i+1].start` (disjoint, sorted) and `iv.start ≥ out.back().start`, so for any earlier `out[i]`, `out[i].end < out.back().start ≤ iv.start` ⇒ no overlap. **Why `max`:** nested `[1,10]` then `[2,3]` must not shrink the end.
4. **Code.** `std::sort` on `vector<vector<int>>` orders lexicographically (start, then end).
5. **Trace `[[1,3],[2,6],[8,10],[15,18]]`:**
```
[1,3]  out=[[1,3]]
[2,6]  2<=3 overlap -> end=max(3,6)=6   out=[[1,6]]
[8,10] 8>6          -> append           out=[[1,6],[8,10]]
[15,18]15>10        -> append           out=[[1,6],[8,10],[15,18]]  ✓
```
   Edges: unsorted `[[8,10],[1,3],[2,6]]` → sorted first → `[[1,6],[8,10]]`. Nested `[[1,10],[2,3],[4,5]]` → `[[1,10]]` (end stays 10 thanks to `max`). Touching `[[1,4],[4,5]]` → `[[1,5]]`. Duplicates → one. Empty → `[]`. Extremes `[INT_MIN, INT_MAX]` — no arithmetic on endpoints, so no overflow.
6. **Final.** O(n log n) time (sort), O(n) output space (O(1) extra beyond the sort).

## P2 · 57 Insert Interval

1. **Clarify.** `intervals` sorted by start, pairwise disjoint; `newInterval` may fall anywhere and may overlap several. Touching merges (`[1,5]` + `[5,7]` → `[1,7]`).
2. **Brute.** Append and call 56: O(n log n).
3. **Optimise.** Linear three-phase scan (A3): (1) `while end < new.start` copy; (2) `while start <= new.end` absorb (`min`/`max`); (3) push `new`, copy the rest. No sort needed since the list is sorted by both start and end. Boundary conditions: phase 1 uses `<` (strictly before: touching must be absorbed), phase 2 uses `<=` (touching absorbed).
4. **Code.** see solution (`new` taken by value so we can mutate it).
5. **Trace `[[1,3],[6,9]]` + `[2,5]`:**
```
phase1: [1,3].end=3 < 2? no
phase2: [1,3].start=1 <= 5 -> new=[min(2,1), max(5,3)]=[1,5]; [6,9].start=6 <= 5? no
phase3: push [1,5]; copy [6,9]  -> [[1,5],[6,9]] ✓
```
   `[[1,2],[3,5],[6,7],[8,10],[12,16]]` + `[4,8]`: phase1 copies `[1,2]`; phase2 absorbs `[3,5]`,`[6,7]`,`[8,10]` → `[3,10]`; phase3 pushes `[3,10]`, copies `[12,16]` ✓. Edges: empty list → `[new]`; new before all → phase 1/2 empty, `new` first; after all → phase 1 copies everything, `new` last; swallowed `[1,5]` + `[2,3]` → `[[1,5]]`; touch `[1,5]` + `[5,7]` → `[[1,7]]`; fits in a gap `[[1,2],[5,6]]` + `[3,4]` → unchanged neighbours.
6. **Final.** O(n) time, O(n) output space.

## P3 · 435 Non-overlapping Intervals

1. **Clarify.** Return the **minimum number to remove** so the rest are pairwise non-overlapping, where touching (`[1,2]`,`[2,3]`) is allowed.
2. **Brute.** Subset enumeration O(2ⁿ·n). A DP over intervals sorted by end (LIS-like, `dp[i] = 1 + max dp[j]` with `end_j ≤ start_i`) is O(n²).
3. **Optimise.** `n − (max compatible subset)`. Greedy: sort by **end**, keep an interval iff `start ≥ last_end`. **Proof (exchange, A4):** among all optimal solutions there is one containing the interval `g` with the smallest end: take an optimal `O`, swap its earliest-ending interval `o` for `g` — `g.end ≤ o.end ≤` every other start in `O`, so `g` stays compatible and `|O|` doesn't change. Remove `g` and all intervals overlapping it; the remaining problem (intervals starting ≥ `g.end`) is the same problem on fewer intervals ⇒ induction.
   *Counter-examples to other greedy rules:* by start — `[0,100],[1,2],[3,4]` keeps 1, optimal keeps 2; by length — `[0,3],[2,4],[3,6]`: shortest is `[2,4]`, which overlaps both others → keeps 1, optimal keeps `[0,3],[3,6]` = 2.
4. **Code.** sort with a comparator on `a[1] < b[1]`; `>=` for touching.
5. **Trace `[[1,2],[2,3],[3,4],[1,3]]`:** sorted by end: `[1,2],[2,3],[1,3],[3,4]` (the tie between `[2,3]` and `[1,3]` may come out in either order — the removal count is the same). keep `[1,2]` (last_end=2); `[2,3]`: `2 ≥ 2` keep (last_end=3); `[1,3]`: `1 ≥ 3`? no → remove; `[3,4]`: `3 ≥ 3` keep. kept 3, `n=4` → remove **1** ✓. `[[1,2],[1,2],[1,2]]`: keep first; others start 1 < 2 → remove 2 ✓. Edge `[[0,100],[1,2],[3,4]]`: sorted by end `[1,2],[3,4],[0,100]` → keep `[1,2]`, `[3,4]`; `[0,100]` removed → **1** ✓ (start-order would keep `[0,100]`). Edge empty → 0; single → 0.
6. **Final.** O(n log n) time (sort), O(1) extra (besides sort).

---

## Part C answers
1. **False.** Only the last output interval can (disjoint + sorted invariant).
2. **False.** Use `max` — nested intervals.
3. **False.** In 435 touching is allowed (non-overlapping).
4. **False.** Sort by **end** (earliest finish).
5. **False.** The input is already sorted: one linear pass.
6. **False.** Sorting costs O(n log n); only pre-sorted inputs (57) are O(n).

## Part D answers
* Closed intervals overlap iff `a ≤ d ∧ c ≤ b`; with `a ≤ c` it simplifies to `c ≤ b`.
* So that a later interval nested inside the current one doesn't shrink it.
* "Let `g` end first. Any optimal solution's first interval ends no earlier than `g`. Swapping it for `g` keeps compatibility with everything else. So an optimal solution contains `g`; recurse on intervals starting after `g`."
* Sort starts and ends separately and sweep (+1 at start, −1 at end), or a min-heap of end times: O(n log n).
