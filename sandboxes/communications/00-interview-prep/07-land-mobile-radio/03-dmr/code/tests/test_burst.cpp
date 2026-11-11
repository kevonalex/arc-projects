#include <gtest/gtest.h>

#include "dmr/burst.hpp"

using namespace dmr;

namespace {
Bits264 patterned_burst() {
  Bits264 b{};
  for (int i = 0; i < kBurstBits; ++i) b[static_cast<std::size_t>(i)] = ((i * i + 3 * i) % 5 < 2) ? 1 : 0;
  return b;
}
}  // namespace

TEST(BurstTiming, SlotAndBurstSizes) {
  EXPECT_EQ(kHalfBits + kMidBits + kHalfBits, kBurstBits);
  EXPECT_EQ(slot_bits(), 288);   // 9600 bit/s * 30 ms
  EXPECT_EQ(gap_bits(), 24);     // 288 - 264
  EXPECT_DOUBLE_EQ(burst_ms(), 27.5);
}

TEST(Burst, SplitAndJoinRoundTrip) {
  const Bits264 b = patterned_burst();
  const BurstParts p = split_burst(b);
  for (int i = 0; i < 108; ++i) EXPECT_EQ(p.first[static_cast<std::size_t>(i)], b[static_cast<std::size_t>(i)]);
  for (int i = 0; i < 48; ++i) EXPECT_EQ(p.mid[static_cast<std::size_t>(i)], b[static_cast<std::size_t>(108 + i)]);
  for (int i = 0; i < 108; ++i) EXPECT_EQ(p.second[static_cast<std::size_t>(i)], b[static_cast<std::size_t>(156 + i)]);
  EXPECT_EQ(join_burst(p), b);
}

TEST(Burst, VoiceFramesStraddleTheMiddle) {
  const Bits264 b = patterned_burst();
  const BurstParts p = split_burst(b);
  const auto f = extract_voice_frames(p);
  for (int k = 0; k < 72; ++k) {
    EXPECT_EQ(f[0][static_cast<std::size_t>(k)], b[static_cast<std::size_t>(k)]) << "frame0 bit " << k;
    EXPECT_EQ(f[2][static_cast<std::size_t>(k)], b[static_cast<std::size_t>(192 + k)]) << "frame2 bit " << k;
  }
  for (int k = 0; k < 36; ++k) {
    EXPECT_EQ(f[1][static_cast<std::size_t>(k)], b[static_cast<std::size_t>(72 + k)]) << "frame1 bit " << k;
    EXPECT_EQ(f[1][static_cast<std::size_t>(36 + k)], b[static_cast<std::size_t>(156 + k)]) << "frame1 bit " << 36 + k;
  }
}

TEST(Burst, PackBits48IsMsbFirst) {
  std::array<std::uint8_t, 48> mid{};
  mid[0] = 1;
  EXPECT_EQ(pack_bits48(mid), 1ULL << 47);
  mid = {};
  mid[47] = 1;
  EXPECT_EQ(pack_bits48(mid), 1ULL);
  for (int i = 0; i < 48; ++i) mid[static_cast<std::size_t>(i)] = 1;
  EXPECT_EQ(pack_bits48(mid), (1ULL << 48) - 1);
}

TEST(Sync, ExactPatterns) {
  EXPECT_EQ(classify_sync(kBsVoiceSync, 0), SyncKind::BsVoice);
  EXPECT_EQ(classify_sync(kBsDataSync, 0), SyncKind::BsData);
  EXPECT_EQ(classify_sync(kMsVoiceSync, 0), SyncKind::MsVoice);
  EXPECT_EQ(classify_sync(kMsDataSync, 0), SyncKind::MsData);
}

TEST(Sync, ToleratesBitErrors) {
  EXPECT_EQ(classify_sync(kBsVoiceSync ^ 0x7ULL, 3), SyncKind::BsVoice);
  EXPECT_EQ(classify_sync(kMsDataSync ^ 0x800000000001ULL, 2), SyncKind::MsData);
  EXPECT_EQ(classify_sync(kBsVoiceSync ^ 0x7ULL, 2), SyncKind::None);  // 3 errors > 2 allowed
}

TEST(Sync, NoiseIsNotSync) {
  EXPECT_EQ(classify_sync(0x123456789ABCULL, 4), SyncKind::None);
  EXPECT_EQ(classify_sync(0x0ULL, 4), SyncKind::None);
}

TEST(Sync, VoiceAndDataBsPatternsAre24BitsApart) {
  // They differ in exactly half the bits: the receiver can tell voice from data by a huge margin.
  const std::uint64_t x = kBsVoiceSync ^ kBsDataSync;
  int w = 0;
  for (int i = 0; i < 48; ++i) w += static_cast<int>((x >> i) & 1u);
  EXPECT_EQ(w, 24);
}
