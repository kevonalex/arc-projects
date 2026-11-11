#include "hardware_q15.hpp"

#include <algorithm>
#include <limits>

namespace dsp {

namespace {
q15_t sat16(std::int64_t v) {
  if (v > std::numeric_limits<q15_t>::max()) return std::numeric_limits<q15_t>::max();
  if (v < std::numeric_limits<q15_t>::min()) return std::numeric_limits<q15_t>::min();
  return static_cast<q15_t>(v);
}
}  // namespace

q15_t float_to_q15(double x) {
  const double scaled = x * 32768.0;
  if (scaled >= 32767.0) return 32767;  // also catches +1.0 and anything larger
  if (scaled <= -32768.0) return -32768;
  return static_cast<q15_t>(std::lround(scaled));
}

double q15_to_float(q15_t v) { return static_cast<double>(v) / 32768.0; }

q15_t q15_add_sat(q15_t a, q15_t b) { return sat16(static_cast<std::int32_t>(a) + static_cast<std::int32_t>(b)); }

q15_t q15_mul(q15_t a, q15_t b) {
  const std::int32_t p = static_cast<std::int32_t>(a) * static_cast<std::int32_t>(b);  // |p| <= 2^30
  return sat16((p + (1 << 14)) >> 15);  // arithmetic shift of a negative value: floor (all mainstream compilers)
}

q31_t q31_mul(q31_t a, q31_t b) {
  const std::int64_t p = static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b);  // |p| <= 2^62
  const std::int64_t r = (p + (std::int64_t{1} << 30)) >> 31;
  if (r > std::numeric_limits<q31_t>::max()) return std::numeric_limits<q31_t>::max();
  if (r < std::numeric_limits<q31_t>::min()) return std::numeric_limits<q31_t>::min();
  return static_cast<q31_t>(r);
}

std::vector<q15_t> q15_fir(const std::vector<q15_t>& x, const std::vector<q15_t>& h) {
  std::vector<q15_t> y(x.size());
  for (std::size_t n = 0; n < x.size(); ++n) {
    std::int64_t acc = 0;
    const std::size_t kmax = std::min(h.size(), n + 1);
    for (std::size_t k = 0; k < kmax; ++k) acc += static_cast<std::int64_t>(h[k]) * static_cast<std::int64_t>(x[n - k]);
    y[n] = sat16((acc + (std::int64_t{1} << 14)) >> 15);
  }
  return y;
}

int guard_bits(const std::vector<double>& h) {
  double s = 0.0;
  for (double v : h) s += std::fabs(v);
  if (s <= 1.0) return 0;
  return static_cast<int>(std::ceil(std::log2(s)));
}

std::vector<cplx> remove_dc(const std::vector<cplx>& x) {
  if (x.empty()) return x;
  cplx mean(0.0, 0.0);
  for (const auto& v : x) mean += v;
  mean /= static_cast<double>(x.size());
  std::vector<cplx> y(x.size());
  for (std::size_t n = 0; n < x.size(); ++n) y[n] = x[n] - mean;
  return y;
}

std::vector<cplx> correct_iq_imbalance(const std::vector<cplx>& x) {
  double sii = 0.0, siq = 0.0, sqq = 0.0;
  for (const auto& v : x) {
    sii += v.real() * v.real();
    siq += v.real() * v.imag();
    sqq += v.imag() * v.imag();
  }
  if (sii <= 0.0) return x;
  const double rho = siq / sii;
  const double p1 = sqq - rho * siq;  // power of Q - rho I
  if (p1 <= 0.0) return x;
  const double c = std::sqrt(sii / p1);
  std::vector<cplx> y(x.size());
  for (std::size_t n = 0; n < x.size(); ++n) {
    const double i = x[n].real();
    y[n] = cplx(i, c * (x[n].imag() - rho * i));
  }
  return y;
}

double cascade_nf_db(const std::vector<Stage>& stages) {
  double f_total = 1.0;
  double g_before = 1.0;
  bool first = true;
  for (const auto& s : stages) {
    const double f = std::pow(10.0, s.nf_db / 10.0);
    f_total += first ? (f - 1.0) : (f - 1.0) / g_before;
    first = false;
    g_before *= std::pow(10.0, s.gain_db / 10.0);
  }
  return 10.0 * std::log10(f_total);
}

double ppm_to_hz(double ppm, double carrier_hz) { return ppm * 1e-6 * carrier_hz; }

double cfo_phase_per_symbol_deg(double cfo_hz, double symbol_rate) { return 360.0 * cfo_hz / symbol_rate; }

double phase_noise_linewidth_hz(double sigma_rad, double fs_hz) { return sigma_rad * sigma_rad * fs_hz / (2.0 * kPi); }

double reciprocal_mixing_floor_dbm(double blocker_dbm, double l_dbc_hz, double bw_hz) {
  return blocker_dbm + l_dbc_hz + 10.0 * std::log10(bw_hz);
}

}  // namespace dsp
