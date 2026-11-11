# 13 · Dynamic Programming — Cheatsheet

**Core idea:** optimal substructure + overlapping subproblems ⇒ store each state once. **Complexity = #states × work/state.** DP = best/count path on a DAG.

## Five-part recipe
**State** (what do indices mean) → **Recurrence** (exhaustive, disjoint last choice) → **Base** → **Order** → **Answer** (+ space reduction).

## Optimal substructure (proof pattern)
Take an optimal solution; its sub-part must be optimal for the subproblem, else cut and paste a better one ⇒ contradiction.
Fails when sub-solutions interfere (longest *simple* path).

## Recipes
| LC | State | Recurrence | Time / Space |
|----|-------|-----------|--------------|
| 70 | `dp[i]` ways to reach `i` | `dp[i-1]+dp[i-2]` | O(n) / O(1) |
| 198 | `best(i)` over first `i` | `max(best(i-1), best(i-2)+a[i])` | O(n) / O(1) |
| 322 | `dp[a]` min coins | `1 + min dp[a-c]`, `dp[0]=0`, inf = `amount+1` | O(amount·coins) / O(amount) |
| 300 | `dp[i]` LIS ending at `i` / `tails[k]` = min tail of len k+1 | `1+max dp[j]` / `lower_bound` | O(n²) / O(n log n) |
| 1143 | `dp[i][j]` LCS of prefixes | match: `dp[i-1][j-1]+1`; else `max(dp[i-1][j], dp[i][j-1])` | O(mn) / O(min(m,n)) |

## Viterbi ↔ DP
Trellis = unrolled FSM (`2^(K-1)` states × T stages). `PM_t(s) = min_{p→s} PM_{t-1}(p) + BM`. Keep **one survivor** per state (cut-and-paste). Traceback = parent pointers. Cost `O(T·2^(K-1))` vs `2^T`. Same shape as 322 (min-plus) and LCS (grid DAG). Toy: `code/viterbi_demo.py`.

## Techniques
* Top-down memo vs bottom-up table; bottom-up allows rolling arrays.
* Reconstruct solution: store parent/choice, walk back.
* Greedy ≠ DP: `[1,3,4], 6`.
* LIS strictness → `lower_bound` (strict) vs `upper_bound` (non-decreasing).
* Count problems: use `long long`/mod; check overflow.

## Gotchas
* Don't use `INT_MAX` as ∞ if you add to it.
* Initialise `dp[0]` correctly (`0` ways vs `1` way: counting uses 1).
* LIS answer = `max dp[i]`, not the last cell; `tails` isn't the LIS.
* LCS: subsequence ≠ substring; rolling rows must keep the previous row.
* 198 with 0/1 elements; 322 with `amount = 0`.
* Iteration order must respect dependencies.

## Script
"Clarify choices/ranges → exponential recursion → define state, recurrence (last choice, exhaustive & disjoint), base → memoise/tabulate → trace the table → #states × work → reduce space."
