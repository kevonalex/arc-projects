// 11-graphs  --  stubs for traversal, topological order, shortest paths and cloning.
//
// OWNERSHIP RULE: NodeArena owns every Node. clone_graph therefore takes the arena that will own
// the copy (deliberate deviation from LeetCode's signature).
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

namespace lc {

struct Node {
    int val;
    std::vector<Node*> neighbors;
    explicit Node(int v = 0) : val(v) {}
};

class NodeArena {
public:
    Node* make(int v) {
        nodes_.push_back(std::make_unique<Node>(v));
        return nodes_.back().get();
    }
    // LeetCode format: adj[i] lists the neighbours of node (i+1), values are 1-based.
    // Returns node 1, or nullptr for an empty graph.
    Node* build_adj(const std::vector<std::vector<int>>& adj) {
        if (adj.empty()) return nullptr;
        std::vector<Node*> by_val;
        for (std::size_t i = 0; i < adj.size(); ++i) by_val.push_back(make(static_cast<int>(i) + 1));
        for (std::size_t i = 0; i < adj.size(); ++i)
            for (int nb : adj[i]) by_val[i]->neighbors.push_back(by_val[static_cast<std::size_t>(nb) - 1]);
        return by_val[0];
    }

private:
    std::vector<std::unique_ptr<Node>> nodes_;
};

// Inverse of build_adj for everything reachable from `start` (values must be 1..n, n = max value seen).
inline std::vector<std::vector<int>> to_adj(const Node* start) {
    std::vector<std::vector<int>> adj;
    if (start == nullptr) return adj;
    std::unordered_set<const Node*> seen{start};
    std::queue<const Node*> q;
    q.push(start);
    while (!q.empty()) {
        const Node* n = q.front();
        q.pop();
        if (adj.size() < static_cast<std::size_t>(n->val)) adj.resize(static_cast<std::size_t>(n->val));
        for (const Node* nb : n->neighbors) {
            adj[static_cast<std::size_t>(n->val) - 1].push_back(nb->val);
            if (seen.insert(nb).second) q.push(nb);
        }
    }
    return adj;
}

// 200  Number of Islands.  Context: count disjoint coverage blobs on a cell-grid map ('1' = covered).
int num_islands(const std::vector<std::vector<char>>& grid);

// 207  Course Schedule.  Context: can a set of "A must be initialised before B" module dependencies be ordered?
// prerequisites[i] = {a, b} means b must come before a.
bool can_finish(int num_courses, const std::vector<std::vector<int>>& prerequisites);

// 743  Network Delay Time.  Context: time for a signal broadcast from node k to reach every node; -1 if some are unreachable.
// times[i] = {u, v, w}: directed link u -> v takes w. Nodes are 1..n.
int network_delay_time(const std::vector<std::vector<int>>& times, int n, int k);

// 133  Clone Graph.  Context: snapshot a neighbour table (e.g. radio-mesh topology) to mutate without disturbing the original.
Node* clone_graph(const Node* node, NodeArena& arena);

}  // namespace lc
