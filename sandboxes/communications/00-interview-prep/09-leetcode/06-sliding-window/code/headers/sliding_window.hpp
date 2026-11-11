// 06-sliding-window  --  stubs for variable-width windows and the two-pointer cousin (11).
#pragma once

#include <string>
#include <vector>

namespace lc {

// 3    Longest Substring Without Repeating Characters.
//      Context: longest burst of distinct sync symbols before a repeat (preamble uniqueness).
int length_of_longest_substring(const std::string& s);

// 424  Longest Repeating Character Replacement.
//      Context: longest run that becomes constant after <= k bit/symbol corrections.  s is uppercase A-Z.
int character_replacement(const std::string& s, int k);

// 76   Minimum Window Substring.
//      Context: shortest stretch of a stream that contains every required marker (with multiplicity).
// Returns "" if impossible.
std::string min_window(const std::string& s, const std::string& t);

// 11   Container With Most Water.   (two pointers, a sibling of sliding window)
//      Context: widest-and-highest pair of antenna masts / the best link-budget pair.
int max_area(const std::vector<int>& height);

}  // namespace lc
