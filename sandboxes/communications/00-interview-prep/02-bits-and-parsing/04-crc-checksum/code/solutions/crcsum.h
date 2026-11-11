// crcsum.h  --  REFERENCE SOLUTION for Pack 04.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace crc {

// Long division in GF(2): no borrows, subtraction == XOR (W 3.2).
inline uint32_t gf2_remainder(uint64_t dividend, unsigned dividend_bits, uint32_t poly_full,
                              unsigned degree) {
  if (degree == 0 || degree > 31 || dividend_bits > 64) return 0;
  for (unsigned i = dividend_bits; i-- > degree;) {  // i = index of the current leading position
    if ((dividend >> i) & 1u) {
      dividend ^= static_cast<uint64_t>(poly_full) << (i - degree);  // divisor aligned under bit i
    }
  }
  return static_cast<uint32_t>(dividend & ((1ull << degree) - 1ull));
}

inline uint32_t reflect_bits(uint32_t v, unsigned width) {
  uint32_t r = 0;
  for (unsigned i = 0; i < width; ++i) {
    if ((v >> i) & 1u) r |= 1u << (width - 1u - i);
  }
  return r;
}

struct CrcParams {
  unsigned width;
  uint32_t poly;
  uint32_t init;
  bool refin;
  bool refout;
  uint32_t xorout;
};
inline constexpr CrcParams kCrc16CcittFalse{16, 0x1021, 0xFFFF, false, false, 0x0000};
inline constexpr CrcParams kCrc16Xmodem{16, 0x1021, 0x0000, false, false, 0x0000};
inline constexpr CrcParams kCrc16Kermit{16, 0x1021, 0x0000, true, true, 0x0000};
inline constexpr CrcParams kCrc16X25{16, 0x1021, 0xFFFF, true, true, 0xFFFF};
inline constexpr CrcParams kCrc16Arc{16, 0x8005, 0x0000, true, true, 0x0000};
inline constexpr CrcParams kCrc32{32, 0x04C11DB7, 0xFFFFFFFFu, true, true, 0xFFFFFFFFu};
inline constexpr CrcParams kCrc32C{32, 0x1EDC6F41, 0xFFFFFFFFu, true, true, 0xFFFFFFFFu};

inline uint32_t crc_generic(const CrcParams& p, const uint8_t* data, size_t len) {
  const uint64_t mask = (p.width == 32) ? 0xFFFFFFFFull : ((1ull << p.width) - 1ull);
  const uint64_t top = 1ull << (p.width - 1);
  uint64_t reg = p.init & mask;
  for (size_t n = 0; n < len; ++n) {
    uint32_t b = data[n];
    if (p.refin) b = reflect_bits(b, 8);
    reg ^= static_cast<uint64_t>(b) << (p.width - 8);  // feed the byte into the top of the register
    for (int k = 0; k < 8; ++k) {
      reg = (reg & top) ? ((reg << 1) ^ p.poly) : (reg << 1);  // shift; subtract poly if a 1 fell out
      reg &= mask;
    }
  }
  uint32_t out = static_cast<uint32_t>(reg);
  if (p.refout) out = reflect_bits(out, p.width);
  return (out ^ p.xorout) & static_cast<uint32_t>(mask);
}

inline uint16_t crc16_ccitt_false(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t n = 0; n < len; ++n) {
    crc = static_cast<uint16_t>(crc ^ (static_cast<uint16_t>(data[n]) << 8));
    for (int k = 0; k < 8; ++k) {
      crc = (crc & 0x8000u) ? static_cast<uint16_t>((crc << 1) ^ 0x1021u) : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}
inline std::array<uint16_t, 256> make_crc16_table() {
  std::array<uint16_t, 256> t{};
  for (unsigned i = 0; i < 256; ++i) {
    uint16_t c = static_cast<uint16_t>(i << 8);
    for (int k = 0; k < 8; ++k) {
      c = (c & 0x8000u) ? static_cast<uint16_t>((c << 1) ^ 0x1021u) : static_cast<uint16_t>(c << 1);
    }
    t[i] = c;
  }
  return t;
}
inline uint16_t crc16_ccitt_false_table(const uint8_t* data, size_t len) {
  static const std::array<uint16_t, 256> table = make_crc16_table();
  uint16_t crc = 0xFFFF;
  for (size_t n = 0; n < len; ++n) {
    const uint8_t idx = static_cast<uint8_t>((crc >> 8) ^ data[n]);
    crc = static_cast<uint16_t>((crc << 8) ^ table[idx]);
  }
  return crc;
}

inline uint32_t crc32_bitwise(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t n = 0; n < len; ++n) {
    crc ^= data[n];
    for (int k = 0; k < 8; ++k) crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
  }
  return ~crc;
}
inline std::array<uint32_t, 256> make_crc32_table() {
  std::array<uint32_t, 256> t{};
  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i;
    for (int k = 0; k < 8; ++k) c = (c & 1u) ? ((c >> 1) ^ 0xEDB88320u) : (c >> 1);
    t[i] = c;
  }
  return t;
}
inline uint32_t crc32_table(const uint8_t* data, size_t len) {
  static const std::array<uint32_t, 256> table = make_crc32_table();
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t n = 0; n < len; ++n) crc = table[(crc ^ data[n]) & 0xFFu] ^ (crc >> 8);
  return ~crc;
}

inline uint16_t ones_complement_sum16(const uint8_t* data, size_t len) {
  uint64_t sum = 0;  // 64-bit accumulator: cannot overflow for any realistic buffer
  size_t i = 0;
  for (; i + 1 < len; i += 2) sum += (static_cast<uint32_t>(data[i]) << 8) | data[i + 1];
  if (i < len) sum += static_cast<uint32_t>(data[i]) << 8;  // odd byte: pad with zero low byte
  while (sum >> 16) sum = (sum & 0xFFFFu) + (sum >> 16);   // end-around carry
  return static_cast<uint16_t>(sum);
}
inline uint16_t internet_checksum(const uint8_t* data, size_t len) {
  return static_cast<uint16_t>(~ones_complement_sum16(data, len) & 0xFFFFu);
}
inline bool internet_checksum_ok(const uint8_t* data, size_t len) {
  return ones_complement_sum16(data, len) == 0xFFFFu;
}

}  // namespace crc
