# 11 · Graphs — Answers & Write-ups

Reference code: `code/solutions/graphs_solutions.cpp`; tests `code/tests/test_graphs.cpp` (9 tests, ASan+UBSan clean).

---

## P1 · 200 Number of Islands

1. **Clarify.** Grid of `'0'/'1'`; island = maximal 4-connected set of `'1'`; count them. Empty grid → 0. We don't mutate the input (a `seen` matrix is used; LeetCode accepts "sinking" cells too, which is O(1) extra space but destroys the input).
2. **Brute.** From every land cell, flood fill without marking visited — revisits cells exponentially/forever; with marking *per fill* but no global memory you recount islands.
3. **Optimise.** Scan cells; whenever an unvisited `'1'` is found, `++islands` and flood-fill its whole component (A1). Mark on **push** so every cell is enqueued at most once. BFS with an explicit queue (no recursion depth problem). Union-find is an alternative (handy if cells arrive online).
4. **Code.** `dr/dc` arrays for the 4 neighbours; bounds check; `seen[r][c]`.
5. **Trace (3-island example):**
```
1 1 0 0 0
1 1 0 0 0
0 0 1 0 0
0 0 0 1 1
scan (0,0): new island #1 -> fills the 2x2 block
scan (2,2): new island #2 -> fills just (2,2); (3,3) is diagonal, NOT a neighbour
scan (3,3): new island #3 -> fills (3,3),(3,4)
```
   → **3** ✓. Edges: `[]` → 0; `[['0']]` → 0; `[['1']]` → 1; checkerboard `101/010/101` → **5** (diagonals don't connect); 300×300 all land → 1 with a queue of at most ~600 entries at a time (recursive DFS would nest 90 000 deep).
6. **Final.** O(R·C) time (each cell queued once), O(R·C) space (`seen` + queue).

## P2 · 207 Course Schedule

1. **Clarify.** `{a,b}`: take `b` before `a`, edge `b → a`. Possible to finish ⇔ the graph is acyclic. Self-loop `{0,0}` is a cycle. Duplicate edges are harmless in Kahn as long as in-degree counts them as many times as the edge is relaxed.
2. **Brute.** From each node DFS along all paths looking for a revisit: exponential in the worst case without memoisation.
3. **Optimise.** Kahn (A2): build adjacency and in-degrees, queue all in-degree-0 nodes, pop `u`, `++done`, decrement successors, enqueue those hitting 0. Answer `done == numCourses`. Proof in A2 (source lemma + cycle nodes never reach 0). Alternative: DFS with three colours (white/gray/black) — a gray→gray edge is a back edge ⇒ cycle.
4. **Code.** see solution.
5. **Trace `2, [[1,0]]`:** edge `0→1`; indeg `[0,1]`; queue `[0]`; pop 0 → indeg[1]=0 → queue `[1]`; pop 1; `done=2 == 2` → **true** ✓. `[[1,0],[0,1]]`: indeg `[1,1]`, queue empty, `done=0` → **false** ✓. Diamond `4, [[1,0],[2,0],[3,1],[3,2]]`: queue `[0]` → 1,2 ready → 3 reaches indeg 0 after both → `done=4` **true** ✓. Cycle behind a tail `{1,0},{2,1},{3,2},{1,3}`: node 0 peeled; node 1 still has predecessor 3 (cycle 1→2→3→1) → never freed → `done=1 ≠ 4` → **false** ✓. `{0,0}`: indeg[0]=1 → never queued → false ✓.
6. **Final.** O(V + E) time and space.

## P3 · 743 Network Delay Time — Dijkstra

1. **Clarify.** Directed weighted graph, `w ≥ 0`, nodes `1..n`; signal starts at `k`; answer = time until the last node receives it = `max δ(v)`; `−1` if some node unreachable. `n = 1` → 0.
2. **Brute.** Bellman–Ford `O(V·E)` (works with negatives); Floyd–Warshall `O(V³)`.
3. **Optimise.** Dijkstra with min-heap and lazy deletion (A3): `dist[k]=0`, push `(0,k)`; pop `(d,u)`; skip if `d > dist[u]` (stale); relax edges; push on improvement.
   **Full proof of safety — restated:** let `u` be the first settled vertex with `dist[u] > δ(u)`; `y` the first unsettled vertex on a true shortest path and `x` its settled predecessor; then `dist[y] = δ(y)` (relaxation of `x→y` + minimality) and, since *weights are ≥ 0*, `δ(y) ≤ δ(u) < dist[u]` — so `y` (with a smaller key) would have been popped before `u`. Contradiction. ∎
   **Counter-example with a negative edge:** `s→a (2)`, `s→b (3)`, `b→a (−2)`. Dijkstra pops `a` at 2 (settled) before `b`; true `δ(a) = 3 + (−2) = 1`. The "settled ⇒ final" step breaks exactly at `δ(y) ≤ δ(u)`.
   Heap size bound: each successful relaxation pushes once, `≤ E` pushes ⇒ `O(E log E) = O(E log V)`.
4. **Code.** `dist` as `long long` (avoid overflow on sums of large weights); `greater<pair<ll,int>>` min-heap; final loop returns `−1` if any `dist == ∞`, else the max.
5. **Trace `times=[[2,1,1],[2,3,1],[3,4,1]], n=4, k=2`:**
```
dist: [∞,∞,0,∞,∞] (1-indexed: dist[2]=0)   heap {(0,2)}
pop (0,2): relax 2->1: dist1=1 push (1,1); relax 2->3: dist3=1 push (1,3)
pop (1,1): no out-edges
pop (1,3): relax 3->4: dist4=2 push (2,4)
pop (2,4): done
dist = {1:1, 2:0, 3:1, 4:2} -> max 2 ✓
```
   Edges: `n=1,k=1, times=[]` → 0. `[[1,2,1]], n=2, k=2` → node 1 unreachable → **−1**. Cheap 2-hop beats heavy direct edge `[[1,2,10],[1,3,1],[3,2,1]], k=1` → `dist2 = 2` (via 3), answer 2 ✓ (a stale heap entry `(10,2)` is skipped by the `d > dist` test). Zero weights `[[1,2,0],[2,3,0]]` → 0. Cycle `1⇄2` with `2→3 (4)`: `δ = {0,1,5}` → 5.
6. **Final.** O((V + E) log V) time, O(V + E) space.

## P4 · 133 Clone Graph

1. **Clarify.** Undirected connected graph, node values `1..n` (given by `NodeArena::build_adj`), neighbour lists as given; null → null. Self-loops possible in our tests. The clone must share **no** nodes with the original, and must preserve neighbour **order**. Ownership: nodes created by `clone_graph` are owned by the arena the caller passes (a deliberate deviation from LeetCode's raw `new`).
2. **Brute.** Recursive copy `new Node(val); for nb: copy(nb)` — never terminates on a cycle (every undirected edge is a 2-cycle!).
3. **Optimise.** BFS with `map original → copy`. `copy[node] = new`. Pop `u`; for each neighbour `v`: if `v` has no copy, create it and enqueue; then append `copy[v]` to `copy[u]->neighbors`. **Invariant (A4):** every node ever enqueued already has its unique clone; a node's neighbour vector is filled exactly once (when it is popped). Self-loop: `v == u` already has a copy ⇒ `copy[u]->neighbors` gets `copy[u]`, i.e. loops onto the **copy** ✓.
4. **Code.** see solution.
5. **Trace `[[2,4],[1,3],[2,4],[1,3]]` (a 4-cycle):**
```
copy{1->1'}; queue [1]
pop 1: nb 2 -> new 2', enqueue; 1'.nb=[2']; nb 4 -> new 4', enqueue; 1'.nb=[2',4']
pop 2: nb 1 -> exists; 2'.nb=[1']; nb 3 -> new 3', enqueue; 2'.nb=[1',3']
pop 4: nb 1 -> 4'.nb=[1']; nb 3 -> exists; 4'.nb=[1',3']
pop 3: nb 2, 4 -> 3'.nb=[2',4']
to_adj(clone) == [[2,4],[1,3],[2,4],[1,3]] ✓; every clone pointer differs from every original pointer ✓
```
   Edges: `nullptr` → `nullptr`; single node with no neighbours → fresh node with none; mutating the clone's neighbour list leaves the original untouched (tested).
6. **Final.** O(V + E) time and space.

---

## Part C answers
1. **False.** Mark on push (enqueue); marking on pop allows duplicates in the queue (up to O(E) entries) and can blow up work.
2. **False** (4-connectivity per the problem statement).
3. **False.** It only means a cycle exists *somewhere*; there may be tails/branches.
4. **False.** Negative edges break the settled-is-final invariant (see counter-example) even with no negative cycle. Use Bellman–Ford.
5. **False.** Use *lazy deletion*: push a new pair, ignore stale ones.
6. **False.** Each undirected edge is a 2-cycle; you need the map.
7. **False.** It's the **maximum** shortest distance.

## Part D answers
* Invariant: "settled ⇒ `dist = δ`"; uses `w ≥ 0` at "`δ(y) ≤ δ(u)`".
* At most `E` pushes + 1 (the source).
* Because the lemma says a DAG always has an in-degree-0 vertex: if peeling ever stalls with vertices left, those vertices contain a cycle.
* `0,1,2,3` or `0,2,1,3` — not unique (1 and 2 are interchangeable).
