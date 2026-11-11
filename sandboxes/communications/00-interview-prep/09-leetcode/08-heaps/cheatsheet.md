# 08 · Heaps — Cheatsheet

**Core idea:** need "best so far" with arrivals/removals ⇒ heap. Bounded heap = top-k. Two heaps = median. Heap of list-heads = k-way merge.

## `std::priority_queue`
```cpp
priority_queue<int> maxh;                                   // max-heap (default)
priority_queue<int, vector<int>, greater<int>> minh;        // min-heap
auto cmp = [](const Node* a, const Node* b){ return a->val > b->val; };   // "a has LOWER priority" -> smallest on top
priority_queue<Node*, vector<Node*>, decltype(cmp)> pq(cmp);
```
push/pop O(log n), top O(1), `make_heap` O(n). No decrease-key (use lazy deletion: push duplicates, skip stale).

## Recipes
| LC | Heap(s) | Invariant | Time / Space |
|----|---------|-----------|--------------|
| 215 | min-heap size k | heap = k largest of prefix; root = k-th largest | O(n log k) / O(k) |
| 23 | min-heap of heads | one head per non-exhausted list | O(N log k) / O(k) |
| 295 | `lo` max-heap, `hi` min-heap | `max(lo) ≤ min(hi)`; `|lo|-|hi| ∈ {0,1}` | add O(log n), median O(1) / O(n) |

## Alternatives
* k-th element: `nth_element` / quickselect expected O(n).
* k-way merge: divide-and-conquer pairwise merges, O(N log k).
* Streaming top-k with deadlines: heap of timers (EDF scheduler).
* Median of a *bounded* window: two multisets / order-statistic tree.

## Gotchas
* Default PQ is a max-heap.
* Custom comparator semantics: `cmp(a,b) == true` means `a` is *below* `b`.
* Even-count median average in `double`.
* `k > n` / `k == 0` guards (not in LeetCode, but in real code).
* Heap of pointers: don't mutate keys while inside the heap.
* Stable merge: add a tiebreak field.
* Remember to push the successor (`n->next`) after popping in 23.

## Script
"Clarify k/duplicates/stream → brute force sorts O(n log n) → I only need the extreme of a shrinking/growing set ⇒ heap of size ___ with invariant ___ → code → trace heap contents → O(n log k) / O(k)."
