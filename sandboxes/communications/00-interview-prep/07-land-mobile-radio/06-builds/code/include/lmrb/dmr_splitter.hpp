#pragma once
// STUB - worksheet 06, Build C (Q9-Q10): a DMR burst splitter for SYNTHETIC bit streams.
//
//   slot  = 24-bit gap/CACH | 264-bit burst                  (288 bits = 30 ms)
//   burst = first 108 | middle 48 (sync or embedded) | second 108
//   voice burst = 3 AMBE+2 frames of 72 bits: frame 1 STRADDLES the middle (last 36 of `first` + first 36 of `second`)
// Constants, sync words and classify_sync() are GIVEN in support/dmr/dmr_given.hpp.
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "dmr/dmr_given.hpp"
#include "lmr/todo.hpp"

namespace lmrb {

// Cut a 264-bit burst into first(108) | mid(48) | second(108).   TODO
inline dmr::BurstParts split_burst(const dmr::Bits264& /*burst*/) { lmr::todo("split_burst"); }

// Exact inverse of split_burst.   TODO
inline dmr::Bits264 join_burst(const dmr::BurstParts& /*parts*/) { lmr::todo("join_burst"); }

// Three 72-bit voice frames: [0] = first[0..72), [1] = first[72..108) + second[0..36), [2] = second[36..108).  TODO
inline std::array<dmr::VoiceFrame, 3> extract_voice_frames(const dmr::BurstParts& /*parts*/) {
  lmr::todo("extract_voice_frames");
}

struct SlotView {
  std::size_t index = 0;                       // slot number counted from `phase`
  dmr::SyncKind sync = dmr::SyncKind::None;    // classification of the 48-bit middle
  dmr::BurstParts parts{};
};

// Split `bits` into slots starting at bit offset `phase`. Slot k occupies [phase + 288k, phase + 288(k+1));
// its burst starts 24 bits into the slot. Only COMPLETE slots are returned (a partial tail is ignored).
// sync = dmr::classify_sync(dmr::pack_bits48(parts.mid), sync_tol).
// TODO
inline std::vector<SlotView> split_stream(const std::vector<std::uint8_t>& /*bits*/, std::size_t /*phase*/,
                                          int /*sync_tol*/) {
  lmr::todo("split_stream");
}

// Find the slot phase (0..287). For every phase count the slots whose middle classifies as a sync word
// (sync != None); return the phase with the MOST hits (ties -> smallest phase). No hits at all -> nullopt.
// Why this works: worksheet 06 Q10 (a wrong phase sees random bits in the middle; P(false hit) ~ 1e-9 at tol 4).
// TODO
inline std::optional<std::size_t> find_slot_alignment(const std::vector<std::uint8_t>& /*bits*/,
                                                      int /*sync_tol*/) {
  lmr::todo("find_slot_alignment");
}

}  // namespace lmrb
