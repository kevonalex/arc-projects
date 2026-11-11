// Pack 06 tests.  The same file is compiled against include/viterbi.h (stubs) and
// solutions/viterbi.h (reference).  All data are synthetic. Expected values were derived by hand in
// worksheet.md and cross-checked with an independent script.
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "viterbi.h"

using namespace cv;

namespace {
Bits from_string(const std::string& s) {
  Bits b;
  for (char c : s)
    if (c == '0' || c == '1') b.push_back(static_cast<uint8_t>(c - '0'));
  return b;
}
std::string to_string(const Bits& b) {
  std::string s;
  for (uint8_t x : b) s.push_back(x ? '1' : '0');
  return s;
}
// 12 synthetic data bits used for the burst experiments (28 coded bits = 4 rows x 7 cols).
const char* kData12 = "101100111010";
}  // namespace

// ---------------------------------------------------------------- encoder (W 3.2)
TEST(ConvEncode, Worked1011) {
  EXPECT_EQ(to_string(conv_encode(from_string("1011"))), "111000010111");  // W 4 worked example
}
TEST(ConvEncode, WithoutTail) {
  // 1 0 1 1 -> pairs 11 10 00 01  (no tail)
  EXPECT_EQ(to_string(conv_encode(from_string("1011"), false)), "11100001");
}
TEST(ConvEncode, ImpulseResponseIsTheGenerators) {
  // A single 1 followed by zeros reads out the generators: 11, 10, 11  (111 and 101 column by column)
  EXPECT_EQ(to_string(conv_encode(from_string("1"))), "111011");
}
TEST(ConvEncode, AllZerosGivesAllZeros) {
  EXPECT_EQ(to_string(conv_encode(from_string("00000"))), "00000000000000");
}
TEST(ConvEncode, LengthIsTwiceInputPlusTail) {
  EXPECT_EQ(conv_encode(from_string("1"), false).size(), 2u);
  EXPECT_EQ(conv_encode(from_string("101100111010")).size(), 28u);
}
TEST(ConvEncode, IsLinear) {
  const Bits a = from_string("1011");
  const Bits b = from_string("0110");
  Bits x(4);
  for (size_t i = 0; i < 4; ++i) x[i] = a[i] ^ b[i];
  const Bits ea = conv_encode(a), eb = conv_encode(b), ex = conv_encode(x);
  ASSERT_EQ(ea.size(), ex.size());
  for (size_t i = 0; i < ex.size(); ++i) EXPECT_EQ(ex[i], ea[i] ^ eb[i]);
}

// ---------------------------------------------------------------- Viterbi (W 3.4)
TEST(Viterbi, CleanRoundTrip) {
  const Bits data = from_string("1011");
  const ViterbiResult r = viterbi_decode(conv_encode(data));
  EXPECT_EQ(to_string(r.bits), "1011");
  EXPECT_EQ(r.metric, 0u);
}
TEST(Viterbi, WorkedExampleOneError) {
  // 11 10 [10] 01 01 11 : the third pair should have been 00 (one bit flipped)
  const ViterbiResult r = viterbi_decode(from_string("111010010111"));
  EXPECT_EQ(to_string(r.bits), "1011");
  EXPECT_EQ(r.metric, 1u);
}
TEST(Viterbi, EverySingleErrorIsCorrected) {
  const Bits data = from_string("1011");
  const Bits coded = conv_encode(data);
  for (size_t i = 0; i < coded.size(); ++i) {
    Bits rx = coded;
    rx[i] ^= 1u;
    const ViterbiResult r = viterbi_decode(rx);
    EXPECT_EQ(to_string(r.bits), "1011") << "error at " << i;
    EXPECT_EQ(r.metric, 1u) << "error at " << i;
  }
}
TEST(Viterbi, EveryPairOfErrorsIsCorrected) {
  // free distance of (7,5) is 5, so any 2 errors are always corrected.
  const Bits data = from_string("1011");
  const Bits coded = conv_encode(data);
  for (size_t i = 0; i < coded.size(); ++i)
    for (size_t j = i + 1; j < coded.size(); ++j) {
      Bits rx = coded;
      rx[i] ^= 1u;
      rx[j] ^= 1u;
      const ViterbiResult r = viterbi_decode(rx);
      EXPECT_EQ(to_string(r.bits), "1011") << "errors at " << i << "," << j;
      EXPECT_EQ(r.metric, 2u) << "errors at " << i << "," << j;
    }
}
TEST(Viterbi, LongerMessageRoundTrip) {
  const Bits data = from_string(kData12);
  const ViterbiResult r = viterbi_decode(conv_encode(data));
  EXPECT_EQ(to_string(r.bits), kData12);
  EXPECT_EQ(r.metric, 0u);
}
TEST(Viterbi, MinimumDistanceBetweenCodewordsIsFive) {
  // free distance: the weight of the encoding of a lone 1 is 5 (11 10 11).
  const Bits c = conv_encode(from_string("1"));
  unsigned w = 0;
  for (uint8_t b : c) w += b;
  EXPECT_EQ(w, 5u);
}
TEST(Viterbi, BadLengthsGiveEmpty) {
  EXPECT_TRUE(viterbi_decode(from_string("111")).bits.empty());
  EXPECT_TRUE(viterbi_decode(from_string("11")).bits.empty());
  EXPECT_TRUE(viterbi_decode(Bits{}).bits.empty());
}
TEST(Viterbi, OnlyTailMeansNoData) {
  const ViterbiResult r = viterbi_decode(conv_encode(Bits{}));  // 00 00
  EXPECT_TRUE(r.bits.empty());
  EXPECT_EQ(r.metric, 0u);
}

// ---------------------------------------------------------------- interleaver (W 3.6)
TEST(Interleave, KnownLayout) {
  // rows=2, cols=3: write 1 2 3 / 4 5 6 row-wise, read column-wise -> 1 4 2 5 3 6
  const Bits in = {1, 2, 3, 4, 5, 6};
  const Bits out = interleave(in, 2, 3);
  const Bits expect = {1, 4, 2, 5, 3, 6};
  EXPECT_EQ(out, expect);
  EXPECT_EQ(deinterleave(out, 2, 3), in);
}
TEST(Interleave, RoundTripAndBadSize) {
  const Bits c = conv_encode(from_string(kData12));
  EXPECT_EQ(deinterleave(interleave(c, 4, 7), 4, 7), c);
  EXPECT_TRUE(interleave(c, 3, 7).empty());
  EXPECT_TRUE(deinterleave(c, 0, 28).empty());
}
TEST(Interleave, BurstBecomesScatteredOffsets) {
  // Burst of 4 at interleaved positions 8..11 (one column of the 4x7 grid) lands at coded
  // positions 2, 9, 16, 23 - seven apart.
  Bits mark(28, 0);
  for (size_t i = 8; i < 12; ++i) mark[i] = 1;
  const Bits d = deinterleave(mark, 4, 7);
  std::vector<size_t> hit;
  for (size_t i = 0; i < d.size(); ++i)
    if (d[i]) hit.push_back(i);
  const std::vector<size_t> expect = {2, 9, 16, 23};
  EXPECT_EQ(hit, expect);
}
TEST(Interleave, PlainBurstDefeatsViterbi) {
  // Without interleaving, a burst of 4 consecutive coded-bit errors always beats the code for this
  // message (tested at every start offset): the decoder returns the WRONG data.
  const Bits data = from_string(kData12);
  const Bits coded = conv_encode(data);
  for (size_t off = 0; off + 4 <= coded.size(); ++off) {
    Bits rx = coded;
    for (size_t k = 0; k < 4; ++k) rx[off + k] ^= 1u;
    EXPECT_NE(viterbi_decode(rx).bits, data) << "burst at " << off;
  }
}
TEST(Interleave, InterleavedBurstIsCorrected) {
  const Bits data = from_string(kData12);
  const Bits tx = interleave(conv_encode(data), 4, 7);
  for (size_t off = 0; off + 4 <= tx.size(); ++off) {
    Bits rx = tx;
    for (size_t k = 0; k < 4; ++k) rx[off + k] ^= 1u;  // the same 4-bit burst on the channel
    const ViterbiResult r = viterbi_decode(deinterleave(rx, 4, 7));
    EXPECT_EQ(r.bits, data) << "burst at " << off;
    EXPECT_EQ(r.metric, 4u) << "burst at " << off;  // four scattered single errors
  }
}

// ---------------------------------------------------------------- LFSR (W 3.7)
TEST(Lfsr, KnownStartOfSequenceForAllOnesSeed) {
  // x^7+x^4+1 with seed 1111111: output starts 0000 1110 1111 0010 ...  (first byte 0x0E)
  const Bits ks = lfsr_keystream(16, 0x7F);
  EXPECT_EQ(to_string(ks), "0000111011110010");
}
TEST(Lfsr, SeedOneStart) {
  EXPECT_EQ(to_string(lfsr_keystream(24, 0x01)), "000100110001011101011011");
}
TEST(Lfsr, RecurrenceHolds) {
  const Bits ks = lfsr_keystream(200, 0x5A);
  for (size_t n = 7; n < ks.size(); ++n) EXPECT_EQ(ks[n], ks[n - 4] ^ ks[n - 7]) << n;
}
TEST(Lfsr, MaximalLengthPeriod127) {
  const Bits ks = lfsr_keystream(254, 0x7F);
  unsigned ones = 0;
  for (size_t i = 0; i < 127; ++i) ones += ks[i];
  EXPECT_EQ(ones, 64u);  // a maximal sequence has 2^(7-1) ones and 2^(7-1)-1 zeros
  for (size_t i = 0; i < 127; ++i) EXPECT_EQ(ks[i], ks[i + 127]) << i;
  // 127 is prime, so the period is 1 or 127; the sequence is not constant, so it is 127.
  EXPECT_NE(ks[3], ks[4]);  // 0 then 1 at the start of the all-ones-seed sequence
}
TEST(Lfsr, ZeroSeedLocksUp) {
  const Bits ks = lfsr_keystream(50, 0x00);
  EXPECT_TRUE(std::all_of(ks.begin(), ks.end(), [](uint8_t b) { return b == 0; }));
}

// ---------------------------------------------------------------- additive scrambler (W 3.8)
TEST(Scrambler, ScrambleThenDescrambleIsIdentity) {
  const Bits data = from_string("1010011100010111010101001111000011");
  const Bits tx = scramble(data, 0x5B);
  EXPECT_NE(tx, data);
  EXPECT_EQ(descramble(tx, 0x5B), data);
}
TEST(Scrambler, WhitensLongRuns) {
  const Bits zeros(127, 0);  // worst case for clock recovery: a constant stream
  const Bits tx = scramble(zeros, 0x7F);
  size_t run = 0, best = 0;
  for (uint8_t b : tx) {
    run = b ? run + 1 : 0;
    best = std::max(best, run);
  }
  EXPECT_LE(best, 7u);  // ones: the longest run of ones in a 7-bit maximal sequence is 7
  size_t zrun = 0, zbest = 0;
  for (uint8_t b : tx) {
    zrun = b ? 0 : zrun + 1;
    zbest = std::max(zbest, zrun);
  }
  EXPECT_LE(zbest, 6u);
}
TEST(Scrambler, WrongSeedDoesNotDescramble) {
  const Bits data = from_string("11001010111000101010");
  EXPECT_NE(descramble(scramble(data, 0x7F), 0x01), data);
}
TEST(Scrambler, SingleChannelErrorStaysSingle) {
  const Bits data(40, 0);
  Bits tx = scramble(data, 0x7F);
  tx[17] ^= 1u;
  const Bits back = descramble(tx, 0x7F);
  unsigned errors = 0;
  for (size_t i = 0; i < back.size(); ++i) errors += back[i] != data[i];
  EXPECT_EQ(errors, 1u);
}

// ---------------------------------------------------------------- self-synchronising scrambler (W 3.9)
TEST(SelfSync, RoundTripWithSameHistory) {
  const Bits data = from_string("1010011100010111010101001111000011");
  const Bits tx = ss_scramble(data);
  EXPECT_NE(tx, data);
  EXPECT_EQ(ss_descramble(tx), data);
}
TEST(SelfSync, RecurrenceHolds) {
  const Bits data = from_string("1100101011100010101011110000");
  const Bits tx = ss_scramble(data);
  for (size_t n = 7; n < tx.size(); ++n) EXPECT_EQ(tx[n], data[n] ^ tx[n - 4] ^ tx[n - 7]) << n;
}
TEST(SelfSync, SynchronisesAfterSevenBitsWhateverTheStartState) {
  Bits data;
  for (size_t i = 0; i < 64; ++i) data.push_back(static_cast<uint8_t>((i * 7 + (i >> 2)) & 1u));
  const Bits tx = ss_scramble(data, 0x00);
  const Bits back = ss_descramble(tx, 0x5D);  // receiver starts with the WRONG history
  for (size_t i = 7; i < data.size(); ++i) EXPECT_EQ(back[i], data[i]) << i;
}
TEST(SelfSync, OneChannelErrorBecomesThree) {
  const Bits data(64, 0);
  Bits tx = ss_scramble(data, 0x2B);
  tx[20] ^= 1u;
  const Bits back = ss_descramble(tx, 0x2B);
  std::vector<size_t> wrong;
  for (size_t i = 0; i < back.size(); ++i)
    if (back[i] != data[i]) wrong.push_back(i);
  const std::vector<size_t> expect = {20, 24, 27};  // n, n+4, n+7  (the taps of x^7+x^4+1)
  EXPECT_EQ(wrong, expect);
}

// ---------------------------------------------------------------- DP cousin (W 3.5)
TEST(Dp, MinFallingPathSum) {
  EXPECT_EQ(min_falling_path_sum({{2, 1, 3}, {6, 5, 4}, {7, 8, 9}}), 13);
  EXPECT_EQ(min_falling_path_sum({{-19, 57}, {-40, -5}}), -59);
  EXPECT_EQ(min_falling_path_sum({{5}}), 5);
  EXPECT_EQ(min_falling_path_sum({}), 0);
}
