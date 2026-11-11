#include <gtest/gtest.h>

#include <cmath>

#include "iq_baseband.hpp"

using namespace dsp;

namespace {
std::vector<cplx> tone(double f, std::size_t n, double phase = 0.0) {
  std::vector<cplx> z(n);
  for (std::size_t i = 0; i < n; ++i) z[i] = std::polar(1.0, 2.0 * kPi * f * static_cast<double>(i) + phase);
  return z;
}
}  // namespace

TEST(Mix, EighthTurnPerSample) {
  const std::vector<cplx> ones(8, cplx(1.0, 0.0));
  const auto y = mix(ones, 0.125, 0.0);
  ASSERT_EQ(y.size(), 8u);
  EXPECT_NEAR(y[0].real(), 1.0, 1e-12);
  EXPECT_NEAR(y[2].real(), 0.0, 1e-12);
  EXPECT_NEAR(y[2].imag(), 1.0, 1e-12);   // 90 degrees after 2 samples
  EXPECT_NEAR(y[4].real(), -1.0, 1e-12);  // 180 degrees
}

TEST(Mix, DownThenUpIsIdentity) {
  const auto x = tone(0.07, 100, 0.3);
  const auto y = mix(mix(x, -0.2, 0.5), 0.2, -0.5);
  for (std::size_t i = 0; i < x.size(); ++i) EXPECT_LT(std::abs(x[i] - y[i]), 1e-9);
}

TEST(Mix, ShiftTheorem) {
  const auto x = tone(0.30, 64);
  const auto y = mix(x, -0.25, 0.0);  // should be a tone at 0.05
  EXPECT_NEAR(estimate_freq_cps(y), 0.05, 1e-9);
}

TEST(Freq, EstimateSignAndValue) {
  EXPECT_NEAR(estimate_freq_cps(tone(0.0123, 200)), 0.0123, 1e-9);
  EXPECT_NEAR(estimate_freq_cps(tone(-0.2, 200)), -0.2, 1e-9);
}

TEST(Freq, Instantaneous) {
  const auto f = instantaneous_freq(tone(0.05, 50));
  ASSERT_EQ(f.size(), 49u);
  for (double v : f) EXPECT_NEAR(v, 0.05, 1e-9);
  EXPECT_TRUE(instantaneous_freq(tone(0.05, 1)).empty());
}

TEST(Downconvert, RealCosineAboveAndBelowCarrierAreDistinguishable) {
  const std::vector<double> boxcar(4, 0.25);
  const std::size_t n = 2000;
  std::vector<double> above(n), below(n);
  for (std::size_t i = 0; i < n; ++i) {
    above[i] = std::cos(2.0 * kPi * 0.26 * static_cast<double>(i));
    below[i] = std::cos(2.0 * kPi * 0.24 * static_cast<double>(i));
  }
  auto za = iq_downconvert(above, 0.25, boxcar);
  auto zb = iq_downconvert(below, 0.25, boxcar);
  ASSERT_EQ(za.size(), n);
  za.erase(za.begin(), za.begin() + 10);  // skip filter start-up
  zb.erase(zb.begin(), zb.begin() + 10);
  EXPECT_NEAR(estimate_freq_cps(za), +0.01, 1e-3);
  EXPECT_NEAR(estimate_freq_cps(zb), -0.01, 1e-3);
  double mag = 0.0;
  for (const auto& v : za) mag += std::abs(v);
  mag /= static_cast<double>(za.size());
  EXPECT_NEAR(mag, 1.0, 0.05);
}

TEST(Image, CleanToneHasHugeRejection) {
  EXPECT_GT(image_rejection_db(tone(0.05, 200), 0.05), 100.0);
}

TEST(Image, PhaseErrorMatchesFormula) {
  const double phi = 0.1;
  const std::size_t n = 200;  // 10 whole periods of f = 0.05
  std::vector<cplx> z(n);
  for (std::size_t i = 0; i < n; ++i) {
    const double th = 2.0 * kPi * 0.05 * static_cast<double>(i);
    z[i] = cplx(std::cos(th), std::sin(th + phi));
  }
  const double expected = 20.0 * std::log10(1.0 / std::tan(phi / 2.0));
  EXPECT_NEAR(image_rejection_db(z, 0.05), expected, 0.05);
}
