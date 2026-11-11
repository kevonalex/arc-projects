# 07 · Linked Lists — Answers & Write-ups

Reference code: `code/solutions/linked_lists_solutions.cpp`; tests `code/tests/test_linked_lists.cpp` (9 tests, ASan+UBSan clean; the arena frees every node, including cyclic lists).

---

## P1 · 206 Reverse Linked List

1. **Clarify.** `nullptr` → `nullptr`; one node → itself. Iterative to avoid O(n) recursion depth (an ISR/embedded stack would overflow).
2. **Brute.** Copy values to a `vector`, write back reversed: O(n) time, O(n) space (and it doesn't relink nodes — pointers held elsewhere would still point to the *old* positions).
3. **Optimise.** Relink in place (A1). Three-statement body, **order matters**: `next = cur->next; cur->next = prev; prev = cur; cur = next;`.
4. **Code.** see solution.
5. **Trace `1→2→3`:**
```
init  prev=∅  cur=1
step1 next=2 ; 1->∅      prev=1 cur=2     [1|∅]   2→3
step2 next=3 ; 2->1      prev=2 cur=3     [2→1|∅] 3
step3 next=∅ ; 3->2      prev=3 cur=∅     [3→2→1|∅]
return prev = 3→2→1
```
   Edge `nullptr`: loop doesn't run; returns `prev = nullptr` ✓. Edge one node: `1->∅`, returns `1` ✓.
6. **Final.** O(n) time, O(1) space.

## P2 · 21 Merge Two Sorted Lists

1. **Clarify.** Inputs sorted ascending; either may be null; equal keys: take from `a` first (stable); relink existing nodes.
2. **Brute.** Dump into a vector, `sort`, rebuild: O((n+m) log(n+m)), O(n+m) space — ignores that the inputs are already sorted.
3. **Optimise.** Dummy head + two-pointer merge (A2, A3). *Correctness:* at each step the smaller head is ≤ every remaining node (both lists sorted) so it is a valid next element of the merge; after one list is exhausted the other remainder is sorted and ≥ all placed (each placed node was ≤ some remaining head, and heads are the minima of their lists). `<=` implements stability.
4. **Code.** `tail->next = a ? a : b` at the end.
5. **Trace `1→2→4` & `1→3→4`:**
```
a=1,b=1  take a(1) (tie→a)   out: 1
a=2,b=1  take b(1)           out: 1,1
a=2,b=3  take a(2)           out: 1,1,2
a=4,b=3  take b(3)           out: 1,1,2,3
a=4,b=4  take a(4)           out: 1,1,2,3,4
a=∅      splice rest of b (4) out: 1,1,2,3,4,4 ✓
```
   Edge `a=∅,b=∅`: loop skipped, `tail->next = b = nullptr` → returns `nullptr`. Edge `[1,2,3]` + `[7,8]`: after `a` is consumed the b-tail is spliced (O(1)).
6. **Final.** O(n+m) time, O(1) extra space.

## P3 · 141 Linked List Cycle — Floyd, with proof

1. **Clarify.** Cycle = some node reachable twice via `next`; compare pointers. Empty list → false.
2. **Brute.** `unordered_set<const ListNode*>`: O(n) time, O(n) space.
3. **Optimise.** Tortoise (1 step) and hare (2 steps). Let the list be a tail of μ nodes then a cycle of λ nodes.
   * **No cycle ⇒ returns false.** `fast` advances 2 per iteration and the list is finite, so `fast` reaches `nullptr` after ≤ ⌈n/2⌉ iterations.
   * **Cycle ⇒ they meet.** After `t ≥ μ` steps both are inside the cycle. Number the cycle positions `0..λ−1` with `slow` at `s(t) = (t − μ) mod λ` and `fast` at `f(t) = (2t − μ) mod λ`. They collide iff `f − s ≡ t ≡ 0 (mod λ)`. The smallest `t ≥ μ` divisible by `λ` is `t* = λ·⌈μ/λ⌉ ≤ μ + λ − 1`, so the loop exits by iteration `μ + λ`. Equivalent gap view: the forward gap from slow to fast changes by exactly +1 mod λ each step, so it cannot skip 0.
   * **They can't "pass through" each other without meeting**, because speeds differ by 1 — positions are integers on a ring and the gap decreases/increases by single units.
4. **Code.** `while (fast && fast->next) { slow = slow->next; fast = fast->next->next; if (slow == fast) return true; }`
5. **Trace `3→2→0→−4→(2)`** (μ=1: node 3; λ=3: nodes 2,0,−4). Positions by value:

| step | slow | fast |
|------|------|------|
| 0 | 3 | 3 |
| 1 | 2 | 0 |
| 2 | 0 | 2 (0→−4→2) |
| 3 | −4 | −4 (2→0→−4) ← meet |

   Formula check: `t* = λ⌈μ/λ⌉ = 3·⌈1/3⌉ = 3` ✓. Edges: self-loop (`μ=0, λ=1`): step 1 → both at the node → true. Two-node ring: step 1: slow at 2nd, fast at 2nd (1→2→1? fast moves 2: node1→node2→node1) — slow=node2, fast=node1; step 2: slow=node1, fast=node1 → true. No cycle, even length 2: `fast=1`, `fast->next=2` → step: slow=2, fast=`2->next`=∅ → `fast && …` false → exit false ✓. Odd length 3: fast jumps to node 3 whose `next` is null → exit false.
   Test also builds μ=997, λ=3 (1000 nodes, tail re-linked to index 997) ⇒ meet at `t* = 3·⌈997/3⌉ = 999`.
6. **Final.** O(μ+λ) = O(n) time, O(1) space.

## P4 · 19 Remove Nth Node From End

1. **Clarify.** `1 ≤ n ≤ length`; if the head is removed return the new head; node is unlinked (caller owns/free it).
2. **Brute.** Pass 1 count `L`; pass 2 walk to node `L−n−1`: O(L) time but two passes (matters when the list is a live stream/DMA-owned).
3. **Optimise.** Dummy + gap-`n` two pointers (A5). Invariant: `lead` is `n` nodes ahead of `lag`; when `lead->next == nullptr`, `lag->next` is the n-th from the end.
4. **Code.** `for i<n: lead=lead->next; while(lead->next){lead=lead->next; lag=lag->next;} lag->next = lag->next->next;`
5. **Trace `1→2→3→4→5, n=2`:** `dummy→1→2→3→4→5`. `lead` advanced 2: at node 2; `lag` at dummy. Loop: lead 2→3 (lag 1), 3→4 (lag 2), 4→5 (lag 3); `lead->next == ∅` stop. `lag` = node 3; `lag->next = 4` → `3->next = 5`. Result `1→2→3→5` ✓. Edge `[1], n=1`: lead=1, `lead->next==∅` immediately, lag=dummy; `dummy->next = ∅` → returns `nullptr` ✓. Edge `[1,2], n=2`: lead ends at node 2 (advance 2 from dummy), lag dummy; remove head → `2` ✓. Edge `n=1` on `[1,2,3]` → lead at 1, loop moves until lead=3, lag=2 → remove 3 ✓.
6. **Final.** O(L) time single pass, O(1) space.

---

## Part C answers
1. **False.** You'd lose the rest of the list.
2. **False.** O(1) extra: nodes are relinked.
3. **False.** They always meet within `μ + λ` steps.
4. **False.** With step 3 and an even cycle they can leapfrog (gap changes by 2).
5. **False.** `fast->next` on `nullptr` is UB; test `fast` first.
6. **False.** It makes "remove the head" identical to every other case.
7. **False.** Given only a *value* you'd need to search (O(n)). (The "copy next node into this one" trick works only when you have a pointer to the node and it is not the tail.)

## Part D answers
* Reversal states: see trace above.
* `μ=5, λ=3`: `t* = 3·⌈5/3⌉ = 6`. `μ=0, λ=4`: the formula gives `t = 0` (both pointers start on the head), but the loop moves *before* comparing, so the first comparison that succeeds is the smallest **positive** multiple of λ: `t = 4`.
* No splice: the remainder is lost (silent truncation). `<` instead of `<=`: unstable — equal elements from `b` jump ahead of `a` (breaks "preserve arrival order").
* 142: after meeting at `t*`, `slow` is `a = t* − μ` into the cycle with `a ≡ −μ (mod λ)`; so a pointer from the head and one from the meeting point, both stepping 1, meet exactly at the cycle entry after μ steps.
