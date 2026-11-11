// Tests for Pack 02. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

#include "byteorder.h"

using namespace byteorder;

// ---------- host order (W 3.2) ----------
TEST(HostOrder, AgreesWithMemoryInspection) {
  const uint32_t probe = 0x11223344u;
  uint8_t bytes[4];
  std::memcpy(bytes, &probe, 4);
  const bool little = (bytes[0] == 0x44);
  EXPECT_EQ(host_is_little_endian(), little);
}

// ---------- bswap (W 3.5) ----------
TEST(Bswap, KnownValues) {
  EXPECT_EQ(bswap16(0x1234u), 0x3412u);
  EXPECT_EQ(bswap16(0x00FFu), 0xFF00u);
  EXPECT_EQ(bswap32(0x11223344u), 0x44332211u);
  EXPECT_EQ(bswap32(0xA1B2C3D4u), 0xD4C3B2A1u);
  EXPECT_EQ(bswap32(0x000000FFu), 0xFF000000u);
}
TEST(Bswap, IsItsOwnInverse) {
  for (uint32_t v = 0; v < 0x10000u; v += 97) EXPECT_EQ(bswap16(bswap16(static_cast<uint16_t>(v))), v);
  uint32_t s = 0xDEADBEEFu;
  for (int i = 0; i < 1000; ++i) {
    s = s * 1664525u + 1013904223u;
    EXPECT_EQ(bswap32(bswap32(s)), s);
  }
}

// ---------- htons / ntohl family (W 3.4) ----------
// Host-independent check: whatever the host, the *memory image* of htons(x) must be big-endian.
TEST(Hton, MemoryImageIsBigEndian16) {
  const uint16_t n = my_htons(0x1234u);
  uint8_t b[2];
  std::memcpy(b, &n, 2);
  EXPECT_EQ(b[0], 0x12);
  EXPECT_EQ(b[1], 0x34);
}
TEST(Hton, MemoryImageIsBigEndian32) {
  const uint32_t n = my_htonl(0x11223344u);
  uint8_t b[4];
  std::memcpy(b, &n, 4);
  EXPECT_EQ(b[0], 0x11);
  EXPECT_EQ(b[1], 0x22);
  EXPECT_EQ(b[2], 0x33);
  EXPECT_EQ(b[3], 0x44);
}
TEST(Hton, RtpPort5004) {
  const uint16_t n = my_htons(5004);  // 0x138C
  uint8_t b[2];
  std::memcpy(b, &n, 2);
  EXPECT_EQ(b[0], 0x13);
  EXPECT_EQ(b[1], 0x8C);
}
TEST(Hton, RoundTrip) {
  for (uint32_t v = 0; v < 0x10000u; v += 101) EXPECT_EQ(my_ntohs(my_htons(static_cast<uint16_t>(v))), v);
  EXPECT_EQ(my_ntohl(my_htonl(0xCAFEBABEu)), 0xCAFEBABEu);
}
TEST(Hton, MatchesBswapOnLittleEndianHosts) {
  if (host_is_little_endian()) {
    EXPECT_EQ(my_htons(0x1234u), 0x3412u);
    EXPECT_EQ(my_htonl(0x11223344u), 0x44332211u);
  } else {
    EXPECT_EQ(my_htons(0x1234u), 0x1234u);
    EXPECT_EQ(my_htonl(0x11223344u), 0x11223344u);
  }
}

// ---------- load / store (W 3.6) ----------
TEST(LoadStore, LoadBigAndLittle) {
  const uint8_t buf[4] = {0x12, 0x34, 0x56, 0x78};
  EXPECT_EQ(load_be16(buf), 0x1234u);
  EXPECT_EQ(load_le16(buf), 0x3412u);
  EXPECT_EQ(load_be32(buf), 0x12345678u);
  EXPECT_EQ(load_le32(buf), 0x78563412u);
}
TEST(LoadStore, LoadHighByteDoesNotSignExtend) {
  const uint8_t buf[4] = {0xFF, 0xFE, 0xFD, 0xFC};
  EXPECT_EQ(load_be32(buf), 0xFFFEFDFCu);
  EXPECT_EQ(load_le32(buf), 0xFCFDFEFFu);
  EXPECT_EQ(load_be16(buf), 0xFFFEu);
}
TEST(LoadStore, StoreBig) {
  uint8_t b[4] = {0xEE, 0xEE, 0xEE, 0xEE};
  store_be32(b, 0x11223344u);
  EXPECT_EQ(b[0], 0x11);
  EXPECT_EQ(b[1], 0x22);
  EXPECT_EQ(b[2], 0x33);
  EXPECT_EQ(b[3], 0x44);
  store_be16(b, 0x138Cu);
  EXPECT_EQ(b[0], 0x13);
  EXPECT_EQ(b[1], 0x8C);
  EXPECT_EQ(b[2], 0x33);  // untouched
}
TEST(LoadStore, StoreLittle) {
  uint8_t b[4] = {0xEE, 0xEE, 0xEE, 0xEE};
  store_le32(b, 0x11223344u);
  EXPECT_EQ(b[0], 0x44);
  EXPECT_EQ(b[1], 0x33);
  EXPECT_EQ(b[2], 0x22);
  EXPECT_EQ(b[3], 0x11);
  store_le16(b, 0x138Cu);
  EXPECT_EQ(b[0], 0x8C);
  EXPECT_EQ(b[1], 0x13);
}
TEST(LoadStore, RoundTrip) {
  uint8_t b[4];
  uint32_t s = 7;
  for (int i = 0; i < 500; ++i) {
    s = s * 1664525u + 1013904223u;
    store_be32(b, s);
    EXPECT_EQ(load_be32(b), s);
    store_le32(b, s);
    EXPECT_EQ(load_le32(b), s);
    store_be16(b, static_cast<uint16_t>(s));
    EXPECT_EQ(load_be16(b), static_cast<uint16_t>(s));
    store_le16(b, static_cast<uint16_t>(s));
    EXPECT_EQ(load_le16(b), static_cast<uint16_t>(s));
  }
}
TEST(LoadStore, BeAndLeAreByteReversals) {
  uint8_t a[4], b[4];
  store_be32(a, 0xA1B2C3D4u);
  store_le32(b, 0xA1B2C3D4u);
  for (int i = 0; i < 4; ++i) EXPECT_EQ(a[i], b[3 - i]);
}

// ---------- sequence arithmetic (W 3.7) ----------
TEST(Seq16, SimpleOrdering) {
  EXPECT_TRUE(seq_newer16(1, 0));
  EXPECT_FALSE(seq_newer16(0, 1));
  EXPECT_FALSE(seq_newer16(7, 7));
}
TEST(Seq16, WraparoundIsHandled) {
  EXPECT_TRUE(seq_newer16(0, 65535));
  EXPECT_FALSE(seq_newer16(65535, 0));
  EXPECT_TRUE(seq_newer16(100, 65500));
  EXPECT_EQ(seq_diff16(1, 65535), 2);
  EXPECT_EQ(seq_diff16(65535, 1), -2);
  EXPECT_EQ(seq_diff16(0, 0), 0);
}
TEST(Seq16, HalfTurnBoundary) {
  EXPECT_TRUE(seq_newer16(32767, 0));    // 32767 ahead: newer
  EXPECT_FALSE(seq_newer16(0, 32767));
  EXPECT_FALSE(seq_newer16(0x8000, 0));  // exactly half a turn: undefined in RFC 1982 -> false both ways
  EXPECT_FALSE(seq_newer16(0, 0x8000));
  EXPECT_EQ(seq_diff16(0x8000, 0), -32768);
  EXPECT_EQ(seq_diff16(0, 0x8000), -32768);
  EXPECT_TRUE(seq_newer16(0, 0x8001));   // 32767 ahead going the short way round
}
TEST(Seq16, MatchesRfc1982Definition) {
  // RFC 1982 sec. 3.2 with SERIAL_BITS = 16: i1 < i2 iff
  //   (i1 < i2 && i2 - i1 < 2^15) || (i1 > i2 && i1 - i2 > 2^15).   "a newer than b" == (b < a)
  for (uint32_t a = 0; a < 65536u; a += 257) {
    for (uint32_t b = 0; b < 65536u; b += 251) {
      const bool rfc = (b < a && a - b < 32768u) || (b > a && b - a > 32768u);
      EXPECT_EQ(seq_newer16(static_cast<uint16_t>(a), static_cast<uint16_t>(b)), rfc) << a << "," << b;
    }
  }
}
TEST(Seq16, AntiSymmetric) {
  for (uint32_t a = 0; a < 65536u; a += 509) {
    for (uint32_t b = 0; b < 65536u; b += 503) {
      const auto A = static_cast<uint16_t>(a), B = static_cast<uint16_t>(b);
      EXPECT_FALSE(seq_newer16(A, B) && seq_newer16(B, A));
    }
  }
}
TEST(Seq32, Wraparound) {
  EXPECT_TRUE(seq_newer32(0u, 0xFFFFFFFFu));
  EXPECT_FALSE(seq_newer32(0xFFFFFFFFu, 0u));
  EXPECT_EQ(seq_diff32(5u, 0xFFFFFFFEu), 7);
  EXPECT_FALSE(seq_newer32(0x80000000u, 0u));
  EXPECT_FALSE(seq_newer32(0u, 0x80000000u));
  EXPECT_TRUE(seq_newer32(0x7FFFFFFFu, 0u));
}

// ---------- SeqExtender (W 3.8) ----------
TEST(SeqExtender, WrapAroundMonotonic) {
  SeqExtender e;
  EXPECT_EQ(e.update(65534), 65534);
  EXPECT_EQ(e.update(65535), 65535);
  EXPECT_EQ(e.update(0), 65536);
  EXPECT_EQ(e.update(1), 65537);
}
TEST(SeqExtender, ReorderedPacketsGetEarlierValuesAndDoNotMoveHighest) {
  SeqExtender e;
  e.update(65534);
  e.update(65535);
  EXPECT_EQ(e.update(0), 65536);
  EXPECT_EQ(e.update(65535), 65535);  // late arrival from before the wrap
  EXPECT_EQ(e.highest(), 65536);
  EXPECT_EQ(e.update(1), 65537);
  EXPECT_EQ(e.update(3), 65539);
  EXPECT_EQ(e.update(2), 65538);  // reordered
  EXPECT_EQ(e.highest(), 65539);
}
TEST(SeqExtender, FirstPacketThenOlderAcrossZero) {
  SeqExtender e;
  EXPECT_EQ(e.update(0), 0);
  EXPECT_EQ(e.update(65535), -1);  // one before the first packet we ever saw
  EXPECT_EQ(e.highest(), 0);
}
TEST(SeqExtender, ManyCycles) {
  SeqExtender e;
  int64_t expect = 40000;
  EXPECT_EQ(e.update(40000), expect);
  for (int i = 0; i < 200000; ++i) {
    ++expect;
    ASSERT_EQ(e.update(static_cast<uint16_t>(expect & 0xFFFF)), expect);
  }
}
