#include <gtest/gtest.h>

#include "p25p1/ldu.hpp"

using namespace p25p1;

TEST(Ldu1, ParseGroupVoiceLc) {
  // SYNTHETIC link control: TG 0x1234 from unit 0x00ABCD, emergency bit set.
  const std::array<std::uint8_t, 9> lc = {0x00, 0x00, 0x84, 0x00, 0x12, 0x34, 0x00, 0xAB, 0xCD};
  const GroupVoiceLc g = parse_group_voice_lc(lc);
  EXPECT_EQ(g.lcf, 0x00);
  EXPECT_EQ(g.mfid, 0x00);
  EXPECT_EQ(g.service_options, 0x84);
  EXPECT_EQ(g.tgid, 0x1234);
  EXPECT_EQ(g.source, 0x00ABCDu);
  EXPECT_TRUE(service_options_emergency(g.service_options));
  EXPECT_FALSE(service_options_emergency(0x04));
}

TEST(Ldu1, LargeIdsKeepAllBits) {
  const std::array<std::uint8_t, 9> lc = {0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  const GroupVoiceLc g = parse_group_voice_lc(lc);
  EXPECT_EQ(g.tgid, 0xFFFF);
  EXPECT_EQ(g.source, 0xFFFFFFu);  // 24-bit, NOT 32-bit
}

TEST(Ldu2, ParseEncryptionSync) {
  const std::array<std::uint8_t, 12> es = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0x84, 0xAB, 0xCD};
  const EncryptionSync e = parse_encryption_sync(es);
  for (std::size_t i = 0; i < 9; ++i) EXPECT_EQ(e.mi[i], i + 1);
  EXPECT_EQ(e.algid, 0x84);
  EXPECT_EQ(e.kid, 0xABCD);
  EXPECT_FALSE(is_clear(e.algid));
}

TEST(Ldu2, ClearVoiceUsesAlgid80) {
  const std::array<std::uint8_t, 12> es = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0};
  const EncryptionSync e = parse_encryption_sync(es);
  EXPECT_TRUE(is_clear(e.algid));
  EXPECT_STREQ(algid_name(e.algid), "clear");
  EXPECT_STREQ(algid_name(0x84), "AES-256");
}
