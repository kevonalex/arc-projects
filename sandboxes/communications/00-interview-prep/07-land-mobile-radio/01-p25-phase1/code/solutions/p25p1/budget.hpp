#pragma once
// SOLUTION - worksheet 01, Q3.
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p1 {

inline constexpr int kSymbolRate = 4800;
inline constexpr int kBitsPerSymbol = 2;
inline constexpr int kBitRate = kSymbolRate * kBitsPerSymbol;
inline constexpr int kLduBits = 1728;

struct LduBudget {
  int sync_bits = 0;
  int nid_bits = 0;
  int imbe_bits = 0;
  int lc_or_es_bits = 0;
  int lsd_bits = 0;
  int status_bits = 0;
  int total() const {
    return sync_bits + nid_bits + imbe_bits + lc_or_es_bits + lsd_bits + status_bits;
  }
};

inline int imbe_frame_bits() { return 4 * 23 + 3 * 15 + 7; }  // 92 + 45 + 7 = 144
inline int imbe_info_bits() { return 4 * 12 + 3 * 11 + 7; }   // 48 + 33 + 7 = 88

inline double duration_ms(int bits) { return 1000.0 * bits / kBitRate; }

inline int status_symbols_in(int total_symbols) { return total_symbols / 36; }

inline LduBudget ldu_budget() {
  LduBudget b;
  b.sync_bits = 48;
  b.nid_bits = 64;
  b.imbe_bits = 9 * imbe_frame_bits();  // 1296
  b.lc_or_es_bits = 240;                // 72 -> RS(24,12) 144 -> Hamming(10,6) 240  (or ES 96 -> RS(24,16))
  b.lsd_bits = 32;                      // 2 x 16 (2 x 8 info, cyclic (16,8))
  b.status_bits = 2 * status_symbols_in(kLduBits / kBitsPerSymbol);  // 2 x 24 = 48
  return b;
}

}  // namespace p25p1
