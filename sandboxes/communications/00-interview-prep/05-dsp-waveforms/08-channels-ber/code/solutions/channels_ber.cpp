#include "channels_ber.hpp"

#include <cmath>

namespace dsp {

double q_function(double x) { return 0.5 * std::erfc(x / std::sqrt(2.0)); }

double bpsk_ber_awgn(double ebn0_db) {
  const double g = std::pow(10.0, ebn0_db / 10.0);
  return q_function(std::sqrt(2.0 * g));
}

double rayleigh_bpsk_ber(double ebn0_db) {
  const double g = std::pow(10.0, ebn0_db / 10.0);
  return 0.5 * (1.0 - std::sqrt(g / (1.0 + g)));
}

double snr_db_to_ebn0_db(double snr_db, double noise_bandwidth_hz, double bitrate_bps) {
  return snr_db + 10.0 * std::log10(noise_bandwidth_hz / bitrate_bps);
}

double ebn0_db_to_snr_db(double ebn0_db, double noise_bandwidth_hz, double bitrate_bps) {
  return ebn0_db - 10.0 * std::log10(noise_bandwidth_hz / bitrate_bps);
}

double noise_floor_dbm(double bandwidth_hz, double noise_figure_db) {
  return -174.0 + 10.0 * std::log10(bandwidth_hz) + noise_figure_db;
}

double doppler_shift_hz(double speed_mps, double carrier_hz) { return speed_mps * carrier_hz / kSpeedOfLight; }

double coherence_time_s(double doppler_hz) { return 0.423 / doppler_hz; }

std::vector<cplx> add_awgn(const std::vector<cplx>& x, double n0, std::mt19937_64& rng) {
  std::normal_distribution<double> g(0.0, std::sqrt(n0 / 2.0));
  std::vector<cplx> y(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) {
    const double re = g(rng);
    const double im = g(rng);
    y[i] = x[i] + cplx(re, im);
  }
  return y;
}

double simulate_qpsk_ber(double ebn0_db, std::size_t nbits, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  const double ebn0 = std::pow(10.0, ebn0_db / 10.0);
  const double eb = 0.5;
  const double n0 = eb / ebn0;
  std::normal_distribution<double> g(0.0, std::sqrt(n0 / 2.0));
  const double a = 1.0 / std::sqrt(2.0);
  const std::size_t nsym = nbits / 2;
  std::size_t errors = 0;
  for (std::size_t s = 0; s < nsym; ++s) {
    const std::uint64_t r = rng();
    const bool b1 = (r & 1u) != 0u;
    const bool b0 = (r & 2u) != 0u;
    const double i = (b1 ? -a : a) + g(rng);
    const double q = (b0 ? -a : a) + g(rng);
    if ((i < 0.0) != b1) ++errors;
    if ((q < 0.0) != b0) ++errors;
  }
  return nsym == 0 ? 0.0 : static_cast<double>(errors) / static_cast<double>(2 * nsym);
}

double simulate_bpsk_rayleigh_ber(double ebn0_db, std::size_t nbits, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  const double n0 = 1.0 / std::pow(10.0, ebn0_db / 10.0);  // Eb = 1
  std::normal_distribution<double> unit(0.0, 1.0);
  const double s_h = std::sqrt(0.5);
  const double s_w = std::sqrt(n0 / 2.0);
  std::size_t errors = 0;
  for (std::size_t k = 0; k < nbits; ++k) {
    const bool bit = (rng() & 1u) != 0u;
    const double a = bit ? -1.0 : 1.0;
    const cplx h(s_h * unit(rng), s_h * unit(rng));
    const cplx w(s_w * unit(rng), s_w * unit(rng));
    const cplx r = h * a + w;
    const double z = (std::conj(h) * r).real();
    if ((z < 0.0) != bit) ++errors;
  }
  return nbits == 0 ? 0.0 : static_cast<double>(errors) / static_cast<double>(nbits);
}

std::vector<cplx> rayleigh_fading(std::size_t n, double fd, std::uint64_t seed, std::size_t n_paths) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(0.0, 2.0 * kPi);
  std::vector<double> w(n_paths), phi(n_paths);
  for (std::size_t m = 0; m < n_paths; ++m) {
    w[m] = 2.0 * kPi * fd * std::cos(u(rng));
    phi[m] = u(rng);
  }
  const double norm = 1.0 / std::sqrt(static_cast<double>(n_paths));
  std::vector<cplx> h(n);
  for (std::size_t k = 0; k < n; ++k) {
    cplx acc(0.0, 0.0);
    for (std::size_t m = 0; m < n_paths; ++m) acc += std::polar(1.0, w[m] * static_cast<double>(k) + phi[m]);
    h[k] = acc * norm;
  }
  return h;
}

std::vector<cplx> rician_fading(std::size_t n, double fd, double k_factor, std::uint64_t seed,
                                std::size_t n_paths) {
  std::vector<cplx> h = rayleigh_fading(n, fd, seed, n_paths);
  std::mt19937_64 rng(seed ^ 0x9E3779B97F4A7C15ULL);
  std::uniform_real_distribution<double> u(0.0, 2.0 * kPi);
  const double theta0 = u(rng);
  const double phi0 = u(rng);
  const double los = std::sqrt(k_factor / (k_factor + 1.0));
  const double scat = std::sqrt(1.0 / (k_factor + 1.0));
  for (std::size_t k = 0; k < n; ++k) {
    h[k] = los * std::polar(1.0, 2.0 * kPi * fd * std::cos(theta0) * static_cast<double>(k) + phi0) + scat * h[k];
  }
  return h;
}

}  // namespace dsp
