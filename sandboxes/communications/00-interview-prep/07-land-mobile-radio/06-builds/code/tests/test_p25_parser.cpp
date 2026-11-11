// Worksheet 06, Build B (Q5-Q8) - parser for SYNTHETIC P25-style frames (never live traffic).
#include <gtest/gtest.h>

#include "lmrb/p25_parser.hpp"
#include "lmrb/rng.hpp"

using namespace lmrb;
using p25p1::Duid;

namespace {
constexpr std::uint16_t kNac = 0x293;

std::vector<std::uint8_t> payload784(std::uint64_t seed) {
  XorShift64 r(seed);
  return random_symbols(r, 784);
}
}  // namespace

TEST(P25Parser, SymbolsToBitsIsMsbFirst) {
  EXPECT_EQ(symbols_to_bits({1, 2, 3, 0}, 0, 4), 0x6Cu);
  EXPECT_EQ(symbols_to_bits({3, 3, 1, 2, 0}, 1, 3), 0x36u);  // symbols 3,1,2 -> 11 01 10 = 0b110110
}

TEST(P25Parser, StatusSymbolRule) {
  std::vector<std::uint8_t> data(70);
  for (std::size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>(i % 4);
  const auto air = p25p1::insert_status_symbols(data, 3);
  ASSERT_EQ(air.size(), 72u);
  EXPECT_EQ(air[35], 3);  // (35 + 1) % 36 == 0
  EXPECT_EQ(air[71], 3);  // (71 + 1) % 36 == 0
  EXPECT_EQ(strip_status_symbols(air, 0), data);
}

TEST(P25Parser, StripCountsFromTheFrameStartNotFromTheStream) {
  std::vector<std::uint8_t> data(60);
  for (std::size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>((i * 3) % 4);
  auto air = p25p1::insert_status_symbols(data, 3);
  std::vector<std::uint8_t> stream(11, 1);  // 11 symbols of junk before the frame
  stream.insert(stream.end(), air.begin(), air.end());
  EXPECT_EQ(strip_status_symbols(stream, 11), data);
}

TEST(P25Parser, ShortStreamsAreSafe) {
  EXPECT_TRUE(strip_status_symbols({}, 0).empty());
  EXPECT_TRUE(strip_status_symbols({1, 2, 3}, 5).empty());
  std::vector<std::uint8_t> d36(36, 2);
  EXPECT_EQ(p25p1::insert_status_symbols(d36, 0).size(), 37u);
}

TEST(P25Parser, FindSyncExactAndTolerant) {
  XorShift64 r(7);
  std::vector<std::uint8_t> air = random_symbols(r, 17);
  const auto sync = p25p1::bits_to_symbols(p25p1::kFrameSync48, 24);
  air.insert(air.end(), sync.begin(), sync.end());
  const auto tail = random_symbols(r, 40);
  air.insert(air.end(), tail.begin(), tail.end());

  auto hit = find_sync(air, 0, 0);
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(hit->pos, 17u);
  EXPECT_EQ(hit->errors, 0);

  // three single-bit errors inside the sync
  air[17 + 2] ^= 1;
  air[17 + 9] ^= 2;
  air[17 + 20] ^= 1;
  EXPECT_FALSE(find_sync(air, 0, 2).has_value());
  hit = find_sync(air, 0, 4);
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(hit->pos, 17u);
  EXPECT_EQ(hit->errors, 3);
}

TEST(P25Parser, FindSyncRespectsFromAndShortInput) {
  const auto sync = p25p1::bits_to_symbols(p25p1::kFrameSync48, 24);
  EXPECT_TRUE(find_sync(sync, 0, 0).has_value());
  EXPECT_FALSE(find_sync(sync, 1, 0).has_value());
  EXPECT_FALSE(find_sync({1, 2, 3}, 0, 4).has_value());
}

TEST(P25Parser, DecodeNidExactAndNoisy) {
  const std::uint64_t nid = p25p1::nid_encode64(kNac, Duid::Ldu1);
  auto r = decode_nid(nid, {kNac}, 11);
  ASSERT_TRUE(r.ok);
  EXPECT_EQ(r.nac, kNac);
  EXPECT_EQ(r.duid, Duid::Ldu1);
  EXPECT_EQ(r.errors, 0);

  // 11 flipped bits (spread out, parity bit included): still unique because d_min of the 64-bit code is 24
  std::uint64_t noisy = nid;
  for (int b : {0, 6, 13, 21, 28, 33, 40, 47, 52, 58, 63}) noisy ^= (1ULL << b);
  r = decode_nid(noisy, {kNac, 0x294}, 11);
  ASSERT_TRUE(r.ok);
  EXPECT_EQ(r.nac, kNac);
  EXPECT_EQ(r.duid, Duid::Ldu1);
  EXPECT_EQ(r.errors, 11);
}

TEST(P25Parser, DecodeNidRejects) {
  const std::uint64_t nid = p25p1::nid_encode64(kNac, Duid::Tsbk);
  EXPECT_FALSE(decode_nid(nid, {0x111}, 11).ok);  // NAC not configured: nothing within 11 bits
  std::uint64_t noisy = nid;
  for (int b : {1, 9, 17, 25, 33}) noisy ^= (1ULL << b);
  EXPECT_FALSE(decode_nid(noisy, {kNac}, 3).ok);  // 5 errors but tolerance 3
}

TEST(P25Parser, DecodeNidWithEmptyListSearchesAllNacs) {
  const auto r = decode_nid(p25p1::nid_encode64(0xABC, Duid::Pdu), {}, 11);
  ASSERT_TRUE(r.ok);
  EXPECT_EQ(r.nac, 0xABC);
  EXPECT_EQ(r.duid, Duid::Pdu);
}

TEST(P25Parser, DispatchTables) {
  EXPECT_EQ(classify(Duid::Hdu), FrameClass::Header);
  EXPECT_EQ(classify(Duid::Ldu1), FrameClass::Voice);
  EXPECT_EQ(classify(Duid::Ldu2), FrameClass::Voice);
  EXPECT_EQ(classify(Duid::Tsbk), FrameClass::Control);
  EXPECT_EQ(classify(Duid::Pdu), FrameClass::Data);
  EXPECT_EQ(classify(Duid::Tdu), FrameClass::Terminator);
  EXPECT_EQ(classify(Duid::TduLc), FrameClass::Terminator);
  EXPECT_EQ(expected_payload_symbols(Duid::Ldu1), std::optional<std::size_t>(784));
  EXPECT_EQ(expected_payload_symbols(Duid::Ldu2), std::optional<std::size_t>(784));
  EXPECT_FALSE(expected_payload_symbols(Duid::Tsbk).has_value());
}

TEST(P25Parser, LduFrameHasEightHundredSixtyFourAirSymbols) {
  // 24 sync + 32 NID + 784 payload = 840 data symbols; 840 / 35 = 24 status symbols -> 864 = 1728 bits = 180 ms
  const auto air = p25p1::make_frame(kNac, Duid::Ldu1, payload784(1));
  EXPECT_EQ(air.size(), 864u);
}

TEST(P25Parser, ParsesACleanLduAfterJunk) {
  XorShift64 r(99);
  std::vector<std::uint8_t> air = random_symbols(r, 13);
  const auto pl = payload784(2);
  const auto frame = p25p1::make_frame(kNac, Duid::Ldu1, pl);
  air.insert(air.end(), frame.begin(), frame.end());

  const auto f = parse_frame(air, {kNac}, 4);
  ASSERT_TRUE(f.has_value());
  EXPECT_EQ(f->frame_start, 13u);
  EXPECT_EQ(f->nac, kNac);
  EXPECT_EQ(f->duid, Duid::Ldu1);
  EXPECT_EQ(f->sync_errors, 0);
  EXPECT_EQ(f->nid_errors, 0);
  EXPECT_EQ(f->payload, pl);
}

TEST(P25Parser, ToleratesSyncAndNidErrors) {
  const auto pl = payload784(3);
  auto air = p25p1::make_frame(kNac, Duid::Ldu2, pl);
  // 2 sync bit errors (data symbols 1 and 10) and 4 NID bit errors (data symbols 24+3, +10, +20, +31)
  for (std::size_t k : {1u, 10u}) air[p25p1::air_index_of_data(k)] ^= 1;
  for (std::size_t k : {27u, 34u, 44u, 55u}) air[p25p1::air_index_of_data(k)] ^= 2;
  const auto f = parse_frame(air, {kNac}, 4);
  ASSERT_TRUE(f.has_value());
  EXPECT_EQ(f->duid, Duid::Ldu2);
  EXPECT_EQ(f->sync_errors, 2);
  EXPECT_EQ(f->nid_errors, 4);
  EXPECT_EQ(f->payload, pl);
}

TEST(P25Parser, SkipsAFalseSync) {
  XorShift64 r(5);
  std::vector<std::uint8_t> air = random_symbols(r, 6);
  const auto fake = p25p1::bits_to_symbols(p25p1::kFrameSync48, 24);  // a sync-looking pattern ...
  air.insert(air.end(), fake.begin(), fake.end());
  const auto junk = random_symbols(r, 32);                            // ... followed by a garbage NID
  air.insert(air.end(), junk.begin(), junk.end());
  const std::size_t real_start = air.size();
  const auto pl = payload784(4);
  const auto frame = p25p1::make_frame(kNac, Duid::Ldu1, pl);
  air.insert(air.end(), frame.begin(), frame.end());

  const auto f = parse_frame(air, {kNac}, 4);
  ASSERT_TRUE(f.has_value());
  EXPECT_EQ(f->frame_start, real_start);
  EXPECT_EQ(f->payload, pl);
}

TEST(P25Parser, RejectsWrongNacMissingSyncAndTruncation) {
  const auto pl = payload784(6);
  const auto frame = p25p1::make_frame(kNac, Duid::Ldu1, pl);
  EXPECT_FALSE(parse_frame(frame, {0x111}, 4).has_value());  // not our network

  XorShift64 r(8);
  EXPECT_FALSE(parse_frame(random_symbols(r, 500), {kNac}, 0).has_value());  // no sync at all

  std::vector<std::uint8_t> cut(frame.begin(), frame.begin() + 500);         // LDU cut short
  EXPECT_FALSE(parse_frame(cut, {kNac}, 4).has_value());
  std::vector<std::uint8_t> tiny(frame.begin(), frame.begin() + 40);        // not even a full NID
  EXPECT_FALSE(parse_frame(tiny, {kNac}, 4).has_value());
}

TEST(P25Parser, TrailingJunkIsNotPartOfAFixedSizeFrame) {
  XorShift64 r(10);
  const auto pl = payload784(7);
  auto air = p25p1::make_frame(kNac, Duid::Ldu1, pl);
  const auto junk = random_symbols(r, 100);
  air.insert(air.end(), junk.begin(), junk.end());
  const auto f = parse_frame(air, {kNac}, 4);
  ASSERT_TRUE(f.has_value());
  EXPECT_EQ(f->payload.size(), 784u);
  EXPECT_EQ(f->payload, pl);
}

TEST(P25Parser, SecondFrameFoundWithFrom) {
  const auto pl1 = payload784(11);
  const auto pl2 = payload784(12);
  auto air = p25p1::make_frame(kNac, Duid::Ldu1, pl1);
  const auto second = p25p1::make_frame(kNac, Duid::Ldu2, pl2);
  air.insert(air.end(), second.begin(), second.end());
  const auto a = parse_frame(air, {kNac}, 4);
  ASSERT_TRUE(a.has_value());
  EXPECT_EQ(a->duid, Duid::Ldu1);
  const auto b = parse_frame(air, {kNac}, 4, a->frame_start + 864);
  ASSERT_TRUE(b.has_value());
  EXPECT_EQ(b->duid, Duid::Ldu2);
  EXPECT_EQ(b->frame_start, 864u);
  EXPECT_EQ(b->payload, pl2);
}
