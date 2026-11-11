// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "hardware_q15.hpp"

namespace dsp {

q15_t float_to_q15([[maybe_unused]] double x) {
  // TODO B1: lround(x * 32768), clamp to int16 range
  return 0;
}

double q15_to_float([[maybe_unused]] q15_t v) {
  // TODO B1
  return 0.0;
}

q15_t q15_add_sat([[maybe_unused]] q15_t a, [[maybe_unused]] q15_t b) {
  // TODO B2: add in int32, then clamp
  return 0;
}

q15_t q15_mul([[maybe_unused]] q15_t a, [[maybe_unused]] q15_t b) {
  // TODO B3: (a*b + (1<<14)) >> 15 in int32, clamp to 32767 (the -1 * -1 case)
  return 0;
}

q31_t q31_mul([[maybe_unused]] q31_t a, [[maybe_unused]] q31_t b) {
  // TODO B3: same with int64
  return 0;
}

std::vector<q15_t> q15_fir(const std::vector<q15_t>& x, [[maybe_unused]] const std::vector<q15_t>& h) {
  // TODO B4: int64 accumulator, round (+1<<14), shift 15, saturate
  return std::vector<q15_t>(x.size(), 0);
}

int guard_bits([[maybe_unused]] const std::vector<double>& h) {
  // TODO B5
  return -1;
}

std::vector<cplx> remove_dc(const std::vector<cplx>& x) {
  // TODO B6
  return x;
}

std::vector<cplx> correct_iq_imbalance(const std::vector<cplx>& x) {
  // TODO B7
  return x;
}

double cascade_nf_db([[maybe_unused]] const std::vector<Stage>& stages) {
  // TODO B8
  return 0.0;
}

double ppm_to_hz([[maybe_unused]] double ppm, [[maybe_unused]] double carrier_hz) {
  // TODO B9a
  return 0.0;
}

double cfo_phase_per_symbol_deg([[maybe_unused]] double cfo_hz, [[maybe_unused]] double symbol_rate) {
  // TODO B9b
  return 0.0;
}

double phase_noise_linewidth_hz([[maybe_unused]] double sigma_rad, [[maybe_unused]] double fs_hz) {
  // TODO B9c
  return 0.0;
}

double reciprocal_mixing_floor_dbm([[maybe_unused]] double blocker_dbm, [[maybe_unused]] double l_dbc_hz,
                                   [[maybe_unused]] double bw_hz) {
  // TODO B9d
  return 0.0;
}

}  // namespace dsp
