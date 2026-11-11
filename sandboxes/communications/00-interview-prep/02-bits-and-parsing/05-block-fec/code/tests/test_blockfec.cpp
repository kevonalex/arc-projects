// Tests for Pack 05. Same file is compiled against the stubs and the solutions.
// Bit convention: codeword position k (1-based) is bit (k-1) of the integer.
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "blockfec.h"

using namespace fec;

// ---------------------------------------------------------------- parity / distance (W 3.2, 3.3)
TEST(Parity, Examples) {
  EXPECT_EQ(parity32(0u), 0u);
  EXPECT_EQ(parity32(1u), 1u);
  EXPECT_EQ(parity32(3u), 0u);
  EXPECT_EQ(parity32(0x69u), 0u);  // 0110 1001 has four 1s
  EXPECT_EQ(parity32(0x68u), 1u);  // three 1s
  EXPECT_EQ(parity32(0xFFFFFFFFu), 0u);
  EXPECT_EQ(parity32(0x7FFFFFFFu), 1u);
  EXPECT_EQ(parity32(0x80000000u), 1u);
}
TEST(Parity, MatchesBitCounting) {
  for (uint32_t x = 0; x < 4096; ++x) {
    unsigned n = 0;
    for (unsigned i = 0; i < 32; ++i) n += (x >> i) & 1u;
    EXPECT_EQ(parity32(x), n & 1u);
  }
}
TEST(Parity, SingleBitFlipAlwaysChangesParityTwoFlipsNever) {
  const uint32_t w = 0xA5C3u;
  for (unsigned i = 0; i < 16; ++i) {
    EXPECT_NE(parity32(w ^ (1u << i)), parity32(w));
    for (unsigned j = i + 1; j < 16; ++j) EXPECT_EQ(parity32(w ^ (1u << i) ^ (1u << j)), parity32(w));
  }
}
TEST(Distance, HammingDistance) {
  EXPECT_EQ(hamming_distance(0u, 0u), 0u);
  EXPECT_EQ(hamming_distance(0b000u, 0b111u), 3u);
  EXPECT_EQ(hamming_distance(0b1011u, 0b0010u), 2u);
  EXPECT_EQ(hamming_distance(0xFFFFFFFFu, 0u), 32u);
}
TEST(Distance, MinDistanceSmallCodes) {
  EXPECT_EQ(min_distance({0b000u, 0b111u}), 3u);  // repetition code
  EXPECT_EQ(min_distance({0b00000u, 0b01011u, 0b10101u, 0b11110u}), 3u);
  EXPECT_EQ(min_distance({0b00u, 0b01u, 0b10u, 0b11u}), 1u);  // no redundancy at all
  EXPECT_EQ(min_distance({0u}), 0u);
}

// ---------------------------------------------------------------- Hamming (7,4) (W 3.4)
static const uint8_t kH74[16] = {0x00, 0x07, 0x19, 0x1E, 0x2A, 0x2D, 0x33, 0x34,
                                 0x4B, 0x4C, 0x52, 0x55, 0x61, 0x66, 0x78, 0x7F};

TEST(Hamming74, EncodeTable) {
  for (unsigned n = 0; n < 16; ++n) EXPECT_EQ(hamming74_encode(static_cast<uint8_t>(n)), kH74[n]) << "nibble " << n;
}
TEST(Hamming74, WorkedExample1011Is0x55) { EXPECT_EQ(hamming74_encode(0xB), 0x55); }
TEST(Hamming74, MinimumDistanceIs3) {
  std::vector<uint32_t> cws;
  for (unsigned n = 0; n < 16; ++n) cws.push_back(hamming74_encode(static_cast<uint8_t>(n)));
  EXPECT_EQ(min_distance(cws), 3u);
}
TEST(Hamming74, IsPerfect_EveryWordIsWithinOneOfExactlyOneCodeword) {
  std::vector<uint8_t> cws;
  for (unsigned n = 0; n < 16; ++n) cws.push_back(hamming74_encode(static_cast<uint8_t>(n)));
  for (uint32_t w = 0; w < 128; ++w) {
    int hits = 0;
    for (uint8_t c : cws) hits += (hamming_distance(w, c) <= 1) ? 1 : 0;
    EXPECT_EQ(hits, 1) << "word " << w;
  }
}
TEST(Hamming74, CleanDecode) {
  for (unsigned n = 0; n < 16; ++n) {
    const Hamming74Result r = hamming74_decode(hamming74_encode(static_cast<uint8_t>(n)));
    EXPECT_EQ(r.data, n);
    EXPECT_EQ(r.syndrome, 0);
    EXPECT_FALSE(r.corrected);
  }
}
TEST(Hamming74, SyndromeIsThePositionOfTheError) {
  for (unsigned n = 0; n < 16; ++n) {
    const uint8_t cw = hamming74_encode(static_cast<uint8_t>(n));
    for (unsigned pos = 1; pos <= 7; ++pos) {
      const uint8_t rx = static_cast<uint8_t>(cw ^ (1u << (pos - 1)));
      const Hamming74Result r = hamming74_decode(rx);
      EXPECT_EQ(r.syndrome, pos) << "nibble " << n << " pos " << pos;
      EXPECT_TRUE(r.corrected);
      EXPECT_EQ(r.data, n);
    }
  }
}
TEST(Hamming74, WorkedDecodeExample0x24) {
  const Hamming74Result r = hamming74_decode(0x24);  // 0x34 with position 5 flipped
  EXPECT_EQ(r.syndrome, 5);
  EXPECT_TRUE(r.corrected);
  EXPECT_EQ(r.data, 7);
}
TEST(Hamming74, TwoErrorsAreMiscorrectedNotDetected) {
  const uint8_t rx = static_cast<uint8_t>(hamming74_encode(0xB) ^ 0x03);  // flip positions 1 and 2
  const Hamming74Result r = hamming74_decode(rx);
  EXPECT_EQ(r.syndrome, 3);  // 1 xor 2: points at position 3, which is fine
  EXPECT_TRUE(r.corrected);  // the decoder is confident...
  EXPECT_NE(r.data, 0xB);    // ...and wrong
  EXPECT_EQ(r.data, 0xA);
}

// ---------------------------------------------------------------- extended Hamming (8,4) (W 3.5)
TEST(Hamming84, EncodeKnown) {
  EXPECT_EQ(hamming84_encode(0xB), 0x55);  // 0x55 already has even weight
  EXPECT_EQ(hamming84_encode(0x1), 0x87);  // 0x07 has odd weight -> parity bit set
  EXPECT_EQ(hamming84_encode(0x0), 0x00);
}
TEST(Hamming84, MinimumDistanceIs4) {
  std::vector<uint32_t> cws;
  for (unsigned n = 0; n < 16; ++n) cws.push_back(hamming84_encode(static_cast<uint8_t>(n)));
  EXPECT_EQ(min_distance(cws), 4u);
}
TEST(Hamming84, EverySingleErrorCorrected) {
  for (unsigned n = 0; n < 16; ++n) {
    const uint8_t cw = hamming84_encode(static_cast<uint8_t>(n));
    EXPECT_EQ(hamming84_decode(cw).status, Status::Ok);
    for (unsigned bit = 0; bit < 8; ++bit) {
      const Hamming84Result r = hamming84_decode(static_cast<uint8_t>(cw ^ (1u << bit)));
      EXPECT_EQ(r.status, Status::Corrected) << "n=" << n << " bit=" << bit;
      EXPECT_EQ(r.data, n);
    }
  }
}
TEST(Hamming84, EveryDoubleErrorDetected) {
  for (unsigned n = 0; n < 16; ++n) {
    const uint8_t cw = hamming84_encode(static_cast<uint8_t>(n));
    for (unsigned i = 0; i < 8; ++i)
      for (unsigned j = i + 1; j < 8; ++j) {
        const Hamming84Result r = hamming84_decode(static_cast<uint8_t>(cw ^ (1u << i) ^ (1u << j)));
        EXPECT_EQ(r.status, Status::Uncorrectable) << "n=" << n << " bits " << i << "," << j;
      }
  }
}
TEST(Hamming84, WorkedCases) {
  EXPECT_EQ(hamming84_decode(0x55).status, Status::Ok);
  EXPECT_EQ(hamming84_decode(0xD5).status, Status::Corrected);        // only the overall parity bit flipped
  EXPECT_EQ(hamming84_decode(0xD5).data, 0xB);
  EXPECT_EQ(hamming84_decode(0x45).status, Status::Corrected);        // position 5 flipped
  EXPECT_EQ(hamming84_decode(0x45).data, 0xB);
  EXPECT_EQ(hamming84_decode(0x56).status, Status::Uncorrectable);    // positions 1,2 flipped
}

// ---------------------------------------------------------------- Golay (23,12,7) (W 3.6)
TEST(Golay, EncodeKnownValues) {
  EXPECT_EQ(golay23_encode(0), 0u);
  EXPECT_EQ(golay23_encode(1), 0xC75u);       // data x^0 -> x^11 mod g = g - x^11 = 0x475; codeword = g itself
  EXPECT_EQ(golay23_encode(0xABC), 0x55E11Eu);
}
TEST(Golay, IsSystematic) {
  for (uint32_t d = 0; d < 4096; d += 13) EXPECT_EQ(golay23_encode(static_cast<uint16_t>(d)) >> 11, d);
}
TEST(Golay, MinimumWeightIs7AndWeightDistribution) {
  unsigned min_w = 99;
  unsigned count[24] = {0};
  for (uint32_t d = 1; d < 4096; ++d) {
    const unsigned w = hamming_distance(golay23_encode(static_cast<uint16_t>(d)), 0u);
    if (w < min_w) min_w = w;
    ++count[w];
  }
  EXPECT_EQ(min_w, 7u);
  EXPECT_EQ(count[7], 253u);
  EXPECT_EQ(count[8], 506u);
  EXPECT_EQ(count[11], 1288u);
  EXPECT_EQ(count[12], 1288u);
  EXPECT_EQ(count[15], 506u);
  EXPECT_EQ(count[16], 253u);
  EXPECT_EQ(count[23], 1u);
}
TEST(Golay, LinearClosure) {  // codeword(a) xor codeword(b) == codeword(a xor b)
  for (uint32_t a = 0; a < 4096; a += 97)
    for (uint32_t b = 0; b < 4096; b += 89)
      EXPECT_EQ(golay23_encode(static_cast<uint16_t>(a)) ^ golay23_encode(static_cast<uint16_t>(b)),
                golay23_encode(static_cast<uint16_t>(a ^ b)));
}
TEST(Golay, CleanDecode) {
  for (uint32_t d = 0; d < 4096; d += 31) {
    const GolayResult r = golay23_decode(golay23_encode(static_cast<uint16_t>(d)));
    EXPECT_EQ(r.data, d);
    EXPECT_EQ(r.errors, 0);
  }
}
TEST(Golay, CorrectsEveryPatternOfUpTo3Errors) {
  const uint16_t data = 0x5A3;
  const uint32_t cw = golay23_encode(data);
  for (unsigned a = 0; a < 23; ++a) {
    GolayResult r = golay23_decode(cw ^ (1u << a));
    EXPECT_EQ(r.data, data);
    EXPECT_EQ(r.errors, 1);
    for (unsigned b = a + 1; b < 23; ++b) {
      r = golay23_decode(cw ^ (1u << a) ^ (1u << b));
      EXPECT_EQ(r.data, data);
      EXPECT_EQ(r.errors, 2);
      for (unsigned c = b + 1; c < 23; ++c) {
        r = golay23_decode(cw ^ (1u << a) ^ (1u << b) ^ (1u << c));
        ASSERT_EQ(r.data, data) << a << "," << b << "," << c;
        EXPECT_EQ(r.errors, 3);
      }
    }
  }
}
TEST(Golay, PerfectCodeFourErrorsAreAlwaysMiscorrected) {
  // Spheres of radius 3 around the 4096 codewords tile all 2^23 words (1+23+253+1771 = 2048 = 2^11),
  // so a 4-error word is ALWAYS decoded confidently to a different codeword.
  const uint16_t data = 0x123;
  const uint32_t cw = golay23_encode(data);
  unsigned tested = 0;
  for (unsigned a = 0; a < 23 && tested < 150; ++a)
    for (unsigned b = a + 1; b < 23 && tested < 150; ++b)
      for (unsigned c = b + 1; c < 23 && tested < 150; ++c)
        for (unsigned d = c + 1; d < 23 && tested < 150; ++d, ++tested) {
          const GolayResult r = golay23_decode(cw ^ (1u << a) ^ (1u << b) ^ (1u << c) ^ (1u << d));
          EXPECT_NE(r.data, data);
          EXPECT_GE(r.errors, 1);
          EXPECT_LE(r.errors, 3);
        }
}
TEST(Golay, AnyReceivedWordIsWithin3OfSomeCodeword) {
  uint32_t s = 17;
  for (int i = 0; i < 300; ++i) {
    s = s * 1664525u + 1013904223u;
    const uint32_t rx = (s >> 5) & 0x7FFFFFu;
    const GolayResult r = golay23_decode(rx);
    ASSERT_GE(r.errors, 0);
    ASSERT_LE(r.errors, 3);
    EXPECT_EQ(hamming_distance(golay23_encode(r.data), rx), static_cast<unsigned>(r.errors));
  }
}

// ---------------------------------------------------------------- toy Reed-Solomon (W 3.8)
TEST(Rs5, EncodeWorkedExample) {
  const auto c = rs5_encode(3, 1);  // p(x) = 3 + x
  EXPECT_EQ(c[0], 3);
  EXPECT_EQ(c[1], 4);
  EXPECT_EQ(c[2], 0);  // 5 mod 5
  EXPECT_EQ(c[3], 1);  // 6 mod 5
}
TEST(Rs5, RecoverWorkedExample) {
  uint8_t a0 = 9, a1 = 9;
  ASSERT_TRUE(rs5_recover(1, 4, 3, 1, a0, a1));
  EXPECT_EQ(a0, 3);
  EXPECT_EQ(a1, 1);
}
TEST(Rs5, AnyTwoOfFourSymbolsRecoverAnyMessage) {
  for (unsigned m0 = 0; m0 < 5; ++m0)
    for (unsigned m1 = 0; m1 < 5; ++m1) {
      const auto c = rs5_encode(static_cast<uint8_t>(m0), static_cast<uint8_t>(m1));
      for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
          uint8_t a0 = 9, a1 = 9;
          ASSERT_TRUE(rs5_recover(i, c[i], j, c[j], a0, a1));
          EXPECT_EQ(a0, m0);
          EXPECT_EQ(a1, m1);
        }
    }
}
TEST(Rs5, MinimumDistanceIsNMinusKPlus1) {
  // Two different messages share at most one codeword symbol (two points define the line).
  for (unsigned m0 = 0; m0 < 5; ++m0)
    for (unsigned m1 = 0; m1 < 5; ++m1)
      for (unsigned n0 = 0; n0 < 5; ++n0)
        for (unsigned n1 = 0; n1 < 5; ++n1) {
          if (m0 == n0 && m1 == n1) continue;
          const auto a = rs5_encode(static_cast<uint8_t>(m0), static_cast<uint8_t>(m1));
          const auto b = rs5_encode(static_cast<uint8_t>(n0), static_cast<uint8_t>(n1));
          unsigned diff = 0;
          for (int i = 0; i < 4; ++i) diff += (a[i] != b[i]) ? 1 : 0;
          EXPECT_GE(diff, 3u);  // d = n - k + 1 = 3
        }
}
TEST(Rs5, RejectsBadArguments) {
  uint8_t a0 = 0, a1 = 0;
  EXPECT_FALSE(rs5_recover(1, 2, 1, 3, a0, a1));  // same x
  EXPECT_FALSE(rs5_recover(0, 2, 4, 3, a0, a1));  // x out of range
}
