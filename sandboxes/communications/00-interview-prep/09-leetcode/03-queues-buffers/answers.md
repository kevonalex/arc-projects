# 03 · Queues & Buffers — Answers & Write-ups

Reference code: `code/solutions/queues_buffers_solutions.cpp`; tests `code/tests/test_queues_buffers.cpp` (12 tests, ASan+UBSan clean).

---

## Part A proofs (completing the worksheet)

**A3 — monotonic deque, full proof.** Let `D_i` be the deque after processing index `i`, `W_i = [i−k+1, i]`.

*Claim 1 (maintenance).* `D_i ⊆ W_i`, values strictly decreasing front→back, and every `j ∈ W_i \ D_i` has some `j' ∈ W_i`, `j' > j`, `nums[j'] ≥ nums[j]`.
*Proof by induction on `i`.* Base: `D` empty. Step: starting from `D_{i−1}`:
* (a) expiry removes at most the front if it equals `i−k` (indices in `D` are increasing so only the front can be oldest). Now `D ⊆ [i−k+1, i−1]`.
* (b) every popped back element `j` has `nums[j] ≤ nums[i]` and `j < i`: it is dominated by `i`, which is in `W_i` — property "dominated by a later ≥ element" holds for it. Elements dominated earlier by some `j'` that gets popped are dominated transitively by `i` (`nums[j'] ≤ nums[i]` ⇒ `nums[j] ≤ nums[j'] ≤ nums[i]`).
* (c) after pops, `nums[back] > nums[i]` (or `D` empty); pushing `i` keeps the order strictly decreasing and indices increasing. An index expired in (a) leaves `W_i`, so no domination claims are needed for it. ∎

*Claim 2 (front is the max).* Let `m ∈ W_i` be an index of a maximal value. If `m ∉ D`, it is dominated by a later `m'` with `nums[m'] ≥ nums[m]`, so `m'` is also maximal; repeat on `m'` — the chain moves strictly right inside `W_i` so it ends at some maximal index in `D`. `D` is strictly decreasing so its largest value is at the front. ∎

*Cost.* An index enters once and leaves at most once ⇒ total work ≤ 2n pushes/pops ⇒ O(n); extra space ≤ k.

*Why `≤`:* equal values: the older equal element can never be *strictly* needed (the newer one lasts longer and equals it). Using `<` is still correct but keeps duplicates, wasting space (and makes the "strictly decreasing" claim false).
*Why indices:* expiry needs to know *when* an element entered; a value alone cannot tell you if it left the window.

---

## P1 · 622 Design Circular Queue

1. **Clarify.** As in the worksheet. Capacity `k ≥ 1`. Single-threaded (SPSC lock-free would use atomics with release/acquire on `head`/`tail`).
2. **Brute.** Vector with `erase(begin)` on dequeue: O(n) per op.
3. **Optimise.** A1. Keep `size_` so `head_ == tail` ambiguity never arises: `isEmpty ⟺ size==0`, `isFull ⟺ size==cap`. Invariant R is preserved by `enQueue`/`deQueue` as shown in A1.
4. **Code.** `enQueue`: `buf[(head+size)%cap] = v; ++size`. `Rear`: `buf[(head+size-1)%cap]` (guard empty first — `size-1` with `size=0` would underflow).
5. **Trace `k=3`:**
```
op          buf        head size  result
enQ 1       [1 _ _]      0    1    true
enQ 2       [1 2 _]      0    2    true
enQ 3       [1 2 3]      0    3    true
enQ 4       [1 2 3]      0    3    false (full)
Rear                                3
deQ         [. 2 3]      1    2    true
enQ 4  slot (1+2)%3=0 -> [4 2 3]    1    3    true  (wrapped)
Rear   (1+3-1)%3 = 0 ->                4
Front  buf[1]                          2
```
   Edge `k=1`: enQ(7) ok, enQ(8) false, deQ → empty again; `Front/Rear = -1`.
6. **Final.** O(1) time per op, O(k) space.

## P2 · 641 Design Circular Deque

1. **Clarify.** As P1 plus front/back ops at both ends.
2. **Brute.** `vector::insert(begin())` O(n).
3. **Optimise.** Same R. `insertLast`: write `(head+size)%cap`; `deleteFront`: `head++` mod cap; **`insertFront`: `head = (head + cap − 1) % cap`, write there, `size++`**; `deleteLast`: only `size--` (head unchanged). `getRear = buf[(head+size−1)%cap]`.
4. **Code.** see solution.
5. **Trace `k=4`:** fresh, `head=0,size=0`. `insertFront(1)`: `head=(0+4−1)%4=3`, `buf[3]=1`, `size=1`. `insertFront(2)`: `head=2`, `buf[2]=2`, `size=2`; `getFront=buf[2]=2`; `getRear=buf[(2+2−1)%4=3]=1` ✓. `deleteFront` → `head=3,size=1`; `deleteLast` → `size=0` empty ✓. (Using `head−1` with `size_t` would give `SIZE_MAX % 4 = 3` by luck for cap=4 but fail for non-power-of-two caps.)
6. **Final.** O(1) per op, O(k) space.

## P3 · 232 Implement Queue using Stacks

1. **Clarify.** `pop/peek` on non-empty; `empty` must consider both stacks.
2. **Brute.** Transfer everything to a temp stack and back per op: O(n) per op.
3. **Optimise.** A2: `in` for pushes, `out` for serving. **Invariant:** `out` top-to-bottom is the oldest-first prefix of the queue; `in` top-to-bottom is the newest-first suffix. Pour only when `out` empty: pouring onto a non-empty `out` would put newer elements *above* older ones. Amortised O(1) by the "each element moves once" argument.
4. **Code.** `refill()` then `out.back()`.
5. **Trace `push1 push2 pop push3 pop pop`:**
```
push1: in=[1]        out=[]
push2: in=[1,2]      out=[]
pop  : out empty -> pour: out=[2,1] in=[] ; pop -> 1 ; out=[2]
push3: in=[3]        out=[2]
pop  : out non-empty (no pour) -> 2 ; out=[]
pop  : out empty -> pour: out=[3]; pop -> 3
```
   Order 1,2,3 ✓. A wrong "always pour" version would pour `[3]` onto `[2]` and return `3` before `2`.
6. **Final.** Amortised O(1) per op; worst single op O(n) (a pour); O(n) space.

## P4 · 239 Sliding Window Maximum

1. **Clarify.** `1 ≤ k ≤ n`; output `n−k+1` values.
2. **Brute.** `max` of each window: O(nk) time, O(1) space (or O(n log k) with a multiset/heap).
3. **Optimise.** Monotonic deque (A3 + proof above) — O(n) time, O(k) space.
4. **Code.** For each `i`: expire front (`dq.front() <= i−k`), pop dominated back (`nums[back] <= nums[i]`), push `i`, emit `nums[front]` once `i ≥ k−1`.
5. **Trace `[1,3,−1,−3,5,3,6,7]`, k=3** (deque shown as indices → values):

| i | x | expire? | pops | deque after | emit |
|---|---|---------|------|-------------|------|
| 0 | 1 | – | – | [0]→[1] | – |
| 1 | 3 | – | pop 0 (1≤3) | [1]→[3] | – |
| 2 | −1 | – | none (3>−1) | [1,2]→[3,−1] | 3 |
| 3 | −3 | – (front 1 > 0) | none | [1,2,3]→[3,−1,−3] | 3 |
| 4 | 5 | front 1 ≤ 4−3=1 → pop | pop 3,2 (≤5) | [4]→[5] | 5 |
| 5 | 3 | – | none | [4,5]→[5,3] | 5 |
| 6 | 6 | – | pop 5,4 | [6]→[6] | 6 |
| 7 | 7 | – | pop 6 | [7]→[7] | 7 |

   Output `3,3,5,5,6,7` ✓. Edge decreasing `9 8 7 6, k=2`: nothing is ever dominated, the deque is always `[i−1, i]`; **expiry** supplies correctness (`9,8,7`). Edge increasing `1 2 3 4, k=2`: every new element dominates the previous; deque size 1; **domination** supplies correctness (`2,3,4`). Edge `k=n`: single output.
6. **Final.** O(n) time, O(k) space.

## P5 · 346 Moving Average from Data Stream

1. **Clarify.** Average of the last `min(count, size)` values; `int` samples up to 10⁵ in LeetCode, but protect with `long long`.
2. **Brute.** Keep all, sum last `size`: O(size) per call.
3. **Optimise.** Ring of `size` ints + `sum`. `next(v)`: `sum -= ring[head]` (0 while filling), write `v`, `sum += v`, `head = (head+1)%size`, `count = min(count+1, size)`. Invariant: `sum = Σ` of the live window.
4. **Code.** see solution.
5. **Trace `size=3`:** `1` → count 1, sum 1 → **1.0**; `10` → sum 11, count 2 → **5.5**; `3` → sum 14, count 3 → **4.667**; `5` → evict the oldest (`1`, slot 0): `sum = 14 − 1 + 5 = 18` → 18/3 = **6.0** ✓. Edge `size=1`: output equals the input each time. Edge two `INT_MAX`: sum 4.29e9 needs 64 bits ✓.
6. **Final.** O(1) time per call, O(size) space.

---

## Part C answers
1. **False.** Ambiguous: also full. Use `size`, a spare slot, or free-running counters.
2. **False.** `0 − 1` wraps to `SIZE_MAX`; `% cap` then gives a wrong slot unless `cap` divides 2⁶⁴. Use `(head + cap − 1) % cap`.
3. **False.** Order breaks (see trace).
4. **False.** Expiry requires positions.
5. **False.** Amortised: each index is popped at most once overall.
6. **False.** Rounding error accumulates; keep an exact integer sum.
7. **False.** Wrap is just index arithmetic; no data movement.

## Part D answers
* `deQueue`: `head' = (head+1) mod cap` keeps `0 ≤ head' < cap`; the old first element `buf[head]` leaves the live set; live set becomes `buf[(head'+i) mod cap], i=0..size−2` = old elements 1..size−1 ✓; `size' = size−1 ≥ 0` because precondition `size>0`.
* Free-running: `size = tail − head`; `isFull = (tail − head == cap)`; `index(i) = (head + i) & (cap − 1)`.
* Input `[2,2,2], k=2`: `≤` keeps deque size 1, `<` keeps both — answers identical `2,2`.
* `k=1`: deque = `[i]` at every step (each element expires the previous one and dominates it).
