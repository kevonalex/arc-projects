#pragma once
// Pack 06 — raised-cosine pulse shaping and eye diagrams.
#include <cstddef>
#include <vector>

namespace dsp {

inline constexpr double kPi = 3.14159265358979323846;

// B1. Raised cosine taps, length 2*span*sps + 1, t = (n - span*sps)/sps  (in symbol periods), peak 1 at centre.
//     h = sinc(t) cos(pi beta t) / (1 - (2 beta t)^2); special cases t==0 -> 1 and |t| == 1/(2 beta)
//     -> (pi/4) sinc(1/(2 beta)).  beta in [0,1]; beta==0 gives the plain sinc.
std::vector<double> raised_cosine(double beta, std::size_t sps, std::size_t span);

// B2. Root raised cosine, same indexing, scaled so that sum(h^2) == 1.
//     t==0 -> 1 - beta + 4 beta/pi ; |t| == 1/(4 beta) -> (beta/sqrt2)[(1+2/pi) sin(pi/(4 beta)) + (1-2/pi) cos(pi/(4 beta))]
//     else [sin(pi t (1-beta)) + 4 beta t cos(pi t (1+beta))] / [pi t (1 - (4 beta t)^2)].
std::vector<double> root_raised_cosine(double beta, std::size_t sps, std::size_t span);

// B3. Zero-stuff `symbols` by sps (symbol k at index k*sps) and convolve fully with taps.
//     Output length = symbols.size()*sps + taps.size() - 1 (empty if either input is empty).
std::vector<double> pulse_shape(const std::vector<double>& symbols, const std::vector<double>& taps,
                                std::size_t sps);

// B4. Fold y into overlapping traces of 2*sps+1 points: trace k = y[offset + k*sps ... offset + k*sps + 2*sps].
//     Only complete traces are returned.
std::vector<std::vector<double>> eye_traces(const std::vector<double>& y, std::size_t sps, std::size_t offset);

// B5. Vertical eye opening for a two-level signal.  Samples at indices offset + k*sps for
//     k = guard .. kmax-guard (inclusive), kmax = (y.size()-1-offset)/sps.
//     Returns min(samples > 0) - max(samples < 0), or 0.0 if either rail is empty / no samples.
double eye_height(const std::vector<double>& y, std::size_t sps, std::size_t offset, std::size_t guard);

// B6. Peak ISI of a symmetric pulse: c = (p.size()-1)/2;  max over m != 0 (that stay in range) of |p[c+m*sps]| / |p[c]|.
double isi_residual(const std::vector<double>& pulse, std::size_t sps);

}  // namespace dsp
