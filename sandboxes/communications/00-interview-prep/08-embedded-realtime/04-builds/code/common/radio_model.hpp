// radio_model.hpp  --  an EMULATED radio peripheral exposed as a register map (GIVEN, not a stub).
// Think of it as the memory-mapped block of a tiny SDR front-end: firmware talks to it only through read32/write32,
// exactly as it would through a volatile pointer. A DSP simulator feeds it samples with push_rx_sample().
// The register layout is invented for this exercise (it is NOT any real chip's).
#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>

namespace radio {

// ---- register map (byte offsets) ----
constexpr uint32_t REG_CTRL = 0x00;      // bit0 EN, bit1 TXSTART (self-clearing in a real chip; stored here)
constexpr uint32_t REG_STATUS = 0x04;    // bit0 TXBUSY, bit1 RXRDY (FIFO not empty), bit2 RXOVF (sticky)
constexpr uint32_t REG_DATA = 0x08;      // read: pop one RX sample (16-bit, zero-extended); write: TX sample
constexpr uint32_t REG_FREQ = 0x0C;      // carrier in units of kFreqStepHz
constexpr uint32_t REG_IRQ_EN = 0x10;    // bit0 RX, bit1 OVF
constexpr uint32_t REG_IRQ_STAT = 0x14;  // write-1-to-clear

constexpr uint32_t CTRL_EN = 1u << 0;
constexpr uint32_t CTRL_TXSTART = 1u << 1;
constexpr uint32_t ST_TXBUSY = 1u << 0;
constexpr uint32_t ST_RXRDY = 1u << 1;
constexpr uint32_t ST_RXOVF = 1u << 2;
constexpr uint32_t IRQ_RX = 1u << 0;
constexpr uint32_t IRQ_OVF = 1u << 1;
constexpr uint32_t kFreqStepHz = 25;
constexpr size_t kRxFifoDepth = 8;

class RadioModel {
 public:
  uint32_t read32(uint32_t off) {
    switch (off) {
      case REG_CTRL: return ctrl_;
      case REG_STATUS: return (txbusy_ ? ST_TXBUSY : 0u) | (!fifo_.empty() ? ST_RXRDY : 0u) | (ovf_ ? ST_RXOVF : 0u);
      case REG_DATA: {  // side effect: pops the FIFO
        if (fifo_.empty()) return 0;
        const uint16_t s = static_cast<uint16_t>(fifo_.front());
        fifo_.pop_front();
        return s;
      }
      case REG_FREQ: return freq_;
      case REG_IRQ_EN: return irq_en_;
      case REG_IRQ_STAT: return irq_stat_;
      default: return 0;
    }
  }
  void write32(uint32_t off, uint32_t v) {
    switch (off) {
      case REG_CTRL: ctrl_ = v; break;
      case REG_DATA: tx_log_.push_back(static_cast<int16_t>(v)); break;
      case REG_FREQ: freq_ = v; break;
      case REG_IRQ_EN: irq_en_ = v; break;
      case REG_IRQ_STAT:  // write-1-to-clear; clearing OVF also clears the sticky status flag
        irq_stat_ &= ~v;
        if (v & IRQ_OVF) ovf_ = false;
        break;
      default: break;
    }
  }
  // ---- simulation side: what the antenna / DSP simulator does ----
  void push_rx_sample(int16_t s) {
    if (!(ctrl_ & CTRL_EN)) return;  // receiver off: sample is dropped
    if (fifo_.size() >= kRxFifoDepth) {
      ovf_ = true;
      irq_stat_ |= IRQ_OVF;
    } else {
      fifo_.push_back(s);
      irq_stat_ |= IRQ_RX;
    }
  }
  bool irq_line() const { return (irq_stat_ & irq_en_) != 0; }
  const std::deque<int16_t>& tx_log() const { return tx_log_; }

 private:
  uint32_t ctrl_ = 0, freq_ = 0, irq_en_ = 0, irq_stat_ = 0;
  bool txbusy_ = false, ovf_ = false;
  std::deque<int16_t> fifo_;
  std::deque<int16_t> tx_log_;
};

}  // namespace radio
