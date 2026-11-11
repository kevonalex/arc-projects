#pragma once
// SOLUTION - worksheet 05, Q1, Q3-Q5, Q12.
#include <cmath>
#include <cstdint>
#include <vector>

#include "lmr/todo.hpp"

namespace lmrsys {

constexpr double kSpeedOfLightKmPerS = 299792.458;

inline double erlang_b(double erlangs, int channels) {
  double b = 1.0;
  for (int m = 1; m <= channels; ++m) {
    const double eb = erlangs * b;
    b = eb / (static_cast<double>(m) + eb);
  }
  return b;
}

inline double path_delay_us(double km) { return km / kSpeedOfLightKmPerS * 1e6; }

inline double symbol_period_us(double symbol_rate) { return 1e6 / symbol_rate; }

inline double max_delay_spread_km(double symbol_rate, double fraction) {
  return fraction * symbol_period_us(symbol_rate) / path_delay_us(1.0);
}

struct VoteSample {
  int receiver_id = 0;
  int corrected_errors = 0;
  bool crc_ok = false;
};

inline int vote(const std::vector<VoteSample>& samples) {
  const VoteSample* best = nullptr;
  for (const auto& s : samples) {
    if (!s.crc_ok) continue;
    if (best == nullptr || s.corrected_errors < best->corrected_errors ||
        (s.corrected_errors == best->corrected_errors && s.receiver_id < best->receiver_id)) {
      best = &s;
    }
  }
  return best == nullptr ? -1 : best->receiver_id;
}

inline double vote_failure_probability(double p, int n) {
  double r = 1.0;
  for (int i = 0; i < n; ++i) r *= p;
  return r;
}

inline double acquire_time_ms(double period_ms, double p_loss) {
  return period_ms / 2.0 + p_loss / (1.0 - p_loss) * period_ms;
}

inline int gcd_int(int a, int b) {
  while (b != 0) {
    const int t = a % b;
    a = b;
    b = t;
  }
  return a;
}

inline int lcm_int(int a, int b) { return a / gcd_int(a, b) * b; }

inline int transcode_alignment_ms(int in_frame_ms, int out_frame_ms) {
  return lcm_int(in_frame_ms, out_frame_ms);
}

inline int max_framing_delay_ms(int in_frame_ms, int out_frame_ms) {
  return in_frame_ms - gcd_int(in_frame_ms, out_frame_ms);
}

inline int samples_per_frame(int frame_ms, int sample_rate_hz) {
  return frame_ms * sample_rate_hz / 1000;
}

}  // namespace lmrsys
