// viterbi.h  --  REFERENCE SOLUTION for Pack 06.
// Bits are stored one per element (std::vector<uint8_t> holding 0 or 1) so every step is visible.
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace cv {

using Bits = std::vector<uint8_t>;

// ---------------------------------------------------------------------------------------------
// Rate-1/2, constraint length K=3 convolutional code, generators (7,5) octal = (111, 101) binary.
// State = the two previous input bits: bit1 = u[n-1] (most recent), bit0 = u[n-2].
//   out0 = u[n] ^ u[n-1] ^ u[n-2]      (generator 111 = 7)
//   out1 = u[n]          ^ u[n-2]      (generator 101 = 5)
// ---------------------------------------------------------------------------------------------
namespace detail {
inline unsigned expected_pair(unsigned state, unsigned u) {
  const unsigned u1 = (state >> 1) & 1u;
  const unsigned u2 = state & 1u;
  const unsigned o0 = u ^ u1 ^ u2;
  const unsigned o1 = u ^ u2;
  return (o0 << 1) | o1;  // 2-bit symbol: first output bit is the high bit
}
inline unsigned next_state(unsigned state, unsigned u) { return ((u << 1) | (state >> 1)) & 3u; }
inline unsigned popcount2(unsigned x) { return (x & 1u) + ((x >> 1) & 1u); }
}  // namespace detail

inline Bits conv_encode(const Bits& bits, bool terminate = true) {
  Bits out;
  out.reserve((bits.size() + 2) * 2);
  unsigned state = 0;
  auto step = [&](unsigned u) {
    const unsigned pair = detail::expected_pair(state, u);
    out.push_back(static_cast<uint8_t>((pair >> 1) & 1u));
    out.push_back(static_cast<uint8_t>(pair & 1u));
    state = detail::next_state(state, u);
  };
  for (uint8_t b : bits) step(b & 1u);
  if (terminate) {
    step(0);  // two zero "tail" bits flush the register back to state 0
    step(0);
  }
  return out;
}

struct ViterbiResult {
  Bits bits;        // decoded data bits (tail removed)
  unsigned metric;  // total Hamming distance between rx and the winning path's coded bits
};

// Hard-decision Viterbi decoder for a TERMINATED stream (ends in state 0).
// Tie-breaking (so every implementation gives the same answer): when comparing candidate paths
// into a state, previous states are visited in ascending order and u=0 before u=1, and a new
// candidate only wins if it is STRICTLY better.
inline ViterbiResult viterbi_decode(const Bits& rx) {
  if (rx.size() % 2 != 0 || rx.size() < 4) return ViterbiResult{Bits{}, 0};
  const size_t steps = rx.size() / 2;
  constexpr unsigned kInf = 0x3FFFFFFFu;
  struct Survivor {
    uint8_t prev;
    uint8_t u;
  };
  std::vector<std::array<Survivor, 4>> surv(steps);
  std::array<unsigned, 4> metric{kInf, kInf, kInf, kInf};
  metric[0] = 0;  // encoder starts in state 0
  for (size_t t = 0; t < steps; ++t) {
    const unsigned r = (static_cast<unsigned>(rx[2 * t] & 1u) << 1) | (rx[2 * t + 1] & 1u);
    std::array<unsigned, 4> next{kInf, kInf, kInf, kInf};
    for (unsigned ps = 0; ps < 4; ++ps) {
      if (metric[ps] >= kInf) continue;
      for (unsigned u = 0; u < 2; ++u) {
        const unsigned ns = detail::next_state(ps, u);
        const unsigned cost = metric[ps] + detail::popcount2(detail::expected_pair(ps, u) ^ r);
        if (cost < next[ns]) {
          next[ns] = cost;
          surv[t][ns] = Survivor{static_cast<uint8_t>(ps), static_cast<uint8_t>(u)};
        }
      }
    }
    metric = next;
  }
  // Traceback from state 0 (the tail forces the encoder to end there).
  Bits rev;
  rev.reserve(steps);
  unsigned state = 0;
  for (size_t t = steps; t-- > 0;) {
    const Survivor s = surv[t][state];
    rev.push_back(s.u);
    state = s.prev;
  }
  std::reverse(rev.begin(), rev.end());
  rev.resize(steps - 2);  // drop the 2 tail bits
  return ViterbiResult{rev, metric[0]};
}

// ---------------------------------------------------------------------------------------------
// Block interleaver: WRITE row by row into a rows x cols grid, READ column by column.
// ---------------------------------------------------------------------------------------------
inline Bits interleave(const Bits& in, size_t rows, size_t cols) {
  if (rows == 0 || cols == 0 || in.size() != rows * cols) return Bits{};
  Bits out(in.size());
  for (size_t r = 0; r < rows; ++r)
    for (size_t c = 0; c < cols; ++c) out[c * rows + r] = in[r * cols + c];
  return out;
}
inline Bits deinterleave(const Bits& in, size_t rows, size_t cols) {
  if (rows == 0 || cols == 0 || in.size() != rows * cols) return Bits{};
  Bits out(in.size());
  for (size_t r = 0; r < rows; ++r)
    for (size_t c = 0; c < cols; ++c) out[r * cols + c] = in[c * rows + r];
  return out;
}

// ---------------------------------------------------------------------------------------------
// LFSR keystream, x^7 + x^4 + 1 (Fibonacci form).  State bit k-1 holds the bit that was
// produced k steps ago.  Each step: fb = s6 ^ s3; shift left; fb enters at bit 0; output fb.
// seed7 must be nonzero (an all-zero register stays zero forever).
// ---------------------------------------------------------------------------------------------
inline Bits lfsr_keystream(size_t n, uint8_t seed7) {
  Bits ks;
  ks.reserve(n);
  unsigned s = seed7 & 0x7Fu;
  for (size_t i = 0; i < n; ++i) {
    const unsigned fb = ((s >> 6) ^ (s >> 3)) & 1u;
    s = ((s << 1) | fb) & 0x7Fu;
    ks.push_back(static_cast<uint8_t>(fb));
  }
  return ks;
}

// Additive (synchronous) scrambler: data XOR keystream.  Descrambling is the SAME operation
// because (d ^ k) ^ k = d.  Both ends must start from the same seed at the same bit.
inline Bits scramble(const Bits& in, uint8_t seed7) {
  const Bits ks = lfsr_keystream(in.size(), seed7);
  Bits out(in.size());
  for (size_t i = 0; i < in.size(); ++i) out[i] = static_cast<uint8_t>((in[i] ^ ks[i]) & 1u);
  return out;
}
inline Bits descramble(const Bits& in, uint8_t seed7) { return scramble(in, seed7); }

// Self-synchronising scrambler: out[n] = in[n] ^ out[n-4] ^ out[n-7].
// The descrambler uses the RECEIVED bits as its history: dec[n] = rx[n] ^ rx[n-4] ^ rx[n-7].
// `history7` bit (k-1) = the bit k steps before the first one (default 0).
inline Bits ss_scramble(const Bits& in, uint8_t history7 = 0) {
  unsigned s = history7 & 0x7Fu;
  Bits out;
  out.reserve(in.size());
  for (uint8_t b : in) {
    const unsigned o = (b ^ (s >> 3) ^ (s >> 6)) & 1u;
    s = ((s << 1) | o) & 0x7Fu;
    out.push_back(static_cast<uint8_t>(o));
  }
  return out;
}
inline Bits ss_descramble(const Bits& rx, uint8_t history7 = 0) {
  unsigned s = history7 & 0x7Fu;
  Bits out;
  out.reserve(rx.size());
  for (uint8_t b : rx) {
    const unsigned d = (b ^ (s >> 3) ^ (s >> 6)) & 1u;
    s = ((s << 1) | (b & 1u)) & 0x7Fu;  // history = what was RECEIVED
    out.push_back(static_cast<uint8_t>(d));
  }
  return out;
}

// ---------------------------------------------------------------------------------------------
// The DP cousin (LeetCode 931 "Minimum Falling Path Sum" -- VERIFY number): cheapest path from the
// top row to the bottom row, moving down one row and at most one column left/right.
// ---------------------------------------------------------------------------------------------
inline int min_falling_path_sum(const std::vector<std::vector<int>>& m) {
  if (m.empty() || m[0].empty()) return 0;
  const size_t n = m.size();
  const size_t w = m[0].size();
  std::vector<int> dp = m[0];
  for (size_t r = 1; r < n; ++r) {
    std::vector<int> next(w);
    for (size_t c = 0; c < w; ++c) {
      int best = dp[c];
      if (c > 0) best = std::min(best, dp[c - 1]);
      if (c + 1 < w) best = std::min(best, dp[c + 1]);
      next[c] = m[r][c] + best;
    }
    dp = next;
  }
  return *std::min_element(dp.begin(), dp.end());
}

}  // namespace cv
