# 03 · Queues & Buffers — Pattern Worksheet

**Pattern:** a fixed-size array + index arithmetic that gives **O(1)** push/pop at the ends; plus the **monotonic deque**, which keeps just enough history to answer "max of the last k" in O(1) amortised.
**Why it matters at a radio/protocol shop:** DMA/ISR → decoder hand-off rings, HARQ and retransmission windows, RSSI/SNR smoothing, jitter buffers. Memory is bounded; allocation on the hot path is forbidden; off-by-one errors become dropped packets.
**Coded:** 622, 641, 232, 239, 346 — `code/headers/queues_buffers.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (capacity, full/empty behaviour, return codes, single- vs multi-threaded) → 2 **Brute force + complexity** → 3 **Optimise** (state the invariant) → 4 **Code** → 5 **Hand-trace** a normal run *and* a wrap-around/edge run, drawing the array and `head/size` → 6 **Final complexity**.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the techniques work

### A1. Ring buffer invariant
Storage `buf[0..cap)`, `head` = index of oldest element, `size` = number stored.
**Invariant R:** the live elements are exactly `buf[(head + i) mod cap]` for `i = 0 … size−1`; `0 ≤ size ≤ cap`; `0 ≤ head < cap`.
* enqueue: write at `(head + size) mod cap`, `size++` (needs `size < cap`)
* dequeue: `head = (head + 1) mod cap`, `size--` (needs `size > 0`)
* each operation preserves R and touches O(1) cells ⇒ **O(1) time, O(cap) space**, **no allocation after construction**.

**Full vs empty.** If you store only `head` and `tail`, `head == tail` is ambiguous (both empty *and* full). Fixes: (a) keep `size` (used here), (b) sacrifice one slot, (c) let `head`/`tail` be *free-running* unsigned counters, `size = tail − head` (works because unsigned subtraction is modular), index with `& (cap−1)` when `cap` is a power of two. (c) is what lock-free SPSC rings use.

**Stepping backwards** (`insertFront`): `(head − 1) mod cap` is wrong for `head = 0` with unsigned types (wraps to a huge number) and with signed `%` (negative result). Use `(head + cap − 1) % cap`.

### A2. Two stacks make a queue (amortised O(1))
`in` receives pushes; `out` serves pops/peeks. Only when `out` is **empty** do we pour all of `in` into `out` (reversing it → oldest on top). **Potential argument:** let `Φ = |in|`. push costs 1 + ΔΦ(1)=2, a pour of `m` elements costs `m` and drops `Φ` by `m` ⇒ amortised cost 0, pop costs 1 ⇒ every op is amortised O(1). Equivalently, every element is moved `in → out` **at most once**. (Pouring while `out` is non-empty would break FIFO order.)

### A3. The monotonic deque (the key idea of 239)
Window after processing index `i` is `W = [i−k+1, i]`. Keep a deque `D` of **indices** `j₁ < j₂ < … < j_m`, all in `W`, such that
> **Invariant M:** `nums[j₁] > nums[j₂] > … > nums[j_m]` (strictly decreasing values) **and** every index in `W` that is *not* in `D` has a *later* index in `W` with a value `≥` its own.

Consequences:
1. **The front is the window maximum.** By M, every index outside `D` is dominated by a later index with an `≥` value, so the maximum value of `W` is attained by some index that *is* in `D`. `D` is strictly decreasing, so its largest value sits at the front: `nums[j₁]`.
2. **Maintaining M on a new `i`:** (a) *expire* — if `j₁ ≤ i−k` pop front; (b) *dominate* — while `nums[back] ≤ nums[i]` pop back; those elements are older **and** not larger, so they can never be a maximum again (`i` outlives them and is ≥); (c) push `i`. After (b) the back is `> nums[i]`, so the order stays strictly decreasing.
3. **Cost:** every index is pushed once and popped at most once ⇒ O(n) total, O(k) space.

### A4. Running aggregates
For a sliding sum keep `sum = Σ window` and update by `+new − evicted` — O(1) per sample instead of O(k). Use a 64-bit sum (32-bit overflows) and compute the mean as `double(sum)/count`. Do **not** keep a floating-point running sum over millions of samples (rounding drift).

---

## Part B — Problems

### P1 · LC 622 Design Circular Queue — `MyCircularQueue`
**Ctx:** fixed-capacity RX ring between a DMA ISR (producer) and a decoder thread (consumer).
1. Clarify: `enQueue` on full → `false`; `deQueue` on empty → `false`; `Front/Rear` on empty → `-1`; capacity ≥ 1; single-threaded.
2. Brute: shift all elements left on each dequeue → ____ per op.
3. Optimise: A1. Why keep `size`?
5. Trace: `k=3`: enQ 1,2,3 (4 fails), deQ, enQ 4 — draw `buf`, `head`, `size` after each step. Edge: `k=1`.
6. Final: ____

### P2 · LC 641 Design Circular Deque — `MyCircularDeque`
**Ctx:** retransmission window where urgent frames re-enter at the *front*.
1. Clarify: same return codes; `insertFront`, `insertLast`, `deleteFront`, `deleteLast`, `getFront`, `getRear`.
2. Brute: `std::deque`-free version using a vector with O(n) front insert → ____
3. Optimise: reuse A1; derive each op in terms of `(head, size)`. Which op does *not* change `head`?
5. Trace: `k=4`, `insertFront(1)` on a fresh deque — what is `head` afterwards? (A1 "stepping backwards")
6. Final: ____

### P3 · LC 232 Implement Queue using Stacks — `MyQueue`
**Ctx:** you only have a LIFO allocator/stack primitive but need FIFO delivery order.
1. Clarify: `push`, `pop`, `peek`, `empty`; `pop/peek` never called on empty.
2. Brute: on every `pop`, move all to a temp stack and back → ____ per pop.
3. Optimise: A2. State the invariant of `out`. Why is "pour only when `out` is empty" necessary?
5. Trace: `push1 push2 pop push3 pop pop` — show both stacks after each op.
6. Final: ____ amortised; worst single op ____

### P4 · LC 239 Sliding Window Maximum — `max_sliding_window`
**Ctx:** peak RSSI over the last `k` samples, one output per sample once the window is full.
1. Clarify: `k ≥ 1`, `k ≤ n`; output length `n−k+1`; equal values?
2. Brute: scan each window → ____
3. Optimise: A3. **Write Invariant M, prove front = max, prove O(n).** Why `≤` (not `<`) in the dominate step? Why store indices not values?
5. Trace: `[1,3,-1,-3,5,3,6,7]`, `k=3` — show the deque (as indices and as values) after each `i`. Edge: strictly decreasing input (expiry matters), strictly increasing input (domination matters).
6. Final: ____

### P5 · LC 346 Moving Average from Data Stream — `MovingAverage`
**Ctx:** smoothed link-quality estimate over the last `size` SNR samples.
1. Clarify: window not yet full → average over what's there; samples are `int` (sum needs 64 bits?).
2. Brute: store all, re-sum the last `size` → ____
3. Optimise: A4; ring buffer of `size` ints + running sum.
5. Trace: `size=3`: `1, 10, 3, 5` → 1, 5.5, 4.667, 6.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "With only `head` and `tail`, `head == tail` means empty."
2. "`(head - 1) % cap` is fine for unsigned `head`."
3. "In 232 I can pour `in → out` on every `pop`."
4. "The monotonic deque stores values; indices are optional."
5. "The monotonic deque is O(nk) because of the inner `while`."
6. "A running float sum is the best way to average a long stream."
7. "A ring buffer must `memmove` on wrap."

## Part D — Self-check
* Prove invariant R is preserved by `deQueue`.
* Give the free-running-counter version of `size`, `isFull`, and `index(i)` for power-of-two `cap`.
* Construct an input where popping on `≥` vs `>` changes the deque contents but not the answers.
* For 239 with `k = 1`, what is the deque at every step?
