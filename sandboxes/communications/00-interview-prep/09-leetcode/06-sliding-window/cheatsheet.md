# 06 · Sliding Window — Cheatsheet

**Core idea:** two pointers `l ≤ r`, O(1) add/remove summary; validity monotone ⇒ both pointers only move right ⇒ O(n).

## Templates
```cpp
// longest window with an "at most" constraint
for r: add(s[r]); while (!valid) { remove(s[l]); ++l; } best = max(best, r-l+1);

// shortest window with an "at least" constraint
for r: add(s[r]); while (valid)  { best = min(best, r-l+1); remove(s[l]); ++l; }
```

## Recipes
| LC | Summary | Key move | Time / Space |
|----|---------|----------|--------------|
| 3 | `next_start[c]` | `l = max(l, next_start[c])` | O(n) / O(Σ) |
| 424 | `cnt[26]`, `maxFreq` | valid iff `len - maxFreq <= k`; slide, don't shrink; `maxFreq` never decreases | O(n) / O(26) |
| 76 | `need[c]`, `missing` | `if (need[c]-- > 0) --missing`; remove: `if (++need[c] > 0) ++missing` | O(n+m) / O(Σ) |
| 11 | none (two pointers) | move the **shorter** wall | O(n) / O(1) |

## When it applies
* contiguous subarray/substring; predicate monotone in the window.
* Doesn't apply: negative numbers with a target sum (use prefix sums + hash), non-contiguous (subsequence → DP).
* Need running max/min in the window → monotonic deque (03).

## Gotchas
* `max(l, last+1)`, never plain assignment (3).
* 424 uses `if`, not `while`, and never lowers `maxFreq`.
* 76: `need` can be negative; count `missing` by multiplicity; `t` longer than `s` ⇒ `""`.
* Use `unsigned char` to index 256-entry tables.
* Record the answer *before* shrinking in "shortest" problems.
* Empty input / `k=0` / `k≥n` edges.

## Script
"Clarify contiguous & validity → brute force is O(n²) substrings → predicate is monotone so `l` never moves back → summary supports add/remove in O(1) → code → trace `(l,r,summary)` → O(n) time, O(alphabet) space."
