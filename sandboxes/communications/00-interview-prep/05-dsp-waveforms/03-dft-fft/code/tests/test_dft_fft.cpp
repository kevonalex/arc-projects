#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "dft_fft.hpp"

using namespace dsp;

namespace {
// Deterministic pseudo-random complex vector (LCG), no <random> dependence.
std::vector<cplx> pseudo_random(std::size_t n, unsigned seed = 12345) {
  std::vector<cplx> v(n);
  unsigned s = seed;
  auto next = [&s]() {
    s = s * 1664525u + 1013904223u;
    return (static_cast<double>(s >> 8) / 8388608.0) - 1.0;  // [-1, 1)
  };
  for (auto& c : v) c = cplx(next(), next());
  return v;
}
double max_err(const std::vector<cplx>& a, const std::vector<cplx>& b) {
  double e = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) e = std::max(e, std::abs(a[i] - b[i]));
  return e;
}
}  // namespace

TEST(Dft, CosineFourPoint) {
  const std::vector<cplx> x = {1, 0, -1, 0};
  const auto X = dft(x);
  ASSERT_EQ(X.size(), 4u);
  EXPECT_NEAR(std::abs(X[0]), 0.0, 1e-12);
  EXPECT_NEAR(X[1].real(), 2.0, 1e-12);
  EXPECT_NEAR(std::abs(X[2]), 0.0, 1e-12);
  EXPECT_NEAR(X[3].real(), 2.0, 1e-12);
}

TEST(Dft, WorkedEightPointExample) {
  const std::vector<cplx> x = {1, 2, 3, 4, 0, 0, 0, 0};
  const auto X = dft(x);
  ASSERT_EQ(X.size(), 8u);
  EXPECT_NEAR(X[0].real(), 10.0, 1e-9);
  EXPECT_NEAR(X[1].real(), -0.4142, 1e-4);
  EXPECT_NEAR(X[1].imag(), -7.2426, 1e-4);
  EXPECT_NEAR(X[2].real(), -2.0, 1e-9);
  EXPECT_NEAR(X[2].imag(), 2.0, 1e-9);
  EXPECT_NEAR(X[3].real(), 2.4142, 1e-4);
  EXPECT_NEAR(X[3].imag(), -1.2426, 1e-4);
  EXPECT_NEAR(X[4].real(), -2.0, 1e-9);
  EXPECT_NEAR(X[7].imag(), 7.2426, 1e-4);
}

TEST(BitReverse, ThreeBits) {
  EXPECT_EQ(bit_reverse(0, 3), 0u);
  EXPECT_EQ(bit_reverse(1, 3), 4u);
  EXPECT_EQ(bit_reverse(3, 3), 6u);
  EXPECT_EQ(bit_reverse(6, 3), 3u);
  EXPECT_EQ(bit_reverse(7, 3), 7u);
  EXPECT_EQ(bit_reverse(1, 4), 8u);
}

TEST(Fft, MatchesDft) {
  for (std::size_t n : {1u, 2u, 4u, 8u, 64u, 256u}) {
    const auto x = pseudo_random(n);
    EXPECT_LT(max_err(fft_radix2(x), dft(x)), 1e-9) << "n=" << n;
  }
}

TEST(Fft, InverseRoundTrip) {
  const auto x = pseudo_random(128, 777);
  EXPECT_LT(max_err(ifft_radix2(fft_radix2(x)), x), 1e-12);
}

TEST(Fft, Parseval) {
  const auto x = pseudo_random(64, 99);
  const auto X = fft_radix2(x);
  double ex = 0.0, eX = 0.0;
  for (const auto& v : x) ex += std::norm(v);
  for (const auto& v : X) eX += std::norm(v);
  EXPECT_NEAR(eX, 64.0 * ex, 1e-9 * eX);
}

TEST(Fft, RejectsNonPowerOfTwo) {
  EXPECT_THROW(fft_radix2(std::vector<cplx>(12)), std::invalid_argument);
  EXPECT_THROW(ifft_radix2(std::vector<cplx>(6)), std::invalid_argument);
  EXPECT_TRUE(fft_radix2({}).empty());
}

TEST(Hann, Shape) {
  const auto w = hann_periodic(64);
  ASSERT_EQ(w.size(), 64u);
  EXPECT_NEAR(w[0], 0.0, 1e-12);
  EXPECT_NEAR(w[32], 1.0, 1e-12);
  double s = 0.0;
  for (double v : w) s += v;
  EXPECT_NEAR(s, 32.0, 1e-9);  // coherent gain 0.5
}

namespace {
std::vector<cplx> windowed_tone(double bin, std::size_t n, bool use_hann) {
  const auto w = hann_periodic(n);
  std::vector<cplx> x(n);
  for (std::size_t i = 0; i < n; ++i) {
    const double ph = 2.0 * kPi * bin * static_cast<double>(i) / static_cast<double>(n);
    x[i] = std::polar(use_hann ? w[i] : 1.0, ph);
  }
  return x;
}
}  // namespace

TEST(Leakage, HannBeatsRectangularFarFromTone) {
  const std::size_t n = 128;
  const auto Xr = fft_radix2(windowed_tone(10.5, n, false));
  const auto Xh = fft_radix2(windowed_tone(10.5, n, true));
  double pr = 0.0, ph = 0.0;
  for (std::size_t k = 0; k < n; ++k) {
    pr = std::max(pr, std::abs(Xr[k]));
    ph = std::max(ph, std::abs(Xh[k]));
  }
  EXPECT_GT(std::abs(Xr[70]) / pr, 1e-3);  // rectangular: ~ -41 dB at 60 bins away
  EXPECT_LT(std::abs(Xh[70]) / ph, 1e-4);  // Hann: ~ -115 dB
}

TEST(PeakEstimate, FractionalBinWithHann) {
  const std::size_t n = 128;
  for (double bin : {10.0, 10.3, 33.7, 61.5}) {
    const auto X = fft_radix2(windowed_tone(bin, n, true));
    EXPECT_NEAR(peak_bin_estimate(X), bin, 0.1) << "bin=" << bin;
  }
  // negative-frequency tone wraps: -5.7 bins == 122.3
  const auto Xn = fft_radix2(windowed_tone(-5.7, n, true));
  EXPECT_NEAR(peak_bin_estimate(Xn), 128.0 - 5.7, 0.1);
}
