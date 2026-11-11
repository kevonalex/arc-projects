// Worksheet 06, Build C (Q9-Q10) - DMR burst splitter on SYNTHETIC bit streams (never live traffic).
#include <gtest/gtest.h>

#include "lmrb/dmr_splitter.hpp"
#include "lmrb/rng.hpp"

using namespace lmrb;

namespace {
dmr::Bits264 counting_burst() {
  dmr::Bits264 b{};
  for (std::size_t i = 0; i < b.size(); ++i) b[i] = static_cast<std::uint8_t>(i % 251);  // unique-ish markers
  return b;
}

// a stream of n slots; slot k gets middle mids[k] (0 = random middle)
std::vector<std::uint8_t> make_stream(XorShift64& rng, const std::vector<std::uint64_t>& mids,
                                      std::size_t junk_prefix) {
  std::vector<std::uint8_t> bits = random_bits(rng, junk_prefix);
  for (const std::uint64_t m : mids) {
    const std::uint64_t mid = (m != 0) ? m : (rng.next() & ((1ULL << 48) - 1));
    const auto slot = dmr::make_slot(rng, mid);
    bits.insert(bits.end(), slot.begin(), slot.end());
  }
  return bits;
}
}  // namespace

TEST(DmrSplitter, SlotArithmetic) {
  EXPECT_EQ(dmr::kGapBits + dmr::kBurstBits, dmr::kSlotBits);
  EXPECT_EQ(2 * dmr::kHalfBits + dmr::kMidBits, dmr::kBurstBits);
  EXPECT_EQ(3 * dmr::kVoiceFrameBits, 2 * dmr::kHalfBits);  // 216 bits of voice: the middle is NOT voice
}

TEST(DmrSplitter, SplitAndJoinRoundTrip) {
  const dmr::Bits264 b = counting_burst();
  const dmr::BurstParts p = split_burst(b);
  EXPECT_EQ(p.first[0], b[0]);
  EXPECT_EQ(p.first[107], b[107]);
  EXPECT_EQ(p.mid[0], b[108]);
  EXPECT_EQ(p.mid[47], b[155]);
  EXPECT_EQ(p.second[0], b[156]);
  EXPECT_EQ(p.second[107], b[263]);
  EXPECT_EQ(join_burst(p), b);
}

TEST(DmrSplitter, VoiceFrameOneStraddlesTheMiddle) {
  const dmr::Bits264 b = counting_burst();
  const auto f = extract_voice_frames(split_burst(b));
  for (std::size_t i = 0; i < 72; ++i) EXPECT_EQ(f[0][i], b[i]) << i;
  for (std::size_t i = 0; i < 36; ++i) EXPECT_EQ(f[1][i], b[72 + i]) << i;           // end of first half
  for (std::size_t i = 0; i < 36; ++i) EXPECT_EQ(f[1][36 + i], b[156 + i]) << i;     // start of second half
  for (std::size_t i = 0; i < 72; ++i) EXPECT_EQ(f[2][i], b[192 + i]) << i;
}

TEST(DmrSplitter, SplitStreamFindsSyncInTheMiddleOfEachSlot) {
  XorShift64 rng(21);
  const auto bits = make_stream(rng, {0, dmr::kBsVoiceSync, 0, dmr::kMsDataSync}, 0);
  ASSERT_EQ(bits.size(), 4 * dmr::kSlotBits);
  const auto slots = split_stream(bits, 0, 4);
  ASSERT_EQ(slots.size(), 4u);
  EXPECT_EQ(slots[0].sync, dmr::SyncKind::None);
  EXPECT_EQ(slots[1].sync, dmr::SyncKind::BsVoice);
  EXPECT_EQ(slots[2].sync, dmr::SyncKind::None);
  EXPECT_EQ(slots[3].sync, dmr::SyncKind::MsData);
  for (std::size_t k = 0; k < slots.size(); ++k) EXPECT_EQ(slots[k].index, k);
  // the burst starts 24 bits into the slot
  EXPECT_EQ(slots[1].parts.first[0], bits[dmr::kSlotBits + dmr::kGapBits]);
}

TEST(DmrSplitter, PartialTailSlotIsIgnoredAndPhaseShiftsTheSlots) {
  XorShift64 rng(22);
  auto bits = make_stream(rng, {0, 0, 0}, 0);
  bits.resize(bits.size() - 10);  // 864 - 10 = 854 bits: the third slot is incomplete
  EXPECT_EQ(split_stream(bits, 0, 4).size(), 2u);     // floor(854 / 288) = 2
  EXPECT_EQ(split_stream(bits, 100, 4).size(), 2u);   // floor(754 / 288) = 2
  EXPECT_EQ(split_stream(bits, 300, 4).size(), 1u);   // floor(554 / 288) = 1
  EXPECT_TRUE(split_stream({}, 0, 4).empty());
}

TEST(DmrSplitter, FindsTheSlotPhaseFromTwoSyncBursts) {
  XorShift64 rng(23);
  // 100 junk bits, then 12 slots with a voice sync in slot 0 and a data sync in slot 6
  std::vector<std::uint64_t> mids(12, 0);
  mids[0] = dmr::kBsVoiceSync;
  mids[6] = dmr::kBsDataSync;
  const auto bits = make_stream(rng, mids, 100);
  const auto phase = find_slot_alignment(bits, 4);
  ASSERT_TRUE(phase.has_value());
  EXPECT_EQ(*phase, 100u);
}

TEST(DmrSplitter, AlignmentToleratesSyncBitErrorsUpToTheThreshold) {
  XorShift64 rng(24);
  std::vector<std::uint64_t> mids(8, 0);
  // 4 bit errors in the voice sync word
  mids[2] = dmr::kBsVoiceSync ^ ((1ULL << 3) | (1ULL << 17) | (1ULL << 30) | (1ULL << 44));
  const auto bits = make_stream(rng, mids, 37);
  const auto ok = find_slot_alignment(bits, 4);
  ASSERT_TRUE(ok.has_value());
  EXPECT_EQ(*ok, 37u);
  EXPECT_FALSE(find_slot_alignment(bits, 3).has_value());  // threshold below the damage: no sync anywhere
}

TEST(DmrSplitter, AlignmentGivesNothingWithoutSyncOrInput) {
  XorShift64 rng(25);
  const auto bits = make_stream(rng, std::vector<std::uint64_t>(6, 0), 11);
  EXPECT_FALSE(find_slot_alignment(bits, 2).has_value());
  EXPECT_FALSE(find_slot_alignment({}, 4).has_value());
}
