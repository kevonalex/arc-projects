// bitops.h  --  REFERENCE SOLUTION for Pack 01.  Do not peek before attempting the stubs.
// Same file name and API as include/bitops.h; CMake picks this one for the *_solution binary.
#pragma once
#include <cstdint>

namespace bits {

// (1u << len) - 1 has exactly `len` ones: 1<<len is a single 1 at bit `len`;
// subtracting 1 borrows through the `len` zeros below it (W 3.3).
// len == 32 would be 1u<<32 == UB, so special-case it.
inline uint32_t low_mask(unsigned len) {
  return len >= 32u ? 0xFFFFFFFFu : ((1u << len) - 1u);
}

inline uint32_t set_bit(uint32_t x, unsigned pos) { return x | (1u << pos); }
inline uint32_t clear_bit(uint32_t x, unsigned pos) { return x & ~(1u << pos); }
inline uint32_t toggle_bit(uint32_t x, unsigned pos) { return x ^ (1u << pos); }
inline bool test_bit(uint32_t x, unsigned pos) { return ((x >> pos) & 1u) != 0u; }

inline uint32_t extract_field(uint32_t x, unsigned pos, unsigned len) {
  if (pos >= 32u) return 0u;  // shifting by >= width is UB; nothing to extract anyway
  return (x >> pos) & low_mask(len);
}

inline uint32_t insert_field(uint32_t x, unsigned pos, unsigned len, uint32_t value) {
  if (len == 0u || pos >= 32u) return x;
  const uint32_t field_mask = low_mask(len) << pos;       // 1s exactly where the field lives
  const uint32_t shifted = (value << pos) & field_mask;   // clip value so it cannot leak
  return (x & ~field_mask) | shifted;                     // clear the hole, drop the value in
}

// (v ^ m) - m, m = 1 << (bits-1).  Done in unsigned arithmetic (wraps, never UB);
// the final conversion to int32_t is modular on every mainstream compiler and
// defined as modular from C++20.
inline int32_t sign_extend(uint32_t v, unsigned bits) {
  if (bits == 0u) return 0;
  if (bits > 32u) bits = 32u;
  const uint32_t m = 1u << (bits - 1u);
  v &= low_mask(bits);
  return static_cast<int32_t>((v ^ m) - m);
}

inline uint32_t lowest_set_bit(uint32_t x) { return x & (0u - x); }  // 0u-x == two's complement -x
inline uint32_t clear_lowest_set_bit(uint32_t x) { return x & (x - 1u); }  // 0 & 0xFFFFFFFF == 0
inline bool is_power_of_two(uint32_t x) { return x != 0u && (x & (x - 1u)) == 0u; }

inline unsigned popcount_kernighan(uint32_t x) {
  unsigned n = 0;
  while (x != 0u) {
    x &= (x - 1u);  // removes exactly one set bit per pass
    ++n;
  }
  return n;
}

struct P25Nid {
  uint16_t nac;
  uint8_t duid;
};
inline P25Nid parse_p25_nid(uint16_t word) {
  return P25Nid{static_cast<uint16_t>(extract_field(word, 4, 12)),
                static_cast<uint8_t>(extract_field(word, 0, 4))};
}
inline uint16_t build_p25_nid(uint16_t nac, uint8_t duid) {
  uint32_t w = 0;
  w = insert_field(w, 4, 12, nac);
  w = insert_field(w, 0, 4, duid);
  return static_cast<uint16_t>(w);
}

}  // namespace bits
