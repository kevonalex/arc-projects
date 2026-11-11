#pragma once
// Pack 04 — convolution, FIR design, biquads, polyphase rate change.
#include <cstddef>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;

// B1. Full convolution, size = x.size() + h.size() - 1 (empty if either input is empty).
std::vector<double> convolve(const std::vector<double>& x, const std::vector<double>& h);

// B2. Symmetric Hamming window, w[n] = 0.54 - 0.46 cos(2 pi n/(N-1)).  N==1 -> {1.0}; N==0 -> {}.
std::vector<double> hamming(std::size_t n);

// B3. Low-pass FIR: h[n] = 2fc sinc(2fc m) w[n], m = n - (N-1)/2, sinc(x)=sin(pi x)/(pi x), sinc(0)=1,
//     w = hamming(N), then normalised so sum(h) == 1.  fc in cycles/sample, 0 < fc < 0.5.
std::vector<double> windowed_sinc_lowpass(std::size_t ntaps, double fc_cycles_per_sample);

struct BiquadCoeffs {
  double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;  // a0 already normalised to 1
};

// B4a. RBJ cookbook low-pass.  w0 = 2 pi f0, alpha = sin(w0)/(2Q), normalise by a0 = 1 + alpha.
BiquadCoeffs biquad_lowpass(double f0_cycles_per_sample, double q);

// B4b. Transposed direct-form II:  y = b0 x + s1;  s1 = b1 x - a1 y + s2;  s2 = b2 x - a2 y.
class Biquad {
 public:
  explicit Biquad(const BiquadCoeffs& c) : c_(c) {}
  double process(double x);
  void reset();

 private:
  BiquadCoeffs c_;
  double s1_ = 0.0, s2_ = 0.0;
};

// B5. y[m] = sum_k h[k] x[mM - k] (x[negative] = 0), m = 0 .. ceil(nx/M)-1.  Use the polyphase structure.
std::vector<double> decimate_polyphase(const std::vector<double>& x, std::size_t m_factor,
                                       const std::vector<double>& h);

// B6. Equivalent to: zero-stuff x by L, convolve with h, keep the first nx*L samples.
//     Use y[nL+p] = sum_q h[qL+p] x[n-q].
std::vector<double> interpolate_polyphase(const std::vector<double>& x, std::size_t l_factor,
                                          const std::vector<double>& h);

}  // namespace dsp
