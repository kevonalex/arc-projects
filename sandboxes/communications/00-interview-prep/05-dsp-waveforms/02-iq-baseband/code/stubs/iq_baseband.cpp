// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "iq_baseband.hpp"

namespace dsp {

std::vector<cplx> mix(const std::vector<cplx>& x, [[maybe_unused]] double f_cycles_per_sample,
                      [[maybe_unused]] double phase0_rad) {
  // TODO B1
  return std::vector<cplx>(x.size());
}

std::vector<cplx> iq_downconvert(const std::vector<double>& x,
                                 [[maybe_unused]] double fc_cycles_per_sample,
                                 [[maybe_unused]] const std::vector<double>& lpf_taps) {
  // TODO B2
  return std::vector<cplx>(x.size());
}

double estimate_freq_cps([[maybe_unused]] const std::vector<cplx>& z) {
  // TODO B3
  return 99.0;
}

std::vector<double> instantaneous_freq([[maybe_unused]] const std::vector<cplx>& z) {
  // TODO B4
  return {};
}

double image_rejection_db([[maybe_unused]] const std::vector<cplx>& z,
                          [[maybe_unused]] double f_cycles_per_sample) {
  // TODO B5
  return -99.0;
}

}  // namespace dsp
