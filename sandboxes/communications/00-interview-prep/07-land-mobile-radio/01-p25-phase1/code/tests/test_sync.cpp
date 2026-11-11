#include <gtest/gtest.h>

#include "p25p1/sync.hpp"

using namespace p25p1;

TEST(Sync, HammingIgnoresBitsAbove48) {
  EXPECT_EQ(hamming48(0, 0), 0);
  EXPECT_EQ(hamming48(0xFFFFULL << 48, 0), 0);  // old samples in a sliding window must not count
  EXPECT_EQ(hamming48(0x1, 0x0), 1);
  EXPECT_EQ(hamming48(kMask48, 0), 48);
}

TEST(Sync, ExactAndNoisyMatch) {
  EXPECT_TRUE(sync_matches(kFrameSync48, 0));
  EXPECT_TRUE(sync_matches(kFrameSync48 ^ 0x1ULL, 1));
  EXPECT_FALSE(sync_matches(kFrameSync48 ^ 0x1ULL, 0));
  EXPECT_TRUE(sync_matches(kFrameSync48 ^ 0x8000000000ULL ^ 0x2ULL, 2));
  EXPECT_FALSE(sync_matches(kFrameSync48 ^ 0x7ULL, 2));  // 3 flipped bits > 2 allowed
}

TEST(Sync, SlidingWindowFindsSyncInsideNoise) {
  // Build a bit string: 20 junk bits, sync, 10 junk bits; slide a 48-bit window.
  std::uint64_t window = 0;
  int found_at = -1;
  const std::uint64_t junk = 0b10110011100011110000;  // 20 bits
  std::uint64_t stream_bits[20 + 48 + 10];
  for (int i = 0; i < 20; ++i) stream_bits[i] = (junk >> (19 - i)) & 1ULL;
  for (int i = 0; i < 48; ++i) stream_bits[20 + i] = (kFrameSync48 >> (47 - i)) & 1ULL;
  for (int i = 0; i < 10; ++i) stream_bits[68 + i] = static_cast<std::uint64_t>(i & 1);
  for (int i = 0; i < 78; ++i) {
    window = ((window << 1) | stream_bits[i]) & kMask48;
    if (i >= 47 && sync_matches(window, 0)) {
      found_at = i;
      break;
    }
  }
  EXPECT_EQ(found_at, 20 + 47);
}

TEST(Sync, FalseSyncProbability) {
  const double two48 = 281474976710656.0;
  EXPECT_NEAR(false_sync_probability(0), 1.0 / two48, 1e-18);
  EXPECT_NEAR(false_sync_probability(1), 49.0 / two48, 1e-17);
  // 1 + 48 + 1128 + 17296 + 194580 = 213053 patterns within 4 errors
  EXPECT_NEAR(false_sync_probability(4), 213053.0 / two48, 1e-14);
  EXPECT_NEAR(false_sync_probability(4), 7.569e-10, 1e-12);
}

TEST(Sync, SecondsBetweenFalseSyncs) {
  const double hours = seconds_between_false_syncs(4) / 3600.0;
  EXPECT_GT(hours, 76.0);
  EXPECT_LT(hours, 77.0);
  // doubling the search rate halves the interval
  EXPECT_NEAR(seconds_between_false_syncs(4, 9600.0) * 2.0, seconds_between_false_syncs(4, 4800.0), 1.0);
  // allowing more errors makes false syncs more frequent
  EXPECT_LT(seconds_between_false_syncs(6), seconds_between_false_syncs(4));
}
