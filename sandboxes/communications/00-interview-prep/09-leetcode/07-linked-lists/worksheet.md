# 07 · Linked Lists — Pattern Worksheet

**Pattern:** pointer surgery with a **loop invariant over a few named pointers** (`prev`, `cur`, `next`, `tail`, `slow`, `fast`) plus a **dummy head** to erase special cases; O(1) extra space.
**Why it matters at a radio/protocol shop:** packet-buffer chains, free-lists of descriptors, retransmit queues and RLC segment lists are intrusive linked lists. Bugs here are not "wrong answer" — they are use-after-free, lost buffers and infinite loops in the ISR.
**Coded:** 206, 21, 141, 19 — `code/headers/linked_lists.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

**Ownership rule in this pack:** the functions *relink* nodes and never `delete` them; the caller (`ListArena`) owns all nodes — like a buffer pool. This keeps cyclic test lists leak-free under ASan.

## The method — every problem
1 **Clarify** (null/one-node list, cycles?, may I mutate/relink? who frees?) → 2 **Brute force + complexity** → 3 **Optimise** (state the loop invariant in words *about which nodes point where*) → 4 **Code** → 5 **Hand-trace** by drawing boxes-and-arrows for a normal list and a 0/1-node list → 6 **Final complexity**.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the techniques work

### A1. Reversal invariant (206)
Maintain `prev` (head of the already-reversed prefix, `nullptr` initially), `cur` (head of the untouched suffix). **Invariant:** *reading `prev` onward gives the original first `j` nodes in reverse order, ending in `nullptr`; reading `cur` onward gives the original remaining `n−j` nodes.* Step: save `next = cur->next` **first** (otherwise the suffix is lost), set `cur->next = prev`, advance `prev = cur`, `cur = next`. After `n` steps `cur == nullptr`, `prev` is the new head. Each node is touched once: O(n) time, O(1) space.

### A2. Dummy (sentinel) head
Any operation that may change *which node is first* (merge, delete the head) normally needs `if (head == nullptr)` branches. A stack-allocated `dummy` whose `next` is the real head turns "the head" into "just another `next` field", so the same code handles every position. Return `dummy.next`.

### A3. Merging sorted lists (21)
**Invariant:** the list built from `dummy.next` to `tail` is the sorted merge of the nodes consumed so far, and every unconsumed node in `a` and `b` is ≥ `tail->val`. Each step takes the smaller head (`≤` favouring `a` ⇒ **stable**). When one list ends, splice the remainder of the other in O(1) — it is already sorted and ≥ everything placed. O(n+m) time, O(1) space (nodes are relinked, not copied).

### A4. Fast & slow pointers: why the tortoise and hare meet (141)
Model the list as a **tail of length μ** feeding a **cycle of length λ** (μ = 0 if the head is on the cycle). Unroll the walk: after `t` steps, `slow` has made `t` moves and `fast` has made `2t` moves.
* **No cycle:** `fast` falls off the end after ≈ `n/2` rounds, hitting `nullptr` ⇒ return false. (Test `fast && fast->next` before moving two.)
* **Cycle:** for `t ≥ μ` both pointers are on the cycle. Define the **gap** `g(t)` = (cycle distance from `slow` to `fast`, going forward) `mod λ`. Each step `fast` advances 2 and `slow` advances 1 ⇒ `g(t+1) ≡ g(t) + 1 (mod λ)`. The gap changes by *exactly 1* each step, so it takes every residue in order and **cannot jump over 0**; after at most `λ` more steps `g ≡ 0 ⇒ slow == fast`. Algebraically they meet at the first `t ≥ μ` with `2t ≡ t (mod λ)`, i.e. `λ | t`: `t = λ·⌈μ/λ⌉ ≤ μ + λ − 1` (for `μ = 0` take the smallest positive multiple, `t = λ`, because the loop moves before it compares). So the loop terminates within `μ + λ` steps — linear in the list length.
* **Why speed difference 1 matters.** If `fast` moved 3 per step the gap would change by 2 and, for even `λ` and odd initial gap, never hit 0 — the pointers could leapfrog forever. (A difference of `d` works iff `gcd(d, λ) = 1` for all λ ⇒ `d = 1`.)
* **Memory:** two pointers, O(1) — versus a hash set of visited nodes, O(n).

### A5. Two pointers with a fixed gap (19)
To reach the *n-th node from the end* in one pass: advance `lead` `n` steps from `dummy`, then move `lead` and `lag` together until `lead->next == nullptr`. **Invariant:** `lead` is always exactly `n` nodes ahead of `lag`; when `lead` is the last node, `lag` is the node *before* the n-th from the end — the one we need to edit. The dummy head makes `n == length` (removing the head) the same code path.

---

## Part B — Problems

### P1 · LC 206 Reverse Linked List — `reverse_list`
**Ctx:** reverse a chain of RLC segments / buffer descriptors in place.
1. Clarify: empty / single-node list? iterative vs recursive (stack depth for 10⁵ nodes)?
2. Brute: copy values into a vector, rewrite reversed → ____ space
3. Optimise: A1. State the invariant; which statement order is critical?
5. Trace: `1→2→3`, and `nullptr`.
6. Final: ____

### P2 · LC 21 Merge Two Sorted Lists — `merge_two_lists`
**Ctx:** merge two timestamp-ordered event lists from two radios while preserving order of equal timestamps.
1. Clarify: either list null? duplicates — which first (stability)? may I allocate new nodes?
2. Brute: concatenate into a vector, sort, rebuild → ____
3. Optimise: A2 + A3.
5. Trace: `1→2→4` with `1→3→4`; edge: one list empty, one list entirely smaller.
6. Final: ____

### P3 · LC 141 Linked List Cycle — `has_cycle`
**Ctx:** detect a corrupted descriptor free-list that loops back on itself without allocating.
1. Clarify: pointer identity (not value) defines a cycle; null → false.
2. Brute: hash set of visited node addresses → ____
3. Optimise: A4 — **write the proof** (gap argument and the `t = λ⌈μ/λ⌉` formula).
5. Trace: `3→2→0→−4→(back to 2)`: list `(slow, fast)` positions each step (μ=1, λ=3). Edge: self-loop; two-node ring; list with no cycle of even and odd length.
6. Final: ____

### P4 · LC 19 Remove Nth Node From End of List — `remove_nth_from_end`
**Ctx:** drop the n-th oldest entry of a singly linked retransmit list in a single pass.
1. Clarify: `1 ≤ n ≤ length`? removing the head? who frees the node?
2. Brute: two passes (count length, then walk `length−n`) → ____
3. Optimise: A5.
5. Trace: `1→2→3→4→5, n=2` (draw `lead`/`lag` each step); edge `[1], n=1`, `[1,2], n=2`.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "In reversal I can write `cur->next = prev` before saving `next`."
2. "Merging needs extra memory proportional to the lists."
3. "`slow`/`fast` might never meet in a cyclic list if the cycle is long."
4. "`fast` could move 3 steps and still always meet `slow`."
5. "Checking `fast->next` before `fast` is fine."
6. "For 19 I need a dummy node only for style."
7. "Deleting a node in a C++ function that is given only that node's *value* is O(1)."

## Part D — Self-check
* Draw the state after each of the three statements in the reversal loop.
* Compute the meeting step for `μ=5, λ=3` and for `μ=0, λ=4`.
* What goes wrong in 21 if you don't splice the remainder? if you use `<` instead of `<=`?
* How would you find the *start* of the cycle (142)? (Hint: reset one pointer to the head, step both by 1 — prove with `μ + a ≡ 0 (mod λ)`.)
