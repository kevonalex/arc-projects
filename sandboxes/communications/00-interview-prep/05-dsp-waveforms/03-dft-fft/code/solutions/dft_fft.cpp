#include "dft_fft.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace dsp {

std::vector<cplx> dft(const std::vector<cplx>& x) {
  const std::size_t n = x.size();
  std::vector<cplx> out(n);
  for (std::size_t k = 0; k < n; ++k) {
    cplx acc(0.0, 0.0);
    for (std::size_t m = 0; m < n; ++m) {
      const std::size_t km = (k * m) % n;  // reduce first: better accuracy for large N
      acc += x[m] * std::polar(1.0, -2.0 * kPi * static_cast<double>(km) / static_cast<double>(n));
    }
    out[k] = acc;
  }
  return out;
}

std::size_t bit_reverse(std::size_t i, unsigned bits) {
  std::size_t r = 0;
  for (unsigned b = 0; b < bits; ++b) {
    r = (r << 1) | (i & 1u);
    i >>= 1;
  }
  return r;
}

namespace {
std::vector<cplx> fft_core(std::vector<cplx> a, bool inverse) {
  const std::size_t n = a.size();
  if (n == 0) return a;
  if ((n & (n - 1)) != 0) throw std::invalid_argument("FFT size must be a power of two");
  unsigned bits = 0;
  while ((static_cast<std::size_t>(1) << bits) < n) ++bits;
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t j = bit_reverse(i, bits);
    if (j > i) std::swap(a[i], a[j]);
  }
  const double sign = inverse ? +1.0 : -1.0;
  for (std::size_t len = 2; len <= n; len <<= 1) {
    const std::size_t half = len / 2;
    for (std::size_t start = 0; start < n; start += len) {
      for (std::size_t k = 0; k < half; ++k) {
        const cplx w = std::polar(1.0, sign * 2.0 * kPi * static_cast<double>(k) / static_cast<double>(len));
        const cplx u = a[start + k];
        const cplx v = a[start + k + half] * w;
        a[start + k] = u + v;
        a[start + k + half] = u - v;
      }
    }
  }
  if (inverse) {
    for (auto& v : a) v /= static_cast<double>(n);
  }
  return a;
}
}  // namespace

std::vector<cplx> fft_radix2(const std::vector<cplx>& x) { return fft_core(x, false); }
std::vector<cplx> ifft_radix2(const std::vector<cplx>& X) { return fft_core(X, true); }

std::vector<double> hann_periodic(std::size_t n) {
  std::vector<double> w(n);
  for (std::size_t i = 0; i < n; ++i) {
    w[i] = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(n));
  }
  return w;
}

double peak_bin_estimate(const std::vector<cplx>& X) {
  const std::size_t n = X.size();
  if (n == 0) return 0.0;
  std::size_t k = 0;
  double best = -1.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double m = std::abs(X[i]);
    if (m > best) {
      best = m;
      k = i;
    }
  }
  const double a = std::log(std::abs(X[(k + n - 1) % n]) + 1e-300);
  const double b = std::log(std::abs(X[k]) + 1e-300);
  const double c = std::log(std::abs(X[(k + 1) % n]) + 1e-300);
  const double denom = a - 2.0 * b + c;
  double delta = 0.0;
  if (denom != 0.0) delta = 0.5 * (a - c) / denom;
  double pos = static_cast<double>(k) + delta;
  const double nn = static_cast<double>(n);
  if (pos < 0.0) pos += nn;
  if (pos >= nn) pos -= nn;
  return pos;
}

}  // namespace dsp
