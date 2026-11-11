// 09-binary-search  --  stubs for "search on a monotone predicate" problems.
#pragma once

#include <vector>

namespace lc {

// 704  Binary Search.  Context: look up a channel number in a sorted channel plan. Return index or -1.
int search(const std::vector<int>& nums, int target);

// 33   Search in Rotated Sorted Array (distinct values).
//      Context: circular buffer of sorted timestamps that has wrapped; find a sample without un-rotating. Index or -1.
int search_rotated(const std::vector<int>& nums, int target);

// 153  Find Minimum in Rotated Sorted Array (distinct values).
//      Context: find the wrap point (oldest sample) of a ring buffer holding sorted data. Non-empty input.
int find_min(const std::vector<int>& nums);

}  // namespace lc
