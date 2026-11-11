# 10 · Trees — Pattern Worksheet

**Pattern:** a tree is defined recursively (`null` or `value + left + right`), so the **recursive algorithm has the same shape as the data**, and its correctness is a *structural induction*. When the question is "level by level", use a **queue** (BFS).
**Why it matters at a radio/protocol shop:** nested protocol headers/TLVs, ASN.1/PER message trees, scheduler hierarchies, decision trees for decoding (Huffman-like / prefix codes), sorted indices.
**Coded:** 104, 226, 102, 98, 297 — `code/headers/trees.hpp` (a `TreeArena` owns nodes; `to_level` converts trees back to LeetCode arrays for assertions). Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (null tree? skewed depth? values range/duplicates? recursion depth limit?) → 2 **Brute force + complexity** → 3 **Optimise** (state what each recursive call *returns* and what it *assumes*, plus the base case) → 4 **Code** → 5 **Hand-trace** a normal tree *and* a null/skewed tree → 6 **Final complexity** (time **and** stack depth).

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the technique works

### A1. Structural induction = the recursion template
To prove `f(root)` correct: (1) base case `f(null)` is right; (2) assuming `f(left)` and `f(right)` are right, show how to combine them with `root->val` to get `f(root)`. The code *is* that proof:
```
f(null)  = base
f(n)     = combine(n->val, f(n->left), f(n->right))
```
**Cost:** every node is visited once, each with O(1) combine work ⇒ **O(n) time**. **Stack depth = tree height `h`** (`O(log n)` balanced, `O(n)` skewed). In embedded/ISR code or on adversarial input, deep recursion = stack overflow ⇒ use an explicit stack/queue, or state the limit.

### A2. Top-down vs bottom-up information flow
* **Bottom-up** (104, 226): children return a value, parent combines (`depth = 1 + max(...)`).
* **Top-down with parameters** (98): the parent passes **constraints** down (`(lo, hi)` bounds), the child checks itself against them.
Decide which direction the needed information flows.

### A3. BFS by levels (102)
Queue + **snapshot the size** at the start of each round. **Invariant:** at the start of a round the queue holds exactly the nodes of depth `d`, left to right. Popping `size` nodes and pushing their children leaves exactly the nodes of depth `d+1` in order. Each node enters/leaves the queue once ⇒ O(n); the queue holds at most one level ⇒ O(width) space.

### A4. BST validity needs *global* bounds (98)
A BST means: for every node, **all** keys in its left subtree `<` key `<` **all** keys in its right subtree. Checking only `left->val < val < right->val` (local) is **wrong**: the tree `5 → (4, 6 → (3, 7))` passes every parent/child check, yet `3` sits in `5`'s *right* subtree. Correct recursion: `valid(n, lo, hi)` requires `lo < n->val < hi`; left child gets `(lo, n->val)`, right child gets `(n->val, hi)`. Each node inherits the **intersection** of all ancestor constraints.
Equivalent: an in-order traversal must be strictly increasing. Use `long long` (or `optional`) bounds so `INT_MIN`/`INT_MAX` node values are not mistaken for "no bound".

### A5. Serialization = a prefix-free encoding (297)
Preorder with explicit `null` markers (`#`) is **uniquely decodable**: the first token is the root (or `#`); the decoder recursively consumes *exactly* the tokens that belong to the left subtree, then those of the right subtree. By induction on tree size the decoder consumes exactly the encoder's output ⇒ `decode(encode(T)) = T`. Without null markers, preorder alone is ambiguous (the same preorder fits many trees). Separators (`,`) are needed so multi-digit and negative numbers parse.

---

## Part B — Problems

### P1 · LC 104 Maximum Depth of Binary Tree — `max_depth`
**Ctx:** depth of a nested TLV/protocol-header tree (how deep can an adversarial message nest?).
1. Clarify: depth counted in nodes or edges (LeetCode: nodes); null → 0.
2. Brute: n/a — recursion *is* the natural solution. Write the BFS version too.
3. Optimise: A1/A2. What is the recursion's stack depth for a skewed tree?
5. Trace: `[3,9,20,null,null,15,7]` → 3; edge null, one node, left-chain of 4.
6. Final: ____ time, ____ stack.

### P2 · LC 226 Invert Binary Tree — `invert_tree`
**Ctx:** mirror a binary decision tree (swap the 0/1 branches of a prefix code).
1. Clarify: in place? return the root; null.
2. Brute: build a new mirrored tree → ____ space
3. Optimise: swap children, recurse. Does the order (swap-then-recurse vs recurse-then-swap) matter? Prove `invert(invert(T)) = T`.
5. Trace: `[4,2,7,1,3,6,9]`; edge null, one node, one child only.
6. Final: ____

### P3 · LC 102 Binary Tree Level Order Traversal — `level_order`
**Ctx:** dump a scheduler hierarchy tier by tier.
1. Clarify: result is a list of levels, each left→right; null → empty.
2. Brute: DFS recording depth into `vector<vector<int>>` (also valid!) → ____
3. Optimise: A3 — write the invariant and why the size snapshot is necessary.
5. Trace: `[3,9,20,null,null,15,7]` → `[[3],[9,20],[15,7]]`; edge skewed tree.
6. Final: ____ time, ____ space (in terms of width).

### P4 · LC 98 Validate Binary Search Tree — `is_valid_bst`
**Ctx:** verify a sorted index/frequency table wasn't corrupted in storage.
1. Clarify: strict `<` (no duplicates); node values can be `INT_MIN`/`INT_MAX`.
2. Brute: for each node scan whole left/right subtrees → ____
3. Optimise: A4. **Draw** the counter-example to the parent-only check; fix with bounds; mention the in-order alternative.
5. Trace: `[5,1,4,null,null,3,6]` (invalid) and `[2,1,3]` (valid); edge `[5,4,6,null,null,3,7]`, `[2,2,2]`, `[INT_MIN]`.
6. Final: ____

### P5 · LC 297 Serialize and Deserialize Binary Tree — `serialize` / `deserialize`
**Ctx:** ship a decision tree across a link as a byte string.
1. Clarify: format is yours; must round-trip every tree including null, negatives, skewed. (In this pack `deserialize` takes a `TreeArena&` — who owns the new nodes?)
2. Brute: store inorder + preorder (needs distinct values) → ____
3. Optimise: A5. Prove decode∘encode = id by structural induction.
5. Trace: `[1,2,3,null,null,4,5]` → `1,2,#,#,3,4,#,#,5,#,#,`; decode step by step. Edge null tree.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "Recursion on a tree is O(log n) space."
2. "To validate a BST it is enough that each node is greater than its left child and less than its right child."
3. "Level order needs recursion."
4. "Preorder alone identifies a binary tree."
5. "Inverting must allocate new nodes."
6. "`INT_MIN` and `INT_MAX` are good sentinels for BST bounds."

## Part D — Self-check
* Prove `max_depth` by structural induction.
* What data structure turns the level-order into a DFS-style traversal? (replace queue by stack: you get preorder-ish order, no level grouping).
* Rewrite `max_depth` iteratively with BFS levels.
* Show that in-order traversal of a BST is strictly increasing (induction).
