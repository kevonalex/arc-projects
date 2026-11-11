#include <gtest/gtest.h>

#include <set>

#include "dmr/bptc.hpp"

using namespace dmr;

TEST(Bptc, Dimensions) {
  const BptcDims d = bptc_dims();
  EXPECT_EQ(d.info_rows, 9);
  EXPECT_EQ(d.info_cols, 11);
  EXPECT_EQ(d.rows, 13);
  EXPECT_EQ(d.cols, 15);
  EXPECT_EQ(d.matrix_bits, 195);
  EXPECT_EQ(d.total_bits, 196);
  EXPECT_EQ(d.info_positions, 99);   // 96 data + 3 reserved
  EXPECT_EQ(d.total_bits / 2, 98);   // each half of the burst carries 98 of the 196 bits
}

TEST(Bptc, GcdAndPermutationCondition) {
  EXPECT_EQ(gcd_int(181, 196), 1);
  EXPECT_EQ(gcd_int(12, 18), 6);
  EXPECT_EQ(gcd_int(7, 0), 7);
}

TEST(Bptc, InterleaveIsAPermutation) {
  std::set<int> seen;
  for (int a = 0; a < 196; ++a) {
    const int p = bptc_interleave_index(a);
    ASSERT_GE(p, 0);
    ASSERT_LT(p, 196);
    seen.insert(p);
  }
  EXPECT_EQ(seen.size(), 196u);
  EXPECT_EQ(bptc_interleave_index(0), 0);
  EXPECT_EQ(bptc_interleave_index(1), 181);
  EXPECT_EQ(bptc_interleave_index(2), 166);
  EXPECT_EQ(bptc_interleave_index(3), 151);
}

TEST(Bptc, DeinterleaveIsTheInverse) {
  for (int a = 0; a < 196; ++a) {
    EXPECT_EQ(bptc_deinterleave_index(bptc_interleave_index(a)), a);
    EXPECT_EQ(bptc_interleave_index(bptc_deinterleave_index(a)), a);
  }
}

TEST(Bptc, CircularDistance) {
  EXPECT_EQ(circular_distance(0, 195, 196), 1);
  EXPECT_EQ(circular_distance(10, 20, 196), 10);
  EXPECT_EQ(circular_distance(0, 98, 196), 98);
}

TEST(Bptc, BurstErrorsAreSpreadApart) {
  // Any two bits within 15 consecutive transmitted positions come from matrix positions >= 13 apart,
  // so a short fade hits many different matrix cells rather than adjacent ones.
  for (int p = 0; p < 196; ++p) {
    for (int d = 1; d <= 14; ++d) {
      const int a = bptc_deinterleave_index(p);
      const int b = bptc_deinterleave_index((p + d) % 196);
      EXPECT_GE(circular_distance(a, b, 196), 13) << "p=" << p << " d=" << d;
    }
  }
}
