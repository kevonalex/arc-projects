#include "sampling_quant.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dsp {

double alias_frequency(double f_hz, double fs_hz) {
  double r = std::fmod(std::fabs(f_hz), fs_hz);
  if (r > 0.5 * fs_hz) r = fs_hz - r;
  return r;
}

std::vector<double> sample_sine(double f_hz, double fs_hz, double phase_rad, std::size_t n) {
  std::vector<double> x(n);
  for (std::size_t i = 0; i < n; ++i) {
    x[i] = std::sin(2.0 * kPi * f_hz * static_cast<double>(i) / fs_hz + phase_rad);
  }
  return x;
}

double quantise_uniform(double x, int bits, double full_scale) {
  const double levels = std::ldexp(1.0, bits);  // 2^bits
  const double step = 2.0 * full_scale / levels;
  double k = std::floor(x / step + 0.5);
  const double k_max = 0.5 * levels - 1.0;
  const double k_min = -0.5 * levels;
  k = std::min(std::max(k, k_min), k_max);
  return k * step;
}

double ideal_sqnr_db(int bits) {
  return 10.0 * std::log10(1.5) + 20.0 * bits * std::log10(2.0);
}

double measured_sqnr_db(int bits, std::size_t n) {
  const double step = 2.0 / std::ldexp(1.0, bits);
  const double amp = 1.0 - step;
  const double f = 0.123456789;
  double ps = 0.0, pn = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double x = amp * std::sin(2.0 * kPi * f * static_cast<double>(i));
    const double e = quantise_uniform(x, bits, 1.0) - x;
    ps += x * x;
    pn += e * e;
  }
  return 10.0 * std::log10(ps / pn);
}

double bandpass_fs_min(double f_low_hz, double f_high_hz) {
  const double bw = f_high_hz - f_low_hz;
  for (int k = static_cast<int>(std::floor(f_high_hz / bw)); k >= 1; --k) {
    const double lo = 2.0 * f_high_hz / k;
    const double hi = (k > 1) ? 2.0 * f_low_hz / (k - 1) : std::numeric_limits<double>::infinity();
    if (lo <= hi) return lo;
  }
  return 2.0 * f_high_hz;  // unreachable for valid input (k = 1 always works)
}

}  // namespace dsp
