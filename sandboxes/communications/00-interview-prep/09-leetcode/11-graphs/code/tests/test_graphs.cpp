#include <gtest/gtest.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "graphs.hpp"

using namespace lc;
using Grid = std::vector<std::vector<char>>;
using Adj = std::vector<std::vector<int>>;

// ---- 200 -------------------------------------------------------------------
TEST(Graph200, Normal) {
    EXPECT_EQ(num_islands(Grid{{'1', '1', '1', '1', '0'}, {'1', '1', '0', '1', '0'}, {'1', '1', '0', '0', '0'}, {'0', '0', '0', '0', '0'}}), 1);
    EXPECT_EQ(num_islands(Grid{{'1', '1', '0', '0', '0'}, {'1', '1', '0', '0', '0'}, {'0', '0', '1', '0', '0'}, {'0', '0', '0', '1', '1'}}), 3);
}
TEST(Graph200, Edges) {
    EXPECT_EQ(num_islands(Grid{}), 0);
    EXPECT_EQ(num_islands(Grid{{'0'}}), 0);
    EXPECT_EQ(num_islands(Grid{{'1'}}), 1);
    EXPECT_EQ(num_islands(Grid{{'1', '0', '1'}, {'0', '1', '0'}, {'1', '0', '1'}}), 5);  // diagonals do NOT connect
    EXPECT_EQ(num_islands(Grid(300, std::vector<char>(300, '1'))), 1);                   // deep flood, no recursion
}

// ---- 207 -------------------------------------------------------------------
TEST(Graph207, Normal) {
    EXPECT_TRUE(can_finish(2, {{1, 0}}));
    EXPECT_FALSE(can_finish(2, {{1, 0}, {0, 1}}));
}
TEST(Graph207, Edges) {
    EXPECT_TRUE(can_finish(1, {}));
    EXPECT_TRUE(can_finish(5, {}));                                 // no prerequisites at all
    EXPECT_FALSE(can_finish(1, {{0, 0}}));                          // self loop
    EXPECT_FALSE(can_finish(4, {{1, 0}, {2, 1}, {3, 2}, {1, 3}}));  // cycle hidden behind a tail
    EXPECT_TRUE(can_finish(4, {{1, 0}, {2, 0}, {3, 1}, {3, 2}}));   // diamond is fine
    EXPECT_TRUE(can_finish(3, {{1, 0}, {1, 0}}));                   // duplicate edge
}

// ---- 743 -------------------------------------------------------------------
TEST(Graph743, Normal) {
    EXPECT_EQ(network_delay_time({{2, 1, 1}, {2, 3, 1}, {3, 4, 1}}, 4, 2), 2);
    EXPECT_EQ(network_delay_time({{1, 2, 1}}, 2, 1), 1);
}
TEST(Graph743, Edges) {
    EXPECT_EQ(network_delay_time({{1, 2, 1}}, 2, 2), -1);   // k cannot reach node 1
    EXPECT_EQ(network_delay_time({}, 1, 1), 0);             // single node
    // shorter multi-hop path beats the direct heavy edge
    EXPECT_EQ(network_delay_time({{1, 2, 10}, {1, 3, 1}, {3, 2, 1}}, 3, 1), 2);
    // zero-weight edges are allowed
    EXPECT_EQ(network_delay_time({{1, 2, 0}, {2, 3, 0}}, 3, 1), 0);
    // cycle in graph
    EXPECT_EQ(network_delay_time({{1, 2, 1}, {2, 1, 1}, {2, 3, 4}}, 3, 1), 5);
}

// ---- 133 -------------------------------------------------------------------
static void collect(const Node* start, std::unordered_set<const Node*>& out) {
    std::vector<const Node*> st{start};
    while (!st.empty()) {
        const Node* n = st.back();
        st.pop_back();
        if (!out.insert(n).second) continue;
        for (const Node* nb : n->neighbors) st.push_back(nb);
    }
}
TEST(Graph133, Normal) {
    NodeArena orig, cp;
    const Adj adj{{2, 4}, {1, 3}, {2, 4}, {1, 3}};
    Node* g = orig.build_adj(adj);
    Node* c = clone_graph(g, cp);
    EXPECT_EQ(to_adj(c), adj);
    std::unordered_set<const Node*> a, b;
    collect(g, a);
    collect(c, b);
    for (const Node* n : b) EXPECT_EQ(a.count(n), 0u) << "copy shares a node with the original";
    EXPECT_EQ(a.size(), b.size());
}
TEST(Graph133, Edges) {
    NodeArena orig, cp;
    EXPECT_EQ(clone_graph(nullptr, cp), nullptr);
    Node* single = orig.build_adj(Adj{{}});
    Node* c1 = clone_graph(single, cp);
    ASSERT_NE(c1, nullptr);
    EXPECT_NE(c1, single);
    EXPECT_TRUE(c1->neighbors.empty());
    Node* self = orig.make(1);
    self->neighbors.push_back(self);  // self loop
    Node* c2 = clone_graph(self, cp);
    ASSERT_EQ(c2->neighbors.size(), 1u);
    EXPECT_EQ(c2->neighbors[0], c2);  // loops onto the COPY, not the original
}
TEST(Graph133, CloneIsIndependent) {
    NodeArena orig, cp;
    Node* g = orig.build_adj(Adj{{2}, {1}});
    Node* c = clone_graph(g, cp);
    c->neighbors.clear();
    EXPECT_EQ(g->neighbors.size(), 1u);  // mutating the clone leaves the original intact
}
