# 11 · Graphs — Cheatsheet

**Pick the tool:**
| Question | Tool | Cost |
|----------|------|------|
| components / reachability / flood fill | BFS/DFS (mark on **push**) | O(V+E) |
| unweighted shortest path | BFS | O(V+E) |
| dependency order / cycle in digraph | Kahn (in-degree) or DFS 3-colour | O(V+E) |
| shortest path, weights ≥ 0 | Dijkstra (min-heap, lazy deletion) | O((V+E) log V) |
| shortest path, negatives | Bellman–Ford | O(VE) |
| all pairs, small V | Floyd–Warshall | O(V³) |
| copy / mapping | `map<orig, copy>`, create copy at discovery | O(V+E) |
| online connectivity | union-find | ~O(α(n)) |

## Dijkstra (lazy deletion)
```cpp
vector<long long> dist(n+1, INF); dist[s]=0;
priority_queue<pair<ll,int>, vector<pair<ll,int>>, greater<>> pq; pq.push({0,s});
while(!pq.empty()){ auto [d,u]=pq.top(); pq.pop(); if(d>dist[u]) continue;
  for(auto [v,w]: adj[u]) if(d+w<dist[v]){ dist[v]=d+w; pq.push({dist[v],v}); } }
```
**Why greedy is safe:** first settled vertex with `dist>δ` ⇒ true path has an unsettled `y` with `dist[y]=δ(y) ≤ δ(u) < dist[u]` (needs `w ≥ 0`) ⇒ `y` pops first ⇒ contradiction.

## Kahn
```cpp
indeg[]; queue of indeg==0; while q: u=pop; ++done; for v in out[u]: if(--indeg[v]==0) push v;
acyclic <=> done == n
```

## Grid BFS
`dr = {1,-1,0,0}, dc = {0,0,1,-1}`; bounds-check; `seen` on push; 4-connectivity unless told otherwise.

## Gotchas
* Mark visited on **push**.
* Dijkstra: no negative weights; use 64-bit distances; skip stale pops; unreachable ⇒ `INF`.
* 743: answer is `max(dist)`, `-1` if any `INF`.
* 207: edge direction `{a,b}` means `b → a`; self-loops are cycles.
* 133: create the clone *before* exploring neighbours; self-loops; ownership of clones.
* Recursive DFS on big grids overflows the stack.
* Disconnected graphs: loop over all start nodes.

## Script
"Clarify directed/weighted/negative/cycles → brute force explores all paths → choose BFS / Kahn / Dijkstra because ___ → state invariant (visited-on-push / settled=final / in-degree) → code → trace a small graph + degenerate → O(V+E) (log V for Dijkstra)."
