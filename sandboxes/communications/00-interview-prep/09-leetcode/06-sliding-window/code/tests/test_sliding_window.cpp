#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "sliding_window.hpp"

using namespace lc;

// ---- 3 ---------------------------------------------------------------------
TEST(Window3, Normal) {
    EXPECT_EQ(length_of_longest_substring("abcabcbb"), 3);
    EXPECT_EQ(length_of_longest_substring("bbbbb"), 1);
    EXPECT_EQ(length_of_longest_substring("pwwkew"), 3);
}
TEST(Window3, Edges) {
    EXPECT_EQ(length_of_longest_substring(""), 0);
    EXPECT_EQ(length_of_longest_substring("a"), 1);
    EXPECT_EQ(length_of_longest_substring("abba"), 2);   // l must never move backwards
    EXPECT_EQ(length_of_longest_substring("tmmzuxt"), 5);
    EXPECT_EQ(length_of_longest_substring("dvdf"), 3);
    EXPECT_EQ(length_of_longest_substring(" "), 1);
}

// ---- 424 -------------------------------------------------------------------
TEST(Window424, Normal) {
    EXPECT_EQ(character_replacement("ABAB", 2), 4);
    EXPECT_EQ(character_replacement("AABABBA", 1), 4);
}
TEST(Window424, Edges) {
    EXPECT_EQ(character_replacement("A", 0), 1);
    EXPECT_EQ(character_replacement("ABCDE", 0), 1);
    EXPECT_EQ(character_replacement("ABCDE", 10), 5);
    EXPECT_EQ(character_replacement("", 3), 0);
    EXPECT_EQ(character_replacement("AAAA", 0), 4);
    EXPECT_EQ(character_replacement("ABBB", 2), 4);
}

// ---- 76 --------------------------------------------------------------------
TEST(Window76, Normal) {
    EXPECT_EQ(min_window("ADOBECODEBANC", "ABC"), "BANC");
    EXPECT_EQ(min_window("a", "a"), "a");
}
TEST(Window76, Edges) {
    EXPECT_EQ(min_window("a", "aa"), "");        // multiplicity matters
    EXPECT_EQ(min_window("a", "b"), "");
    EXPECT_EQ(min_window("aa", "aa"), "aa");
    EXPECT_EQ(min_window("abc", ""), "");
    EXPECT_EQ(min_window("ab", "b"), "b");
    EXPECT_EQ(min_window("bba", "ab"), "ba");
    EXPECT_EQ(min_window("cabwefgewcwaefgcf", "cae"), "cwae");
}

// ---- 11 --------------------------------------------------------------------
TEST(Window11, Normal) {
    EXPECT_EQ(max_area({1, 8, 6, 2, 5, 4, 8, 3, 7}), 49);
    EXPECT_EQ(max_area({1, 1}), 1);
}
TEST(Window11, Edges) {
    EXPECT_EQ(max_area({}), 0);
    EXPECT_EQ(max_area({5}), 0);
    EXPECT_EQ(max_area({0, 0, 0}), 0);
    EXPECT_EQ(max_area({4, 3, 2, 1, 4}), 16);
    EXPECT_EQ(max_area({1, 2, 1}), 2);
}
TEST(Window11, MatchesBruteForce) {
    const std::vector<int> h{3, 9, 4, 1, 7, 8, 2, 6, 5, 10, 2, 3};
    int brute = 0;
    for (std::size_t i = 0; i < h.size(); ++i)
        for (std::size_t j = i + 1; j < h.size(); ++j)
            brute = std::max(brute, std::min(h[i], h[j]) * static_cast<int>(j - i));
    EXPECT_EQ(max_area(h), brute);
}
