#pragma once
// STUB - worksheet 02, Q3/Q4/Q6/Q9 : P25 Phase 2 timing, capacity, slot arithmetic.
// SYNTHETIC FRAMES ONLY. Numbers are derived in worksheet section 3; VERIFY against
// TIA-102.BBAB (physical layer) and TIA-102.BBAC (MAC layer).
#include <array>
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p2 {

inline constexpr int kSymbolRate = 6000;         // symbols per second
inline constexpr int kBitsPerSymbol = 2;         // DQPSK / CPM dibits
inline constexpr int kSlotMs = 30;               // one timeslot (VERIFY)
inline constexpr int kSlotsPerSuperframe = 12;   // per RF carrier, both logical channels (VERIFY)
inline constexpr int kLogicalChannels = 2;
inline constexpr int kAmbeFrameBits = 72;        // AMBE+2 half-rate: 49 speech + 23 FEC per 20 ms
inline constexpr int kAmbeFrameMs = 20;

// GIVEN teaching layout of ONE logical channel's six slots inside a 360 ms superframe.
// (Real ordering and the S/ISCH placement: VERIFY TIA-102.BBAC.)
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

// TODO: kSymbolRate * kBitsPerSymbol  (12000)
inline int gross_bit_rate() { lmr::todo("gross_bit_rate"); }

// TODO: symbols in one slot = symbol rate * slot duration (180)
inline int symbols_per_slot() { lmr::todo("symbols_per_slot"); }

// TODO: bits in one slot (360)
inline int bits_per_slot() { lmr::todo("bits_per_slot"); }

// TODO: superframe duration in ms = slots * slot ms (360)
inline int superframe_ms() { lmr::todo("superframe_ms"); }

// TODO: slots one logical channel owns per superframe (6)
inline int slots_per_channel_per_superframe() { lmr::todo("slots_per_channel_per_superframe"); }

// TODO: sum voice_frames_in over channel_pattern()  (18 = 360 ms / 20 ms)
inline int voice_frames_per_channel_per_superframe() {
  lmr::todo("voice_frames_per_channel_per_superframe");
}

// TODO: a 4V slot carries 4 AMBE+2 frames; everything else in the slot is overhead (VERIFY meaning).
inline int overhead_bits_in_4v_slot() { lmr::todo("overhead_bits_in_4v_slot"); }

// TODO: bit/s per Hz
inline double spectral_efficiency(double /*bit_rate*/, double /*bandwidth_hz*/) {
  lmr::todo("spectral_efficiency");
}

// TODO: fraction of time one radio transmits if the carrier is shared by n logical channels (1/n)
inline double tx_duty_cycle(int /*channels*/) { lmr::todo("tx_duty_cycle"); }

// TODO: how many voice calls fit: floor((gross - overhead) / per_call), all in bit/s.
inline int calls_that_fit(int /*gross_bps*/, int /*per_call_bps*/, int /*overhead_bps*/) {
  lmr::todo("calls_that_fit");
}

struct SlotPosition {
  std::uint64_t superframe = 0;
  int slot_in_superframe = 0;  // 0..11
  int channel = 0;             // 0 or 1 : even slots -> 0, odd slots -> 1
  int symbol_in_slot = 0;      // 0..179
};

// TODO: locate an absolute symbol index (counted from superframe 0, slot 0, symbol 0).
inline SlotPosition position_of_symbol(std::uint64_t /*symbol_index*/) {
  lmr::todo("position_of_symbol");
}

}  // namespace p25p2
