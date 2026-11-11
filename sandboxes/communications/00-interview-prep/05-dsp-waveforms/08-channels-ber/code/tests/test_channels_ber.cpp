#include <gtest/gtest.h>

#include <cmath>

#include "channels_ber.hpp"

using namespace dsp;

TEST(Analytic, QFunction) {
  EXPECT_NEAR(q_function(0.0), 0.5, 1e-12);
  EXPECT_NEAR(q_function(1.0), 0.158655, 1e-6);
  EXPECT_NEAR(q_function(3.0), 0.0013499, 1e-7);
  EXPECT_NEAR(q_function(-1.0), 1.0 - 0.158655, 1e-6);
}

TEST(Analytic, BerCurves) {
  EXPECT_NEAR(bpsk_ber_awgn(0.0), 0.078650, 1e-5);   // Q(sqrt2)
  EXPECT_NEAR(bpsk_ber_awgn(10.0), 3.872e-6, 5e-8);  // Q(sqrt20)
  EXPECT_NEAR(bpsk_ber_awgn(6.79), 1e-3, 1e-4);
  EXPECT_NEAR(bpsk_ber_awgn(9.6), 1e-5, 2e-6);
  EXPECT_NEAR(rayleigh_bpsk_ber(10.0), 0.0232685, 1e-6);
  EXPECT_NEAR(rayleigh_bpsk_ber(20.0), 0.0024813, 1e-6);
  // large-SNR approximation 1/(4 g)
  EXPECT_NEAR(rayleigh_bpsk_ber(30.0) * 4000.0, 1.0, 2e-3);
}

TEST(Conversions, EbN0AndNoiseFloor) {
  EXPECT_NEAR(snr_db_to_ebn0_db(10.0, 12500.0, 9600.0), 11.1463, 1e-3);
  EXPECT_NEAR(ebn0_db_to_snr_db(9.6, 12500.0, 9600.0), 9.6 - 1.14634, 1e-3);
  EXPECT_NEAR(ebn0_db_to_snr_db(snr_db_to_ebn0_db(3.3, 25000.0, 36000.0), 25000.0, 36000.0), 3.3, 1e-12);
  EXPECT_NEAR(noise_floor_dbm(12500.0, 0.0), -133.03, 0.01);
  EXPECT_NEAR(noise_floor_dbm(12500.0, 6.0), -127.03, 0.01);
}

TEST(Doppler, ShiftAndCoherenceTime) {
  const double fd = doppler_shift_hz(100.0 / 3.6, 400e6);
  EXPECT_NEAR(fd, 37.063, 0.01);
  EXPECT_NEAR(coherence_time_s(fd), 0.423 / fd, 1e-12);
  EXPECT_NEAR(coherence_time_s(fd), 0.01141, 2e-4);
  EXPECT_NEAR(doppler_shift_hz(0.0, 400e6), 0.0, 1e-12);
}

TEST(Awgn, NoiseVarianceIsN0SplitAcrossIAndQ) {
  std::mt19937_64 rng(5);
  const std::vector<cplx> zeros(200000, cplx(0.0, 0.0));
  const double n0 = 0.4;
  const auto y = add_awgn(zeros, n0, rng);
  double pi = 0.0, pq = 0.0, cross = 0.0;
  for (const auto& v : y) {
    pi += v.real() * v.real();
    pq += v.imag() * v.imag();
    cross += v.real() * v.imag();
  }
  const double n = static_cast<double>(y.size());
  EXPECT_NEAR(pi / n, n0 / 2.0, 0.01);
  EXPECT_NEAR(pq / n, n0 / 2.0, 0.01);
  EXPECT_NEAR(cross / n, 0.0, 0.01);
}

TEST(Ber, QpskSimulationMatchesTheory) {
  for (double ebn0 : {2.0, 4.0, 6.0}) {
    const double sim = simulate_qpsk_ber(ebn0, 2000000, 12345);
    const double theory = bpsk_ber_awgn(ebn0);
    EXPECT_NEAR(sim / theory, 1.0, 0.10) << "Eb/N0=" << ebn0 << " sim=" << sim << " theory=" << theory;
  }
}

TEST(Ber, RayleighSimulationMatchesFormula) {
  for (double ebn0 : {10.0, 20.0}) {
    const double sim = simulate_bpsk_rayleigh_ber(ebn0, 2000000, 777);
    const double theory = rayleigh_bpsk_ber(ebn0);
    EXPECT_NEAR(sim / theory, 1.0, 0.08) << "Eb/N0=" << ebn0 << " sim=" << sim << " theory=" << theory;
  }
}

namespace {
void stats(const std::vector<cplx>& h, double& mean_p, double& var_p, double& frac_below_01) {
  const double n = static_cast<double>(h.size());
  double s = 0.0, s2 = 0.0, below = 0.0;
  for (const auto& v : h) {
    const double p = std::norm(v);
    s += p;
    s2 += p * p;
    if (p < 0.1) below += 1.0;
  }
  mean_p = s / n;
  var_p = s2 / n - mean_p * mean_p;
  frac_below_01 = below / n;
}
}  // namespace

TEST(Fading, RayleighStatistics) {
  const auto h = rayleigh_fading(100000, 0.02, 4242);
  ASSERT_EQ(h.size(), 100000u);
  double m, v, f;
  stats(h, m, v, f);
  EXPECT_NEAR(m, 1.0, 0.15);
  EXPECT_NEAR(f, 1.0 - std::exp(-0.1), 0.04);  // 0.0952
  EXPECT_GT(v, 0.6);                            // exponential distribution has variance 1
}

TEST(Fading, RayleighIsReproducibleAndTimeCorrelated) {
  const auto a = rayleigh_fading(500, 0.01, 9);
  const auto b = rayleigh_fading(500, 0.01, 9);
  for (std::size_t i = 0; i < a.size(); ++i) EXPECT_EQ(a[i], b[i]);
  // adjacent samples (fd = 0.01) are strongly correlated: |h[n+1]-h[n]| small vs |h|
  double d = 0.0, p = 0.0;
  for (std::size_t i = 0; i + 1 < a.size(); ++i) {
    d += std::norm(a[i + 1] - a[i]);
    p += std::norm(a[i]);
  }
  EXPECT_LT(d / p, 0.05);
}

TEST(Fading, RicianHasShallowerFadesThanRayleigh) {
  const auto h = rician_fading(100000, 0.02, 10.0, 31337);
  double m, v, f;
  stats(h, m, v, f);
  EXPECT_NEAR(m, 1.0, 0.15);
  EXPECT_LT(v, 0.4);      // theory (1+2K)/(1+K)^2 = 0.174 for K = 10
  EXPECT_LT(f, 0.005);    // far fewer deep fades than Rayleigh's 9.5 %
}
