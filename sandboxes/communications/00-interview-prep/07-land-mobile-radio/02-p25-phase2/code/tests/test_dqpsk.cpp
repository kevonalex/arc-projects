#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <vector>

#include "p25p2/dqpsk.hpp"

using namespace p25p2;

TEST(Dqpsk, StepTable) {
  EXPECT_NEAR(dibit_to_phase_step(0), kPi / 4, 1e-12);
  EXPECT_NEAR(dibit_to_phase_step(1), 3 * kPi / 4, 1e-12);
  EXPECT_NEAR(dibit_to_phase_step(2), -kPi / 4, 1e-12);
  EXPECT_NEAR(dibit_to_phase_step(3), -3 * kPi / 4, 1e-12);
  EXPECT_NEAR(dibit_to_phase_step(0xFD), 3 * kPi / 4, 1e-12);  // only low 2 bits matter (0xFD & 3 == 1)
}

TEST(Dqpsk, NoisyStepsSliceToNearest) {
  EXPECT_EQ(phase_step_to_dibit(kPi / 4 + 0.3), 0);
  EXPECT_EQ(phase_step_to_dibit(3 * kPi / 4 - 0.3), 1);
  EXPECT_EQ(phase_step_to_dibit(-kPi / 4 - 0.3), 2);
  EXPECT_EQ(phase_step_to_dibit(-3 * kPi / 4 + 0.3), 3);
  // an unwrapped step (a whole turn too big) must wrap back to the same dibit
  EXPECT_EQ(phase_step_to_dibit(kPi / 4 + 2 * kPi), 0);
}

TEST(Dqpsk, RoundTrip) {
  std::mt19937 rng(12345);
  std::vector<std::uint8_t> dibits(500);
  for (auto& d : dibits) d = static_cast<std::uint8_t>(rng() & 3);
  const auto phases = differential_encode(dibits, 0.0);
  ASSERT_EQ(phases.size(), dibits.size());
  EXPECT_EQ(differential_decode(phases, 0.0), dibits);
}

TEST(Dqpsk, PhasesStayWrapped) {
  std::vector<std::uint8_t> dibits(100, 1);  // +3pi/4 every symbol: must wrap
  for (double p : differential_encode(dibits)) {
    EXPECT_GT(p, -kPi - 1e-12);
    EXPECT_LE(p, kPi + 1e-12);
  }
}

TEST(Dqpsk, ConstantPhaseOffsetIsInvisible) {
  std::mt19937 rng(777);
  std::vector<std::uint8_t> dibits(300);
  for (auto& d : dibits) d = static_cast<std::uint8_t>(rng() & 3);

  // Transmit a known first (reference) symbol followed by data.
  std::vector<std::uint8_t> with_ref{0};
  with_ref.insert(with_ref.end(), dibits.begin(), dibits.end());

  for (double offset : {0.0, 0.7, -1.9, 3.0}) {
    auto phases = differential_encode(with_ref, 0.0);
    for (double& p : phases) p = wrap_phase(p + offset);  // the channel rotates everything
    EXPECT_EQ(differential_decode_blind(phases), dibits) << "offset " << offset;
  }
}

TEST(Dqpsk, PiOver4Property_NoPhaseOfExactlyPi) {
  // Every step is an ODD multiple of pi/4, so the absolute phase alternates between the two
  // QPSK constellations (0, pi/2, ...) and (pi/4, 3pi/4, ...).
  std::mt19937 rng(99);
  std::vector<std::uint8_t> dibits(200);
  for (auto& d : dibits) d = static_cast<std::uint8_t>(rng() & 3);
  const auto phases = differential_encode(dibits, 0.0);
  for (std::size_t i = 0; i < phases.size(); ++i) {
    const int n = static_cast<int>(i) + 1;  // number of steps taken
    if (n % 2 == 0) {
      EXPECT_NEAR(std::sin(2 * phases[i]), 0.0, 1e-9) << "i=" << i;  // multiple of pi/2
    } else {
      EXPECT_NEAR(std::cos(2 * phases[i]), 0.0, 1e-9) << "i=" << i;  // odd multiple of pi/4
    }
  }
}

TEST(Dqpsk, BlindDecodeShortInput) {
  EXPECT_TRUE(differential_decode_blind({}).empty());
  EXPECT_TRUE(differential_decode_blind({1.0}).empty());
}
