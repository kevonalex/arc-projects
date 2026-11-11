// viterbi.h  --  STUBS for Pack 06 (student version).  Reference: ../solutions/viterbi.h
// Bits are stored one per element (std::vector<uint8_t> holding 0 or 1) so every step is visible.
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace cv {

using Bits = std::vector<uint8_t>;

// ---- W 3.2 / 3.3: rate-1/2, K=3 convolutional code, generators (7,5) octal = (111, 101) ----
// State = the two previous input bits. For input u and previous inputs u1 (most recent), u2:
//   out0 = u ^ u1 ^ u2      (generator 111 = 7)       out1 = u ^ u2      (generator 101 = 5)
// Output order per input bit: out0 then out1.
// terminate = true appends two 0 input bits ("tail") so the encoder ends back in state 0:
//   conv_encode({1,0,1,1}) == 1,1, 1,0, 0,0, 0,1, 0,1, 1,1   (6 pairs = 12 bits)
inline Bits conv_encode([[maybe_unused]] const Bits& bits, [[maybe_unused]] bool terminate = true) {
  return Bits{};  // TODO W 3.2
}

struct ViterbiResult {
  Bits bits;        // decoded data bits (tail removed)
  unsigned metric;  // total Hamming distance between rx and the winning path's coded bits
};

// ---- W 3.4: hard-decision Viterbi decoder for a TERMINATED stream (ends in state 0) ----
// rx has 2 bits per trellis step. Return empty bits if rx.size() is odd or < 4.
// Branch metric = Hamming distance between the received pair and the branch's expected pair.
// Tie-break rule (so everyone gets the same answer): previous states ascending, u = 0 before
// u = 1, and a new candidate wins only if STRICTLY better. Traceback from state 0, then drop the
// 2 tail bits.
inline ViterbiResult viterbi_decode([[maybe_unused]] const Bits& rx) {
  return ViterbiResult{Bits{}, 999999};  // TODO W 3.4
}

// ---- W 3.6: block interleaver. WRITE row by row into a rows x cols grid, READ column by column.
// Return an empty vector if in.size() != rows * cols (or rows/cols is 0).
inline Bits interleave([[maybe_unused]] const Bits& in, [[maybe_unused]] size_t rows, [[maybe_unused]] size_t cols) {
  return Bits{};  // TODO W 3.6
}
inline Bits deinterleave([[maybe_unused]] const Bits& in, [[maybe_unused]] size_t rows, [[maybe_unused]] size_t cols) {
  return Bits{};  // TODO W 3.6
}

// ---- W 3.7: LFSR x^7 + x^4 + 1, Fibonacci form ----
// State s (7 bits): bit (k-1) holds the bit produced k steps ago (seed7 bit0 = "1 step ago").
// Each step:  fb = ((s >> 6) ^ (s >> 3)) & 1;  s = ((s << 1) | fb) & 0x7F;  output fb.
// Consequence: ks[n] = ks[n-4] ^ ks[n-7].  seed7 = 0 gives all zeros forever (never use it).
inline Bits lfsr_keystream([[maybe_unused]] size_t n, [[maybe_unused]] uint8_t seed7) {
  return Bits{};  // TODO W 3.7
}

// ---- W 3.8: additive (synchronous) scrambler: out = in XOR keystream. Descramble is the SAME op.
inline Bits scramble([[maybe_unused]] const Bits& in, [[maybe_unused]] uint8_t seed7) {
  return Bits{};  // TODO W 3.8
}
inline Bits descramble([[maybe_unused]] const Bits& in, [[maybe_unused]] uint8_t seed7) {
  return Bits{};  // TODO W 3.8
}

// ---- W 3.9: self-synchronising scrambler: out[n] = in[n] ^ out[n-4] ^ out[n-7].
// The DEscrambler uses the RECEIVED bits as its history: dec[n] = rx[n] ^ rx[n-4] ^ rx[n-7].
// history7 bit (k-1) = the bit k steps before the first one (default 0).
inline Bits ss_scramble([[maybe_unused]] const Bits& in, [[maybe_unused]] uint8_t history7 = 0) {
  return Bits{};  // TODO W 3.9
}
inline Bits ss_descramble([[maybe_unused]] const Bits& rx, [[maybe_unused]] uint8_t history7 = 0) {
  return Bits{};  // TODO W 3.9
}

// ---- W 3.5 / Q12: the DP cousin of Viterbi (LeetCode 931 "Minimum Falling Path Sum" -- VERIFY).
// Cheapest top-to-bottom path; each step goes down one row and moves at most one column left/right.
// Empty matrix -> 0.  {{2,1,3},{6,5,4},{7,8,9}} -> 13;  {{-19,57},{-40,-5}} -> -59.
inline int min_falling_path_sum([[maybe_unused]] const std::vector<std::vector<int>>& m) {
  return -1;  // TODO W 3.5
}

}  // namespace cv
