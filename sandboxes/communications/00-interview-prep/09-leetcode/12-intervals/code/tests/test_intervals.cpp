#include <gtest/gtest.h>

#include <climits>

#include "intervals.hpp"

using namespace lc;

// ---- 56 --------------------------------------------------------------------
TEST(Interval56, Normal) {
    EXPECT_EQ(merge_intervals({{1, 3}, {2, 6}, {8, 10}, {15, 18}}), (Intervals{{1, 6}, {8, 10}, {15, 18}}));
    EXPECT_EQ(merge_intervals({{1, 4}, {4, 5}}), (Intervals{{1, 5}}));  // touching merges
}
TEST(Interval56, Edges) {
    EXPECT_EQ(merge_intervals({}), (Intervals{}));
    EXPECT_EQ(merge_intervals({{5, 7}}), (Intervals{{5, 7}}));
    EXPECT_EQ(merge_intervals({{8, 10}, {1, 3}, {2, 6}}), (Intervals{{1, 6}, {8, 10}}));  // unsorted input
    EXPECT_EQ(merge_intervals({{1, 10}, {2, 3}, {4, 5}}), (Intervals{{1, 10}}));          // nested: keep the max end
    EXPECT_EQ(merge_intervals({{1, 4}, {0, 0}}), (Intervals{{0, 0}, {1, 4}}));
    EXPECT_EQ(merge_intervals({{2, 3}, {2, 3}}), (Intervals{{2, 3}}));                    // exact duplicates
    EXPECT_EQ(merge_intervals({{INT_MIN, INT_MAX}, {0, 1}}), (Intervals{{INT_MIN, INT_MAX}}));
}

// ---- 57 --------------------------------------------------------------------
TEST(Interval57, Normal) {
    EXPECT_EQ(insert_interval({{1, 3}, {6, 9}}, {2, 5}), (Intervals{{1, 5}, {6, 9}}));
    EXPECT_EQ(insert_interval({{1, 2}, {3, 5}, {6, 7}, {8, 10}, {12, 16}}, {4, 8}), (Intervals{{1, 2}, {3, 10}, {12, 16}}));
}
TEST(Interval57, Edges) {
    EXPECT_EQ(insert_interval({}, {5, 7}), (Intervals{{5, 7}}));
    EXPECT_EQ(insert_interval({{1, 5}}, {2, 3}), (Intervals{{1, 5}}));                      // swallowed
    EXPECT_EQ(insert_interval({{1, 5}}, {6, 8}), (Intervals{{1, 5}, {6, 8}}));              // after, no touch
    EXPECT_EQ(insert_interval({{3, 5}}, {0, 1}), (Intervals{{0, 1}, {3, 5}}));              // before
    EXPECT_EQ(insert_interval({{1, 5}}, {5, 7}), (Intervals{{1, 7}}));                      // touching end merges
    EXPECT_EQ(insert_interval({{1, 5}}, {0, 1}), (Intervals{{0, 5}}));                      // touching start merges
    EXPECT_EQ(insert_interval({{1, 2}, {3, 4}, {5, 6}}, {0, 10}), (Intervals{{0, 10}}));    // swallows all
    EXPECT_EQ(insert_interval({{1, 2}, {5, 6}}, {3, 4}), (Intervals{{1, 2}, {3, 4}, {5, 6}}));  // fits in a gap
}

// ---- 435 -------------------------------------------------------------------
TEST(Interval435, Normal) {
    EXPECT_EQ(erase_overlap_intervals({{1, 2}, {2, 3}, {3, 4}, {1, 3}}), 1);
    EXPECT_EQ(erase_overlap_intervals({{1, 2}, {1, 2}, {1, 2}}), 2);
    EXPECT_EQ(erase_overlap_intervals({{1, 2}, {2, 3}}), 0);  // touching is fine
}
TEST(Interval435, Edges) {
    EXPECT_EQ(erase_overlap_intervals({}), 0);
    EXPECT_EQ(erase_overlap_intervals({{1, 100}}), 0);
    EXPECT_EQ(erase_overlap_intervals({{1, 100}, {11, 22}, {1, 11}, {2, 12}}), 2);
    EXPECT_EQ(erase_overlap_intervals({{0, 2}, {1, 3}, {2, 4}, {3, 5}, {4, 6}}), 2);
    // the classic greedy trap: sorting by START would keep the long one and lose two short ones
    EXPECT_EQ(erase_overlap_intervals({{0, 100}, {1, 2}, {3, 4}}), 1);
    EXPECT_EQ(erase_overlap_intervals({{INT_MIN, INT_MAX}, {0, 1}}), 1);
}
