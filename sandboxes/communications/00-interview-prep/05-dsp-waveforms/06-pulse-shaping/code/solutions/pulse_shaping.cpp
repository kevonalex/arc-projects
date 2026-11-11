#include "pulse_shaping.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dsp {

namespace {
double sinc(double x) {
  if (std::fabs(x) < 1e-12) return 1.0;
  return std::sin(kPi * x) / (kPi * x);
}
}  // namespace

std::vector<double> raised_cosine(double beta, std::size_t sps, std::size_t span) {
  const std::size_t n = 2 * span * sps + 1;
  std::vector<double> h(n);
  const double centre = static_cast<double>(span * sps);
  for (std::size_t i = 0; i < n; ++i) {
    const double t = (static_cast<double>(i) - centre) / static_cast<double>(sps);
    if (std::fabs(t) < 1e-12) {
      h[i] = 1.0;
    } else if (beta > 0.0 && std::fabs(std::fabs(t) - 1.0 / (2.0 * beta)) < 1e-9) {
      h[i] = (kPi / 4.0) * sinc(1.0 / (2.0 * beta));
    } else {
      h[i] = sinc(t) * std::cos(kPi * beta * t) / (1.0 - (2.0 * beta * t) * (2.0 * beta * t));
    }
  }
  return h;
}

std::vector<double> root_raised_cosine(double beta, std::size_t sps, std::size_t span) {
  const std::size_t n = 2 * span * sps + 1;
  std::vector<double> h(n);
  const double centre = static_cast<double>(span * sps);
  double energy = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double t = (static_cast<double>(i) - centre) / static_cast<double>(sps);
    double v;
    if (std::fabs(t) < 1e-12) {
      v = 1.0 - beta + 4.0 * beta / kPi;
    } else if (beta > 0.0 && std::fabs(std::fabs(t) - 1.0 / (4.0 * beta)) < 1e-9) {
      v = (beta / std::sqrt(2.0)) * ((1.0 + 2.0 / kPi) * std::sin(kPi / (4.0 * beta)) +
                                     (1.0 - 2.0 / kPi) * std::cos(kPi / (4.0 * beta)));
    } else {
      const double num = std::sin(kPi * t * (1.0 - beta)) + 4.0 * beta * t * std::cos(kPi * t * (1.0 + beta));
      const double den = kPi * t * (1.0 - (4.0 * beta * t) * (4.0 * beta * t));
      v = num / den;
    }
    h[i] = v;
    energy += v * v;
  }
  const double scale = 1.0 / std::sqrt(energy);
  for (auto& v : h) v *= scale;
  return h;
}

std::vector<double> pulse_shape(const std::vector<double>& symbols, const std::vector<double>& taps,
                                std::size_t sps) {
  if (symbols.empty() || taps.empty() || sps == 0) return {};
  std::vector<double> y(symbols.size() * sps + taps.size() - 1, 0.0);
  for (std::size_t k = 0; k < symbols.size(); ++k) {
    const std::size_t base = k * sps;
    for (std::size_t j = 0; j < taps.size(); ++j) y[base + j] += symbols[k] * taps[j];
  }
  return y;
}

std::vector<std::vector<double>> eye_traces(const std::vector<double>& y, std::size_t sps, std::size_t offset) {
  std::vector<std::vector<double>> out;
  const std::size_t len = 2 * sps + 1;
  for (std::size_t start = offset; start + len <= y.size(); start += sps) {
    out.emplace_back(y.begin() + static_cast<std::ptrdiff_t>(start),
                     y.begin() + static_cast<std::ptrdiff_t>(start + len));
  }
  return out;
}

double eye_height(const std::vector<double>& y, std::size_t sps, std::size_t offset, std::size_t guard) {
  if (y.size() <= offset || sps == 0) return 0.0;
  const std::size_t kmax = (y.size() - 1 - offset) / sps;
  if (kmax < 2 * guard) return 0.0;
  double min_pos = std::numeric_limits<double>::infinity();
  double max_neg = -std::numeric_limits<double>::infinity();
  for (std::size_t k = guard; k + guard <= kmax; ++k) {
    const double v = y[offset + k * sps];
    if (v > 0.0) min_pos = std::min(min_pos, v);
    else if (v < 0.0) max_neg = std::max(max_neg, v);
  }
  if (!std::isfinite(min_pos) || !std::isfinite(max_neg)) return 0.0;
  return min_pos - max_neg;
}

double isi_residual(const std::vector<double>& pulse, std::size_t sps) {
  if (pulse.empty() || sps == 0) return 0.0;
  const std::size_t c = (pulse.size() - 1) / 2;
  const double peak = std::fabs(pulse[c]);
  double worst = 0.0;
  for (std::size_t m = 1; c + m * sps < pulse.size() && m * sps <= c; ++m) {
    worst = std::max(worst, std::fabs(pulse[c + m * sps]));
    worst = std::max(worst, std::fabs(pulse[c - m * sps]));
  }
  return worst / peak;
}

}  // namespace dsp
