#include "modulation.hpp"

#include <cmath>
#include <stdexcept>

namespace dsp {

int fsk4_level(std::uint8_t dibit) {
  switch (dibit & 0x3u) {
    case 0: return +1;
    case 1: return +3;
    case 2: return -1;
    default: return -3;
  }
}

std::vector<cplx> fsk4_modulate(const std::vector<std::uint8_t>& dibits, std::size_t sps, double dev_unit_hz,
                                double fs_hz) {
  std::vector<cplx> out;
  out.reserve(dibits.size() * sps);
  double phase = 0.0;
  for (std::uint8_t d : dibits) {
    const double step = 2.0 * kPi * fsk4_level(d) * dev_unit_hz / fs_hz;
    for (std::size_t i = 0; i < sps; ++i) {
      out.push_back(std::polar(1.0, phase));
      phase += step;
      if (phase > kPi) phase -= 2.0 * kPi;
      else if (phase < -kPi) phase += 2.0 * kPi;
    }
  }
  return out;
}

std::vector<std::uint8_t> fsk4_demodulate(const std::vector<cplx>& x, std::size_t sps, double dev_unit_hz,
                                          double fs_hz) {
  std::vector<std::uint8_t> out;
  if (sps == 0) return out;
  const std::size_t nsym = x.size() / sps;
  out.reserve(nsym);
  for (std::size_t s = 0; s < nsym; ++s) {
    double acc = 0.0;
    std::size_t cnt = 0;
    for (std::size_t i = 0; i < sps; ++i) {
      const std::size_t n = s * sps + i;
      if (n + 1 >= x.size()) break;
      acc += std::arg(x[n + 1] * std::conj(x[n]));
      ++cnt;
    }
    const double f_hz = (cnt > 0) ? (acc / static_cast<double>(cnt)) * fs_hz / (2.0 * kPi) : 0.0;
    const double lvl = f_hz / dev_unit_hz;
    std::uint8_t d;
    if (lvl > 2.0) d = 1;        // +3
    else if (lvl > 0.0) d = 0;   // +1
    else if (lvl > -2.0) d = 2;  // -1
    else d = 3;                  // -3
    out.push_back(d);
  }
  return out;
}

cplx qpsk_map(std::uint8_t dibit) {
  const double r = 1.0 / std::sqrt(2.0);
  const double i = ((dibit >> 1) & 1u) ? -r : r;
  const double q = (dibit & 1u) ? -r : r;
  return cplx(i, q);
}

std::uint8_t qpsk_demap(cplx s) {
  const unsigned b1 = (s.real() < 0.0) ? 1u : 0u;
  const unsigned b0 = (s.imag() < 0.0) ? 1u : 0u;
  return static_cast<std::uint8_t>((b1 << 1) | b0);
}

namespace {
// increments in units of pi/4, indexed by dibit
int pi4_inc(std::uint8_t d) {
  switch (d & 0x3u) {
    case 0: return +1;   // 00
    case 1: return +3;   // 01
    case 2: return -1;   // 10
    default: return -3;  // 11
  }
}
}  // namespace

std::vector<cplx> pi4dqpsk_modulate(const std::vector<std::uint8_t>& dibits) {
  std::vector<cplx> out;
  out.reserve(dibits.size());
  int k = 0;  // phase in units of pi/4, kept in [0, 8)
  for (std::uint8_t d : dibits) {
    k = ((k + pi4_inc(d)) % 8 + 8) % 8;
    out.push_back(std::polar(1.0, k * kPi / 4.0));
  }
  return out;
}

std::vector<std::uint8_t> pi4dqpsk_demodulate(const std::vector<cplx>& symbols, cplx reference) {
  std::vector<std::uint8_t> out;
  out.reserve(symbols.size());
  cplx prev = reference;
  for (const cplx& s : symbols) {
    const double dphi = std::arg(s * std::conj(prev));
    std::uint8_t best = 0;
    double best_err = 1e9;
    for (std::uint8_t d = 0; d < 4; ++d) {
      const double target = pi4_inc(d) * kPi / 4.0;
      const double err = std::fabs(std::arg(std::polar(1.0, dphi - target)));
      if (err < best_err) {
        best_err = err;
        best = d;
      }
    }
    out.push_back(best);
    prev = s;
  }
  return out;
}

cplx qam16_map(std::uint8_t nibble) {
  auto level = [](unsigned two_bits) {
    switch (two_bits & 3u) {
      case 0: return -3.0;  // 00
      case 1: return -1.0;  // 01
      case 3: return +1.0;  // 11
      default: return +3.0; // 10
    }
  };
  const double scale = 1.0 / std::sqrt(10.0);
  return cplx(level(static_cast<unsigned>(nibble >> 2)) * scale, level(static_cast<unsigned>(nibble)) * scale);
}

std::vector<cplx> ofdm_modulate(const std::vector<cplx>& X, std::size_t cp_len) {
  const std::size_t n = X.size();
  if (cp_len > n) throw std::invalid_argument("cyclic prefix longer than symbol");
  const std::vector<cplx> x = dft_naive(X, true);
  std::vector<cplx> out;
  out.reserve(n + cp_len);
  for (std::size_t i = 0; i < cp_len; ++i) out.push_back(x[n - cp_len + i]);
  out.insert(out.end(), x.begin(), x.end());
  return out;
}

std::vector<cplx> ofdm_demodulate(const std::vector<cplx>& rx, std::size_t n_sub, std::size_t cp_len) {
  if (rx.size() < cp_len + n_sub) throw std::invalid_argument("received block too short");
  const std::vector<cplx> body(rx.begin() + static_cast<std::ptrdiff_t>(cp_len),
                               rx.begin() + static_cast<std::ptrdiff_t>(cp_len + n_sub));
  return dft_naive(body, false);
}

std::vector<cplx> ofdm_equalise(const std::vector<cplx>& Y, const std::vector<cplx>& h_taps) {
  const std::size_t n = Y.size();
  if (h_taps.size() > n) throw std::invalid_argument("channel longer than FFT size");
  std::vector<cplx> hp(n, cplx(0.0, 0.0));
  for (std::size_t i = 0; i < h_taps.size(); ++i) hp[i] = h_taps[i];
  const std::vector<cplx> H = dft_naive(hp, false);
  std::vector<cplx> out(n);
  for (std::size_t k = 0; k < n; ++k) out[k] = Y[k] / H[k];
  return out;
}

std::vector<cplx> fm_modulate(const std::vector<double>& m, double kf_hz, double fs_hz) {
  std::vector<cplx> z;
  z.reserve(m.size());
  double phi = 0.0;
  for (double v : m) {
    z.push_back(std::polar(1.0, phi));
    phi += 2.0 * kPi * kf_hz * v / fs_hz;
    if (phi > kPi) phi -= 2.0 * kPi;
    else if (phi < -kPi) phi += 2.0 * kPi;
  }
  return z;
}

std::vector<double> fm_demodulate(const std::vector<cplx>& z, double kf_hz, double fs_hz) {
  std::vector<double> m;
  if (z.size() < 2) return m;
  m.reserve(z.size() - 1);
  for (std::size_t n = 0; n + 1 < z.size(); ++n) {
    m.push_back(std::arg(z[n + 1] * std::conj(z[n])) * fs_hz / (2.0 * kPi * kf_hz));
  }
  return m;
}

double carson_bandwidth_hz(double dev_hz, double fm_hz) { return 2.0 * (dev_hz + fm_hz); }

}  // namespace dsp
