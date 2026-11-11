#include <gtest/gtest.h>

#include "tetra/timing.hpp"

using namespace tetra;

TEST(TetraTiming, RatesAndDurations) {
  EXPECT_EQ(gross_bit_rate(), 36000);
  EXPECT_NEAR(slot_ms(), 14.16667, 1e-4);
  EXPECT_NEAR(frame_ms(), 56.66667, 1e-4);
  EXPECT_NEAR(multiframe_ms(), 1020.0, 1e-6);
  EXPECT_NEAR(hyperframe_s(), 61.2, 1e-9);
}

TEST(TetraTiming, NetRateDerivation) {
  EXPECT_EQ(traffic_frames_per_multiframe(), 17);
  EXPECT_EQ(overhead_bits_in_normal_burst(), 78);
  EXPECT_NEAR(tch72_net_rate(), 7200.0, 1e-6);      // 432 * 17 / 1.02
  EXPECT_NEAR(net_rate_per_carrier(), 28800.0, 1e-6);
  EXPECT_NEAR(net_over_gross(), 0.8, 1e-9);
}

TEST(TetraTiming, SpeechFitsTheMultiframeExactly) {
  EXPECT_NEAR(speech_rate_bps(), 4566.6667, 1e-3);
  EXPECT_EQ(speech_frames_per_slot(), 2);
  EXPECT_EQ(speech_ms_per_slot(), 60);
  // 17 traffic slots x 60 ms of speech = 1020 ms = exactly one multiframe: no buffer ever grows.
  EXPECT_NEAR(traffic_frames_per_multiframe() * speech_ms_per_slot(), multiframe_ms(), 1e-6);
}

TEST(TetraTiming, TimeOfSlot) {
  TetraTime t = time_of_slot(0);
  EXPECT_EQ(t.timeslot, 1);
  EXPECT_EQ(t.frame, 1);
  EXPECT_EQ(t.multiframe, 1);

  t = time_of_slot(3);
  EXPECT_EQ(t.timeslot, 4);
  EXPECT_EQ(t.frame, 1);

  t = time_of_slot(4);
  EXPECT_EQ(t.timeslot, 1);
  EXPECT_EQ(t.frame, 2);

  t = time_of_slot(68);  // 17 frames * 4 slots -> first slot of frame 18
  EXPECT_EQ(t.frame, 18);
  EXPECT_EQ(t.timeslot, 1);
  EXPECT_TRUE(is_control_frame(t.frame));

  t = time_of_slot(72);  // 18 * 4 slots = one multiframe
  EXPECT_EQ(t.multiframe, 2);
  EXPECT_EQ(t.frame, 1);

  t = time_of_slot(72ULL * 60);  // one hyperframe later: wraps to multiframe 1
  EXPECT_EQ(t.multiframe, 1);
  EXPECT_EQ(t.frame, 1);
  EXPECT_EQ(t.timeslot, 1);

  t = time_of_slot(72ULL * 60 - 1);  // very last slot of the hyperframe
  EXPECT_EQ(t.multiframe, 60);
  EXPECT_EQ(t.frame, 18);
  EXPECT_EQ(t.timeslot, 4);
}

TEST(TetraTiming, OnlyFrame18IsControl) {
  for (int f = 1; f <= 17; ++f) EXPECT_FALSE(is_control_frame(f));
  EXPECT_TRUE(is_control_frame(18));
}
