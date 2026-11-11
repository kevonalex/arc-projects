# 08 · Heaps — Answers & Write-ups

Reference code: `code/solutions/heaps_solutions.cpp`; tests `code/tests/test_heaps.cpp` (8 tests, ASan+UBSan clean).

---

## P1 · 215 Kth Largest Element in an Array

1. **Clarify.** The k-th largest in *sorted order* (duplicates count: in `[5,5,5]` the 3rd largest is 5). `1 ≤ k ≤ n`.
2. **Brute.** Sort descending, return `a[k−1]`: O(n log n) time, O(1)/O(n) space.
3. **Optimise.** Min-heap of size ≤ k (A2).
   *Proof of invariant.* Base: empty. Step: if heap size `< k`, push — it still holds all items. Otherwise the heap has `k` items and we consider the `k+1` candidates `heap ∪ {x}`. If `x ≤ root`, then `x` is (one of) the smallest of the `k+1`, so it cannot be among the `k` largest — drop it. If `x > root`, then `root` is the unique smallest of the `k+1`, so it cannot be among the `k` largest — evict it and insert `x`. The invariant holds, so at the end the root is the smallest of the `k` largest = the k-th largest.
   Alternatives: `std::nth_element` (introselect, expected O(n), mutates), quickselect with random pivot (expected O(n), worst O(n²)), median-of-medians (worst O(n), large constant). Heap is the streaming-friendly answer.
4. **Code.** see solution.
5. **Trace `[3,2,1,5,6,4], k=2`:**
```
x=3  heap {3}
x=2  heap {2,3}          root 2
x=1  1 <= 2  skip        heap {2,3}
x=5  5 > 2   pop 2 push 5 -> {3,5}   root 3
x=6  6 > 3   pop 3 push 6 -> {5,6}   root 5
x=4  4 <= 5  skip        heap {5,6}
answer root = 5 ✓ (second largest)
```
   Edge `k=1`: heap holds the running max → answer max. Edge `k=n`: nothing is ever dropped → root is the minimum. Edge `[5,5,5], k=3` → 5. Edge `INT_MIN`/`INT_MAX` need no arithmetic, so no overflow.
6. **Final.** O(n log k) time, O(k) space.

## P2 · 23 Merge k Sorted Lists

1. **Clarify.** `k ≥ 0` lists (possibly null entries); output one sorted list reusing nodes; empty input → `nullptr`.
2. **Brute.** Collect all `N` values/nodes, sort: O(N log N). Or fold lists one by one: the i-th merge costs O(size so far) ⇒ O(kN) worst case.
3. **Optimise.** Min-heap of list heads (A3). Comparator by `val` (`greater`-style: pop smallest). *Invariant:* heap = current heads of non-exhausted lists. Correctness: every list is sorted, so the smallest remaining node overall is some list's head; popping the heap minimum therefore outputs the next element of the merged order. Cost `N · O(log k)`.
4. **Code.** `priority_queue<ListNode*, vector<ListNode*>, decltype(cmp)>`; at the end set `tail->next = nullptr` (the last popped node's `next` is already null, because a non-null successor would have been pushed and popped later — the explicit assignment is purely defensive).
5. **Trace `[1,4,5], [1,3,4], [2,6]`:**
```
heap heads {1a,1b,2}            pop 1a -> out 1 ; push 4a   heap {1b,2,4a}
pop 1b -> out 1,1 ; push 3b     heap {2,3b,4a}
pop 2  -> out 1,1,2 ; push 6    heap {3b,4a,6}
pop 3b -> …,3 ; push 4b         heap {4a,4b,6}
pop 4a -> …,4 ; push 5          heap {4b,5,6}
pop 4b -> …,4 ; (no successor)  heap {5,6}
pop 5, pop 6                     -> 1,1,2,3,4,4,5,6 ✓
```
   Edge `{}` → empty heap → `dummy.next = nullptr`. Edge `{nullptr}` → null entries skipped. Edge unequal lengths `[1..6]` & `[0]`: `0` first, then the long list drains while the heap has size 1.
6. **Final.** O(N log k) time, O(k) extra space (nodes relinked).

## P3 · 295 Find Median from Data Stream

1. **Clarify.** After each `addNum`, `findMedian` returns the middle element (odd count) or the mean of the two middle ones (even count) as `double`. Both operations frequent ⇒ want O(log n) / O(1).
2. **Brute.** Vector + `nth_element` per query O(n); or maintain a sorted vector with `lower_bound` insert: O(n) insert (memmove) / O(1) query.
3. **Optimise.** Two heaps (A4). *Insertion proof:* before insertion the invariants (1) `max(lo) ≤ min(hi)` and (2) sizes hold. Put `x` in `lo` if `x ≤ lo.top()` else `hi`. (1) is preserved: if `x ≤ lo.top()`, `x ≤ lo.top() ≤ min(hi)`, and the new max of `lo` is still ≤ `min(hi)`; if `x > lo.top()` it goes to `hi`, where it doesn't lower `min(hi)` below `max(lo)`. Sizes may now be off by one: if `|lo| > |hi|+1` move `lo.top()` to `hi`; if `|hi| > |lo|` move `hi.top()` to `lo`. A moved element is the boundary element of its heap, so (1) is preserved; (2) is restored since sizes differed by at most 2 before the move.
4. **Code.** `findMedian`: `lo.size() > hi.size() ? lo.top() : (lo.top()+hi.top())/2.0` with `double` arithmetic.
5. **Trace `add 1, 2, 3`:**
```
add 1: lo empty -> lo{1}                           sizes (1,0)  median 1
add 2: 2 > lo.top 1 -> hi{2}                       sizes (1,1)  median (1+2)/2 = 1.5
add 3: 3 > 1 -> hi{2,3}; |hi|=2 > |lo|=1 -> move 2 to lo -> lo{1,2} hi{3}   median lo.top = 2 ✓
```
   Edge single element: `lo.top()` as double. Edge `-1,-2,INT_MAX,INT_MAX`: final even median `(−1 + INT_MAX)/2.0` computed in `double` (in `int` it would be fine here but `INT_MAX + INT_MAX` is not).
6. **Final.** `addNum` O(log n); `findMedian` O(1); space O(n).

---

## Part C answers
1. **False.** Only parent/child order; siblings and levels are unordered.
2. **False.** It is a max-heap by default; use `greater<>` for min.
3. **False.** A *min*-heap of size `k` — you evict the smallest survivor.
4. **False.** O(n log k).
5. **False.** Accumulator folding is O(kN) (the accumulator is re-walked every time).
6. **False.** `lo` is a max-heap, `hi` a min-heap (so each exposes the boundary element).
7. **False.** Even count ⇒ average of the two middles.

## Part D answers
* A complete tree with `n` nodes has `⌊log₂ n⌋ + 1` levels because level `d` holds `2^d` nodes.
* `lo.top()` is `max(lo)`; every element of `hi` is `≥ max(lo)`; after moving, `hi`'s new minimum is `max(lo)` which is `≤` the old `min(hi)` — still ≥ all remaining `lo` elements.
* Max-heap of size `n−k+1` holding the smallest `n−k+1` items, root = k-th largest (its largest).
* Store `(val, list_index)` in the heap and break ties by index.
