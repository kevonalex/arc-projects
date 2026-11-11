#pragma once
// Pack 09 — SDR front-end impairments and fixed-point (Q15/Q31) arithmetic.
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
using cplx = std::complex<double>;
using q15_t = std::int16_t;  // value = raw / 2^15, range [-1, 1 - 2^-15]
using q31_t = std::int32_t;  // value = raw / 2^31

// ------------------------------------------------------------------ provided helpers (do not edit)
// I' = I, Q' = g (Q cos(phi) + I sin(phi)):  gain error g and phase error phi (rad) in the Q branch.
inline std::vector<cplx> apply_iq_imbalance(const std::vector<cplx>& z, double g, double phi) {
  std::vector<cplx> y(z.size());
  for (std::size_t n = 0; n < z.size(); ++n) {
    const double i = z[n].real(), q = z[n].imag();
    y[n] = cplx(i, g * (q * std::cos(phi) + i * std::sin(phi)));
  }
  return y;
}
// 10 log10( |X(+f)|^2 / |X(-f)|^2 ), f in cycles/sample (use integer cycles per record for exactness).
inline double measure_image_rejection_db(const std::vector<cplx>& x, double f) {
  cplx pos(0, 0), neg(0, 0);
  for (std::size_t n = 0; n < x.size(); ++n) {
    pos += x[n] * std::polar(1.0, -2.0 * kPi * f * static_cast<double>(n));
    neg += x[n] * std::polar(1.0, +2.0 * kPi * f * static_cast<double>(n));
  }
  return 10.0 * std::log10(std::norm(pos) / (std::norm(neg) + 1e-300));
}
// SQNR in dB of a quantised sequence against a double reference (both same length, q scaled by 2^15).
inline double q15_sqnr_db(const std::vector<double>& ref, const std::vector<q15_t>& q) {
  double s = 0.0, e = 0.0;
  for (std::size_t i = 0; i < ref.size(); ++i) {
    const double d = static_cast<double>(q[i]) / 32768.0 - ref[i];
    s += ref[i] * ref[i];
    e += d * d;
  }
  return 10.0 * std::log10(s / e);
}

// ------------------------------------------------------------------ Q15 / Q31 primitives (B1–B3)
// B1. round to nearest (ties away from zero) of x * 2^15, saturated to [-32768, 32767].
q15_t float_to_q15(double x);
double q15_to_float(q15_t v);
// B2. saturating add (no wrap-around).
q15_t q15_add_sat(q15_t a, q15_t b);
// B3. (a*b + 2^14) >> 15 in a 32-bit intermediate (arithmetic shift), saturating (-1)*(-1) to 32767.
q15_t q15_mul(q15_t a, q15_t b);
// B3. same idea for Q31: ((int64)a*b + 2^30) >> 31, saturating.
q31_t q31_mul(q31_t a, q31_t b);

// ------------------------------------------------------------------ B4/B5: FIR and word-length planning
// B4. y[n] = sat( (sum_k h[k] x[n-k] + 2^14) >> 15 ), x[n<0] = 0, 64-bit accumulator.  y.size() == x.size().
std::vector<q15_t> q15_fir(const std::vector<q15_t>& x, const std::vector<q15_t>& h);
// B5. Worst-case accumulator growth in bits: ceil(log2(sum |h|)) if sum |h| > 1, else 0.
int guard_bits(const std::vector<double>& h);

// ------------------------------------------------------------------ B6/B7: front-end clean-up
// B6. Subtract the mean (estimate of the DC offset) from I and Q.
std::vector<cplx> remove_dc(const std::vector<cplx>& x);
// B7. Blind Gram–Schmidt IQ-imbalance correction (assumes a circular signal, DC already removed):
//     rho = sum(I Q)/sum(I I);  Q1 = Q - rho I;  c = sqrt(sum(I I)/sum(Q1 Q1));  out = I + j c Q1.
//     If the power sums are not positive return x unchanged.
std::vector<cplx> correct_iq_imbalance(const std::vector<cplx>& x);

// ------------------------------------------------------------------ B8/B9: small link-budget helpers
struct Stage {
  double gain_db;  // negative for a lossy stage
  double nf_db;    // for a passive loss equal to its loss in dB
};
// B8. Friis cascade: F = F1 + (F2-1)/G1 + (F3-1)/(G1 G2) + ...   Returns the noise figure in dB.
double cascade_nf_db(const std::vector<Stage>& stages);
// B9a. ppm * 1e-6 * fc.
double ppm_to_hz(double ppm, double carrier_hz);
// B9b. Rotation per symbol in degrees: 360 * cfo / symbol_rate.
double cfo_phase_per_symbol_deg(double cfo_hz, double symbol_rate);
// B9c. Wiener phase noise with increment std sigma (rad/sample): Lorentzian FWHM = sigma^2 * fs / (2 pi).
double phase_noise_linewidth_hz(double sigma_rad, double fs_hz);
// B9d. Reciprocal-mixing noise floor in dBm: blocker_dbm + L(offset) [dBc/Hz] + 10 log10(bw).
double reciprocal_mixing_floor_dbm(double blocker_dbm, double l_dbc_hz, double bw_hz);

}  // namespace dsp
