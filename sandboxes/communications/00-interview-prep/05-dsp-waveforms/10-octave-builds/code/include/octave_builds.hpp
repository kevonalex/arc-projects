#pragma once
// Pack 10 — build C++ blocks and check them against an Octave model.
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
using cplx = std::complex<double>;

// ------------------------------------------------------------------ B1: FFT
// Iterative radix-2 decimation-in-time FFT, X[k] = sum_n x[n] exp(-j 2 pi k n / N) (no scaling).
// Throws std::invalid_argument if x.size() is 0 or not a power of two.
std::vector<cplx> fft(const std::vector<cplx>& x);

// ------------------------------------------------------------------ B2: streaming FIR
// y[n] = sum_{k=0}^{N-1} taps[k] * x[n-k], with x[n<0] = 0.  Output index n matches Octave filter(h,1,x).
// Streaming means: feeding samples one at a time, or in blocks of any size, gives identical output.
// Throws std::invalid_argument if taps is empty.
class FirFilter {
 public:
  explicit FirFilter(std::vector<double> taps);
  double process(double x);
  std::vector<double> process_block(const std::vector<double>& x);
  void reset();  // clears the delay line
  std::size_t num_taps() const { return taps_.size(); }

 private:
  std::vector<double> taps_;
  std::vector<double> state_;  // circular delay line, size == taps_.size()
  std::size_t pos_ = 0;
};

// ------------------------------------------------------------------ B3: 4FSK modulator
// Dibit (value 0..3 = two bits b1 b0) -> level: 01 -> +3, 00 -> +1, 10 -> -1, 11 -> -3   (VERIFY vs TIA-102.BAAA).
// Throws std::invalid_argument if a dibit is outside 0..3.
std::vector<int> fsk4_levels(const std::vector<int>& dibits);
// Rectangular-frequency-pulse 4FSK at complex baseband.  Instantaneous frequency during a symbol is
// level * dev_unit_hz.  phi[0] = 0; phi[n+1] = phi[n] + 2 pi f[n] / fs_hz; out[n] = exp(j phi[n]).
// Output length = dibits.size() * sps.
std::vector<cplx> fsk4_modulate(const std::vector<int>& dibits, std::size_t sps, double dev_unit_hz, double fs_hz);

// ------------------------------------------------------------------ B4: 48-bit sync correlator (integer, bit-exact)
// d[l] = number of positions where bits[l+i] != word[i], l = 0 .. bits.size()-word.size().
// Empty result if word is empty or longer than bits.
std::vector<int> sync_hamming_distances(const std::vector<int>& bits, const std::vector<int>& word);
// Lags with distance <= max_errors, ascending.
std::vector<std::size_t> sync_detect(const std::vector<int>& distances, int max_errors);

// ------------------------------------------------------------------ B5: CFO estimate and correction
// Data-aided estimate (cycles/sample) from rx[n] = known[n] * exp(j(2 pi f n + theta)) + noise, known real (+-1):
//   r[n] = rx[n]*known[n];  f = arg( sum_{n>=1} r[n] conj(r[n-1]) ) / (2 pi).
// Throws std::invalid_argument if sizes differ or are < 2.
double estimate_cfo(const std::vector<cplx>& rx, const std::vector<double>& known);
// out[n] = x[n] * exp(-j 2 pi f n).
std::vector<cplx> correct_cfo(const std::vector<cplx>& x, double f_cycles_per_sample);

// ------------------------------------------------------------------ B6: QPSK over AWGN vs theory
// Theory per bit: Q(sqrt(2 Eb/N0)) = 0.5 erfc(sqrt(Eb/N0)); argument in dB.
double qpsk_ber_theory(double ebn0_db);
// Monte-Carlo Gray QPSK (Es = 1, Eb = 0.5), N0 = Eb/(Eb/N0), noise N(0, N0/2) per component, std::mt19937_64(seed),
// std::normal_distribution.  nsym = nbits/2.  NOTE: results are statistical and NOT identical across standard libraries.
double simulate_qpsk_ber(double ebn0_db, std::size_t nbits, std::uint64_t seed);

// ------------------------------------------------------------------ B7: Q15 FIR (integer, bit-exact)
// y[n] = sat16( (sum_k h[k] x[n-k] + 2^14) >> 15 ), int64 accumulator, x[n<0] = 0.  y.size() == x.size().
std::vector<std::int16_t> q15_fir(const std::vector<std::int16_t>& x, const std::vector<std::int16_t>& h);

}  // namespace dsp
