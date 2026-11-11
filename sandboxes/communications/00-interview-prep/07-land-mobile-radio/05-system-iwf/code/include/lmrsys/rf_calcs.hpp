#pragma once
// STUB - worksheet 05, Q1, Q3-Q5, Q12 : the arithmetic behind trunking gain, simulcast, voting,
// late entry and transcoding. All inputs are numbers or synthetic samples - there is no radio code here.
#include <cstdint>
#include <vector>

#include "lmr/todo.hpp"

namespace lmrsys {

constexpr double kSpeedOfLightKmPerS = 299792.458;

// Q1. Erlang-B blocking probability: offered load `erlangs` on `channels` identical channels,
// blocked calls cleared. Recursion:  B(E,0) = 1;  B(E,m) = E*B(E,m-1) / (m + E*B(E,m-1)).
// TODO. Examples: erlang_b(1,2) = 0.2; erlang_b(2,4) = 0.0952; erlang_b(0.5,1) = 0.3333.
inline double erlang_b(double /*erlangs*/, int /*channels*/) { lmr::todo("erlang_b"); }

// Q3. One-way free-space propagation delay in microseconds for `km` kilometres.
// TODO: km / kSpeedOfLightKmPerS * 1e6      (about 3.3356 us per km)
inline double path_delay_us(double /*km*/) { lmr::todo("path_delay_us"); }

// Q3. Symbol period in microseconds for a given symbol rate (symbols/s).
// TODO: 1e6 / symbol_rate     (4800 sym/s -> 208.333 us)
inline double symbol_period_us(double /*symbol_rate*/) { lmr::todo("symbol_period_us"); }

// Q3. Largest DIFFERENCE in path length (km) between two simulcast transmitters at a receiver such
// that the arrival-time difference stays below `fraction` of one symbol period.
// TODO: fraction * symbol_period_us(rate) / path_delay_us(1.0)
inline double max_delay_spread_km(double /*symbol_rate*/, double /*fraction*/) {
  lmr::todo("max_delay_spread_km");
}

// Q4. One receiver's copy of one frame (synthetic numbers).
struct VoteSample {
  int receiver_id = 0;
  int corrected_errors = 0;  // FEC-corrected error count: lower is better
  bool crc_ok = false;       // frame decoded to a valid codeword
};

// Q4. Frame-by-frame voting. Among samples with crc_ok, return the receiver_id with the fewest
// corrected_errors (ties -> lowest receiver_id). If no sample has crc_ok (or the list is empty) return -1.
// TODO
inline int vote(const std::vector<VoteSample>& /*samples*/) { lmr::todo("vote"); }

// Q4. Probability that ALL n independent receivers lose the frame, each with probability p.
// TODO: p^n   (n == 0 -> 1.0)
inline double vote_failure_probability(double /*p*/, int /*n*/) { lmr::todo("vote_failure_probability"); }

// Q5. Mean time (ms) for a late-joining receiver to acquire the call when the needed information repeats
// every `period_ms` and each repetition is lost independently with probability p_loss (0 <= p_loss < 1).
//   first wait is uniform in (0, period]            -> mean period/2
//   each lost repetition adds one period; the number of losses is geometric -> mean p/(1-p)
// TODO: period_ms/2 + p_loss/(1-p_loss) * period_ms
inline double acquire_time_ms(double /*period_ms*/, double /*p_loss*/) { lmr::todo("acquire_time_ms"); }

// Q12. Greatest common divisor / least common multiple of positive integers.
// TODO
inline int gcd_int(int /*a*/, int /*b*/) { lmr::todo("gcd_int"); }
inline int lcm_int(int /*a*/, int /*b*/) { lmr::todo("lcm_int"); }

// Q12. Smallest span (ms) after which a codec with `in_frame_ms` frames and a codec with `out_frame_ms`
// frames are both exactly on a frame boundary again. TODO: lcm_int
inline int transcode_alignment_ms(int /*in_frame_ms*/, int /*out_frame_ms*/) {
  lmr::todo("transcode_alignment_ms");
}

// Q12. Worst-case extra buffering delay (ms) caused ONLY by re-framing from `in_frame_ms` input frames to
// `out_frame_ms` output frames (an output frame can be produced only after the input frame covering its
// last sample has fully arrived).  TODO: in_frame_ms - gcd_int(in_frame_ms, out_frame_ms)
//   (30 -> 20 gives 20 ms; 20 -> 30 gives 10 ms; equal frames give 0)
inline int max_framing_delay_ms(int /*in_frame_ms*/, int /*out_frame_ms*/) {
  lmr::todo("max_framing_delay_ms");
}

// Q12. PCM samples in one frame: frame_ms * sample_rate_hz / 1000.   TODO
inline int samples_per_frame(int /*frame_ms*/, int /*sample_rate_hz*/) { lmr::todo("samples_per_frame"); }

}  // namespace lmrsys
