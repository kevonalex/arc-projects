#pragma once
// SOLUTION - worksheet 03, Q5/Q11.
#include <cstdint>
#include <cstdlib>

#include "lmr/todo.hpp"

namespace dmr {

struct BptcDims {
  int info_rows = 0;
  int info_cols = 0;
  int rows = 0;
  int cols = 0;
  int matrix_bits = 0;
  int total_bits = 0;
  int info_positions = 0;
};

inline BptcDims bptc_dims() {
  BptcDims d;
  d.info_rows = 9;
  d.info_cols = 11;
  d.rows = d.info_rows + 4;  // Hamming(13,9): 4 parity rows
  d.cols = d.info_cols + 4;  // Hamming(15,11): 4 parity columns
  d.matrix_bits = d.rows * d.cols;
  d.total_bits = d.matrix_bits + 1;  // one leading reserved bit
  d.info_positions = d.info_rows * d.info_cols;
  return d;
}

inline int bptc_interleave_index(int a) { return (a * 181) % 196; }

inline int bptc_deinterleave_index(int p) { return (p * 13) % 196; }

inline int gcd_int(int a, int b) {
  while (b != 0) {
    const int t = a % b;
    a = b;
    b = t;
  }
  return a;
}

inline int circular_distance(int a, int b, int n) {
  const int d = std::abs(a - b);
  return d < n - d ? d : n - d;
}

}  // namespace dmr
