#pragma once
// SOLUTION - worksheet 01, Q10.
#include <bitset>
#include <cstdint>
#include <vector>

#include "lmr/todo.hpp"
#include "p25p1/gf64.hpp"
#include "p25p1/nid.hpp"

namespace p25p1 {

inline constexpr int kBchN = 63;
inline constexpr int kBchK = 16;
inline constexpr int kBchParity = kBchN - kBchK;  // 47

inline std::uint64_t poly_mod(std::uint64_t value, std::uint64_t gen, int gen_degree) {
  // Long division over GF(2): whenever the leading term is present, subtract (XOR) g aligned to it.
  for (int i = 63; i >= gen_degree; --i) {
    if ((value >> i) & 1ULL) value ^= gen << (i - gen_degree);
  }
  return value;
}

inline std::uint64_t bch63_16_encode(std::uint16_t data) {
  const std::uint64_t shifted = static_cast<std::uint64_t>(data) << kBchParity;
  return shifted | poly_mod(shifted, bch63_16_generator(), kBchParity);
}

struct BchResult {
  bool ok = false;
  std::uint16_t data = 0;
  int errors = 0;
};

inline BchResult bch63_16_decode(std::uint64_t received63) {
  static const std::vector<std::uint64_t> codewords = [] {
    std::vector<std::uint64_t> v(65536);
    for (std::uint32_t d = 0; d < 65536; ++d) v[d] = bch63_16_encode(static_cast<std::uint16_t>(d));
    return v;
  }();
  int best = 64;
  std::uint32_t best_data = 0;
  for (std::uint32_t d = 0; d < 65536; ++d) {
    const int dist = static_cast<int>(std::bitset<64>(codewords[d] ^ received63).count());
    if (dist < best) {
      best = dist;
      best_data = d;
    }
  }
  BchResult r;
  r.errors = best;
  r.data = static_cast<std::uint16_t>(best_data);
  r.ok = best <= 11;  // t = 11 for d_min = 23
  return r;
}

inline std::uint64_t nid_encode64(std::uint16_t nac, Duid duid) {
  const std::uint64_t cw = bch63_16_encode(pack_nid_data(nac, duid));
  const std::uint64_t parity = std::bitset<64>(cw).count() & 1ULL;  // makes total weight even
  return (cw << 1) | parity;
}

}  // namespace p25p1
