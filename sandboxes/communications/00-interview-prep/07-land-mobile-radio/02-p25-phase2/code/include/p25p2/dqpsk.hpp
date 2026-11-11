#pragma once
// STUB - worksheet 02, Q5/Q10 : differential QPSK (pi/4 family), the idea behind the Phase 2
// downlink (H-DQPSK).  TEACHING mapping below; the exact Phase 2 dibit->phase table is
// VERIFY TIA-102.BBAB.
//
//   dibit 00 -> +pi/4     dibit 01 -> +3pi/4     dibit 10 -> -pi/4     dibit 11 -> -3pi/4
#include <cstdint>
#include <vector>

#include "lmr/todo.hpp"

namespace p25p2 {

inline constexpr double kPi = 3.14159265358979323846;

// GIVEN: wrap an angle into (-pi, pi].
inline double wrap_phase(double x) {
  while (x > kPi) x -= 2.0 * kPi;
  while (x <= -kPi) x += 2.0 * kPi;
  return x;
}

// TODO: phase CHANGE for a dibit (use the low 2 bits only).
inline double dibit_to_phase_step(std::uint8_t /*dibit*/) { lmr::todo("dibit_to_phase_step"); }

// TODO: nearest dibit for a (possibly noisy) phase change. step is wrapped first.
inline std::uint8_t phase_step_to_dibit(double /*step*/) { lmr::todo("phase_step_to_dibit"); }

// TODO: accumulate phase steps: phase[i] = wrap(phase[i-1] + step(dibit[i])), phase[-1] = start_phase.
inline std::vector<double> differential_encode(const std::vector<std::uint8_t>& /*dibits*/,
                                               double /*start_phase*/ = 0.0) {
  lmr::todo("differential_encode");
}

// TODO: inverse, given the reference phase that preceded phases[0].
inline std::vector<std::uint8_t> differential_decode(const std::vector<double>& /*phases*/,
                                                     double /*start_phase*/ = 0.0) {
  lmr::todo("differential_decode");
}

// TODO: decode WITHOUT knowing the absolute phase: treat phases[0] as the reference and return
//       phases.size()-1 dibits.  A constant phase offset on every sample must not change the result.
inline std::vector<std::uint8_t> differential_decode_blind(const std::vector<double>& /*phases*/) {
  lmr::todo("differential_decode_blind");
}

}  // namespace p25p2
