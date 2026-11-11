#pragma once
// STUB - worksheet 03, Q5/Q11 : BPTC(196,96) geometry and interleaver.
//
// BPTC = Block Product Turbo Code: a 2-D array where every ROW is a Hamming(15,11,3) codeword and
// every COLUMN is a Hamming(13,9,3) codeword.  Geometry (ETSI TS 102 361-1 Annex B - VERIFY):
//   * 9 information rows x 11 information columns = 99 positions  = 96 data bits + 3 reserved (R) bits
//   * + 4 column-parity rows (13 rows total), + 4 row-parity columns (15 columns total)
//   * matrix = 13 x 15 = 195 bits, plus one leading reserved bit = 196 bits on air
//   * the 196 bits are interleaved: matrix index a  ->  transmitted position (a * 181) mod 196
#include <cstdint>

#include "lmr/todo.hpp"

namespace dmr {

struct BptcDims {
  int info_rows = 0;
  int info_cols = 0;
  int rows = 0;
  int cols = 0;
  int matrix_bits = 0;     // rows * cols
  int total_bits = 0;      // matrix + leading reserved bit
  int info_positions = 0;  // info_rows * info_cols
};

// TODO: fill {9, 11, 13, 15, 195, 196, 99}.  Derive: rows = 9 + 4, cols = 11 + 4.
inline BptcDims bptc_dims() { lmr::todo("bptc_dims"); }

// TODO: transmitted position of matrix index a (0..195): (a * 181) % 196.
inline int bptc_interleave_index(int /*a*/) { lmr::todo("bptc_interleave_index"); }

// TODO: inverse mapping: matrix index of transmitted position p.
//       Hint: 181 * 13 = 2353 = 12 * 196 + 1, so 13 is the modular inverse of 181 mod 196.
inline int bptc_deinterleave_index(int /*p*/) { lmr::todo("bptc_deinterleave_index"); }

// TODO: greatest common divisor (Euclid).  A multiplier permutes 0..N-1 iff gcd(multiplier, N) == 1.
inline int gcd_int(int /*a*/, int /*b*/) { lmr::todo("gcd_int"); }

// TODO: circular distance between two indices on a ring of N positions: min(|a-b|, N-|a-b|).
inline int circular_distance(int /*a*/, int /*b*/, int /*n*/) { lmr::todo("circular_distance"); }

}  // namespace dmr
