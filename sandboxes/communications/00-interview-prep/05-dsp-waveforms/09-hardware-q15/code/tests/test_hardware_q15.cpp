#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include "hardware_q15.hpp"

using namespace dsp;

// ---------------------------------------------------------------- B1
TEST(Q15Convert, KnownValues) {
  EXPECT_EQ(float_to_q15(0.0), 0);
  EXPECT_EQ(float_to_q15(0.5), 16384);
  EXPECT_EQ(float_to_q15(-0.5), -16384);
  EXPECT_EQ(float_to_q15(0.1), 3277);   // 3276.8
  EXPECT_EQ(float_to_q15(-0.1), -3277);
  EXPECT_EQ(float_to_q15(0.3), 9830);   // 9830.4
  EXPECT_EQ(float_to_q15(0.7), 22938);  // 22937.6
  EXPECT_EQ(float_to_q15(0.999), 32735);
  EXPECT_EQ(float_to_q15(-1.0), -32768);
}

TEST(Q15Convert, SaturatesInsteadOfWrapping) {
  EXPECT_EQ(float_to_q15(1.0), 32767);
  EXPECT_EQ(float_to_q15(1.5), 32767);
  EXPECT_EQ(float_to_q15(100.0), 32767);
  EXPECT_EQ(float_to_q15(-1.5), -32768);
  EXPECT_EQ(float_to_q15(-100.0), -32768);
}

TEST(Q15Convert, TiesRoundAwayFromZero) {
  const double half_lsb = 0.5 / 32768.0;  // exactly representable
  EXPECT_EQ(float_to_q15(half_lsb), 1);
  EXPECT_EQ(float_to_q15(-half_lsb), -1);
  EXPECT_EQ(float_to_q15(0.49 / 32768.0), 0);
}

TEST(Q15Convert, BackToFloat) {
  EXPECT_DOUBLE_EQ(q15_to_float(16384), 0.5);
  EXPECT_DOUBLE_EQ(q15_to_float(-32768), -1.0);
  EXPECT_DOUBLE_EQ(q15_to_float(32767), 0.999969482421875);
}

TEST(Q15Convert, RoundTripErrorAtMostHalfLsb) {
  for (int k = -1000; k <= 1000; ++k) {
    const double x = 0.000913 * k;  // stays inside (-1, 1)
    EXPECT_LE(std::fabs(q15_to_float(float_to_q15(x)) - x), 0.5 / 32768.0 + 1e-15) << x;
  }
}

TEST(Q15Convert, SqnrOfHalfScaleSine) {
  // Theory: 10 log10(1.5 * 2^30) = 92.07 dB for amplitude 0.5 (uniform error model).
  std::vector<double> ref(20000);
  std::vector<q15_t> q(ref.size());
  for (std::size_t n = 0; n < ref.size(); ++n) {
    ref[n] = 0.5 * std::sin(2.0 * kPi * 0.0123456789 * static_cast<double>(n));
    q[n] = float_to_q15(ref[n]);
  }
  EXPECT_NEAR(q15_sqnr_db(ref, q), 92.07, 1.0);
}

// ---------------------------------------------------------------- B2
TEST(Q15Add, SaturatesAtBothEnds) {
  EXPECT_EQ(q15_add_sat(24576, 16384), 32767);    // 0.75 + 0.5 would wrap to -0.75
  EXPECT_EQ(q15_add_sat(-24576, -16384), -32768);
  EXPECT_EQ(q15_add_sat(32767, 1), 32767);
  EXPECT_EQ(q15_add_sat(-32768, -1), -32768);
  EXPECT_EQ(q15_add_sat(1000, -400), 600);
  EXPECT_EQ(q15_add_sat(32767, -32768), -1);
}

// ---------------------------------------------------------------- B3
TEST(Q15Mul, WorkedExamples) {
  EXPECT_EQ(q15_mul(16384, 16384), 8192);  // 0.5 * 0.5 = 0.25
  EXPECT_EQ(q15_mul(16384, 9830), 4915);   // 0.5 * 0.3
  EXPECT_EQ(q15_mul(22938, 22938), 16057); // 0.7 * 0.7 -> 0.49002
  EXPECT_EQ(q15_mul(3, 3), 0);
  EXPECT_EQ(q15_mul(0, 12345), 0);
}

TEST(Q15Mul, MinusOneTimesMinusOneSaturates) {
  EXPECT_EQ(q15_mul(-32768, -32768), 32767);
  EXPECT_EQ(q15_mul(-32768, 32767), -32767);
  EXPECT_EQ(q15_mul(32767, 32767), 32766);  // 0.99994 -> 32766.0 after rounding (1073676289+16384)>>15
}

TEST(Q15Mul, RoundingConventionTiesTowardPlusInfinity) {
  EXPECT_EQ(q15_mul(1, 16384), 1);   // +0.5 LSB rounds up
  EXPECT_EQ(q15_mul(-1, 16384), 0);  // -0.5 LSB rounds up (toward +inf)
  EXPECT_EQ(q15_mul(-3, 16384), -1); // -1.5 -> -1
}

TEST(Q15Mul, ErrorAtMostHalfLsbVsExact) {
  for (int a = -32768; a <= 32767; a += 997) {
    for (int b = -32768; b <= 32767; b += 1009) {
      if (a == -32768 && b == -32768) continue;
      const double exact = static_cast<double>(a) * b / 32768.0;
      const double got = q15_mul(static_cast<q15_t>(a), static_cast<q15_t>(b));
      EXPECT_LE(std::fabs(got - exact), 0.5 + 1e-9) << a << " * " << b;
    }
  }
}

TEST(Q31Mul, Basics) {
  EXPECT_EQ(q31_mul(1 << 30, 1 << 30), 1 << 29);  // 0.5 * 0.5
  EXPECT_EQ(q31_mul(std::numeric_limits<q31_t>::min(), std::numeric_limits<q31_t>::min()),
            std::numeric_limits<q31_t>::max());
  EXPECT_EQ(q31_mul(std::numeric_limits<q31_t>::min(), 1 << 30), -(1 << 30));  // -1 * 0.5 = -0.5
  EXPECT_EQ(q31_mul(0, 123456789), 0);
}

// ---------------------------------------------------------------- B4
TEST(Q15Fir, ScalesByHalf) {
  const std::vector<q15_t> x = {100, -200, 300, 0, 32767, -32768};
  const std::vector<q15_t> h = {16384};
  const auto y = q15_fir(x, h);
  ASSERT_EQ(y.size(), x.size());
  EXPECT_EQ(y[0], 50);
  EXPECT_EQ(y[1], -100);
  EXPECT_EQ(y[2], 150);
  EXPECT_EQ(y[3], 0);
  EXPECT_EQ(y[4], 16384);   // (32767*16384 + 16384) >> 15 = 16384
  EXPECT_EQ(y[5], -16384);
}

TEST(Q15Fir, SmoothingKernelSteadyState) {
  const std::vector<q15_t> h = {8192, 16384, 8192};  // 0.25, 0.5, 0.25
  const std::vector<q15_t> x(10, 16384);             // 0.5 constant
  const auto y = q15_fir(x, h);
  EXPECT_EQ(y[0], 4096);   // only first tap overlaps: 0.25*0.5 = 0.125
  EXPECT_EQ(y[1], 12288);  // 0.375
  for (std::size_t n = 2; n < y.size(); ++n) EXPECT_EQ(y[n], 16384) << n;
}

TEST(Q15Fir, ImpulseResponseReturnsTaps) {
  const std::vector<q15_t> h = {1234, -5678, 9012, -3456, 789};
  std::vector<q15_t> x(8, 0);
  x[0] = 32767;
  const auto y = q15_fir(x, h);
  for (std::size_t k = 0; k < h.size(); ++k) {
    // impulse of 0.99997: output is h[k] * 32767 / 32768, rounded
    const double exact = static_cast<double>(h[k]) * 32767.0 / 32768.0;
    EXPECT_LE(std::fabs(static_cast<double>(y[k]) - exact), 0.5 + 1e-9) << k;
  }
  EXPECT_EQ(y[5], 0);
}

TEST(Q15Fir, WideAccumulatorAndSaturation) {
  // Four full-scale taps on a full-scale input: the sum is ~4.0 (needs > 32 bits), result must saturate.
  const std::vector<q15_t> h(4, 32767);
  const std::vector<q15_t> x(6, 32767);
  const auto y = q15_fir(x, h);
  EXPECT_EQ(y[0], 32766);   // single term
  EXPECT_EQ(y[1], 32767);   // saturated
  EXPECT_EQ(y[5], 32767);
  const std::vector<q15_t> xn(6, -32768);
  const auto yn = q15_fir(xn, h);
  EXPECT_EQ(yn[5], -32768);
}

TEST(Q15Fir, MatchesDoubleReferenceWithinHalfLsb) {
  std::uint32_t s = 12345;
  auto rnd = [&s]() {
    s = s * 1664525u + 1013904223u;
    return static_cast<int>((s >> 16) & 0x3FFF) - 0x2000;  // [-8192, 8191]
  };
  std::vector<q15_t> h(15), x(300);
  for (auto& v : h) v = static_cast<q15_t>(rnd() / 4);  // |h| <= 2048 (0.0625): no saturation possible
  for (auto& v : x) v = static_cast<q15_t>(rnd());
  const auto y = q15_fir(x, h);
  for (std::size_t n = 0; n < x.size(); ++n) {
    double acc = 0.0;
    for (std::size_t k = 0; k < h.size() && k <= n; ++k) acc += static_cast<double>(h[k]) * x[n - k];
    const double exact = acc / 32768.0;
    EXPECT_LE(std::fabs(static_cast<double>(y[n]) - exact), 0.5 + 1e-9) << n;
  }
}

// ---------------------------------------------------------------- B5
TEST(GuardBits, Examples) {
  EXPECT_EQ(guard_bits({0.5, 0.5}), 0);                 // sum 1
  EXPECT_EQ(guard_bits({0.5, 0.5, 0.5}), 1);            // sum 1.5
  EXPECT_EQ(guard_bits(std::vector<double>(8, 0.5)), 2);  // sum 4 exactly -> 2 bits
  EXPECT_EQ(guard_bits(std::vector<double>(9, 0.5)), 3);  // sum 4.5
  EXPECT_EQ(guard_bits({0.2, -0.3}), 0);                // |.| sum 0.5
  EXPECT_EQ(guard_bits({0.9, -0.9, 0.9}), 2);           // 2.7 -> log2 = 1.43 -> 2
  EXPECT_EQ(guard_bits(std::vector<double>(256, 1.0)), 8);
}

// ---------------------------------------------------------------- B6/B7
namespace {
std::vector<cplx> tone(std::size_t n, double f) {
  std::vector<cplx> z(n);
  for (std::size_t i = 0; i < n; ++i) z[i] = std::polar(1.0, 2.0 * kPi * f * static_cast<double>(i));
  return z;
}
}  // namespace

TEST(DcRemoval, RemovesOffsetKeepsTone) {
  auto z = tone(1024, 0.125);
  const cplx dc(0.3, -0.2);
  for (auto& v : z) v += dc;
  const auto y = remove_dc(z);
  cplx mean(0, 0);
  for (const auto& v : y) mean += v;
  EXPECT_LT(std::abs(mean) / static_cast<double>(y.size()), 1e-12);
  const auto ref = tone(1024, 0.125);
  for (std::size_t i = 0; i < y.size(); ++i) EXPECT_LT(std::abs(y[i] - ref[i]), 1e-9) << i;
}

TEST(IqImbalance, ImageBeforeAndAfter) {
  const auto z = tone(1024, 0.125);  // 128 integer cycles
  EXPECT_GT(measure_image_rejection_db(z, 0.125), 150.0);  // clean tone: image at numerical-noise level
  const double g = 1.1, phi = 5.0 * kPi / 180.0;
  const auto bad = apply_iq_imbalance(z, g, phi);
  const double before = measure_image_rejection_db(bad, 0.125);
  EXPECT_NEAR(before, 23.8, 0.3);  // |1+g e^{j phi}|^2 / |1-g e^{-j phi}|^2
  const auto fixed = correct_iq_imbalance(bad);
  EXPECT_GT(measure_image_rejection_db(fixed, 0.125), 60.0);
}

TEST(IqImbalance, GainOnlyErrorTheoryMatches) {
  const auto z = tone(512, 0.25);
  const auto bad = apply_iq_imbalance(z, 1.01, 0.0);
  EXPECT_NEAR(measure_image_rejection_db(bad, 0.25), 46.1, 0.2);  // ((g+1)/(g-1))^2
}

TEST(IqImbalance, CorrectionLeavesBalancedSignalAlone) {
  const auto z = tone(1024, 0.0625);
  const auto y = correct_iq_imbalance(z);
  for (std::size_t i = 0; i < z.size(); ++i) EXPECT_LT(std::abs(y[i] - z[i]), 1e-9) << i;
}

TEST(IqImbalance, DcOffsetAndImbalanceTogether) {
  const auto z = tone(1024, 0.125);
  auto bad = apply_iq_imbalance(z, 0.9, -3.0 * kPi / 180.0);
  for (auto& v : bad) v += cplx(0.05, 0.08);
  const auto fixed = correct_iq_imbalance(remove_dc(bad));
  EXPECT_GT(measure_image_rejection_db(fixed, 0.125), 60.0);
}

TEST(IqImbalance, EmptyAndSilentInputsDoNotCrash) {
  EXPECT_TRUE(correct_iq_imbalance({}).empty());
  const std::vector<cplx> zeros(16, cplx(0, 0));
  const auto y = correct_iq_imbalance(zeros);
  ASSERT_EQ(y.size(), 16u);
  for (const auto& v : y) EXPECT_EQ(v, cplx(0, 0));
  EXPECT_TRUE(remove_dc({}).empty());
}

// ---------------------------------------------------------------- B8/B9
TEST(Cascade, FriisNoiseFigure) {
  // preselector filter (1 dB loss), LNA (+15 dB, NF 1.5), mixer (-7 dB, NF 7), IF amp (+20 dB, NF 3)
  const std::vector<Stage> filter_first = {{-1.0, 1.0}, {15.0, 1.5}, {-7.0, 7.0}, {20.0, 3.0}};
  EXPECT_NEAR(cascade_nf_db(filter_first), 3.30, 0.02);
  const std::vector<Stage> lna_first = {{15.0, 1.5}, {-1.0, 1.0}, {-7.0, 7.0}, {20.0, 3.0}};
  EXPECT_NEAR(cascade_nf_db(lna_first), 2.50, 0.05);  // loss after the LNA is divided by 31.6 (G_LNA)
  const std::vector<Stage> only_loss = {{-3.0, 3.0}};
  EXPECT_NEAR(cascade_nf_db(only_loss), 3.0, 1e-9);
}

TEST(Cascade, FirstStageDominates) {
  const std::vector<Stage> a = {{30.0, 1.0}, {0.0, 10.0}};  // F = 1.259 + 9/1000 = 1.268 -> 1.03 dB
  EXPECT_NEAR(cascade_nf_db(a), 1.03, 0.03);
}

TEST(Impairments, CfoPhaseNoiseReciprocalMixing) {
  EXPECT_NEAR(ppm_to_hz(1.0, 450e6), 450.0, 1e-9);
  EXPECT_NEAR(ppm_to_hz(2.5, 800e6), 2000.0, 1e-9);
  EXPECT_NEAR(cfo_phase_per_symbol_deg(450.0, 4800.0), 33.75, 1e-9);
  EXPECT_NEAR(phase_noise_linewidth_hz(0.01, 76800.0), 1.2223, 1e-3);
  EXPECT_NEAR(reciprocal_mixing_floor_dbm(-48.0, -120.0, 12500.0), -127.03, 0.01);
}
