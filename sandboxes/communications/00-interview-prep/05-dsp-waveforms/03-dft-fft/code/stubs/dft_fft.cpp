// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "dft_fft.hpp"

#include <stdexcept>

namespace dsp {

std::vector<cplx> dft(const std::vector<cplx>& x) {
  // TODO B1
  return std::vector<cplx>(x.size());
}

std::size_t bit_reverse([[maybe_unused]] std::size_t i, [[maybe_unused]] unsigned bits) {
  // TODO B2
  return 0;
}

std::vector<cplx> fft_radix2(const std::vector<cplx>& x) {
  // TODO B3: bit-reverse permute, then log2(N) stages of butterflies.
  // Must throw std::invalid_argument when x.size() is not a power of two.
  return std::vector<cplx>(x.size());
}

std::vector<cplx> ifft_radix2(const std::vector<cplx>& X) {
  // TODO B3 (inverse): conj trick or conjugate twiddles, then divide by N.
  return std::vector<cplx>(X.size());
}

std::vector<double> hann_periodic(std::size_t n) {
  // TODO B4
  return std::vector<double>(n, 0.0);
}

double peak_bin_estimate([[maybe_unused]] const std::vector<cplx>& X) {
  // TODO B5
  return -1.0;
}

}  // namespace dsp
