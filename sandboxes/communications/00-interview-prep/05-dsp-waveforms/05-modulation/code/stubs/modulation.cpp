// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "modulation.hpp"

#include <stdexcept>

namespace dsp {

int fsk4_level([[maybe_unused]] std::uint8_t dibit) {
  // TODO B1
  return 0;
}

std::vector<cplx> fsk4_modulate([[maybe_unused]] const std::vector<std::uint8_t>& dibits,
                                [[maybe_unused]] std::size_t sps, [[maybe_unused]] double dev_unit_hz,
                                [[maybe_unused]] double fs_hz) {
  // TODO B1
  return {};
}

std::vector<std::uint8_t> fsk4_demodulate([[maybe_unused]] const std::vector<cplx>& x,
                                          [[maybe_unused]] std::size_t sps,
                                          [[maybe_unused]] double dev_unit_hz,
                                          [[maybe_unused]] double fs_hz) {
  // TODO B2
  return {};
}

cplx qpsk_map([[maybe_unused]] std::uint8_t dibit) {
  // TODO B3
  return cplx(0.0, 0.0);
}

std::uint8_t qpsk_demap([[maybe_unused]] cplx s) {
  // TODO B3
  return 0;
}

std::vector<cplx> pi4dqpsk_modulate([[maybe_unused]] const std::vector<std::uint8_t>& dibits) {
  // TODO B4
  return {};
}

std::vector<std::uint8_t> pi4dqpsk_demodulate([[maybe_unused]] const std::vector<cplx>& symbols,
                                              [[maybe_unused]] cplx reference) {
  // TODO B4
  return {};
}

cplx qam16_map([[maybe_unused]] std::uint8_t nibble) {
  // TODO B5
  return cplx(0.0, 0.0);
}

std::vector<cplx> ofdm_modulate([[maybe_unused]] const std::vector<cplx>& X,
                                [[maybe_unused]] std::size_t cp_len) {
  // TODO B6 (throw std::invalid_argument if cp_len > X.size())
  return {};
}

std::vector<cplx> ofdm_demodulate([[maybe_unused]] const std::vector<cplx>& rx,
                                  [[maybe_unused]] std::size_t n_sub, [[maybe_unused]] std::size_t cp_len) {
  // TODO B6 (throw std::invalid_argument if rx.size() < cp_len + n_sub)
  return {};
}

std::vector<cplx> ofdm_equalise([[maybe_unused]] const std::vector<cplx>& Y,
                                [[maybe_unused]] const std::vector<cplx>& h_taps) {
  // TODO B6
  return {};
}

std::vector<cplx> fm_modulate([[maybe_unused]] const std::vector<double>& m, [[maybe_unused]] double kf_hz,
                              [[maybe_unused]] double fs_hz) {
  // TODO B7
  return {};
}

std::vector<double> fm_demodulate([[maybe_unused]] const std::vector<cplx>& z, [[maybe_unused]] double kf_hz,
                                  [[maybe_unused]] double fs_hz) {
  // TODO B7
  return {};
}

double carson_bandwidth_hz([[maybe_unused]] double dev_hz, [[maybe_unused]] double fm_hz) {
  // TODO B7
  return 0.0;
}

}  // namespace dsp
