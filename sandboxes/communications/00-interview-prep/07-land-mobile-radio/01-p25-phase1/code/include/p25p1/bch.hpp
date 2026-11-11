#pragma once
// STUB - worksheet 01, Q10 (implement systematic cyclic encoding, then decode by brute force).
// Generator polynomial g(x) comes from support/p25p1/gf64.hpp (derived, not copied).
#include <cstdint>

#include "lmr/todo.hpp"
#include "p25p1/gf64.hpp"
#include "p25p1/nid.hpp"

namespace p25p1 {

inline constexpr int kBchN = 63;
inline constexpr int kBchK = 16;
inline constexpr int kBchParity = kBchN - kBchK;  // 47

// TODO: remainder of `value` (as a polynomial over GF(2), bit i = coeff of x^i) divided by
//       `gen` (degree `gen_degree`, so bit gen_degree of gen is set).  Result has < gen_degree bits.
inline std::uint64_t poly_mod(std::uint64_t /*value*/, std::uint64_t /*gen*/, int /*gen_degree*/) {
  lmr::todo("poly_mod");
}

// TODO: systematic encode.  codeword63 = (data << 47) | ((data << 47) mod g).
inline std::uint64_t bch63_16_encode(std::uint16_t /*data*/) { lmr::todo("bch63_16_encode"); }

struct BchResult {
  bool ok = false;
  std::uint16_t data = 0;
  int errors = 0;  // number of bit errors corrected (only meaningful if ok)
};

// TODO: brute-force nearest-codeword decoder.  Compare `received63` with all 65536 codewords,
//       pick the closest.  ok = (distance <= 11).  errors = that distance.
inline BchResult bch63_16_decode(std::uint64_t /*received63*/) { lmr::todo("bch63_16_decode"); }

// TODO: the 64-bit on-air NID: (codeword63 << 1) | even-parity-bit-of-codeword63.
//       VERIFY the exact parity convention in TIA-102.BAAA.
inline std::uint64_t nid_encode64(std::uint16_t /*nac*/, Duid /*duid*/) {
  lmr::todo("nid_encode64");
}

}  // namespace p25p1
