// Worksheet 06, Build A (Q1-Q4) - Golay(23,12) and Golay(24,12). Synthetic words only.
#include <gtest/gtest.h>

#include <algorithm>
#include <bitset>
#include <map>
#include <set>

#include "lmrb/golay.hpp"

using namespace lmrb;

namespace {
int wt(std::uint32_t v) { return static_cast<int>(std::bitset<32>(v).count()); }
}  // namespace

TEST(GolayBuild, GeneratorDividesItself) {
  EXPECT_EQ(golay_remainder(kGolayGen), 0u);
  for (int s = 0; s <= 11; ++s) EXPECT_EQ(golay_remainder(kGolayGen << s), 0u) << "shift " << s;
}

TEST(GolayBuild, RemainderIgnoresBitsAbove23) {
  EXPECT_EQ(golay_remainder(0x800000u | 0x123u), golay_remainder(0x123u));
}

TEST(GolayBuild, KnownEncodings) {
  EXPECT_EQ(golay23_encode(0), 0u);
  EXPECT_EQ(golay23_encode(1), 0xC75u);  // x^11 mod g = g - x^11, so (x^11 | rem) = g itself
  EXPECT_EQ(golay23_encode(0x1000), golay23_encode(0));  // only 12 info bits are used
}

TEST(GolayBuild, EncodeIsSystematicAndLinear) {
  for (std::uint32_t d : {0x123u, 0xFFFu, 0x800u, 0x0A5u}) {
    EXPECT_EQ(golay23_encode(static_cast<std::uint16_t>(d)) >> 11, d);
    for (std::uint32_t e : {0x001u, 0x7FFu, 0xABCu}) {
      EXPECT_EQ(golay23_encode(static_cast<std::uint16_t>(d ^ e)),
                golay23_encode(static_cast<std::uint16_t>(d)) ^ golay23_encode(static_cast<std::uint16_t>(e)));
    }
  }
}

TEST(GolayBuild, AllCodewordsAreDivisibleAndMinimumWeightIsSeven) {
  int min_weight = 99;
  for (std::uint32_t d = 0; d < 4096; ++d) {
    const std::uint32_t cw = golay23_encode(static_cast<std::uint16_t>(d));
    EXPECT_EQ(golay_remainder(cw), 0u) << d;
    if (d != 0) min_weight = std::min(min_weight, wt(cw));
  }
  EXPECT_EQ(min_weight, 7);
}

TEST(GolayBuild, WeightDistributionIsTheKnownOne) {
  std::map<int, int> count;
  for (std::uint32_t d = 0; d < 4096; ++d) ++count[wt(golay23_encode(static_cast<std::uint16_t>(d)))];
  const std::map<int, int> expected = {{0, 1},  {7, 253},  {8, 506},  {11, 1288},
                                       {12, 1288}, {15, 506}, {16, 253}, {23, 1}};
  EXPECT_EQ(count, expected);
}

TEST(GolayBuild, SyndromeTableIsPerfect) {
  const auto& t = golay_pattern_table();
  ASSERT_EQ(t.size(), 2048u);
  EXPECT_EQ(t[0], 0u);
  std::set<std::uint32_t> distinct;
  int by_weight[4] = {0, 0, 0, 0};
  for (std::uint32_t s = 0; s < 2048; ++s) {
    const int w = wt(t[s]);
    ASSERT_LE(w, 3) << s;
    EXPECT_EQ(golay_remainder(t[s]), s) << s;
    ++by_weight[w];
    distinct.insert(t[s]);
  }
  EXPECT_EQ(distinct.size(), 2048u);
  EXPECT_EQ(by_weight[0], 1);
  EXPECT_EQ(by_weight[1], 23);
  EXPECT_EQ(by_weight[2], 253);
  EXPECT_EQ(by_weight[3], 1771);
}

TEST(GolayBuild, CorrectsEveryPatternUpToThree) {
  const std::uint16_t info = 0xABC;
  const std::uint32_t cw = golay23_encode(info);
  int tested = 0;
  for (std::uint32_t p = 0; p < (1u << 23); ++p) {
    if (wt(p) > 3) continue;
    const Golay23Result r = golay23_decode(cw ^ p);
    ASSERT_EQ(r.codeword, cw) << std::hex << p;
    ASSERT_EQ(r.data, info);
    ASSERT_EQ(r.errors, wt(p));
    ++tested;
  }
  EXPECT_EQ(tested, 2048);
}

TEST(GolayBuild, PerfectCodeMiscorrectsEveryFourErrorPattern) {
  const std::uint32_t cw = golay23_encode(0x5A5);
  int miscorrected = 0;
  int total = 0;
  for (std::uint32_t p = 0; p < (1u << 23); ++p) {
    if (wt(p) != 4) continue;
    ++total;
    const Golay23Result r = golay23_decode(cw ^ p);
    if (r.codeword != cw && r.errors <= 3) ++miscorrected;
  }
  EXPECT_EQ(total, 8855);
  EXPECT_EQ(miscorrected, 8855);  // a perfect code has no 'I give up' answer
}

TEST(Golay24, EncodeHasEvenWeightAndMinimumWeightEight) {
  int min_weight = 99;
  for (std::uint32_t d = 0; d < 4096; ++d) {
    const std::uint32_t w24 = golay24_encode(static_cast<std::uint16_t>(d));
    EXPECT_EQ(wt(w24) % 2, 0) << d;
    EXPECT_EQ(w24 >> 1, golay23_encode(static_cast<std::uint16_t>(d)));
    if (d != 0) min_weight = std::min(min_weight, wt(w24));
  }
  EXPECT_EQ(min_weight, 8);
}

TEST(Golay24, CorrectsThreeAndDetectsEveryFour) {
  const std::uint16_t info = 0x3C9;
  const std::uint32_t cw = golay24_encode(info);
  int corrected = 0;
  int detected4 = 0;
  for (std::uint32_t p = 0; p < (1u << 24); ++p) {
    const int w = wt(p);
    if (w > 4) continue;
    const Golay24Result r = golay24_decode(cw ^ p);
    if (w <= 3) {
      ASSERT_EQ(r.codeword, cw) << std::hex << p;
      ASSERT_EQ(r.data, info);
      ASSERT_EQ(r.errors, w);
      ASSERT_EQ(r.status, w == 0 ? Golay24Status::Ok : Golay24Status::Corrected);
      ++corrected;
    } else {
      ASSERT_EQ(r.status, Golay24Status::Uncorrectable) << std::hex << p;
      ++detected4;
    }
  }
  EXPECT_EQ(corrected, 1 + 24 + 276 + 2024);  // 2325
  EXPECT_EQ(detected4, 10626);                 // C(24,4)
}
