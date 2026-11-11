// Tests for Pack 08/02. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "periph.hpp"

using namespace pp;

// ---------------------------------------------------------------- NVIC (W 3.1)
TEST(Nvic, TopBitsAreImplemented) {
  EXPECT_EQ(nvic_level(0x80, 4), 8);
  EXPECT_EQ(nvic_level(0x10, 4), 1);
  EXPECT_EQ(nvic_level(0xFF, 4), 15);
  EXPECT_EQ(nvic_level(0xE0, 3), 7);
  EXPECT_EQ(nvic_level(0x20, 8), 0x20);
}
TEST(Nvic, PreemptionRules) {
  EXPECT_TRUE(nvic_preempts(0x10, 0x80));
  EXPECT_FALSE(nvic_preempts(0x80, 0x10));
  EXPECT_FALSE(nvic_preempts(0x40, 0x40));  // equal priority waits (tail-chains), never preempts
}
TEST(Nvic, PickOrder) {
  EXPECT_EQ(nvic_pick({}), -1);
  EXPECT_EQ(nvic_pick({{5, 0x40}, {3, 0x20}, {9, 0x20}}), 3);  // tie on 0x20 -> lower IRQ number
  EXPECT_EQ(nvic_pick({{7, 0xFF}}), 7);
}

// ---------------------------------------------------------------- UART (W 3.2)
TEST(Uart, BrrRoundsFclkOverBaud) {
  EXPECT_EQ(uart_brr(72000000u, 115200u), 625u);
  EXPECT_EQ(uart_brr(16000000u, 115200u), 139u);  // 138.89 rounds up
  EXPECT_EQ(uart_brr(8000000u, 9600u), 833u);
}
TEST(Uart, ActualBaudAndError) {
  EXPECT_NEAR(uart_actual_baud(16000000u, 139u), 115107.9, 0.1);
  EXPECT_NEAR(uart_baud_error_percent(16000000u, 115200u), -0.08, 0.005);
  EXPECT_NEAR(uart_baud_error_percent(72000000u, 115200u), 0.0, 1e-9);
  EXPECT_NEAR(uart_baud_error_percent(8000000u, 115200u), 0.644, 0.001);  // brr 69.44 -> 69 -> 115942 baud
}
TEST(Uart, ByteTime) {
  EXPECT_NEAR(uart_byte_time_us(115200), 86.806, 1e-3);
  EXPECT_NEAR(uart_byte_time_us(9600), 1041.667, 1e-3);
  EXPECT_NEAR(uart_byte_time_us(115200, 11), 95.486, 1e-3);  // 8E1 / 8N2
}

// ---------------------------------------------------------------- SPI / I2C (W 3.3)
TEST(Spi, FullDuplexSwapsTheBytes) {
  SpiShift s{0xA5, 0x3C};
  spi_clock_n(s, 8);
  EXPECT_EQ(s.master, 0x3C);
  EXPECT_EQ(s.slave, 0xA5);
}
TEST(Spi, PartialClocksShiftIn) {
  SpiShift s{0b10110000, 0b01000000};
  spi_clock_n(s, 1);  // master MSB 1 -> slave LSB; slave MSB 0 -> master LSB
  EXPECT_EQ(s.master, 0b01100000);
  EXPECT_EQ(s.slave, 0b10000001);
  spi_clock_n(s, 0);
  EXPECT_EQ(s.master, 0b01100000);
}
TEST(Spi, ModesAndSampling) {
  EXPECT_EQ(spi_mode(0, 0), 0);
  EXPECT_EQ(spi_mode(0, 1), 1);
  EXPECT_EQ(spi_mode(1, 0), 2);
  EXPECT_EQ(spi_mode(1, 1), 3);
  EXPECT_TRUE(spi_samples_on_rising(0, 0));
  EXPECT_FALSE(spi_samples_on_rising(0, 1));
  EXPECT_FALSE(spi_samples_on_rising(1, 0));
  EXPECT_TRUE(spi_samples_on_rising(1, 1));
  EXPECT_NEAR(spi_transfer_time_us(4, 1.0e6), 32.0, 1e-9);
}
TEST(I2c, AddressByteAndTiming) {
  EXPECT_EQ(i2c_addr_byte(0x50, false), 0xA0);
  EXPECT_EQ(i2c_addr_byte(0x50, true), 0xA1);
  EXPECT_EQ(i2c_addr_byte(0x68, false), 0xD0);
  EXPECT_EQ(i2c_addr_byte(0xFF, false), 0xFE);  // address masked to 7 bits
  EXPECT_NEAR(i2c_transfer_time_us(2, 100000.0), 270.0, 1e-9);
  EXPECT_NEAR(i2c_transfer_time_us(0, 400000.0), 22.5, 1e-9);
}

// ---------------------------------------------------------------- timer (W 3.4)
TEST(Timer, UpdateRate) {
  EXPECT_NEAR(timer_update_hz(72e6, 71, 999), 1000.0, 1e-9);
  EXPECT_NEAR(timer_update_hz(72e6, 0, 8999), 8000.0, 1e-9);
}
TEST(Timer, ConfigSearch) {
  auto a = timer_cfg_for(72e6, 8000.0);
  EXPECT_EQ(a.psc, 0u);
  EXPECT_EQ(a.arr, 8999u);
  EXPECT_NEAR(a.actual_hz, 8000.0, 1e-9);
  auto b = timer_cfg_for(72e6, 1000.0);  // psc = 0 would need arr = 71999 > 65535
  EXPECT_EQ(b.psc, 1u);
  EXPECT_EQ(b.arr, 35999u);
  auto c = timer_cfg_for(72e6, 1.0);  // 1 Hz: needs both prescaler and reload
  EXPECT_NEAR(c.actual_hz, 1.0, 1e-9);
  EXPECT_LE(c.psc, 65535u);
  EXPECT_LE(c.arr, 65535u);
}

// ---------------------------------------------------------------- ADC (W 3.5)
TEST(Adc, CodeToMillivolts) {
  EXPECT_NEAR(adc_to_mv(2048, 12, 3300.0), 1650.0, 1e-9);
  EXPECT_NEAR(adc_to_mv(0, 12, 3300.0), 0.0, 1e-12);
  EXPECT_NEAR(adc_to_mv(4095, 12, 3300.0), 3299.19, 0.01);  // never reaches Vref: full scale is 1 LSB short
  EXPECT_NEAR(adc_to_mv(512, 10, 1000.0), 500.0, 1e-9);
}
TEST(Adc, MillivoltsToCodeClamps) {
  EXPECT_EQ(mv_to_adc(1650.0, 12, 3300.0), 2048u);
  EXPECT_EQ(mv_to_adc(-50.0, 12, 3300.0), 0u);
  EXPECT_EQ(mv_to_adc(5000.0, 12, 3300.0), 4095u);
  EXPECT_EQ(mv_to_adc(3300.0, 12, 3300.0), 4095u);
}
TEST(Adc, QuantisationSnr) {
  EXPECT_NEAR(ideal_snr_db(8), 49.92, 1e-9);
  EXPECT_NEAR(ideal_snr_db(12), 74.0, 1e-9);
  EXPECT_NEAR(ideal_snr_db(16), 98.08, 1e-9);
  EXPECT_NEAR(enob_from_sinad(62.0), 10.0, 0.01);
  EXPECT_NEAR(enob_from_sinad(ideal_snr_db(12)), 12.0, 1e-9);
}

// ---------------------------------------------------------------- DMA ping-pong (W 3.6)
TEST(Dma, HalfPeriod) {
  EXPECT_NEAR(dma_half_period_ms(160, 8000.0), 20.0, 1e-9);
  EXPECT_NEAR(dma_half_period_ms(256, 48000.0), 5.3333, 1e-3);
}
TEST(Dma, CallbacksFireOncePerFilledHalfInOrder) {
  DmaRing ring(4);
  std::vector<int> events;
  std::vector<std::vector<int16_t>> seen;
  ring.on_half_ready = [&](int h) {
    events.push_back(h);
    ring.cpu_begin(h);
    seen.emplace_back(ring.half_ptr(h), ring.half_ptr(h) + 4);
    ring.cpu_done(h);  // the CPU is fast: it finishes before the DMA comes back
  };
  for (int16_t i = 0; i < 12; ++i) ring.feed(i);
  ASSERT_EQ(events.size(), 3u);
  EXPECT_EQ(events[0], 0);
  EXPECT_EQ(events[1], 1);
  EXPECT_EQ(events[2], 0);  // wrapped
  ASSERT_EQ(seen.size(), 3u);
  EXPECT_EQ(seen[0], (std::vector<int16_t>{0, 1, 2, 3}));
  EXPECT_EQ(seen[1], (std::vector<int16_t>{4, 5, 6, 7}));
  EXPECT_EQ(seen[2], (std::vector<int16_t>{8, 9, 10, 11}));
  EXPECT_EQ(ring.overruns(), 0);
}
TEST(Dma, NoCallbackUntilAHalfIsFull) {
  DmaRing ring(4);
  int calls = 0;
  ring.on_half_ready = [&](int) { ++calls; };
  for (int16_t i = 0; i < 3; ++i) ring.feed(i);
  EXPECT_EQ(calls, 0);
  ring.feed(3);
  EXPECT_EQ(calls, 1);
}
TEST(Dma, SlowCpuCausesOverrun) {
  DmaRing ring(4);
  ring.on_half_ready = [&](int h) { ring.cpu_begin(h); };  // CPU grabs the half and never finishes in time
  for (int16_t i = 0; i < 8; ++i) ring.feed(i);
  EXPECT_EQ(ring.overruns(), 0);  // so far DMA only wrote into idle halves
  ring.feed(8);                   // DMA starts rewriting half 0 while the CPU still holds it
  EXPECT_EQ(ring.overruns(), 1);
  ring.cpu_done(0);
  for (int16_t i = 9; i < 12; ++i) ring.feed(i);
  EXPECT_EQ(ring.overruns(), 1);  // within a half only the FIRST sample is checked
}
