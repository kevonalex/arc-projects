#include <gtest/gtest.h>

#include <algorithm>
#include <bitset>
#include <cstdint>

#include "p25p1/bch.hpp"

using namespace p25p1;

namespace {
int weight(std::uint64_t v) { return static_cast<int>(std::bitset<64>(v).count()); }
}  // namespace

TEST(Gf64Support, GeneratorHasDegree47) {
  const std::uint64_t g = bch63_16_generator();
  EXPECT_TRUE((g >> 47) & 1ULL);
  EXPECT_EQ(g >> 48, 0ULL);
  EXPECT_TRUE(g & 1ULL);  // constant term of a generator is 1
}

TEST(Bch, PolyModToyHamming74) {
  // g = x^3 + x + 1 = 0b1011.  x^6 + x^5 + x^3 = 0b1101000  ->  remainder x^0 = 1
  EXPECT_EQ(poly_mod(0b1101000, 0b1011, 3), 0b001u);
  // multiples of g leave remainder 0
  EXPECT_EQ(poly_mod(0b1011000, 0b1011, 3), 0u);
  // x^4 mod g = x^2 + x
  EXPECT_EQ(poly_mod(0b10000, 0b1011, 3), 0b110u);
}

TEST(Bch, EncodeIsSystematicAndDivisible) {
  for (std::uint32_t d : {0u, 1u, 0x2935u, 0xFFFFu, 0xA5A5u}) {
    const std::uint64_t cw = bch63_16_encode(static_cast<std::uint16_t>(d));
    EXPECT_EQ(cw >> 47, d) << "data must sit in the top 16 of 63 bits";
    EXPECT_EQ(cw >> 63, 0ULL);
    EXPECT_EQ(poly_mod(cw, bch63_16_generator(), 47), 0u) << "valid codeword => syndrome 0";
  }
}

TEST(Bch, EncodeIsLinear) {
  const std::uint16_t a = 0x1234, b = 0xCAFE;
  EXPECT_EQ(bch63_16_encode(a) ^ bch63_16_encode(b), bch63_16_encode(a ^ b));
  EXPECT_EQ(bch63_16_encode(0), 0ULL);
}

TEST(Bch, MinimumDistanceIsAtLeast23) {
  // BCH bound: roots alpha^1..alpha^22 => d >= 23. Prove it by exhausting all 65535 non-zero words.
  int dmin = 64;
  for (std::uint32_t d = 1; d < 65536; ++d) {
    dmin = std::min(dmin, weight(bch63_16_encode(static_cast<std::uint16_t>(d))));
  }
  EXPECT_GE(dmin, 23);
}

TEST(Bch, DecodeCorrectsUpToElevenErrors) {
  const std::uint16_t data = 0x2935;
  const std::uint64_t cw = bch63_16_encode(data);
  // flip k bits spread across the word
  for (int k = 0; k <= 11; ++k) {
    std::uint64_t rx = cw;
    for (int i = 0; i < k; ++i) rx ^= 1ULL << ((i * 5 + 3) % 63);
    const BchResult r = bch63_16_decode(rx);
    EXPECT_TRUE(r.ok) << "k=" << k;
    EXPECT_EQ(r.data, data) << "k=" << k;
    EXPECT_EQ(r.errors, k) << "k=" << k;
  }
}

TEST(Bch, DecodeReportsFailureWhenTooNoisy) {
  const std::uint64_t cw = bch63_16_encode(0x2935);
  std::uint64_t rx = cw;
  for (int i = 0; i < 30; ++i) rx ^= 1ULL << ((i * 2) % 63);  // 30 distinct flips
  const BchResult r = bch63_16_decode(rx);
  // With 30 errors the received word is far from EVERY codeword: either ok=false or a wrong decode.
  EXPECT_TRUE(!r.ok || r.data != 0x2935);
}

TEST(Bch, NidEncode64HasEvenWeightAndPayload) {
  const std::uint64_t nid = nid_encode64(0x293, Duid::Ldu1);
  EXPECT_EQ(weight(nid) % 2, 0) << "even parity over all 64 bits";
  EXPECT_EQ(nid >> 1, bch63_16_encode(0x2935));
  // different DUID => different word, far apart (d >= 23)
  const std::uint64_t other = nid_encode64(0x293, Duid::Ldu2);
  EXPECT_GE(weight(nid ^ other), 23);
}
