// periph.hpp  --  REFERENCE SOLUTION for Pack 08/02 (UART/SPI/I2C/timer/ADC arithmetic, NVIC priority, DMA ping-pong).
// Register-level conventions (STM32-style USART_BRR, NVIC "top bits implemented") are from memory: VERIFY on the target.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace pp {

// ------------------------------------------------------------------ UART (W 3.2)
// 16x oversampling: USARTDIV = fclk / (16 * baud); BRR holds USARTDIV * 16 = fclk / baud, rounded.
inline uint32_t uart_brr(uint32_t fclk, uint32_t baud) { return (fclk + baud / 2) / baud; }
inline double uart_actual_baud(uint32_t fclk, uint32_t brr) { return static_cast<double>(fclk) / brr; }
inline double uart_baud_error_percent(uint32_t fclk, uint32_t baud) {
  return 100.0 * (uart_actual_baud(fclk, uart_brr(fclk, baud)) - baud) / baud;
}
// 8N1 = start + 8 data + stop = 10 bit times per byte.
inline double uart_byte_time_us(uint32_t baud, int bits_per_frame = 10) { return 1.0e6 * bits_per_frame / baud; }

// ------------------------------------------------------------------ SPI / I2C (W 3.3)
// Two shift registers wired in a ring: each clock, MSBs leave and the other side's bit enters the LSB.
struct SpiShift {
  uint8_t master;
  uint8_t slave;
};
inline void spi_clock_n(SpiShift& s, int n) {
  for (int i = 0; i < n; ++i) {
    const uint8_t m_bit = (s.master >> 7) & 1u, s_bit = (s.slave >> 7) & 1u;
    s.master = static_cast<uint8_t>((s.master << 1) | s_bit);
    s.slave = static_cast<uint8_t>((s.slave << 1) | m_bit);
  }
}
inline int spi_mode(int cpol, int cpha) { return cpol * 2 + cpha; }
// Data is sampled on the RISING edge exactly when CPOL == CPHA (modes 0 and 3).
inline bool spi_samples_on_rising(int cpol, int cpha) { return cpol == cpha; }
inline double spi_transfer_time_us(int bytes, double sck_hz) { return bytes * 8.0 / sck_hz * 1.0e6; }
// 7-bit address in bits 7..1, R/W in bit 0 (1 = read).
inline uint8_t i2c_addr_byte(uint8_t addr7, bool read) { return static_cast<uint8_t>(((addr7 & 0x7Fu) << 1) | (read ? 1u : 0u)); }
// Each byte on the wire is 8 bits + 1 ACK bit; START/STOP ignored.  One address byte + n data bytes.
inline double i2c_transfer_time_us(int data_bytes, double scl_hz) { return 9.0 * (1 + data_bytes) / scl_hz * 1.0e6; }

// ------------------------------------------------------------------ timer (W 3.4)
inline double timer_update_hz(double fclk, uint32_t psc, uint32_t arr) { return fclk / (static_cast<double>(psc + 1) * (arr + 1.0)); }
struct TimerCfg {
  uint32_t psc;
  uint32_t arr;
  double actual_hz;
};
// Choose psc, arr (both <= 65535) minimising |actual - target|; ties go to the smallest psc (finest resolution).
inline TimerCfg timer_cfg_for(double fclk, double target_hz) {
  TimerCfg best{0, 0, 0.0};
  double best_err = 1e300;
  for (uint32_t psc = 0; psc <= 65535; ++psc) {
    const double arr_f = std::round(fclk / (target_hz * (psc + 1.0))) - 1.0;
    if (arr_f < 0.0 || arr_f > 65535.0) continue;
    const uint32_t arr = static_cast<uint32_t>(arr_f);
    const double hz = timer_update_hz(fclk, psc, arr);
    const double err = std::fabs(hz - target_hz);
    if (err < best_err) {
      best_err = err;
      best = {psc, arr, hz};
    }
  }
  return best;
}

// ------------------------------------------------------------------ ADC / DAC (W 3.5)
inline double adc_to_mv(uint32_t code, int bits, double vref_mv) { return code * vref_mv / static_cast<double>(1u << bits); }
inline uint32_t mv_to_adc(double mv, int bits, double vref_mv) {
  const double full = static_cast<double>(1u << bits);
  double c = std::floor(mv / vref_mv * full + 0.5);
  if (c < 0) c = 0;
  if (c > full - 1) c = full - 1;
  return static_cast<uint32_t>(c);
}
inline double ideal_snr_db(int bits) { return 6.02 * bits + 1.76; }
inline double enob_from_sinad(double sinad_db) { return (sinad_db - 1.76) / 6.02; }

// ------------------------------------------------------------------ NVIC (W 3.1)
// Only the top `implemented_bits` of the 8-bit priority field exist; lower number = more urgent.
inline uint8_t nvic_level(uint8_t raw, int implemented_bits) { return static_cast<uint8_t>(raw >> (8 - implemented_bits)); }
inline bool nvic_preempts(uint8_t pending_raw, uint8_t running_raw) { return pending_raw < running_raw; }
// Among pending (irq, raw priority) pairs pick the one the NVIC runs first: lowest raw value; tie -> lowest IRQ number.
inline int nvic_pick(const std::vector<std::pair<int, uint8_t>>& pending) {
  int best = -1;
  uint8_t best_raw = 0xFF;
  for (const auto& p : pending) {
    if (best < 0 || p.second < best_raw || (p.second == best_raw && p.first < best)) {
      best = p.first;
      best_raw = p.second;
    }
  }
  return best;
}

// ------------------------------------------------------------------ DMA ping-pong (W 3.6)
inline double dma_half_period_ms(size_t half_samples, double fs_hz) { return half_samples / fs_hz * 1000.0; }
class DmaRing {
 public:
  explicit DmaRing(size_t half) : half_(half), buf_(2 * half, 0) {}
  // Called with 0 or 1: the half that JUST filled (the CPU may now read it while DMA fills the other).
  std::function<void(int)> on_half_ready;
  // One more sample arrives from the peripheral.
  void feed(int16_t s) {
    const size_t h = idx_ / half_;
    if (idx_ % half_ == 0 && busy_[h]) ++overruns_;  // DMA is about to overwrite a half the CPU still owns
    buf_[idx_] = s;
    ++idx_;
    if (idx_ % half_ == 0) {
      const int filled = static_cast<int>(idx_ / half_) - 1;
      if (idx_ == buf_.size()) idx_ = 0;  // circular mode: wrap
      if (on_half_ready) on_half_ready(filled);
    }
  }
  void cpu_begin(int h) { busy_[h] = true; }
  void cpu_done(int h) { busy_[h] = false; }
  int overruns() const { return overruns_; }
  const int16_t* half_ptr(int h) const { return buf_.data() + static_cast<size_t>(h) * half_; }

 private:
  size_t half_;
  std::vector<int16_t> buf_;
  size_t idx_ = 0;
  bool busy_[2] = {false, false};
  int overruns_ = 0;
};

}  // namespace pp
