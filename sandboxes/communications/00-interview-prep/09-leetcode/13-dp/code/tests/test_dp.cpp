#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

#include "dp.hpp"

using namespace lc;
using V = std::vector<int>;

// ---- 70 --------------------------------------------------------------------
TEST(Dp70, Normal) {
    EXPECT_EQ(climb_stairs(2), 2);
    EXPECT_EQ(climb_stairs(3), 3);
    EXPECT_EQ(climb_stairs(5), 8);
}
TEST(Dp70, Edges) {
    EXPECT_EQ(climb_stairs(1), 1);
    EXPECT_EQ(climb_stairs(45), 1836311903);  // largest n that fits in int
}

// ---- 198 -------------------------------------------------------------------
TEST(Dp198, Normal) {
    EXPECT_EQ(rob({1, 2, 3, 1}), 4);
    EXPECT_EQ(rob({2, 7, 9, 3, 1}), 12);
}
TEST(Dp198, Edges) {
    EXPECT_EQ(rob({}), 0);
    EXPECT_EQ(rob({5}), 5);
    EXPECT_EQ(rob({2, 1}), 2);
    EXPECT_EQ(rob({0, 0, 0}), 0);
    EXPECT_EQ(rob({2, 1, 1, 2}), 4);        // skip two in the middle
    EXPECT_EQ(rob({100, 1, 1, 100}), 200);
}

// ---- 322 -------------------------------------------------------------------
TEST(Dp322, Normal) {
    EXPECT_EQ(coin_change({1, 2, 5}, 11), 3);
    EXPECT_EQ(coin_change({2}, 3), -1);
}
TEST(Dp322, Edges) {
    EXPECT_EQ(coin_change({1}, 0), 0);
    EXPECT_EQ(coin_change({2}, 0), 0);
    EXPECT_EQ(coin_change({186, 419, 83, 408}, 6249), 20);
    EXPECT_EQ(coin_change({1, 3, 4}, 6), 2);   // greedy would give 4+1+1 = 3 coins
    EXPECT_EQ(coin_change({5}, 5), 1);
    EXPECT_EQ(coin_change({}, 5), -1);
    EXPECT_EQ(coin_change({2147483647}, 1), -1);
}

// ---- 300 -------------------------------------------------------------------
TEST(Dp300, Normal) {
    EXPECT_EQ(length_of_lis({10, 9, 2, 5, 3, 7, 101, 18}), 4);
    EXPECT_EQ(length_of_lis({0, 1, 0, 3, 2, 3}), 4);
}
TEST(Dp300, Edges) {
    EXPECT_EQ(length_of_lis({}), 0);
    EXPECT_EQ(length_of_lis({7, 7, 7, 7}), 1);      // strictly increasing: equal does not extend
    EXPECT_EQ(length_of_lis({5}), 1);
    EXPECT_EQ(length_of_lis({5, 4, 3, 2, 1}), 1);
    EXPECT_EQ(length_of_lis({1, 2, 3, 4, 5}), 5);
    EXPECT_EQ(length_of_lis({4, 10, 4, 3, 8, 9}), 3);
}
TEST(Dp300, MatchesQuadratic) {
    V v;
    unsigned s = 11;
    for (int i = 0; i < 150; ++i) { s = s * 1103515245u + 12345u; v.push_back(static_cast<int>((s >> 16) % 50)); }
    V dp(v.size(), 1);
    int best = 0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j) if (v[j] < v[i]) dp[i] = std::max(dp[i], dp[j] + 1);
        best = std::max(best, dp[i]);
    }
    EXPECT_EQ(length_of_lis(v), best);
}

// ---- 1143 ------------------------------------------------------------------
TEST(Dp1143, Normal) {
    EXPECT_EQ(longest_common_subsequence("abcde", "ace"), 3);
    EXPECT_EQ(longest_common_subsequence("abc", "abc"), 3);
    EXPECT_EQ(longest_common_subsequence("abc", "def"), 0);
}
TEST(Dp1143, Edges) {
    EXPECT_EQ(longest_common_subsequence("", "abc"), 0);
    EXPECT_EQ(longest_common_subsequence("abc", ""), 0);
    EXPECT_EQ(longest_common_subsequence("", ""), 0);
    EXPECT_EQ(longest_common_subsequence("a", "a"), 1);
    EXPECT_EQ(longest_common_subsequence("aaaa", "aa"), 2);
    EXPECT_EQ(longest_common_subsequence("bsbininm", "jmjkbkjkv"), 1);
    EXPECT_EQ(longest_common_subsequence("abcba", "abcbcba"), 5);
    EXPECT_EQ(longest_common_subsequence("ace", "abcde"), 3);  // symmetric
}
