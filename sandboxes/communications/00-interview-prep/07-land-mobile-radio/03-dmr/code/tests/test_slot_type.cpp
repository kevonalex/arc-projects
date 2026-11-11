#include <gtest/gtest.h>

#include <set>
#include <stdexcept>

#include "dmr/slot_type.hpp"

using namespace dmr;

// ---- support/dmr/golay23.hpp sanity (given code) ------------------------------------------------
TEST(GolaySupport, CodeIsPerfect) {
  // 1 + 23 + 253 + 1771 = 2048 syndromes, each hit exactly once.
  std::set<std::uint32_t> syndromes;
  for (int a = 0; a < 23; ++a) {
    syndromes.insert(golay_syndrome(1u << a));
    for (int b = a + 1; b < 23; ++b) {
      syndromes.insert(golay_syndrome((1u << a) | (1u << b)));
      for (int c = b + 1; c < 23; ++c) syndromes.insert(golay_syndrome((1u << a) | (1u << b) | (1u << c)));
    }
  }
  EXPECT_EQ(syndromes.size(), 2047u);  // all non-zero syndromes, no collisions => d_min >= 7
  EXPECT_EQ(syndromes.count(0), 0u);
}

TEST(GolaySupport, EncodeDecodeCorrectsThree) {
  const std::uint32_t cw = golay23_encode(0xABC);
  EXPECT_EQ(golay_syndrome(cw), 0u);
  const std::uint32_t rx = cw ^ (1u << 2) ^ (1u << 11) ^ (1u << 22);
  const Golay23Result r = golay23_decode(rx);
  EXPECT_EQ(r.data, 0xABC);
  EXPECT_EQ(r.errors, 3);
}

// ---- slot type -------------------------------------------------------------------------------
TEST(SlotType, ByteRoundTrip) {
  EXPECT_EQ(pack_slot_type_byte(1, 1), 0x11);
  EXPECT_EQ(pack_slot_type_byte(15, 10), 0xFA);
  const SlotType s = unpack_slot_type_byte(0x93);
  EXPECT_EQ(s.colour_code, 9);
  EXPECT_EQ(s.data_type, 3);
}

TEST(SlotType, RejectsWideFields) {
  EXPECT_THROW(pack_slot_type_byte(16, 0), std::out_of_range);
  EXPECT_THROW(pack_slot_type_byte(0, 16), std::out_of_range);
}

TEST(SlotType, EncodeUsesTwentyBitsAndPreservesInfoBits) {
  for (int cc = 0; cc < 16; ++cc) {
    for (int dt = 0; dt < 16; ++dt) {
      const std::uint32_t w = slot_type_encode(static_cast<std::uint8_t>(cc), static_cast<std::uint8_t>(dt));
      EXPECT_LT(w, 1u << 20);
      // systematic: info byte sits in bits 19..12 (above 11 Golay parity bits + 1 overall parity)
      EXPECT_EQ((w >> 12) & 0xFFu, static_cast<std::uint32_t>((cc << 4) | dt));
      EXPECT_EQ(popcount32(w) % 2, 0) << "even overall weight";
    }
  }
}

TEST(SlotType, HandWorkedVector) {
  // Worksheet 03 section 4: CC=1, data type=1 (voice LC header).
  // info12 = 0x011 -> x^11 * info mod g = 0x5C6 -> codeword23 = 0x8DC6 -> word19 = 0x8DC6,
  // overall parity of 0x8DC6 (8 ones) = 0 -> word20 = 0x8DC6 << 1 = 0x11B8C.
  EXPECT_EQ(slot_type_encode(1, 1), 0x11B8Cu);
}

TEST(SlotType, NoiseAcceptanceDensity) {
  // d_min = 8 => the radius-3 decoding spheres around the 256 codewords are disjoint.
  // Each sphere holds 1 + 20 + 190 + 1140 = 1351 words, so a decoder that accepts <= 3 errors
  // accepts exactly 256 * 1351 = 345 856 of the 2^20 possible words: ~33 % of PURE NOISE.
  // (Which is why the colour-code check and later CRCs matter.)
  std::uint32_t accepted = 0;
  for (std::uint32_t w = 0; w < (1u << 20); ++w) {
    if (slot_type_decode(w).ok) ++accepted;
  }
  EXPECT_EQ(accepted, 256u * 1351u);
}

TEST(SlotType, CleanRoundTripAllValues) {
  for (int cc = 0; cc < 16; ++cc) {
    for (int dt = 0; dt < 16; ++dt) {
      const auto r = slot_type_decode(slot_type_encode(static_cast<std::uint8_t>(cc), static_cast<std::uint8_t>(dt)));
      ASSERT_TRUE(r.ok);
      EXPECT_EQ(r.errors, 0);
      EXPECT_EQ(r.slot_type.colour_code, cc);
      EXPECT_EQ(r.slot_type.data_type, dt);
    }
  }
}

TEST(SlotType, CorrectsUpToThreeErrors) {
  for (int info = 0; info < 256; info += 5) {
    const std::uint8_t cc = static_cast<std::uint8_t>(info >> 4), dt = static_cast<std::uint8_t>(info & 15);
    const std::uint32_t w = slot_type_encode(cc, dt);
    for (int a = 0; a < 20; ++a) {
      for (int b = a; b < 20; ++b) {
        for (int c = b; c < 20; ++c) {
          // a==b or b==c cancel pairs -> patterns of weight 1..3 (and 0 when a==b==c? no: weight 1)
          std::uint32_t rx = w ^ (1u << a);
          int flips = 1;
          if (b != a) { rx ^= (1u << b); ++flips; }
          if (c != b) { rx ^= (1u << c); ++flips; }
          const auto r = slot_type_decode(rx);
          ASSERT_TRUE(r.ok) << "info " << info << " flips " << flips;
          EXPECT_EQ(r.slot_type.colour_code, cc);
          EXPECT_EQ(r.slot_type.data_type, dt);
          EXPECT_EQ(r.errors, flips);
        }
      }
    }
  }
}

TEST(SlotType, DetectsFourErrors) {
  // d_min = 8: every pattern of 4 errors must be flagged, never silently mis-corrected.
  const std::uint32_t w = slot_type_encode(5, 3);
  for (int a = 0; a < 20; ++a)
    for (int b = a + 1; b < 20; ++b)
      for (int c = b + 1; c < 20; ++c)
        for (int d = c + 1; d < 20; ++d) {
          const std::uint32_t rx = w ^ (1u << a) ^ (1u << b) ^ (1u << c) ^ (1u << d);
          EXPECT_FALSE(slot_type_decode(rx).ok) << a << "," << b << "," << c << "," << d;
        }
}
