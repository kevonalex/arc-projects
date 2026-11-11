# 13 · Dynamic Programming — Pattern Worksheet

**Pattern:** solve a problem by solving **smaller instances of the same problem once each** and combining them — works when the problem has **optimal substructure** and **overlapping subproblems**. Equivalent view: *DP is shortest/longest-path (or path-counting) on a DAG of states.*
**Why it matters at a radio/protocol shop:** the **Viterbi decoder** is DP on a trellis; sequence alignment, scheduling with constraints, resource allocation (grants/PRBs), and bit-error-optimal decoding are all the same recurrence shape.
**Coded:** 70, 198, 322, 300, 1143 — `code/headers/dp.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`. A runnable toy trellis is in `code/viterbi_demo.py`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (ranges, can the answer overflow, what if impossible, are choices ordered?) → 2 **Brute force + complexity** (write the exponential recursion first) → 3 **Optimise**: define the **state**, the **recurrence**, the **base cases**, the **evaluation order**, where the **answer** lives, and the **space reduction** → 4 **Code** → 5 **Hand-trace** the table for a normal input + an edge input → 6 **Final complexity** (`#states × cost per state`).

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the technique works

### A1. Optimal substructure (the cut-and-paste argument)
*A problem has optimal substructure if an optimal solution contains optimal solutions to its subproblems.* The standard proof: take an optimal solution `S` of the whole problem; the part of `S` that solves subproblem `P` must be optimal for `P` — **otherwise cut it out and paste in a better solution of `P`, getting a better whole solution, contradicting the optimality of `S`.**
Example (322, fewest coins for amount `a`): suppose an optimal solution for `a` uses coin `c` as its last coin; the other coins form a solution for `a − c`. If that sub-solution used more coins than `dp[a−c]`, replace it by an optimal one and get fewer than the claimed minimum — contradiction. Hence `dp[a] = 1 + min_c dp[a − c]`.
**Counter-example (it doesn't always hold):** the *longest simple path* in a graph — the two halves of a longest path need not be longest paths themselves, because the replacement may reuse vertices. Always check that sub-solutions are *independent* of the rest.

### A2. Overlapping subproblems ⇒ memoise
The naive recursion `ways(n) = ways(n−1) + ways(n−2)` recomputes `ways(k)` exponentially often (`φⁿ` calls). Storing each result makes it `n` states × O(1) each. **Complexity of any DP = (number of distinct states) × (work per state).** Top-down (memoised recursion) and bottom-up (table in dependency order) are the same DP; bottom-up avoids recursion depth and enables **space reduction**: if `dp[i]` only needs the last 1–2 rows/entries keep only those.

### A3. The five-part recipe
1. **State** — what do the indices *mean*? (`dp[i]` = best answer using the first `i` items / ending at `i` / for amount `i`).
2. **Recurrence** — partition the choices for the last step so the cases are **exhaustive and disjoint** (counting) or each is a legal last move (optimising).
3. **Base cases** — smallest states with known answers (`dp[0]`, `dp[1]`).
4. **Order** — every state depends only on states computed earlier.
5. **Answer** — the cell (or `min/max` over cells) that represents the original question.

### A4. DP = path problems on a DAG
Draw states as nodes, transitions as edges with costs. `dp[v] = min/max/sum over incoming edges (dp[u] + w(u,v))`. Because the graph is a DAG, a topological order is a valid evaluation order. 70 counts paths (sum), 198/300 find longest-weight paths, 322 finds the shortest path in the "amount" line DAG (min-plus), 1143 is the longest path in a grid DAG.

### A5. Connection to Viterbi (02-bits/06-conv-viterbi)
A convolutional encoder is a **finite-state machine**: for constraint length `K`, the state is the last `K−1` input bits (`2^(K−1)` states). Unroll it over time → a **trellis**: `T` stages × `2^(K−1)` states, two edges (input bit 0/1) leaving each state, each edge labelled with the code symbols it emits.
Maximum-likelihood decoding = *find the trellis path whose code symbols are closest (Hamming distance for hard decisions) to the received symbols*. Define
> `PM_t(s) = min over predecessors p →ᵘ s of ( PM_{t−1}(p) + BM(r_t, out(p,u)) )`, with `PM_0(start) = 0`, others `∞`.

This **is** the recurrence of 322 (`dp[a] = 1 + min dp[a−c]`) with “amount” replaced by (time, state) and “+1” replaced by the branch metric `BM`. **Optimal substructure (cut-and-paste):** the best path into state `s` at time `t` must pass through *some* predecessor `p` at `t−1`, and its prefix must be the best path into `p` — else swap in the better prefix. So per state we keep only the **survivor** (one best incoming path) — `2^(K−1)` survivors per step instead of `2^t` paths. Cost: `O(T · 2^(K−1))` versus `O(2^T)` brute force. Traceback of survivor pointers reconstructs the decoded bits exactly as you reconstruct the coins in 322. See `answers.md` for a worked 4-state trellis. Same idea for sequence alignment (1143 / edit distance) = DP on an alignment trellis.

---

## Part B — Problems

### P1 · LC 70 Climbing Stairs — `climb_stairs`
**Ctx:** number of distinct ways to tile `n` consecutive slots with 1-slot and 2-slot grants.
1. Clarify: `n ≥ 1`; answer fits `int` for `n ≤ 45`.
2. Brute: recursion `f(n)=f(n−1)+f(n−2)` → ____ time (why?)
3. Optimise: state/recurrence/base/order/answer (A3). Prove the recurrence by partitioning on the *last* move. Reduce space.
5. Trace: `n = 5` table; edge `n = 1, 2`.
6. Final: ____

### P2 · LC 198 House Robber — `rob`
**Ctx:** pick transmit slots of maximum total value with no two adjacent (guard-slot constraint).
1. Clarify: non-negative values; empty; one element.
2. Brute: recursion over take/skip → ____
3. Optimise: `best(i) = max(best(i−1), best(i−2) + a[i])`; justify take/skip exhaustiveness; two-variable rolling version.
5. Trace: `[2,7,9,3,1]` → 12 as a table; edge `[2,1,1,2]`, `[]`.
6. Final: ____

### P3 · LC 322 Coin Change — `coin_change`
**Ctx:** fewest grants (PRB chunks) of sizes in a given set that add up to exactly `amount`.
1. Clarify: unlimited supply; return `-1` if impossible; `amount = 0` → 0; large `amount`/coin overflow of an "infinity".
2. Brute: recursion trying every coin → ____
3. Optimise: A1 (**write the cut-and-paste proof**), `dp[a] = 1 + min dp[a−c]`; show greedy fails (`[1,3,4], 6`); choose `inf = amount+1` (why not `INT_MAX`?).
5. Trace: `[1,2,5], 11` table; edge `[2], 3`, `[1,3,4], 6`.
6. Final: ____

### P4 · LC 300 Longest Increasing Subsequence — `length_of_lis`
**Ctx:** longest in-order run of sequence numbers hidden inside a reordered stream.
1. Clarify: **strictly** increasing; subsequence (not contiguous); length only.
2. Brute: all subsets → ____
3. Optimise: (a) `dp[i] = 1 + max dp[j]`, `j<i`, `a[j]<a[i]` → ____ ; (b) patience/`tails` with binary search → ____ . Write the `tails` invariant and prove why replacing is safe.
5. Trace: `[10,9,2,5,3,7,101,18]` → 4 (show `tails` after each element); edge all equal, decreasing, increasing.
6. Final: ____

### P5 · LC 1143 Longest Common Subsequence — `longest_common_subsequence`
**Ctx:** align the received symbol stream with the expected one — the largest in-order agreement.
1. Clarify: subsequence not substring; strings may be empty.
2. Brute: all subsequences of one string checked against the other → ____
3. Optimise: 2-D state `dp[i][j]`; recurrence by comparing last characters; justify the "match ⇒ diagonal" case; reduce space.
5. Trace: `"abcde"` vs `"ace"` → 3 (fill the table); edge empty strings, identical strings.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "DP is just recursion with a fancy array."
2. "Greedy gives the optimum for Coin Change if you always take the biggest coin."
3. "`dp` must be two-dimensional for 2-D problems."
4. "LIS `tails` array is the longest increasing subsequence itself."
5. "Use `INT_MAX` as ∞ and add 1 freely."
6. "Every optimisation problem has optimal substructure."
7. "Viterbi is a different kind of algorithm from DP."

## Part D — Self-check
* For each problem name: state, recurrence, base, order, answer.
* Give the number of states × work per state for all five.
* State the cut-and-paste proof in your own words for one problem.
* Map each Viterbi term (state, stage, branch metric, survivor, traceback) onto a Coin Change term.
