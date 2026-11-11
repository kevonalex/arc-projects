#pragma once
// SOLUTION - worksheet 06, Build C.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "dmr/dmr_given.hpp"
#include "lmr/todo.hpp"

namespace lmrb {

inline dmr::BurstParts split_burst(const dmr::Bits264& burst) {
  dmr::BurstParts p;
  const auto h = static_cast<std::ptrdiff_t>(dmr::kHalfBits);
  const auto m = static_cast<std::ptrdiff_t>(dmr::kMidBits);
  std::copy(burst.begin(), burst.begin() + h, p.first.begin());
  std::copy(burst.begin() + h, burst.begin() + h + m, p.mid.begin());
  std::copy(burst.begin() + h + m, burst.end(), p.second.begin());
  return p;
}

inline dmr::Bits264 join_burst(const dmr::BurstParts& parts) {
  dmr::Bits264 b{};
  const auto h = static_cast<std::ptrdiff_t>(dmr::kHalfBits);
  const auto m = static_cast<std::ptrdiff_t>(dmr::kMidBits);
  std::copy(parts.first.begin(), parts.first.end(), b.begin());
  std::copy(parts.mid.begin(), parts.mid.end(), b.begin() + h);
  std::copy(parts.second.begin(), parts.second.end(), b.begin() + h + m);
  return b;
}

inline std::array<dmr::VoiceFrame, 3> extract_voice_frames(const dmr::BurstParts& parts) {
  std::array<dmr::VoiceFrame, 3> f{};
  std::copy(parts.first.begin(), parts.first.begin() + 72, f[0].begin());
  std::copy(parts.first.begin() + 72, parts.first.end(), f[1].begin());                // 36 bits
  std::copy(parts.second.begin(), parts.second.begin() + 36, f[1].begin() + 36);       // 36 bits
  std::copy(parts.second.begin() + 36, parts.second.end(), f[2].begin());              // 72 bits
  return f;
}

struct SlotView {
  std::size_t index = 0;
  dmr::SyncKind sync = dmr::SyncKind::None;
  dmr::BurstParts parts{};
};

inline std::vector<SlotView> split_stream(const std::vector<std::uint8_t>& bits, std::size_t phase,
                                          int sync_tol) {
  std::vector<SlotView> out;
  for (std::size_t k = 0; phase + (k + 1) * dmr::kSlotBits <= bits.size(); ++k) {
    const std::size_t b = phase + k * dmr::kSlotBits + dmr::kGapBits;
    dmr::Bits264 burst{};
    std::copy(bits.begin() + static_cast<std::ptrdiff_t>(b),
              bits.begin() + static_cast<std::ptrdiff_t>(b + dmr::kBurstBits), burst.begin());
    SlotView v;
    v.index = k;
    v.parts = split_burst(burst);
    v.sync = dmr::classify_sync(dmr::pack_bits48(v.parts.mid), sync_tol);
    out.push_back(v);
  }
  return out;
}

inline std::optional<std::size_t> find_slot_alignment(const std::vector<std::uint8_t>& bits, int sync_tol) {
  std::size_t best_phase = 0;
  std::size_t best_hits = 0;
  for (std::size_t phase = 0; phase < dmr::kSlotBits; ++phase) {
    std::size_t hits = 0;
    for (const SlotView& s : split_stream(bits, phase, sync_tol)) {
      if (s.sync != dmr::SyncKind::None) ++hits;
    }
    if (hits > best_hits) {
      best_hits = hits;
      best_phase = phase;
    }
  }
  if (best_hits == 0) return std::nullopt;
  return best_phase;
}

}  // namespace lmrb
