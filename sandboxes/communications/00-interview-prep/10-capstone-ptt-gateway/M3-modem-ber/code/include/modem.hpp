// modem.hpp  --  STUBS for Capstone M3 (student version).  Reference: ../solutions/modem.hpp
//
// Complex baseband, 8 samples per symbol, continuous-phase 4FSK with ORTHOGONAL tones: -1.5, -0.5, +0.5, +1.5 cycles per
// symbol (spacing = 1/T), so a non-coherent detector (|correlation| per tone) works with unknown carrier phase.
// Synthetic signals only.
#pragma once
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gw {

using cd = std::complex<double>;
constexpr double kPi = 3.14159265358979323846;
constexpr int kSps = 8;                   // samples per symbol
constexpr uint32_t kSyncWord = 0x02B31E;  // same 24-bit sync as Capstone M1
constexpr int kSyncBits = 24;

// ---- symbol mapping + modulation (W 3.1 - 3.2) ----
// Gray: bits (b1 b0) -> tone index 0..3 in increasing frequency: 00->0, 01->1, 11->2, 10->3.  An odd trailing bit is ignored.
inline std::vector<uint8_t> bits_to_symbols([[maybe_unused]] const std::vector<uint8_t>& bits) { return {}; }  // TODO W 3.1
inline std::vector<uint8_t> symbols_to_bits([[maybe_unused]] const std::vector<uint8_t>& syms) { return {}; }  // TODO W 3.1: b1 first
inline double tone_cycles_per_symbol(int sym) { return sym - 1.5; }  // given
// Continuous-phase modulation: ONE running phase, advanced by 2*pi*f/sps per sample, x[n] = e^{j phase}, unit amplitude.
inline std::vector<cd> modulate([[maybe_unused]] const std::vector<uint8_t>& syms, [[maybe_unused]] int sps = kSps) { return {}; }  // TODO W 3.2
// Given helper: |correlation|^2 of one symbol window with each of the 4 tones.
inline void tone_energies(const cd* w, int sps, double out[4]) {
  for (int k = 0; k < 4; ++k) {
    cd acc = 0;
    for (int n = 0; n < sps; ++n) acc += w[n] * std::polar(1.0, -2.0 * kPi * tone_cycles_per_symbol(k) * n / sps);
    out[k] = std::norm(acc);
  }
}
// Pick the tone with the largest energy (use tone_energies).
inline uint8_t demod_symbol([[maybe_unused]] const cd* w, [[maybe_unused]] int sps = kSps) { return 99; }  // TODO W 3.2
// Demodulate nsym symbols starting at sample `start`; stop early (shorter result) if the buffer runs out.
inline std::vector<uint8_t> demodulate([[maybe_unused]] const std::vector<cd>& iq, [[maybe_unused]] size_t start,
                                       [[maybe_unused]] size_t nsym, [[maybe_unused]] int sps = kSps) {
  return {};  // TODO W 3.2
}

// ---- channel (W 3.3) ----
// Deterministic PRNG (xorshift64*) + Box-Muller so results are identical on every platform. Given.
class Rng {
 public:
  explicit Rng(uint64_t seed) : s_(seed ? seed : 0x9E3779B97F4A7C15ull) {}
  uint64_t next() {
    s_ ^= s_ >> 12;
    s_ ^= s_ << 25;
    s_ ^= s_ >> 27;
    return s_ * 2685821657736338717ull;
  }
  double uniform() { return (static_cast<double>(next() >> 11) + 0.5) / 9007199254740992.0; }  // (0,1)
  double gauss() {
    if (have_) {
      have_ = false;
      return spare_;
    }
    const double u1 = uniform(), u2 = uniform();
    const double r = std::sqrt(-2.0 * std::log(u1));
    spare_ = r * std::sin(2.0 * kPi * u2);
    have_ = true;
    return r * std::cos(2.0 * kPi * u2);
  }

 private:
  uint64_t s_;
  bool have_ = false;
  double spare_ = 0.0;
};
// Unit-amplitude signal => Es = sps.  Complex noise of total variance sigma2 per sample has N0 = sigma2, so
// Es/N0 = sps / sigma2.  Return the standard deviation of EACH of I and Q (= sqrt(sigma2 / 2)).
inline double noise_sigma_per_component([[maybe_unused]] double esn0_db, [[maybe_unused]] int sps = kSps) { return 0.0; }  // TODO W 3.3
inline void add_awgn([[maybe_unused]] std::vector<cd>& iq, [[maybe_unused]] double esn0_db, [[maybe_unused]] Rng& rng,
                     [[maybe_unused]] int sps = kSps) {}  // TODO W 3.3: iq[n] += (sg*gauss, sg*gauss)
// Rotate sample n by exp(j 2 pi cfo n / sps); cfo in cycles per SYMBOL.
inline void apply_cfo([[maybe_unused]] std::vector<cd>& iq, [[maybe_unused]] double cfo_cyc_per_sym, [[maybe_unused]] int sps = kSps) {}  // TODO W 3.3
// Given: `delay` zero samples in front, 4*sps zeros behind, then CFO, then AWGN over everything.
inline std::vector<cd> channel(const std::vector<cd>& tx, double esn0_db, double cfo_cyc_per_sym, size_t delay, Rng& rng, int sps = kSps) {
  std::vector<cd> y(delay, cd(0, 0));
  y.insert(y.end(), tx.begin(), tx.end());
  y.insert(y.end(), static_cast<size_t>(4 * sps), cd(0, 0));
  apply_cfo(y, cfo_cyc_per_sym, sps);
  add_awgn(y, esn0_db, rng, sps);
  return y;
}

// ---- synchronisation + CFO (W 3.4 - 3.5) ----
// The 24 sync bits (MSB first) mapped to 12 symbols.
inline std::vector<uint8_t> sync_symbols() { return {}; }  // TODO W 3.4
// Non-coherent metric at sample offset t: sum over sync symbols of |<window_i, expected tone>|^2, divided by
// (sps * total energy of the 12*sps samples).  1.0 = perfect match; pure noise ~ 1/sps.
inline double sync_metric([[maybe_unused]] const std::vector<cd>& iq, [[maybe_unused]] size_t t,
                          [[maybe_unused]] const std::vector<uint8_t>& sync, [[maybe_unused]] int sps = kSps) {
  return -1.0;  // TODO W 3.4
}
// Sample offset with the highest metric; -1 if that peak is below `threshold`.  Store the peak in *peak if given.
inline long find_frame_start([[maybe_unused]] const std::vector<cd>& iq, [[maybe_unused]] const std::vector<uint8_t>& sync,
                             [[maybe_unused]] double threshold, [[maybe_unused]] double* peak = nullptr, [[maybe_unused]] int sps = kSps) {
  return -2;  // TODO W 3.4
}
// CFO (cycles/symbol) from known symbols at sample `start`:
//   angle( sum_n [ r[n+1] conj(r[n]) ] * conj( s[n+1] conj(s[n]) ) ) * sps / (2 pi),  s = modulate(known).
inline double estimate_cfo([[maybe_unused]] const std::vector<cd>& iq, [[maybe_unused]] size_t start,
                           [[maybe_unused]] const std::vector<uint8_t>& known, [[maybe_unused]] int sps = kSps) {
  return 99.0;  // TODO W 3.5
}
inline void derotate(std::vector<cd>& iq, double cfo_cyc_per_sym, int sps = kSps) { apply_cfo(iq, -cfo_cyc_per_sym, sps); }  // given

// ---- transmit / receive chain (W 3.6) ----
// `frame_bits` must START with the 24 sync bits (as produced by Capstone M1).  Given.
inline std::vector<cd> transmit(const std::vector<uint8_t>& frame_bits, int sps = kSps) { return modulate(bits_to_symbols(frame_bits), sps); }
struct RxResult {
  bool found = false;
  size_t start = 0;            // sample offset of the first sync sample
  double metric = 0;
  double cfo = 0;              // estimated, cycles per symbol
  std::vector<uint8_t> bits;   // total_bits demodulated bits, sync included
};
// find sync (metric >= sync_threshold) -> estimate CFO on the sync symbols -> derotate a COPY of the input ->
// demodulate total_bits/2 symbols from the frame start -> symbols_to_bits.  found=false (empty bits) if no sync.
inline RxResult receive([[maybe_unused]] const std::vector<cd>& iq_in, [[maybe_unused]] size_t total_bits,
                        [[maybe_unused]] double sync_threshold = 0.5, [[maybe_unused]] int sps = kSps) {
  return RxResult{};  // TODO W 3.6
}

// ---- BER bookkeeping (W 3.7) ----
// Differing bits over the common length, plus one error per bit of length mismatch.
inline size_t count_bit_errors([[maybe_unused]] const std::vector<uint8_t>& a, [[maybe_unused]] const std::vector<uint8_t>& b) { return 12345; }  // TODO
inline double esn0_to_ebn0_db(double esn0_db, int bits_per_symbol = 2) { return esn0_db - 10.0 * std::log10(static_cast<double>(bits_per_symbol)); }  // given
inline double ebn0_to_esn0_db(double ebn0_db, int bits_per_symbol = 2) { return ebn0_db + 10.0 * std::log10(static_cast<double>(bits_per_symbol)); }  // given
// Non-coherent orthogonal 4-FSK, g = Es/N0 (linear):  Ps = 3/2 e^{-g/2} - e^{-2g/3} + 1/4 e^{-3g/4};  Pb = (2/3) Ps.
inline double ber_4fsk_noncoherent([[maybe_unused]] double esn0_db) { return 0.5; }  // TODO W 3.7
// errors / ber
inline double bits_needed_for_ber([[maybe_unused]] double ber, [[maybe_unused]] int errors = 100) { return 0.0; }  // TODO W 3.7

}  // namespace gw
