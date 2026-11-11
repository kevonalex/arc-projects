// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "channels_ber.hpp"

namespace dsp {

double q_function([[maybe_unused]] double x) {
  // TODO B1
  return -1.0;
}

double bpsk_ber_awgn([[maybe_unused]] double ebn0_db) {
  // TODO B1
  return -1.0;
}

double rayleigh_bpsk_ber([[maybe_unused]] double ebn0_db) {
  // TODO B1
  return -1.0;
}

double snr_db_to_ebn0_db([[maybe_unused]] double snr_db, [[maybe_unused]] double noise_bandwidth_hz,
                         [[maybe_unused]] double bitrate_bps) {
  // TODO B2
  return 0.0;
}

double ebn0_db_to_snr_db([[maybe_unused]] double ebn0_db, [[maybe_unused]] double noise_bandwidth_hz,
                         [[maybe_unused]] double bitrate_bps) {
  // TODO B2
  return 0.0;
}

double noise_floor_dbm([[maybe_unused]] double bandwidth_hz, [[maybe_unused]] double noise_figure_db) {
  // TODO B2
  return 0.0;
}

double doppler_shift_hz([[maybe_unused]] double speed_mps, [[maybe_unused]] double carrier_hz) {
  // TODO B3
  return 0.0;
}

double coherence_time_s([[maybe_unused]] double doppler_hz) {
  // TODO B3
  return 0.0;
}

std::vector<cplx> add_awgn(const std::vector<cplx>& x, [[maybe_unused]] double n0,
                           [[maybe_unused]] std::mt19937_64& rng) {
  // TODO B4a
  return x;
}

double simulate_qpsk_ber([[maybe_unused]] double ebn0_db, [[maybe_unused]] std::size_t nbits,
                         [[maybe_unused]] std::uint64_t seed) {
  // TODO B4b
  return 0.5;
}

double simulate_bpsk_rayleigh_ber([[maybe_unused]] double ebn0_db, [[maybe_unused]] std::size_t nbits,
                                  [[maybe_unused]] std::uint64_t seed) {
  // TODO B5
  return 0.5;
}

std::vector<cplx> rayleigh_fading(std::size_t n, [[maybe_unused]] double fd_cycles_per_sample,
                                  [[maybe_unused]] std::uint64_t seed, [[maybe_unused]] std::size_t n_paths) {
  // TODO B6a
  return std::vector<cplx>(n, cplx(0.0, 0.0));
}

std::vector<cplx> rician_fading(std::size_t n, [[maybe_unused]] double fd_cycles_per_sample,
                                [[maybe_unused]] double k_factor, [[maybe_unused]] std::uint64_t seed,
                                [[maybe_unused]] std::size_t n_paths) {
  // TODO B6b
  return std::vector<cplx>(n, cplx(0.0, 0.0));
}

}  // namespace dsp
