#pragma once
// Pack 01 — sampling, aliasing, quantisation.  Declarations only; implement in stubs/sampling_quant.cpp.
#include <cstddef>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;

// B1. Apparent frequency in [0, fs/2] of a REAL tone at f_hz (f_hz may be negative or > fs).
//     r = |f| mod fs;  return r <= fs/2 ? r : fs - r.
double alias_frequency(double f_hz, double fs_hz);

// B2. x[n] = sin(2*pi*f*n/fs + phase), n = 0..n-1.
std::vector<double> sample_sine(double f_hz, double fs_hz, double phase_rad, std::size_t n);

// B3. Mid-tread uniform quantiser.  step = 2*full_scale / 2^bits, code k = floor(x/step + 0.5),
//     clamp k to [-2^(bits-1), 2^(bits-1)-1], return k*step.  (+full_scale itself is NOT representable.)
double quantise_uniform(double x, int bits, double full_scale);

// B4a. Theory: 10*log10(1.5) + 20*bits*log10(2)   (= 1.76 + 6.02*bits).
double ideal_sqnr_db(int bits);

// B4b. Experiment: sine of amplitude A = 1 - 2^(1-bits) (never clips, FS = 1), frequency 0.123456789
//      cycles/sample, n samples; return 10*log10( mean(x^2) / mean((q-x)^2) ).
double measured_sqnr_db(int bits, std::size_t n);

// B5. Lowest valid band-pass sampling rate for a band [f_low, f_high] (f_high > f_low >= 0).
//     For k = floor(f_high/B) down to 1: lo = 2*f_high/k, hi = (k>1) ? 2*f_low/(k-1) : +inf;
//     return lo for the first k with lo <= hi.
double bandpass_fs_min(double f_low_hz, double f_high_hz);

}  // namespace dsp
