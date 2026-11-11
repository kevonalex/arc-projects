#pragma once
// SOLUTION - worksheet 01, Q4.
#include <bitset>
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p1 {

inline constexpr std::uint64_t kFrameSync48 = 0x5575F5FF77FFULL;  // VERIFY
inline constexpr std::uint64_t kMask48 = (1ULL << 48) - 1;

inline double binom(int n, int k) {
  double c = 1.0;
  for (int i = 1; i <= k; ++i) c = c * (n - k + i) / i;
  return c;
}

inline int hamming48(std::uint64_t a, std::uint64_t b) {
  // Mask FIRST: bits above 47 belong to older samples in a sliding window, not to the sync.
  return static_cast<int>(std::bitset<64>((a ^ b) & kMask48).count());
}

inline bool sync_matches(std::uint64_t window, int max_bit_errors) {
  return hamming48(window, kFrameSync48) <= max_bit_errors;
}

inline double false_sync_probability(int max_bit_errors) {
  double favourable = 0.0;
  for (int k = 0; k <= max_bit_errors && k <= 48; ++k) favourable += binom(48, k);
  return favourable / static_cast<double>(1ULL << 48);
}

inline double seconds_between_false_syncs(int max_bit_errors,
                                          double positions_per_second = 4800.0) {
  const double p = false_sync_probability(max_bit_errors);
  return 1.0 / (p * positions_per_second);
}

}  // namespace p25p1
