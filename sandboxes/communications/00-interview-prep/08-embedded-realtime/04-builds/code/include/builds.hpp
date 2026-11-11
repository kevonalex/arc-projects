// builds.hpp  --  STUBS for Pack 08/04 (student version).  Reference: ../solutions/builds.hpp
// Constraints for firmware-style code: no exceptions, no RTTI, no heap on the hot path, bounded ISRs.
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "radio_model.hpp"
#include "rtos_policy.hpp"
#include "uart_model.hpp"

namespace builds {

// ---- build 1: driver for the emulated radio (W 3.1) ----
// Bus has uint32_t read32(uint32_t) and void write32(uint32_t, uint32_t).  Use ONLY the register map in
// common/radio_model.hpp (REG_*, CTRL_*, ST_*, IRQ_*, kFreqStepHz); never touch the model's internals.
template <class Bus>
class RadioDriver {
 public:
  explicit RadioDriver(Bus& bus) : bus_(bus) {}
  void enable([[maybe_unused]] bool on) {}          // TODO W 3.1: CTRL.EN
  void enable_rx_irq([[maybe_unused]] bool on) {}   // TODO W 3.1: IRQ_EN = RX | OVF (or 0)
  // 30 MHz .. 6 GHz, 25 Hz steps (round to nearest); return false and write nothing when out of range.
  bool set_frequency_hz([[maybe_unused]] uint64_t hz) { return true; }  // TODO W 3.1
  uint64_t frequency_hz() { return 0; }                                  // TODO W 3.1: read FREQ * step
  // If STATUS.RXRDY is set read DATA exactly once (the read pops the FIFO) and return true.
  bool read_sample([[maybe_unused]] int16_t& out) { return true; }       // TODO W 3.1
  size_t drain([[maybe_unused]] std::vector<int16_t>& sink) { return 999; }  // TODO W 3.1: loop read_sample
  bool overflowed() { return false; }                                    // TODO W 3.1: STATUS.RXOVF
  // IRQ_STAT is write-1-to-clear: WRITE the bit; never read-modify-write it.
  void clear_overflow() {}                                               // TODO W 3.1
  void ack_rx_irq() {}                                                   // TODO W 3.1

 private:
  [[maybe_unused]] Bus& bus_;
};

// ---- build 2: SPSC ring (W 3.2) ----
// One producer, one consumer, N a power of two, usable capacity N.  Free-running head/tail; publish with
// release stores, observe with acquire loads.  No locks, no allocation.
template <class T, size_t N>
class SpscRing {
  static_assert(N != 0 && (N & (N - 1)) == 0, "N must be a power of two");

 public:
  bool push([[maybe_unused]] const T& v) { return true; }   // TODO W 3.2
  bool pop([[maybe_unused]] T& out) { return true; }        // TODO W 3.2
  size_t size() const { return 999; }                         // TODO W 3.2
  bool empty() const { return false; }                        // TODO W 3.2

 private:
  [[maybe_unused]] T buf_[N];
  [[maybe_unused]] std::atomic<size_t> head_{0}, tail_{0};
};

// ---- build 3: interrupt-driven UART over SPSC rings (W 3.3) ----
template <size_t TxN, size_t RxN>
class UartDriver {
 public:
  explicit UartDriver(uartsim::UartModel& u) : u_(u) { u_.rxneie = true; }
  // Main context: queue as many bytes as fit into the TX ring, return that count, then enable txeie.
  size_t write([[maybe_unused]] const uint8_t* p, [[maybe_unused]] size_t n) { return 0; }  // TODO W 3.3
  bool read([[maybe_unused]] uint8_t& b) { return false; }                                   // TODO W 3.3
  // ISR: if rxne -> read DR once, push to RX ring (count rx_dropped if full).
  //      if txe && txeie -> pop one byte and write DR; if the ring is empty clear txeie (mask the interrupt).
  void isr() {}  // TODO W 3.3
  size_t rx_dropped() const { return 999; }  // TODO W 3.3

 private:
  [[maybe_unused]] uartsim::UartModel& u_;
  [[maybe_unused]] SpscRing<uint8_t, TxN> tx_;
  [[maybe_unused]] SpscRing<uint8_t, RxN> rx_;
};

// ---- build 4: audio pipeline, inversion then fix (W 3.4) ----
struct Scenario {
  std::vector<rtos::TaskSpec> tasks;
  int nmutex = 0;
  size_t critical = 0;     // index of the deadline-critical task
  int deadline_ticks = 0;  // max allowed (finish - start) for that task; 1 tick = 1 ms
};
// Design a 3-task scenario (see worksheet W 3.4): a HIGH-priority codec task and a LOW-priority logger task share
// mutex 0; a MEDIUM-priority CPU-bound DSP task does not.  Pick numbers so that WITHOUT inheritance the codec
// misses `deadline_ticks` and WITH inheritance it meets it.
inline Scenario audio_pipeline_scenario() { return Scenario{}; }  // TODO W 3.4
// 1 if the critical task misses its deadline (or never finishes), else 0.  Use rtos::simulate and rtos::response_ticks.
inline int deadline_misses([[maybe_unused]] const Scenario& sc, [[maybe_unused]] bool inheritance) { return -1; }  // TODO W 3.4

}  // namespace builds
