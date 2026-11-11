#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <numeric>
#include <vector>

#include "binary_search.hpp"

using namespace lc;
using V = std::vector<int>;

// ---- 704 -------------------------------------------------------------------
TEST(BSearch704, Normal) {
    EXPECT_EQ(search({-1, 0, 3, 5, 9, 12}, 9), 4);
    EXPECT_EQ(search({-1, 0, 3, 5, 9, 12}, 2), -1);
}
TEST(BSearch704, Edges) {
    EXPECT_EQ(search({}, 1), -1);
    EXPECT_EQ(search({5}, 5), 0);
    EXPECT_EQ(search({5}, 4), -1);
    EXPECT_EQ(search({1, 3}, 1), 0);
    EXPECT_EQ(search({1, 3}, 3), 1);
    EXPECT_EQ(search({1, 3}, 0), -1);   // below range
    EXPECT_EQ(search({1, 3}, 4), -1);   // above range
    EXPECT_EQ(search({INT_MIN, 0, INT_MAX}, INT_MAX), 2);  // lo+hi would overflow with a naive mid
}

// ---- 33 --------------------------------------------------------------------
TEST(BSearch33, Normal) {
    EXPECT_EQ(search_rotated({4, 5, 6, 7, 0, 1, 2}, 0), 4);
    EXPECT_EQ(search_rotated({4, 5, 6, 7, 0, 1, 2}, 3), -1);
}
TEST(BSearch33, Edges) {
    EXPECT_EQ(search_rotated({1}, 0), -1);
    EXPECT_EQ(search_rotated({1}, 1), 0);
    EXPECT_EQ(search_rotated({3, 1}, 1), 1);       // lo == mid case
    EXPECT_EQ(search_rotated({3, 1}, 3), 0);
    EXPECT_EQ(search_rotated({1, 2, 3, 4}, 4), 3); // not rotated
    EXPECT_EQ(search_rotated({}, 4), -1);
}
TEST(BSearch33, ExhaustiveAllRotations) {
    for (int n = 1; n <= 12; ++n) {
        V base(static_cast<std::size_t>(n));
        std::iota(base.begin(), base.end(), 10);  // 10, 11, ...
        for (int r = 0; r < n; ++r) {
            V rot = base;
            std::rotate(rot.begin(), rot.begin() + r, rot.end());
            for (int t = 8; t < 10 + n + 2; ++t) {
                const auto it = std::find(rot.begin(), rot.end(), t);
                const int expect = it == rot.end() ? -1 : static_cast<int>(it - rot.begin());
                ASSERT_EQ(search_rotated(rot, t), expect) << "n=" << n << " r=" << r << " t=" << t;
            }
        }
    }
}

// ---- 153 -------------------------------------------------------------------
TEST(BSearch153, Normal) {
    EXPECT_EQ(find_min({3, 4, 5, 1, 2}), 1);
    EXPECT_EQ(find_min({4, 5, 6, 7, 0, 1, 2}), 0);
    EXPECT_EQ(find_min({11, 13, 15, 17}), 11);
}
TEST(BSearch153, Edges) {
    EXPECT_EQ(find_min({1}), 1);
    EXPECT_EQ(find_min({2, 1}), 1);
    EXPECT_EQ(find_min({1, 2}), 1);
    EXPECT_EQ(find_min({5, 1, 2, 3, 4}), 1);   // minimum at index 1
    EXPECT_EQ(find_min({2, 3, 4, 5, 1}), 1);   // minimum at last index
    EXPECT_EQ(find_min({-3, -2, -1, -5, -4}), -5);
}
TEST(BSearch153, ExhaustiveAllRotations) {
    for (int n = 1; n <= 12; ++n) {
        V base(static_cast<std::size_t>(n));
        std::iota(base.begin(), base.end(), -5);
        for (int r = 0; r < n; ++r) {
            V rot = base;
            std::rotate(rot.begin(), rot.begin() + r, rot.end());
            ASSERT_EQ(find_min(rot), -5) << "n=" << n << " r=" << r;
        }
    }
}
