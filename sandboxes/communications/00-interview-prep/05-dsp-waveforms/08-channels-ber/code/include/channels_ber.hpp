#pragma once
// Pack 08 — AWGN, fading, Doppler, Eb/N0 <-> SNR, Monte-Carlo BER.
#include <complex>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kSpeedOfLight = 299792458.0;  // m/s
using cplx = std::complex<double>;

// B1. Q(x) = 0.5*erfc(x/sqrt2).
double q_function(double x);
// BPSK (and per-bit QPSK) BER in AWGN: Q(sqrt(2*Eb/N0)); argument in dB.
double bpsk_ber_awgn(double ebn0_db);
// BPSK, flat Rayleigh fading, coherent detection, mean Eb/N0 g (linear): 0.5*(1 - sqrt(g/(1+g))).
double rayleigh_bpsk_ber(double ebn0_db);

// B2. Eb/N0 = SNR * B/Rb   (dB: ebn0 = snr + 10 log10(B/Rb)).
double snr_db_to_ebn0_db(double snr_db, double noise_bandwidth_hz, double bitrate_bps);
double ebn0_db_to_snr_db(double ebn0_db, double noise_bandwidth_hz, double bitrate_bps);
// Thermal noise floor in dBm: -174 + 10 log10(B) + noise_figure_db.
double noise_floor_dbm(double bandwidth_hz, double noise_figure_db);

// B3. Doppler shift v*fc/c (Hz) and coherence time 0.423/fd (seconds).
double doppler_shift_hz(double speed_mps, double carrier_hz);
double coherence_time_s(double doppler_hz);

// B4a. Add complex white Gaussian noise: each of I and Q gets N(0, n0/2).  Returns x + noise.
std::vector<cplx> add_awgn(const std::vector<cplx>& x, double n0, std::mt19937_64& rng);
// B4b. Monte-Carlo Gray-QPSK (Es = 1: symbols (+-1 +- j)/sqrt2) BER over AWGN at Eb/N0 (dB).
//      nsym = nbits/2 symbols; bits from rng; noise per component sigma = sqrt(N0/2), N0 = Eb/(Eb/N0), Eb = 0.5.
//      Return errors / (2*nsym).
double simulate_qpsk_ber(double ebn0_db, std::size_t nbits, std::uint64_t seed);

// B5. Monte-Carlo BPSK over flat Rayleigh fading with an INDEPENDENT CN(0,1) gain per bit (ideal interleaving),
//     coherent detection with known h: z = Re(conj(h) r), r = h*a*sqrt(Eb=1) + w, w ~ CN(0, N0), N0 = 1/(Eb/N0).
double simulate_bpsk_rayleigh_ber(double ebn0_db, std::size_t nbits, std::uint64_t seed);

// B6a. Clarke sum-of-sinusoids with n_paths: h[n] = (1/sqrt(M)) sum_m exp(j(2 pi fd cos(alpha_m) n + phi_m)),
//      alpha_m, phi_m ~ U[0, 2 pi) drawn from mt19937_64(seed).  fd in cycles/sample.
std::vector<cplx> rayleigh_fading(std::size_t n, double fd_cycles_per_sample, std::uint64_t seed,
                                  std::size_t n_paths = 64);
// B6b. Rician: sqrt(K/(K+1)) exp(j(2 pi fd cos(theta0) n + phi0)) + sqrt(1/(K+1)) * rayleigh_fading(...).
std::vector<cplx> rician_fading(std::size_t n, double fd_cycles_per_sample, double k_factor,
                                std::uint64_t seed, std::size_t n_paths = 64);

}  // namespace dsp
