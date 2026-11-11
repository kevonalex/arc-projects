#pragma once
// GIVEN (complete): the perfect binary Golay code (23,12,7).
//
// Why "perfect"?  Count the error patterns of weight <= 3 in 23 bits:
//     1 + 23 + 253 + 1771 = 2048 = 2^11,
// and there are exactly 2^(23-12) = 2048 syndromes.  So EVERY syndrome maps to exactly one
// pattern of weight <= 3 and every received word is within distance 3 of exactly one codeword.
//
// Generator polynomial g(x) = x^11 + x^10 + x^6 + x^5 + x^4 + x^2 + 1 = 0xC75 (well known).
// Pack 06 asks you to BUILD this from scratch; here it is only a helper for the DMR slot type.
#include <array>
#include <bitset>
#include <cstdint>

namespace dmr {

inline constexpr std::uint32_t kGolayGen = 0xC75;

inline int popcount32(std::uint32_t v) { return static_cast<int>(std::bitset<32>(v).count()); }

// remainder of a 23-bit word divided by g(x) (11 bits)
inline std::uint32_t golay_syndrome(std::uint32_t v23) {
  v23 &= 0x7FFFFFu;
  for (int i = 22; i >= 11; --i) {
    if ((v23 >> i) & 1u) v23 ^= kGolayGen << (i - 11);
  }
  return v23;
}

// systematic encode: codeword = (info12 << 11) | parity11
inline std::uint32_t golay23_encode(std::uint16_t info12) {
  const std::uint32_t shifted = static_cast<std::uint32_t>(info12 & 0xFFFu) << 11;
  return shifted | golay_syndrome(shifted);
}

inline const std::array<std::uint32_t, 2048>& golay_pattern_table() {
  static const std::array<std::uint32_t, 2048> table = [] {
    std::array<std::uint32_t, 2048> t{};
    for (int a = 0; a < 23; ++a) {
      const std::uint32_t pa = 1u << a;
      t[golay_syndrome(pa)] = pa;
      for (int b = a + 1; b < 23; ++b) {
        const std::uint32_t pb = pa | (1u << b);
        t[golay_syndrome(pb)] = pb;
        for (int c = b + 1; c < 23; ++c) {
          const std::uint32_t pc = pb | (1u << c);
          t[golay_syndrome(pc)] = pc;
        }
      }
    }
    return t;
  }();
  return table;
}

struct Golay23Result {
  std::uint32_t codeword = 0;  // corrected 23-bit codeword
  std::uint16_t data = 0;      // 12 info bits
  int errors = 0;              // number of bits corrected (always <= 3)
};

inline Golay23Result golay23_decode(std::uint32_t received23) {
  received23 &= 0x7FFFFFu;
  const std::uint32_t pattern = golay_pattern_table()[golay_syndrome(received23)];
  Golay23Result r;
  r.codeword = received23 ^ pattern;
  r.data = static_cast<std::uint16_t>(r.codeword >> 11);
  r.errors = popcount32(pattern);
  return r;
}

}  // namespace dmr
