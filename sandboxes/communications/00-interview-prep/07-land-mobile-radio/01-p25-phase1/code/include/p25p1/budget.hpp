#pragma once
// STUB - worksheet 01, Q3 (bit budget of one LDU).  All numbers must be DERIVED, then checked.
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p1 {

inline constexpr int kSymbolRate = 4800;                 // C4FM, symbols per second
inline constexpr int kBitsPerSymbol = 2;                 // 4 levels
inline constexpr int kBitRate = kSymbolRate * kBitsPerSymbol;  // 9600 bit/s
inline constexpr int kLduBits = 1728;                    // one LDU1 or LDU2 on air (incl. status)

struct LduBudget {
  int sync_bits = 0;
  int nid_bits = 0;
  int imbe_bits = 0;      // 9 voice frames
  int lc_or_es_bits = 0;  // LDU1: link control, LDU2: encryption sync (after FEC)
  int lsd_bits = 0;       // low speed data (after FEC)
  int status_bits = 0;    // status symbols interleaved in the stream
  int total() const {
    return sync_bits + nid_bits + imbe_bits + lc_or_es_bits + lsd_bits + status_bits;
  }
};

// TODO: fill the budget {48, 64, 9*144, 240, 32, 48}.  Derive each number first (worksheet 3.6).
inline LduBudget ldu_budget() { lmr::todo("ldu_budget"); }

// TODO: bits in ONE IMBE voice frame after FEC = 4 x Golay(23,12) + 3 x Hamming(15,11) + 7 raw bits.
inline int imbe_frame_bits() { lmr::todo("imbe_frame_bits"); }

// TODO: information (speech) bits in one IMBE frame = 4x12 + 3x11 + 7.
inline int imbe_info_bits() { lmr::todo("imbe_info_bits"); }

// TODO: duration of `bits` on air at 9600 bit/s, in milliseconds.
inline double duration_ms(int /*bits*/) { lmr::todo("duration_ms"); }

// TODO: how many 2-bit status symbols appear in `symbols` total symbols if one status symbol
//       follows every 35 data symbols (i.e. every 36th symbol is a status symbol).
inline int status_symbols_in(int /*total_symbols*/) { lmr::todo("status_symbols_in"); }

}  // namespace p25p1
