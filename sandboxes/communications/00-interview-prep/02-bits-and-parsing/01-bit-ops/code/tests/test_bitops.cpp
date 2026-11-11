// Tests for Pack 01. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cstdint>

#include "bitops.h"

using namespace bits;

// ---------- low_mask (W 3.3) ----------
TEST(LowMask, KnownValues) {
  EXPECT_EQ(low_mask(0), 0x0u);
  EXPECT_EQ(low_mask(1), 0x1u);
  EXPECT_EQ(low_mask(4), 0xFu);
  EXPECT_EQ(low_mask(12), 0xFFFu);
  EXPECT_EQ(low_mask(31), 0x7FFFFFFFu);
}
TEST(LowMask, Len32DoesNotInvokeUB) {  // UBSan fires if you wrote 1u<<32
  EXPECT_EQ(low_mask(32), 0xFFFFFFFFu);
}
TEST(LowMask, HasExactlyLenOnes) {
  for (unsigned len = 0; len <= 32; ++len) {
    uint32_t m = low_mask(len);
    unsigned ones = 0;
    for (unsigned i = 0; i < 32; ++i) ones += (m >> i) & 1u;
    EXPECT_EQ(ones, len) << "len=" << len;
    if (len < 32) EXPECT_EQ(m >> len, 0u) << "bits above len must be 0, len=" << len;
  }
}

// ---------- set / clear / toggle / test (W 3.2) ----------
TEST(SingleBit, SetClearToggleTest) {
  EXPECT_EQ(set_bit(0x00u, 3), 0x08u);
  EXPECT_EQ(set_bit(0x08u, 3), 0x08u);  // idempotent: set twice is still set
  EXPECT_EQ(clear_bit(0xFFu, 0), 0xFEu);
  EXPECT_EQ(clear_bit(0xFEu, 0), 0xFEu);  // idempotent
  EXPECT_EQ(toggle_bit(0xA5u, 2), 0xA1u);
  EXPECT_EQ(toggle_bit(toggle_bit(0xA5u, 2), 2), 0xA5u);  // involution
  EXPECT_EQ(set_bit(0u, 31), 0x80000000u);
  EXPECT_TRUE(test_bit(0xA5u, 0));
  EXPECT_FALSE(test_bit(0xA5u, 1));
  EXPECT_TRUE(test_bit(0x80000000u, 31));
}
TEST(SingleBit, OnlyTargetBitChanges) {
  const uint32_t x = 0xC3A5F00Fu;
  for (unsigned p = 0; p < 32; ++p) {
    EXPECT_EQ(set_bit(x, p) ^ x, test_bit(x, p) ? 0u : (1u << p));
    EXPECT_EQ(clear_bit(x, p) ^ x, test_bit(x, p) ? (1u << p) : 0u);
    EXPECT_EQ(toggle_bit(x, p) ^ x, 1u << p);
  }
}

// ---------- extract_field (W 3.3) ----------
TEST(ExtractField, P25WordNacDuid) {
  const uint32_t w = 0x2935u;  // 0010 1001 0011 | 0101  -> NAC 0x293, DUID 5
  EXPECT_EQ(extract_field(w, 4, 12), 0x293u);
  EXPECT_EQ(extract_field(w, 0, 4), 0x5u);
}
TEST(ExtractField, FullWidthLen32) {
  EXPECT_EQ(extract_field(0xDEADBEEFu, 0, 32), 0xDEADBEEFu);
}
TEST(ExtractField, ZeroLenIsZero) { EXPECT_EQ(extract_field(0xFFFFFFFFu, 7, 0), 0u); }
TEST(ExtractField, TopFieldNoSignLeak) {
  EXPECT_EQ(extract_field(0x80000000u, 28, 4), 0x8u);
  EXPECT_EQ(extract_field(0xF0000000u, 31, 1), 1u);
}
TEST(ExtractField, MatchesBitByBitReference) {
  const uint32_t x = 0x9E3779B9u;
  for (unsigned pos = 0; pos < 32; ++pos) {
    for (unsigned len = 0; pos + len <= 32; ++len) {
      uint64_t expect = 0;
      for (unsigned i = 0; i < len; ++i) expect |= static_cast<uint64_t>((x >> (pos + i)) & 1u) << i;
      EXPECT_EQ(extract_field(x, pos, len), static_cast<uint32_t>(expect)) << pos << "," << len;
    }
  }
}

// ---------- insert_field (W 3.4) ----------
TEST(InsertField, Basic) {
  EXPECT_EQ(insert_field(0xFFFFu, 4, 4, 0x2u), 0xFF2Fu);
  EXPECT_EQ(insert_field(0x0000u, 4, 12, 0x293u), 0x2930u);
}
TEST(InsertField, OversizedValueDoesNotLeak) {
  // 0x1F is 5 bits; the field is 4 bits wide. Bit 4 of value must be dropped.
  EXPECT_EQ(insert_field(0x0000u, 0, 4, 0x1Fu), 0x000Fu);
  EXPECT_EQ(insert_field(0x0F0Fu, 4, 4, 0xABCu), 0x0FCFu);
}
TEST(InsertField, FullWidth) { EXPECT_EQ(insert_field(0x12345678u, 0, 32, 0xAAAAAAAAu), 0xAAAAAAAAu); }
TEST(InsertField, ExtractAfterInsertRoundTrips) {
  uint32_t x = 0x13572468u;
  for (unsigned pos = 0; pos < 32; ++pos) {
    for (unsigned len = 1; pos + len <= 32; ++len) {
      uint32_t v = 0xA5C3E17Bu & low_mask(len);
      uint32_t y = insert_field(x, pos, len, v);
      EXPECT_EQ(extract_field(y, pos, len), v);
      // everything outside the field is untouched
      uint32_t outside = ~(low_mask(len) << pos);
      EXPECT_EQ(y & outside, x & outside);
    }
  }
}

// ---------- sign_extend (W 3.6) ----------
TEST(SignExtend, FourBitEveryValue) {
  for (uint32_t v = 0; v < 16; ++v) {
    int32_t expect = (v >= 8) ? static_cast<int32_t>(v) - 16 : static_cast<int32_t>(v);
    EXPECT_EQ(sign_extend(v, 4), expect) << "v=" << v;
  }
}
TEST(SignExtend, ThreeBitEveryValue) {
  const int32_t expect[8] = {0, 1, 2, 3, -4, -3, -2, -1};
  for (uint32_t v = 0; v < 8; ++v) EXPECT_EQ(sign_extend(v, 3), expect[v]);
}
TEST(SignExtend, TwelveBitAdcSamples) {
  EXPECT_EQ(sign_extend(0xF85u, 12), -123);
  EXPECT_EQ(sign_extend(0x07Fu, 12), 127);
  EXPECT_EQ(sign_extend(0x800u, 12), -2048);
  EXPECT_EQ(sign_extend(0x7FFu, 12), 2047);
  EXPECT_EQ(sign_extend(0xFFFu, 12), -1);
}
TEST(SignExtend, IgnoresGarbageAboveWidth) { EXPECT_EQ(sign_extend(0xABCDEF05u, 4), 5); }
TEST(SignExtend, FullWidthAndOneBit) {
  EXPECT_EQ(sign_extend(0xFFFFFFFFu, 32), -1);
  EXPECT_EQ(sign_extend(0x80000000u, 32), INT32_MIN);
  EXPECT_EQ(sign_extend(1u, 1), -1);
  EXPECT_EQ(sign_extend(0u, 1), 0);
}

// ---------- lowest set bit / power of two (W 3.7) ----------
TEST(LowestSetBit, Examples) {
  EXPECT_EQ(lowest_set_bit(0x68u), 0x08u);  // 0110 1000 -> 0000 1000
  EXPECT_EQ(lowest_set_bit(0x01u), 0x01u);
  EXPECT_EQ(lowest_set_bit(0x80000000u), 0x80000000u);
  EXPECT_EQ(lowest_set_bit(0u), 0u);
  EXPECT_EQ(lowest_set_bit(0xFFFFFFFFu), 1u);
}
TEST(ClearLowestSetBit, Examples) {
  EXPECT_EQ(clear_lowest_set_bit(0x68u), 0x60u);
  EXPECT_EQ(clear_lowest_set_bit(0u), 0u);
  EXPECT_EQ(clear_lowest_set_bit(0x80000000u), 0u);
  EXPECT_EQ(clear_lowest_set_bit(0xFFFFFFFFu), 0xFFFFFFFEu);
}
TEST(PowerOfTwo, Examples) {
  EXPECT_FALSE(is_power_of_two(0u));  // the classic trap
  for (unsigned i = 0; i < 32; ++i) EXPECT_TRUE(is_power_of_two(1u << i)) << i;
  EXPECT_FALSE(is_power_of_two(3u));
  EXPECT_FALSE(is_power_of_two(0x80000001u));
  EXPECT_FALSE(is_power_of_two(0xFFFFFFFFu));
}

// ---------- popcount (W 3.8) ----------
static unsigned naive_popcount(uint32_t x) {
  unsigned n = 0;
  for (unsigned i = 0; i < 32; ++i) n += (x >> i) & 1u;
  return n;
}
TEST(PopcountKernighan, Examples) {
  EXPECT_EQ(popcount_kernighan(0u), 0u);
  EXPECT_EQ(popcount_kernighan(0xB4u), 4u);
  EXPECT_EQ(popcount_kernighan(0xFFFFFFFFu), 32u);
  EXPECT_EQ(popcount_kernighan(0x80000000u), 1u);
}
TEST(PopcountKernighan, MatchesNaive) {
  for (uint32_t x = 0; x < 5000; ++x) EXPECT_EQ(popcount_kernighan(x), naive_popcount(x));
  uint32_t s = 0x12345678u;
  for (int i = 0; i < 2000; ++i) {
    s = s * 1664525u + 1013904223u;  // LCG, unsigned wrap is defined
    EXPECT_EQ(popcount_kernighan(s), naive_popcount(s));
  }
}

// ---------- P25 NID (W 3.3, 4) ----------
TEST(P25Nid, ParseKnownWord) {
  P25Nid n = parse_p25_nid(0x2935u);
  EXPECT_EQ(n.nac, 0x293u);
  EXPECT_EQ(n.duid, 0x5u);
}
TEST(P25Nid, ParseOtherWords) {
  P25Nid a = parse_p25_nid(0xF7E0u);
  EXPECT_EQ(a.nac, 0xF7Eu);
  EXPECT_EQ(a.duid, 0x0u);
  P25Nid b = parse_p25_nid(0x001Fu);
  EXPECT_EQ(b.nac, 0x001u);
  EXPECT_EQ(b.duid, 0xFu);
}
TEST(P25Nid, BuildAndRoundTrip) {
  EXPECT_EQ(build_p25_nid(0x293u, 0x5u), 0x2935u);
  for (uint32_t w = 0; w < 0x10000u; w += 37) {
    P25Nid n = parse_p25_nid(static_cast<uint16_t>(w));
    EXPECT_EQ(build_p25_nid(n.nac, n.duid), w);
  }
}
TEST(P25Nid, BuildMasksOversizedInputs) {
  EXPECT_EQ(build_p25_nid(0x1293u, 0x15u), 0x2935u);  // extra high bits must not corrupt neighbours
}
