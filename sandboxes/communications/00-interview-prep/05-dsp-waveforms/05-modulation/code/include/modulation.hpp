#pragma once
// Pack 05 — AM/FM/FSK/PSK/QAM/OFDM building blocks (complex baseband).
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
using cplx = std::complex<double>;

// PROVIDED (not an exercise): naive DFT.  inverse=true uses exp(+j...) and divides by N.
inline std::vector<cplx> dft_naive(const std::vector<cplx>& x, bool inverse) {
  const std::size_t n = x.size();
  std::vector<cplx> out(n);
  const double sign = inverse ? 1.0 : -1.0;
  for (std::size_t k = 0; k < n; ++k) {
    cplx acc(0.0, 0.0);
    for (std::size_t m = 0; m < n; ++m) {
      const std::size_t km = (k * m) % n;
      acc += x[m] * std::polar(1.0, sign * 2.0 * kPi * static_cast<double>(km) / static_cast<double>(n));
    }
    out[k] = inverse ? acc / static_cast<double>(n) : acc;
  }
  return out;
}

// ---- B1/B2: 4FSK (P25 C4FM / DMR style).  Level map used here (VERIFY vs standard):
//      dibit 0b00 -> +1, 0b01 -> +3, 0b10 -> -1, 0b11 -> -3   (times the deviation unit)
int fsk4_level(std::uint8_t dibit);  // uses only the two LSBs

// Phase-continuous 4FSK: for each dibit hold frequency level*dev_unit_hz for `sps` samples.
// Output sample n is exp(j*phase) emitted BEFORE the phase step; after each sample phase += 2 pi f / fs.
// Output length = dibits.size()*sps; phase starts at 0.
std::vector<cplx> fsk4_modulate(const std::vector<std::uint8_t>& dibits, std::size_t sps,
                                double dev_unit_hz, double fs_hz);

// Discriminator demodulator.  For each whole symbol s average arg(x[n+1] conj(x[n])) over its sps samples
// n = s*sps .. s*sps+sps-1 (skip n+1 >= x.size()), convert to Hz, divide by dev_unit_hz and slice:
//   > +2 -> +3 (0b01);  0..+2 -> +1 (0b00);  -2..0 -> -1 (0b10);  < -2 -> -3 (0b11).
std::vector<std::uint8_t> fsk4_demodulate(const std::vector<cplx>& x, std::size_t sps, double dev_unit_hz,
                                          double fs_hz);

// ---- B3: Gray QPSK:  I = (b1 ? -1 : +1)/sqrt2, Q = (b0 ? -1 : +1)/sqrt2 with dibit = b1 b0.
cplx qpsk_map(std::uint8_t dibit);
std::uint8_t qpsk_demap(cplx s);  // sign decisions

// ---- B4: pi/4-DQPSK.  Phase increments (VERIFY vs TETRA): 00:+pi/4, 01:+3pi/4, 11:-3pi/4, 10:-pi/4.
// Start phase 0; one unit-magnitude output symbol per dibit (emitted AFTER adding the increment).
std::vector<cplx> pi4dqpsk_modulate(const std::vector<std::uint8_t>& dibits);
// Differential decode: delta = arg(s[n] * conj(prev)), prev starts at `reference`; pick the nearest of the four
// increments.
std::vector<std::uint8_t> pi4dqpsk_demodulate(const std::vector<cplx>& symbols,
                                              cplx reference = cplx(1.0, 0.0));

// ---- B5: Gray 16-QAM, nibble = b3 b2 b1 b0.  I from (b3 b2), Q from (b1 b0); level map 00:-3 01:-1 11:+1 10:+3;
//      scaled by 1/sqrt(10) so the average symbol energy is 1.
cplx qam16_map(std::uint8_t nibble);

// ---- B6: OFDM.
// x = IDFT(X) (dft_naive(X, true)); output = last cp_len samples of x followed by x.  Throws std::invalid_argument
// if cp_len > X.size().
std::vector<cplx> ofdm_modulate(const std::vector<cplx>& X, std::size_t cp_len);
// Drop the first cp_len samples, take n_sub samples, forward DFT.  Throws std::invalid_argument if rx is too short.
std::vector<cplx> ofdm_demodulate(const std::vector<cplx>& rx, std::size_t n_sub, std::size_t cp_len);
// Single-tap equaliser: H = DFT(h zero-padded to Y.size()); out[k] = Y[k]/H[k].
std::vector<cplx> ofdm_equalise(const std::vector<cplx>& Y, const std::vector<cplx>& h_taps);

// ---- B7: FM.
// z[n] = exp(j*phi[n]), phi[0] = 0, phi[n+1] = phi[n] + 2 pi kf m[n]/fs.
std::vector<cplx> fm_modulate(const std::vector<double>& m, double kf_hz, double fs_hz);
// m_hat[n] = arg(z[n+1] conj(z[n])) * fs/(2 pi kf), size z.size()-1 (or 0).
std::vector<double> fm_demodulate(const std::vector<cplx>& z, double kf_hz, double fs_hz);
// Carson's rule: 2*(dev + fm).
double carson_bandwidth_hz(double dev_hz, double fm_hz);

}  // namespace dsp
