// periph.hpp  --  STUBS for Pack 08/02 (student version).  Reference: ../solutions/periph.hpp
// Register-level conventions (STM32-style USART_BRR, NVIC "top bits implemented") are from memory: VERIFY on the target.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace pp {

// ---- UART (W 3.2) ----
// 16x oversampling: BRR = round(fclk / baud).
inline uint32_t uart_brr([[maybe_unused]] uint32_t fclk, [[maybe_unused]] uint32_t baud) { return 0; }  // TODO W 3.2
inline double uart_actual_baud([[maybe_unused]] uint32_t fclk, [[maybe_unused]] uint32_t brr) { return 0.0; }  // TODO: fclk/brr
inline double uart_baud_error_percent([[maybe_unused]] uint32_t fclk, [[maybe_unused]] uint32_t baud) { return 99.0; }  // TODO
// 8N1: 10 bit times per byte.
inline double uart_byte_time_us([[maybe_unused]] uint32_t baud, [[maybe_unused]] int bits_per_frame = 10) { return 0.0; }  // TODO

// ---- SPI / I2C (W 3.3) ----
struct SpiShift {
  uint8_t master;
  uint8_t slave;
};
// n SCK cycles of a full-duplex MSB-first transfer: each clock the MSBs leave and the other side's bit enters the LSB.
inline void spi_clock_n([[maybe_unused]] SpiShift& s, [[maybe_unused]] int n) {}  // TODO W 3.3
inline int spi_mode([[maybe_unused]] int cpol, [[maybe_unused]] int cpha) { return -1; }  // TODO: cpol*2 + cpha
// Which edge samples?  Rising iff CPOL == CPHA.
inline bool spi_samples_on_rising([[maybe_unused]] int cpol, [[maybe_unused]] int cpha) { return false; }  // TODO W 3.3
inline double spi_transfer_time_us([[maybe_unused]] int bytes, [[maybe_unused]] double sck_hz) { return 0.0; }  // TODO
// 7-bit address in bits 7..1, R/W in bit 0 (1 = read).
inline uint8_t i2c_addr_byte([[maybe_unused]] uint8_t addr7, [[maybe_unused]] bool read) { return 0; }  // TODO W 3.3
// 9 bit times per byte (8 + ACK); one address byte + data_bytes data bytes; ignore START/STOP.
inline double i2c_transfer_time_us([[maybe_unused]] int data_bytes, [[maybe_unused]] double scl_hz) { return 0.0; }  // TODO

// ---- timer (W 3.4) ----
inline double timer_update_hz([[maybe_unused]] double fclk, [[maybe_unused]] uint32_t psc, [[maybe_unused]] uint32_t arr) {
  return 0.0;  // TODO W 3.4: fclk / ((psc+1) * (arr+1))
}
struct TimerCfg {
  uint32_t psc;
  uint32_t arr;
  double actual_hz;
};
// Choose psc, arr (both <= 65535) minimising |actual - target|; ties go to the SMALLEST psc.
inline TimerCfg timer_cfg_for([[maybe_unused]] double fclk, [[maybe_unused]] double target_hz) { return {0, 0, 0.0}; }  // TODO W 3.4

// ---- ADC / DAC (W 3.5) ----
// Ideal converter: LSB = Vref / 2^bits; Vin = code * LSB.
inline double adc_to_mv([[maybe_unused]] uint32_t code, [[maybe_unused]] int bits, [[maybe_unused]] double vref_mv) { return -1.0; }  // TODO
// Round to nearest code, clamp to [0, 2^bits - 1].
inline uint32_t mv_to_adc([[maybe_unused]] double mv, [[maybe_unused]] int bits, [[maybe_unused]] double vref_mv) { return 0xFFFFFFFFu; }  // TODO
inline double ideal_snr_db([[maybe_unused]] int bits) { return 0.0; }                 // TODO W 3.5: 6.02 N + 1.76
inline double enob_from_sinad([[maybe_unused]] double sinad_db) { return 0.0; }        // TODO W 3.5: (SINAD - 1.76) / 6.02

// ---- NVIC (W 3.1) ----
// Only the TOP `implemented_bits` of the 8-bit priority field exist: level = raw >> (8 - bits).
inline uint8_t nvic_level([[maybe_unused]] uint8_t raw, [[maybe_unused]] int implemented_bits) { return 255; }  // TODO W 3.1
// Lower number = more urgent; equal priority never preempts.
inline bool nvic_preempts([[maybe_unused]] uint8_t pending_raw, [[maybe_unused]] uint8_t running_raw) { return false; }  // TODO
// Among pending (irq, raw) pairs return the irq the NVIC runs first (lowest raw; tie -> lowest irq); -1 if none.
inline int nvic_pick([[maybe_unused]] const std::vector<std::pair<int, uint8_t>>& pending) { return -2; }  // TODO W 3.1

// ---- DMA ping-pong (W 3.6) ----
inline double dma_half_period_ms([[maybe_unused]] size_t half_samples, [[maybe_unused]] double fs_hz) { return 0.0; }  // TODO
class DmaRing {
 public:
  explicit DmaRing(size_t half) : half_(half), buf_(2 * half, 0) {}
  // Called with 0 or 1: the half that JUST filled (the CPU may read it while DMA fills the other half).
  std::function<void(int)> on_half_ready;
  // One more sample arrives.  Write it at the current index; before the FIRST sample of a half, if the CPU still
  // holds that half (cpu_begin without cpu_done) count an overrun; after the LAST sample of a half call
  // on_half_ready(that half); after the last sample of half 1 the index wraps to 0.
  void feed([[maybe_unused]] int16_t s) {}  // TODO W 3.6
  void cpu_begin(int h) { busy_[h] = true; }
  void cpu_done(int h) { busy_[h] = false; }
  int overruns() const { return overruns_; }
  const int16_t* half_ptr(int h) const { return buf_.data() + static_cast<size_t>(h) * half_; }

 private:
  size_t half_;
  std::vector<int16_t> buf_;
  [[maybe_unused]] size_t idx_ = 0;
  bool busy_[2] = {false, false};
  int overruns_ = 0;
};

}  // namespace pp
