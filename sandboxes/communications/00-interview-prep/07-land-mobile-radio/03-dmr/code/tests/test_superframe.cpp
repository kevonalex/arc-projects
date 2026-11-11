#include <gtest/gtest.h>

#include "dmr/superframe.hpp"

using namespace dmr;

TEST(Superframe, SixBurstsOf60msIs360ms) {
  EXPECT_EQ(superframe_ms(), 360);
  EXPECT_EQ(voice_frames_per_superframe(), 18);
  EXPECT_EQ(voice_frames_per_superframe() * 20, superframe_ms());  // 18 x 20 ms AMBE+2 frames
}

TEST(Superframe, LettersCycleAtoF) {
  EXPECT_EQ(burst_letter(0), VoiceBurst::A);
  EXPECT_EQ(burst_letter(1), VoiceBurst::B);
  EXPECT_EQ(burst_letter(5), VoiceBurst::F);
  EXPECT_EQ(burst_letter(6), VoiceBurst::A);
  EXPECT_EQ(burst_letter(6000000001ULL), static_cast<VoiceBurst>(6000000001ULL % 6));
}

TEST(Superframe, OnlyAHasVoiceSync) {
  EXPECT_TRUE(has_voice_sync(VoiceBurst::A));
  for (VoiceBurst b : {VoiceBurst::B, VoiceBurst::C, VoiceBurst::D, VoiceBurst::E, VoiceBurst::F}) {
    EXPECT_FALSE(has_voice_sync(b));
  }
}

TEST(Superframe, EmbeddedLcInBthroughE) {
  EXPECT_FALSE(carries_embedded_lc(VoiceBurst::A));
  EXPECT_TRUE(carries_embedded_lc(VoiceBurst::B));
  EXPECT_TRUE(carries_embedded_lc(VoiceBurst::C));
  EXPECT_TRUE(carries_embedded_lc(VoiceBurst::D));
  EXPECT_TRUE(carries_embedded_lc(VoiceBurst::E));
  EXPECT_FALSE(carries_embedded_lc(VoiceBurst::F));
  EXPECT_EQ(embedded_lc_capacity_bits(), 128);
}

TEST(Superframe, LateEntryWorstCase) {
  EXPECT_EQ(late_entry_worst_case_ms(), 540);       // 6*60 + 3*60
  EXPECT_EQ(late_entry_worst_case_ms(3), 480);      // fewer fragments -> sooner
  EXPECT_EQ(late_entry_worst_case_ms(4, 30, 12), 12 * 30 + 3 * 30);  // generic formula
}
