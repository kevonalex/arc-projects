# 10 · Trees — Answers & Write-ups

Reference code: `code/solutions/trees_solutions.cpp`; tests `code/tests/test_trees.cpp` (12 tests, ASan+UBSan clean).

---

## P1 · 104 Maximum Depth of Binary Tree

1. **Clarify.** Depth = number of nodes on the longest root-to-leaf path. `null → 0`.
2. **Brute.** There's nothing slower than visiting every node; the recursion is already optimal. BFS alternative: count levels in the level-order loop.
3. **Optimise.** `depth(null)=0`; `depth(n) = 1 + max(depth(l), depth(r))`. *Proof (structural induction):* the longest path from `n` goes through the deeper child, plus `n` itself. Stack depth = `h`: `O(log n)` balanced, `O(n)` skewed (10⁵ nodes ⇒ may overflow a small stack — use the BFS version or an explicit stack when inputs are untrusted).
4. **Code.** one line.
5. **Trace `[3,9,20,null,null,15,7]`:** `d(9)=1`, `d(15)=1`, `d(7)=1`, `d(20)=1+max(1,1)=2`, `d(3)=1+max(1,2)=3` ✓. Edges: null → 0; single node → 1; left chain `[1,2,null,3,null,4]` → 4.
6. **Final.** O(n) time, O(h) stack.

## P2 · 226 Invert Binary Tree

1. **Clarify.** Mirror the tree; return the root; null OK.
2. **Brute.** Build a new tree by recursion `new(n->val, mirror(r), mirror(l))`: O(n) time **and** O(n) extra nodes.
3. **Optimise.** In place: swap `left`/`right`, recurse into both children. Order is irrelevant (swap-then-recurse or recurse-then-swap give the same tree since the two subtrees are inverted independently). *Correctness by induction:* `invert(n)` = node with `left = invert(old right)`, `right = invert(old left)`, which is the definition of the mirror image. `invert∘invert = id` because mirroring twice restores every child order (checked in tests).
4. **Code.** `swap(root->left, root->right); invert(left); invert(right);`
5. **Trace `[4,2,7,1,3,6,9]`:** swap at 4 → left=7-subtree, right=2-subtree; recurse into 7: swap `6,9` → `9,6`; into 2: swap `1,3` → `3,1`. Level order: `4, 7, 2, 9, 6, 3, 1` ✓. Edge `[1,2]` (only a left child) → `[1,null,2]` ✓ — the swap moves a child into the empty slot. Null → null.
6. **Final.** O(n) time, O(h) stack, O(1) extra heap.

## P3 · 102 Binary Tree Level Order Traversal

1. **Clarify.** Output `vector<vector<int>>`, level by level left→right; empty tree → `[]`.
2. **Brute.** DFS carrying `depth`, appending to `result[depth]` — also O(n) and perfectly valid; BFS is the idiomatic answer and avoids deep recursion.
3. **Optimise.** Queue with **size snapshot** (A3). *Invariant:* at the top of each round the queue holds exactly the nodes of one depth in left-to-right order. Without the snapshot (`for` over `q.size()` evaluated *each* iteration) you'd mix levels as children are pushed.
4. **Code.** `width = q.size(); for i<width: pop, record, push children`.
5. **Trace `[3,9,20,null,null,15,7]`:**
```
q=[3]            width1 -> row [3]       push 9,20
q=[9,20]         width2 -> row [9,20]    push 15,7 (9 has no children)
q=[15,7]         width2 -> row [15,7]
result [[3],[9,20],[15,7]] ✓
```
   Edge skewed `[1,2,null,3]` → `[[1],[2],[3]]` (queue size is always 1). Edge null → `[]`.
6. **Final.** O(n) time, O(w) space where `w` = max width (up to n/2).

## P4 · 98 Validate Binary Search Tree

1. **Clarify.** Strict BST (no duplicates). Values span the full `int` range, so sentinel bounds of `INT_MIN/INT_MAX` are *not* safe.
2. **Brute.** For each node, scan its entire left subtree for any key `≥ val` and right subtree for any `≤ val`: O(n·h) = O(n²) worst.
3. **Optimise.** Pass bounds down (A4). `valid(n, lo, hi)`: null → true; `n->val ≤ lo || n->val ≥ hi` → false; else `valid(left, lo, n->val) && valid(right, n->val, hi)`. Root call `(−∞, +∞)` using `long long`.
   *Why the parent-only check fails:* tree `5 → (4, 6 → (3, 7))`: node 6 has children 3 and 7; locally `3 < 6 < 7` ✓, yet 3 is in 5's right subtree and `3 < 5`. With bounds, node 3 is checked against `(5, 6)` and rejected.
   *In-order alternative:* traverse in-order keeping the previous value; any `cur ≤ prev` ⇒ invalid. Equivalent, same cost.
4. **Code.** helper `bst_ok(n, lo, hi)` with `LLONG_MIN/MAX`.
5. **Trace `[5,1,4,null,null,3,6]`:** root 5 `(−∞,∞)` ok; left 1 `(−∞,5)` ok; right 4 `(5,∞)` → `4 ≤ 5` → **false** ✓. Trace `[2,1,3]`: 2 ok; 1 in `(−∞,2)` ok; 3 in `(2,∞)` ok → **true**. Edge `[5,4,6,null,null,3,7]`: 3 in `(5,6)`: `3 ≤ 5` → false ✓. Edge `[2,2,2]`: left 2 in `(−∞,2)` → `2 ≥ 2` → false ✓. Edge `[INT_MIN]` → `INT_MIN > LLONG_MIN` → true ✓ (with `int` sentinels it would be wrongly rejected). Edge `[INT_MIN, null, INT_MIN]` → right child bound `(INT_MIN, ∞)` rejects equal → false.
6. **Final.** O(n) time, O(h) stack.

## P5 · 297 Serialize and Deserialize Binary Tree

1. **Clarify.** Any tree must round-trip, including empty and skewed, negative values, `INT_MIN`. Format `1,2,#,#,3,#,#,` (comma terminated; `#` = null).
2. **Brute.** Store inorder + preorder and rebuild (requires distinct values, O(n²) naive search) — fragile.
3. **Optimise.** Preorder with null markers (A5). *Proof of round-trip (induction on size):* encode(null)=`#,`; encode(n) = `val,` + encode(l) + encode(r). Decoder reads one token: `#` ⇒ null (consumed exactly encode(null)); otherwise value `v` creates node `n`, then **recursively** decodes `l` — by the inductive hypothesis it consumes exactly encode(l) — then `r` likewise. So the cursor ends exactly after encode(n) and `decode(encode(T)) = T`. Ownership: nodes created by `deserialize` come from the caller's `TreeArena`.
   Alternative: level-order with `null` markers (LeetCode's own format) — iterative, no recursion depth problem.
4. **Code.** `ser` appends; `des` reads tokens with `find(',')` and `stoi`.
5. **Trace `[1,2,3,null,null,4,5]`:** serialize: `1,` → left `2,` → its children `#,#,` → right `3,` → left `4,#,#,` → right `5,#,#,` ⇒ `1,2,#,#,3,4,#,#,5,#,#,`. Deserialize: `1` → node; `des`: `2` → node; `des`: `#` → null (2.left); `des`: `#` → null (2.right); back in 1: `des`: `3` → node; `4` (with two `#`); `5` (with two `#`) → `[1,2,3,null,null,4,5]` ✓. Edge null tree → `"#,"` → null ✓. Edge `INT_MIN`: `stoi("-2147483648")` OK ✓.
   *Caveat:* recursion depth = tree height; a 10⁵-deep chain may overflow the stack — use the iterative level-order variant for untrusted input.
6. **Final.** O(n) time and O(n) space for the string; O(h) recursion stack.

---

## Part C answers
1. **False.** O(h): O(n) for a skewed tree.
2. **False.** Needs global bounds (see counter-example).
3. **False.** A queue (iterative) is the standard approach; recursion with depth works too.
4. **False** without null markers. `[1,2]` with 2 left or right child have the same preorder `1,2`.
5. **False.** Swap pointers in place.
6. **False.** Node values may *equal* them; use wider bounds (`long long`) or `optional`/pointer bounds.

## Part D answers
* Induction: `depth(null)=0` correct; for `n` the longest path = 1 + longer child path.
* Stack instead of queue gives DFS (root, then deepest-first); level boundaries are lost.
* BFS: `levels = 0; while queue: ++levels; process one level (size snapshot)`.
* In-order(BST) = in-order(left) · val · in-order(right); by IH each part is increasing, and `max(left) < val < min(right)` ⇒ whole is strictly increasing.
