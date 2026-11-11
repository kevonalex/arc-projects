#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "octave_builds.hpp"
#include "vector_io.hpp"

using namespace dsp;

// Skip (not fail) when the Octave vector file has not been generated yet.
#define LOAD_OR_SKIP(name, vec)                                                              \
  do {                                                                                       \
    if (!read_vector((name), (vec)))                                                         \
      GTEST_SKIP() << "missing " << vector_path(name) << " -- run: octave octave/gen_vectors.m"; \
  } while (0)

namespace {
std::vector<cplx> naive_dft(const std::vector<cplx>& x) {
  const std::size_t n = x.size();
  std::vector<cplx> X(n);
  for (std::size_t k = 0; k < n; ++k) {
    cplx acc(0, 0);
    for (std::size_t m = 0; m < n; ++m) acc += x[m] * std::polar(1.0, -2.0 * kPi * static_cast<double>(k * m % n) / static_cast<double>(n));
    X[k] = acc;
  }
  return X;
}

std::vector<double> lcg_uniform(std::size_t n, std::uint64_t seed) {  // same Park-Miller as the Octave script
  std::vector<double> u(n);
  std::uint64_t x = seed;
  for (std::size_t i = 0; i < n; ++i) {
    x = (16807ull * x) % 2147483647ull;
    u[i] = static_cast<double>(x) / 2147483647.0;
  }
  return u;
}
}  // namespace

// ================================================================ analytic tests (always run)

TEST(Fft, ImpulseGivesFlatSpectrum) {
  std::vector<cplx> x(16, cplx(0, 0));
  x[0] = 1.0;
  for (const auto& v : fft(x)) EXPECT_LT(std::abs(v - cplx(1, 0)), 1e-12);
}

TEST(Fft, ToneLandsInOneBin) {
  const std::size_t n = 32;
  std::vector<cplx> x(n);
  for (std::size_t i = 0; i < n; ++i) x[i] = std::polar(1.0, 2.0 * kPi * 5.0 * static_cast<double>(i) / n);
  const auto X = fft(x);
  for (std::size_t k = 0; k < n; ++k) {
    if (k == 5)
      EXPECT_LT(std::abs(X[k] - cplx(32, 0)), 1e-9);
    else
      EXPECT_LT(std::abs(X[k]), 1e-9) << k;
  }
}

TEST(Fft, MatchesNaiveDftAndParseval) {
  const auto u = lcg_uniform(128, 99);
  std::vector<cplx> x(64);
  for (std::size_t i = 0; i < 64; ++i) x[i] = cplx(u[i] - 0.5, u[64 + i] - 0.5);
  const auto X = fft(x);
  const auto R = naive_dft(x);
  double ex = 0.0, eX = 0.0;
  for (std::size_t k = 0; k < 64; ++k) {
    EXPECT_LT(std::abs(X[k] - R[k]), 1e-12) << k;
    ex += std::norm(x[k]);
    eX += std::norm(X[k]);
  }
  EXPECT_NEAR(eX, 64.0 * ex, 1e-9);
}

TEST(Fft, SmallSizesAndErrors) {
  EXPECT_EQ(fft({cplx(3, -1)}).size(), 1u);
  EXPECT_LT(std::abs(fft({cplx(3, -1)})[0] - cplx(3, -1)), 1e-15);
  const auto X2 = fft({cplx(1, 0), cplx(2, 0)});
  EXPECT_LT(std::abs(X2[0] - cplx(3, 0)), 1e-15);
  EXPECT_LT(std::abs(X2[1] - cplx(-1, 0)), 1e-15);
  EXPECT_THROW(fft(std::vector<cplx>(6)), std::invalid_argument);
  EXPECT_THROW(fft(std::vector<cplx>()), std::invalid_argument);
}

TEST(FirFilterTest, ImpulseResponseIsTheTaps) {
  const std::vector<double> h = {0.25, -0.5, 1.0, 0.125};
  FirFilter f(h);
  EXPECT_EQ(f.num_taps(), 4u);
  std::vector<double> x(8, 0.0);
  x[0] = 1.0;
  const auto y = f.process_block(x);
  for (std::size_t k = 0; k < h.size(); ++k) EXPECT_DOUBLE_EQ(y[k], h[k]);
  for (std::size_t k = h.size(); k < y.size(); ++k) EXPECT_DOUBLE_EQ(y[k], 0.0);
}

TEST(FirFilterTest, BlockSizeDoesNotMatter) {
  const auto u = lcg_uniform(100, 7);
  const std::vector<double> h = {0.1, 0.2, 0.3, 0.2, 0.1};
  FirFilter a(h), b(h), c(h);
  const auto ya = a.process_block(u);
  std::vector<double> yb;
  for (double v : u) yb.push_back(b.process(v));
  std::vector<double> yc;
  for (std::size_t i = 0; i < u.size();) {
    const std::size_t len = std::min<std::size_t>(1 + (i * 7) % 13, u.size() - i);  // irregular block sizes
    const auto part = c.process_block(std::vector<double>(u.begin() + static_cast<std::ptrdiff_t>(i), u.begin() + static_cast<std::ptrdiff_t>(i + len)));
    yc.insert(yc.end(), part.begin(), part.end());
    i += len;
  }
  ASSERT_EQ(ya.size(), yb.size());
  ASSERT_EQ(ya.size(), yc.size());
  for (std::size_t i = 0; i < ya.size(); ++i) {
    EXPECT_EQ(ya[i], yb[i]) << i;  // identical operations in identical order -> bit-identical
    EXPECT_EQ(ya[i], yc[i]) << i;
  }
}

TEST(FirFilterTest, MatchesDirectConvolutionAndReset) {
  const auto u = lcg_uniform(60, 3);
  const std::vector<double> h = {0.5, -0.25, 0.125, 0.0625, -0.03125};
  FirFilter f(h);
  const auto y = f.process_block(u);
  for (std::size_t n = 0; n < u.size(); ++n) {
    double acc = 0.0;
    for (std::size_t k = 0; k < h.size() && k <= n; ++k) acc += h[k] * u[n - k];
    EXPECT_NEAR(y[n], acc, 1e-14) << n;
  }
  f.reset();
  const auto y2 = f.process_block(u);
  for (std::size_t n = 0; n < y.size(); ++n) EXPECT_EQ(y[n], y2[n]);
  EXPECT_THROW(FirFilter(std::vector<double>{}), std::invalid_argument);
}

TEST(Fsk4, LevelsAndErrors) {
  const std::vector<int> lv = fsk4_levels({0, 1, 2, 3});  // 00 01 10 11
  ASSERT_EQ(lv.size(), 4u);
  EXPECT_EQ(lv[0], +1);
  EXPECT_EQ(lv[1], +3);
  EXPECT_EQ(lv[2], -1);
  EXPECT_EQ(lv[3], -3);
  EXPECT_THROW(fsk4_levels({4}), std::invalid_argument);
  EXPECT_THROW(fsk4_levels({-1}), std::invalid_argument);
}

TEST(Fsk4, ModulatorFrequencyAndEnvelope) {
  const std::size_t sps = 10;
  const double dev = 600.0, fs = 48000.0;
  const std::vector<int> dibits = {1, 1, 0, 2, 3};
  const auto z = fsk4_modulate(dibits, sps, dev, fs);
  ASSERT_EQ(z.size(), dibits.size() * sps);
  EXPECT_LT(std::abs(z[0] - cplx(1, 0)), 1e-15);
  const double expect_level[5] = {+3, +3, +1, -1, -3};
  for (std::size_t s = 0; s < dibits.size(); ++s) {
    for (std::size_t j = 0; j + 1 < sps; ++j) {
      const std::size_t n = s * sps + j;
      EXPECT_NEAR(std::abs(z[n]), 1.0, 1e-12);
      const double dphi = std::arg(z[n + 1] * std::conj(z[n]));
      EXPECT_NEAR(dphi, 2.0 * kPi * expect_level[s] * dev / fs, 1e-9) << "sym " << s << " j " << j;
    }
  }
  // the first dibit '01' (+3 -> 1800 Hz) advances 2*pi*1800/48000 = 0.2356 rad per sample
  EXPECT_NEAR(std::arg(z[1]), 0.235619449, 1e-8);
}

TEST(Sync, DistancesAndDetection) {
  // P25 frame sync pattern 0x5575F5FF77FF (VERIFY against TIA-102.BAAA)
  const std::uint64_t hex = 0x5575F5FF77FFull;
  std::vector<int> word(48);
  for (int i = 0; i < 48; ++i) word[static_cast<std::size_t>(i)] = static_cast<int>((hex >> (47 - i)) & 1u);
  std::vector<int> rx(150, 0);
  for (std::size_t i = 0; i < 48; ++i) rx[30 + i] = word[i];
  const auto d0 = sync_hamming_distances(rx, word);
  ASSERT_EQ(d0.size(), 150u - 48u + 1u);
  EXPECT_EQ(d0[30], 0);
  EXPECT_EQ(sync_detect(d0, 0), std::vector<std::size_t>{30});  // only an exact match
  // flip two bits: distance 2 at lag 30, no exact match any more, still found with a 2-error budget
  rx[30 + 4] ^= 1;
  rx[30 + 29] ^= 1;
  const auto d2 = sync_hamming_distances(rx, word);
  EXPECT_EQ(d2[30], 2);
  EXPECT_TRUE(sync_detect(d2, 0).empty());
  const auto found = sync_detect(d2, 2);
  EXPECT_TRUE(std::find(found.begin(), found.end(), 30u) != found.end());
  // brute-force cross-check
  for (std::size_t l = 0; l < d2.size(); ++l) {
    int e = 0;
    for (std::size_t i = 0; i < 48; ++i) e += rx[l + i] != word[i];
    EXPECT_EQ(d2[l], e) << l;
  }
  EXPECT_TRUE(sync_hamming_distances(std::vector<int>(10, 0), word).empty());
  EXPECT_TRUE(sync_hamming_distances(rx, {}).empty());
}

TEST(Cfo, NoiselessEstimateAndCorrection) {
  const std::size_t n = 128;
  const double f0 = 0.0123, theta = 0.7;
  const auto u = lcg_uniform(n, 5);
  std::vector<double> known(n);
  std::vector<cplx> rx(n);
  for (std::size_t i = 0; i < n; ++i) {
    known[i] = u[i] < 0.5 ? -1.0 : 1.0;
    rx[i] = known[i] * std::polar(1.0, 2.0 * kPi * f0 * static_cast<double>(i) + theta);
  }
  EXPECT_NEAR(estimate_cfo(rx, known), f0, 1e-12);
  const auto out = correct_cfo(rx, estimate_cfo(rx, known));
  for (std::size_t i = 0; i < n; ++i) EXPECT_LT(std::abs(out[i] - known[i] * std::polar(1.0, theta)), 1e-9) << i;
  EXPECT_THROW(estimate_cfo(rx, std::vector<double>(3, 1.0)), std::invalid_argument);
}

TEST(Cfo, NegativeOffsetAndNoise) {
  const std::size_t n = 128;
  const double f0 = -0.3;
  const auto u = lcg_uniform(3 * n, 11);
  std::vector<double> known(n);
  std::vector<cplx> rx(n);
  for (std::size_t i = 0; i < n; ++i) {
    known[i] = u[i] < 0.5 ? -1.0 : 1.0;
    rx[i] = known[i] * std::polar(1.0, 2.0 * kPi * f0 * static_cast<double>(i) + 1.0) +
            cplx(0.02 * (u[n + i] - 0.5), 0.02 * (u[2 * n + i] - 0.5));
  }
  EXPECT_NEAR(estimate_cfo(rx, known), f0, 1e-3);
}

TEST(Qpsk, TheoryValues) {
  EXPECT_NEAR(qpsk_ber_theory(0.0), 0.0786496, 1e-6);   // Q(sqrt 2)
  EXPECT_NEAR(qpsk_ber_theory(4.0), 0.0125008, 5e-5);   // Q(sqrt(2*2.5119))
  EXPECT_NEAR(qpsk_ber_theory(10.0), 3.872e-6, 5e-8);
}

TEST(Qpsk, SimulationMatchesTheory) {
  for (double ebn0 : {2.0, 4.0, 6.0}) {
    const double sim = simulate_qpsk_ber(ebn0, 2000000, 2024);
    EXPECT_NEAR(sim / qpsk_ber_theory(ebn0), 1.0, 0.10) << "Eb/N0 = " << ebn0 << " dB, sim = " << sim;
  }
}

TEST(Q15Fir, KnownValuesAndSaturation) {
  EXPECT_EQ(q15_fir({100, -200, 300}, {16384}), (std::vector<std::int16_t>{50, -100, 150}));
  const std::vector<std::int16_t> h(4, 32767), x(4, 32767);
  const auto y = q15_fir(x, h);
  EXPECT_EQ(y[0], 32766);
  EXPECT_EQ(y[1], 32767);  // saturated
  EXPECT_EQ(y[3], 32767);
  const auto yn = q15_fir(std::vector<std::int16_t>(4, -32768), h);
  EXPECT_EQ(yn[3], -32768);
}

// ================================================================ Octave-vector tests (skip if files missing)

TEST(OctaveVectors, FftMatches) {
  std::vector<double> xr, xi, Xr, Xi;
  LOAD_OR_SKIP("fft_in_re.txt", xr);
  LOAD_OR_SKIP("fft_in_im.txt", xi);
  LOAD_OR_SKIP("fft_out_re.txt", Xr);
  LOAD_OR_SKIP("fft_out_im.txt", Xi);
  const auto X = fft(merge_cplx(xr, xi));
  ASSERT_EQ(X.size(), Xr.size());
  // Float results of two different FFT implementations are NOT bit-identical; error ~ eps * |X| * log2(N)
  // ~ 1e-16 * 3 * 6 = 2e-15 here, so 1e-12 leaves ~500x margin yet still catches float-precision twiddles (~1e-7).
  for (std::size_t k = 0; k < X.size(); ++k) {
    EXPECT_NEAR(X[k].real(), Xr[k], 1e-12) << k;
    EXPECT_NEAR(X[k].imag(), Xi[k], 1e-12) << k;
  }
}

TEST(OctaveVectors, FirMatches) {
  std::vector<double> h, x, y;
  LOAD_OR_SKIP("fir_taps.txt", h);
  LOAD_OR_SKIP("fir_in.txt", x);
  LOAD_OR_SKIP("fir_out.txt", y);
  FirFilter f(h);
  const auto mine = f.process_block(x);
  ASSERT_EQ(mine.size(), y.size());
  for (std::size_t i = 0; i < y.size(); ++i) EXPECT_NEAR(mine[i], y[i], 1e-12) << i;
}

TEST(OctaveVectors, Fsk4Matches) {
  std::vector<int> dibits, lev;
  std::vector<double> re, im;
  if (!read_int_vector("fsk4_dibits.txt", dibits) || !read_int_vector("fsk4_levels.txt", lev) ||
      !read_vector("fsk4_out_re.txt", re) || !read_vector("fsk4_out_im.txt", im))
    GTEST_SKIP() << "missing fsk4 vectors -- run: octave octave/gen_vectors.m";
  EXPECT_EQ(fsk4_levels(dibits), lev);  // integers: bit-exact
  const auto z = fsk4_modulate(dibits, 10, 600.0, 48000.0);
  ASSERT_EQ(z.size(), re.size());
  for (std::size_t n = 0; n < z.size(); ++n) {
    EXPECT_NEAR(z[n].real(), re[n], 1e-9) << n;
    EXPECT_NEAR(z[n].imag(), im[n], 1e-9) << n;
  }
}

TEST(OctaveVectors, SyncDistancesBitExact) {
  std::vector<int> word, rx, dist;
  if (!read_int_vector("sync_word.txt", word) || !read_int_vector("sync_rx.txt", rx) ||
      !read_int_vector("sync_dist.txt", dist))
    GTEST_SKIP() << "missing sync vectors -- run: octave octave/gen_vectors.m";
  const auto mine = sync_hamming_distances(rx, word);
  EXPECT_EQ(mine, dist);  // integer path: must be bit-exact
  const auto found = sync_detect(mine, 4);
  EXPECT_TRUE(std::find(found.begin(), found.end(), 100u) != found.end());  // embedded at 0-based lag 100 with 2 errors
}

TEST(OctaveVectors, CfoMatches) {
  std::vector<double> known, re, im, est, ore, oim;
  LOAD_OR_SKIP("cfo_known.txt", known);
  LOAD_OR_SKIP("cfo_in_re.txt", re);
  LOAD_OR_SKIP("cfo_in_im.txt", im);
  LOAD_OR_SKIP("cfo_est.txt", est);
  LOAD_OR_SKIP("cfo_out_re.txt", ore);
  LOAD_OR_SKIP("cfo_out_im.txt", oim);
  const auto rx = merge_cplx(re, im);
  const double f = estimate_cfo(rx, known);
  EXPECT_NEAR(f, est[0], 1e-12);
  EXPECT_NEAR(f, 0.0123, 1e-3);  // sanity: the generator used f0 = 0.0123
  const auto out = correct_cfo(rx, est[0]);
  for (std::size_t n = 0; n < out.size(); ++n) {
    EXPECT_NEAR(out[n].real(), ore[n], 1e-9) << n;
    EXPECT_NEAR(out[n].imag(), oim[n], 1e-9) << n;
  }
}

TEST(OctaveVectors, Q15FirBitExact) {
  std::vector<int> x, h, y;
  if (!read_int_vector("q15_x.txt", x) || !read_int_vector("q15_h.txt", h) || !read_int_vector("q15_y.txt", y))
    GTEST_SKIP() << "missing q15 vectors -- run: octave octave/gen_vectors.m";
  const std::vector<std::int16_t> x16(x.begin(), x.end()), h16(h.begin(), h.end());
  const auto mine = q15_fir(x16, h16);
  ASSERT_EQ(mine.size(), y.size());
  for (std::size_t i = 0; i < y.size(); ++i) EXPECT_EQ(static_cast<int>(mine[i]), y[i]) << i;  // zero tolerance
  EXPECT_EQ(mine[3], 32767);  // the saturation case is really exercised
}

TEST(OctaveVectors, BerTheoryAndSimulation) {
  std::vector<double> t;
  LOAD_OR_SKIP("ber_theory.txt", t);
  ASSERT_EQ(t.size() % 2, 0u);
  for (std::size_t i = 0; i + 1 < t.size(); i += 2) {
    const double ebn0 = t[i], ber = t[i + 1];
    EXPECT_NEAR(qpsk_ber_theory(ebn0), ber, 1e-12) << ebn0;
    if (ebn0 <= 6.0) {  // enough errors in 2e6 bits for a 10 % statistical check
      const double sim = simulate_qpsk_ber(ebn0, 2000000, 77 + static_cast<std::uint64_t>(i));
      EXPECT_NEAR(sim / ber, 1.0, 0.10) << ebn0;
    }
  }
}
