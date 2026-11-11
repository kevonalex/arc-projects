#pragma once
// SOLUTION - worksheet 02, Q3/Q4/Q6/Q9.
#include <array>
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p2 {

inline constexpr int kSymbolRate = 6000;
inline constexpr int kBitsPerSymbol = 2;
inline constexpr int kSlotMs = 30;
inline constexpr int kSlotsPerSuperframe = 12;
inline constexpr int kLogicalChannels = 2;
inline constexpr int kAmbeFrameBits = 72;
inline constexpr int kAmbeFrameMs = 20;

enum class Burst { V4, V2, Sacch };
inline std::array<Burst, 6> channel_pattern() {
  return {Burst::V4, Burst::V4, Burst::V4, Burst::V4, Burst::V2, Burst::Sacch};
}
inline int voice_frames_in(Burst b) {
  switch (b) {
    case Burst::V4: return 4;
    case Burst::V2: return 2;
    case Burst::Sacch: return 0;
  }
  return 0;
}

inline int gross_bit_rate() { return kSymbolRate * kBitsPerSymbol; }

inline int symbols_per_slot() { return kSymbolRate * kSlotMs / 1000; }

inline int bits_per_slot() { return symbols_per_slot() * kBitsPerSymbol; }

inline int superframe_ms() { return kSlotsPerSuperframe * kSlotMs; }

inline int slots_per_channel_per_superframe() { return kSlotsPerSuperframe / kLogicalChannels; }

inline int voice_frames_per_channel_per_superframe() {
  int total = 0;
  for (Burst b : channel_pattern()) total += voice_frames_in(b);
  return total;
}

inline int overhead_bits_in_4v_slot() { return bits_per_slot() - 4 * kAmbeFrameBits; }

inline double spectral_efficiency(double bit_rate, double bandwidth_hz) {
  return bit_rate / bandwidth_hz;
}

inline double tx_duty_cycle(int channels) { return 1.0 / channels; }

inline int calls_that_fit(int gross_bps, int per_call_bps, int overhead_bps) {
  const int usable = gross_bps - overhead_bps;
  return usable < 0 ? 0 : usable / per_call_bps;
}

struct SlotPosition {
  std::uint64_t superframe = 0;
  int slot_in_superframe = 0;
  int channel = 0;
  int symbol_in_slot = 0;
};

inline SlotPosition position_of_symbol(std::uint64_t symbol_index) {
  const std::uint64_t sps = static_cast<std::uint64_t>(symbols_per_slot());
  const std::uint64_t slot_abs = symbol_index / sps;
  SlotPosition p;
  p.superframe = slot_abs / kSlotsPerSuperframe;
  p.slot_in_superframe = static_cast<int>(slot_abs % kSlotsPerSuperframe);
  p.channel = p.slot_in_superframe % kLogicalChannels;
  p.symbol_in_slot = static_cast<int>(symbol_index % sps);
  return p;
}

}  // namespace p25p2
