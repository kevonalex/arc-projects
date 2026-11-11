// 13-dp  --  stubs for 1-D / 2-D dynamic programming.
//
// Every problem here is "shortest/longest/count paths through a DAG of states" -- the same
// shape as the Viterbi trellis in 02-bits/06-conv-viterbi (see worksheet.md Part A).
#pragma once

#include <string>
#include <vector>

namespace lc {

// 70   Climbing Stairs.  Context: number of distinct ways to cover n slots with 1- and 2-slot grants.
// 1 <= n <= 45 fits int.
int climb_stairs(int n);

// 198  House Robber.  Context: pick max-value transmit slots with no two adjacent (guard-slot constraint).
int rob(const std::vector<int>& nums);

// 322  Coin Change.  Context: fewest PRBs / grants whose sizes add up to exactly `amount`. Return -1 if impossible.
int coin_change(const std::vector<int>& coins, int amount);

// 300  Longest Increasing Subsequence.  Context: longest in-order run of sequence numbers inside a reordered stream.
int length_of_lis(const std::vector<int>& nums);

// 1143 Longest Common Subsequence.  Context: align a received symbol stream against the expected one (edit-distance family).
int longest_common_subsequence(const std::string& a, const std::string& b);

}  // namespace lc
