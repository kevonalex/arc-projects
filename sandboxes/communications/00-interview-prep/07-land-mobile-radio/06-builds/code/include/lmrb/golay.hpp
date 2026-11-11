#pragma once
// STUB - worksheet 06, Build A (Q1-Q4): the perfect Golay(23,12,7) code and its extension Golay(24,12,8).
// Synthetic words only. Every function below throws until you implement it.
//
// Facts you will DERIVE in the worksheet (do not just copy them):
//   g(x) = x^11 + x^10 + x^6 + x^5 + x^4 + x^2 + 1 = 0xC75       (12 bits: bit 11 is the leading term)
//   Perfect: 1 + 23 + C(23,2) + C(23,3) = 1 + 23 + 253 + 1771 = 2048 = 2^11  syndromes.
//   Extended: add one overall (even) parity bit -> d_min 8: corrects 3 errors, DETECTS 4.
#include <array>
#include <cstdint>

#include "lmr/todo.hpp"

namespace lmrb {

inline constexpr std::uint32_t kGolayGen = 0xC75;

// Remainder of the 23-bit polynomial `v23` divided by g(x). Result has at most 11 bits.
// TODO: for bit i from 22 down to 11, if bit i is set XOR (g << (i - 11)). Mask the input to 23 bits first.
inline std::uint32_t golay_remainder(std::uint32_t /*v23*/) { lmr::todo("golay_remainder"); }

// Systematic encode: codeword = (info12 << 11) | remainder(info12 << 11).   (info is masked to 12 bits)
// TODO
inline std::uint32_t golay23_encode(std::uint16_t /*info12*/) { lmr::todo("golay23_encode"); }

// Table indexed by SYNDROME (0..2047) giving the unique error pattern of weight <= 3 with that syndrome.
// TODO: enumerate all patterns of weight 0..3 over 23 bits and store pattern at table[remainder(pattern)].
// Perfect-code property: this fills every one of the 2048 entries exactly once.
inline const std::array<std::uint32_t, 2048>& golay_pattern_table() { lmr::todo("golay_pattern_table"); }

struct Golay23Result {
  std::uint32_t codeword = 0;  // corrected 23-bit codeword
  std::uint16_t data = 0;      // the 12 information bits (codeword >> 11)
  int errors = 0;              // weight of the error pattern that was removed (always <= 3)
};

// TODO: syndrome -> pattern -> XOR it off. A perfect code NEVER fails: with >= 4 errors it silently
// "corrects" to a wrong codeword. That is why the 24-bit extension below exists.
inline Golay23Result golay23_decode(std::uint32_t /*received23*/) { lmr::todo("golay23_decode"); }

// 24-bit word = (golay23 codeword << 1) | overall parity bit (parity chosen so the 24-bit weight is EVEN).
// TODO
inline std::uint32_t golay24_encode(std::uint16_t /*info12*/) { lmr::todo("golay24_encode"); }

enum class Golay24Status { Ok, Corrected, Uncorrectable };

struct Golay24Result {
  Golay24Status status = Golay24Status::Uncorrectable;
  std::uint32_t codeword = 0;  // corrected 24-bit codeword (== input when Uncorrectable)
  std::uint16_t data = 0;      // 12 info bits (0 when Uncorrectable)
  int errors = 0;              // e + m, see below
};

// TODO:
//   r23 = r24 >> 1, p = r24 & 1.   d = golay23_decode(r23).
//   expected parity of d.codeword = popcount(d.codeword) & 1;   m = (expected != p) ? 1 : 0.
//   total = d.errors + m.     total == 0 -> Ok;   1..3 -> Corrected;   >= 4 -> Uncorrectable.
// Why that detects every 4-error pattern: worksheet Q3.
inline Golay24Result golay24_decode(std::uint32_t /*received24*/) { lmr::todo("golay24_decode"); }

}  // namespace lmrb
