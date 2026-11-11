#pragma once
// SOLUTION - worksheet 02, Q5/Q10.
#include <cstddef>
#include <cstdint>
#include <vector>

#include "lmr/todo.hpp"

namespace p25p2 {

inline constexpr double kPi = 3.14159265358979323846;

inline double wrap_phase(double x) {
  while (x > kPi) x -= 2.0 * kPi;
  while (x <= -kPi) x += 2.0 * kPi;
  return x;
}

inline double dibit_to_phase_step(std::uint8_t dibit) {
  switch (dibit & 0x3) {
    case 0: return kPi / 4.0;
    case 1: return 3.0 * kPi / 4.0;
    case 2: return -kPi / 4.0;
    default: return -3.0 * kPi / 4.0;
  }
}

inline std::uint8_t phase_step_to_dibit(double step) {
  step = wrap_phase(step);
  if (step >= 0.0) return step < kPi / 2.0 ? 0 : 1;
  return step > -kPi / 2.0 ? 2 : 3;
}

inline std::vector<double> differential_encode(const std::vector<std::uint8_t>& dibits,
                                               double start_phase = 0.0) {
  std::vector<double> out;
  out.reserve(dibits.size());
  double ph = start_phase;
  for (std::uint8_t d : dibits) {
    ph = wrap_phase(ph + dibit_to_phase_step(d));
    out.push_back(ph);
  }
  return out;
}

inline std::vector<std::uint8_t> differential_decode(const std::vector<double>& phases,
                                                     double start_phase = 0.0) {
  std::vector<std::uint8_t> out;
  out.reserve(phases.size());
  double prev = start_phase;
  for (double p : phases) {
    out.push_back(phase_step_to_dibit(p - prev));
    prev = p;
  }
  return out;
}

inline std::vector<std::uint8_t> differential_decode_blind(const std::vector<double>& phases) {
  std::vector<std::uint8_t> out;
  if (phases.size() < 2) return out;
  out.reserve(phases.size() - 1);
  for (std::size_t i = 1; i < phases.size(); ++i) {
    out.push_back(phase_step_to_dibit(phases[i] - phases[i - 1]));
  }
  return out;
}

}  // namespace p25p2
