# 08 · Heaps — Pattern Worksheet

**Pattern:** when you repeatedly need "the smallest/largest *so far*" while elements keep arriving or leaving, keep them in a **binary heap** (O(log n) insert/extract, O(1) peek) — often **bounded** (size `k`) or in a **pair** (two heaps around a median).
**Why it matters at a radio/protocol shop:** schedulers (earliest deadline first, timer wheels as a heap), top-k strongest cells, merging k timestamp-ordered logs, running median of jitter/RTT.
**Coded:** 215, 23, 295 — `code/headers/heaps.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (k bounds, duplicates, ties, stream vs batch, can I reorder/mutate?) → 2 **Brute force + complexity** → 3 **Optimise** (which heap(s), what size, what invariant) → 4 **Code** (mind min- vs max-heap in `std::priority_queue`) → 5 **Hand-trace** the heap contents for a normal + edge input → 6 **Final complexity**.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the techniques work

### A1. The binary heap
A complete binary tree stored in an array (`children of i: 2i+1, 2i+2`), with the **heap property**: every parent ≤ its children (min-heap). Consequences:
* the minimum is the root: **peek O(1)**.
* *Push:* append at the end (keeps the tree complete), **sift up** while smaller than the parent: ≤ height = `⌊log₂ n⌋` swaps.
* *Pop:* move the last element to the root, **sift down** swapping with the smaller child: ≤ height swaps.
* The heap property is **local** (parent vs child) yet implies the global fact "root is minimal" by induction along the path to the root. It does *not* order siblings — a heap is not a sorted array.
* `std::priority_queue<T>` is a **max**-heap (its comparator says "a has lower priority than b"); for a min-heap use `std::greater<T>`.
Building a heap from `n` items is O(n) (`make_heap`), sorting with it O(n log n).

### A2. Bounded heap for "k-th largest" (215)
Keep a **min-heap of size ≤ k** containing the `k` largest items seen. **Invariant:** *after processing a prefix, the heap equals the multiset of the k largest items of that prefix (or all items if fewer than k), and the root is the smallest of them.* A new `x` is relevant iff `x >` root: then root is no longer among the k largest (there are now `k` items ≥ … greater than it), so replace it. Hence after all `n` items the root is the **k-th largest**. Cost O(n log k), space O(k). (Quickselect: expected O(n), worst O(n²), mutates input; `nth_element` in C++.)
Why a *min*-heap for the *largest*? Because the item you need to evict is the smallest of the survivors.

### A3. k-way merge (23)
All `k` lists are sorted, so the global minimum of the remaining data is the minimum of the `k` list *heads*. Keep those ≤ k heads in a min-heap: pop the min (it **is** the next output element), push its successor. **Invariant:** the heap contains exactly the current head of every non-exhausted list. Each of the `N` outputs costs one pop + ≤ one push on a heap of size ≤ k ⇒ **O(N log k)**, O(k) extra space. (Merging pairwise in a divide-and-conquer tree also gives O(N log k).) Information-theoretically, merging `k` sorted runs needs `Ω(N log k)` comparisons.

### A4. Two heaps around the median (295)
Split the data into a lower half `lo` (**max**-heap) and an upper half `hi` (**min**-heap):
> **Invariants:** (1) every element of `lo` ≤ every element of `hi`; (2) `|lo| = |hi|` or `|lo| = |hi| + 1`.

Then the median is `lo.top()` (odd count) or `(lo.top() + hi.top()) / 2` (even) — both O(1). *Insertion:* put `x` in `lo` if `x ≤ lo.top()` else in `hi` (keeps (1)); then **rebalance by moving at most one element** (`lo.top → hi` if `lo` too big, `hi.top → lo` if `hi` bigger) — moving the max of `lo` into `hi` or the min of `hi` into `lo` preserves (1) because those are the boundary elements. O(log n) per insert. Compare: sorted vector insert O(n); order-statistic tree O(log n) but heavier.
Compute the even-count average in `double` — `int + int` overflows.

---

## Part B — Problems

### P1 · LC 215 Kth Largest Element in an Array — `find_kth_largest`
**Ctx:** the k-th strongest RSSI among `N` neighbour cells defines the handover threshold.
1. Clarify: `1 ≤ k ≤ n`; duplicates count separately (`[3,2,3,1,2,4,5,5,6], k=4 → 4`); input may be mutated? stream?
2. Brute: sort descending, take index `k−1` → ____
3. Optimise: A2. Write the invariant; say why `min`-heap; compare with `nth_element`/quickselect.
5. Trace: `[3,2,1,5,6,4], k=2` — heap contents after each element. Edge: `k = 1`, `k = n`, all equal.
6. Final: ____

### P2 · LC 23 Merge k Sorted Lists — `merge_k_lists`
**Ctx:** merge `k` per-antenna timestamp-ordered event logs into one global timeline.
1. Clarify: some lists null; `k = 0`; equal keys; relink existing nodes (no allocation).
2. Brute: dump all nodes in a vector, sort, relink → ____ ; or merge lists one by one into an accumulator → ____
3. Optimise: A3. State the heap invariant and the cost of each output.
5. Trace: `[1,4,5], [1,3,4], [2,6]`. Edge: `{}`, `{nullptr}`, very unequal lengths.
6. Final: ____

### P3 · LC 295 Find Median from Data Stream — `MedianFinder`
**Ctx:** running median of RTT/jitter samples — robust against outlier spikes unlike the mean.
1. Clarify: `addNum` O(log n), `findMedian` O(1); even count → average of the two middle values; negatives; large values.
2. Brute: keep a vector, sort on each query → ____ ; keep a sorted vector with insertion → ____
3. Optimise: A4. Prove both invariants hold after an insert + rebalance.
5. Trace: add `1, 2, 3` — show `lo` and `hi` after each add. Edge: single element, two `INT_MAX`.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "A heap is a sorted array."
2. "`std::priority_queue<int>` is a min-heap."
3. "To find the k-th *largest* I need a max-heap of all elements."
4. "Heap size `k` makes the 215 solution O(n)."
5. "Merging `k` lists by repeatedly merging two lists into an accumulator is just as fast as a heap."
6. "Both median heaps must be the same type."
7. "The median of an even number of elements is `lo.top()`."

## Part D — Self-check
* Why is the height of an `n`-node heap `⌊log₂ n⌋`?
* Show why moving `lo.top()` to `hi` keeps invariant (1).
* What changes in 215 if `k` is close to `n`? (Use a max-heap of size `n−k+1` or `nth_element`.)
* How would you give *stable* output for equal timestamps in 23? (Tie-break on list index.)
