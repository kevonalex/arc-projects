// Tests for Capstone M3. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "modem.hpp"

using namespace gw;

namespace {
std::vector<uint8_t> random_bits(size_t n, uint64_t seed) {
  Rng r(seed);
  std::vector<uint8_t> b(n);
  for (auto& x : b) x = static_cast<uint8_t>(r.next() >> 63);
  return b;
}
// A synthetic "frame": the 24 sync bits followed by random bits.
std::vector<uint8_t> fake_frame(size_t total_bits, uint64_t seed) {
  std::vector<uint8_t> b;
  for (int i = kSyncBits - 1; i >= 0; --i) b.push_back(static_cast<uint8_t>((kSyncWord >> i) & 1u));
  auto rest = random_bits(total_bits - kSyncBits, seed);
  b.insert(b.end(), rest.begin(), rest.end());
  return b;
}
}  // namespace

// ---------------------------------------------------------------- mapping + modulation (W 3.1 - 3.2)
TEST(Mapping, GrayCodeAndRoundTrip) {
  EXPECT_EQ(bits_to_symbols({0, 0, 0, 1, 1, 1, 1, 0}), (std::vector<uint8_t>{0, 1, 2, 3}));
  EXPECT_EQ(symbols_to_bits({0, 1, 2, 3}), (std::vector<uint8_t>{0, 0, 0, 1, 1, 1, 1, 0}));
  EXPECT_EQ(bits_to_symbols({1, 0, 1}).size(), 1u);  // odd trailing bit ignored
  auto b = random_bits(200, 3);
  EXPECT_EQ(symbols_to_bits(bits_to_symbols(b)), b);
  // Gray property: neighbouring tones differ in exactly one bit, so the likeliest error (adjacent tone) costs 1 bit.
  for (uint8_t s = 0; s < 3; ++s) {
    auto x = symbols_to_bits({s});
    auto y = symbols_to_bits({static_cast<uint8_t>(s + 1)});
    ASSERT_EQ(x.size(), 2u);
    ASSERT_EQ(y.size(), 2u);
    EXPECT_EQ((x[0] != y[0]) + (x[1] != y[1]), 1);
  }
}
TEST(Modulate, ConstantEnvelopeAndLength) {
  auto x = modulate({0, 1, 2, 3, 3, 0});
  ASSERT_EQ(x.size(), 6u * kSps);
  for (const cd& v : x) EXPECT_NEAR(std::abs(v), 1.0, 1e-12);
}
TEST(Modulate, PhaseIsContinuousAcrossSymbolBoundaries) {
  const std::vector<uint8_t> syms = {0, 3, 1, 2, 0, 3};
  auto x = modulate(syms);
  ASSERT_EQ(x.size(), syms.size() * kSps);
  for (size_t n = 0; n + 1 < x.size(); ++n) {
    const double want = 2.0 * kPi * tone_cycles_per_symbol(syms[n / kSps]) / kSps;  // increment applied leaving sample n
    double got = std::arg(x[n + 1] * std::conj(x[n]));
    double diff = std::remainder(got - want, 2.0 * kPi);
    EXPECT_NEAR(diff, 0.0, 1e-9) << "n=" << n;  // no phase jump at the symbol edge: narrow spectrum
  }
}
TEST(Demod, TonesAreOrthogonalOverOneSymbol) {
  for (int a = 0; a < 4; ++a) {
    auto x = modulate({static_cast<uint8_t>(a)});
    ASSERT_EQ(x.size(), static_cast<size_t>(kSps));
    double e[4];
    tone_energies(x.data(), kSps, e);
    for (int k = 0; k < 4; ++k) EXPECT_NEAR(e[k], k == a ? double(kSps * kSps) : 0.0, 1e-9);
  }
}
TEST(Demod, NoiselessRoundTrip) {
  auto syms = bits_to_symbols(random_bits(2000, 5));
  ASSERT_EQ(syms.size(), 1000u);
  auto iq = modulate(syms);
  ASSERT_EQ(iq.size(), syms.size() * kSps);
  EXPECT_EQ(demodulate(iq, 0, syms.size()), syms);
  EXPECT_EQ(demodulate(iq, 0, syms.size() + 50).size(), syms.size());  // stops at the end of the buffer
  EXPECT_EQ(demod_symbol(&iq[0]), syms[0]);
}

// ---------------------------------------------------------------- noise and CFO (W 3.3)
TEST(Channel, NoiseSigmaFormula) {
  EXPECT_NEAR(noise_sigma_per_component(0.0), std::sqrt(0.5 * 8), 1e-12);
  EXPECT_NEAR(noise_sigma_per_component(10.0), std::sqrt(0.5 * 8 / 10.0), 1e-12);
  EXPECT_NEAR(noise_sigma_per_component(20.0, 4), std::sqrt(0.5 * 4 / 100.0), 1e-12);
}
TEST(Channel, MeasuredNoisePowerMatchesEsN0) {
  Rng rng(11);
  std::vector<cd> z(200000, cd(0, 0));
  add_awgn(z, 6.0, rng);
  double p = 0;
  for (const cd& v : z) p += std::norm(v);
  p /= static_cast<double>(z.size());
  EXPECT_NEAR(p, kSps / std::pow(10.0, 0.6), 0.03 * kSps / std::pow(10.0, 0.6));  // total complex variance within 3 %
}
TEST(Channel, CfoRotatesPhaseLinearly) {
  std::vector<cd> x(16, cd(1, 0));
  apply_cfo(x, 0.5, 8);  // half a cycle per symbol = pi/8 rad per sample
  for (size_t n = 0; n < x.size(); ++n) {
    EXPECT_NEAR(x[n].real(), std::cos(kPi / 8 * static_cast<double>(n)), 1e-12);
    EXPECT_NEAR(x[n].imag(), std::sin(kPi / 8 * static_cast<double>(n)), 1e-12);
  }
}
TEST(Channel, CfoBelowHalfToneSpacingIsHarmlessAboveItBreaksEverything) {
  auto bits = random_bits(4000, 21);
  auto syms = bits_to_symbols(bits);
  Rng rng(1);
  auto iq = modulate(syms);
  apply_cfo(iq, 0.2, kSps);  // 20 % of the tone spacing
  EXPECT_EQ(demodulate(iq, 0, syms.size()), syms);
  iq = modulate(syms);
  apply_cfo(iq, 0.6, kSps);  // more than half the tone spacing: every tone now looks like its neighbour
  auto got = demodulate(iq, 0, syms.size());
  ASSERT_EQ(got.size(), syms.size());
  size_t wrong = 0;
  for (size_t i = 0; i < syms.size(); ++i) wrong += got[i] != syms[i];
  EXPECT_GT(wrong, syms.size() * 6 / 10);  // ~3/4 of symbols slide to the next tone up
}

// ---------------------------------------------------------------- BER against theory (W 3.7)
TEST(Ber, TheoryValues) {
  EXPECT_NEAR(ber_4fsk_noncoherent(8.0), 0.0342, 5e-4);
  EXPECT_NEAR(ber_4fsk_noncoherent(12.0), 3.46e-4, 2e-5);
  EXPECT_NEAR(ber_4fsk_noncoherent(0.0), 0.343, 0.005);  // hopeless but < 0.5
  EXPECT_LT(ber_4fsk_noncoherent(12.0), ber_4fsk_noncoherent(8.0));
}
TEST(Ber, ConversionsAndMeasurementBudget) {
  EXPECT_NEAR(esn0_to_ebn0_db(10.0), 6.9897, 1e-3);
  EXPECT_NEAR(ebn0_to_esn0_db(6.9897), 10.0, 1e-3);
  EXPECT_DOUBLE_EQ(bits_needed_for_ber(1e-3), 1e5);
  EXPECT_DOUBLE_EQ(bits_needed_for_ber(1e-6), 1e8);
  EXPECT_DOUBLE_EQ(bits_needed_for_ber(1e-2, 50), 5000.0);
}
TEST(Ber, CountErrors) {
  EXPECT_EQ(count_bit_errors({0, 1, 1, 0}, {0, 1, 1, 0}), 0u);
  EXPECT_EQ(count_bit_errors({0, 1, 1, 0}, {1, 1, 0, 0}), 2u);
  EXPECT_EQ(count_bit_errors({1, 1, 1}, {1, 1, 1, 0, 0}), 2u);  // missing bits are errors
}
TEST(Ber, SimulatedBerTracksTheoryWithinStatisticalNoise) {
  for (double esn0 : {6.0, 9.0}) {
    auto bits = random_bits(40000, 100 + static_cast<uint64_t>(esn0));
    auto syms = bits_to_symbols(bits);
    Rng rng(777);
    auto iq = modulate(syms);
    add_awgn(iq, esn0, rng);
    auto rx = symbols_to_bits(demodulate(iq, 0, syms.size()));
    const double ber = static_cast<double>(count_bit_errors(bits, rx)) / static_cast<double>(bits.size());
    const double theory = ber_4fsk_noncoherent(esn0);
    EXPECT_GT(ber, 0.8 * theory) << "Es/N0 = " << esn0;
    EXPECT_LT(ber, 1.25 * theory) << "Es/N0 = " << esn0;
  }
}

// ---------------------------------------------------------------- sync + CFO estimation (W 3.4 - 3.5)
TEST(Sync, SymbolsAndPerfectMatch) {
  auto s = sync_symbols();
  ASSERT_EQ(s.size(), 12u);
  auto iq = modulate(s);
  EXPECT_NEAR(sync_metric(iq, 0, s), 1.0, 1e-9);
}
TEST(Sync, FindsFrameStartThroughDelayNoiseAndCfo) {
  auto frame = fake_frame(400, 9);
  auto tx = transmit(frame);
  Rng rng(5);
  auto rx = channel(tx, 10.0, 0.1, 37, rng);
  double peak = 0;
  long start = find_frame_start(rx, sync_symbols(), 0.5, &peak);
  EXPECT_GE(start, 36);
  EXPECT_LE(start, 38);
  EXPECT_GT(peak, 0.6);
}
TEST(Sync, NoiseAloneDoesNotTrigger) {
  Rng rng(8);
  std::vector<cd> noise(3000, cd(0, 0));
  add_awgn(noise, 10.0, rng);
  double peak = 0;
  EXPECT_EQ(find_frame_start(noise, sync_symbols(), 0.5, &peak), -1);
  EXPECT_LT(peak, 0.5);
}
TEST(Sync, CfoEstimateIsAccurate) {
  auto sync = sync_symbols();
  for (double cfo : {-0.15, 0.0, 0.12, 0.3}) {
    auto tx = modulate(sync);
    Rng rng(2);
    auto rx = channel(tx, 15.0, cfo, 0, rng);
    EXPECT_NEAR(estimate_cfo(rx, 0, sync), cfo, 0.03) << "true cfo " << cfo;
  }
}

// ---------------------------------------------------------------- whole chain (W 3.6)
TEST(Chain, ErrorFreeAtHighSnrWithCfoAndDelay) {
  const size_t n = 2338;  // the Capstone M1 frame length
  auto frame = fake_frame(n, 31);
  Rng rng(6);
  auto rx = channel(transmit(frame), 16.0, 0.12, 53, rng);
  auto r = receive(rx, n);
  ASSERT_TRUE(r.found);
  EXPECT_NEAR(static_cast<double>(r.start), 53.0, 1.0);
  EXPECT_NEAR(r.cfo, 0.12, 0.03);
  ASSERT_EQ(r.bits.size(), n);
  EXPECT_EQ(count_bit_errors(r.bits, frame), 0u);
}
TEST(Chain, ReceiverReportsNoFrameInNoise) {
  Rng rng(9);
  std::vector<cd> noise(5000, cd(0, 0));
  add_awgn(noise, 10.0, rng);
  EXPECT_FALSE(receive(noise, 2338).found);
}
TEST(Chain, BerAtLowSnrIsWithinAFactorOfTheory) {
  const size_t n = 2338;
  size_t errs = 0, total = 0;
  Rng rng(12);
  for (int f = 0; f < 15; ++f) {
    auto frame = fake_frame(n, 500 + static_cast<uint64_t>(f));
    auto rx = channel(transmit(frame), 10.0, 0.05, 20, rng);
    auto r = receive(rx, n, 0.4);
    ASSERT_TRUE(r.found) << "frame " << f;
    errs += count_bit_errors(r.bits, frame);
    total += n;
  }
  const double ber = static_cast<double>(errs) / static_cast<double>(total);
  EXPECT_LT(ber, 3.0 * ber_4fsk_noncoherent(10.0) + 1e-4);
  EXPECT_GT(ber, 0.2 * ber_4fsk_noncoherent(10.0));
}
