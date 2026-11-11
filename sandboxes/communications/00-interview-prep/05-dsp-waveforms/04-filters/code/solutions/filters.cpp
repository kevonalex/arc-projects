#include "filters.hpp"

#include <cmath>

namespace dsp {

std::vector<double> convolve(const std::vector<double>& x, const std::vector<double>& h) {
  if (x.empty() || h.empty()) return {};
  std::vector<double> y(x.size() + h.size() - 1, 0.0);
  for (std::size_t i = 0; i < x.size(); ++i) {
    for (std::size_t j = 0; j < h.size(); ++j) y[i + j] += x[i] * h[j];
  }
  return y;
}

std::vector<double> hamming(std::size_t n) {
  if (n == 0) return {};
  if (n == 1) return {1.0};
  std::vector<double> w(n);
  for (std::size_t i = 0; i < n; ++i) {
    w[i] = 0.54 - 0.46 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(n - 1));
  }
  return w;
}

std::vector<double> windowed_sinc_lowpass(std::size_t ntaps, double fc) {
  if (ntaps == 0) return {};
  const std::vector<double> w = hamming(ntaps);
  std::vector<double> h(ntaps);
  const double centre = 0.5 * static_cast<double>(ntaps - 1);
  double sum = 0.0;
  for (std::size_t i = 0; i < ntaps; ++i) {
    const double m = static_cast<double>(i) - centre;
    const double arg = 2.0 * fc * m;
    const double sinc = (std::fabs(arg) < 1e-12) ? 1.0 : std::sin(kPi * arg) / (kPi * arg);
    h[i] = 2.0 * fc * sinc * w[i];
    sum += h[i];
  }
  for (auto& v : h) v /= sum;
  return h;
}

BiquadCoeffs biquad_lowpass(double f0, double q) {
  const double w0 = 2.0 * kPi * f0;
  const double cw = std::cos(w0);
  const double alpha = std::sin(w0) / (2.0 * q);
  const double a0 = 1.0 + alpha;
  BiquadCoeffs c;
  c.b0 = (1.0 - cw) / 2.0 / a0;
  c.b1 = (1.0 - cw) / a0;
  c.b2 = (1.0 - cw) / 2.0 / a0;
  c.a1 = -2.0 * cw / a0;
  c.a2 = (1.0 - alpha) / a0;
  return c;
}

double Biquad::process(double x) {
  const double y = c_.b0 * x + s1_;
  s1_ = c_.b1 * x - c_.a1 * y + s2_;
  s2_ = c_.b2 * x - c_.a2 * y;
  return y;
}

void Biquad::reset() {
  s1_ = 0.0;
  s2_ = 0.0;
}

std::vector<double> decimate_polyphase(const std::vector<double>& x, std::size_t m_factor,
                                       const std::vector<double>& h) {
  if (m_factor == 0 || x.empty()) return {};
  const std::size_t ny = (x.size() + m_factor - 1) / m_factor;
  std::vector<double> y(ny, 0.0);
  for (std::size_t m = 0; m < ny; ++m) {
    double acc = 0.0;
    for (std::size_t p = 0; p < m_factor; ++p) {      // polyphase branch p: taps h[p], h[p+M], ...
      for (std::size_t q = 0; q * m_factor + p < h.size(); ++q) {
        const std::size_t k = q * m_factor + p;
        const std::size_t pos = m * m_factor;
        if (k > pos) break;                           // x[negative index] = 0
        acc += h[k] * x[pos - k];
      }
    }
    y[m] = acc;
  }
  return y;
}

std::vector<double> interpolate_polyphase(const std::vector<double>& x, std::size_t l_factor,
                                          const std::vector<double>& h) {
  if (l_factor == 0 || x.empty()) return {};
  std::vector<double> y(x.size() * l_factor, 0.0);
  for (std::size_t n = 0; n < x.size(); ++n) {
    for (std::size_t p = 0; p < l_factor; ++p) {
      double acc = 0.0;
      for (std::size_t q = 0; q * l_factor + p < h.size() && q <= n; ++q) {
        acc += h[q * l_factor + p] * x[n - q];
      }
      y[n * l_factor + p] = acc;
    }
  }
  return y;
}

}  // namespace dsp
