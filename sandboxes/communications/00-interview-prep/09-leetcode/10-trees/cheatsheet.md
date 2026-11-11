# 10 · Trees — Cheatsheet

**Core idea:** recursion shaped like the data; prove with structural induction; BFS+queue for levels. Cost = O(n) time, **O(h)** stack.

## Templates
```cpp
// bottom-up
int f(Node* n){ if(!n) return BASE; return combine(n->val, f(n->left), f(n->right)); }

// top-down with constraints
bool g(Node* n, long long lo, long long hi){
  if(!n) return true;
  if(n->val <= lo || n->val >= hi) return false;
  return g(n->left, lo, n->val) && g(n->right, n->val, hi); }

// level order
queue<Node*> q{{root}};
while(!q.empty()){ size_t w=q.size(); vector<int> row;
  while(w--){ auto* n=q.front(); q.pop(); row.push_back(n->val);
              if(n->left) q.push(n->left); if(n->right) q.push(n->right);} out.push_back(row); }
```

## Recipes
| LC | Move | Time / Space |
|----|------|--------------|
| 104 | `1 + max(l, r)` | O(n) / O(h) |
| 226 | swap children, recurse | O(n) / O(h) |
| 102 | queue + size snapshot | O(n) / O(width) |
| 98 | pass `(lo,hi)` down (64-bit) or in-order strictly increasing | O(n) / O(h) |
| 297 | preorder + `#` markers, comma separated | O(n) / O(n) |

## Traversal facts
* inorder of BST = sorted; preorder = serialize order; postorder = children before parent (delete/size).
* Preorder + inorder (distinct) ⇒ unique tree; preorder alone ⇒ ambiguous (needs null markers).
* Iterative inorder: push left spine, pop, go right.
* Morris traversal: O(1) space by threading.

## Gotchas
* Null checks first. `root` null is a legal input.
* BST: **global** bounds, strict inequality, 64-bit bounds.
* Level order: snapshot `q.size()` before the inner loop.
* Deep recursion on skewed input ⇒ explicit stack / BFS.
* Serialization: separators, negative numbers, and who owns the nodes.
* Invert: swap, don't allocate.

## Script
"Clarify null/skew/duplicates → brute force rescans subtrees or allocates → define what each call returns/assumes → base case + combine → trace a 3-level tree and null → O(n) time, O(h) stack."
