// blockfec.h  --  REFERENCE SOLUTION for Pack 05.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fec {

inline unsigned parity32(uint32_t x) {
  x ^= x >> 16;  // fold: after each step bit 0 holds the XOR of a wider and wider slice
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1u;
}
inline unsigned hamming_distance(uint32_t a, uint32_t b) {
  uint32_t x = a ^ b;  // 1 exactly where they differ
  unsigned n = 0;
  while (x != 0u) {
    x &= x - 1u;  // Kernighan (Pack 01 3.8)
    ++n;
  }
  return n;
}
inline unsigned min_distance(const std::vector<uint32_t>& codewords) {
  if (codewords.size() < 2) return 0;
  unsigned best = 0xFFFFFFFFu;
  for (size_t i = 0; i < codewords.size(); ++i)
    for (size_t j = i + 1; j < codewords.size(); ++j) {
      const unsigned d = hamming_distance(codewords[i], codewords[j]);
      if (d < best) best = d;
    }
  return best;
}

// ---- Hamming (7,4).  Position k lives in bit k-1. Group masks:
//   p1 covers positions {1,3,5,7} -> bits {0,2,4,6} = 0x55
//   p2 covers positions {2,3,6,7} -> bits {1,2,5,6} = 0x66
//   p4 covers positions {4,5,6,7} -> bits {3,4,5,6} = 0x78
inline uint8_t hamming74_encode(uint8_t nibble) {
  uint32_t cw = 0;
  cw |= static_cast<uint32_t>((nibble >> 0) & 1u) << 2;  // d1 -> position 3
  cw |= static_cast<uint32_t>((nibble >> 1) & 1u) << 4;  // d2 -> position 5
  cw |= static_cast<uint32_t>((nibble >> 2) & 1u) << 5;  // d3 -> position 6
  cw |= static_cast<uint32_t>((nibble >> 3) & 1u) << 6;  // d4 -> position 7
  cw |= parity32(cw & 0x55u) << 0;                       // p1 makes group 1 even
  cw |= parity32(cw & 0x66u) << 1;                       // p2
  cw |= parity32(cw & 0x78u) << 3;                       // p4
  return static_cast<uint8_t>(cw);
}

struct Hamming74Result {
  uint8_t data;
  uint8_t syndrome;
  bool corrected;
};
inline uint8_t hamming74_extract(uint8_t cw) {
  return static_cast<uint8_t>(((cw >> 2) & 1u) | (((cw >> 4) & 1u) << 1) | (((cw >> 5) & 1u) << 2) |
                              (((cw >> 6) & 1u) << 3));
}
inline unsigned hamming74_syndrome(uint8_t cw7) {
  const unsigned s1 = parity32(cw7 & 0x55u);
  const unsigned s2 = parity32(cw7 & 0x66u);
  const unsigned s4 = parity32(cw7 & 0x78u);
  return s1 | (s2 << 1) | (s4 << 2);  // read as a binary number == position of the flipped bit
}
inline Hamming74Result hamming74_decode(uint8_t cw7) {
  cw7 = static_cast<uint8_t>(cw7 & 0x7Fu);
  const unsigned s = hamming74_syndrome(cw7);
  bool fixed = false;
  if (s != 0) {
    cw7 = static_cast<uint8_t>(cw7 ^ (1u << (s - 1u)));  // position s lives in bit s-1
    fixed = true;
  }
  return Hamming74Result{hamming74_extract(cw7), static_cast<uint8_t>(s), fixed};
}

// ---- Extended Hamming (8,4,4)
enum class Status { Ok, Corrected, Uncorrectable };
inline uint8_t hamming84_encode(uint8_t nibble) {
  const uint8_t cw7 = hamming74_encode(nibble);
  return static_cast<uint8_t>(cw7 | (parity32(cw7) << 7));
}
struct Hamming84Result {
  uint8_t data;
  Status status;
};
inline Hamming84Result hamming84_decode(uint8_t cw8) {
  const uint8_t low = static_cast<uint8_t>(cw8 & 0x7Fu);
  const unsigned s = hamming74_syndrome(low);
  const unsigned overall = parity32(cw8);  // 0 if the whole 8-bit word still has even parity
  if (s == 0 && overall == 0) return {hamming74_extract(low), Status::Ok};
  if (overall == 1) {  // odd number of errors: assume ONE error
    uint8_t fixed = low;
    if (s != 0) fixed = static_cast<uint8_t>(low ^ (1u << (s - 1u)));  // s == 0: the parity bit itself
    return {hamming74_extract(fixed), Status::Corrected};
  }
  return {hamming74_extract(low), Status::Uncorrectable};  // s != 0 but parity even: two errors
}

// ---- Golay (23,12,7)
inline uint32_t golay23_encode(uint16_t data12) {
  const uint32_t d = data12 & 0xFFFu;
  uint32_t work = d << 11;
  for (int i = 22; i >= 11; --i) {  // long division by g = 0xC75, exactly as in Pack 04 3.2
    if ((work >> i) & 1u) work ^= 0xC75u << (i - 11);
  }
  return (d << 11) | (work & 0x7FFu);
}
struct GolayResult {
  uint16_t data;
  int errors;
};
inline GolayResult golay23_decode(uint32_t rx23) {
  static const std::array<uint32_t, 4096> table = [] {
    std::array<uint32_t, 4096> t{};
    for (uint32_t d = 0; d < 4096; ++d) t[d] = golay23_encode(static_cast<uint16_t>(d));
    return t;
  }();
  rx23 &= 0x7FFFFFu;
  unsigned best = 99;
  uint32_t best_d = 0;
  for (uint32_t d = 0; d < 4096; ++d) {
    const unsigned dist = hamming_distance(table[d], rx23);
    if (dist < best) {
      best = dist;
      best_d = d;
    }
  }
  if (best <= 3) return GolayResult{static_cast<uint16_t>(best_d), static_cast<int>(best)};
  return GolayResult{static_cast<uint16_t>(rx23 >> 11), -1};
}

// ---- toy Reed-Solomon over GF(5)
inline std::array<uint8_t, 4> rs5_encode(uint8_t a0, uint8_t a1) {
  std::array<uint8_t, 4> out{};
  for (unsigned x = 0; x < 4; ++x) out[x] = static_cast<uint8_t>((a0 + a1 * x) % 5u);
  return out;
}
inline bool rs5_recover(int x1, int y1, int x2, int y2, uint8_t& a0, uint8_t& a1) {
  if (x1 < 0 || x1 > 3 || x2 < 0 || x2 > 3 || x1 == x2) return false;
  static const int inv[5] = {0, 1, 3, 2, 4};  // multiplicative inverses mod 5
  const int dx = ((x2 - x1) % 5 + 5) % 5;
  const int dy = ((y2 - y1) % 5 + 5) % 5;
  const int slope = (dy * inv[dx]) % 5;
  const int intercept = (((y1 - slope * x1) % 5) + 5) % 5;
  a1 = static_cast<uint8_t>(slope);
  a0 = static_cast<uint8_t>(intercept);
  return true;
}

}  // namespace fec
