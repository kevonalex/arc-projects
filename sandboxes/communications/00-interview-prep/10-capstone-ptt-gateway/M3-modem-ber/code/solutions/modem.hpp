// modem.hpp  --  REFERENCE SOLUTION for Capstone M3 (4FSK modem, AWGN + CFO channel, sync, demod, BER).
//
// Complex baseband, 8 samples per symbol, continuous-phase 4FSK with ORTHOGONAL tones: -1.5, -0.5, +0.5, +1.5 cycles per
// symbol (spacing = 1/T).  Orthogonal tones let a non-coherent detector (|correlation| per tone) work with unknown phase.
// Real LMR 4FSK/C4FM uses much tighter spacing (VERIFY: P25 deviation +-600/+-1800 Hz at 4800 sym/s) and a coherent-ish
// discriminator receiver; we trade bandwidth for simplicity.  Synthetic signals only.
#pragma once
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gw {

using cd = std::complex<double>;
constexpr double kPi = 3.14159265358979323846;
constexpr int kSps = 8;                    // samples per symbol
constexpr uint32_t kSyncWord = 0x02B31E;   // same 24-bit sync as Capstone M1
constexpr int kSyncBits = 24;

// ------------------------------------------------------------------ symbol mapping + modulation (W 3.1 - 3.2)
// Gray: bits (b1 b0) -> tone index 0..3 in increasing frequency: 00->0, 01->1, 11->2, 10->3.
inline std::vector<uint8_t> bits_to_symbols(const std::vector<uint8_t>& bits) {
  static const uint8_t map[4] = {0, 1, 3, 2};  // index = (b1<<1)|b0
  std::vector<uint8_t> s;
  for (size_t i = 0; i + 1 < bits.size(); i += 2) s.push_back(map[((bits[i] & 1u) << 1) | (bits[i + 1] & 1u)]);
  return s;
}
inline std::vector<uint8_t> symbols_to_bits(const std::vector<uint8_t>& syms) {
  static const uint8_t map[4] = {0, 1, 3, 2};  // tone index -> (b1<<1)|b0  (the Gray map is its own inverse)
  std::vector<uint8_t> b;
  for (uint8_t s : syms) {
    const uint8_t v = map[s & 3u];
    b.push_back(static_cast<uint8_t>(v >> 1));
    b.push_back(static_cast<uint8_t>(v & 1u));
  }
  return b;
}
inline double tone_cycles_per_symbol(int sym) { return sym - 1.5; }
// Continuous-phase modulation: one running phase, advanced by 2*pi*f/sps per sample.
inline std::vector<cd> modulate(const std::vector<uint8_t>& syms, int sps = kSps) {
  std::vector<cd> x;
  x.reserve(syms.size() * static_cast<size_t>(sps));
  double phase = 0.0;
  for (uint8_t s : syms) {
    const double dphi = 2.0 * kPi * tone_cycles_per_symbol(s) / sps;
    for (int n = 0; n < sps; ++n) {
      x.push_back(std::polar(1.0, phase));
      phase += dphi;
    }
  }
  return x;
}
// Correlate one symbol window (sps samples) with each of the 4 tones; |.|^2 per tone.
inline void tone_energies(const cd* w, int sps, double out[4]) {
  for (int k = 0; k < 4; ++k) {
    cd acc = 0;
    for (int n = 0; n < sps; ++n) acc += w[n] * std::polar(1.0, -2.0 * kPi * tone_cycles_per_symbol(k) * n / sps);
    out[k] = std::norm(acc);
  }
}
inline uint8_t demod_symbol(const cd* w, int sps = kSps) {
  double e[4];
  tone_energies(w, sps, e);
  int best = 0;
  for (int k = 1; k < 4; ++k)
    if (e[k] > e[best]) best = k;
  return static_cast<uint8_t>(best);
}
// Demodulate nsym symbols starting at sample `start`; stops early (shorter result) if the buffer runs out.
inline std::vector<uint8_t> demodulate(const std::vector<cd>& iq, size_t start, size_t nsym, int sps = kSps) {
  std::vector<uint8_t> out;
  for (size_t i = 0; i < nsym; ++i) {
    const size_t pos = start + i * static_cast<size_t>(sps);
    if (pos + static_cast<size_t>(sps) > iq.size()) break;
    out.push_back(demod_symbol(&iq[pos], sps));
  }
  return out;
}

// ------------------------------------------------------------------ channel (W 3.3)
// Deterministic PRNG (xorshift64*) + Box-Muller so results are identical on every platform. Given, not a stub.
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
// Unit-amplitude signal: Es = sps (sum of |x|^2 over a symbol).  Complex noise of total variance sigma2 per sample has
// N0 = sigma2, so Es/N0 = sps / sigma2  ->  sigma2 = sps / 10^(EsN0dB/10); each of I and Q gets sigma2/2.
inline double noise_sigma_per_component(double esn0_db, int sps = kSps) { return std::sqrt(0.5 * sps / std::pow(10.0, esn0_db / 10.0)); }
inline void add_awgn(std::vector<cd>& iq, double esn0_db, Rng& rng, int sps = kSps) {
  const double sg = noise_sigma_per_component(esn0_db, sps);
  for (cd& v : iq) v += cd(sg * rng.gauss(), sg * rng.gauss());
}
// Carrier frequency offset: rotate sample n by exp(j 2 pi cfo n / sps), cfo in cycles per SYMBOL.
inline void apply_cfo(std::vector<cd>& iq, double cfo_cyc_per_sym, int sps = kSps) {
  for (size_t n = 0; n < iq.size(); ++n) iq[n] *= std::polar(1.0, 2.0 * kPi * cfo_cyc_per_sym * static_cast<double>(n) / sps);
}
// Whole channel: `delay` zero samples in front, `tail` zero samples behind, then CFO, then AWGN over everything.
inline std::vector<cd> channel(const std::vector<cd>& tx, double esn0_db, double cfo_cyc_per_sym, size_t delay, Rng& rng, int sps = kSps) {
  std::vector<cd> y(delay, cd(0, 0));
  y.insert(y.end(), tx.begin(), tx.end());
  y.insert(y.end(), static_cast<size_t>(4 * sps), cd(0, 0));
  apply_cfo(y, cfo_cyc_per_sym, sps);
  add_awgn(y, esn0_db, rng, sps);
  return y;
}

// ------------------------------------------------------------------ synchronisation + CFO (W 3.4 - 3.5)
inline std::vector<uint8_t> sync_symbols() {
  std::vector<uint8_t> bits;
  for (int i = kSyncBits - 1; i >= 0; --i) bits.push_back(static_cast<uint8_t>((kSyncWord >> i) & 1u));
  return bits_to_symbols(bits);  // 12 symbols
}
// Non-coherent sync metric at sample offset t: sum over sync symbols of |<window, expected tone>|^2, divided by
// (sps * window energy).  1.0 = perfect match; pure noise ~ 1/sps.  Tolerates unknown phase AND moderate CFO.
inline double sync_metric(const std::vector<cd>& iq, size_t t, const std::vector<uint8_t>& sync, int sps = kSps) {
  double num = 0, energy = 0;
  for (size_t i = 0; i < sync.size(); ++i) {
    cd acc = 0;
    const double f = tone_cycles_per_symbol(sync[i]);
    for (int n = 0; n < sps; ++n) {
      const cd x = iq[t + i * static_cast<size_t>(sps) + static_cast<size_t>(n)];
      acc += x * std::polar(1.0, -2.0 * kPi * f * n / sps);
      energy += std::norm(x);
    }
    num += std::norm(acc);
  }
  return energy <= 0 ? 0.0 : num / (sps * energy);
}
// Best-matching sample offset, or -1 if the peak metric is below `threshold`.
inline long find_frame_start(const std::vector<cd>& iq, const std::vector<uint8_t>& sync, double threshold, double* peak = nullptr, int sps = kSps) {
  const size_t len = sync.size() * static_cast<size_t>(sps);
  long best = -1;
  double best_m = 0;
  if (iq.size() >= len) {
    for (size_t t = 0; t + len <= iq.size(); ++t) {
      const double m = sync_metric(iq, t, sync, sps);
      if (m > best_m) {
        best_m = m;
        best = static_cast<long>(t);
      }
    }
  }
  if (peak) *peak = best_m;
  return best_m >= threshold ? best : -1;
}
// CFO from a known symbol sequence at sample `start`: the angle of sum[ r[n+1] r*[n] ] * conj( s[n+1] s*[n] ).
// Returns cycles per symbol.  Unambiguous for |cfo| < sps/2 cycles per symbol; useful range is far smaller.
inline double estimate_cfo(const std::vector<cd>& iq, size_t start, const std::vector<uint8_t>& known, int sps = kSps) {
  const std::vector<cd> ref = modulate(known, sps);
  cd acc = 0;
  for (size_t n = 0; n + 1 < ref.size() && start + n + 1 < iq.size(); ++n)
    acc += (iq[start + n + 1] * std::conj(iq[start + n])) * std::conj(ref[n + 1] * std::conj(ref[n]));
  return std::arg(acc) * sps / (2.0 * kPi);
}
inline void derotate(std::vector<cd>& iq, double cfo_cyc_per_sym, int sps = kSps) { apply_cfo(iq, -cfo_cyc_per_sym, sps); }

// ------------------------------------------------------------------ transmit / receive chain (W 3.6)
// `frame_bits` must START with the 24 sync bits (as produced by Capstone M1).
inline std::vector<cd> transmit(const std::vector<uint8_t>& frame_bits, int sps = kSps) { return modulate(bits_to_symbols(frame_bits), sps); }
struct RxResult {
  bool found = false;
  size_t start = 0;  // sample offset of the first sync sample
  double metric = 0;
  double cfo = 0;  // estimated, cycles per symbol
  std::vector<uint8_t> bits;  // total_bits demodulated bits, sync included
};
inline RxResult receive(const std::vector<cd>& iq_in, size_t total_bits, double sync_threshold = 0.5, int sps = kSps) {
  RxResult r;
  const std::vector<uint8_t> sync = sync_symbols();
  const long s = find_frame_start(iq_in, sync, sync_threshold, &r.metric, sps);
  if (s < 0) return r;
  r.found = true;
  r.start = static_cast<size_t>(s);
  r.cfo = estimate_cfo(iq_in, r.start, sync, sps);
  std::vector<cd> iq = iq_in;
  derotate(iq, r.cfo, sps);
  r.bits = symbols_to_bits(demodulate(iq, r.start, total_bits / 2, sps));
  return r;
}

// ------------------------------------------------------------------ BER bookkeeping (W 3.7)
inline size_t count_bit_errors(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
  const size_t n = a.size() < b.size() ? a.size() : b.size();
  size_t e = a.size() > b.size() ? a.size() - b.size() : b.size() - a.size();  // missing bits count as errors
  for (size_t i = 0; i < n; ++i) e += (a[i] & 1u) != (b[i] & 1u);
  return e;
}
inline double esn0_to_ebn0_db(double esn0_db, int bits_per_symbol = 2) { return esn0_db - 10.0 * std::log10(static_cast<double>(bits_per_symbol)); }
inline double ebn0_to_esn0_db(double ebn0_db, int bits_per_symbol = 2) { return ebn0_db + 10.0 * std::log10(static_cast<double>(bits_per_symbol)); }
// Non-coherent orthogonal 4-FSK: Ps = sum_{k=1..3} (-1)^(k+1) C(3,k)/(k+1) exp(-k/(k+1) Es/N0);  Pb = (M/2)/(M-1) * Ps = 2/3 Ps.
inline double ber_4fsk_noncoherent(double esn0_db) {
  const double g = std::pow(10.0, esn0_db / 10.0);
  const double ps = 1.5 * std::exp(-0.5 * g) - 1.0 * std::exp(-(2.0 / 3.0) * g) + 0.25 * std::exp(-0.75 * g);
  return ps * 2.0 / 3.0;
}
// Bits you must send to SEE `errors` errors at a true BER of `ber` (100 errors -> about +-20 % at 95 % confidence).
inline double bits_needed_for_ber(double ber, int errors = 100) { return errors / ber; }

}  // namespace gw
