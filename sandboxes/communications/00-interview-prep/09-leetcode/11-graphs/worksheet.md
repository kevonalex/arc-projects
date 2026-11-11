# 11 · Graphs — Pattern Worksheet

**Pattern:** model "things + relationships" as vertices + edges; then pick the traversal that matches the question — **BFS/DFS** for reachability/components, **Kahn (in-degree peeling)** for dependency order, **Dijkstra** for cheapest paths with non-negative costs, **node→clone map** for copying.
**Why it matters at a radio/protocol shop:** cell/neighbour topologies, mesh routing and link costs, module/state initialisation order, coverage-map blobs, signalling dependency graphs.
**Coded:** 200, 207, 743, 133 — `code/headers/graphs.hpp` (`NodeArena` owns nodes; `to_adj` turns a graph back into LeetCode's adjacency list for assertions). Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (directed? weighted? negative weights? cycles/self-loops/duplicates? disconnected? how are nodes numbered?) → 2 **Brute force + complexity** → 3 **Optimise** (choose algorithm; state the invariant; say what "visited" means) → 4 **Code** → 5 **Hand-trace** a normal graph and a degenerate one (empty, one node, cycle, unreachable) → 6 **Final complexity** in `V` and `E`.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the techniques work

### A1. Traversal = reachability, each vertex once
BFS/DFS visits every vertex reachable from the start exactly once if you mark vertices **when you enqueue/push them** (marking on *pop* lets the same vertex be enqueued many times). Each vertex is processed once and each edge examined once ⇒ **O(V + E)**. A **connected component** (or an "island") is the set found by one traversal; the number of components = the number of traversals you have to start (200).
Grid graphs: vertices are cells, edges are the 4 neighbours (diagonals are *not* edges unless specified). Prefer BFS/explicit stack over recursion for big grids (a 300×300 all-land grid is 90 000 deep for recursive DFS).

### A2. Topological order & Kahn's algorithm (207)
A directed graph has a topological order ⇔ it has **no directed cycle**.
* **Lemma:** every non-empty DAG has a vertex of in-degree 0. *Proof:* otherwise start anywhere and keep stepping to a predecessor; after more than `V` steps some vertex repeats ⇒ a cycle.
* **Kahn:** repeatedly remove an in-degree-0 vertex and decrement its successors' in-degrees. Removing a source from a DAG leaves a DAG, so by induction we remove **all** vertices of a DAG.
* **If a cycle exists** none of its vertices ever reaches in-degree 0 (each has a predecessor *on the cycle* that is never removed), so the count of removed vertices `< V`. Therefore *"removed count == V ⇔ acyclic"*. O(V + E).

### A3. Dijkstra and why greedy is safe (743)
Single-source shortest paths when **all edge weights ≥ 0**. Maintain `dist[]` (upper bounds, initially `∞`, `dist[s]=0`) and a **min-heap** of `(dist, vertex)`. Repeatedly extract the vertex `u` with the smallest tentative distance, declare it *settled*, and **relax** its outgoing edges: `dist[v] = min(dist[v], dist[u] + w)`.

> **Invariant (the safety claim):** when `u` is extracted (settled), `dist[u] = δ(u)`, the true shortest distance.

*Proof sketch (by contradiction).* Let `u` be the first vertex settled with `dist[u] > δ(u)`. Take a true shortest path `s → … → u`. Let `y` be the first vertex on it that is **not** settled and `x` its predecessor, which is settled (maybe `x = s`). By minimality of `u`, `dist[x] = δ(x)`, and when `x` was settled the edge `x→y` was relaxed, so `dist[y] ≤ δ(x) + w(x,y) = δ(y)`; also `dist[y] ≥ δ(y)` always (it is the length of a real path), hence `dist[y] = δ(y)`. **Because weights are ≥ 0**, the part of the path from `y` to `u` costs ≥ 0, so `δ(y) ≤ δ(u) < dist[u]`. Then `y` sits in the heap with a key strictly smaller than `dist[u]`, so the heap would have extracted `y` before `u` — contradiction (if `y = u` we get `dist[u] = δ(u)` directly). ∎
**Where non-negativity is essential:** with a negative edge on the tail of the path, `δ(y) ≤ δ(u)` fails, so a vertex can be improved *after* being settled. Example: `s→a (2)`, `s→b (3)`, `b→a (−2)`: textbook Dijkstra settles `a` at 2 and never looks back, but the true `δ(a) = 1`. (A lazy-deletion implementation that happens to re-relax settled vertices would "repair" this example, but it then loses the `O(E log V)` guarantee — it can go exponential. With negative edges use Bellman–Ford.)
**Cost:** with a binary heap and *lazy deletion* (push a new pair whenever `dist` improves, skip stale pops), each edge causes ≤ 1 push ⇒ `O(E log E) = O(E log V)`. Zero-weight edges are fine.
**743 specifics:** the time for a signal sent from `k` to reach everyone is `max_v δ(v)`; if any vertex is unreachable the answer is `−1`.

### A4. Cloning a graph (133)
Need a **bijection original → copy** so that a cycle (or a shared neighbour) never creates two copies of the same node. Keep `unordered_map<const Node*, Node*>`; **create a node's copy at discovery time**, before exploring its neighbours. **Invariant:** every discovered original has exactly one clone, and a node's neighbour list is filled exactly once, when it is dequeued. O(V + E).

---

## Part B — Problems

### P1 · LC 200 Number of Islands — `num_islands`
**Ctx:** count disjoint coverage blobs on a cell-grid (`'1'` = covered). Diagonals don't connect.
1. Clarify: 4- or 8-connectivity (4); may I mutate the grid? (here: no — use a `seen` matrix); empty grid.
2. Brute: for each land cell, flood fill without marking → ____ (what blows up?)
3. Optimise: A1 — mark on push; BFS vs recursive DFS vs union-find.
5. Trace: the 4×5 example giving 3 islands; edges: empty, all water, checkerboard (diagonals), 300×300 land.
6. Final: ____

### P2 · LC 207 Course Schedule — `can_finish`
**Ctx:** can a set of "A must initialise before B" module dependencies be ordered (no deadlock)?
1. Clarify: `prerequisites[i] = {a, b}` means `b → a`; self-loop `{0,0}`? duplicate edges? disconnected nodes.
2. Brute: DFS all paths from each node looking for a repeat → ____
3. Optimise: A2 Kahn. State both lemmas. Compare with DFS 3-colour cycle detection.
5. Trace: `2, [[1,0]]` → true; `2, [[1,0],[0,1]]` → false; diamond; cycle hidden behind a tail.
6. Final: ____

### P3 · LC 743 Network Delay Time — `network_delay_time`
**Ctx:** time for a broadcast from node `k` to reach every radio node over directed links with latencies.
1. Clarify: directed edges `{u,v,w}`, `w ≥ 0`; nodes `1..n`; unreachable ⇒ `-1`; answer = max shortest distance.
2. Brute: Bellman–Ford `O(VE)`, or Floyd–Warshall `O(V³)` → ____
3. Optimise: A3 — **write the invariant and the full proof of why greedy is safe**, and give the negative-edge counter-example.
5. Trace: `times=[[2,1,1],[2,3,1],[3,4,1]], n=4, k=2` → 2 (draw heap states); edges: single node, unreachable, zero-weight edges, a cycle in the graph, a heavy direct edge vs a cheap 2-hop path.
6. Final: ____

### P4 · LC 133 Clone Graph — `clone_graph`
**Ctx:** snapshot a neighbour table (mesh topology) so it can be mutated without disturbing the live one.
1. Clarify: connected undirected graph, values `1..n`; null input; self-loops? **who owns the clone?** (in this pack: the `NodeArena` passed in)
2. Brute: recursive copy without a map → ____ (infinite on a cycle)
3. Optimise: A4. BFS or DFS; when exactly do you create the copy?
5. Trace: the 4-cycle `[[2,4],[1,3],[2,4],[1,3]]`; edges: null, single node, self-loop (the copy must loop onto the **copy**), mutation independence.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "Mark a node visited when you pop it from the BFS queue."
2. "Diagonal neighbours are connected in Number of Islands."
3. "A graph with no in-degree-0 node must be a single big cycle."
4. "Dijkstra works with negative edges as long as there are no negative cycles."
5. "Dijkstra needs a `decrease-key`; `std::priority_queue` can't do it."
6. "Cloning a graph by recursion needs no visited structure if the graph is undirected."
7. "The answer to Network Delay Time is the sum of all shortest distances."

## Part D — Self-check
* State the Dijkstra invariant and point at the line in the proof that uses `w ≥ 0`.
* What is the total number of heap pushes in lazy-deletion Dijkstra, worst case?
* Why does Kahn's algorithm detect cycles *without* ever searching for one?
* Give a topological order for the diamond `0→1, 0→2, 1→3, 2→3`; is it unique?
