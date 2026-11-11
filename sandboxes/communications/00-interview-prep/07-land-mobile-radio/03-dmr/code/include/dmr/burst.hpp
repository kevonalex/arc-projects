#pragma once
// STUB - worksheet 03, Q3/Q4/Q9 : the DMR 264-bit burst (108 + 48 + 108) and its sync words.
// SYNTHETIC BITS ONLY. Layout: ETSI TS 102 361-1 (air interface protocol). VERIFY clause numbers.
#include <array>
#include <cstdint>

#include "lmr/todo.hpp"

namespace dmr {

inline constexpr int kSymbolRate = 4800;
inline constexpr int kBitRate = 9600;  // 4FSK: 2 bits/symbol
inline constexpr int kSlotMs = 30;
inline constexpr int kBurstBits = 264;
inline constexpr int kHalfBits = 108;
inline constexpr int kMidBits = 48;
inline constexpr int kVoiceFrameBits = 72;  // one AMBE+2 frame (20 ms)

// One bit per byte (value 0 or 1), index 0 = first bit transmitted.
using Bits264 = std::array<std::uint8_t, kBurstBits>;
using VoiceFrame = std::array<std::uint8_t, kVoiceFrameBits>;

struct BurstParts {
  std::array<std::uint8_t, kHalfBits> first{};
  std::array<std::uint8_t, kMidBits> mid{};  // SYNC (voice A / data) or EMB+embedded (voice B-F)
  std::array<std::uint8_t, kHalfBits> second{};
};

// TODO: slot length in bits = bit rate * slot ms  (288)
inline int slot_bits() { lmr::todo("slot_bits"); }

// TODO: bits in a slot NOT covered by the burst (24: CACH on a base-station downlink, guard on uplink; VERIFY)
inline int gap_bits() { lmr::todo("gap_bits"); }

// TODO: on-air duration of the burst itself in ms (27.5)
inline double burst_ms() { lmr::todo("burst_ms"); }

// TODO: bits [0,108) -> first, [108,156) -> mid, [156,264) -> second.
inline BurstParts split_burst(const Bits264& /*burst*/) { lmr::todo("split_burst"); }

// TODO: inverse of split_burst.
inline Bits264 join_burst(const BurstParts& /*parts*/) { lmr::todo("join_burst"); }

// TODO: a voice burst carries 3 AMBE+2 frames (216 bits) spread over the two payload halves:
//   frame 0 = first[0..71]
//   frame 1 = first[72..107] followed by second[0..35]     (straddles the 48-bit middle!)
//   frame 2 = second[36..107]
inline std::array<VoiceFrame, 3> extract_voice_frames(const BurstParts& /*parts*/) {
  lmr::todo("extract_voice_frames");
}

// TODO: pack the 48 mid bits, MSB first (mid[0] is bit 47 of the result).
inline std::uint64_t pack_bits48(const std::array<std::uint8_t, kMidBits>& /*mid*/) {
  lmr::todo("pack_bits48");
}

// 48-bit sync words (ETSI TS 102 361-1 Annex; also widely documented). VERIFY every digit.
inline constexpr std::uint64_t kBsVoiceSync = 0x755FD7DF75F7ULL;
inline constexpr std::uint64_t kBsDataSync = 0xDFF57D75DF5DULL;
inline constexpr std::uint64_t kMsVoiceSync = 0x7F7D5DD57DFDULL;
inline constexpr std::uint64_t kMsDataSync = 0xD5D7F77FD757ULL;

enum class SyncKind { BsVoice, BsData, MsVoice, MsData, None };

// TODO: return the sync pattern the 48-bit word is within max_errors of; None if no pattern
//       matches (or if two patterns are equally close).
inline SyncKind classify_sync(std::uint64_t /*mid48*/, int /*max_errors*/) {
  lmr::todo("classify_sync");
}

}  // namespace dmr
