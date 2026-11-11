// Tests for Pack 06/01. The same file is compiled against the stubs and against the solutions.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "cell2g3g.hpp"

using namespace cell;

// ---------------------------------------------------------------- GSM numbers (W 3.1 - 3.3)
TEST(Gsm, SlotFrameAndBitRate) {
  EXPECT_NEAR(gsm_bit_rate_bps(), 270833.333, 0.01);
  EXPECT_NEAR(gsm_slot_ms(), 0.576923, 1e-5);
  EXPECT_NEAR(gsm_frame_ms(), 4.615385, 1e-5);
  EXPECT_NEAR(gsm_bits_per_slot(), 156.25, 1e-6);  // 148 burst bits + 8.25 guard
}
TEST(Gsm, ArfcnMapping) {
  EXPECT_NEAR(gsm_ul_mhz(1), 890.2, 1e-9);
  EXPECT_NEAR(gsm_dl_mhz(1), 935.2, 1e-9);
  EXPECT_NEAR(gsm_ul_mhz(62), 902.4, 1e-9);
  EXPECT_NEAR(gsm_dl_mhz(124), 959.8, 1e-9);
  EXPECT_DOUBLE_EQ(gsm_ul_mhz(0), -1.0);
  EXPECT_DOUBLE_EQ(gsm_ul_mhz(125), -1.0);
  EXPECT_DOUBLE_EQ(gsm_dl_mhz(200), -1.0);
}
TEST(Gsm, TimingAdvanceAndCoding) {
  EXPECT_NEAR(gsm_ta_step_m(), 553.46, 0.05);
  EXPECT_NEAR(63 * gsm_ta_step_m() / 1000.0, 34.87, 0.05);  // max range ~35 km
  EXPECT_NEAR(gsm_tch_coded_rate_bps(), 22800.0, 1e-9);
}
TEST(Gsm, FrequencyReuse) {
  EXPECT_EQ(cluster_size(1, 1), 3);
  EXPECT_EQ(cluster_size(2, 0), 4);
  EXPECT_EQ(cluster_size(2, 1), 7);
  EXPECT_EQ(cluster_size(2, 2), 12);
  EXPECT_NEAR(reuse_distance_ratio(7), 4.5826, 1e-3);
  EXPECT_NEAR(reuse_distance_ratio(3), 3.0, 1e-12);
}
TEST(Gsm, MskDeviation) { EXPECT_NEAR(msk_deviation_hz(gsm_bit_rate_bps()), 67708.3, 0.1); }

// ---------------------------------------------------------------- WCDMA numbers (W 3.4)
TEST(Wcdma, SpreadingFactorAndGain) {
  EXPECT_EQ(spreading_factor(3.84e6, 15e3), 256);
  EXPECT_EQ(spreading_factor(3.84e6, 960e3), 4);
  EXPECT_EQ(spreading_factor(3.84e6, 30e3), 128);
  EXPECT_NEAR(processing_gain_db(256), 24.082, 1e-3);
  EXPECT_NEAR(processing_gain_db(4), 6.0206, 1e-3);
}

// ---------------------------------------------------------------- OVSF tree (W 3.5)
TEST(Ovsf, SmallCodesExplicit) {
  EXPECT_EQ(ovsf_code(1, 0), (std::vector<int8_t>{1}));
  EXPECT_EQ(ovsf_code(2, 0), (std::vector<int8_t>{1, 1}));
  EXPECT_EQ(ovsf_code(2, 1), (std::vector<int8_t>{1, -1}));
  EXPECT_EQ(ovsf_code(4, 0), (std::vector<int8_t>{1, 1, 1, 1}));
  EXPECT_EQ(ovsf_code(4, 1), (std::vector<int8_t>{1, 1, -1, -1}));
  EXPECT_EQ(ovsf_code(4, 2), (std::vector<int8_t>{1, -1, 1, -1}));
  EXPECT_EQ(ovsf_code(4, 3), (std::vector<int8_t>{1, -1, -1, 1}));
}
TEST(Ovsf, SameSfCodesAreMutuallyOrthogonal) {
  for (int sf : {4, 8, 16}) {
    for (int a = 0; a < sf; ++a) {
      auto ca = ovsf_code(sf, a);
      ASSERT_EQ(static_cast<int>(ca.size()), sf);
      for (int b = 0; b < sf; ++b) {
        auto cb = ovsf_code(sf, b);
        EXPECT_EQ(cross_correlation(ca, cb, 0), a == b ? sf : 0) << "sf=" << sf << " a=" << a << " b=" << b;
      }
    }
  }
}
TEST(Ovsf, ConflictMatchesChipPrefixRule) {
  // Child codes start with their parent's chips: conflict <=> the longer code begins with the shorter one.
  for (int sfa : {1, 2, 4, 8}) {
    for (int sfb : {1, 2, 4, 8}) {
      for (int ia = 0; ia < sfa; ++ia) {
        for (int ib = 0; ib < sfb; ++ib) {
          auto a = ovsf_code(sfa, ia), b = ovsf_code(sfb, ib);
          const auto& s = a.size() <= b.size() ? a : b;
          const auto& l = a.size() <= b.size() ? b : a;
          bool prefix = true;
          for (size_t i = 0; i < s.size(); ++i) prefix = prefix && (s[i] == l[i]);
          EXPECT_EQ(ovsf_conflict(sfa, ia, sfb, ib), prefix) << sfa << "," << ia << " vs " << sfb << "," << ib;
        }
      }
    }
  }
}
TEST(Ovsf, ConflictExamples) {
  EXPECT_TRUE(ovsf_conflict(4, 1, 8, 2));   // C8,2 is a child of C4,1
  EXPECT_TRUE(ovsf_conflict(4, 1, 8, 3));   // so is C8,3
  EXPECT_FALSE(ovsf_conflict(4, 1, 8, 4));  // C8,4 belongs to C4,2
  EXPECT_TRUE(ovsf_conflict(1, 0, 256, 77));  // the root conflicts with everything
}

// ---------------------------------------------------------------- spreading separates users (W 3.5)
TEST(Spreading, SpreadThenDespreadRoundTrips) {
  auto code = ovsf_code(8, 5);
  std::vector<int> sym = {1, -1, -1, 1, 1};
  auto chips = spread(sym, code);
  ASSERT_EQ(chips.size(), 40u);
  auto back = despread(chips, code);
  ASSERT_EQ(back.size(), sym.size());
  for (size_t i = 0; i < sym.size(); ++i) EXPECT_DOUBLE_EQ(back[i], sym[i]);
}
TEST(Spreading, TwoUsersShareOneChipStreamAndSeparate) {
  auto c1 = ovsf_code(8, 1), c2 = ovsf_code(8, 6);
  std::vector<int> u1 = {1, 1, -1, -1, 1, -1}, u2 = {-1, 1, 1, -1, -1, -1};
  auto x1 = spread(u1, c1), x2 = spread(u2, c2);
  ASSERT_EQ(x1.size(), x2.size());
  std::vector<int> air(x1.size());
  for (size_t i = 0; i < air.size(); ++i) air[i] = x1[i] + x2[i];  // both transmit at once, same band
  auto r1 = despread(air, c1), r2 = despread(air, c2);
  ASSERT_EQ(r1.size(), u1.size());
  for (size_t i = 0; i < u1.size(); ++i) {
    EXPECT_DOUBLE_EQ(r1[i], u1[i]);
    EXPECT_DOUBLE_EQ(r2[i], u2[i]);
  }
}
TEST(Spreading, ParentCodeInterferesWithChildCode) {
  // C4,1 = (1,1,-1,-1) and its child C8,2 = (1,1,-1,-1,1,1,-1,-1): same-SF orthogonality does not rescue them.
  auto parent = ovsf_code(4, 1), child = ovsf_code(8, 2);
  std::vector<int> a = {1, 1}, b = {1};  // parent sends two symbols, child sends one in the same 8 chips
  auto xa = spread(a, parent), xb = spread(b, child);
  ASSERT_EQ(xa.size(), 8u);
  ASSERT_EQ(xb.size(), 8u);
  std::vector<int> air(8);
  for (size_t i = 0; i < 8; ++i) air[i] = xa[i] + xb[i];
  auto ra = despread(air, parent);  // two 4-chip blocks
  ASSERT_EQ(ra.size(), 2u);
  EXPECT_NE(ra[0], 1.0);  // child's first half leaks into the parent's first symbol
}
TEST(Spreading, ChipMisalignmentDestroysOrthogonality) {
  auto a = ovsf_code(8, 1), b = ovsf_code(8, 3);
  EXPECT_EQ(cross_correlation(a, b, 0), 0);   // perfectly aligned: orthogonal
  EXPECT_EQ(cross_correlation(a, b, 1), -4);  // one chip late: half the full-correlation value (8)
  EXPECT_EQ(cross_correlation(a, b, 2), -8);  // two chips late: as strong as the wanted signal itself!
  EXPECT_EQ(cross_correlation(a, a, 0), 8);
  EXPECT_EQ(cross_correlation(a, ovsf_code(4, 0), 0), 0);  // length mismatch -> 0 by contract
}
