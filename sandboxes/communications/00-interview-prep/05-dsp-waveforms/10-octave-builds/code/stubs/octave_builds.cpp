// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "octave_builds.hpp"

#include <stdexcept>
#include <utility>

namespace dsp {

std::vector<cplx> fft(const std::vector<cplx>& x) {
  // TODO B1: size check, bit-reversal permutation, log2(N) stages of butterflies
  return std::vector<cplx>(x.size(), cplx(0.0, 0.0));
}

FirFilter::FirFilter(std::vector<double> taps) : taps_(std::move(taps)) {
  // TODO B2: throw if empty; size the delay line
}

double FirFilter::process([[maybe_unused]] double x) {
  // TODO B2
  return 0.0;
}

std::vector<double> FirFilter::process_block(const std::vector<double>& x) {
  // TODO B2: call process() for each sample
  return std::vector<double>(x.size(), 0.0);
}

void FirFilter::reset() {
  // TODO B2
}

std::vector<int> fsk4_levels(const std::vector<int>& dibits) {
  // TODO B3
  return std::vector<int>(dibits.size(), 0);
}

std::vector<cplx> fsk4_modulate(const std::vector<int>& dibits, std::size_t sps, [[maybe_unused]] double dev_unit_hz,
                                [[maybe_unused]] double fs_hz) {
  // TODO B3
  return std::vector<cplx>(dibits.size() * sps, cplx(0.0, 0.0));
}

std::vector<int> sync_hamming_distances([[maybe_unused]] const std::vector<int>& bits,
                                        [[maybe_unused]] const std::vector<int>& word) {
  // TODO B4
  return {};
}

std::vector<std::size_t> sync_detect([[maybe_unused]] const std::vector<int>& distances,
                                     [[maybe_unused]] int max_errors) {
  // TODO B4
  return {};
}

double estimate_cfo([[maybe_unused]] const std::vector<cplx>& rx, [[maybe_unused]] const std::vector<double>& known) {
  // TODO B5
  return 0.0;
}

std::vector<cplx> correct_cfo(const std::vector<cplx>& x, [[maybe_unused]] double f_cycles_per_sample) {
  // TODO B5
  return x;
}

double qpsk_ber_theory([[maybe_unused]] double ebn0_db) {
  // TODO B6
  return -1.0;
}

double simulate_qpsk_ber([[maybe_unused]] double ebn0_db, [[maybe_unused]] std::size_t nbits,
                         [[maybe_unused]] std::uint64_t seed) {
  // TODO B6
  return 0.5;
}

std::vector<std::int16_t> q15_fir(const std::vector<std::int16_t>& x, [[maybe_unused]] const std::vector<std::int16_t>& h) {
  // TODO B7
  return std::vector<std::int16_t>(x.size(), 0);
}

}  // namespace dsp
