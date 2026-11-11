#include <gtest/gtest.h>

#include "p25p1/budget.hpp"

using namespace p25p1;

TEST(Budget, RatesDerivedFromSymbols) {
  EXPECT_EQ(kBitRate, 9600);
}

TEST(Budget, ImbeFrame) {
  EXPECT_EQ(imbe_frame_bits(), 144);
  EXPECT_EQ(imbe_info_bits(), 88);
  EXPECT_EQ(imbe_frame_bits() * 50, 7200);  // 50 frames/s of 20 ms
  EXPECT_EQ(imbe_info_bits() * 50, 4400);   // speech rate
}

TEST(Budget, DurationOfLduIs180ms) {
  EXPECT_DOUBLE_EQ(duration_ms(kLduBits), 180.0);
  EXPECT_DOUBLE_EQ(duration_ms(48), 5.0);
}

TEST(Budget, StatusSymbols) {
  EXPECT_EQ(status_symbols_in(864), 24);
  EXPECT_EQ(status_symbols_in(35), 0);
  EXPECT_EQ(status_symbols_in(36), 1);
  EXPECT_EQ(status_symbols_in(72), 2);
}

TEST(Budget, LduBudgetAddsUpTo1728) {
  const LduBudget b = ldu_budget();
  EXPECT_EQ(b.sync_bits, 48);
  EXPECT_EQ(b.nid_bits, 64);
  EXPECT_EQ(b.imbe_bits, 1296);
  EXPECT_EQ(b.lc_or_es_bits, 240);
  EXPECT_EQ(b.lsd_bits, 32);
  EXPECT_EQ(b.status_bits, 48);
  EXPECT_EQ(b.total(), kLduBits);
}
