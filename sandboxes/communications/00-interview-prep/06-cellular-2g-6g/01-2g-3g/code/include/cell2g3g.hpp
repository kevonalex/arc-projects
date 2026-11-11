// cell2g3g.hpp  --  STUBS for Pack 06/01 (student version).  Reference: ../solutions/cell2g3g.hpp
// Fill in each function; every TODO names the worksheet section that derives it.
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace cell {

// ---- 2G GSM numbers (W 3.1 - 3.3) ----
inline double gsm_bit_rate_bps() { return 0.0; }  // TODO W 3.1: 13 MHz / 48
inline double gsm_slot_ms() { return 0.0; }       // TODO W 3.1: 15/26 ms
inline double gsm_frame_ms() { return 0.0; }      // TODO W 3.1: 8 slots
inline double gsm_bits_per_slot() { return 0.0; } // TODO W 3.1: bit rate * slot time (expect 156.25)
// P-GSM 900, ARFCN 1..124: UL = 890 + 0.2n MHz, DL = UL + 45.  Return -1.0 when out of range.
inline double gsm_ul_mhz([[maybe_unused]] int arfcn) { return 0.0; }  // TODO W 3.2
inline double gsm_dl_mhz([[maybe_unused]] int arfcn) { return 0.0; }  // TODO W 3.2
// Metres of range represented by one timing-advance step (one bit period, round trip).
inline double gsm_ta_step_m() { return 0.0; }  // TODO W 3.2
inline double gsm_tch_coded_rate_bps() { return 0.0; }  // TODO W 3.3: 456 bits / 20 ms
inline int cluster_size([[maybe_unused]] int i, [[maybe_unused]] int j) { return 0; }  // TODO W 3.3
inline double reuse_distance_ratio([[maybe_unused]] int n) { return 0.0; }             // TODO W 3.3: sqrt(3N)
inline double msk_deviation_hz([[maybe_unused]] double bitrate_bps) { return 0.0; }    // TODO W 3.3: Rb/4

// ---- 3G WCDMA (W 3.4 - 3.6) ----
inline int spreading_factor([[maybe_unused]] double chip_rate, [[maybe_unused]] double symbol_rate) { return 0; }  // TODO W 3.4
inline double processing_gain_db([[maybe_unused]] int sf) { return 0.0; }  // TODO W 3.4: 10 log10(SF)
// OVSF tree: C(1,0)={+1}; C(2n,2k)=[C,C]; C(2n,2k+1)=[C,-C] where C = C(n,k).
inline std::vector<int8_t> ovsf_code([[maybe_unused]] int sf, [[maybe_unused]] int index) { return {}; }  // TODO W 3.5
// True if the two (sf, index) codes may NOT be used together (one is an ancestor of / equal to the other).
inline bool ovsf_conflict([[maybe_unused]] int sf_a, [[maybe_unused]] int idx_a, [[maybe_unused]] int sf_b,
                          [[maybe_unused]] int idx_b) {
  return false;  // TODO W 3.5
}
// symbol (+1/-1) -> code.size() chips (symbol * code[i]).
inline std::vector<int> spread([[maybe_unused]] const std::vector<int>& symbols,
                               [[maybe_unused]] const std::vector<int8_t>& code) {
  return {};  // TODO W 3.5
}
// Correlate each code-length block of chips with the code, divide by code length.
inline std::vector<double> despread([[maybe_unused]] const std::vector<int>& chips,
                                    [[maybe_unused]] const std::vector<int8_t>& code) {
  return {};  // TODO W 3.5
}
// Cyclic cross-correlation: sum_i a[i] * b[(i+shift) mod N]. Return 0 if lengths differ or are empty.
inline int cross_correlation([[maybe_unused]] const std::vector<int8_t>& a, [[maybe_unused]] const std::vector<int8_t>& b,
                             [[maybe_unused]] int shift) {
  return 12345;  // TODO W 3.6
}

}  // namespace cell
