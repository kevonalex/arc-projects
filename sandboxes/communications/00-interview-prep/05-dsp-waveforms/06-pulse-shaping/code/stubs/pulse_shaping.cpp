// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "pulse_shaping.hpp"

namespace dsp {

std::vector<double> raised_cosine([[maybe_unused]] double beta, std::size_t sps, std::size_t span) {
  // TODO B1
  return std::vector<double>(2 * span * sps + 1, 0.0);
}

std::vector<double> root_raised_cosine([[maybe_unused]] double beta, std::size_t sps, std::size_t span) {
  // TODO B2
  return std::vector<double>(2 * span * sps + 1, 0.0);
}

std::vector<double> pulse_shape([[maybe_unused]] const std::vector<double>& symbols,
                                [[maybe_unused]] const std::vector<double>& taps,
                                [[maybe_unused]] std::size_t sps) {
  // TODO B3
  return {};
}

std::vector<std::vector<double>> eye_traces([[maybe_unused]] const std::vector<double>& y,
                                            [[maybe_unused]] std::size_t sps,
                                            [[maybe_unused]] std::size_t offset) {
  // TODO B4
  return {};
}

double eye_height([[maybe_unused]] const std::vector<double>& y, [[maybe_unused]] std::size_t sps,
                  [[maybe_unused]] std::size_t offset, [[maybe_unused]] std::size_t guard) {
  // TODO B5
  return -99.0;
}

double isi_residual([[maybe_unused]] const std::vector<double>& pulse, [[maybe_unused]] std::size_t sps) {
  // TODO B6
  return 99.0;
}

}  // namespace dsp
