#include <gtest/gtest.h>

#include <cmath>

#include "pulse_shaping.hpp"

using namespace dsp;

namespace {
std::vector<double> bpsk(std::size_t n, unsigned seed = 3) {
  std::vector<double> v(n);
  unsigned s = seed;
  for (auto& x : v) {
    s = s * 1664525u + 1013904223u;
    x = ((s >> 16) & 1u) ? 1.0 : -1.0;
  }
  return v;
}
std::vector<double> convolve(const std::vector<double>& a, const std::vector<double>& b) {
  std::vector<double> y(a.size() + b.size() - 1, 0.0);
  for (std::size_t i = 0; i < a.size(); ++i)
    for (std::size_t j = 0; j < b.size(); ++j) y[i + j] += a[i] * b[j];
  return y;
}
}  // namespace

TEST(RaisedCosine, ZeroIsiAndKnownValues) {
  const std::size_t sps = 8, span = 8;
  const auto h = raised_cosine(0.35, sps, span);
  ASSERT_EQ(h.size(), 2 * span * sps + 1);
  const std::size_t c = span * sps;
  EXPECT_NEAR(h[c], 1.0, 1e-12);
  for (int m = 1; m <= 7; ++m) {
    EXPECT_NEAR(h[c + static_cast<std::size_t>(m) * sps], 0.0, 1e-12) << "m=" << m;
    EXPECT_NEAR(h[c - static_cast<std::size_t>(m) * sps], 0.0, 1e-12) << "m=" << m;
  }
  EXPECT_NEAR(h[c + sps / 2], 0.6186, 1e-3);  // h(0.5T) for beta = 0.35
  for (std::size_t i = 0; i < h.size(); ++i) EXPECT_NEAR(h[i], h[h.size() - 1 - i], 1e-12);
}

TEST(RaisedCosine, SingularPointIsFinite) {
  // beta = 0.5: singular at t = T/(2*0.5) = T  -> value 0 (zero-ISI point). beta = 0.25: t = 2T.
  for (double beta : {0.5, 0.25, 1.0}) {
    const auto h = raised_cosine(beta, 8, 8);
    for (double v : h) EXPECT_TRUE(std::isfinite(v)) << "beta=" << beta;
  }
  const auto h = raised_cosine(0.5, 8, 8);
  EXPECT_NEAR(h[8 * 8 + 8], 0.0, 1e-9);
}

TEST(RaisedCosine, BetaZeroIsSinc) {
  const auto h = raised_cosine(0.0, 8, 4);
  const std::size_t c = 4 * 8;
  EXPECT_NEAR(h[c + 4], std::sin(kPi * 0.5) / (kPi * 0.5), 1e-12);
}

TEST(Rrc, UnitEnergyAndSymmetry) {
  const auto h = root_raised_cosine(0.35, 8, 8);
  double e = 0.0;
  for (double v : h) e += v * v;
  EXPECT_NEAR(e, 1.0, 1e-12);
  for (std::size_t i = 0; i < h.size(); ++i) EXPECT_NEAR(h[i], h[h.size() - 1 - i], 1e-12);
}

TEST(Rrc, FiniteAtSpecialPoints) {
  // beta = 0.25 -> 1/(4 beta) = 1 symbol; beta = 0.5 -> 0.5 symbol (both land on samples for sps = 8)
  for (double beta : {0.25, 0.5, 0.2, 0.35}) {
    const auto h = root_raised_cosine(beta, 8, 8);
    for (double v : h) EXPECT_TRUE(std::isfinite(v)) << "beta=" << beta;
  }
}

TEST(Rrc, CascadeIsNyquist) {
  const std::size_t sps = 8;
  const auto h = root_raised_cosine(0.35, sps, 8);
  const auto pair = convolve(h, h);
  const std::size_t c = h.size() - 1;
  EXPECT_NEAR(pair[c], 1.0, 1e-9);                 // unit-energy pulse -> autocorrelation peak 1
  EXPECT_LT(isi_residual(pair, sps), 5e-3);        // truncation-limited
  // RRC alone is NOT zero-ISI
  EXPECT_GT(isi_residual(h, sps), 0.03);
}

TEST(Rrc, CascadeMatchesRaisedCosine) {
  const std::size_t sps = 8;
  const auto h = root_raised_cosine(0.35, sps, 8);
  const auto pair = convolve(h, h);
  const auto rc = raised_cosine(0.35, sps, 8);
  const std::size_t c = h.size() - 1, rc_c = 8 * sps;
  for (int j = -3 * static_cast<int>(sps); j <= 3 * static_cast<int>(sps); ++j) {
    EXPECT_NEAR(pair[static_cast<std::size_t>(static_cast<int>(c) + j)] / pair[c],
                rc[static_cast<std::size_t>(static_cast<int>(rc_c) + j)], 5e-3)
        << "j=" << j;
  }
}

TEST(PulseShape, SmallExample) {
  const auto y = pulse_shape({1.0, -1.0}, {1.0, 2.0, 3.0}, 2);
  const double expect[] = {1, 2, 2, -2, -3, 0};
  ASSERT_EQ(y.size(), 6u);
  for (int i = 0; i < 6; ++i) EXPECT_NEAR(y[static_cast<std::size_t>(i)], expect[i], 1e-12);
}

TEST(Eye, TracesFromRamp) {
  std::vector<double> y(100);
  for (std::size_t i = 0; i < y.size(); ++i) y[i] = static_cast<double>(i);
  const auto tr = eye_traces(y, 4, 1);
  ASSERT_EQ(tr.size(), 23u);
  ASSERT_EQ(tr[0].size(), 9u);
  EXPECT_DOUBLE_EQ(tr[0][0], 1.0);
  EXPECT_DOUBLE_EQ(tr[0][8], 9.0);
  EXPECT_DOUBLE_EQ(tr[1][0], 5.0);
}

TEST(Eye, OpenAtCorrectPhaseClosedAtHalfSymbol) {
  const std::size_t sps = 8, span = 8;
  const auto rc = raised_cosine(0.35, sps, span);
  const auto sym = bpsk(600);
  const auto y = pulse_shape(sym, rc, sps);
  const std::size_t delay = span * sps;  // centre tap delay
  const double good = eye_height(y, sps, delay, 12);
  const double bad = eye_height(y, sps, delay + sps / 2, 12);
  EXPECT_GT(good, 1.98);
  EXPECT_LT(good, 2.02);
  EXPECT_LT(bad, 1.0);
  // a quarter-symbol error is worse than 0 but still better than half a symbol
  const double mid = eye_height(y, sps, delay + sps / 4, 12);
  EXPECT_LT(mid, good);
  EXPECT_GT(mid, bad);
}

TEST(Eye, DegenerateInputs) {
  EXPECT_DOUBLE_EQ(eye_height({}, 4, 0, 0), 0.0);
  EXPECT_DOUBLE_EQ(eye_height({1.0, 1.0, 1.0, 1.0, 1.0}, 1, 0, 0), 0.0);  // no negative rail
}
