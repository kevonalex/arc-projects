#include <gtest/gtest.h>

#include <climits>
#include <optional>
#include <vector>

#include "trees.hpp"

using namespace lc;
using V2 = std::vector<std::vector<int>>;
constexpr auto N = std::nullopt;

// ---- 104 -------------------------------------------------------------------
TEST(Tree104, Normal) {
    TreeArena a;
    EXPECT_EQ(max_depth(a.build({3, 9, 20, N, N, 15, 7})), 3);
    EXPECT_EQ(max_depth(a.build({1, N, 2})), 2);
}
TEST(Tree104, Edges) {
    TreeArena a;
    EXPECT_EQ(max_depth(nullptr), 0);
    EXPECT_EQ(max_depth(a.build({0})), 1);
    EXPECT_EQ(max_depth(a.build({1, 2, N, 3, N, 4})), 4);  // left-leaning chain
}

// ---- 226 -------------------------------------------------------------------
TEST(Tree226, Normal) {
    TreeArena a;
    EXPECT_EQ(to_level(invert_tree(a.build({4, 2, 7, 1, 3, 6, 9}))), (Level{4, 7, 2, 9, 6, 3, 1}));
    EXPECT_EQ(to_level(invert_tree(a.build({2, 1, 3}))), (Level{2, 3, 1}));
}
TEST(Tree226, Edges) {
    TreeArena a;
    EXPECT_EQ(invert_tree(nullptr), nullptr);
    EXPECT_EQ(to_level(invert_tree(a.build({1}))), (Level{1}));
    EXPECT_EQ(to_level(invert_tree(a.build({1, 2}))), (Level{1, N, 2}));
    TreeNode* t = a.build({1, 2, 3, 4});
    EXPECT_EQ(to_level(invert_tree(invert_tree(t))), (Level{1, 2, 3, 4}));  // involution
}

// ---- 102 -------------------------------------------------------------------
TEST(Tree102, Normal) {
    TreeArena a;
    EXPECT_EQ(level_order(a.build({3, 9, 20, N, N, 15, 7})), (V2{{3}, {9, 20}, {15, 7}}));
}
TEST(Tree102, Edges) {
    TreeArena a;
    EXPECT_EQ(level_order(nullptr), V2{});
    EXPECT_EQ(level_order(a.build({1})), (V2{{1}}));
    EXPECT_EQ(level_order(a.build({1, 2, N, 3})), (V2{{1}, {2}, {3}}));
    EXPECT_EQ(level_order(a.build({1, N, 2, N, 3})), (V2{{1}, {2}, {3}}));
}

// ---- 98 --------------------------------------------------------------------
TEST(Tree98, Normal) {
    TreeArena a;
    EXPECT_TRUE(is_valid_bst(a.build({2, 1, 3})));
    EXPECT_FALSE(is_valid_bst(a.build({5, 1, 4, N, N, 3, 6})));
}
TEST(Tree98, TheClassicTrap) {
    TreeArena a;
    // 3 sits in the RIGHT subtree of 5 but is smaller than 5: parent-only comparison misses it.
    EXPECT_FALSE(is_valid_bst(a.build({5, 4, 6, N, N, 3, 7})));
}
TEST(Tree98, Edges) {
    TreeArena a;
    EXPECT_TRUE(is_valid_bst(nullptr));
    EXPECT_TRUE(is_valid_bst(a.build({1})));
    EXPECT_FALSE(is_valid_bst(a.build({2, 2, 2})));          // duplicates not allowed
    EXPECT_TRUE(is_valid_bst(a.build({INT_MIN})));
    EXPECT_TRUE(is_valid_bst(a.build({INT_MAX})));
    EXPECT_FALSE(is_valid_bst(a.build({INT_MIN, N, INT_MIN})));
    EXPECT_TRUE(is_valid_bst(a.build({0, INT_MIN, INT_MAX})));
}

// ---- 297 -------------------------------------------------------------------
TEST(Tree297, RoundTripNormal) {
    TreeArena a, b;
    TreeNode* t = a.build({1, 2, 3, N, N, 4, 5});
    EXPECT_EQ(to_level(deserialize(serialize(t), b)), (Level{1, 2, 3, N, N, 4, 5}));
}
TEST(Tree297, RoundTripEdges) {
    TreeArena a, b;
    EXPECT_EQ(serialize(nullptr), "#,");
    EXPECT_EQ(deserialize(serialize(nullptr), b), nullptr);
    EXPECT_EQ(to_level(deserialize(serialize(a.build({-7})), b)), (Level{-7}));
    TreeNode* skew = a.build({1, N, 2, N, 3, N, 4});
    EXPECT_EQ(to_level(deserialize(serialize(skew), b)), (Level{1, N, 2, N, 3, N, 4}));
    TreeNode* neg = a.build({INT_MIN, INT_MAX, 0});
    EXPECT_EQ(to_level(deserialize(serialize(neg), b)), (Level{INT_MIN, INT_MAX, 0}));
}
TEST(Tree297, WireFormat) {
    TreeArena a;
    EXPECT_EQ(serialize(a.build({1, 2, 3})), "1,2,#,#,3,#,#,");
}
