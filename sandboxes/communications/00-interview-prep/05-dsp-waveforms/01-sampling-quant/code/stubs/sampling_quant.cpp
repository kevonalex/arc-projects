// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "sampling_quant.hpp"

namespace dsp {

double alias_frequency([[maybe_unused]] double f_hz, [[maybe_unused]] double fs_hz) {
  // TODO B1
  return -1.0;
}

std::vector<double> sample_sine([[maybe_unused]] double f_hz, [[maybe_unused]] double fs_hz,
                                [[maybe_unused]] double phase_rad, [[maybe_unused]] std::size_t n) {
  // TODO B2
  return {};
}

double quantise_uniform([[maybe_unused]] double x, [[maybe_unused]] int bits,
                        [[maybe_unused]] double full_scale) {
  // TODO B3
  return 0.0;
}

double ideal_sqnr_db([[maybe_unused]] int bits) {
  // TODO B4a
  return 0.0;
}

double measured_sqnr_db([[maybe_unused]] int bits, [[maybe_unused]] std::size_t n) {
  // TODO B4b
  return 0.0;
}

double bandpass_fs_min([[maybe_unused]] double f_low_hz, [[maybe_unused]] double f_high_hz) {
  // TODO B5
  return 0.0;
}

}  // namespace dsp
