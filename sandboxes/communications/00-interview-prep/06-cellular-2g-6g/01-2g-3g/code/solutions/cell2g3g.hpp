// cell2g3g.hpp  --  REFERENCE SOLUTION for Pack 06/01 (2G GSM TDMA, 3G WCDMA/OVSF).
// Every function cites the worksheet section that derives it. Numbers marked VERIFY in the worksheet.
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace cell {

// ------------------------------------------------------------------ 2G GSM numbers (W 3.1 - 3.3)
// Gross air bit rate = 13 MHz / 48 = 270.833 kbit/s (GMSK, 1 bit per symbol).
inline double gsm_bit_rate_bps() { return 13.0e6 / 48.0; }
// One TDMA slot lasts 15/26 ms = 576.9 us; 8 slots make a 4.615 ms TDMA frame.
inline double gsm_slot_ms() { return 15.0 / 26.0; }
inline double gsm_frame_ms() { return 8.0 * gsm_slot_ms(); }
// Bit periods that fit in one slot (148 burst bits + 8.25 guard bit periods = 156.25).
inline double gsm_bits_per_slot() { return gsm_bit_rate_bps() * gsm_slot_ms() / 1000.0; }
// P-GSM 900 (ARFCN 1..124): UL = 890 + 0.2*n MHz, DL = UL + 45 MHz.  Returns -1 if out of range.
inline double gsm_ul_mhz(int arfcn) { return (arfcn >= 1 && arfcn <= 124) ? 890.0 + 0.2 * arfcn : -1.0; }
inline double gsm_dl_mhz(int arfcn) {
  const double ul = gsm_ul_mhz(arfcn);
  return ul < 0 ? -1.0 : ul + 45.0;
}
// Timing-advance step: 1 bit period of round-trip delay = c * Tb / 2 metres of range (~553 m).
inline double gsm_ta_step_m() { return 299792458.0 * (1.0 / gsm_bit_rate_bps()) / 2.0; }
// Full-rate speech after channel coding: 456 bits per 20 ms block.
inline double gsm_tch_coded_rate_bps() { return 456.0 / 0.020; }
// Cluster size for hexagonal reuse N = i^2 + i*j + j^2, and co-channel distance ratio D/R = sqrt(3N).
inline int cluster_size(int i, int j) { return i * i + i * j + j * j; }
inline double reuse_distance_ratio(int n) { return std::sqrt(3.0 * n); }
// MSK / GMSK modulation index 0.5  =>  peak frequency deviation = bit_rate / 4.
inline double msk_deviation_hz(double bitrate_bps) { return bitrate_bps / 4.0; }

// ------------------------------------------------------------------ 3G WCDMA (W 3.4 - 3.6)
// Spreading factor = chips per symbol = chip_rate / symbol_rate (rounded to nearest integer).
inline int spreading_factor(double chip_rate, double symbol_rate) {
  return static_cast<int>(std::lround(chip_rate / symbol_rate));
}
inline double processing_gain_db(int sf) { return 10.0 * std::log10(static_cast<double>(sf)); }

// OVSF code tree: C(1,0) = {+1};  C(2n,2k) = [C(n,k), C(n,k)];  C(2n,2k+1) = [C(n,k), -C(n,k)].
inline std::vector<int8_t> ovsf_code(int sf, int index) {
  if (sf <= 1) return {1};
  std::vector<int8_t> parent = ovsf_code(sf / 2, index >> 1);
  std::vector<int8_t> out = parent;
  for (int8_t c : parent) out.push_back((index & 1) ? static_cast<int8_t>(-c) : c);
  return out;
}
// Can't both be in use: one is an ancestor (or equal) of the other in the tree.
inline bool ovsf_conflict(int sf_a, int idx_a, int sf_b, int idx_b) {
  if (sf_a > sf_b) return ovsf_conflict(sf_b, idx_b, sf_a, idx_a);
  // now sf_a <= sf_b; a is an ancestor of b iff b's index shifted up to a's level equals a's index
  int shift = 0;
  while ((sf_a << shift) < sf_b) ++shift;
  return (idx_b >> shift) == idx_a;
}
// Spread: each symbol (+1/-1) becomes code.size() chips = symbol * code[i].
inline std::vector<int> spread(const std::vector<int>& symbols, const std::vector<int8_t>& code) {
  std::vector<int> chips;
  chips.reserve(symbols.size() * code.size());
  for (int s : symbols)
    for (int8_t c : code) chips.push_back(s * c);
  return chips;
}
// Despread: correlate each code-length block with the code and divide by the length.
inline std::vector<double> despread(const std::vector<int>& chips, const std::vector<int8_t>& code) {
  std::vector<double> out;
  const size_t n = code.size();
  if (n == 0) return out;
  for (size_t b = 0; b + n <= chips.size(); b += n) {
    long acc = 0;
    for (size_t i = 0; i < n; ++i) acc += static_cast<long>(chips[b + i]) * code[i];
    out.push_back(static_cast<double>(acc) / static_cast<double>(n));
  }
  return out;
}
// Cyclic cross-correlation at an integer chip shift: sum_i a[i] * b[(i+shift) mod N]  (equal lengths).
inline int cross_correlation(const std::vector<int8_t>& a, const std::vector<int8_t>& b, int shift) {
  const int n = static_cast<int>(a.size());
  if (n == 0 || static_cast<int>(b.size()) != n) return 0;
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += a[static_cast<size_t>(i)] * b[static_cast<size_t>(((i + shift) % n + n) % n)];
  return acc;
}

}  // namespace cell
