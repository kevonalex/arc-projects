#include <gtest/gtest.h>

#include <bitset>
#include <cstdint>

#include "lmr_legacy/legacy.hpp"

using namespace lmr_legacy;

TEST(Dpmr, RateAndEfficiency) {
  EXPECT_EQ(dpmr_gross_bit_rate(), 4800);
  EXPECT_EQ(dmr_per_call_bit_rate(), 4800);  // 9600 / 2 slots
  // Same bits per call, same bandwidth per call (12.5 kHz / 2 = 6.25 kHz): dPMR is the FDMA twin of one DMR slot.
  EXPECT_NEAR(bits_per_hz(dpmr_gross_bit_rate(), kDpmrChannelHz), 0.768, 1e-9);
  EXPECT_NEAR(bits_per_hz(9600, 12500), bits_per_hz(4800, 6250), 1e-12);
}

TEST(Mpt, FfskGeometry) {
  EXPECT_DOUBLE_EQ(ffsk_tone_spacing_hz(1200), 600.0);
  EXPECT_DOUBLE_EQ(cycles_per_bit(1200, 1200), 1.0);
  EXPECT_DOUBLE_EQ(cycles_per_bit(1800, 1200), 1.5);
  // the two tones differ by exactly half a cycle per bit: the minimum-shift (MSK) condition
  EXPECT_DOUBLE_EQ(cycles_per_bit(1800, 1200) - cycles_per_bit(1200, 1200), 0.5);
}

TEST(Mpt, CodewordTimingAndSyncWords) {
  EXPECT_NEAR(mpt_codeword_ms(), 53.3333, 1e-3);
  EXPECT_EQ(static_cast<std::uint16_t>(kMptSync ^ kMptSyncT), 0xFFFF);  // SYNCT is the complement of SYNC
}

TEST(Mpt, EncodeProducesValidCodewords) {
  for (std::uint64_t info : {0ULL, 1ULL, 0x123456789ABCULL, 0xFFFFFFFFFFFFULL, 0xA5A5A5A5A5A5ULL}) {
    const std::uint64_t cw = mpt_encode(info);
    EXPECT_TRUE(mpt_check(cw)) << std::hex << info;
    EXPECT_EQ(cw >> 16, info) << "info bits are the top 48 bits";
    EXPECT_EQ(std::bitset<64>(cw).count() % 2, 0u) << "even total weight";
  }
  EXPECT_EQ(mpt_encode(0), 0ULL);
}

TEST(Mpt, CrcOfZeroInfoIsZeroAndIsLinear) {
  EXPECT_EQ(mpt_crc15(0), 0u);
  const std::uint64_t a = 0x1234567890ABULL, b = 0x0FEDCBA98765ULL;
  EXPECT_EQ(mpt_crc15(a) ^ mpt_crc15(b), mpt_crc15(a ^ b));
}

TEST(Mpt, DetectsEverySingleBitError) {
  const std::uint64_t cw = mpt_encode(0x123456789ABCULL);
  for (int i = 0; i < 64; ++i) EXPECT_FALSE(mpt_check(cw ^ (1ULL << i))) << "bit " << i;
}

TEST(Mpt, DetectsEveryBurstUpToFifteenBitsInTheBody) {
  // CRC property: with deg g = 15 and g(0) = 1, an error burst of length <= 15 is never divisible by g.
  const std::uint64_t cw = mpt_encode(0xA5A5C3C3F0F0ULL);
  for (int len = 1; len <= 15; ++len) {
    for (int start = 1; start + len - 1 <= 63; ++start) {
      for (std::uint32_t inner = 0; inner < 8; ++inner) {
        // burst pattern: first and last bit set, inner bits taken from `inner`
        std::uint64_t pattern = 1ULL | (len > 1 ? (1ULL << (len - 1)) : 0ULL);
        for (int k = 1; k < len - 1 && k <= 3; ++k) {
          if ((inner >> (k - 1)) & 1u) pattern |= 1ULL << k;
        }
        EXPECT_FALSE(mpt_check(cw ^ (pattern << start))) << "len " << len << " start " << start << " inner " << inner;
      }
    }
  }
}
