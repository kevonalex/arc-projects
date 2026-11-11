#pragma once
// STUB - worksheet 01, Q4 (frame sync) and Q8 (find the bug).
// Implement every function that calls lmr::todo(). Run the tests until green.
// SYNTHETIC FRAMES ONLY - never feed this live public-safety traffic.
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p1 {

// 48-bit P25 Phase 1 frame sync (TIA-102.BAAA). VERIFY exact value against the standard.
inline constexpr std::uint64_t kFrameSync48 = 0x5575F5FF77FFULL;
inline constexpr std::uint64_t kMask48 = (1ULL << 48) - 1;

// GIVEN: C(n,k) as double (exact enough for n <= 64).
inline double binom(int n, int k) {
  double c = 1.0;
  for (int i = 1; i <= k; ++i) c = c * (n - k + i) / i;
  return c;
}

// TODO: number of bit positions in which a and b differ, looking ONLY at the low 48 bits.
inline int hamming48(std::uint64_t /*a*/, std::uint64_t /*b*/) { lmr::todo("hamming48"); }

// TODO: true if the low 48 bits of `window` are within max_bit_errors of kFrameSync48.
inline bool sync_matches(std::uint64_t /*window*/, int /*max_bit_errors*/) {
  lmr::todo("sync_matches");
}

// TODO: probability that 48 RANDOM bits land within max_bit_errors of the sync word.
//       = sum_{k=0..m} C(48,k) / 2^48
inline double false_sync_probability(int /*max_bit_errors*/) {
  lmr::todo("false_sync_probability");
}

// TODO: mean seconds between false sync detections on pure noise when the receiver tests
//       one window per symbol (positions_per_second = 4800 for P25 Phase 1).
inline double seconds_between_false_syncs(int /*max_bit_errors*/,
                                          double /*positions_per_second*/ = 4800.0) {
  lmr::todo("seconds_between_false_syncs");
}

}  // namespace p25p1
