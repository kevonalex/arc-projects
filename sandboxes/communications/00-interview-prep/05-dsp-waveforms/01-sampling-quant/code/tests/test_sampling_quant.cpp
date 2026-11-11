#include <gtest/gtest.h>

#include <cmath>

#include "sampling_quant.hpp"

using namespace dsp;

TEST(Alias, KnownValues) {
  EXPECT_DOUBLE_EQ(alias_frequency(3000, 8000), 3000);
  EXPECT_DOUBLE_EQ(alias_frequency(5000, 8000), 3000);
  EXPECT_DOUBLE_EQ(alias_frequency(7000, 8000), 1000);
  EXPECT_DOUBLE_EQ(alias_frequency(-7000, 8000), 1000);
  EXPECT_DOUBLE_EQ(alias_frequency(13000, 8000), 3000);
  EXPECT_DOUBLE_EQ(alias_frequency(16500, 8000), 500);
  EXPECT_DOUBLE_EQ(alias_frequency(4000, 8000), 4000);
  EXPECT_DOUBLE_EQ(alias_frequency(8000, 8000), 0);
  EXPECT_NEAR(alias_frequency(30000, 44100), 14100, 1e-9);
}

TEST(SampleSine, AliasedTonesGiveSameSamples) {
  // cos == sin shifted by pi/2; 7 kHz and 1 kHz at 8 kHz must match for cosine.
  const auto a = sample_sine(7000, 8000, kPi / 2, 32);
  const auto b = sample_sine(1000, 8000, kPi / 2, 32);
  ASSERT_EQ(a.size(), 32u);
  for (std::size_t i = 0; i < a.size(); ++i) EXPECT_NEAR(a[i], b[i], 1e-9);
  // sine flips sign
  const auto c = sample_sine(7000, 8000, 0.0, 32);
  const auto d = sample_sine(1000, 8000, 0.0, 32);
  for (std::size_t i = 0; i < c.size(); ++i) EXPECT_NEAR(c[i], -d[i], 1e-9);
}

TEST(SampleSine, NyquistSineIsZero) {
  const auto x = sample_sine(4000, 8000, 0.0, 16);
  for (double v : x) EXPECT_NEAR(v, 0.0, 1e-9);
}

TEST(Quantise, ThreeBitExamples) {
  EXPECT_DOUBLE_EQ(quantise_uniform(0.10, 3, 1.0), 0.0);
  EXPECT_DOUBLE_EQ(quantise_uniform(0.13, 3, 1.0), 0.25);
  EXPECT_DOUBLE_EQ(quantise_uniform(-0.30, 3, 1.0), -0.25);
  EXPECT_DOUBLE_EQ(quantise_uniform(10.0, 3, 1.0), 0.75);   // clips at FS - step
  EXPECT_DOUBLE_EQ(quantise_uniform(-10.0, 3, 1.0), -1.0);  // clips at -FS
}

TEST(Quantise, ErrorBoundedByHalfStep) {
  const int bits = 6;
  const double step = 2.0 / 64.0;
  for (int i = -900; i <= 900; ++i) {
    const double x = i / 1000.0;  // inside [-0.9, 0.9]: never clips
    EXPECT_LE(std::fabs(quantise_uniform(x, bits, 1.0) - x), step / 2 + 1e-12);
  }
}

TEST(Sqnr, IdealFormula) {
  for (int n = 2; n <= 20; ++n) EXPECT_NEAR(ideal_sqnr_db(n), 6.02 * n + 1.76, 0.02);
}

TEST(Sqnr, MeasuredMatchesTheory) {
  for (int bits : {8, 10, 12}) {
    const double step = 2.0 / std::ldexp(1.0, bits);
    const double amp = 1.0 - step;
    const double expected = ideal_sqnr_db(bits) + 20.0 * std::log10(amp);
    EXPECT_NEAR(measured_sqnr_db(bits, 1u << 16), expected, 0.5) << "bits=" << bits;
  }
}

TEST(Bandpass, KnownBands) {
  EXPECT_NEAR(bandpass_fs_min(21e6, 26e6), 10.4e6, 1.0);
  EXPECT_NEAR(bandpass_fs_min(0.0, 5e6), 10e6, 1.0);   // baseband: 2*fH
  EXPECT_NEAR(bandpass_fs_min(20e6, 25e6), 10e6, 1.0);  // edge: window of zero width
}
