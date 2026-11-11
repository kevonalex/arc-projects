#pragma once
// STUB - worksheet 04, Q4/Q5/Q11 : dPMR (6.25 kHz FDMA) rate arithmetic and MPT 1327 (1200 bit/s FFSK).
// SYNTHETIC ONLY.  dPMR: ETSI TS 102 490 / TS 102 658 (VERIFY numbers).  MPT 1327 / MPT 1343 (UK, VERIFY).
#include <cstdint>

#include "lmr/todo.hpp"

namespace lmr_legacy {

// ---------------- dPMR ----------------
inline constexpr int kDpmrSymbolRate = 2400;  // 4FSK
inline constexpr int kDpmrBitsPerSymbol = 2;
inline constexpr double kDpmrChannelHz = 6250.0;

// TODO: 2400 * 2 = 4800
inline int dpmr_gross_bit_rate() { lmr::todo("dpmr_gross_bit_rate"); }

// TODO: the bit rate ONE call gets in DMR = (9600 bit/s carrier) / (2 slots) = 4800
inline int dmr_per_call_bit_rate() { lmr::todo("dmr_per_call_bit_rate"); }

// TODO: bit/s per Hz
inline double bits_per_hz(double /*bit_rate*/, double /*bandwidth_hz*/) { lmr::todo("bits_per_hz"); }

// ---------------- MPT 1327 ----------------
inline constexpr int kMptBitRate = 1200;
inline constexpr std::uint16_t kMptSync = 0xC4D7;   // control channel SYNC word (VERIFY)
inline constexpr std::uint16_t kMptSyncT = 0x3B28;  // traffic channel SYNCT word = ~SYNC (VERIFY)

// Check-bit polynomial g(x) = x^15 + x^14 + x^13 + x^11 + x^4 + x^2 + 1  -> 0xE815 (VERIFY)
inline constexpr std::uint32_t kMptGen = 0xE815;

// TODO: FFSK tone spacing for orthogonal/minimum-shift signalling = baud / 2
inline double ffsk_tone_spacing_hz(double /*baud*/) { lmr::todo("ffsk_tone_spacing_hz"); }

// TODO: how many cycles of a tone fit in one bit period: tone_hz / baud
inline double cycles_per_bit(double /*tone_hz*/, double /*baud*/) { lmr::todo("cycles_per_bit"); }

// TODO: duration of a 64-bit codeword at 1200 bit/s in ms (53.33)
inline double mpt_codeword_ms() { lmr::todo("mpt_codeword_ms"); }

// TODO: 15 check bits = remainder of (info48 << 15) divided by g (degree 15).
inline std::uint32_t mpt_crc15(std::uint64_t /*info48*/) { lmr::todo("mpt_crc15"); }

// TODO: TEACHING codeword: (info48 << 16) | (crc15 << 1) | p, where p makes the TOTAL weight even.
//       (Real MPT 1327 complements the check bits - VERIFY - this pack does not.)
inline std::uint64_t mpt_encode(std::uint64_t /*info48*/) { lmr::todo("mpt_encode"); }

// TODO: valid iff total weight is even AND (cw >> 1) is divisible by g (remainder 0).
inline bool mpt_check(std::uint64_t /*cw64*/) { lmr::todo("mpt_check"); }

}  // namespace lmr_legacy
