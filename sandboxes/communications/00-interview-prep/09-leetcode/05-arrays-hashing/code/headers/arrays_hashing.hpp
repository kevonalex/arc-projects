// 05-arrays-hashing  --  stubs for the "trade memory for time / reuse the input as the table" pack.
#pragma once

#include <string>
#include <vector>

namespace lc {

// 1    Two Sum.                   Context: find two TBS entries whose sizes fill a transport block exactly.
// Return the two indices (any order). Exactly one solution is guaranteed.
std::vector<int> two_sum(const std::vector<int>& nums, int target);

// 49   Group Anagrams.            Context: group frames that are permutations of the same symbol multiset.
// Order of groups and of strings inside a group is unspecified.
std::vector<std::vector<std::string>> group_anagrams(const std::vector<std::string>& strs);

// 238  Product of Array Except Self.  Context: leave-one-out gain products without division (zeros / dB-linear conversion).
std::vector<int> product_except_self(const std::vector<int>& nums);

// 128  Longest Consecutive Sequence.  Context: longest unbroken run of received sequence numbers in an unordered batch.
int longest_consecutive(const std::vector<int>& nums);

// 41   First Missing Positive.    Context: smallest free positive id (RNTI / handle) in a table, O(1) extra space.
// Mutates `nums` (allowed; it is the working table).
int first_missing_positive(std::vector<int>& nums);

}  // namespace lc
