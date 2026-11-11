#include <gtest/gtest.h>

#include "p25p2/timing.hpp"

using namespace p25p2;

TEST(Timing, RatesAndSlotSizes) {
  EXPECT_EQ(gross_bit_rate(), 12000);
  EXPECT_EQ(symbols_per_slot(), 180);
  EXPECT_EQ(bits_per_slot(), 360);
  EXPECT_EQ(superframe_ms(), 360);
  EXPECT_EQ(slots_per_channel_per_superframe(), 6);
}

TEST(Timing, VoiceFramesPerSuperframe) {
  // 360 ms of speech / 20 ms per AMBE+2 frame = 18, whatever the slot pattern.
  EXPECT_EQ(voice_frames_per_channel_per_superframe(), 18);
  EXPECT_EQ(voice_frames_per_channel_per_superframe() * kAmbeFrameMs, superframe_ms());
}

TEST(Timing, FourVBurstOverhead) {
  EXPECT_EQ(overhead_bits_in_4v_slot(), 360 - 4 * 72);
  EXPECT_EQ(overhead_bits_in_4v_slot(), 72);
}

TEST(Timing, SpectralEfficiency) {
  EXPECT_NEAR(spectral_efficiency(9600, 12500), 0.768, 1e-9);   // Phase 1
  EXPECT_NEAR(spectral_efficiency(12000, 12500), 0.96, 1e-9);   // Phase 2
  EXPECT_NEAR(spectral_efficiency(12000, 12500) / spectral_efficiency(9600, 12500), 1.25, 1e-9);
}

TEST(Timing, DutyCycle) {
  EXPECT_DOUBLE_EQ(tx_duty_cycle(1), 1.0);
  EXPECT_DOUBLE_EQ(tx_duty_cycle(2), 0.5);
}

TEST(Timing, CallsThatFit) {
  EXPECT_EQ(calls_that_fit(9600, 7200, 2400), 1);    // Phase 1: IMBE
  EXPECT_EQ(calls_that_fit(12000, 3600, 4800), 2);   // Phase 2: AMBE+2 x2
  EXPECT_EQ(calls_that_fit(12000, 7200, 4800), 1);   // IMBE in a TDMA carrier does NOT fit twice
  EXPECT_EQ(calls_that_fit(12000, 3600, 4801), 1);   // one bit/s too much overhead
  EXPECT_EQ(calls_that_fit(1000, 3600, 4800), 0);    // never negative
}

TEST(Timing, PositionOfSymbol) {
  SlotPosition p = position_of_symbol(0);
  EXPECT_EQ(p.superframe, 0u);
  EXPECT_EQ(p.slot_in_superframe, 0);
  EXPECT_EQ(p.channel, 0);
  EXPECT_EQ(p.symbol_in_slot, 0);

  p = position_of_symbol(179);
  EXPECT_EQ(p.slot_in_superframe, 0);
  EXPECT_EQ(p.symbol_in_slot, 179);

  p = position_of_symbol(180);
  EXPECT_EQ(p.slot_in_superframe, 1);
  EXPECT_EQ(p.channel, 1);
  EXPECT_EQ(p.symbol_in_slot, 0);

  p = position_of_symbol(360);
  EXPECT_EQ(p.slot_in_superframe, 2);
  EXPECT_EQ(p.channel, 0);

  p = position_of_symbol(2160);  // 12 slots * 180 symbols = one superframe = 360 ms
  EXPECT_EQ(p.superframe, 1u);
  EXPECT_EQ(p.slot_in_superframe, 0);

  p = position_of_symbol(10000);  // slot 55 -> superframe 4, slot 7, channel 1, symbol 100
  EXPECT_EQ(p.superframe, 4u);
  EXPECT_EQ(p.slot_in_superframe, 7);
  EXPECT_EQ(p.channel, 1);
  EXPECT_EQ(p.symbol_in_slot, 100);
}

TEST(Timing, SuperframeSymbolsEqual360msAt6000) {
  EXPECT_EQ(static_cast<int>(kSlotsPerSuperframe) * symbols_per_slot(), 6000 * 360 / 1000);
}
