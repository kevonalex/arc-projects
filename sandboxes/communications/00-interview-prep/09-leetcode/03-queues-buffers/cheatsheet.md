# 03 · Queues & Buffers — Cheatsheet

## Ring buffer
```
live = buf[(head+i) % cap], i in [0,size)
enq: buf[(head+size)%cap]=v; ++size          (size<cap)
deq: head=(head+1)%cap; --size               (size>0)
front: buf[head]    rear: buf[(head+size-1)%cap]   (size>0!)
push_front: head=(head+cap-1)%cap; buf[head]=v; ++size
pop_back: --size
```
Full vs empty: keep `size` | waste a slot | free-running unsigned counters + `& (cap-1)` (cap = 2^m).

## Two stacks → queue
push → `in`; pop/peek → `out`; **pour `in→out` only if `out` empty**. Amortised O(1) (each element moves once).

## Monotonic deque (max of last k)
```
for i: if (dq.front() <= i-k) pop_front;
       while (!dq.empty() && a[dq.back()] <= a[i]) pop_back;
       push_back(i);
       if (i >= k-1) out.push_back(a[dq.front()]);
```
Invariant: indices increasing, values strictly decreasing, front = window max. Dominated (older & ≤) elements can never matter again. O(n)/O(k). Min version: flip comparison.

## Running window aggregates
`sum += new - evicted` with `long long`; `avg = double(sum)/count`.

## Recipes
| LC | Structure | Time / Space |
|----|-----------|--------------|
| 622 | ring + size | O(1) / O(k) |
| 641 | ring, front insert steps `head` back | O(1) / O(k) |
| 232 | two stacks | O(1) amortised / O(n) |
| 239 | monotonic deque | O(n) / O(k) |
| 346 | ring + running sum | O(1) / O(size) |

## Gotchas
* `Rear` on empty: guard before computing `size-1`.
* Unsigned `head-1` underflow; negative `%`.
* `empty()` in 232 must check **both** stacks.
* 239: expire *before* or after pop-back both work, but you must expire by **index**.
* Emit only when `i >= k-1`.
* In real ring buffers shared across threads: SPSC = one atomic per index; MPMC needs more.

## Script
"Clarify capacity/full/empty semantics → brute force shifts/rescans O(n) or O(nk) → keep an invariant over indices → code → trace a wrap-around → O(1) per op / O(n) total."
