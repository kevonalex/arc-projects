# 12 · Intervals — Cheatsheet

**Core idea:** sort, then sweep with a tiny summary. Ask: closed vs half-open? touching = overlap?

## Overlap
Closed `[a,b]`,`[c,d]`: overlap ⇔ `a ≤ d && c ≤ b`. After sorting by start: overlap ⇔ `c ≤ b`. Half-open `[a,b)`: use `<` instead of `≤`.

## Templates
```cpp
// 56 merge
sort(v.begin(), v.end());
for (auto& iv : v) {
  if (!out.empty() && iv[0] <= out.back()[1]) out.back()[1] = max(out.back()[1], iv[1]);
  else out.push_back(iv); }

// 57 insert into sorted disjoint
while (i<n && v[i][1] <  nv[0]) out.push_back(v[i++]);
while (i<n && v[i][0] <= nv[1]) { nv[0]=min(nv[0],v[i][0]); nv[1]=max(nv[1],v[i][1]); ++i; }
out.push_back(nv); while (i<n) out.push_back(v[i++]);

// 435 min removals (earliest finish)
sort by end; keep=1; last=v[0][1];
for i>=1: if (v[i][0] >= last) { ++keep; last = v[i][1]; }   return n - keep;
```

| LC | Sort key | Time / Space |
|----|----------|--------------|
| 56 | start | O(n log n) / O(n) |
| 57 | none (already sorted) | O(n) / O(n) |
| 435 | **end** | O(n log n) / O(1) |

## Proof reminders
* 56: invariant "out sorted & disjoint ⇒ only `out.back()` can overlap the next interval".
* 435: exchange argument — swapping any optimal solution's first interval for the earliest-ending one keeps it feasible.
* Counter-examples: sort-by-start fails `[0,100],[1,2],[3,4]`; shortest-first fails `[0,3],[2,4],[3,6]`.

## Related
* Meeting rooms II (max overlap): events sweep / min-heap of ends.
* Interval intersection: two pointers on two sorted lists.
* Total covered length: merge then sum.
* Byte-range reassembly = merge of received ranges.

## Gotchas
* `max` on merge (nested).
* Touching: `<=` for 56/57, `>=` (allowed) for 435.
* Don't mutate while iterating a copy you still need.
* Sort comparator must be a strict weak order.
* `INT` extremes: avoid `end - start` arithmetic or use 64-bit.
* Empty input.

## Script
"Clarify closed/touching/sorted → brute force pairwise O(n²) → sort by ___ so that only the last/closest matters → code the sweep → trace nested + touching + empty → O(n log n)."
