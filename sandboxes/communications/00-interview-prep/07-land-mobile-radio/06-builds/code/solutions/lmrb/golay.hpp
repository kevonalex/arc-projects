#pragma once
// SOLUTION - worksheet 06, Build A.
#include <array>
#include <bitset>
#include <cstdint>

#include "lmr/todo.hpp"

namespace lmrb {

inline constexpr std::uint32_t kGolayGen = 0xC75;

inline int popcount32(std::uint32_t v) { return static_cast<int>(std::bitset<32>(v).count()); }

inline std::uint32_t golay_remainder(std::uint32_t v23) {
  v23 &= 0x7FFFFFu;
  for (int i = 22; i >= 11; --i) {
    if ((v23 >> i) & 1u) v23 ^= kGolayGen << (i - 11);
  }
  return v23;
}

inline std::uint32_t golay23_encode(std::uint16_t info12) {
  const std::uint32_t shifted = static_cast<std::uint32_t>(info12 & 0xFFFu) << 11;
  return shifted | golay_remainder(shifted);
}

inline const std::array<std::uint32_t, 2048>& golay_pattern_table() {
  static const std::array<std::uint32_t, 2048> table = [] {
    std::array<std::uint32_t, 2048> t{};  // t[0] = 0: the empty pattern
    for (int a = 0; a < 23; ++a) {
      const std::uint32_t pa = 1u << a;
      t[golay_remainder(pa)] = pa;
      for (int b = a + 1; b < 23; ++b) {
        const std::uint32_t pb = pa | (1u << b);
        t[golay_remainder(pb)] = pb;
        for (int c = b + 1; c < 23; ++c) {
          const std::uint32_t pc = pb | (1u << c);
          t[golay_remainder(pc)] = pc;
        }
      }
    }
    return t;
  }();
  return table;
}

struct Golay23Result {
  std::uint32_t codeword = 0;
  std::uint16_t data = 0;
  int errors = 0;
};

inline Golay23Result golay23_decode(std::uint32_t received23) {
  received23 &= 0x7FFFFFu;
  const std::uint32_t pattern = golay_pattern_table()[golay_remainder(received23)];
  Golay23Result r;
  r.codeword = received23 ^ pattern;
  r.data = static_cast<std::uint16_t>(r.codeword >> 11);
  r.errors = popcount32(pattern);
  return r;
}

inline std::uint32_t golay24_encode(std::uint16_t info12) {
  const std::uint32_t cw = golay23_encode(info12);
  return (cw << 1) | static_cast<std::uint32_t>(popcount32(cw) & 1);
}

enum class Golay24Status { Ok, Corrected, Uncorrectable };

struct Golay24Result {
  Golay24Status status = Golay24Status::Uncorrectable;
  std::uint32_t codeword = 0;
  std::uint16_t data = 0;
  int errors = 0;
};

inline Golay24Result golay24_decode(std::uint32_t received24) {
  received24 &= 0xFFFFFFu;
  const std::uint32_t r23 = received24 >> 1;
  const std::uint32_t p = received24 & 1u;
  const Golay23Result d = golay23_decode(r23);
  const std::uint32_t expected = static_cast<std::uint32_t>(popcount32(d.codeword) & 1);
  const int m = (expected != p) ? 1 : 0;
  const int total = d.errors + m;
  Golay24Result r;
  r.errors = total;
  if (total >= 4) {
    r.status = Golay24Status::Uncorrectable;
    r.codeword = received24;
    r.data = 0;
    return r;
  }
  r.status = (total == 0) ? Golay24Status::Ok : Golay24Status::Corrected;
  r.codeword = (d.codeword << 1) | expected;
  r.data = d.data;
  return r;
}

}  // namespace lmrb
