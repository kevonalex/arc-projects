# 13 · Dynamic Programming — Answers & Write-ups

Reference code: `code/solutions/dp_solutions.cpp`; tests `code/tests/test_dp.cpp` (11 tests, ASan+UBSan clean). Viterbi toy: `code/viterbi_demo.py` (output reproduced below).

---

## P1 · 70 Climbing Stairs

1. **Clarify.** Reach step `n` taking 1 or 2 steps; count distinct sequences. `1 ≤ n ≤ 45` fits `int` (`F(46)` = 1 836 311 903 < 2³¹; this is `n=45`).
2. **Brute.** `f(n) = f(n−1) + f(n−2)` recursion: the call tree has `Θ(φⁿ)` nodes (φ ≈ 1.618), each leaf contributes 1.
3. **Optimise.**
   * **State** `dp[i]` = number of ways to reach step `i`. **Recurrence** partition by the *last* move: it was a 1-step from `i−1` or a 2-step from `i−2` — exhaustive and disjoint ⇒ `dp[i] = dp[i−1] + dp[i−2]`. **Base** `dp[1]=1, dp[2]=2`. **Order** increasing `i`. **Answer** `dp[n]`. **Space** only the last two values.
   * (It is the Fibonacci sequence; matrix power gives O(log n) — overkill here.)
4. **Code.** two rolling variables.
5. **Trace `n=5`:** `dp1=1, dp2=2, dp3=3, dp4=5, dp5=8` → **8** ✓ (`11111, 1112 ×4 positions, 122 ×3 positions → 1 + 4 + 3 = 8`). Edge `n=1 → 1`, `n=2 → 2`, `n=45 → 1836311903`.
6. **Final.** 𝑛 states × O(1) = O(n) time, O(1) space.

## P2 · 198 House Robber

1. **Clarify.** Choose a subset of indices with **no two adjacent** maximising the sum; values ≥ 0; empty → 0.
2. **Brute.** Recursion over take/skip: `T(n) = T(n−1) + T(n−2)` ⇒ exponential; or enumerate 2ⁿ subsets.
3. **Optimise.** **State** `best(i)` = max value using the first `i` items. **Recurrence:** item `i` is either *skipped* (`best(i−1)`) or *taken*, in which case `i−1` is forbidden (`best(i−2) + a[i]`): `best(i) = max(best(i−1), best(i−2) + a[i])`. **Base** `best(0)=0`, `best(1)=a[1]`. **Optimal substructure:** if an optimal set takes `i`, its remainder is a valid set on the first `i−2` items and must be optimal there (cut-and-paste). Rolling variables `skip = best(i−2)`, `take = best(i−1)`.
4. **Code.** `cur = max(take, skip + x); skip = take; take = cur;`
5. **Trace `[2,7,9,3,1]`:**

| i | a[i] | best(i−2) | best(i−1) | best(i) = max(best(i−1), best(i−2)+a[i]) |
|---|------|-----------|-----------|----------------------------------------|
| 1 | 2 | 0 | 0 | 2 |
| 2 | 7 | 0 | 2 | 7 |
| 3 | 9 | 2 | 7 | 11 |
| 4 | 3 | 7 | 11 | 11 |
| 5 | 1 | 11 | 11 | **12** |

   Answer 12 (`2 + 9 + 1`) ✓. Edge `[2,1,1,2]` → `best`: 2,2,3,**4** (take both 2s) ✓. Edge `[]` → 0; `[5]` → 5; `[100,1,1,100]` → 200.
6. **Final.** O(n) time, O(1) space.

## P3 · 322 Coin Change

1. **Clarify.** Unlimited coins of each denomination; fewest coins summing exactly to `amount`; `−1` if impossible; `amount=0 → 0`. Coins positive.
2. **Brute.** `f(a) = 1 + min_c f(a−c)` as plain recursion: up to `|coins|^(a / min coin)` calls.
3. **Optimise.**
   * **Optimal substructure (cut-and-paste):** suppose an optimal solution for `a` uses coin `c` (it must use *some* coin). The remaining coins sum to `a−c` and must use `dp[a−c]` coins — otherwise replace them with a better set and get fewer than `dp[a]` coins total, a contradiction.
   * **State** `dp[a]` = min coins for amount `a`. **Recurrence** `dp[a] = 1 + min_{c ≤ a} dp[a−c]`. **Base** `dp[0]=0`. **Order** `a = 1..amount`. **Answer** `dp[amount]` (`≥ inf` ⇒ `−1`). **Infinity:** `amount + 1` — larger than any real answer (≤ `amount` coins of value 1) and `inf + 1` can't overflow, unlike `INT_MAX + 1`.
   * **Greedy fails:** `[1,3,4]`, amount 6: greedy `4+1+1 = 3` coins, optimum `3+3 = 2` coins. (DP also handles non-canonical coin systems.)
   * This is the **shortest path in a DAG with nodes `0..amount` and an edge `a−c → a` of weight 1 for each coin** — min-plus, same shape as Viterbi.
4. **Code.** see solution.
5. **Trace `[1,2,5], 11`:**

| a | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|----|----|
| dp[a] | 0 | 1 | 1 | 2 | 2 | 1 | 2 | 2 | 3 | 3 | 2 | **3** |

   e.g. `dp[11] = 1 + min(dp[10]=2, dp[9]=3, dp[6]=2) = 3` (`5+5+1`) ✓. To **reconstruct** which coins, store the best coin per amount (a "parent pointer") and walk back — exactly like Viterbi traceback. Edge `[2], 3`: `dp[1]=inf, dp[2]=1, dp[3]=min(dp[1]+1)=inf → −1` ✓. Edge `[1,3,4], 6` → 2 ✓. Edge `amount=0` → 0. Edge `coins={}` → −1.
6. **Final.** `amount × |coins|` time, O(amount) space.

## P4 · 300 Longest Increasing Subsequence

1. **Clarify.** **Strictly** increasing, not contiguous; return the length; empty → 0.
2. **Brute.** Enumerate all 2ⁿ subsequences.
3. **Optimise.**
   * **O(n²) DP.** `dp[i]` = length of the LIS **ending at index `i`**; `dp[i] = 1 + max{dp[j] : j < i, a[j] < a[i]}` (or 1). Answer `max dp[i]` (not `dp[n−1]` — the LIS needn't end last). Optimal substructure: the prefix of an LIS ending at `i` (before its last element `a[j]`) is an LIS ending at `j`.
   * **O(n log n) — `tails`.** Maintain `tails[k]` = the **smallest possible last element of an increasing subsequence of length `k+1`** among the elements processed so far.
     **Invariant:** `tails` is strictly increasing. *Proof:* if a subsequence of length `k+2` ends with `v`, deleting its last element gives one of length `k+1` ending with something `< v`, so `tails[k] < tails[k+1]`.
     **Update for `x`:** `p = lower_bound(tails, x)` (first index with `tails[p] ≥ x`; *strict* increase ⇒ equal values replace, not extend). If `p == len`, `x` is larger than every tail ⇒ extends the longest: push. Else `x ≤ tails[p]` and `tails[p−1] < x`, so there *is* an increasing subsequence of length `p+1` ending at `x` and it is at least as good (smaller tail) as the old one ⇒ set `tails[p] = x`.
     Answer = `tails.size()`. **`tails` is not itself an LIS** — it stores best *tails*, not a real subsequence (e.g. `[2,5,3]` → `tails=[2,3]`, but `3` came after `5`).
4. **Code.** `lower_bound` on a `vector<int>`.
5. **Trace `[10,9,2,5,3,7,101,18]`:**
```
x=10   [10]
x=9    p=0 -> [9]
x=2    p=0 -> [2]
x=5    p=1 (end) push -> [2,5]
x=3    p=1 (5>=3) -> [2,3]
x=7    push -> [2,3,7]
x=101  push -> [2,3,7,101]
x=18   p=3 (101>=18) -> [2,3,7,18]       size 4 ✓
```
   Edges: all equal `[7,7,7,7]` → `lower_bound` always 0 → size 1 (strictness). Decreasing → 1. Increasing → n. `[4,10,4,3,8,9]` → 3. `[]` → 0.
6. **Final.** O(n log n) time, O(n) space (O(n²) time for the textbook DP).

## P5 · 1143 Longest Common Subsequence

1. **Clarify.** Longest subsequence (order preserved, not contiguous) common to both strings; lengths ≤ 1000; empty → 0.
2. **Brute.** Enumerate all 2^m subsequences of `a` and check each against `b`: O(2^m · n).
3. **Optimise.** **State** `dp[i][j]` = LCS length of `a[0..i)` and `b[0..j)`. **Recurrence:** compare the last characters.
   * `a[i−1] == b[j−1]`: `dp[i][j] = dp[i−1][j−1] + 1`. *Why matching is never worse:* in any common subsequence we can match these two last characters to each other (if the last char of `a` is matched elsewhere, the exchange argument lets us re-match it to the last char of `b`) — so the optimum includes the pair.
   * else: at most one of them can be in the subsequence's last position ⇒ `dp[i][j] = max(dp[i−1][j], dp[i][j−1])`.
   **Base** `dp[0][*] = dp[*][0] = 0`. **Order** row by row. **Answer** `dp[m][n]`. **Space:** row `i` needs only row `i−1` (and the current row) ⇒ two rows of length `min(m,n)+1`.
   Same trellis idea as edit distance / sequence alignment: a path through an `m × n` grid DAG (diagonal = match).
4. **Code.** two rolling rows; the inner string is the shorter one.
5. **Trace `a = "abcde"`, `b = "ace"`** (rows = prefix of `a`, columns = prefix of `b`):

| | ∅ | a | c | e |
|---|---|---|---|---|
| ∅ | 0 | 0 | 0 | 0 |
| a | 0 | **1** | 1 | 1 |
| b | 0 | 1 | 1 | 1 |
| c | 0 | 1 | **2** | 2 |
| d | 0 | 1 | 2 | 2 |
| e | 0 | 1 | 2 | **3** |

   Bold cells are matches (`a=a`, `c=c`, `e=e`) taking the diagonal `+1`. Answer **3** (`"ace"`) ✓. Edges: either string empty → 0; identical strings of length L → L; `"aaaa"` vs `"aa"` → 2; symmetric (`LCS(a,b)=LCS(b,a)`, checked in tests).
6. **Final.** `m·n` states × O(1) = O(mn) time, O(min(m,n)) space.

---

## Viterbi worked example — DP on a trellis

Code: rate-½, constraint length K=3, generators `(7,5)₈`. State = `(u[t−1], u[t−2])` ∈ {00, 01, 10, 11}. Input `u`, state `(a,b)` ⇒ outputs `(u⊕a⊕b, u⊕b)`, next state `(u, a)`.

| from | u=0 → (out, next) | u=1 → (out, next) |
|------|------------------|------------------|
| 00 | 00 → 00 | 11 → 10 |
| 01 | 11 → 00 | 00 → 10 |
| 10 | 10 → 01 | 01 → 11 |
| 11 | 01 → 01 | 10 → 11 |

Message `1 0 1` + two tail zeros `0 0` (to return to state 00) encodes to `11 10 00 10 11`. Flip one channel bit (first bit of the 3rd pair) ⇒ received `11 10 10 10 11`. Path metrics `PM_t(s)` (Hamming branch metrics, `∞` = unreachable), `PM_0 = {00:0}`:

| t | received | PM(00) | PM(01) | PM(10) | PM(11) |
|---|----------|--------|--------|--------|--------|
| 1 | 11 | 2 | ∞ | 0 | ∞ |
| 2 | 10 | 3 | 0 | 3 | 2 |
| 3 | 10 | 1 | 3 | 1 | 2 |
| 4 | 10 | 2 | 1 | 2 | 2 |
| 5 | 11 | **1** | 3 | 2 | 3 |

Ending in the known flushed state 00 with metric 1 (one channel error), traceback of the survivors yields `1 0 1 0 0` ⇒ the data bits `1 0 1` are recovered **despite the error**. (Reproduce with `python code/viterbi_demo.py`.)

**Mapping onto the other problems**

| Viterbi (trellis) | Coin Change (322) | LCS (1143) |
|-------------------|-------------------|------------|
| state `s` at stage `t` | amount `a` | cell `(i,j)` |
| edge `p →ᵘ s` | `a−c → a` | step `(i−1,j−1)`, `(i−1,j)` or `(i,j−1)` |
| branch metric (Hamming distance) | `+1` per coin | `+1` on a match (maximise) |
| `PM_t(s) = min_p (PM_{t−1}(p)+BM)` | `dp[a] = 1 + min dp[a−c]` | `dp[i][j] = max(...)` |
| survivor pointer | best coin | which of the 3 moves |
| traceback | read coins back | read the alignment back |
| complexity | `T · 2^(K−1) · 2` vs `2^T` brute force | `amount × #coins` | `m × n` |

---

## Part C answers
1. **False.** Recursion without reuse is exponential; DP's content is *reusing* subproblem answers and identifying a small state space.
2. **False.** `[1,3,4], 6`.
3. **False.** 2-D problems often need only the previous row (LCS) or even a 1-D array.
4. **False.** `tails` holds best tails; its length is the answer, its contents need not be a real subsequence.
5. **False.** `INT_MAX + 1` overflows (UB); use `amount + 1`.
6. **False.** Longest simple path is the classic failure.
7. **False.** Viterbi *is* DP: path-metric recurrence over a trellis.

## Part D answers
* 70: `dp[i]` ways; `dp[i−1]+dp[i−2]`; base 1,2; increasing; `dp[n]`. 198: `best(i)`; take/skip; 0,a₁; increasing; `best(n)`. 322: `dp[a]`; `1+min`; `dp[0]=0`; increasing; `dp[amount]`. 300: `dp[i]` ending at `i` / `tails`; `max` / `lower_bound`; 1; left→right; `max dp` / `tails.size()`. 1143: `dp[i][j]`; match diagonal else max; zeros; row-major; `dp[m][n]`.
* States×work: n·1; n·1; amount·|coins|; n·n (or n·log n); m·n·1.
* See the 322 proof and the Viterbi paragraph in the worksheet (A5).
* Viterbi→Coin Change: state ↔ amount, stage ↔ (unused: amount order), branch metric ↔ +1, survivor ↔ best coin, traceback ↔ coin list.
