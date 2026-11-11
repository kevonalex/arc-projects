#pragma once
// Pack 03 — DFT, FFT, windowing.
#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
using cplx = std::complex<double>;

// B1. Direct O(N^2) DFT: X[k] = sum_n x[n] exp(-j 2 pi k n / N).
std::vector<cplx> dft(const std::vector<cplx>& x);

// B2. Reverse the lowest `bits` bits of i.  bit_reverse(1,3)=4, bit_reverse(3,3)=6, bit_reverse(6,3)=3.
std::size_t bit_reverse(std::size_t i, unsigned bits);

// B3. Iterative radix-2 FFT (forward) / inverse (includes the 1/N).  Throws std::invalid_argument if the
//     size is not a power of two.  Empty input returns empty output.
std::vector<cplx> fft_radix2(const std::vector<cplx>& x);
std::vector<cplx> ifft_radix2(const std::vector<cplx>& X);

// B4. Periodic Hann window: w[n] = 0.5 - 0.5 cos(2 pi n / N), n = 0..N-1.
std::vector<double> hann_periodic(std::size_t n);

// B5. Fractional bin position of the strongest line using parabolic interpolation on log|X|, wrapping
//     around the ends (bins -1 and N map to N-1 and 0).  Result in [0, N).
double peak_bin_estimate(const std::vector<cplx>& X);

}  // namespace dsp
