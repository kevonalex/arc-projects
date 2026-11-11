// bitops.h  --  STUBS for Pack 01 (student version).
// Implement every function. Each TODO names the worksheet section that derives it.
// Build/run: see CMakeLists.txt.  Reference answers: ../solutions/bitops.h
// All stubs compile warning-free but return wrong values on purpose.
#pragma once
#include <cstdint>

namespace bits {

// Mask with the low `len` bits set. len in [0,32].
//   low_mask(0)=0, low_mask(4)=0xF, low_mask(32)=0xFFFFFFFF   (W 3.3)
// TRAP: 1u<<32 is undefined behaviour. Handle len==32 separately.
inline uint32_t low_mask([[maybe_unused]] unsigned len) {
  return 0;  // TODO W 3.3
}

// Single-bit operations on bit `pos` (0 = least significant), pos in [0,31]. (W 3.2)
inline uint32_t set_bit(uint32_t x, [[maybe_unused]] unsigned pos) { return x; }     // TODO
inline uint32_t clear_bit(uint32_t x, [[maybe_unused]] unsigned pos) { return x; }   // TODO
inline uint32_t toggle_bit(uint32_t x, [[maybe_unused]] unsigned pos) { return x; }  // TODO
inline bool test_bit([[maybe_unused]] uint32_t x, [[maybe_unused]] unsigned pos) { return false; }  // TODO

// Return the `len`-bit field whose least-significant bit is bit `pos`.
// Precondition: pos <= 31, pos + len <= 32, len in [0,32].   (W 3.3)
inline uint32_t extract_field([[maybe_unused]] uint32_t x, [[maybe_unused]] unsigned pos,
                              [[maybe_unused]] unsigned len) {
  return 0;  // TODO W 3.3
}

// Return x with the `len`-bit field at `pos` replaced by the low `len` bits of `value`.
// Bits of `value` above `len` must NOT leak into neighbouring fields.   (W 3.4)
inline uint32_t insert_field(uint32_t x, [[maybe_unused]] unsigned pos,
                             [[maybe_unused]] unsigned len, [[maybe_unused]] uint32_t value) {
  return x;  // TODO W 3.4
}

// Interpret the low `bits` bits of v as a two's-complement number. bits in [1,32].   (W 3.6)
// Use the (v ^ m) - m trick with m = 1u << (bits-1).
inline int32_t sign_extend([[maybe_unused]] uint32_t v, [[maybe_unused]] unsigned bits) {
  return 0;  // TODO W 3.6
}

// x & -x : isolate the lowest set bit (0 if x==0).                    (W 3.7)
inline uint32_t lowest_set_bit([[maybe_unused]] uint32_t x) { return 0xFFFFFFFFu; }  // TODO
// x & (x-1) : clear the lowest set bit (0 stays 0, no wrap problem).  (W 3.7)
inline uint32_t clear_lowest_set_bit([[maybe_unused]] uint32_t x) { return 0xFFFFFFFFu; }  // TODO
// True iff x is 1,2,4,8,... (NOT 0!).                                  (W 3.7)
inline bool is_power_of_two([[maybe_unused]] uint32_t x) { return false; }  // TODO

// Kernighan population count: loop runs once per SET bit.             (W 3.8)
inline unsigned popcount_kernighan([[maybe_unused]] uint32_t x) { return 99; }  // TODO

// ---- P25 Network ID (NID) first 16 bits: NAC(12) | DUID(4)  --- (W 3.3, 4)
struct P25Nid {
  uint16_t nac;  // 12-bit Network Access Code
  uint8_t duid;  // 4-bit Data Unit ID
};
// Split a 16-bit word laid out as [NAC:12][DUID:4] (NAC in the high bits).
inline P25Nid parse_p25_nid([[maybe_unused]] uint16_t word) {
  return P25Nid{0xFFFu, 0xFu};  // TODO W 4
}
// Inverse of parse_p25_nid. nac/duid are masked to 12/4 bits.
inline uint16_t build_p25_nid([[maybe_unused]] uint16_t nac, [[maybe_unused]] uint8_t duid) {
  return 0xFFFFu;  // TODO W 4
}

}  // namespace bits
