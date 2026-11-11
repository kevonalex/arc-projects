# 09 · Binary Search — Cheatsheet

**Core idea:** monotone predicate `F…F T…T`; keep the boundary inside `[lo,hi]`; halve. O(log n).

## Two templates (never mix)
```cpp
// exact match, closed interval
int lo=0, hi=n-1;
while (lo <= hi) { int mid = lo + (hi-lo)/2;
  if (a[mid]==t) return mid;
  if (a[mid] < t) lo = mid+1; else hi = mid-1; }
return -1;

// first index where P is true, in [0, n]
int lo=0, hi=n;                       // hi = n means "never true"
while (lo < hi) { int mid = lo + (hi-lo)/2;
  if (P(mid)) hi = mid; else lo = mid+1; }
return lo;
```
STL: `lower_bound` (first ≥), `upper_bound` (first >), `binary_search`, `equal_range`.

## Rotated array (distinct)
* One half is always sorted: `a[lo] <= a[mid]` ⇒ left sorted.
* 33: if target in the sorted half's range go there else other half.
* 153: compare `a[mid]` with `a[hi]`: `>` ⇒ `lo=mid+1`; else `hi=mid`.

| LC | Predicate / fact | Time / Space |
|----|------------------|--------------|
| 704 | `a[mid] < t` | O(log n) / O(1) |
| 33 | sorted half + range test | O(log n) / O(1) |
| 153 | `a[i] <= a[n-1]` | O(log n) / O(1) |

## Binary search on the answer
Feasibility `f(x)` monotone in `x` ⇒ search smallest feasible `x` in `[lo,hi]`; cost `O(log(range) · cost(f))`.

## Gotchas
* `mid = lo + (hi-lo)/2` (no overflow).
* `lo<=hi` ⇒ `hi=mid-1`; `lo<hi` ⇒ `hi=mid`.
* `<=` when testing the sorted left half (`lo==mid`).
* Empty array: `hi = -1` / `hi = 0` — check loop entry.
* Duplicates break 33/153's O(log n) worst case.
* Floating-point "binary search": iterate a fixed count, not `lo<hi`.
* Prove with the invariant, don't rely on testing small cases.

## Script
"Clarify sorted/rotated/distinct → brute force O(n) → predicate is monotone because ___ → invariant: boundary ∈ [lo,hi] → code with safe mid → trace (lo,hi,mid) → O(log n)/O(1)."
