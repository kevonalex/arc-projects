#include <gtest/gtest.h>

#include <cmath>
#include <complex>

#include "filters.hpp"

using namespace dsp;

namespace {
std::vector<double> lcg_signal(std::size_t n, unsigned seed = 42) {
  std::vector<double> v(n);
  unsigned s = seed;
  for (auto& x : v) {
    s = s * 1664525u + 1013904223u;
    x = (static_cast<double>(s >> 8) / 8388608.0) - 1.0;
  }
  return v;
}
// |H(f)| of an FIR at f cycles/sample.
double mag_response(const std::vector<double>& h, double f) {
  std::complex<double> acc(0.0, 0.0);
  for (std::size_t k = 0; k < h.size(); ++k) {
    acc += h[k] * std::polar(1.0, -2.0 * kPi * f * static_cast<double>(k));
  }
  return std::abs(acc);
}
}  // namespace

TEST(Convolve, SmallExamples) {
  const auto y = convolve({1, 2, 3}, {1, -1});
  ASSERT_EQ(y.size(), 4u);
  EXPECT_DOUBLE_EQ(y[0], 1);
  EXPECT_DOUBLE_EQ(y[1], 1);
  EXPECT_DOUBLE_EQ(y[2], 1);
  EXPECT_DOUBLE_EQ(y[3], -3);
  EXPECT_TRUE(convolve({}, {1}).empty());
}

TEST(Convolve, MovingAverageExample) {
  const auto y = convolve({0, 0, 6, 0, 0, 6, 6, 6}, {1.0 / 3, 1.0 / 3, 1.0 / 3});
  const double expect[] = {0, 0, 2, 2, 2, 2, 4, 6};
  for (int i = 0; i < 8; ++i) EXPECT_NEAR(y[static_cast<std::size_t>(i)], expect[i], 1e-12);
}

TEST(Convolve, Commutative) {
  const auto a = lcg_signal(20, 1), b = lcg_signal(7, 2);
  const auto ab = convolve(a, b), ba = convolve(b, a);
  ASSERT_EQ(ab.size(), ba.size());
  for (std::size_t i = 0; i < ab.size(); ++i) EXPECT_NEAR(ab[i], ba[i], 1e-12);
}

TEST(Hamming, EndpointsAndSymmetry) {
  const auto w = hamming(11);
  EXPECT_NEAR(w[0], 0.08, 1e-12);
  EXPECT_NEAR(w[5], 1.0, 1e-12);
  EXPECT_NEAR(w[1], 0.16785, 1e-4);
  for (std::size_t i = 0; i < 11; ++i) EXPECT_NEAR(w[i], w[10 - i], 1e-12);
  EXPECT_EQ(hamming(1).size(), 1u);
}

TEST(WindowedSinc, WorkedExampleTaps) {
  const auto h = windowed_sinc_lowpass(11, 0.1);
  ASSERT_EQ(h.size(), 11u);
  EXPECT_NEAR(h[5], 0.2370, 1e-3);
  EXPECT_NEAR(h[4], 0.2022, 1e-3);
  EXPECT_NEAR(h[3], 0.1224, 1e-3);
  double s = 0.0;
  for (double v : h) s += v;
  EXPECT_NEAR(s, 1.0, 1e-12);
}

TEST(WindowedSinc, LinearPhaseSymmetryAndStopband) {
  const auto h = windowed_sinc_lowpass(31, 0.1);
  for (std::size_t i = 0; i < h.size(); ++i) EXPECT_NEAR(h[i], h[h.size() - 1 - i], 1e-12);
  EXPECT_NEAR(mag_response(h, 0.0), 1.0, 1e-9);
  EXPECT_NEAR(mag_response(h, 0.1), 0.5, 0.1);  // ~ -6 dB at the design cutoff
  for (double f : {0.25, 0.3, 0.4, 0.5}) {
    EXPECT_LT(mag_response(h, f), std::pow(10.0, -45.0 / 20.0)) << "f=" << f;
  }
}

TEST(Biquad, CoefficientsMatchCookbookExample) {
  const auto c = biquad_lowpass(0.1, 0.70710678);
  EXPECT_NEAR(c.b0, 0.067455, 1e-4);
  EXPECT_NEAR(c.b1, 0.134911, 1e-4);
  EXPECT_NEAR(c.b2, 0.067455, 1e-4);
  EXPECT_NEAR(c.a1, -1.142980, 1e-4);
  EXPECT_NEAR(c.a2, 0.412802, 1e-4);
}

TEST(Biquad, ImpulseStepAndNyquist) {
  const auto c = biquad_lowpass(0.1, 0.70710678);
  Biquad bq(c);
  EXPECT_NEAR(bq.process(1.0), 0.067455, 1e-4);
  EXPECT_NEAR(bq.process(0.0), 0.212010, 1e-4);

  bq.reset();
  double y = 0.0;
  for (int i = 0; i < 400; ++i) y = bq.process(1.0);
  EXPECT_NEAR(y, 1.0, 1e-6);  // unity DC gain

  bq.reset();
  for (int i = 0; i < 400; ++i) y = bq.process((i % 2 == 0) ? 1.0 : -1.0);
  EXPECT_NEAR(y, 0.0, 1e-6);  // zero at Nyquist
}

TEST(Decimate, PolyphaseEqualsFilterThenDiscard) {
  const auto x = lcg_signal(101, 7);
  const std::vector<double> h = {0.1, 0.2, 0.3, 0.25, 0.15, 0.1, -0.05, 0.02, 0.01, 0.03};
  for (std::size_t m : {1u, 2u, 3u, 4u, 7u}) {
    const auto full = convolve(x, h);
    const auto got = decimate_polyphase(x, m, h);
    ASSERT_EQ(got.size(), (x.size() + m - 1) / m) << "M=" << m;
    for (std::size_t i = 0; i < got.size(); ++i) EXPECT_NEAR(got[i], full[i * m], 1e-12) << "M=" << m << " i=" << i;
  }
}

TEST(Interpolate, PolyphaseEqualsZeroStuffThenFilter) {
  const auto x = lcg_signal(40, 9);
  const std::vector<double> h = lcg_signal(12, 5);
  for (std::size_t l : {1u, 2u, 3u, 5u}) {
    std::vector<double> up(x.size() * l, 0.0);
    for (std::size_t i = 0; i < x.size(); ++i) up[i * l] = x[i];
    const auto ref = convolve(up, h);
    const auto got = interpolate_polyphase(x, l, h);
    ASSERT_EQ(got.size(), x.size() * l) << "L=" << l;
    for (std::size_t i = 0; i < got.size(); ++i) EXPECT_NEAR(got[i], ref[i], 1e-12) << "L=" << l << " i=" << i;
  }
}

TEST(Interpolate, DcGainRestoresAmplitude) {
  // L=3 with all-ones taps of length 3 (DC gain 3): constant input -> constant output after start-up.
  const std::vector<double> x(10, 1.0);
  const auto y = interpolate_polyphase(x, 3, {1.0, 1.0, 1.0});
  for (std::size_t i = 3; i < y.size(); ++i) EXPECT_NEAR(y[i], 1.0, 1e-12);
}
