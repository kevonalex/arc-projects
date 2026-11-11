#include "iq_baseband.hpp"

#include <algorithm>
#include <cmath>

namespace dsp {

std::vector<cplx> mix(const std::vector<cplx>& x, double f_cycles_per_sample, double phase0_rad) {
  std::vector<cplx> y(x.size());
  for (std::size_t n = 0; n < x.size(); ++n) {
    const double ph = 2.0 * kPi * f_cycles_per_sample * static_cast<double>(n) + phase0_rad;
    y[n] = x[n] * std::polar(1.0, ph);
  }
  return y;
}

std::vector<cplx> iq_downconvert(const std::vector<double>& x, double fc_cycles_per_sample,
                                 const std::vector<double>& lpf_taps) {
  const std::size_t n = x.size();
  std::vector<double> raw_i(n), raw_q(n);
  for (std::size_t k = 0; k < n; ++k) {
    const double ph = 2.0 * kPi * fc_cycles_per_sample * static_cast<double>(k);
    raw_i[k] = 2.0 * x[k] * std::cos(ph);
    raw_q[k] = -2.0 * x[k] * std::sin(ph);
  }
  std::vector<cplx> z(n);
  for (std::size_t k = 0; k < n; ++k) {
    double i_acc = 0.0, q_acc = 0.0;
    const std::size_t m_max = std::min(k + 1, lpf_taps.size());
    for (std::size_t m = 0; m < m_max; ++m) {
      i_acc += lpf_taps[m] * raw_i[k - m];
      q_acc += lpf_taps[m] * raw_q[k - m];
    }
    z[k] = cplx(i_acc, q_acc);
  }
  return z;
}

double estimate_freq_cps(const std::vector<cplx>& z) {
  if (z.size() < 2) return 0.0;
  cplx acc(0.0, 0.0);
  for (std::size_t n = 0; n + 1 < z.size(); ++n) acc += z[n + 1] * std::conj(z[n]);
  return std::arg(acc) / (2.0 * kPi);
}

std::vector<double> instantaneous_freq(const std::vector<cplx>& z) {
  std::vector<double> f;
  if (z.size() < 2) return f;
  f.reserve(z.size() - 1);
  for (std::size_t n = 0; n + 1 < z.size(); ++n) {
    f.push_back(std::arg(z[n + 1] * std::conj(z[n])) / (2.0 * kPi));
  }
  return f;
}

double image_rejection_db(const std::vector<cplx>& z, double f_cycles_per_sample) {
  cplx wanted(0.0, 0.0), image(0.0, 0.0);
  for (std::size_t n = 0; n < z.size(); ++n) {
    const double ph = 2.0 * kPi * f_cycles_per_sample * static_cast<double>(n);
    wanted += z[n] * std::polar(1.0, -ph);
    image += z[n] * std::polar(1.0, +ph);
  }
  const double w = std::abs(wanted);
  const double i = std::max(std::abs(image), 1e-300);
  return 20.0 * std::log10(w / i);
}

}  // namespace dsp
