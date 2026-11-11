// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "filters.hpp"

namespace dsp {

std::vector<double> convolve([[maybe_unused]] const std::vector<double>& x,
                             [[maybe_unused]] const std::vector<double>& h) {
  // TODO B1
  return {};
}

std::vector<double> hamming(std::size_t n) {
  // TODO B2
  return std::vector<double>(n, 0.0);
}

std::vector<double> windowed_sinc_lowpass(std::size_t ntaps,
                                          [[maybe_unused]] double fc_cycles_per_sample) {
  // TODO B3
  return std::vector<double>(ntaps, 0.0);
}

BiquadCoeffs biquad_lowpass([[maybe_unused]] double f0_cycles_per_sample, [[maybe_unused]] double q) {
  // TODO B4a
  return BiquadCoeffs{};
}

double Biquad::process([[maybe_unused]] double x) {
  // TODO B4b
  return 0.0;
}

void Biquad::reset() {
  // TODO B4b
}

std::vector<double> decimate_polyphase([[maybe_unused]] const std::vector<double>& x,
                                       [[maybe_unused]] std::size_t m_factor,
                                       [[maybe_unused]] const std::vector<double>& h) {
  // TODO B5
  return {};
}

std::vector<double> interpolate_polyphase([[maybe_unused]] const std::vector<double>& x,
                                          [[maybe_unused]] std::size_t l_factor,
                                          [[maybe_unused]] const std::vector<double>& h) {
  // TODO B6
  return {};
}

}  // namespace dsp
