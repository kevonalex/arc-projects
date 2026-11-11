#include "graphs.hpp"

#include <climits>
#include <functional>
#include <unordered_map>
#include <utility>

namespace lc {

// 200 -- BFS flood fill from every unvisited land cell. Each cell enters the queue at most once: O(R*C).
// BFS (not recursive DFS) so a 300x300 all-land grid cannot overflow the stack.
int num_islands(const std::vector<std::vector<char>>& grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const int rows = static_cast<int>(grid.size());
    const int cols = static_cast<int>(grid[0].size());
    std::vector<std::vector<char>> seen(static_cast<std::size_t>(rows), std::vector<char>(static_cast<std::size_t>(cols), 0));
    static constexpr int dr[4] = {1, -1, 0, 0};
    static constexpr int dc[4] = {0, 0, 1, -1};
    int islands = 0;
    for (int r0 = 0; r0 < rows; ++r0) {
        for (int c0 = 0; c0 < cols; ++c0) {
            if (grid[static_cast<std::size_t>(r0)][static_cast<std::size_t>(c0)] != '1' ||
                seen[static_cast<std::size_t>(r0)][static_cast<std::size_t>(c0)])
                continue;
            ++islands;
            std::queue<std::pair<int, int>> q;
            q.emplace(r0, c0);
            seen[static_cast<std::size_t>(r0)][static_cast<std::size_t>(c0)] = 1;  // mark on PUSH, not pop
            while (!q.empty()) {
                const auto [r, c] = q.front();
                q.pop();
                for (int d = 0; d < 4; ++d) {
                    const int nr = r + dr[d], nc = c + dc[d];
                    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                    if (grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] != '1' ||
                        seen[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)])
                        continue;
                    seen[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] = 1;
                    q.emplace(nr, nc);
                }
            }
        }
    }
    return islands;
}

// 207 -- Kahn's algorithm. A DAG always has a node of in-degree 0; peel them off.
// If we peel all n nodes there was no cycle; any leftover node sits on / behind a cycle.
bool can_finish(int num_courses, const std::vector<std::vector<int>>& prerequisites) {
    const auto n = static_cast<std::size_t>(num_courses);
    std::vector<std::vector<int>> out(n);
    std::vector<int> indeg(n, 0);
    for (const auto& e : prerequisites) {
        out[static_cast<std::size_t>(e[1])].push_back(e[0]);  // b -> a
        ++indeg[static_cast<std::size_t>(e[0])];
    }
    std::queue<int> ready;
    for (std::size_t i = 0; i < n; ++i)
        if (indeg[i] == 0) ready.push(static_cast<int>(i));
    int done = 0;
    while (!ready.empty()) {
        const int u = ready.front();
        ready.pop();
        ++done;
        for (int v : out[static_cast<std::size_t>(u)])
            if (--indeg[static_cast<std::size_t>(v)] == 0) ready.push(v);
    }
    return done == num_courses;
}

// 743 -- Dijkstra with a lazy-deletion min-heap. Safe because all weights are >= 0:
// the closest unsettled node cannot be improved by any path through other unsettled nodes.
int network_delay_time(const std::vector<std::vector<int>>& times, int n, int k) {
    std::vector<std::vector<std::pair<int, int>>> adj(static_cast<std::size_t>(n) + 1);
    for (const auto& t : times) adj[static_cast<std::size_t>(t[0])].emplace_back(t[1], t[2]);
    std::vector<long long> dist(static_cast<std::size_t>(n) + 1, LLONG_MAX);
    using Item = std::pair<long long, int>;  // (distance, node)
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    dist[static_cast<std::size_t>(k)] = 0;
    pq.emplace(0, k);
    while (!pq.empty()) {
        const auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[static_cast<std::size_t>(u)]) continue;  // stale entry
        for (const auto& [v, w] : adj[static_cast<std::size_t>(u)]) {
            if (d + w < dist[static_cast<std::size_t>(v)]) {
                dist[static_cast<std::size_t>(v)] = d + w;
                pq.emplace(d + w, v);
            }
        }
    }
    long long worst = 0;
    for (int v = 1; v <= n; ++v) {
        if (dist[static_cast<std::size_t>(v)] == LLONG_MAX) return -1;
        worst = std::max(worst, dist[static_cast<std::size_t>(v)]);
    }
    return static_cast<int>(worst);
}

// 133 -- map original -> copy. Create the copy of a node BEFORE exploring its neighbours, so cycles
// find an existing copy instead of recursing forever.
Node* clone_graph(const Node* node, NodeArena& arena) {
    if (node == nullptr) return nullptr;
    std::unordered_map<const Node*, Node*> copy;
    copy[node] = arena.make(node->val);
    std::queue<const Node*> q;
    q.push(node);
    while (!q.empty()) {
        const Node* u = q.front();
        q.pop();
        for (const Node* v : u->neighbors) {
            if (copy.find(v) == copy.end()) {
                copy[v] = arena.make(v->val);
                q.push(v);
            }
            copy[u]->neighbors.push_back(copy[v]);
        }
    }
    return copy[node];
}

}  // namespace lc
