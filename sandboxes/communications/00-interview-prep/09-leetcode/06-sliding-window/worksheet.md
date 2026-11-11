# 06 · Sliding Window — Pattern Worksheet

**Pattern:** maintain a window `[l, r]` over a sequence with an incrementally updated summary (counts, sums, last-seen positions). Advance `r` to grow; advance `l` to restore validity. Each pointer only moves right ⇒ **O(n)**.
**Why it matters at a radio/protocol shop:** every "over the last N symbols/samples" question — burst detection, preamble uniqueness, shortest segment containing all markers, error-correction budgets.
**Coded:** 3, 424, 76, 11 — `code/headers/sliding_window.hpp`. (11 is a *two-pointer* problem; it's here because it proves the same "safe elimination" idea.) Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (alphabet, case, empty, `k` bounds, "longest" vs "shortest", what exactly makes a window valid) → 2 **Brute force + complexity** → 3 **Optimise** (state the validity predicate, why it is monotone, and what the window summary is) → 4 **Code** → 5 **Hand-trace** a normal and an edge input as a table of `(l, r, summary, best)` → 6 **Final complexity**.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the technique works

### A1. Monotone validity ⇒ monotone pointers
Define `valid(l, r)` for a window. The technique is correct when **validity is monotone under shrinking or growing**:

* **"At most" constraints** (3: no repeats; 424: ≤ k replacements): *if `[l,r]` is valid then every sub-window is valid.* So for each right end `r`, the set of valid `l` is a suffix `[L*(r), r]`, and `L*(r)` is **non-decreasing in `r`** (a sub-window of a valid window is valid; hence if `[l, r+1]` is valid so is `[l, r]`, i.e. `L*(r+1) ≥ L*(r)`). We can therefore find `L*(r)` by only ever moving `l` rightwards — no backtracking.
* **"At least" constraints** (76: contains all of `t`): *if `[l,r]` is valid then every super-window is valid.* For each `l` the minimal valid `r` is non-decreasing in `l`. We grow `r` until valid, then shrink `l` while still valid.

**Complexity proof.** `r` moves at most `n` times and `l` moves at most `n` times (it never exceeds `r`, never goes back). Each move updates the summary in O(1) ⇒ **O(n) time**; extra space = size of the summary (O(alphabet) or O(k)).

### A2. The window summary must support *add* and *remove* in O(1)
Counts per character, a running sum, a `last_seen[c]` array, a "missing" counter. If you can't remove an element in O(1) (e.g. a running *max*), use a monotonic deque (03-queues-buffers).

### A3. Longest-window problems — jump vs. step
In 3, a repeat at position `p` means every window containing both `p` and `r` is invalid, so `l` can **jump** to `p+1` using `last_seen` (but never backwards: `l = max(l, last+1)` — "abba" breaks the min-less version).

### A4. The non-shrinking window (424)
Window `[l,r]` is valid iff `(r−l+1) − maxFreq ≤ k` (everything not the most common letter gets replaced). We only want the *longest*. Keep the window size monotone: when the window becomes invalid, slide it (move `l` one step) instead of shrinking until valid. `maxFreq` is only ever *increased*: if it's stale (too big), the window might look valid when it isn't — but then its size equals a size we've already achieved, so `best` cannot be wrongly increased. The answer only grows when a **true** larger `maxFreq` occurs. Hence the stale maximum is harmless.

### A5. Two-pointer elimination (11)
Take walls `l < r`. Area `= min(h[l], h[r])·(r−l)`. Suppose `h[l] ≤ h[r]`. Any pair `(l, r')` with `r' < r` has smaller width and height `≤ h[l]` ⇒ area ≤ current. So wall `l` can **never** be part of a better pair with a closer right wall and has been fully evaluated against all farther ones ⇒ discard it (`l++`). Symmetric for `r`. Each step discards one wall ⇒ O(n); and the optimum is never discarded **before** it has been examined (it would have to be paired with a wall that is already discarded, which we just showed is not better).

---

## Part B — Problems

### P1 · LC 3 Longest Substring Without Repeating Characters — `length_of_longest_substring`
**Ctx:** longest burst of pairwise-distinct symbols (e.g. before a symbol repeats in a hopping/sync sequence).
1. Clarify: ASCII? empty string → 0? substring (contiguous) not subsequence?
2. Brute: all substrings with a set check → ____
3. Optimise: A1 (monotone, "at most") + A3 (jump). Write the invariant on `last_seen`.
5. Trace: `"abcabcbb"`, `"abba"`, `"tmmzuxt"` as `(l, r, window)` tables.
6. Final: ____

### P2 · LC 424 Longest Repeating Character Replacement — `character_replacement`
**Ctx:** longest stretch that can be made constant (all same symbol) with at most `k` symbol corrections.
1. Clarify: uppercase A–Z; `k ≥ 0`; replace to *any* letter.
2. Brute: for each substring and each target letter count mismatches → ____
3. Optimise: A4. Why is "replacements needed = len − maxFreq"? Explain the stale-`maxFreq` argument.
5. Trace: `"AABABBA", k=1` → 4; edge `"ABCDE", k=0` → 1.
6. Final: ____

### P3 · LC 76 Minimum Window Substring — `min_window`
**Ctx:** shortest stretch of a captured stream that contains every required marker with its multiplicity.
1. Clarify: `t` may have duplicates (multiset); case-sensitive; if impossible return `""`; ties → any (LeetCode guarantees uniqueness).
2. Brute: all windows × multiset check → ____
3. Optimise: A1 ("at least"). Describe `need[]` and the `missing` counter precisely: when exactly does `missing` change?
5. Trace: `s="ADOBECODEBANC", t="ABC"` → `BANC`; edge `s="a", t="aa"`.
6. Final: ____

### P4 · LC 11 Container With Most Water — `max_area`
**Ctx:** choose two masts maximising `min(height)·distance` (a link-budget toy).
1. Clarify: heights ≥ 0; at least 2 walls; width = index difference.
2. Brute: all pairs → ____
3. Optimise: A5. Prove that moving the *shorter* wall is safe. Why is moving the taller wall useless?
5. Trace: `[1,8,6,2,5,4,8,3,7]` → 49.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "Sliding window is O(n·k) because of the inner `while`."
2. "In 3, set `l = last_seen[c] + 1` unconditionally."
3. "Sliding window works for any array problem about subarrays (e.g. with negative numbers and a target sum)."
4. "In 424 you must recompute `maxFreq` when the left pointer moves."
5. "In 76, `missing` should decrease on every character of `s`."
6. "Container With Most Water is a sliding-window problem with a fixed window."

## Part D — Self-check
* Give a predicate that is *not* monotone (hint: subarray sum equals target with negatives) and say why two pointers fails.
* Show a trace of `"abba"` where `l = last+1` (no `max`) goes wrong.
* Why does 76 need `need[c]--` to go negative for non-required characters?
