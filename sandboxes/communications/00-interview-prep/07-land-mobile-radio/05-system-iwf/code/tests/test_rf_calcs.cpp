// Worksheet 05, Q1, Q3-Q5, Q12 - arithmetic with hand-checkable answers.
#include <gtest/gtest.h>

#include "lmrsys/rf_calcs.hpp"

using namespace lmrsys;

TEST(Erlang, HandComputedValues) {
  EXPECT_DOUBLE_EQ(erlang_b(2.0, 0), 1.0);
  EXPECT_NEAR(erlang_b(0.5, 1), 1.0 / 3.0, 1e-12);
  EXPECT_NEAR(erlang_b(1.0, 2), 0.2, 1e-12);
  EXPECT_NEAR(erlang_b(2.0, 2), 0.4, 1e-12);
  EXPECT_NEAR(erlang_b(2.0, 3), 0.8 / 3.8, 1e-12);  // 0.2105
  EXPECT_NEAR(erlang_b(2.0, 4), 0.095238, 1e-6);
}

TEST(Erlang, PoolingBeatsSplitting) {
  // 4 channels shared by 2 Erlangs vs four 1-channel groups of 0.5 Erlang each
  EXPECT_LT(erlang_b(2.0, 4), erlang_b(0.5, 1));
  // even after giving one of the 4 channels to a control channel
  EXPECT_LT(erlang_b(2.0, 3), erlang_b(0.5, 1));
}

TEST(Simulcast, DelayPerKilometre) {
  EXPECT_NEAR(path_delay_us(1.0), 3.3356, 1e-3);
  EXPECT_NEAR(path_delay_us(30.0), 100.07, 1e-2);
  EXPECT_DOUBLE_EQ(path_delay_us(0.0), 0.0);
}

TEST(Simulcast, SymbolPeriods) {
  EXPECT_NEAR(symbol_period_us(4800.0), 208.333, 1e-3);   // P25 Phase 1
  EXPECT_NEAR(symbol_period_us(6000.0), 166.667, 1e-3);   // P25 Phase 2
  EXPECT_NEAR(symbol_period_us(18000.0), 55.556, 1e-3);   // TETRA
}

TEST(Simulcast, MaxPathDifferenceForQuarterSymbol) {
  // 0.25 * 208.333 us = 52.083 us  ->  52.083 / 3.3356 = 15.61 km
  EXPECT_NEAR(max_delay_spread_km(4800.0, 0.25), 15.614, 0.01);
  // a faster symbol rate leaves less room
  EXPECT_LT(max_delay_spread_km(18000.0, 0.25), max_delay_spread_km(4800.0, 0.25));
}

TEST(Voting, PicksFewestCorrectedErrorsAmongValid) {
  std::vector<VoteSample> s = {{1, 3, true}, {2, 0, true}, {3, 0, false}, {4, 0, true}};
  EXPECT_EQ(vote(s), 2);  // 2 and 4 tie on 0 errors -> lowest id; 3 is invalid despite 0 errors
}

TEST(Voting, PrefersLowErrorsOverLowId) {
  std::vector<VoteSample> s = {{1, 5, true}, {2, 1, true}};
  EXPECT_EQ(vote(s), 2);
}

TEST(Voting, NoValidSampleGivesMinusOne) {
  EXPECT_EQ(vote({}), -1);
  std::vector<VoteSample> s = {{1, 0, false}, {2, 0, false}};
  EXPECT_EQ(vote(s), -1);
}

TEST(Voting, IndependentFailureProbability) {
  EXPECT_NEAR(vote_failure_probability(0.1, 1), 0.1, 1e-12);
  EXPECT_NEAR(vote_failure_probability(0.1, 3), 1e-3, 1e-12);
  EXPECT_NEAR(vote_failure_probability(0.5, 4), 0.0625, 1e-12);
  EXPECT_DOUBLE_EQ(vote_failure_probability(0.3, 0), 1.0);
}

TEST(LateEntry, MeanAcquireTime) {
  EXPECT_NEAR(acquire_time_ms(360.0, 0.0), 180.0, 1e-9);
  EXPECT_NEAR(acquire_time_ms(360.0, 0.5), 540.0, 1e-9);   // 180 + 1 * 360
  EXPECT_NEAR(acquire_time_ms(360.0, 0.9), 3420.0, 1e-6);  // 180 + 9 * 360
}

TEST(Transcode, GcdLcm) {
  EXPECT_EQ(gcd_int(20, 30), 10);
  EXPECT_EQ(lcm_int(20, 30), 60);
  EXPECT_EQ(lcm_int(7, 13), 91);
  EXPECT_EQ(gcd_int(17, 0), 17);
}

TEST(Transcode, FramesRealignEverySixtyMs) {
  EXPECT_EQ(transcode_alignment_ms(20, 30), 60);   // 3 x 20 ms = 2 x 30 ms
  EXPECT_EQ(transcode_alignment_ms(20, 20), 20);
}

TEST(Transcode, FramingDelay) {
  EXPECT_EQ(max_framing_delay_ms(30, 20), 20);
  EXPECT_EQ(max_framing_delay_ms(20, 30), 10);
  EXPECT_EQ(max_framing_delay_ms(20, 20), 0);
}

TEST(Transcode, SamplesPerFrame) {
  EXPECT_EQ(samples_per_frame(20, 8000), 160);
  EXPECT_EQ(samples_per_frame(30, 8000), 240);
}
