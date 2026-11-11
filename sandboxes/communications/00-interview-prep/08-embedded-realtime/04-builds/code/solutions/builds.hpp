// builds.hpp  --  REFERENCE SOLUTION for Pack 08/04 (radio driver over a register map, SPSC ring,
// interrupt-driven UART, priority-inversion audio pipeline).  No exceptions, no RTTI, no heap on the hot path.
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "radio_model.hpp"
#include "rtos_policy.hpp"
#include "uart_model.hpp"

namespace builds {

// ------------------------------------------------------------------ build 1: driver for the emulated radio (W 3.1)
// Bus is anything with uint32_t read32(uint32_t) and void write32(uint32_t, uint32_t): the model here, a volatile
// pointer wrapper on real hardware.  The driver never touches the model's internals - only the register map.
template <class Bus>
class RadioDriver {
 public:
  explicit RadioDriver(Bus& bus) : bus_(bus) {}
  void enable(bool on) { bus_.write32(radio::REG_CTRL, on ? radio::CTRL_EN : 0u); }
  void enable_rx_irq(bool on) { bus_.write32(radio::REG_IRQ_EN, on ? (radio::IRQ_RX | radio::IRQ_OVF) : 0u); }
  // 30 MHz .. 6 GHz in 25 Hz steps; returns false (and writes nothing) outside the range.
  bool set_frequency_hz(uint64_t hz) {
    if (hz < 30000000ull || hz > 6000000000ull) return false;
    bus_.write32(radio::REG_FREQ, static_cast<uint32_t>((hz + radio::kFreqStepHz / 2) / radio::kFreqStepHz));
    return true;
  }
  uint64_t frequency_hz() { return static_cast<uint64_t>(bus_.read32(radio::REG_FREQ)) * radio::kFreqStepHz; }
  // Poll STATUS.RXRDY; if set, read DATA exactly once (the read pops the FIFO).
  bool read_sample(int16_t& out) {
    if (!(bus_.read32(radio::REG_STATUS) & radio::ST_RXRDY)) return false;
    out = static_cast<int16_t>(bus_.read32(radio::REG_DATA));
    return true;
  }
  size_t drain(std::vector<int16_t>& sink) {
    size_t n = 0;
    int16_t s;
    while (read_sample(s)) {
      sink.push_back(s);
      ++n;
    }
    return n;
  }
  bool overflowed() { return (bus_.read32(radio::REG_STATUS) & radio::ST_RXOVF) != 0; }
  // IRQ_STAT is write-1-to-clear: write the bit, do NOT read-modify-write.
  void clear_overflow() { bus_.write32(radio::REG_IRQ_STAT, radio::IRQ_OVF); }
  void ack_rx_irq() { bus_.write32(radio::REG_IRQ_STAT, radio::IRQ_RX); }

 private:
  Bus& bus_;
};

// ------------------------------------------------------------------ build 2: SPSC ring (W 3.2)
// One producer thread/ISR, one consumer.  N must be a power of two; usable capacity is N.
// Indices are free-running; head is written only by the producer, tail only by the consumer.
template <class T, size_t N>
class SpscRing {
  static_assert(N != 0 && (N & (N - 1)) == 0, "N must be a power of two");

 public:
  bool push(const T& v) {
    const size_t h = head_.load(std::memory_order_relaxed);
    if (h - tail_.load(std::memory_order_acquire) == N) return false;  // full
    buf_[h & (N - 1)] = v;
    head_.store(h + 1, std::memory_order_release);  // publish the element AFTER it is written
    return true;
  }
  bool pop(T& out) {
    const size_t t = tail_.load(std::memory_order_relaxed);
    if (t == head_.load(std::memory_order_acquire)) return false;  // empty
    out = buf_[t & (N - 1)];
    tail_.store(t + 1, std::memory_order_release);  // free the slot AFTER it is read
    return true;
  }
  size_t size() const { return head_.load(std::memory_order_acquire) - tail_.load(std::memory_order_acquire); }
  bool empty() const { return size() == 0; }

 private:
  T buf_[N];
  std::atomic<size_t> head_{0}, tail_{0};
};

// ------------------------------------------------------------------ build 3: interrupt-driven UART over SPSC rings (W 3.3)
template <size_t TxN, size_t RxN>
class UartDriver {
 public:
  explicit UartDriver(uartsim::UartModel& u) : u_(u) { u_.rxneie = true; }
  // Thread/main context: queue bytes, return how many fit; then enable the TX-empty interrupt to start draining.
  size_t write(const uint8_t* p, size_t n) {
    size_t k = 0;
    while (k < n && tx_.push(p[k])) ++k;
    if (k > 0) u_.txeie = true;
    return k;
  }
  bool read(uint8_t& b) { return rx_.pop(b); }
  // ISR context: short, bounded, no allocation, no blocking.
  void isr() {
    if (u_.rxne) {
      const uint8_t b = u_.read_dr();
      if (!rx_.push(b)) ++rx_dropped_;
    }
    if (u_.txe && u_.txeie) {
      uint8_t b;
      if (tx_.pop(b))
        u_.write_dr(b);
      else
        u_.txeie = false;  // nothing left: mask the interrupt or it fires forever
    }
  }
  size_t rx_dropped() const { return rx_dropped_; }

 private:
  uartsim::UartModel& u_;
  SpscRing<uint8_t, TxN> tx_;
  SpscRing<uint8_t, RxN> rx_;
  size_t rx_dropped_ = 0;
};

// ------------------------------------------------------------------ build 4: audio pipeline, inversion then fix (W 3.4)
struct Scenario {
  std::vector<rtos::TaskSpec> tasks;
  int nmutex = 0;
  size_t critical = 0;  // index of the deadline-critical task
  int deadline_ticks = 0;  // max allowed (finish - start) for that task; 1 tick = 1 ms
};
// codec_out (prio 3) needs the frame-pool mutex that logger (prio 1) holds; dsp_resample (prio 2) is CPU-bound.
inline Scenario audio_pipeline_scenario() {
  using rtos::Op;
  Scenario s;
  s.tasks = {
      {"codec_out", 3, 2, {{Op::Lock, 0}, {Op::Run, 2}, {Op::Unlock, 0}}},
      {"dsp_resample", 2, 3, {{Op::Run, 10}}},
      {"logger", 1, 0, {{Op::Lock, 0}, {Op::Run, 5}, {Op::Unlock, 0}}},
  };
  s.nmutex = 1;
  s.critical = 0;
  s.deadline_ticks = 8;
  return s;
}
// 1 if the critical task misses its deadline (or never finishes), else 0.
inline int deadline_misses(const Scenario& sc, bool inheritance) {
  const rtos::Result r = rtos::simulate(sc.tasks, sc.nmutex, inheritance, 200);
  const int resp = rtos::response_ticks(r, sc.tasks, sc.critical);
  return (resp < 0 || resp > sc.deadline_ticks) ? 1 : 0;
}

}  // namespace builds
