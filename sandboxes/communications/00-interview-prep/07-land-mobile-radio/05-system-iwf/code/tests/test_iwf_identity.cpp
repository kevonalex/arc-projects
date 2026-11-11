// Worksheet 05, Q9-Q10 - identity packing, URIs and floor-control mapping. Synthetic values only.
#include <gtest/gtest.h>

#include <set>
#include <stdexcept>

#include "lmrsys/iwf_identity.hpp"

using namespace lmrsys;

TEST(Identity, PackGivesFiftySixBits) {
  const GlobalUnitId id{0xBEE00, 0x001, 0x00ABCD};
  EXPECT_EQ(pack_unit(id), 0xBEE0000100ABCDULL);
  const GlobalUnitId max{0xFFFFF, 0xFFF, 0xFFFFFF};
  EXPECT_EQ(pack_unit(max), 0x00FFFFFFFFFFFFFFULL);
}

TEST(Identity, UnpackInvertsPack) {
  const GlobalUnitId id{0x12345, 0x6AB, 0xCDEF01};
  const auto back = unpack_unit(pack_unit(id));
  EXPECT_EQ(back.wacn, id.wacn);
  EXPECT_EQ(back.sysid, id.sysid);
  EXPECT_EQ(back.unit, id.unit);
}

TEST(Identity, OutOfRangeThrows) {
  EXPECT_THROW(pack_unit(GlobalUnitId{0x100000, 0, 0}), std::out_of_range);
  EXPECT_THROW(pack_unit(GlobalUnitId{0, 0x1000, 0}), std::out_of_range);
  EXPECT_THROW(pack_unit(GlobalUnitId{0, 0, 0x1000000}), std::out_of_range);
  EXPECT_THROW(unpack_unit(1ULL << 56), std::out_of_range);
}

TEST(Uri, UnitFormat) {
  EXPECT_EQ(to_unit_uri(GlobalUnitId{0xBEE00, 0x001, 0x00ABCD}, "iwf.example"),
            "sip:p25-BEE00-001-00ABCD@iwf.example");
}

TEST(Uri, GroupFormat) {
  EXPECT_EQ(to_group_uri(GlobalGroupId{0xBEE00, 0x001, 0x1234}, "iwf.example"),
            "sip:p25tg-BEE00-001-1234@iwf.example");
}

TEST(Uri, ParseRoundTrip) {
  const GlobalUnitId id{0x00A1B, 0x0F2, 0x000007};
  const auto uri = to_unit_uri(id, "iwf.example");
  const auto back = parse_unit_uri(uri, "iwf.example");
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(back->wacn, id.wacn);
  EXPECT_EQ(back->sysid, id.sysid);
  EXPECT_EQ(back->unit, id.unit);
}

TEST(Uri, ParseAcceptsLowercaseHex) {
  const auto r = parse_unit_uri("sip:p25-bee00-001-00abcd@iwf.example", "iwf.example");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r->unit, 0x00ABCDu);
}

TEST(Uri, ParseRejectsEverythingElse) {
  const char* d = "iwf.example";
  EXPECT_FALSE(parse_unit_uri("", d));
  EXPECT_FALSE(parse_unit_uri("sip:p25-BEE00-001-00ABCD@other.example", d));  // wrong domain
  EXPECT_FALSE(parse_unit_uri("sip:p25-BEE00-001-00ABCD@iwf.example.evil", d));  // longer domain
  EXPECT_FALSE(parse_unit_uri("sip:p25tg-BEE00-001-1234@iwf.example", d));    // group URI, not unit
  EXPECT_FALSE(parse_unit_uri("sip:p25-BEE00-001-00ABCG@iwf.example", d));    // non-hex digit
  EXPECT_FALSE(parse_unit_uri("sip:p25-BEE0-0001-00ABCD@iwf.example", d));    // separators misplaced
  EXPECT_FALSE(parse_unit_uri("sip:p25-BEE00-001-00ABC@iwf.example", d));     // too short
  EXPECT_FALSE(parse_unit_uri("tel:p25-BEE00-001-00ABCD@iwf.example", d));    // wrong scheme
}

TEST(Floor, MappingIsABijection) {
  const LmrFloorEvent all[] = {LmrFloorEvent::PttPress,          LmrFloorEvent::PttRelease,
                               LmrFloorEvent::GrantToRequester,  LmrFloorEvent::TalkerChangedForOthers,
                               LmrFloorEvent::Deny,              LmrFloorEvent::Queued,
                               LmrFloorEvent::Revoked,           LmrFloorEvent::ChannelIdle};
  std::set<int> seen;
  for (auto e : all) {
    const auto m = to_mcptt(e);
    seen.insert(static_cast<int>(m));
    EXPECT_EQ(to_lmr(m), e);
  }
  EXPECT_EQ(seen.size(), 8u);
}

TEST(Floor, SpotChecks) {
  EXPECT_EQ(to_mcptt(LmrFloorEvent::PttPress), McpttFloorMsg::FloorRequest);
  EXPECT_EQ(to_mcptt(LmrFloorEvent::GrantToRequester), McpttFloorMsg::FloorGranted);
  EXPECT_EQ(to_mcptt(LmrFloorEvent::TalkerChangedForOthers), McpttFloorMsg::FloorTaken);
  EXPECT_EQ(to_mcptt(LmrFloorEvent::Revoked), McpttFloorMsg::FloorRevoke);
  EXPECT_EQ(to_lmr(McpttFloorMsg::FloorIdle), LmrFloorEvent::ChannelIdle);
}
