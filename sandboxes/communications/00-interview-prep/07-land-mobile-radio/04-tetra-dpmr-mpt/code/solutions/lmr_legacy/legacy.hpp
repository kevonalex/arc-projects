#pragma once
// SOLUTION - worksheet 04, Q4/Q5/Q11.
#include <bitset>
#include <cstdint>

#include "lmr/todo.hpp"

namespace lmr_legacy {

inline constexpr int kDpmrSymbolRate = 2400;
inline constexpr int kDpmrBitsPerSymbol = 2;
inline constexpr double kDpmrChannelHz = 6250.0;

inline int dpmr_gross_bit_rate() { return kDpmrSymbolRate * kDpmrBitsPerSymbol; }

inline int dmr_per_call_bit_rate() { return 9600 / 2; }

inline double bits_per_hz(double bit_rate, double bandwidth_hz) { return bit_rate / bandwidth_hz; }

inline constexpr int kMptBitRate = 1200;
inline constexpr std::uint16_t kMptSync = 0xC4D7;
inline constexpr std::uint16_t kMptSyncT = 0x3B28;
inline constexpr std::uint32_t kMptGen = 0xE815;

inline double ffsk_tone_spacing_hz(double baud) { return baud / 2.0; }

inline double cycles_per_bit(double tone_hz, double baud) { return tone_hz / baud; }

inline double mpt_codeword_ms() { return 1000.0 * 64.0 / kMptBitRate; }

inline std::uint32_t mpt_crc15(std::uint64_t info48) {
  std::uint64_t v = (info48 & ((1ULL << 48) - 1)) << 15;  // 63 bits: positions 0..62
  for (int i = 62; i >= 15; --i) {
    if ((v >> i) & 1ULL) v ^= static_cast<std::uint64_t>(kMptGen) << (i - 15);
  }
  return static_cast<std::uint32_t>(v & 0x7FFFu);
}

inline std::uint64_t mpt_encode(std::uint64_t info48) {
  info48 &= (1ULL << 48) - 1;
  const std::uint64_t body = (info48 << 15) | mpt_crc15(info48);  // 63 bits
  const std::uint64_t parity = std::bitset<64>(body).count() & 1ULL;
  return (body << 1) | parity;
}

inline bool mpt_check(std::uint64_t cw64) {
  if (std::bitset<64>(cw64).count() % 2 != 0) return false;
  std::uint64_t v = cw64 >> 1;  // 63-bit body
  for (int i = 62; i >= 15; --i) {
    if ((v >> i) & 1ULL) v ^= static_cast<std::uint64_t>(kMptGen) << (i - 15);
  }
  return v == 0;
}

}  // namespace lmr_legacy
