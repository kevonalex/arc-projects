// Tests for Capstone M1. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "framing.hpp"

using namespace gw;

namespace {
std::vector<uint8_t> make_payload(uint32_t seed) {
  std::vector<uint8_t> p(kPayloadBytes);
  uint32_t s = seed * 2654435761u + 12345u;
  for (auto& b : p) {
    s = s * 1664525u + 1013904223u;
    b = static_cast<uint8_t>(s >> 24);
  }
  return p;
}
void flip(Bits& b, size_t i) { b[i] ^= 1u; }
}  // namespace

// ---------------------------------------------------------------- CRC-16 (W 3.1)
TEST(Crc16, StandardCheckValue) {
  const char* s = "123456789";
  EXPECT_EQ(crc16_ccitt(reinterpret_cast<const uint8_t*>(s), 9), 0x29B1);
  EXPECT_EQ(crc16_ccitt(nullptr, 0), 0xFFFF);
  const uint8_t one[] = {0x00};
  EXPECT_EQ(crc16_ccitt(one, 1), 0xE1F0);
}
TEST(Crc16, DetectsEverySingleBitFlipInAFrameSizedMessage) {
  auto p = make_payload(1);
  const uint16_t good = crc16_ccitt(p.data(), p.size());
  for (size_t i = 0; i < p.size() * 8; ++i) {
    p[i / 8] ^= static_cast<uint8_t>(1u << (i % 8));
    EXPECT_NE(crc16_ccitt(p.data(), p.size()), good) << "bit " << i;
    p[i / 8] ^= static_cast<uint8_t>(1u << (i % 8));
  }
}

// ---------------------------------------------------------------- Golay (W 3.2)
TEST(Golay, EncodeVectors) {
  EXPECT_EQ(golay23_encode(0), 0u);
  EXPECT_EQ(golay23_encode(1), 0xC75u);
  EXPECT_EQ(golay23_encode(0xABC), 0x55E11Eu);
}
TEST(Golay, CorrectsUpToThreeErrors) {
  for (uint16_t d : {0x000, 0xABC, 0xFFF, 0x123}) {
    const uint32_t cw = golay23_encode(d);
    EXPECT_EQ(golay23_decode(cw).data, d);
    EXPECT_EQ(golay23_decode(cw).errors, 0);
    for (int i = 0; i < 23; ++i) {
      auto r1 = golay23_decode(cw ^ (1u << i));
      EXPECT_EQ(r1.data, d);
      EXPECT_EQ(r1.errors, 1);
      for (int j = i + 1; j < 23; ++j) {
        auto r2 = golay23_decode(cw ^ (1u << i) ^ (1u << j));
        EXPECT_EQ(r2.data, d);
        EXPECT_EQ(r2.errors, 2);
      }
    }
    auto r3 = golay23_decode(cw ^ (1u << 0) ^ (1u << 9) ^ (1u << 22));
    EXPECT_EQ(r3.data, d);
    EXPECT_EQ(r3.errors, 3);
  }
}
TEST(Golay, FourErrorsMiscorrectSilently) {
  const uint32_t cw = golay23_encode(0xABC);
  auto r = golay23_decode(cw ^ 0x00000F00u);  // four adjacent bit errors
  EXPECT_NE(r.data, 0xABC);  // the decoder confidently returns a DIFFERENT codeword
  EXPECT_LE(r.errors, 3);
}

// ---------------------------------------------------------------- Hamming(7,4) (W 3.3)
TEST(Hamming74, KnownCodewordsAndRoundTrip) {
  EXPECT_EQ(hamming74_encode(0xB), 0x55);
  EXPECT_EQ(hamming74_encode(0x6), 0x33);
  EXPECT_EQ(hamming74_encode(0x0), 0x00);
  for (uint8_t n = 0; n < 16; ++n) {
    auto r = hamming74_decode(hamming74_encode(n));
    EXPECT_EQ(r.nibble, n);
    EXPECT_FALSE(r.corrected);
  }
}
TEST(Hamming74, EverySingleErrorIsCorrected) {
  for (uint8_t n = 0; n < 16; ++n) {
    const uint8_t cw = hamming74_encode(n);
    for (int i = 0; i < 7; ++i) {
      auto r = hamming74_decode(static_cast<uint8_t>(cw ^ (1u << i)));
      EXPECT_EQ(r.nibble, n);
      EXPECT_TRUE(r.corrected);
    }
  }
}
TEST(Hamming74, TwoErrorsMiscorrect) {
  auto r = hamming74_decode(0x56);  // 0x55 with positions 1 and 2 flipped
  EXPECT_EQ(r.nibble, 0xA);         // wrong, and it believes it fixed something
  EXPECT_TRUE(r.corrected);
}

// ---------------------------------------------------------------- header + frame layout (W 3.4 - 3.5)
TEST(Header, WordPackingRoundTrip) {
  Header h;
  h.version = 1;
  h.type = 2;
  h.talkgroup = 0xA5;
  h.seq = 0x9AB;
  EXPECT_EQ(header_to_word24(h), 0x6A59ABu);  // 01 10 10100101 100110101011
  EXPECT_TRUE(word24_to_header(0x6A59AB) == h);
  Header d;
  EXPECT_EQ(header_to_word24(d), 0x400000u);  // version 1, rest zero
}
TEST(Frame, SizeConstants) {
  EXPECT_EQ(kFrameBits, 2338u);
  EXPECT_EQ(kBodyBits, 2268u);
  auto p = make_payload(2);
  Header h;
  auto bits = build_frame_bits(h, p.data());
  ASSERT_EQ(bits.size(), kFrameBits);
  for (uint8_t b : bits) ASSERT_LE(b, 1);
  EXPECT_EQ(get_bits(bits, 0, 24), kSyncWord);
}
TEST(Frame, CleanRoundTrip) {
  auto p = make_payload(3);
  Header h;
  h.type = 0;
  h.talkgroup = 42;
  h.seq = 1234;
  auto r = parse_frame_bits(build_frame_bits(h, p.data()));
  ASSERT_EQ(r.status, Status::Ok);
  EXPECT_TRUE(r.header == h);
  EXPECT_EQ(r.payload, p);
  EXPECT_EQ(r.header_corrections, 0);
  EXPECT_EQ(r.payload_corrections, 0);
}
TEST(Frame, TruncatedAndWrongSync) {
  auto p = make_payload(4);
  Header h;
  auto bits = build_frame_bits(h, p.data());
  ASSERT_EQ(bits.size(), kFrameBits);
  Bits cut(bits.begin(), bits.end() - 1);
  EXPECT_EQ(parse_frame_bits(cut).status, Status::Truncated);
  for (size_t i = 0; i < 6; ++i) flip(bits, i * 3);  // six sync bits wrong: beyond the tolerance of 3
  EXPECT_EQ(parse_frame_bits(bits).status, Status::SyncFail);
}
TEST(Frame, EverySingleBitFlipAnywhereIsRepaired) {
  auto p = make_payload(5);
  Header h;
  h.talkgroup = 7;
  h.seq = 77;
  const Bits good = build_frame_bits(h, p.data());
  ASSERT_EQ(good.size(), kFrameBits);
  for (size_t i = 0; i < kFrameBits; ++i) {
    Bits bad = good;
    flip(bad, i);
    auto r = parse_frame_bits(bad);
    ASSERT_EQ(r.status, Status::Ok) << "flipped bit " << i;
    EXPECT_EQ(r.payload, p);
    EXPECT_TRUE(r.header == h);
    if (i >= kSyncBits) {
      EXPECT_EQ(r.header_corrections + r.payload_corrections, 1) << "bit " << i;
    }
  }
}
TEST(Frame, ThreeErrorsInEachGolayHeaderWordAreRepaired) {
  auto p = make_payload(6);
  Header h;
  h.talkgroup = 200;
  h.seq = 4000;
  Bits bad = build_frame_bits(h, p.data());
  ASSERT_EQ(bad.size(), kFrameBits);
  for (size_t i : {24u, 30u, 45u, 47u, 60u, 69u}) flip(bad, i);  // 3 in word 0 (bits 24..46), 3 in word 1 (47..69)
  auto r = parse_frame_bits(bad);
  ASSERT_EQ(r.status, Status::Ok);
  EXPECT_TRUE(r.header == h);
  EXPECT_EQ(r.header_corrections, 6);
}
TEST(Frame, FourErrorsInAHeaderWordAreCaughtByTheCrcNotByGolay) {
  auto p = make_payload(7);
  Header h;
  h.talkgroup = 9;
  Bits bad = build_frame_bits(h, p.data());
  ASSERT_EQ(bad.size(), kFrameBits);
  for (size_t i : {26u, 27u, 28u, 29u}) flip(bad, i);
  auto r = parse_frame_bits(bad);
  EXPECT_NE(r.status, Status::Ok);  // CrcFail (or BadHeader): never a silent wrong header
  EXPECT_TRUE(r.payload.empty());
}
TEST(Frame, TwoErrorsInOneHammingWordAreCaughtByTheCrc) {
  auto p = make_payload(8);
  Header h;
  Bits bad = build_frame_bits(h, p.data());
  ASSERT_EQ(bad.size(), kFrameBits);
  const size_t first_body_bit = kSyncBits + kHeaderBits;
  flip(bad, first_body_bit + 0);
  flip(bad, first_body_bit + 1);  // same 7-bit codeword: Hamming miscorrects
  auto r = parse_frame_bits(bad);
  EXPECT_EQ(r.status, Status::CrcFail);
}
TEST(Frame, ErrorsInSeparateHammingWordsAreAllRepaired) {
  auto p = make_payload(9);
  Header h;
  Bits bad = build_frame_bits(h, p.data());
  ASSERT_EQ(bad.size(), kFrameBits);
  const size_t first_body_bit = kSyncBits + kHeaderBits;
  for (size_t cw = 0; cw < 100; ++cw) flip(bad, first_body_bit + cw * 7 + (cw % 7));  // one error in each of 100 codewords
  auto r = parse_frame_bits(bad);
  ASSERT_EQ(r.status, Status::Ok);
  EXPECT_EQ(r.payload, p);
  EXPECT_EQ(r.payload_corrections, 100);
}
TEST(Frame, RandomBitErrorsAtOneInAThousand) {
  const int frames = 300;
  int ok = 0, crc_fail = 0, silent_wrong = 0;
  uint64_t s = 0x1234567ull;
  for (int f = 0; f < frames; ++f) {
    auto p = make_payload(100 + static_cast<uint32_t>(f));
    Header h;
    h.seq = static_cast<uint16_t>(f);
    Bits bits = build_frame_bits(h, p.data());
    for (auto& b : bits) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      if (((s >> 33) % 1000u) == 0) b ^= 1u;  // BER 1e-3
    }
    auto r = parse_frame_bits(bits);
    if (r.status == Status::Ok) {
      ++ok;
      if (r.payload != p || !(r.header == h)) ++silent_wrong;
    } else {
      ++crc_fail;
    }
  }
  EXPECT_EQ(silent_wrong, 0);        // the CRC makes undetected corruption vanishingly rare
  EXPECT_GE(ok, frames * 90 / 100);  // ~2.3 raw errors per frame, nearly all repaired
}

// ---------------------------------------------------------------- sync search (W 3.6)
TEST(Sync, FindsExactAndDamagedSync) {
  Bits stream;
  uint64_t s = 99;
  for (int i = 0; i < 37; ++i) {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    stream.push_back(static_cast<uint8_t>((s >> 40) & 1u));
  }
  put_bits(stream, kSyncWord, 24);
  for (int i = 0; i < 50; ++i) {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    stream.push_back(static_cast<uint8_t>((s >> 40) & 1u));
  }
  EXPECT_EQ(find_sync(stream, 0), 37);
  flip(stream, 37 + 2);
  flip(stream, 37 + 11);
  flip(stream, 37 + 20);
  EXPECT_EQ(find_sync(stream, 3), 37);
  EXPECT_EQ(find_sync(stream, 2), -1);  // three damaged bits: a tolerance of 2 refuses
}
TEST(Sync, ShortStreamAndMissingSync) {
  EXPECT_EQ(find_sync(Bits(10, 0), 3), -1);
  EXPECT_EQ(find_sync(Bits(200, 0), 0), -1);
}
TEST(Sync, AutocorrelationSidelobesAreSmall) {
  // aperiodic autocorrelation of the +/-1 sync pattern: peak 24, every sidelobe magnitude <= 3
  int v[24];
  for (int i = 0; i < 24; ++i) v[i] = ((kSyncWord >> (23 - i)) & 1u) ? 1 : -1;
  int worst = 0;
  for (int sft = 1; sft < 24; ++sft) {
    int acc = 0;
    for (int i = 0; i + sft < 24; ++i) acc += v[i] * v[i + sft];
    if (acc < 0) acc = -acc;
    if (acc > worst) worst = acc;
  }
  EXPECT_LE(worst, 3);
}
