#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <vector>

#include "heaps.hpp"

using namespace lc;
using V = std::vector<int>;

// ---- 215 -------------------------------------------------------------------
TEST(Heap215, Normal) {
    EXPECT_EQ(find_kth_largest({3, 2, 1, 5, 6, 4}, 2), 5);
    EXPECT_EQ(find_kth_largest({3, 2, 3, 1, 2, 4, 5, 5, 6}, 4), 4);
}
TEST(Heap215, Edges) {
    EXPECT_EQ(find_kth_largest({1}, 1), 1);
    EXPECT_EQ(find_kth_largest({5, 5, 5}, 3), 5);      // duplicates count separately
    EXPECT_EQ(find_kth_largest({2, 1}, 2), 1);         // k == n -> minimum
    EXPECT_EQ(find_kth_largest({2, 1}, 1), 2);         // k == 1 -> maximum
    EXPECT_EQ(find_kth_largest({INT_MIN, INT_MAX, 0}, 2), 0);
    EXPECT_EQ(find_kth_largest({-1, -2, -3}, 2), -2);
}
TEST(Heap215, MatchesSort) {
    V v;
    unsigned s = 99;
    for (int i = 0; i < 300; ++i) { s = s * 1664525u + 1013904223u; v.push_back(static_cast<int>(s >> 20) - 2048); }
    V sorted = v;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int k : {1, 2, 10, 150, 300}) EXPECT_EQ(find_kth_largest(v, k), sorted[static_cast<std::size_t>(k - 1)]);
}

// ---- 23 --------------------------------------------------------------------
TEST(Heap23, Normal) {
    ListArena a;
    std::vector<ListNode*> lists{a.build({1, 4, 5}), a.build({1, 3, 4}), a.build({2, 6})};
    EXPECT_EQ(to_vector(merge_k_lists(lists)), (V{1, 1, 2, 3, 4, 4, 5, 6}));
}
TEST(Heap23, Edges) {
    ListArena a;
    EXPECT_EQ(merge_k_lists({}), nullptr);
    EXPECT_EQ(merge_k_lists({nullptr}), nullptr);
    EXPECT_EQ(merge_k_lists({nullptr, nullptr}), nullptr);
    EXPECT_EQ(to_vector(merge_k_lists({nullptr, a.build({1})})), (V{1}));
    EXPECT_EQ(to_vector(merge_k_lists({a.build({1, 2, 3})})), (V{1, 2, 3}));
    EXPECT_EQ(to_vector(merge_k_lists({a.build({-5}), a.build({-5}), a.build({-5})})), (V{-5, -5, -5}));
    // very unequal lengths
    EXPECT_EQ(to_vector(merge_k_lists({a.build({1, 2, 3, 4, 5, 6}), a.build({0})})), (V{0, 1, 2, 3, 4, 5, 6}));
}

// ---- 295 -------------------------------------------------------------------
TEST(Heap295, LeetCodeExample) {
    MedianFinder m;
    m.addNum(1);
    m.addNum(2);
    EXPECT_DOUBLE_EQ(m.findMedian(), 1.5);
    m.addNum(3);
    EXPECT_DOUBLE_EQ(m.findMedian(), 2.0);
}
TEST(Heap295, SingleAndNegatives) {
    MedianFinder m;
    m.addNum(-1);
    EXPECT_DOUBLE_EQ(m.findMedian(), -1.0);
    m.addNum(-2);
    EXPECT_DOUBLE_EQ(m.findMedian(), -1.5);
    m.addNum(INT_MAX);
    m.addNum(INT_MAX);
    EXPECT_DOUBLE_EQ(m.findMedian(), (-1.0 + INT_MAX) / 2.0);  // no int overflow when averaging
}
TEST(Heap295, MatchesSortedMedian) {
    MedianFinder m;
    V seen;
    unsigned s = 5;
    for (int i = 0; i < 200; ++i) {
        s = s * 1103515245u + 12345u;
        const int x = static_cast<int>((s >> 16) % 101) - 50;
        m.addNum(x);
        seen.push_back(x);
        V sorted = seen;
        std::sort(sorted.begin(), sorted.end());
        const std::size_t n = sorted.size();
        const double expect = (n % 2 == 1) ? sorted[n / 2] : (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
        ASSERT_DOUBLE_EQ(m.findMedian(), expect) << "after " << n << " samples";
    }
}
