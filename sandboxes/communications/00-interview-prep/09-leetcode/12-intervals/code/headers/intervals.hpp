// 12-intervals  --  stubs for sort-then-sweep problems.
//
// Intervals are closed [start, end]; touching intervals ([1,2],[2,3]) DO overlap for 56 / 57 and
// do NOT conflict for 435 (as on LeetCode). State this in "Clarify" every time.
#pragma once

#include <vector>

namespace lc {

using Intervals = std::vector<std::vector<int>>;  // each element is {start, end}

// 56   Merge Intervals.  Context: coalesce overlapping reserved time slots / spectrum blocks.
Intervals merge_intervals(Intervals intervals);

// 57   Insert Interval.  Context: add one new grant to an already sorted, non-overlapping grant table.
Intervals insert_interval(const Intervals& sorted_disjoint, std::vector<int> new_interval);

// 435  Non-overlapping Intervals.  Context: minimum number of transmissions to cancel so the rest never collide.
int erase_overlap_intervals(Intervals intervals);

}  // namespace lc
