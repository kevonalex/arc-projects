#include "octave_builds.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace dsp {

// ------------------------------------------------------------------ B1
std::vector<cplx> fft(const std::vector<cplx>& x) {
  const std::size_t n = x.size();
  if (n == 0 || (n & (n - 1)) != 0) throw std::invalid_argument("fft: size must be a power of two");
  unsigned bits = 0;
  while ((std::size_t{1} << bits) < n) ++bits;

  std::vector<cplx> a(n);
  for (std::size_t i = 0; i < n; ++i) {
    std::size_t r = 0;
    for (unsigned b = 0; b < bits; ++b) {
      if (i & (std::size_t{1} << b)) r |= std::size_t{1} << (bits - 1 - b);
    }
    a[r] = x[i];
  }

  std::vector<cplx> w(n / 2 + 1);
  for (std::size_t len = 2; len <= n; len <<= 1) {
    const std::size_t half = len / 2;
    const double ang = -2.0 * kPi / static_cast<double>(len);
    for (std::size_t k = 0; k < half; ++k) w[k] = std::polar(1.0, ang * static_cast<double>(k));
    for (std::size_t i = 0; i < n; i += len) {
      for (std::size_t k = 0; k < half; ++k) {
        const cplx u = a[i + k];
        const cplx v = a[i + k + half] * w[k];
        a[i + k] = u + v;
        a[i + k + half] = u - v;
      }
    }
  }
  return a;
}

// ------------------------------------------------------------------ B2
FirFilter::FirFilter(std::vector<double> taps) : taps_(std::move(taps)) {
  if (taps_.empty()) throw std::invalid_argument("FirFilter: no taps");
  state_.assign(taps_.size(), 0.0);
}

double FirFilter::process(double x) {
  const std::size_t n = taps_.size();
  state_[pos_] = x;
  double acc = 0.0;
  for (std::size_t k = 0; k < n; ++k) acc += taps_[k] * state_[(pos_ + n - k) % n];
  pos_ = (pos_ + 1) % n;
  return acc;
}

std::vector<double> FirFilter::process_block(const std::vector<double>& x) {
  std::vector<double> y(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) y[i] = process(x[i]);
  return y;
}

void FirFilter::reset() {
  std::fill(state_.begin(), state_.end(), 0.0);
  pos_ = 0;
}

// ------------------------------------------------------------------ B3
std::vector<int> fsk4_levels(const std::vector<int>& dibits) {
  static const int kLevel[4] = {+1, +3, -1, -3};  // index = dibit: 00 -> +1, 01 -> +3, 10 -> -1, 11 -> -3
  std::vector<int> lv(dibits.size());
  for (std::size_t i = 0; i < dibits.size(); ++i) {
    if (dibits[i] < 0 || dibits[i] > 3) throw std::invalid_argument("fsk4_levels: dibit out of range");
    lv[i] = kLevel[dibits[i]];
  }
  return lv;
}

std::vector<cplx> fsk4_modulate(const std::vector<int>& dibits, std::size_t sps, double dev_unit_hz, double fs_hz) {
  const std::vector<int> lv = fsk4_levels(dibits);
  std::vector<cplx> out;
  out.reserve(lv.size() * sps);
  double phase = 0.0;
  for (int level : lv) {
    const double dphi = 2.0 * kPi * (static_cast<double>(level) * dev_unit_hz) / fs_hz;
    for (std::size_t j = 0; j < sps; ++j) {
      out.push_back(std::polar(1.0, phase));
      phase += dphi;
    }
  }
  return out;
}

// ------------------------------------------------------------------ B4
std::vector<int> sync_hamming_distances(const std::vector<int>& bits, const std::vector<int>& word) {
  if (word.empty() || bits.size() < word.size()) return {};
  const std::size_t nl = bits.size() - word.size() + 1;
  std::vector<int> d(nl, 0);
  for (std::size_t l = 0; l < nl; ++l) {
    int e = 0;
    for (std::size_t i = 0; i < word.size(); ++i) e += (bits[l + i] != word[i]) ? 1 : 0;
    d[l] = e;
  }
  return d;
}

std::vector<std::size_t> sync_detect(const std::vector<int>& distances, int max_errors) {
  std::vector<std::size_t> lags;
  for (std::size_t l = 0; l < distances.size(); ++l) {
    if (distances[l] <= max_errors) lags.push_back(l);
  }
  return lags;
}

// ------------------------------------------------------------------ B5
double estimate_cfo(const std::vector<cplx>& rx, const std::vector<double>& known) {
  if (rx.size() != known.size() || rx.size() < 2) throw std::invalid_argument("estimate_cfo: bad sizes");
  cplx acc(0.0, 0.0);
  cplx prev = rx[0] * known[0];
  for (std::size_t n = 1; n < rx.size(); ++n) {
    const cplx cur = rx[n] * known[n];
    acc += cur * std::conj(prev);
    prev = cur;
  }
  return std::arg(acc) / (2.0 * kPi);
}

std::vector<cplx> correct_cfo(const std::vector<cplx>& x, double f) {
  std::vector<cplx> y(x.size());
  for (std::size_t n = 0; n < x.size(); ++n) y[n] = x[n] * std::polar(1.0, -2.0 * kPi * f * static_cast<double>(n));
  return y;
}

// ------------------------------------------------------------------ B6
double qpsk_ber_theory(double ebn0_db) {
  const double g = std::pow(10.0, ebn0_db / 10.0);
  return 0.5 * std::erfc(std::sqrt(g));
}

double simulate_qpsk_ber(double ebn0_db, std::size_t nbits, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  const double ebn0 = std::pow(10.0, ebn0_db / 10.0);
  const double n0 = 0.5 / ebn0;  // Eb = 0.5
  std::normal_distribution<double> g(0.0, std::sqrt(n0 / 2.0));
  const double a = 1.0 / std::sqrt(2.0);
  const std::size_t nsym = nbits / 2;
  std::size_t errors = 0;
  for (std::size_t s = 0; s < nsym; ++s) {
    const std::uint64_t r = rng();
    const bool b1 = (r & 1u) != 0u;
    const bool b0 = (r & 2u) != 0u;
    const double i = (b1 ? -a : a) + g(rng);
    const double q = (b0 ? -a : a) + g(rng);
    if ((i < 0.0) != b1) ++errors;
    if ((q < 0.0) != b0) ++errors;
  }
  return nsym == 0 ? 0.0 : static_cast<double>(errors) / static_cast<double>(2 * nsym);
}

// ------------------------------------------------------------------ B7
std::vector<std::int16_t> q15_fir(const std::vector<std::int16_t>& x, const std::vector<std::int16_t>& h) {
  std::vector<std::int16_t> y(x.size());
  for (std::size_t n = 0; n < x.size(); ++n) {
    std::int64_t acc = 0;
    const std::size_t kmax = std::min(h.size(), n + 1);
    for (std::size_t k = 0; k < kmax; ++k) acc += static_cast<std::int64_t>(h[k]) * static_cast<std::int64_t>(x[n - k]);
    std::int64_t v = (acc + (std::int64_t{1} << 14)) >> 15;
    v = std::max<std::int64_t>(v, std::numeric_limits<std::int16_t>::min());
    v = std::min<std::int64_t>(v, std::numeric_limits<std::int16_t>::max());
    y[n] = static_cast<std::int16_t>(v);
  }
  return y;
}

}  // namespace dsp
