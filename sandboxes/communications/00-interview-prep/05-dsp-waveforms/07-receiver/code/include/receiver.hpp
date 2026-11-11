#pragma once
// Pack 07 — matched filter, sync correlation, Gardner timing, Costas loop, AGC.
#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;
using cplx = std::complex<double>;

// B1. Output SNR of FIR h applied to pulse s (real), sampled at output index N-1 (N = s.size()):
//     y_s = sum_k h[k]*s[N-1-k];  SNR = y_s^2 / (noise_var * sum h^2).  Throws std::invalid_argument if
//     h.size() != s.size().  (Returns 0 if sum h^2 == 0.)
double matched_filter_output_snr(const std::vector<double>& h, const std::vector<double>& s, double noise_var);

// B2a. Normalised sliding correlation  rho[l] = |sum_k rx[l+k] conj(ref[k])|^2 / (sum|rx[l..]|^2 * sum|ref|^2)
//      for l = 0 .. rx.size()-ref.size().  (0 where the rx window has zero energy.)  Returns best (first max) lag.
struct SyncResult {
  std::size_t lag = 0;
  double metric = 0.0;
};
SyncResult sync_search(const std::vector<cplx>& rx, const std::vector<cplx>& ref);

// B2b. Coherent-correlation loss for a CFO of f cycles/symbol over len symbols: |sin(pi len f)/(len sin(pi f))|
//      (1.0 when f == 0).
double sync_cfo_loss(std::size_t len, double cfo_cycles_per_symbol);

// B2c. P(random bits match a `bits`-bit word with at most max_errors bit errors) = sum_{k<=d} C(bits,k) / 2^bits.
double sync_false_alarm_prob(unsigned bits, unsigned max_errors);

// B3a. Gardner timing error  e = (cur - prev) * mid   (e > 0 means sampling late).
double gardner_error(double prev_strobe, double mid, double cur_strobe);

// B3b. Gardner loop with linear interpolation on a real signal x (sps samples/symbol, sps even).
//      interp(x,t) = x[i](1-f) + x[i+1] f  with i = floor(t), f = t - i.
//        t = t_start;  y_prev = interp(t);  out = [y_prev];  corr = 0;  integ = 0
//        loop:  t = t + sps - corr
//               stop if floor(t) + 1 >= x.size()  or  t - sps/2 < 0
//               y = interp(t);  mid = interp(t - sps/2)
//               e = gardner_error(y_prev, mid, y)
//               integ += ki*e;  corr = kp*e + integ;  out.push_back(y);  y_prev = y
//      Returns all strobe values (empty if t_start is out of range).
//      Suggested gains for sps = 8: kp = 0.15, ki = 0.0015.
std::vector<double> gardner_recover(const std::vector<double>& x, std::size_t sps, double kp, double ki,
                                    double t_start);

// B4a. Second-order loop gains from the normalised noise bandwidth bn_t:
//      theta = bn_t/(zeta + 1/(4 zeta)); d = 1 + 2 zeta theta + theta^2; alpha = 4 zeta theta/d; beta = 4 theta^2/d.
struct LoopGains {
  double alpha = 0.0, beta = 0.0;
};
LoopGains loop_gains(double bn_t, double zeta);

// B4b. BPSK Costas loop.  For each input: y = x*exp(-j*phase); e = Re(y)*Im(y); freq += beta*e;
//      phase += freq + alpha*e; (wrap phase to (-pi, pi]).  Returns the derotated samples y.
std::vector<cplx> costas_bpsk(const std::vector<cplx>& x, double alpha, double beta);

// B5. Log-domain AGC: y = gain*x; gain *= exp(mu*(reference - |y|)); gain clamped to [1e-6, 1e6].
class Agc {
 public:
  Agc(double reference, double mu, double initial_gain = 1.0)
      : ref_(reference), mu_(mu), gain_(initial_gain) {}
  cplx process(cplx x);
  double gain() const { return gain_; }

 private:
  double ref_, mu_, gain_;
};

}  // namespace dsp
