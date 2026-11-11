// 01-bits  --  stubs for the bit-manipulation pattern pack.
//
// Practise: copy solutions/bits_solutions.cpp to attempts/, blank the bodies
// (return {} / 0), then build with  -DIMPL_DIR=attempts  and make the tests pass.
//
// Rule of thumb for this whole pack: do bit tricks on UNSIGNED types.
// Signed left-shift into the sign bit and signed overflow are undefined
// behaviour in C++ (UBSan will tell you), unsigned wrap-around is defined.
#pragma once

#include <cstdint>
#include <vector>

namespace lc {

// 191  Number of 1 Bits.        Context: popcount of a 32-bit MAC/PDCP header word.
int hamming_weight(std::uint32_t n);

// 190  Reverse Bits.            Context: MSB-first <-> LSB-first bit order (e.g. CRC / air-interface byte order).
std::uint32_t reverse_bits(std::uint32_t n);

// 338  Counting Bits.           Context: popcount lookup table for 0..n (e.g. building a syndrome-weight table).
std::vector<int> count_bits(int n);

// 136  Single Number.           Context: one corrupted frame id among duplicated/ack'd ids.
int single_number(const std::vector<int>& nums);

// 137  Single Number II.        Context: every sequence id is received 3x (triple redundancy) except one.
int single_number_ii(const std::vector<int>& nums);

// 268  Missing Number.          Context: which HARQ process id in [0..n] never reported in?
int missing_number(const std::vector<int>& nums);

// 461  Hamming Distance.        Context: bit errors between a sent and a received symbol.
int hamming_distance(int x, int y);

// 477  Total Hamming Distance.  Context: total pairwise distance of a code book (minimum-distance analysis).
int total_hamming_distance(const std::vector<int>& nums);

// 371  Sum of Two Integers.     Context: carry-less (XOR) vs carry (AND) -- the half adder hiding inside a CRC/FPGA adder.
int get_sum(int a, int b);

// 201  Bitwise AND of Numbers Range.  Context: longest common prefix of two addresses = subnet mask / CIDR block.
int range_bitwise_and(int left, int right);

// 89   Gray Code.               Context: constellation labelling -- neighbouring symbols differ in 1 bit.
std::vector<int> gray_code(int n);

// 393  UTF-8 Validation.        Context: validating a multi-byte length-prefixed field (variable-length header).
bool valid_utf8(const std::vector<int>& data);

}  // namespace lc
