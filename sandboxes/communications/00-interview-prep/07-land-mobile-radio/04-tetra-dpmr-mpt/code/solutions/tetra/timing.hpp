#pragma once
// SOLUTION - worksheet 04, Q3/Q4/Q6/Q9.
#include <cstdint>

#include "lmr/todo.hpp"

namespace tetra {

inline constexpr int kSymbolRate = 18000;
inline constexpr int kBitsPerSymbol = 2;
inline constexpr int kBitsPerSlot = 510;
inline constexpr int kSlotsPerFrame = 4;
inline constexpr int kFramesPerMultiframe = 18;
inline constexpr int kMultiframesPerHyperframe = 60;
inline constexpr int kTchBlockBits = 432;
inline constexpr int kSpeechFrameBits = 137;
inline constexpr int kSpeechFrameMs = 30;

inline int gross_bit_rate() { return kSymbolRate * kBitsPerSymbol; }

inline double slot_ms() { return 1000.0 * kBitsPerSlot / gross_bit_rate(); }

inline double frame_ms() { return kSlotsPerFrame * slot_ms(); }

inline double multiframe_ms() { return kFramesPerMultiframe * frame_ms(); }

inline double hyperframe_s() { return kMultiframesPerHyperframe * multiframe_ms() / 1000.0; }

inline int traffic_frames_per_multiframe() { return kFramesPerMultiframe - 1; }

inline int overhead_bits_in_normal_burst() { return kBitsPerSlot - kTchBlockBits; }

inline double tch72_net_rate() {
  return kTchBlockBits * traffic_frames_per_multiframe() / (multiframe_ms() / 1000.0);
}

inline double net_rate_per_carrier() { return kSlotsPerFrame * tch72_net_rate(); }

inline double net_over_gross() { return net_rate_per_carrier() / gross_bit_rate(); }

inline double speech_rate_bps() { return 1000.0 * kSpeechFrameBits / kSpeechFrameMs; }

inline int speech_frames_per_slot() { return 2; }
inline int speech_ms_per_slot() { return speech_frames_per_slot() * kSpeechFrameMs; }

struct TetraTime {
  int timeslot = 1;
  int frame = 1;
  int multiframe = 1;
};

inline TetraTime time_of_slot(std::uint64_t n) {
  TetraTime t;
  t.timeslot = static_cast<int>(n % kSlotsPerFrame) + 1;
  t.frame = static_cast<int>((n / kSlotsPerFrame) % kFramesPerMultiframe) + 1;
  t.multiframe =
      static_cast<int>((n / (kSlotsPerFrame * kFramesPerMultiframe)) % kMultiframesPerHyperframe) + 1;
  return t;
}

inline bool is_control_frame(int frame) { return frame == kFramesPerMultiframe; }

}  // namespace tetra
