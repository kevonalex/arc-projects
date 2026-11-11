#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "receiver.hpp"

using namespace dsp;

namespace {
unsigned next_u(unsigned& s) {
  s = s * 1664525u + 1013904223u;
  return s >> 8;
}
double uni(unsigned& s) { return static_cast<double>(next_u(s)) / 8388608.0 - 1.0; }  // [-1,1)
std::vector<double> bpsk(std::size_t n, unsigned seed) {
  std::vector<double> v(n);
  unsigned s = seed;
  for (auto& x : v) x = (next_u(s) & 0x100u) ? 1.0 : -1.0;
  return v;
}
double sinc(double x) { return std::fabs(x) < 1e-12 ? 1.0 : std::sin(kPi * x) / (kPi * x); }
double rc(double t, double beta) {
  if (std::fabs(t) < 1e-12) return 1.0;
  const double d = 1.0 - 4.0 * beta * beta * t * t;
  if (std::fabs(d) < 1e-9) return (kPi / 4.0) * sinc(1.0 / (2.0 * beta));
  return sinc(t) * std::cos(kPi * beta * t) / d;
}
// continuous-time RC-shaped BPSK evaluated at t (in samples); symbol k is centred at k*sps + delay.
double eval_signal(const std::vector<double>& a, std::size_t sps, double delay, double t) {
  const double pos = (t - delay) / static_cast<double>(sps);
  const long kc = static_cast<long>(std::floor(pos + 0.5));
  double y = 0.0;
  for (long k = kc - 10; k <= kc + 10; ++k) {
    if (k < 0 || k >= static_cast<long>(a.size())) continue;
    y += a[static_cast<std::size_t>(k)] * rc(pos - static_cast<double>(k), 0.35);
  }
  return y;
}
}  // namespace

// ---------------- matched filter ----------------
TEST(MatchedFilter, WorkedExample) {
  const std::vector<double> s = {1, 2, 3};
  EXPECT_NEAR(matched_filter_output_snr({3, 2, 1}, s, 1.0), 14.0, 1e-12);  // = Es/sigma^2
  EXPECT_NEAR(matched_filter_output_snr({1, 1, 1}, s, 1.0), 12.0, 1e-12);
  EXPECT_NEAR(matched_filter_output_snr({1, 0, 0}, s, 1.0), 9.0, 1e-12);
  EXPECT_NEAR(matched_filter_output_snr({3, 2, 1}, s, 0.5), 28.0, 1e-12);
  EXPECT_THROW(matched_filter_output_snr({1, 1}, s, 1.0), std::invalid_argument);
}

TEST(MatchedFilter, NoFilterBeatsMatchedFilter) {
  const std::vector<double> s = {2, -1, 1, 0.5, -3};
  double es = 0.0;
  for (double v : s) es += v * v;
  const double bound = es / 0.7;
  unsigned seed = 5;
  for (int trial = 0; trial < 200; ++trial) {
    std::vector<double> h(s.size());
    for (auto& v : h) v = uni(seed);
    EXPECT_LE(matched_filter_output_snr(h, s, 0.7), bound + 1e-9);
  }
  std::vector<double> rev(s.rbegin(), s.rend());
  EXPECT_NEAR(matched_filter_output_snr(rev, s, 0.7), bound, 1e-9);
}

// ---------------- sync ----------------
TEST(Sync, FindsEmbeddedWordWithRotationAndNoise) {
  const std::size_t L = 24;
  std::vector<cplx> ref(L);
  unsigned s = 77;
  for (auto& v : ref) v = cplx((next_u(s) & 0x80u) ? 1.0 : -1.0, 0.0);
  std::vector<cplx> rx(200);
  for (auto& v : rx) v = cplx(0.3 * uni(s), 0.3 * uni(s));
  const cplx rot = std::polar(0.8, 0.5);
  for (std::size_t k = 0; k < L; ++k) rx[37 + k] += rot * ref[k];
  const auto r = sync_search(rx, ref);
  EXPECT_EQ(r.lag, 37u);
  EXPECT_GT(r.metric, 0.7);
  EXPECT_LE(r.metric, 1.0 + 1e-12);
}

TEST(Sync, PerfectCopyGivesMetricOne) {
  std::vector<cplx> ref = {cplx(1, 1), cplx(-1, 1), cplx(-1, -1), cplx(1, -1), cplx(1, 1)};
  std::vector<cplx> rx(30, cplx(0.01, 0.0));
  for (std::size_t k = 0; k < ref.size(); ++k) rx[10 + k] = ref[k] * std::polar(2.0, -1.0);
  const auto r = sync_search(rx, ref);
  EXPECT_EQ(r.lag, 10u);
  EXPECT_NEAR(r.metric, 1.0, 1e-12);
  EXPECT_EQ(sync_search(std::vector<cplx>(3), ref).metric, 0.0);  // rx shorter than ref
}

TEST(Sync, CfoLossAndFalseAlarm) {
  EXPECT_NEAR(sync_cfo_loss(24, 0.0625), 0.2136, 1e-3);
  EXPECT_NEAR(sync_cfo_loss(24, 0.0), 1.0, 1e-12);
  EXPECT_NEAR(sync_cfo_loss(1, 0.3), 1.0, 1e-12);
  EXPECT_NEAR(sync_false_alarm_prob(48, 0), std::ldexp(1.0, -48), 1e-25);
  EXPECT_NEAR(sync_false_alarm_prob(48, 4), 213053.0 / 281474976710656.0, 1e-18);
  EXPECT_NEAR(sync_false_alarm_prob(4, 4), 1.0, 1e-12);
}

// ---------------- Gardner ----------------
TEST(Gardner, ErrorSign) {
  EXPECT_NEAR(gardner_error(0.9, -0.3, -1.0), 0.57, 1e-12);  // late
  EXPECT_NEAR(gardner_error(0.9, 0.3, -1.0), -0.57, 1e-12);  // early
  EXPECT_NEAR(gardner_error(1.0, 0.5, 1.0), 0.0, 1e-12);     // no transition -> no information
}

TEST(Gardner, SCurveHasCorrectSignAndZero) {
  const std::size_t sps = 8;
  const auto a = bpsk(400, 11);
  auto mean_e = [&](double tau) {
    double acc = 0.0;
    int n = 0;
    for (std::size_t k = 20; k < a.size() - 20; ++k) {
      const double t = static_cast<double>(k * sps) + tau;
      const double y = eval_signal(a, sps, 0.0, t);
      const double yp = eval_signal(a, sps, 0.0, t - static_cast<double>(sps));
      const double mid = eval_signal(a, sps, 0.0, t - 0.5 * static_cast<double>(sps));
      acc += gardner_error(yp, mid, y);
      ++n;
    }
    return acc / n;
  };
  EXPECT_NEAR(mean_e(0.0), 0.0, 0.1);
  EXPECT_GT(mean_e(+2.0), 0.2);  // late
  EXPECT_LT(mean_e(-2.0), -0.2); // early
}

TEST(Gardner, LoopAcquiresFractionalOffset) {
  const std::size_t sps = 8, nsym = 1200;
  const auto a = bpsk(nsym, 21);
  const double delay = 1.6;  // 0.2 T
  std::vector<double> x(nsym * sps);
  for (std::size_t n = 0; n < x.size(); ++n) x[n] = eval_signal(a, sps, delay, static_cast<double>(n));
  const auto y = gardner_recover(x, sps, 0.15, 0.0015, 32.0);
  ASSERT_GT(y.size(), 1000u);
  double best = 0.0;
  for (std::size_t shift = 0; shift <= 8; ++shift) {
    int agree = 0, total = 0;
    for (std::size_t j = 600; j < 900; ++j) {
      if (j + shift >= a.size()) continue;
      ++total;
      if ((y[j] > 0.0) == (a[j + shift] > 0.0)) ++agree;
    }
    best = std::max(best, static_cast<double>(agree) / total);
  }
  EXPECT_GE(best, 0.97);
}

TEST(Gardner, OutOfRangeStart) {
  EXPECT_TRUE(gardner_recover(std::vector<double>(10, 0.0), 8, 0.1, 0.0, 50.0).empty());
}

// ---------------- Costas ----------------
TEST(Costas, LoopGainsFromNoiseBandwidth) {
  const auto g = loop_gains(0.02, 0.70710678);
  EXPECT_NEAR(g.alpha, 0.05193, 2e-4);
  EXPECT_NEAR(g.beta, 0.0013848, 2e-5);
}

TEST(Costas, LocksToPhaseAndFrequencyOffset) {
  const std::size_t n = 4000;
  const auto a = bpsk(n, 31);
  const double cfo = 0.001;  // cycles/sample
  std::vector<cplx> x(n);
  for (std::size_t i = 0; i < n; ++i) x[i] = a[i] * std::polar(1.0, 2.0 * kPi * cfo * static_cast<double>(i) + 1.0);
  const auto g = loop_gains(0.02, 0.70710678);
  const auto y = costas_bpsk(x, g.alpha, g.beta);
  ASSERT_EQ(y.size(), n);
  for (std::size_t i = n - 500; i < n; ++i) {
    EXPECT_LT(std::fabs(y[i].imag()), 0.05) << "i=" << i;
    EXPECT_NEAR(std::fabs(y[i].real()), 1.0, 0.05) << "i=" << i;
  }
}

// ---------------- AGC ----------------
TEST(Agc, ConvergesFromWeakAndStrongInputs) {
  for (double amp : {0.01, 100.0}) {
    Agc agc(1.0, 0.01);
    cplx y;
    for (int i = 0; i < 5000; ++i) y = agc.process(std::polar(amp, 0.3 * i));
    EXPECT_NEAR(std::abs(y), 1.0, 0.03) << "amp=" << amp;
    EXPECT_NEAR(agc.gain() * amp, 1.0, 0.03) << "amp=" << amp;
  }
}

TEST(Agc, TracksAStepInLevel) {
  Agc agc(1.0, 0.02);
  cplx y;
  for (int i = 0; i < 3000; ++i) y = agc.process(std::polar(0.05, 0.1 * i));
  EXPECT_NEAR(std::abs(y), 1.0, 0.03);
  for (int i = 0; i < 3000; ++i) y = agc.process(std::polar(2.0, 0.1 * i));
  EXPECT_NEAR(std::abs(y), 1.0, 0.03);
}
