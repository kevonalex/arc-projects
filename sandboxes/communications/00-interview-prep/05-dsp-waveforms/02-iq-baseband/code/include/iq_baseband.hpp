#pragma once
// Pack 02 — I/Q and complex baseband.
#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
using cplx = std::complex<double>;

// B1. y[n] = x[n] * exp(+j*(2*pi*f*n + phase0)).   f in cycles/sample.  (Positive f shifts UP.)
std::vector<cplx> mix(const std::vector<cplx>& x, double f_cycles_per_sample, double phase0_rad);

// B2. Quadrature down-conversion of a REAL signal.
//     raw_i[n] =  2*x[n]*cos(2*pi*fc*n),  raw_q[n] = -2*x[n]*sin(2*pi*fc*n)
//     I = causal FIR(lpf) of raw_i, Q = same FIR of raw_q  (zero initial state, same length as x).
//     return I + jQ.
std::vector<cplx> iq_downconvert(const std::vector<double>& x, double fc_cycles_per_sample,
                                 const std::vector<double>& lpf_taps);

// B3. Lag-1 autocorrelation frequency estimator: arg( sum z[n+1]*conj(z[n]) ) / (2*pi), cycles/sample.
//     Returns 0 for fewer than 2 samples.
double estimate_freq_cps(const std::vector<cplx>& z);

// B4. Per-sample FM discriminator: out[n] = arg(z[n+1]*conj(z[n])) / (2*pi); size = z.size()-1 (or 0).
std::vector<double> instantaneous_freq(const std::vector<cplx>& z);

// B5. 20*log10( |sum z[n] e^{-j2 pi f n}| / |sum z[n] e^{+j2 pi f n}| ); denominator floored at 1e-300.
double image_rejection_db(const std::vector<cplx>& z, double f_cycles_per_sample);

}  // namespace dsp
