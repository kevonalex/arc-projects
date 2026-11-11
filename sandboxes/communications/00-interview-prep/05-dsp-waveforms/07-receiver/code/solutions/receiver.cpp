#include "receiver.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace dsp {

double matched_filter_output_snr(const std::vector<double>& h, const std::vector<double>& s, double noise_var) {
  if (h.size() != s.size()) throw std::invalid_argument("h and s must have equal length");
  const std::size_t n = s.size();
  double ys = 0.0, eh = 0.0;
  for (std::size_t k = 0; k < n; ++k) {
    ys += h[k] * s[n - 1 - k];
    eh += h[k] * h[k];
  }
  if (eh == 0.0) return 0.0;
  return ys * ys / (noise_var * eh);
}

SyncResult sync_search(const std::vector<cplx>& rx, const std::vector<cplx>& ref) {
  SyncResult best;
  if (ref.empty() || rx.size() < ref.size()) return best;
  double e_ref = 0.0;
  for (const auto& v : ref) e_ref += std::norm(v);
  const std::size_t nlags = rx.size() - ref.size() + 1;
  bool first = true;
  for (std::size_t l = 0; l < nlags; ++l) {
    cplx c(0.0, 0.0);
    double e_rx = 0.0;
    for (std::size_t k = 0; k < ref.size(); ++k) {
      c += rx[l + k] * std::conj(ref[k]);
      e_rx += std::norm(rx[l + k]);
    }
    const double rho = (e_rx > 0.0 && e_ref > 0.0) ? std::norm(c) / (e_rx * e_ref) : 0.0;
    if (first || rho > best.metric) {
      best.lag = l;
      best.metric = rho;
      first = false;
    }
  }
  return best;
}

double sync_cfo_loss(std::size_t len, double f) {
  if (len == 0) return 0.0;
  const double s = std::sin(kPi * f);
  if (std::fabs(s) < 1e-12) return 1.0;
  return std::fabs(std::sin(kPi * static_cast<double>(len) * f) / (static_cast<double>(len) * s));
}

double sync_false_alarm_prob(unsigned bits, unsigned max_errors) {
  double c = 1.0, sum = 0.0;
  for (unsigned k = 0; k <= std::min(bits, max_errors); ++k) {
    if (k > 0) c *= static_cast<double>(bits - k + 1) / static_cast<double>(k);
    sum += c;
  }
  return sum / std::ldexp(1.0, static_cast<int>(bits));
}

double gardner_error(double prev_strobe, double mid, double cur_strobe) {
  return (cur_strobe - prev_strobe) * mid;
}

namespace {
double interp(const std::vector<double>& x, double t) {
  const std::size_t i = static_cast<std::size_t>(std::floor(t));
  const double f = t - static_cast<double>(i);
  return x[i] * (1.0 - f) + x[i + 1] * f;
}
}  // namespace

std::vector<double> gardner_recover(const std::vector<double>& x, std::size_t sps, double kp, double ki,
                                    double t_start) {
  std::vector<double> out;
  if (t_start < 0.0 || static_cast<std::size_t>(std::floor(t_start)) + 1 >= x.size()) return out;
  const double half = 0.5 * static_cast<double>(sps);
  double t = t_start;
  double y_prev = interp(x, t);
  out.push_back(y_prev);
  double corr = 0.0, integ = 0.0;
  for (;;) {
    t = t + static_cast<double>(sps) - corr;
    if (t - half < 0.0 || static_cast<std::size_t>(std::floor(t)) + 1 >= x.size()) break;
    const double y = interp(x, t);
    const double mid = interp(x, t - half);
    const double e = gardner_error(y_prev, mid, y);
    integ += ki * e;
    corr = kp * e + integ;
    out.push_back(y);
    y_prev = y;
  }
  return out;
}

LoopGains loop_gains(double bn_t, double zeta) {
  const double theta = bn_t / (zeta + 1.0 / (4.0 * zeta));
  const double d = 1.0 + 2.0 * zeta * theta + theta * theta;
  return LoopGains{4.0 * zeta * theta / d, 4.0 * theta * theta / d};
}

std::vector<cplx> costas_bpsk(const std::vector<cplx>& x, double alpha, double beta) {
  std::vector<cplx> out;
  out.reserve(x.size());
  double phase = 0.0, freq = 0.0;
  for (const cplx& v : x) {
    const cplx y = v * std::polar(1.0, -phase);
    const double e = y.real() * y.imag();
    freq += beta * e;
    phase += freq + alpha * e;
    if (phase > kPi) phase -= 2.0 * kPi;
    else if (phase <= -kPi) phase += 2.0 * kPi;
    out.push_back(y);
  }
  return out;
}

cplx Agc::process(cplx x) {
  const cplx y = gain_ * x;
  gain_ *= std::exp(mu_ * (ref_ - std::abs(y)));
  gain_ = std::min(std::max(gain_, 1e-6), 1e6);
  return y;
}

}  // namespace dsp
