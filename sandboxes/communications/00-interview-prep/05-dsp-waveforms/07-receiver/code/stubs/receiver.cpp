// STUDENT FILE — replace each TODO.  Tests fail until you do.
#include "receiver.hpp"

#include <stdexcept>

namespace dsp {

double matched_filter_output_snr([[maybe_unused]] const std::vector<double>& h,
                                 [[maybe_unused]] const std::vector<double>& s,
                                 [[maybe_unused]] double noise_var) {
  // TODO B1
  return -1.0;
}

SyncResult sync_search([[maybe_unused]] const std::vector<cplx>& rx,
                       [[maybe_unused]] const std::vector<cplx>& ref) {
  // TODO B2a
  return SyncResult{9999, -1.0};
}

double sync_cfo_loss([[maybe_unused]] std::size_t len, [[maybe_unused]] double cfo_cycles_per_symbol) {
  // TODO B2b
  return -1.0;
}

double sync_false_alarm_prob([[maybe_unused]] unsigned bits, [[maybe_unused]] unsigned max_errors) {
  // TODO B2c
  return -1.0;
}

double gardner_error([[maybe_unused]] double prev_strobe, [[maybe_unused]] double mid,
                     [[maybe_unused]] double cur_strobe) {
  // TODO B3a
  return 0.0;
}

std::vector<double> gardner_recover([[maybe_unused]] const std::vector<double>& x,
                                    [[maybe_unused]] std::size_t sps, [[maybe_unused]] double kp,
                                    [[maybe_unused]] double ki, [[maybe_unused]] double t_start) {
  // TODO B3b
  return {};
}

LoopGains loop_gains([[maybe_unused]] double bn_t, [[maybe_unused]] double zeta) {
  // TODO B4a
  return LoopGains{};
}

std::vector<cplx> costas_bpsk(const std::vector<cplx>& x, [[maybe_unused]] double alpha,
                              [[maybe_unused]] double beta) {
  // TODO B4b
  return x;  // pass-through placeholder
}

cplx Agc::process(cplx x) {
  // TODO B5
  return x;
}

}  // namespace dsp
