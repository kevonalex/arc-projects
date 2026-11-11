# 06 · Sliding Window — Answers & Write-ups

Reference code: `code/solutions/sliding_window_solutions.cpp`; tests `code/tests/test_sliding_window.cpp` (9 tests, ASan+UBSan clean).

---

## P1 · 3 Longest Substring Without Repeating Characters

1. **Clarify.** Contiguous substring; any byte; empty → 0.
2. **Brute.** Every `(i,j)` with a set check: O(n²) pairs × O(n) check = O(n³); with early exit per start O(n²).
3. **Optimise.** "At most" predicate ⇒ valid windows are closed under sub-window ⇒ `L*(r)` non-decreasing (A1). Keep `next_start[c] = (last index of c) + 1`. At `r`: `l = max(l, next_start[s[r]])`; `best = max(best, r−l+1)`; `next_start[s[r]] = r+1`. **Invariant:** window `[l, r]` has no repeated characters, and `l` is the smallest such left end. The `max` matters: a stale occurrence *before* `l` must not pull `l` backwards.
4. **Code.** `array<int,256> next_start{}` (zero = "start anywhere").
5. **Trace `"abcabcbb"`:**

| r | c | l (after) | window | best |
|---|---|-----------|--------|------|
| 0 | a | 0 | a | 1 |
| 1 | b | 0 | ab | 2 |
| 2 | c | 0 | abc | 3 |
| 3 | a | max(0,1)=1 | bca | 3 |
| 4 | b | max(1,2)=2 | cab | 3 |
| 5 | c | max(2,3)=3 | abc | 3 |
| 6 | b | max(3,5)=5 | cb | 3 |
| 7 | b | max(5,7)=7 | b | 3 |

   Edge `"abba"`: r=0 a (l=0,best1); r=1 b (best2); r=2 b → `l=max(0,2)=2`, best 2; r=3 a → `next_start[a]=1`, `l = max(2,1) = 2` (**not** 1 — without `max` the window `[1,3]="bba"` would count a repeated `b` and report 3) → window `ba`, best **2** ✓. Edge `""` → 0.
6. **Final.** O(n) time, O(Σ)=O(256) space.

## P2 · 424 Longest Repeating Character Replacement

1. **Clarify.** Uppercase only; at most `k` replacements to any letters; longest resulting uniform substring.
2. **Brute.** For each substring, for each of 26 targets count mismatches: O(26·n²).
3. **Optimise.** For a window, the cheapest way to make it uniform is to keep the most frequent letter, so replacements needed = `len − maxFreq`. Window valid iff `len − maxFreq ≤ k`. Slide with a window that **never shrinks** (A4): for each `r`, add `s[r]`, update `maxFreq = max(maxFreq, cnt[s[r]])`; if `len − maxFreq > k`, drop `s[l]` and `l++` (window slides, size unchanged). `best = max(best, len)`.
   *Why a stale `maxFreq` is safe:* after a slide `maxFreq` might exceed the true max of the current window. The condition `len − maxFreq ≤ k` then may be true while the window is truly invalid — but then `len` equals the size of an earlier valid window that actually achieved that `maxFreq`, so `best` is not increased wrongly. `best` can grow only when `cnt[s[r]]` itself exceeds the old `maxFreq` — a true frequency in the current window.
4. **Code.** `cnt` array of 26; the `if` (not `while`) is intentional.
5. **Trace `"AABABBA", k=1`:**

| r | char | cnt | maxFreq | len | len−maxF | action | l | best |
|---|------|-----|---------|-----|----------|--------|---|------|
| 0 | A | A1 | 1 | 1 | 0 | – | 0 | 1 |
| 1 | A | A2 | 2 | 2 | 0 | – | 0 | 2 |
| 2 | B | A2 B1 | 2 | 3 | 1 | – | 0 | 3 |
| 3 | A | A3 B1 | 3 | 4 | 1 | – | 0 | 4 |
| 4 | B | A3 B2 | 3 | 5 | 2>1 | drop s[0]=A, l=1 | 1 | 4 |
| 5 | B | A2 B3 | 3 | 5 | 2>1 | drop s[1]=A, l=2 | 2 | 4 |
| 6 | A | A2 B3 | 3 | 5 | 2>1 | drop s[2]=B, l=3 | 3 | 4 |

   Answer **4** ✓. (Counts shown are *before* the drop.) After the last row the window is `"ABBA"` whose true max frequency is 2 but `maxFreq` is still 3 — a **stale** value. It is harmless: the window length stayed 4, equal to an already achieved size, so `best` did not change. Edge `"ABCDE", k=0` → every window of 2 is invalid, size stays 1 → **1**. Edge `k ≥ n` → `n`.
6. **Final.** O(n) time, O(26) space.

## P3 · 76 Minimum Window Substring

1. **Clarify.** `t` is a multiset; window must contain every character of `t` at least its multiplicity times; return the shortest such substring or `""`.
2. **Brute.** All `O(n²)` substrings × O(n) multiset check.
3. **Optimise.** "At least" predicate (A1). `need[c]` = required count (can go **negative** for surplus/irrelevant chars); `missing` = number of required characters still lacking (starts `|t|`).
   * add `s[r]`: `if (need[c]-- > 0) --missing;` — only when the character was still *needed* does it reduce `missing`.
   * while `missing == 0` (window valid): record if shorter; remove `s[l]`: `if (++need[c] > 0) ++missing;` — `missing` rises only if removal drops a required character below its quota (surplus chars have `need ≤ 0` after the increment); `l++`.
   **Invariant:** `need[c] = required(c) − count of c in window`; `missing = Σ_c max(0, need[c])`. Complexity: each pointer moves ≤ n.
4. **Code.** `array<int,256>` indexed by `unsigned char`.
5. **Trace `s=ADOBECODEBANC, t=ABC`:**

Initial `need = {A:1,B:1,C:1}`, `missing = 3`, `l = 0`. Indices: `A0 D1 O2 B3 E4 C5 O6 D7 E8 B9 A10 N11 C12`.

| r | char | effect of adding | missing | window valid? / shrinking | best so far |
|---|------|------------------|---------|---------------------------|-------------|
| 0 | A | need[A] 1→0 (was >0) | 2 | no | – |
| 1 | D | need[D] 0→−1 (surplus) | 2 | no | – |
| 2 | O | need[O] 0→−1 | 2 | no | – |
| 3 | B | need[B] 1→0 | 1 | no | – |
| 4 | E | need[E] 0→−1 | 1 | no | – |
| 5 | C | need[C] 1→0 | 0 | **valid** `[0,5]="ADOBEC"` len 6 → record. Remove A: need[A] 0→1 (>0) → missing 1, `l=1` | `ADOBEC` (6) |
| 6–8 | O D E | surplus, need[·] more negative | 1 | no | 6 |
| 9 | B | need[B] 0→−1 (surplus) | 1 | no | 6 |
| 10 | A | need[A] 1→0 | 0 | **valid** `[1,10]` len 10. Shrink: drop D, O (surplus), B (need[B] −1→0, not >0, still valid), E (surplus) → `[5,10]="CODEBA"` len 6, not shorter. Drop C: need[C] 0→1 → missing 1, `l=6` | 6 |
| 11 | N | surplus | 1 | no | 6 |
| 12 | C | need[C] 1→0 | 0 | **valid** `[6,12]="ODEBANC"` len 7. Shrink: drop O (7→`DEBANC`, len 6), D (`EBANC`, len 5 → record), E (`BANC`, len 4 → record). Drop B: need[B] −1→0… *see note* | **`BANC` (4)** |

   *Note:* at `r=12`, before dropping `B` at index 9, `need[B]` is `0` (the earlier surplus `B3` was dropped at `r=10`, bringing it from −1 to 0), so dropping `B9` gives `need[B]=1>0` → `missing=1`, `l=10`; the loop ends. Result `BANC` ✓. Edge `s="a", t="aa"`: after `a`, `need[a]=1`, `missing=1` forever → `""` ✓. Edge `t=""` → `""` (by our convention).
6. **Final.** O(|s| + |t|) time, O(Σ) space.

## P4 · 11 Container With Most Water

1. **Clarify.** Choose two indices `l<r`; area `min(h[l],h[r])·(r−l)`; maximise. Fewer than 2 walls → 0.
2. **Brute.** All pairs: O(n²).
3. **Optimise.** Start with the widest pair `(0, n−1)`. Move the **shorter** wall inward (A5). *Proof:* let `h[l] ≤ h[r]`. For every `r' < r`: width `r'−l < r−l` and height `min(h[l],h[r']) ≤ h[l]`, so area `≤ h[l]·(r−l) = current`. Therefore pairing `l` with anything closer than `r` cannot beat the current area; `l` has been fully dominated and can be discarded. Moving the taller wall instead could only shrink the width while the height stays capped by `h[l]` ⇒ useless. Each step removes one wall ⇒ n−1 steps.
4. **Code.** `while (l<r)`: evaluate, `if (h[l]<h[r]) ++l else --r`.
5. **Trace `[1,8,6,2,5,4,8,3,7]`:**

| l | r | h[l],h[r] | area | move |
|---|---|-----------|------|------|
| 0 | 8 | 1,7 | 8 | l++ |
| 1 | 8 | 8,7 | 7·7=49 | r-- |
| 1 | 7 | 8,3 | 3·6=18 | r-- |
| 1 | 6 | 8,8 | 8·5=40 | r-- (tie: either) |
| 1 | 5 | 8,4 | 4·4=16 | r-- |
| 1 | 4 | 8,5 | 5·3=15 | r-- |
| 1 | 3 | 8,2 | 2·2=4 | r-- |
| 1 | 2 | 8,6 | 6·1=6 | r-- → `l==r`, stop |

   best **49** ✓ (checked exhaustively vs brute force in tests). Edge `[1,1]` → 1; `[]`/`[5]` → 0; `[0,0,0]` → 0.
6. **Final.** O(n) time, O(1) space.

---

## Part C answers
1. **False.** Each pointer moves ≤ n; total ≤ 2n.
2. **False.** Use `max(l, …)`; `"abba"`.
3. **False.** Needs a monotone predicate; negative numbers break it (use prefix sums + hash map instead).
4. **False.** Stale `maxFreq` is safe (A4).
5. **False.** Only when the character was still needed (`need[c] > 0` before the decrement).
6. **False.** The window width is variable; it's the two-pointer elimination argument.

## Part D answers
* "Subarray sum = 7" with negatives: window `[l,r]` valid does not imply sub-windows valid or super-windows valid ⇒ no monotone pointer.
* `"abba"` without `max`: at r=3, `l` becomes 1 although the window `[2,3] = "ba"` is the smallest valid one — it would count `bba` (length 3).
* So that `need[c] ≤ 0` encodes *surplus* and the removal rule `++need[c] > 0` can tell required-dropping from surplus-dropping.
