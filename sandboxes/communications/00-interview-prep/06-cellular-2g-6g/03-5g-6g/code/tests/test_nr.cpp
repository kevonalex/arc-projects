// Tests for Pack 06/03. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include "nr.hpp"

using namespace nr;

// ---------------------------------------------------------------- numerology (W 3.1)
TEST(Numerology, ScsDoublesWithMu) {
  EXPECT_EQ(scs_khz(0), 15);
  EXPECT_EQ(scs_khz(1), 30);
  EXPECT_EQ(scs_khz(2), 60);
  EXPECT_EQ(scs_khz(3), 120);
  EXPECT_EQ(scs_khz(4), 240);
  EXPECT_EQ(scs_khz(-1), -1);
  EXPECT_EQ(scs_khz(5), -1);
}
TEST(Numerology, SlotsHalveInDuration) {
  for (int mu = 0; mu <= 4; ++mu) {
    EXPECT_EQ(slots_per_subframe(mu), 1 << mu);
    EXPECT_EQ(slots_per_frame(mu), 10 << mu);
    EXPECT_NEAR(slot_ms(mu) * slots_per_subframe(mu), 1.0, 1e-12);  // a subframe is always 1 ms
  }
  EXPECT_DOUBLE_EQ(slot_ms(0), 1.0);
  EXPECT_DOUBLE_EQ(slot_ms(1), 0.5);
  EXPECT_DOUBLE_EQ(slot_ms(3), 0.125);
  EXPECT_EQ(slot_ms(9), -1.0);
}
TEST(Numerology, SymbolLengthIsInverseOfScs) {
  EXPECT_EQ(symbols_per_slot(), 14);
  EXPECT_NEAR(symbol_useful_us(0), 66.6667, 1e-3);
  EXPECT_NEAR(symbol_useful_us(1), 33.3333, 1e-3);
  EXPECT_NEAR(symbol_useful_us(3), 8.3333, 1e-3);
  EXPECT_NEAR(symbol_avg_us(0), 71.4286, 1e-3);
  EXPECT_NEAR(symbol_avg_us(1), 35.7143, 1e-3);
  EXPECT_NEAR(symbol_avg_us(3), 8.9286, 1e-3);
  for (int mu = 0; mu < 4; ++mu) EXPECT_NEAR(symbol_useful_us(mu + 1) * 2.0, symbol_useful_us(mu), 1e-9);
}

// ---------------------------------------------------------------- allowed numerologies, PRBs, TDD (W 3.2)
TEST(Numerology, FrequencyRanges) {
  EXPECT_TRUE(numerology_allowed(FreqRange::FR1, 0));
  EXPECT_TRUE(numerology_allowed(FreqRange::FR1, 1));
  EXPECT_TRUE(numerology_allowed(FreqRange::FR1, 2));
  EXPECT_FALSE(numerology_allowed(FreqRange::FR1, 3));
  EXPECT_FALSE(numerology_allowed(FreqRange::FR2, 1));
  EXPECT_TRUE(numerology_allowed(FreqRange::FR2, 2));
  EXPECT_TRUE(numerology_allowed(FreqRange::FR2, 3));
}
TEST(Numerology, PrbTable) {
  EXPECT_EQ(prb_count(20, 0), 106);
  EXPECT_EQ(prb_count(20, 1), 51);
  EXPECT_EQ(prb_count(100, 1), 273);
  EXPECT_EQ(prb_count(100, 3), 66);
  EXPECT_EQ(prb_count(400, 3), 264);
  EXPECT_EQ(prb_count(7, 0), -1);
}
TEST(Numerology, TddPatternSlots) {
  EXPECT_EQ(slots_in_period(2.5, 1), 5);   // classic DDDSU at 30 kHz SCS
  EXPECT_EQ(slots_in_period(5.0, 1), 10);
  EXPECT_EQ(slots_in_period(5.0, 3), 40);
  EXPECT_EQ(slots_in_period(1.0, 0), 1);
}

// ---------------------------------------------------------------- coding (W 3.4)
TEST(Coding, DataVersusControl) {
  EXPECT_EQ(channel_code_for(NrChannel::PDSCH), Code::LDPC);
  EXPECT_EQ(channel_code_for(NrChannel::PUSCH), Code::LDPC);
  EXPECT_EQ(channel_code_for(NrChannel::PDCCH), Code::Polar);
  EXPECT_EQ(channel_code_for(NrChannel::PBCH), Code::Polar);
}
TEST(Coding, BaseGraphSelection) {
  EXPECT_EQ(ldpc_base_graph(200, 0.9), 2);     // tiny block: BG2 whatever the rate
  EXPECT_EQ(ldpc_base_graph(10000, 0.2), 2);   // very low rate: BG2
  EXPECT_EQ(ldpc_base_graph(3000, 0.5), 2);    // small and not high-rate: BG2
  EXPECT_EQ(ldpc_base_graph(3000, 0.8), 1);    // small but high-rate: BG1
  EXPECT_EQ(ldpc_base_graph(20000, 0.5), 1);   // big block: BG1
}
TEST(Coding, CodeBlockSegmentation) {
  EXPECT_EQ(num_code_blocks(8448, 1), 1);
  EXPECT_EQ(num_code_blocks(8449, 1), 2);
  EXPECT_EQ(num_code_blocks(16848, 1), 2);  // 2 * (8448 - 24)
  EXPECT_EQ(num_code_blocks(16849, 1), 3);
  EXPECT_EQ(num_code_blocks(3840, 2), 1);
  EXPECT_EQ(num_code_blocks(10000, 2), 3);  // ceil(10000 / 3816)
}

// ---------------------------------------------------------------- link physics (W 3.5 - 3.6)
TEST(Link, FreeSpacePathLoss) {
  EXPECT_NEAR(fspl_db(2.4e9, 1.0), 40.05, 0.01);
  EXPECT_NEAR(fspl_db(3.5e9, 1000.0), 103.33, 0.01);
  EXPECT_NEAR(fspl_db(28e9, 1000.0), 121.39, 0.01);
  EXPECT_NEAR(fspl_db(2e9, 550e3), 153.28, 0.01);     // LEO satellite
  EXPECT_NEAR(fspl_db(2e9, 35786e3), 189.54, 0.01);   // GEO satellite
  EXPECT_NEAR(fspl_db(2e9, 2000.0) - fspl_db(2e9, 1000.0), 6.0206, 1e-3);  // double distance: +6 dB
  EXPECT_NEAR(fspl_db(28e9, 1000.0) - fspl_db(2.8e9, 1000.0), 20.0, 1e-9); // 10x frequency: +20 dB
}
TEST(Link, ShannonAndDelayAndDoppler) {
  EXPECT_NEAR(shannon_capacity_bps(20e6, 20.0) / 1e6, 133.16, 0.01);
  EXPECT_NEAR(shannon_capacity_bps(100e6, 10.0) / 1e6, 345.94, 0.01);
  EXPECT_NEAR(one_way_delay_ms(550.0), 1.8346, 1e-3);
  EXPECT_NEAR(one_way_delay_ms(35786.0), 119.37, 0.01);
  EXPECT_NEAR(max_doppler_hz(500.0 / 3.6, 3.5e9), 1621.5, 0.5);   // high-speed train at 3.5 GHz
  EXPECT_NEAR(max_doppler_hz(7560.0, 2e9), 50435.0, 1.0);          // LEO at 2 GHz
}

// ---------------------------------------------------------------- generations (W 3.7)
TEST(Generations, ThereIsNoSeventhGeneration) {
  EXPECT_EQ(generation_status(2), StandardStatus::Deployed);
  EXPECT_EQ(generation_status(5), StandardStatus::Deployed);
  EXPECT_EQ(generation_status(6), StandardStatus::InStandardisation);
  EXPECT_EQ(generation_status(7), StandardStatus::NoStandard);
  EXPECT_EQ(generation_status(8), StandardStatus::NoStandard);
  EXPECT_EQ(generation_status(1), StandardStatus::NoStandard);  // 1G was analogue, never a 3GPP generation
}
