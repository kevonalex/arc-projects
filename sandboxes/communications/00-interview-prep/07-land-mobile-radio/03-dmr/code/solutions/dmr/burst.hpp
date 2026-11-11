#pragma once
// SOLUTION - worksheet 03, Q3/Q4/Q9.
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>

#include "lmr/todo.hpp"

namespace dmr {

inline constexpr int kSymbolRate = 4800;
inline constexpr int kBitRate = 9600;
inline constexpr int kSlotMs = 30;
inline constexpr int kBurstBits = 264;
inline constexpr int kHalfBits = 108;
inline constexpr int kMidBits = 48;
inline constexpr int kVoiceFrameBits = 72;

using Bits264 = std::array<std::uint8_t, kBurstBits>;
using VoiceFrame = std::array<std::uint8_t, kVoiceFrameBits>;

struct BurstParts {
  std::array<std::uint8_t, kHalfBits> first{};
  std::array<std::uint8_t, kMidBits> mid{};
  std::array<std::uint8_t, kHalfBits> second{};
};

inline int slot_bits() { return kBitRate * kSlotMs / 1000; }

inline int gap_bits() { return slot_bits() - kBurstBits; }

inline double burst_ms() { return 1000.0 * kBurstBits / kBitRate; }

inline BurstParts split_burst(const Bits264& burst) {
  BurstParts p;
  std::copy(burst.begin(), burst.begin() + kHalfBits, p.first.begin());
  std::copy(burst.begin() + kHalfBits, burst.begin() + kHalfBits + kMidBits, p.mid.begin());
  std::copy(burst.begin() + kHalfBits + kMidBits, burst.end(), p.second.begin());
  return p;
}

inline Bits264 join_burst(const BurstParts& parts) {
  Bits264 b{};
  std::copy(parts.first.begin(), parts.first.end(), b.begin());
  std::copy(parts.mid.begin(), parts.mid.end(), b.begin() + kHalfBits);
  std::copy(parts.second.begin(), parts.second.end(), b.begin() + kHalfBits + kMidBits);
  return b;
}

inline std::array<VoiceFrame, 3> extract_voice_frames(const BurstParts& parts) {
  std::array<VoiceFrame, 3> f{};
  // frame 0: first 72 bits of the first half
  std::copy(parts.first.begin(), parts.first.begin() + 72, f[0].begin());
  // frame 1: last 36 of the first half + first 36 of the second half
  std::copy(parts.first.begin() + 72, parts.first.end(), f[1].begin());
  std::copy(parts.second.begin(), parts.second.begin() + 36, f[1].begin() + 36);
  // frame 2: last 72 of the second half
  std::copy(parts.second.begin() + 36, parts.second.end(), f[2].begin());
  return f;
}

inline std::uint64_t pack_bits48(const std::array<std::uint8_t, kMidBits>& mid) {
  std::uint64_t v = 0;
  for (std::uint8_t b : mid) v = (v << 1) | (b & 1u);
  return v;
}

inline constexpr std::uint64_t kBsVoiceSync = 0x755FD7DF75F7ULL;
inline constexpr std::uint64_t kBsDataSync = 0xDFF57D75DF5DULL;
inline constexpr std::uint64_t kMsVoiceSync = 0x7F7D5DD57DFDULL;
inline constexpr std::uint64_t kMsDataSync = 0xD5D7F77FD757ULL;

enum class SyncKind { BsVoice, BsData, MsVoice, MsData, None };

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

}  // namespace dmr
