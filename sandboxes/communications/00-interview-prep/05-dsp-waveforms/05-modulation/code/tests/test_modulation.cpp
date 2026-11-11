#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "modulation.hpp"

using namespace dsp;

namespace {
std::vector<std::uint8_t> dibit_stream(std::size_t n, unsigned seed = 1) {
  std::vector<std::uint8_t> v(n);
  unsigned s = seed;
  for (auto& d : v) {
    s = s * 1664525u + 1013904223u;
    d = static_cast<std::uint8_t>((s >> 16) & 3u);
  }
  return v;
}
int popcount4(unsigned v) {
  int c = 0;
  for (; v; v >>= 1) c += static_cast<int>(v & 1u);
  return c;
}
}  // namespace

// ---------------- 4FSK ----------------
TEST(Fsk4, LevelMap) {
  EXPECT_EQ(fsk4_level(0), +1);
  EXPECT_EQ(fsk4_level(1), +3);
  EXPECT_EQ(fsk4_level(2), -1);
  EXPECT_EQ(fsk4_level(3), -3);
}

TEST(Fsk4, ConstantEnvelopeAndPhaseContinuity) {
  // 4800 sym/s at 48 kS/s -> sps = 10; 9600 bit/s = 4800 * 2
  const auto d = dibit_stream(100);
  const auto x = fsk4_modulate(d, 10, 600.0, 48000.0);
  ASSERT_EQ(x.size(), 1000u);
  const double max_step = 2.0 * kPi * 1800.0 / 48000.0 + 1e-9;
  for (std::size_t n = 0; n + 1 < x.size(); ++n) {
    EXPECT_NEAR(std::abs(x[n]), 1.0, 1e-12);
    EXPECT_LE(std::fabs(std::arg(x[n + 1] * std::conj(x[n]))), max_step);
  }
}

TEST(Fsk4, OuterLevelIs1800Hz) {
  const std::vector<std::uint8_t> d(20, 1);  // all "01" -> +3 -> +1800 Hz
  const auto x = fsk4_modulate(d, 10, 600.0, 48000.0);
  double acc = 0.0;
  for (std::size_t n = 0; n + 1 < x.size(); ++n) acc += std::arg(x[n + 1] * std::conj(x[n]));
  const double f_hz = acc / static_cast<double>(x.size() - 1) * 48000.0 / (2.0 * kPi);
  EXPECT_NEAR(f_hz, 1800.0, 1e-6);
}

TEST(Fsk4, RoundTrip) {
  const auto d = dibit_stream(300, 5);
  const auto x = fsk4_modulate(d, 10, 600.0, 48000.0);
  EXPECT_EQ(fsk4_demodulate(x, 10, 600.0, 48000.0), d);
}

TEST(Fsk4, SmallCfoToleratedLargeCfoBreaksSlicer) {
  const auto d = dibit_stream(300, 9);
  auto x = fsk4_modulate(d, 10, 600.0, 48000.0);
  auto shifted = [&](double cfo_hz) {
    auto y = x;
    for (std::size_t n = 0; n < y.size(); ++n) {
      y[n] *= std::polar(1.0, 2.0 * kPi * cfo_hz * static_cast<double>(n) / 48000.0);
    }
    return y;
  };
  EXPECT_EQ(fsk4_demodulate(shifted(200.0), 10, 600.0, 48000.0), d);   // < 1 D : fine
  EXPECT_NE(fsk4_demodulate(shifted(700.0), 10, 600.0, 48000.0), d);   // > 1 D : levels cross thresholds
}

// ---------------- QPSK / pi/4-DQPSK ----------------
TEST(Qpsk, GrayAndRoundTrip) {
  for (unsigned d = 0; d < 4; ++d) {
    const cplx s = qpsk_map(static_cast<std::uint8_t>(d));
    EXPECT_NEAR(std::abs(s), 1.0, 1e-12);
    EXPECT_EQ(qpsk_demap(s), d);
  }
  // neighbours (sharing a quadrant edge) differ by one bit
  EXPECT_EQ(popcount4(0b00u ^ 0b01u), 1);
  EXPECT_GT(qpsk_map(0).real(), 0.0);
  EXPECT_GT(qpsk_map(0).imag(), 0.0);
  EXPECT_LT(qpsk_map(3).real(), 0.0);
  EXPECT_LT(qpsk_map(3).imag(), 0.0);
}

TEST(Qpsk, OppositeSymbolsPassThroughOrigin) {
  const cplx a = qpsk_map(0), b = qpsk_map(3);
  EXPECT_NEAR(std::abs(0.5 * (a + b)), 0.0, 1e-12);  // 180 degree transition: zero crossing
}

TEST(Pi4Dqpsk, RoundTripAndAlternatingConstellations) {
  const auto d = dibit_stream(200, 3);
  const auto s = pi4dqpsk_modulate(d);
  ASSERT_EQ(s.size(), d.size());
  EXPECT_EQ(pi4dqpsk_demodulate(s), d);
  for (std::size_t n = 0; n < s.size(); ++n) {
    EXPECT_NEAR(std::abs(s[n]), 1.0, 1e-12);
    const double units = std::arg(s[n]) / (kPi / 4.0);  // multiples of pi/4
    const double rounded = std::round(units);
    EXPECT_NEAR(units, rounded, 1e-9);
    const long parity = std::labs(static_cast<long>(rounded)) % 2;
    EXPECT_EQ(parity, (n % 2 == 0) ? 1 : 0) << "n=" << n;  // odd, even, odd, ...
  }
}

TEST(Pi4Dqpsk, KnownPhaseSequence) {
  // dibits 00,01,11,10 -> increments +1,+3,-3,-1 (units of pi/4) -> phases 1,4,1,0
  const auto s = pi4dqpsk_modulate({0, 1, 3, 2});
  ASSERT_EQ(s.size(), 4u);
  EXPECT_NEAR(std::arg(s[0]), kPi / 4.0, 1e-12);
  EXPECT_NEAR(std::fabs(std::arg(s[1])), kPi, 1e-12);
  EXPECT_NEAR(std::arg(s[2]), kPi / 4.0, 1e-12);
  EXPECT_NEAR(std::arg(s[3]), 0.0, 1e-12);
}

TEST(Pi4Dqpsk, NoZeroCrossings) {
  const auto s = pi4dqpsk_modulate(dibit_stream(2000, 11));
  const double min_allowed = std::cos(3.0 * kPi / 8.0) - 1e-9;  // 0.3827
  for (std::size_t n = 1; n < s.size(); ++n) EXPECT_GE(std::abs(0.5 * (s[n] + s[n - 1])), min_allowed);
}

TEST(Pi4Dqpsk, ConstantPhaseOffsetCancels) {
  const auto d = dibit_stream(100, 21);
  auto s = pi4dqpsk_modulate(d);
  const cplx rot = std::polar(1.0, 0.7);
  for (auto& v : s) v *= rot;
  const auto got = pi4dqpsk_demodulate(s, rot);  // reference carries the same rotation
  EXPECT_EQ(got, d);
  // without knowing the offset only the FIRST dibit is affected
  const auto got2 = pi4dqpsk_demodulate(s);
  ASSERT_EQ(got2.size(), d.size());
  for (std::size_t n = 1; n < d.size(); ++n) EXPECT_EQ(got2[n], d[n]);
}

// ---------------- QAM ----------------
TEST(Qam16, EnergyDistanceAndGray) {
  double e = 0.0;
  for (unsigned a = 0; a < 16; ++a) e += std::norm(qam16_map(static_cast<std::uint8_t>(a)));
  EXPECT_NEAR(e / 16.0, 1.0, 1e-12);
  const double dmin = 2.0 / std::sqrt(10.0);
  for (unsigned a = 0; a < 16; ++a) {
    for (unsigned b = a + 1; b < 16; ++b) {
      const double dist = std::abs(qam16_map(static_cast<std::uint8_t>(a)) - qam16_map(static_cast<std::uint8_t>(b)));
      EXPECT_GE(dist, dmin - 1e-9);
      if (std::fabs(dist - dmin) < 1e-9) EXPECT_EQ(popcount4(a ^ b), 1) << a << "," << b;
    }
  }
}

// ---------------- OFDM ----------------
TEST(Ofdm, SubcarrierIsComplexExponential) {
  std::vector<cplx> X(8, cplx(0, 0));
  X[1] = 1.0;
  const auto tx = ofdm_modulate(X, 0);
  ASSERT_EQ(tx.size(), 8u);
  EXPECT_NEAR(tx[2].real(), 0.0, 1e-12);
  EXPECT_NEAR(tx[2].imag(), 1.0 / 8.0, 1e-12);  // e^{j2pi*1*2/8}/8 = j/8
}

TEST(Ofdm, CyclicPrefixCopiesTail) {
  std::vector<cplx> X(16);
  const auto d = dibit_stream(16, 4);
  for (std::size_t k = 0; k < 16; ++k) X[k] = qpsk_map(d[k]);
  const auto tx = ofdm_modulate(X, 4);
  ASSERT_EQ(tx.size(), 20u);
  for (std::size_t i = 0; i < 4; ++i) EXPECT_LT(std::abs(tx[i] - tx[16 + i]), 1e-12);
  EXPECT_THROW(ofdm_modulate(X, 17), std::invalid_argument);
}

namespace {
std::vector<cplx> through_channel(const std::vector<cplx>& tx, const std::vector<cplx>& h, std::size_t out_len) {
  std::vector<cplx> rx(out_len, cplx(0, 0));
  for (std::size_t n = 0; n < out_len; ++n) {
    for (std::size_t k = 0; k < h.size() && k <= n; ++k) {
      if (n - k < tx.size()) rx[n] += h[k] * tx[n - k];
    }
  }
  return rx;
}
double max_err(const std::vector<cplx>& a, const std::vector<cplx>& b) {
  double e = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) e = std::max(e, std::abs(a[i] - b[i]));
  return e;
}
}  // namespace

TEST(Ofdm, SingleTapEqualisationThroughMultipath) {
  const std::size_t N = 16;
  std::vector<cplx> X(N);
  const auto d = dibit_stream(N, 8);
  for (std::size_t k = 0; k < N; ++k) X[k] = qpsk_map(d[k]);
  const std::vector<cplx> h = {cplx(0.9, 0.0), cplx(0.3, -0.2), cplx(0.0, 0.1)};  // 3 taps -> CP >= 2
  const auto tx = ofdm_modulate(X, 4);
  const auto rx = through_channel(tx, h, tx.size());
  const auto Y = ofdm_demodulate(rx, N, 4);
  EXPECT_LT(max_err(ofdm_equalise(Y, h), X), 1e-9);
  EXPECT_THROW(ofdm_demodulate(std::vector<cplx>(10), N, 4), std::invalid_argument);
}

TEST(Ofdm, TooShortPrefixBreaksOrthogonality) {
  const std::size_t N = 16;
  std::vector<cplx> X(N);
  const auto d = dibit_stream(N, 8);
  for (std::size_t k = 0; k < N; ++k) X[k] = qpsk_map(d[k]);
  const std::vector<cplx> h = {cplx(0.9, 0.0), cplx(0.3, -0.2), cplx(0.0, 0.1)};
  const auto tx = ofdm_modulate(X, 1);  // channel needs CP >= 2
  const auto rx = through_channel(tx, h, tx.size());
  const auto Y = ofdm_demodulate(rx, N, 1);
  EXPECT_GT(max_err(ofdm_equalise(Y, h), X), 1e-3);
}

// ---------------- FM ----------------
TEST(Fm, RoundTripAndCarson) {
  std::vector<double> m(500);
  for (std::size_t n = 0; n < m.size(); ++n) m[n] = std::sin(2.0 * kPi * 0.01 * static_cast<double>(n));
  const auto z = fm_modulate(m, 2500.0, 48000.0);
  ASSERT_EQ(z.size(), m.size());
  const auto r = fm_demodulate(z, 2500.0, 48000.0);
  ASSERT_EQ(r.size(), m.size() - 1);
  for (std::size_t n = 0; n < r.size(); ++n) EXPECT_NEAR(r[n], m[n], 1e-9);
  EXPECT_DOUBLE_EQ(carson_bandwidth_hz(2500.0, 3000.0), 11000.0);
}
