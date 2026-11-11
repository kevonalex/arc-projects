#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <set>
#include <string>
#include <vector>

#include "arrays_hashing.hpp"

using namespace lc;

// ---- 1 ---------------------------------------------------------------------
static void expect_pair(std::vector<int> idx, const std::vector<int>& nums, int target) {
    ASSERT_EQ(idx.size(), 2u);
    EXPECT_NE(idx[0], idx[1]);
    EXPECT_EQ(static_cast<long long>(nums[static_cast<std::size_t>(idx[0])]) + nums[static_cast<std::size_t>(idx[1])], target);
}
TEST(Hash1, Normal) {
    expect_pair(two_sum({2, 7, 11, 15}, 9), {2, 7, 11, 15}, 9);
    expect_pair(two_sum({3, 2, 4}, 6), {3, 2, 4}, 6);
}
TEST(Hash1, Edges) {
    expect_pair(two_sum({3, 3}, 6), {3, 3}, 6);               // duplicate values
    expect_pair(two_sum({-1, -2, -3, -4, -5}, -8), {-1, -2, -3, -4, -5}, -8);
    expect_pair(two_sum({0, 4, 3, 0}, 0), {0, 4, 3, 0}, 0);
    expect_pair(two_sum({INT_MAX, INT_MIN, 5}, -1), {INT_MAX, INT_MIN, 5}, -1);  // target - x overflows int
}

// ---- 49 --------------------------------------------------------------------
static std::set<std::multiset<std::string>> canon(const std::vector<std::vector<std::string>>& g) {
    std::set<std::multiset<std::string>> s;
    for (const auto& grp : g) s.insert(std::multiset<std::string>(grp.begin(), grp.end()));
    return s;
}
TEST(Hash49, Normal) {
    const auto got = group_anagrams({"eat", "tea", "tan", "ate", "nat", "bat"});
    EXPECT_EQ(got.size(), 3u);
    EXPECT_EQ(canon(got), (std::set<std::multiset<std::string>>{{"eat", "tea", "ate"}, {"tan", "nat"}, {"bat"}}));
}
TEST(Hash49, Edges) {
    EXPECT_EQ(canon(group_anagrams({""})), (std::set<std::multiset<std::string>>{{""}}));
    EXPECT_EQ(canon(group_anagrams({"a"})), (std::set<std::multiset<std::string>>{{"a"}}));
    EXPECT_EQ(canon(group_anagrams({})).size(), 0u);
    // duplicates stay together, near-misses stay apart
    const auto got = group_anagrams({"aab", "aba", "abb", "aab"});
    EXPECT_EQ(got.size(), 2u);
}

// ---- 238 -------------------------------------------------------------------
TEST(Hash238, Normal) {
    EXPECT_EQ(product_except_self({1, 2, 3, 4}), (std::vector<int>{24, 12, 8, 6}));
}
TEST(Hash238, Zeros) {
    EXPECT_EQ(product_except_self({-1, 1, 0, -3, 3}), (std::vector<int>{0, 0, 9, 0, 0}));
    EXPECT_EQ(product_except_self({0, 0, 5}), (std::vector<int>{0, 0, 0}));
    EXPECT_EQ(product_except_self({0, 4, 5}), (std::vector<int>{20, 0, 0}));
    EXPECT_EQ(product_except_self({2, 3}), (std::vector<int>{3, 2}));
}

// ---- 128 -------------------------------------------------------------------
TEST(Hash128, Normal) {
    EXPECT_EQ(longest_consecutive({100, 4, 200, 1, 3, 2}), 4);
    EXPECT_EQ(longest_consecutive({0, 3, 7, 2, 5, 8, 4, 6, 0, 1}), 9);
}
TEST(Hash128, Edges) {
    EXPECT_EQ(longest_consecutive({}), 0);
    EXPECT_EQ(longest_consecutive({5}), 1);
    EXPECT_EQ(longest_consecutive({1, 1, 1}), 1);
    EXPECT_EQ(longest_consecutive({-2, -1, 0, 1}), 4);
    EXPECT_EQ(longest_consecutive({INT_MAX - 1, INT_MAX, INT_MIN, INT_MIN + 1}), 2);  // no overflow at the ends
}

// ---- 41 --------------------------------------------------------------------
static int fmp(std::vector<int> v) { return first_missing_positive(v); }
TEST(Hash41, Normal) {
    EXPECT_EQ(fmp({1, 2, 0}), 3);
    EXPECT_EQ(fmp({3, 4, -1, 1}), 2);
    EXPECT_EQ(fmp({7, 8, 9, 11, 12}), 1);
}
TEST(Hash41, Edges) {
    EXPECT_EQ(fmp({1}), 2);
    EXPECT_EQ(fmp({2}), 1);
    EXPECT_EQ(fmp({1, 1}), 2);                 // duplicates would loop forever without the != guard
    EXPECT_EQ(fmp({2, 2, 2}), 1);
    EXPECT_EQ(fmp({INT_MAX, -1, 1}), 2);
    EXPECT_EQ(fmp({}), 1);
    EXPECT_EQ(fmp({1, 2, 3, 4, 5}), 6);
}
