# 12 · Intervals — Pattern Worksheet

**Pattern:** **sort**, then make **one sweep** while keeping a tiny summary of "what is open right now" (the last merged interval, or the last chosen end). Sorting turns a 2-D relationship (every pair) into a 1-D one (neighbours).
**Why it matters at a radio/protocol shop:** time-slot / TDMA grants, spectrum blocks, reserved resource-block ranges, scheduled transmissions, retransmission windows, byte-range reassembly.
**Coded:** 56, 57, 435 — `code/headers/intervals.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (closed vs half-open? touching intervals overlap? sorted input? duplicates? empty list? `INT` extremes?) → 2 **Brute force + complexity** → 3 **Optimise** (what do I sort by, and why does that make the sweep correct?) → 4 **Code** → 5 **Hand-trace** a normal list + nested/touching/empty cases → 6 **Final complexity** (sorting dominates unless input is pre-sorted).

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the technique works

### A1. Overlap test and what sorting buys
Closed intervals `[a,b]` and `[c,d]` **overlap** (share at least one point) iff `a ≤ d` **and** `c ≤ b`. If we sort by start (`a ≤ c`) the first condition is automatic, so overlap ⇔ `c ≤ b`: "the later one starts before the earlier one ends". **Touching** intervals (`b == c`) overlap in this convention (56/57) — and *don't conflict* in 435 (a slot ending at 2 and a slot starting at 2 can both be used). **Always ask.**

### A2. Merge sweep correctness (56)
Sort by start. Keep an output list `out` that, as an **invariant**, is sorted and pairwise disjoint (`out[i].end < out[i+1].start`) and equals the merge of the processed prefix.
For the next interval `iv` (so `iv.start ≥` every earlier start): it can only overlap the **last** output interval — any earlier `out[i]` ends before `out.back().start ≤ iv.start`. If `iv.start ≤ out.back().end`, extend: `out.back().end = max(out.back().end, iv.end)` (`max` because `iv` may be nested inside). Otherwise append. Both branches preserve the invariant. **Cost:** sort `O(n log n)` + sweep `O(n)`.

### A3. Insert into a sorted disjoint list (57)
Input is already sorted and disjoint ⇒ no sort. Three phases over one index: (1) copy intervals that end **strictly before** `new.start`; (2) while the next interval starts at or before `new.end`, absorb it (`new.start = min`, `new.end = max`); (3) emit `new`, then copy the rest. Each interval is touched once: `O(n)`. Correct because the list is sorted by both start and end (disjoint), so the three groups are contiguous: before / overlapping / after.

### A4. Greedy interval scheduling — earliest finish time (435)
Goal: remove the **fewest** intervals so the remainder is pairwise non-overlapping = keep the **largest** compatible subset; answer `n − kept`.
**Greedy:** sort by **end**; keep an interval iff its start ≥ the end of the last kept one.
**Exchange argument:** let `g` be the interval with the smallest end. Take *any* optimal solution `O` and let `o` be its interval with the smallest end (so `g.end ≤ o.end`). Replace `o` by `g`: every other interval in `O` starts at or after `o.end ≥ g.end`, so `g` is compatible with all of them and `|O|` is unchanged. Hence **some optimal solution contains `g`**; remove `g` and everything overlapping it and recurse on the rest (intervals starting ≥ `g.end`) — induction gives optimality.
**Why not sort by start?** `[0,100], [1,2], [3,4]`: start-order keeps `[0,100]` and loses both short ones (kept 1, optimal 2). **Why not shortest-first?** A short interval in the middle can block two longer compatible ones: `[0,3],[2,4],[3,6]` — the shortest is `[2,4]`, which overlaps both others, so shortest-first keeps 1 interval while the optimum keeps `[0,3],[3,6]` = 2. (Earliest-finish keeps `[0,3]` then `[3,6]` ✓.)

### A5. Related sweeps (know them, not coded)
Max simultaneous overlap (meeting rooms II): sort start & end events separately or use a min-heap of ends; `O(n log n)`. Union-length: merge then sum.

---

## Part B — Problems

### P1 · LC 56 Merge Intervals — `merge_intervals`
**Ctx:** coalesce overlapping reserved time slots into maximal busy blocks.
1. Clarify: input unsorted? touching `[1,4],[4,5]` merge? nested? duplicates? empty list; `INT` extremes.
2. Brute: repeatedly find any overlapping pair and merge until none → ____
3. Optimise: A2. Why only compare with the *last* output interval?
5. Trace: `[[1,3],[2,6],[8,10],[15,18]]`; edges: unsorted, nested `[1,10],[2,3]`, touching, duplicates, one interval.
6. Final: ____

### P2 · LC 57 Insert Interval — `insert_interval`
**Ctx:** add one new grant to an existing sorted, non-overlapping grant table.
1. Clarify: input sorted & disjoint; new may touch/swallow several; list may be empty.
2. Brute: append, then run 56 → ____
3. Optimise: A3 — three phases; why no sort?
5. Trace: `[[1,3],[6,9]]` + `[2,5]`; `[[1,2],[3,5],[6,7],[8,10],[12,16]]` + `[4,8]`; edges: before all, after all, in a gap, swallows all, touches an end.
6. Final: ____

### P3 · LC 435 Non-overlapping Intervals — `erase_overlap_intervals`
**Ctx:** minimum number of scheduled transmissions to cancel so the rest never collide on the same channel.
1. Clarify: touching OK (`[1,2],[2,3]`); returns the **count** to remove.
2. Brute: try all subsets → ____ ; DP over sorted intervals → ____
3. Optimise: A4 — state the greedy and write the exchange argument; give the counter-example for sorting by start.
5. Trace: `[[1,2],[2,3],[3,4],[1,3]]` → 1; `[[1,2],[1,2],[1,2]]` → 2; edge `[[0,100],[1,2],[3,4]]`.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "After sorting by start, a new interval might overlap any earlier output interval."
2. "When merging, just set `end = iv.end`."
3. "`[1,2]` and `[2,3]` overlap in 435."
4. "For 435, sort by start and keep the first."
5. "57 needs a sort."
6. "Intervals problems are O(n) — the sort is free."

## Part D — Self-check
* State the overlap predicate for closed intervals and simplify it after sorting by start.
* Why is `max(end, iv.end)` needed in 56?
* Write the exchange argument for earliest-finish in 4 sentences.
* How do you count the maximum number of overlapping intervals?
