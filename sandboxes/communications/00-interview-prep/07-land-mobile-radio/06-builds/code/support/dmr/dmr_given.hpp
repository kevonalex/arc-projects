#pragma once
// GIVEN (complete; built in pack 03): DMR constants, sync words, sync classification, and a builder
// for SYNTHETIC slot streams. Nothing here touches a radio.
//
// Stream layout used by this pack (VERIFY against ETSI TS 102 361-1):
//   slot = 24-bit gap/CACH | 264-bit burst            -> 288 bits = 30 ms at 9600 bit/s
//   burst = 108 bits | 48-bit middle (sync or embedded signalling) | 108 bits
// Bits are stored one per uint8_t (0 or 1), transmitted order.
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "lmrb/rng.hpp"

namespace dmr {

inline constexpr std::size_t kSlotBits = 288;
inline constexpr std::size_t kGapBits = 24;
inline constexpr std::size_t kBurstBits = 264;
inline constexpr std::size_t kHalfBits = 108;
inline constexpr std::size_t kMidBits = 48;
inline constexpr std::size_t kVoiceFrameBits = 72;

using Bits264 = std::array<std::uint8_t, kBurstBits>;
using VoiceFrame = std::array<std::uint8_t, kVoiceFrameBits>;

struct BurstParts {
  std::array<std::uint8_t, kHalfBits> first{};
  std::array<std::uint8_t, kMidBits> mid{};
  std::array<std::uint8_t, kHalfBits> second{};
};

inline constexpr std::uint64_t kBsVoiceSync = 0x755FD7DF75F7ULL;  // VERIFY (all four)
inline constexpr std::uint64_t kBsDataSync = 0xDFF57D75DF5DULL;
inline constexpr std::uint64_t kMsVoiceSync = 0x7F7D5DD57DFDULL;
inline constexpr std::uint64_t kMsDataSync = 0xD5D7F77FD757ULL;

enum class SyncKind { BsVoice, BsData, MsVoice, MsData, None };

inline std::uint64_t pack_bits48(const std::array<std::uint8_t, kMidBits>& mid) {
  std::uint64_t v = 0;
  for (const std::uint8_t b : mid) v = (v << 1) | (b & 1u);
  return v;
}

// Closest of the four sync words within max_errors; ties and misses give None.
inline SyncKind classify_sync(std::uint64_t mid48, int max_errors) {
  constexpr std::uint64_t mask = (1ULL << 48) - 1;
  const std::uint64_t pats[4] = {kBsVoiceSync, kBsDataSync, kMsVoiceSync, kMsDataSync};
  const SyncKind kinds[4] = {SyncKind::BsVoice, SyncKind::BsData, SyncKind::MsVoice, SyncKind::MsData};
  int best = 49;
  int best_idx = -1;
  bool tie = false;
  for (int i = 0; i < 4; ++i) {
    const int d = static_cast<int>(std::bitset<64>((mid48 ^ pats[i]) & mask).count());
    if (d < best) {
      best = d;
      best_idx = i;
      tie = false;
    } else if (d == best) {
      tie = true;
    }
  }
  if (best_idx < 0 || best > max_errors || tie) return SyncKind::None;
  return kinds[best_idx];
}

inline void append_u48(std::vector<std::uint8_t>& bits, std::uint64_t v) {
  for (int i = 47; i >= 0; --i) bits.push_back(static_cast<std::uint8_t>((v >> i) & 1ULL));
}

// One synthetic slot: random gap, random first half, the given 48-bit middle, random second half.
inline std::vector<std::uint8_t> make_slot(lmrb::XorShift64& rng, std::uint64_t mid48) {
  std::vector<std::uint8_t> s = lmrb::random_bits(rng, kGapBits + kHalfBits);
  append_u48(s, mid48);
  const std::vector<std::uint8_t> tail = lmrb::random_bits(rng, kHalfBits);
  s.insert(s.end(), tail.begin(), tail.end());
  return s;
}

}  // namespace dmr
