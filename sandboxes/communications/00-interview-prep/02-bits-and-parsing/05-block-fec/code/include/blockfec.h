// blockfec.h  --  STUBS for Pack 05 (student version).  Reference: ../solutions/blockfec.h
// Bit/position convention for ALL codewords here: codeword position k (1-based, as in the
// worksheet's tables) is stored in bit (k-1) of the integer. So position 1 = least-significant bit.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fec {

// ---- W 3.2 / 3.3 ----
// Even-parity bit of x: 1 if x has an odd number of 1 bits, else 0.  (XOR-fold or popcount&1)
inline unsigned parity32([[maybe_unused]] uint32_t x) { return 2; }  // TODO W 3.2
// Number of bit positions where a and b differ.
inline unsigned hamming_distance([[maybe_unused]] uint32_t a, [[maybe_unused]] uint32_t b) { return 99; }  // TODO W 3.3
// Smallest Hamming distance between any two DIFFERENT entries (assume distinct). 0 if < 2 words.
inline unsigned min_distance([[maybe_unused]] const std::vector<uint32_t>& codewords) { return 0; }  // TODO W 3.3

// ---- W 3.4: Hamming (7,4) ----
// Positions 1..7 (bit k-1): p1 p2 d1 p4 d2 d3 d4.  Nibble bit0 -> position 3, bit1 -> pos 5,
// bit2 -> pos 6, bit3 -> pos 7.  p1 covers positions {1,3,5,7}, p2 {2,3,6,7}, p4 {4,5,6,7}; all even parity.
inline uint8_t hamming74_encode([[maybe_unused]] uint8_t nibble) { return 0xFF; }  // TODO W 3.4
struct Hamming74Result {
  uint8_t data;      // decoded nibble
  uint8_t syndrome;  // 0 = no error seen, else the (1-based) position the decoder believes is wrong
  bool corrected;    // true if a bit was flipped
};
inline Hamming74Result hamming74_decode([[maybe_unused]] uint8_t cw7) { return {0xFF, 0xFF, false}; }  // TODO W 3.4

// ---- W 3.5: extended Hamming (8,4,4): the 7-bit word plus an overall even-parity bit in bit 7 ----
enum class Status { Ok, Corrected, Uncorrectable };
inline uint8_t hamming84_encode([[maybe_unused]] uint8_t nibble) { return 0xFF; }  // TODO W 3.5
struct Hamming84Result {
  uint8_t data;
  Status status;
};
inline Hamming84Result hamming84_decode([[maybe_unused]] uint8_t cw8) { return {0xFF, Status::Ok}; }  // TODO W 3.5

// ---- W 3.6: binary Golay (23,12,7), generator g(x) = x^11+x^10+x^6+x^5+x^4+x^2+1 = 0xC75 ----
// Systematic: codeword = (data12 << 11) | ((data12 << 11) mod g).   (Same long division as Pack 04.)
inline uint32_t golay23_encode([[maybe_unused]] uint16_t data12) { return 0xFFFFFFFFu; }  // TODO W 3.6
struct GolayResult {
  uint16_t data;  // decoded 12 data bits
  int errors;     // number of bit errors corrected (0..3), or -1 if no codeword is within distance 3
};
// Nearest-codeword (minimum-distance) decoding. Brute force over all 4096 codewords is fine.
inline GolayResult golay23_decode([[maybe_unused]] uint32_t rx23) { return {0xFFFF, -2}; }  // TODO W 3.6

// ---- W 3.8: toy Reed-Solomon over GF(5): message (a0,a1) is the polynomial a0 + a1*x; codeword is
// its values at x = 0,1,2,3 (n=4, k=2, d=n-k+1=3). Symbols are 0..4. ----
inline std::array<uint8_t, 4> rs5_encode([[maybe_unused]] uint8_t a0, [[maybe_unused]] uint8_t a1) {
  return {9, 9, 9, 9};  // TODO W 3.8
}
// Given any two surviving symbols (xi in 0..3 distinct, yi = value at xi), rebuild (a0,a1).
// Return false if x1 == x2 or an x is out of range.
inline bool rs5_recover([[maybe_unused]] int x1, [[maybe_unused]] int y1, [[maybe_unused]] int x2,
                        [[maybe_unused]] int y2, [[maybe_unused]] uint8_t& a0, [[maybe_unused]] uint8_t& a1) {
  return false;  // TODO W 3.8
}

}  // namespace fec
