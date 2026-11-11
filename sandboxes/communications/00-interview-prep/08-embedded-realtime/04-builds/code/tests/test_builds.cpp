// Tests for Pack 08/04. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>

#include "builds.hpp"

using namespace builds;

// ---------------------------------------------------------------- build 1: radio driver (W 3.1)
namespace {
// "DSP simulator": a deterministic tone, fed to the peripheral's RX FIFO.
std::vector<int16_t> tone(size_t n) {
  std::vector<int16_t> v;
  for (size_t i = 0; i < n; ++i) v.push_back(static_cast<int16_t>(std::lround(10000.0 * std::sin(0.3 * static_cast<double>(i)))));
  return v;
}
}  // namespace

TEST(RadioDriver, FrequencyRoundTripAndLimits) {
  radio::RadioModel m;
  RadioDriver<radio::RadioModel> d(m);
  ASSERT_TRUE(d.set_frequency_hz(446006250ull));  // 446.00625 MHz = 17840250 * 25 Hz
  EXPECT_EQ(m.read32(radio::REG_FREQ), 17840250u);
  EXPECT_EQ(d.frequency_hz(), 446006250ull);
  ASSERT_TRUE(d.set_frequency_hz(446006260ull));  // rounds to the nearest 25 Hz step
  EXPECT_EQ(d.frequency_hz(), 446006250ull);
  EXPECT_FALSE(d.set_frequency_hz(1000ull));
  EXPECT_FALSE(d.set_frequency_hz(7000000000ull));
  EXPECT_EQ(d.frequency_hz(), 446006250ull);  // rejected writes changed nothing
}
TEST(RadioDriver, EnableWritesCtrl) {
  radio::RadioModel m;
  RadioDriver<radio::RadioModel> d(m);
  d.enable(true);
  EXPECT_EQ(m.read32(radio::REG_CTRL) & radio::CTRL_EN, radio::CTRL_EN);
  d.enable(false);
  EXPECT_EQ(m.read32(radio::REG_CTRL) & radio::CTRL_EN, 0u);
}
TEST(RadioDriver, DrainsSamplesInOrder) {
  radio::RadioModel m;
  RadioDriver<radio::RadioModel> d(m);
  d.enable(true);
  const auto x = tone(6);
  for (int16_t s : x) m.push_rx_sample(s);
  int16_t first = 0;
  ASSERT_TRUE(d.read_sample(first));
  EXPECT_EQ(first, x[0]);
  std::vector<int16_t> rest;
  EXPECT_EQ(d.drain(rest), 5u);
  EXPECT_EQ(rest, std::vector<int16_t>(x.begin() + 1, x.end()));
  EXPECT_FALSE(d.read_sample(first));  // FIFO empty: no DATA read happened
  EXPECT_EQ(d.drain(rest), 0u);
}
TEST(RadioDriver, ReceiverOffDropsSamples) {
  radio::RadioModel m;
  RadioDriver<radio::RadioModel> d(m);
  m.push_rx_sample(123);
  std::vector<int16_t> sink;
  EXPECT_EQ(d.drain(sink), 0u);
}
TEST(RadioDriver, FifoOverflowIsStickyUntilCleared) {
  radio::RadioModel m;
  RadioDriver<radio::RadioModel> d(m);
  d.enable(true);
  const auto x = tone(radio::kRxFifoDepth + 3);
  for (int16_t s : x) m.push_rx_sample(s);
  EXPECT_TRUE(d.overflowed());
  std::vector<int16_t> sink;
  EXPECT_EQ(d.drain(sink), radio::kRxFifoDepth);
  EXPECT_EQ(sink, std::vector<int16_t>(x.begin(), x.begin() + radio::kRxFifoDepth));  // the newest 3 were lost
  EXPECT_TRUE(d.overflowed());  // draining does not clear it
  d.clear_overflow();
  EXPECT_FALSE(d.overflowed());
}
TEST(RadioDriver, InterruptHandshake) {
  radio::RadioModel m;
  RadioDriver<radio::RadioModel> d(m);
  d.enable(true);
  d.enable_rx_irq(true);
  EXPECT_FALSE(m.irq_line());
  m.push_rx_sample(7);
  EXPECT_TRUE(m.irq_line());
  std::vector<int16_t> sink;
  d.drain(sink);
  EXPECT_TRUE(m.irq_line());  // still asserted: draining the FIFO does not acknowledge the interrupt
  d.ack_rx_irq();
  EXPECT_FALSE(m.irq_line());
  d.enable_rx_irq(false);
  m.push_rx_sample(8);
  EXPECT_FALSE(m.irq_line());  // masked
}

// ---------------------------------------------------------------- build 2: SPSC ring (W 3.2)
TEST(Spsc, FifoOrderFullAndEmpty) {
  SpscRing<int, 4> r;
  int v = -1;
  EXPECT_TRUE(r.empty());
  EXPECT_FALSE(r.pop(v));
  for (int i = 1; i <= 4; ++i) EXPECT_TRUE(r.push(i * 10));
  EXPECT_FALSE(r.push(50));  // full at N elements
  EXPECT_EQ(r.size(), 4u);
  for (int i = 1; i <= 4; ++i) {
    ASSERT_TRUE(r.pop(v));
    EXPECT_EQ(v, i * 10);
  }
  EXPECT_TRUE(r.empty());
}
TEST(Spsc, WrapsAroundManyTimes) {
  SpscRing<uint32_t, 8> r;
  uint32_t out = 0;
  for (uint32_t i = 0; i < 1000; ++i) {
    ASSERT_TRUE(r.push(i));
    ASSERT_TRUE(r.push(i + 100000));
    ASSERT_TRUE(r.pop(out));
    EXPECT_EQ(out, i);
    ASSERT_TRUE(r.pop(out));
    EXPECT_EQ(out, i + 100000);
  }
  EXPECT_TRUE(r.empty());
}
TEST(Spsc, TwoThreadsPreserveOrderAndLoseNothing) {
  SpscRing<uint32_t, 64> r;
  const uint32_t total = 200000;
  std::thread prod([&] {
    for (uint32_t i = 0; i < total;) {
      if (r.push(i)) ++i;
    }
  });
  std::vector<uint32_t> got;
  got.reserve(total);
  uint64_t spins = 0;
  while (got.size() < total && spins < 400000000ull) {  // bounded: a broken ring must fail, not hang
    uint32_t v = 0;
    if (r.pop(v))
      got.push_back(v);
    else
      ++spins;
  }
  prod.join();  // always join before asserting
  ASSERT_EQ(got.size(), static_cast<size_t>(total));
  bool in_order = true;
  for (uint32_t i = 0; i < total; ++i) in_order = in_order && got[i] == i;
  EXPECT_TRUE(in_order);
  EXPECT_TRUE(r.empty());
}

// ---------------------------------------------------------------- build 3: UART driver (W 3.3)
namespace {
// The interrupt controller: keep entering the ISR while the line is high.  Bounded so a broken ISR fails the test
// instead of hanging it (a real unmasked, never-cleared interrupt WOULD hang the CPU: that is the lesson).
template <class Drv>
void service(uartsim::UartModel& u, Drv& d) {
  for (int i = 0; i < 64 && u.irq_pending(); ++i) d.isr();
}
template <class Drv>
void pump(uartsim::UartModel& u, Drv& d, int ticks) {
  for (int t = 0; t < ticks; ++t) {
    service(u, d);
    u.tick();
  }
}
}  // namespace
TEST(Uart, TransmitsEverythingInOrderThenMasksItsInterrupt) {
  uartsim::UartModel u;
  UartDriver<16, 16> d(u);
  const uint8_t msg[] = {'H', 'E', 'L', 'L', 'O', '!', 0x00, 0xFF};
  EXPECT_EQ(d.write(msg, sizeof msg), sizeof msg);
  pump(u, d, 40);
  EXPECT_EQ(u.wire, std::vector<uint8_t>(msg, msg + sizeof msg));
  EXPECT_FALSE(u.txeie);  // an unmasked TXE interrupt with nothing to send would spin the CPU forever
  EXPECT_FALSE(u.irq_pending());
}
TEST(Uart, WriteReportsHowManyFit) {
  uartsim::UartModel u;
  UartDriver<4, 4> d(u);
  const uint8_t big[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  EXPECT_EQ(d.write(big, sizeof big), 4u);  // ring holds 4; the rest is the caller's problem (backpressure)
  pump(u, d, 20);
  EXPECT_EQ(u.wire, (std::vector<uint8_t>{1, 2, 3, 4}));
  EXPECT_EQ(d.write(big + 4, 6), 4u);       // ring drained: room again
}
TEST(Uart, ReceivesViaInterruptIntoRing) {
  uartsim::UartModel u;
  UartDriver<8, 8> d(u);
  for (uint8_t b : {0x10, 0x20, 0x30}) {
    u.inject_rx(b);
    service(u, d);
  }
  uint8_t b = 0;
  ASSERT_TRUE(d.read(b));
  EXPECT_EQ(b, 0x10);
  ASSERT_TRUE(d.read(b));
  EXPECT_EQ(b, 0x20);
  ASSERT_TRUE(d.read(b));
  EXPECT_EQ(b, 0x30);
  EXPECT_FALSE(d.read(b));
  EXPECT_EQ(u.overruns, 0);
  EXPECT_EQ(d.rx_dropped(), 0u);
}
TEST(Uart, FullRxRingDropsAndCounts) {
  uartsim::UartModel u;
  UartDriver<4, 4> d(u);
  for (uint8_t i = 0; i < 6; ++i) {
    u.inject_rx(i);
    service(u, d);
  }
  EXPECT_EQ(d.rx_dropped(), 2u);
  uint8_t b = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    ASSERT_TRUE(d.read(b));
    EXPECT_EQ(b, i);  // the OLDEST four survive; the newest two were dropped
  }
  EXPECT_FALSE(d.read(b));
}
TEST(Uart, FullDuplexTrafficDoesNotInterfere) {
  uartsim::UartModel u;
  UartDriver<16, 16> d(u);
  const uint8_t out[] = {1, 2, 3, 4, 5};
  d.write(out, sizeof out);
  for (int t = 0; t < 30; ++t) {
    if (t == 2 || t == 5) u.inject_rx(static_cast<uint8_t>(0xA0 + t));
    service(u, d);
    u.tick();
  }
  EXPECT_EQ(u.wire, std::vector<uint8_t>(out, out + sizeof out));
  uint8_t b = 0;
  ASSERT_TRUE(d.read(b));
  EXPECT_EQ(b, 0xA2);
  ASSERT_TRUE(d.read(b));
  EXPECT_EQ(b, 0xA5);
}

// ---------------------------------------------------------------- build 4: priority inversion in an audio pipeline (W 3.4)
TEST(AudioPipeline, ScenarioIsStructurallyAnInversionSetup) {
  Scenario s = audio_pipeline_scenario();
  ASSERT_GE(s.tasks.size(), 3u);
  ASSERT_GE(s.nmutex, 1);
  ASSERT_LT(s.critical, s.tasks.size());
  ASSERT_GT(s.deadline_ticks, 0);
  auto locks = [](const rtos::TaskSpec& t) {
    for (const auto& st : t.steps)
      if (st.op == rtos::Op::Lock && st.arg == 0) return true;
    return false;
  };
  int crit_prio = s.tasks[s.critical].prio;
  EXPECT_TRUE(locks(s.tasks[s.critical]));
  bool low_locker = false, mid_hog = false;
  for (size_t i = 0; i < s.tasks.size(); ++i) {
    if (i == s.critical) continue;
    EXPECT_LT(s.tasks[i].prio, crit_prio) << "the critical task must have the highest priority";
    if (locks(s.tasks[i])) low_locker = true;
    int run = 0;
    for (const auto& st : s.tasks[i].steps)
      if (st.op == rtos::Op::Run) run += st.arg;
    if (!locks(s.tasks[i]) && run >= 3) mid_hog = true;
  }
  EXPECT_TRUE(low_locker);
  EXPECT_TRUE(mid_hog);
}
TEST(AudioPipeline, InversionMissesTheDeadlineInheritanceFixesIt) {
  Scenario s = audio_pipeline_scenario();
  EXPECT_EQ(deadline_misses(s, false), 1);
  EXPECT_EQ(deadline_misses(s, true), 0);
}
