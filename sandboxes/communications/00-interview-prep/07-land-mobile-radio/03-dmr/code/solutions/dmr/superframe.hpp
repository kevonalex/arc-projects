#pragma once
// SOLUTION - worksheet 03, Q7/Q8.
#include <cstdint>

#include "lmr/todo.hpp"

namespace dmr {

enum class VoiceBurst : std::uint8_t { A = 0, B, C, D, E, F };

inline constexpr int kBurstsPerSuperframe = 6;
inline constexpr int kBurstPeriodMs = 60;

inline VoiceBurst burst_letter(std::uint64_t n) {
  return static_cast<VoiceBurst>(n % kBurstsPerSuperframe);
}

inline int superframe_ms() { return kBurstsPerSuperframe * kBurstPeriodMs; }

inline bool has_voice_sync(VoiceBurst b) { return b == VoiceBurst::A; }

inline bool carries_embedded_lc(VoiceBurst b) {
  return b == VoiceBurst::B || b == VoiceBurst::C || b == VoiceBurst::D || b == VoiceBurst::E;
}

inline int embedded_lc_capacity_bits() {
  int bursts = 0;
  for (int i = 0; i < kBurstsPerSuperframe; ++i) {
    if (carries_embedded_lc(static_cast<VoiceBurst>(i))) ++bursts;
  }
  return bursts * 32;
}

inline int late_entry_worst_case_ms(int fragments = 4, int burst_ms = kBurstPeriodMs,
                                    int cycle_bursts = kBurstsPerSuperframe) {
  return cycle_bursts * burst_ms + (fragments - 1) * burst_ms;
}

inline int voice_frames_per_superframe() { return kBurstsPerSuperframe * 3; }

}  // namespace dmr
