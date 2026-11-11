// Tests for Pack 04. Same file is compiled against the stubs and the solutions.
// Expected values were cross-checked against an independent bit-level implementation (VERIFY against
// the Rocksoft/RevEng CRC catalogue: https://reveng.sourceforge.io/crc-catalogue/ ).
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "crcsum.h"

using namespace crc;

static const uint8_t* bytes_of(const std::string& s) { return reinterpret_cast<const uint8_t*>(s.data()); }
static const std::string k123 = "123456789";

// ---------------------------------------------------------------- GF(2) division (W 3.2)
TEST(Gf2, WorkedExampleCrc3) {
  // x^3 + x + 1 = 0b1011.  message 1101 -> append 000 -> 1101000 (7 bits)
  EXPECT_EQ(gf2_remainder(0b1101000, 7, 0b1011, 3), 0b001u);
  // codeword = message | remainder = 1101001 must divide evenly
  EXPECT_EQ(gf2_remainder(0b1101001, 7, 0b1011, 3), 0u);
}
TEST(Gf2, PowersOfXModG) {
  // x^i mod (x^3+x+1) for i = 0..7: 001 010 100 011 110 111 101 001   (period 7)
  const uint32_t expect[8] = {1, 2, 4, 3, 6, 7, 5, 1};
  for (unsigned i = 0; i < 8; ++i) {
    EXPECT_EQ(gf2_remainder(1ull << i, i + 1, 0b1011, 3), expect[i]) << "i=" << i;
  }
}
TEST(Gf2, EveryFourBitMessageGivesDivisibleCodeword) {
  for (uint32_t m = 0; m < 16; ++m) {
    const uint32_t rem = gf2_remainder(static_cast<uint64_t>(m) << 3, 7, 0b1011, 3);
    const uint32_t cw = (m << 3) | rem;
    EXPECT_EQ(gf2_remainder(cw, 7, 0b1011, 3), 0u) << "m=" << m;
  }
}
TEST(Gf2, SingleBitErrorsGiveDistinctNonzeroSyndromes) {
  const uint32_t cw = 0b1101001;
  bool seen[8] = {false};
  for (unsigned pos = 0; pos < 7; ++pos) {
    const uint32_t s = gf2_remainder(cw ^ (1u << pos), 7, 0b1011, 3);
    EXPECT_NE(s, 0u);
    EXPECT_FALSE(seen[s]) << "syndrome repeated for pos " << pos;
    seen[s] = true;
    EXPECT_EQ(s, gf2_remainder(1ull << pos, pos + 1, 0b1011, 3));  // syndrome == x^pos mod g
  }
}
TEST(Gf2, LinearityOfRemainder) {  // rem(a xor b) == rem(a) xor rem(b): XOR is "subtraction"
  for (uint32_t a = 0; a < 128; a += 7) {
    for (uint32_t b = 0; b < 128; b += 5) {
      EXPECT_EQ(gf2_remainder(a ^ b, 7, 0b1011, 3), gf2_remainder(a, 7, 0b1011, 3) ^ gf2_remainder(b, 7, 0b1011, 3));
    }
  }
}

// ---------------------------------------------------------------- reflect (W 3.4)
TEST(Reflect, Examples) {
  EXPECT_EQ(reflect_bits(0x01, 8), 0x80u);
  EXPECT_EQ(reflect_bits(0xA5, 8), 0xA5u);  // 1010 0101 is a palindrome
  EXPECT_EQ(reflect_bits(0x1234, 16), 0x2C48u);
  EXPECT_EQ(reflect_bits(0x00000001u, 32), 0x80000000u);
  EXPECT_EQ(reflect_bits(0x04C11DB7u, 32), 0xEDB88320u);  // CRC-32 poly <-> reflected poly
  EXPECT_EQ(reflect_bits(0x1021, 16), 0x8408u);
}
TEST(Reflect, IsAnInvolution) {
  for (uint32_t v = 0; v < 65536u; v += 251) EXPECT_EQ(reflect_bits(reflect_bits(v, 16), 16), v);
}

// ---------------------------------------------------------------- check values "123456789" (W 3.4, 3.6)
TEST(Generic, CatalogueCheckValues) {
  const uint8_t* d = bytes_of(k123);
  EXPECT_EQ(crc_generic(kCrc16CcittFalse, d, 9), 0x29B1u);
  EXPECT_EQ(crc_generic(kCrc16Xmodem, d, 9), 0x31C3u);
  EXPECT_EQ(crc_generic(kCrc16Kermit, d, 9), 0x2189u);
  EXPECT_EQ(crc_generic(kCrc16X25, d, 9), 0x906Eu);
  EXPECT_EQ(crc_generic(kCrc16Arc, d, 9), 0xBB3Du);
  EXPECT_EQ(crc_generic(kCrc32, d, 9), 0xCBF43926u);
  EXPECT_EQ(crc_generic(kCrc32C, d, 9), 0xE3069283u);
}
TEST(Generic, EmptyInputIsInitXorXorout) {
  EXPECT_EQ(crc_generic(kCrc16CcittFalse, nullptr, 0), 0xFFFFu);
  EXPECT_EQ(crc_generic(kCrc32, nullptr, 0), 0u);  // 0xFFFFFFFF ^ 0xFFFFFFFF
  EXPECT_EQ(crc_generic(kCrc16Xmodem, nullptr, 0), 0u);
}
TEST(Generic, OnlyParametersDifferBetweenCcittVariants) {
  // Same polynomial 0x1021, four different answers: the parameters ARE the algorithm.
  const uint8_t* d = bytes_of(k123);
  std::vector<uint32_t> v = {crc_generic(kCrc16CcittFalse, d, 9), crc_generic(kCrc16Xmodem, d, 9),
                             crc_generic(kCrc16Kermit, d, 9), crc_generic(kCrc16X25, d, 9)};
  for (size_t i = 0; i < v.size(); ++i)
    for (size_t j = i + 1; j < v.size(); ++j) EXPECT_NE(v[i], v[j]);
}

// ---------------------------------------------------------------- roles of init / xorout (W 3.4)
TEST(Roles, ZeroInitCannotSeeLeadingZeroBytes) {
  const uint8_t a[2] = {0x00, 0x01};
  const uint8_t b[1] = {0x01};
  EXPECT_EQ(crc_generic(kCrc16Xmodem, a, 2), crc_generic(kCrc16Xmodem, b, 1));  // both 0x1021: blind!
  EXPECT_EQ(crc_generic(kCrc16Xmodem, b, 1), 0x1021u);
  EXPECT_EQ(crc_generic(kCrc16CcittFalse, a, 2), 0x0D2Eu);
  EXPECT_EQ(crc_generic(kCrc16CcittFalse, b, 1), 0xF1D1u);  // init 0xFFFF sees the extra 0x00
}
TEST(Roles, ResidueWhenCrcIsAppended) {
  // CRC-16/CCITT-FALSE, CRC appended big-endian: running the CRC over message+crc gives 0.
  const std::string m = k123;
  std::vector<uint8_t> v(m.begin(), m.end());
  const uint16_t c = static_cast<uint16_t>(crc_generic(kCrc16CcittFalse, v.data(), v.size()));
  v.push_back(static_cast<uint8_t>(c >> 8));
  v.push_back(static_cast<uint8_t>(c & 0xFF));
  EXPECT_EQ(crc_generic(kCrc16CcittFalse, v.data(), v.size()), 0u);
  v.push_back(0x00);  // trailing garbage zero byte: residue is STILL 0 -> undetected (xorout = 0 weakness)
  EXPECT_EQ(crc_generic(kCrc16CcittFalse, v.data(), v.size()), 0u);
}
TEST(Roles, Crc32ResidueIsAConstantAndExtraZeroIsDetected) {
  std::vector<uint8_t> v(k123.begin(), k123.end());
  const uint32_t c = crc_generic(kCrc32, v.data(), v.size());
  for (int i = 0; i < 4; ++i) v.push_back(static_cast<uint8_t>((c >> (8 * i)) & 0xFF));  // little-endian
  EXPECT_EQ(crc_generic(kCrc32, v.data(), v.size()), 0x2144DF1Cu);  // fixed residue (xorout applied)
  v.push_back(0x00);
  EXPECT_NE(crc_generic(kCrc32, v.data(), v.size()), 0x2144DF1Cu);  // nonzero xorout: stray byte detected
}
TEST(Roles, X25ResidueIs0x0F47) {
  std::vector<uint8_t> v(k123.begin(), k123.end());
  const uint16_t c = static_cast<uint16_t>(crc_generic(kCrc16X25, v.data(), v.size()));
  v.push_back(static_cast<uint8_t>(c & 0xFF));  // reflected CRC: low byte first
  v.push_back(static_cast<uint8_t>(c >> 8));
  EXPECT_EQ(crc_generic(kCrc16X25, v.data(), v.size()), 0x0F47u);
}
TEST(Roles, LinearityWithZeroInitZeroXorout) {  // CRC(a xor b) == CRC(a) xor CRC(b), equal lengths
  const uint8_t a[4] = {0x12, 0x34, 0x56, 0x78};
  const uint8_t b[4] = {0xA5, 0x5A, 0xFF, 0x00};
  uint8_t x[4];
  for (int i = 0; i < 4; ++i) x[i] = a[i] ^ b[i];
  EXPECT_EQ(crc_generic(kCrc16Xmodem, x, 4), crc_generic(kCrc16Xmodem, a, 4) ^ crc_generic(kCrc16Xmodem, b, 4));
}

// ---------------------------------------------------------------- bitwise vs table (W 3.5)
TEST(Crc16Ccitt, BitwiseKnownValues) {
  EXPECT_EQ(crc16_ccitt_false(bytes_of(k123), 9), 0x29B1u);
  const std::string fox = "The quick brown fox jumps over the lazy dog";
  EXPECT_EQ(crc16_ccitt_false(bytes_of(fox), fox.size()), 0x8FDDu);
  const uint8_t d[4] = {1, 2, 3, 4};
  EXPECT_EQ(crc16_ccitt_false(d, 4), 0x89C3u);
  EXPECT_EQ(crc16_ccitt_false(nullptr, 0), 0xFFFFu);
}
TEST(Crc16Ccitt, TableEntries) {
  const auto t = make_crc16_table();
  EXPECT_EQ(t[0], 0x0000u);
  EXPECT_EQ(t[1], 0x1021u);
  EXPECT_EQ(t[2], 0x2042u);
  EXPECT_EQ(t[128], 0x9188u);
  EXPECT_EQ(t[255], 0x1EF0u);
}
TEST(Crc16Ccitt, TableDrivenEqualsBitwise) {
  EXPECT_EQ(crc16_ccitt_false_table(bytes_of(k123), 9), 0x29B1u);
  uint32_t s = 2024;
  std::vector<uint8_t> buf;
  for (int len = 0; len < 200; ++len) {
    EXPECT_EQ(crc16_ccitt_false_table(buf.data(), buf.size()), crc16_ccitt_false(buf.data(), buf.size()));
    EXPECT_EQ(crc16_ccitt_false(buf.data(), buf.size()), crc_generic(kCrc16CcittFalse, buf.data(), buf.size()));
    s = s * 1664525u + 1013904223u;
    buf.push_back(static_cast<uint8_t>(s >> 24));
  }
}

// ---------------------------------------------------------------- CRC-32 (W 3.5, 3.6)
TEST(Crc32, KnownValues) {
  EXPECT_EQ(crc32_bitwise(bytes_of(k123), 9), 0xCBF43926u);
  const std::string a = "a";
  EXPECT_EQ(crc32_bitwise(bytes_of(a), 1), 0xE8B7BE43u);
  const std::string fox = "The quick brown fox jumps over the lazy dog";
  EXPECT_EQ(crc32_bitwise(bytes_of(fox), fox.size()), 0x414FA339u);
  const uint8_t d[4] = {1, 2, 3, 4};
  EXPECT_EQ(crc32_bitwise(d, 4), 0xB63CFBCDu);
  EXPECT_EQ(crc32_bitwise(nullptr, 0), 0u);
}
TEST(Crc32, TableEntries) {
  const auto t = make_crc32_table();
  EXPECT_EQ(t[0], 0u);
  EXPECT_EQ(t[1], 0x77073096u);
  EXPECT_EQ(t[2], 0xEE0E612Cu);
  EXPECT_EQ(t[128], 0xEDB88320u);  // the reflected polynomial itself
  EXPECT_EQ(t[255], 0x2D02EF8Du);
}
TEST(Crc32, TableEqualsBitwiseEqualsGeneric) {
  uint32_t s = 7;
  std::vector<uint8_t> buf;
  for (int len = 0; len < 300; ++len) {
    const uint32_t a = crc32_table(buf.data(), buf.size());
    EXPECT_EQ(a, crc32_bitwise(buf.data(), buf.size()));
    EXPECT_EQ(a, crc_generic(kCrc32, buf.data(), buf.size()));
    s = s * 1664525u + 1013904223u;
    buf.push_back(static_cast<uint8_t>(s >> 24));
  }
}
TEST(Crc32, DetectsEverySingleBitFlipIn64ByteMessage) {
  std::vector<uint8_t> v(64);
  for (size_t i = 0; i < v.size(); ++i) v[i] = static_cast<uint8_t>(i * 7 + 3);
  const uint32_t good = crc32_table(v.data(), v.size());
  for (size_t bit = 0; bit < v.size() * 8; ++bit) {
    v[bit / 8] ^= static_cast<uint8_t>(1u << (bit % 8));
    EXPECT_NE(crc32_table(v.data(), v.size()), good) << "bit " << bit;
    v[bit / 8] ^= static_cast<uint8_t>(1u << (bit % 8));
  }
}

// ---------------------------------------------------------------- Internet checksum (W 3.8)
TEST(InetChecksum, Rfc1071Example) {
  const uint8_t d[8] = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
  EXPECT_EQ(ones_complement_sum16(d, 8), 0xDDF2u);
  EXPECT_EQ(internet_checksum(d, 8), 0x220Du);
}
TEST(InetChecksum, Ipv4HeaderExample) {
  uint8_t h[20] = {0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                   0x00, 0x00, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7};  // checksum field zeroed
  EXPECT_EQ(internet_checksum(h, 20), 0xB861u);
  h[10] = 0xB8;
  h[11] = 0x61;  // fill it in
  EXPECT_EQ(ones_complement_sum16(h, 20), 0xFFFFu);
  EXPECT_TRUE(internet_checksum_ok(h, 20));
  h[15] ^= 0x01;  // corrupt one bit of the source address
  EXPECT_FALSE(internet_checksum_ok(h, 20));
}
TEST(InetChecksum, OddLengthPadsLowByteWithZero) {
  const uint8_t d[3] = {0x01, 0x02, 0x03};
  EXPECT_EQ(ones_complement_sum16(d, 3), 0x0402u);  // 0x0102 + 0x0300
  EXPECT_EQ(internet_checksum(d, 3), 0xFBFDu);
}
TEST(InetChecksum, EndAroundCarry) {
  const uint8_t d[4] = {0xFF, 0xFF, 0x00, 0x01};
  EXPECT_EQ(ones_complement_sum16(d, 4), 0x0001u);  // 0x10000 folds to 1
}
TEST(InetChecksum, AllZeroDataChecksumIsFFFF) {
  const uint8_t d[4] = {0, 0, 0, 0};
  EXPECT_EQ(ones_complement_sum16(d, 4), 0u);
  EXPECT_EQ(internet_checksum(d, 4), 0xFFFFu);
}
TEST(InetChecksum, ByteOrderIndependence) {
  // Byte-swapping every 16-bit word byte-swaps the sum (RFC 1071 section 2(B)).
  const uint8_t a[8] = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
  const uint8_t b[8] = {0x01, 0x00, 0x03, 0xf2, 0xf5, 0xf4, 0xf7, 0xf6};  // same words, bytes swapped
  const uint16_t sa = ones_complement_sum16(a, 8);
  const uint16_t sb = ones_complement_sum16(b, 8);
  EXPECT_EQ(sb, static_cast<uint16_t>((sa << 8) | (sa >> 8)));
  EXPECT_EQ(sb, 0xF2DDu);
}
TEST(InetChecksum, WordOrderIsInvisibleWeakness) {
  // Addition commutes, so reordering 16-bit words is NOT detected (unlike a CRC).
  const uint8_t a[4] = {0x00, 0x01, 0x00, 0x02};
  const uint8_t b[4] = {0x00, 0x02, 0x00, 0x01};
  EXPECT_EQ(internet_checksum(a, 4), internet_checksum(b, 4));
  EXPECT_NE(crc16_ccitt_false(a, 4), crc16_ccitt_false(b, 4));
}
