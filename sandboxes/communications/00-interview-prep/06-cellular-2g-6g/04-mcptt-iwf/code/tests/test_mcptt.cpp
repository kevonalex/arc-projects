// Tests for Pack 06/04. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "mcptt.hpp"

using namespace iwf;

// ---------------------------------------------------------------- GTP-U parser (W 3.1)
TEST(Gtpu, BasicGPdu) {
  // flags 0x30 = version 1, PT 1, no E/S/PN; type 0xFF; length 4; TEID 0x11223344; payload "45 00 00 14"
  const uint8_t pkt[] = {0x30, 0xFF, 0x00, 0x04, 0x11, 0x22, 0x33, 0x44, 0x45, 0x00, 0x00, 0x14};
  GtpuHeader h;
  ASSERT_EQ(parse_gtpu(pkt, sizeof pkt, h), GtpuStatus::Ok);
  EXPECT_EQ(h.version, 1);
  EXPECT_EQ(h.pt, 1);
  EXPECT_FALSE(h.e);
  EXPECT_FALSE(h.s);
  EXPECT_FALSE(h.pn);
  EXPECT_EQ(h.msg_type, 0xFF);
  EXPECT_EQ(h.length, 4);
  EXPECT_EQ(h.teid, 0x11223344u);
  EXPECT_EQ(h.header_len, 8u);
  EXPECT_TRUE(h.ext_types.empty());
}
TEST(Gtpu, SequenceNumberAddsFourBytes) {
  // S flag set: flags 0x32; length = 4 optional + 2 payload = 6; seq 0xBEEF, N-PDU 0, next-ext 0
  const uint8_t pkt[] = {0x32, 0xFF, 0x00, 0x06, 0, 0, 0, 7, 0xBE, 0xEF, 0x00, 0x00, 0xAA, 0xBB};
  GtpuHeader h;
  ASSERT_EQ(parse_gtpu(pkt, sizeof pkt, h), GtpuStatus::Ok);
  EXPECT_TRUE(h.s);
  EXPECT_EQ(h.seq, 0xBEEF);
  EXPECT_EQ(h.teid, 7u);
  EXPECT_EQ(h.header_len, 12u);
}
TEST(Gtpu, ExtensionHeaderChainFiveGPduSessionContainer) {
  // E flag set (0x34). One extension (type 0x85, 1 unit = 4 bytes: len, 2 content bytes, next=0).
  const uint8_t pkt[] = {0x34, 0xFF, 0x00, 0x0A, 0, 0, 1, 0, 0x00, 0x00, 0x00, 0x85, 0x01, 0x00, 0x09, 0x00, 0xDE, 0xAD};
  GtpuHeader h;
  ASSERT_EQ(parse_gtpu(pkt, sizeof pkt, h), GtpuStatus::Ok);
  EXPECT_TRUE(h.e);
  ASSERT_EQ(h.ext_types.size(), 1u);
  EXPECT_EQ(h.ext_types[0], 0x85);
  EXPECT_EQ(h.header_len, 16u);  // 8 + 4 optional + 4 extension
}
TEST(Gtpu, TwoExtensionsChained) {
  const uint8_t pkt[] = {0x34, 0xFF, 0x00, 0x0E, 0, 0, 0, 1, 0, 0, 0, 0x85,
                         0x01, 0x11, 0x22, 0xC0,   // ext 1, next type 0xC0
                         0x01, 0x33, 0x44, 0x00,   // ext 2, end
                         0xDE, 0xAD};
  GtpuHeader h;
  ASSERT_EQ(parse_gtpu(pkt, sizeof pkt, h), GtpuStatus::Ok);
  ASSERT_EQ(h.ext_types.size(), 2u);
  EXPECT_EQ(h.ext_types[0], 0x85);
  EXPECT_EQ(h.ext_types[1], 0xC0);
  EXPECT_EQ(h.header_len, 20u);
}
TEST(Gtpu, MalformedInputsAreRejectedNotRead) {
  GtpuHeader h;
  const uint8_t shorty[] = {0x30, 0xFF, 0x00};
  EXPECT_EQ(parse_gtpu(shorty, sizeof shorty, h), GtpuStatus::TooShort);
  const uint8_t v2[] = {0x50, 0xFF, 0, 0, 0, 0, 0, 1};  // version 2
  EXPECT_EQ(parse_gtpu(v2, sizeof v2, h), GtpuStatus::BadVersion);
  const uint8_t gtpp[] = {0x20, 0xFF, 0, 0, 0, 0, 0, 1};  // PT = 0 (GTP')
  EXPECT_EQ(parse_gtpu(gtpp, sizeof gtpp, h), GtpuStatus::NotGtpu);
  const uint8_t trunc[] = {0x30, 0xFF, 0x00, 0x10, 0, 0, 0, 1, 1, 2, 3};  // claims 16 bytes, has 3
  EXPECT_EQ(parse_gtpu(trunc, sizeof trunc, h), GtpuStatus::LengthMismatch);
  const uint8_t zero_ext[] = {0x34, 0xFF, 0x00, 0x08, 0, 0, 0, 1, 0, 0, 0, 0x85, 0x00, 0, 0, 0};  // ext length 0
  EXPECT_EQ(parse_gtpu(zero_ext, sizeof zero_ext, h), GtpuStatus::BadExtension);
  const uint8_t overrun[] = {0x34, 0xFF, 0x00, 0x08, 0, 0, 0, 1, 0, 0, 0, 0x85, 0x05, 0, 0, 0};  // ext claims 20 bytes
  EXPECT_EQ(parse_gtpu(overrun, sizeof overrun, h), GtpuStatus::BadExtension);
  const uint8_t flags_no_room[] = {0x32, 0xFF, 0x00, 0x02, 0, 0, 0, 1, 0xAA, 0xBB};  // S set but length < 4
  EXPECT_EQ(parse_gtpu(flags_no_room, sizeof flags_no_room, h), GtpuStatus::LengthMismatch);
}

// ---------------------------------------------------------------- GTP-U builder (W 3.2)
TEST(Gtpu, BuildMatchesHandComputedBytes) {
  const uint8_t payload[] = {0x45, 0x00};
  auto b = build_gtpu(0xA1B2C3D4u, payload, sizeof payload);
  const std::vector<uint8_t> want = {0x30, 0xFF, 0x00, 0x02, 0xA1, 0xB2, 0xC3, 0xD4, 0x45, 0x00};
  EXPECT_EQ(b, want);
  auto s = build_gtpu(1u, payload, sizeof payload, true, 0x0102);
  const std::vector<uint8_t> want_s = {0x32, 0xFF, 0x00, 0x06, 0, 0, 0, 1, 0x01, 0x02, 0x00, 0x00, 0x45, 0x00};
  EXPECT_EQ(s, want_s);
}
TEST(Gtpu, BuildThenParseRoundTrip) {
  std::vector<uint8_t> payload(100);
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = static_cast<uint8_t>(i * 3);
  for (bool seq : {false, true}) {
    auto b = build_gtpu(0xDEADBEEFu, payload.data(), payload.size(), seq, 4242);
    GtpuHeader h;
    ASSERT_EQ(parse_gtpu(b.data(), b.size(), h), GtpuStatus::Ok);
    EXPECT_EQ(h.teid, 0xDEADBEEFu);
    EXPECT_EQ(h.s, seq);
    if (seq) {
      EXPECT_EQ(h.seq, 4242);
    }
    ASSERT_EQ(b.size() - h.header_len, payload.size());
    EXPECT_EQ(b[h.header_len], payload[0]);
    EXPECT_EQ(h.length, payload.size() + (seq ? 4u : 0u));
  }
}

// ---------------------------------------------------------------- NR numerology row (W 3.3)
TEST(Numerology, RowForTwentyMhz) {
  auto r0 = numerology_row(0, 20.0);
  EXPECT_EQ(r0.mu, 0);
  EXPECT_EQ(r0.scs_khz, 15);
  EXPECT_EQ(r0.slots_per_subframe, 1);
  EXPECT_NEAR(r0.slot_ms, 1.0, 1e-12);
  EXPECT_NEAR(r0.symbol_useful_us, 66.6667, 1e-3);
  EXPECT_EQ(r0.prbs, 106);
  EXPECT_NEAR(r0.occupied_mhz, 19.08, 1e-9);  // 106 * 12 * 15 kHz: 95.4 % of 20 MHz
  auto r1 = numerology_row(1, 20.0);
  EXPECT_EQ(r1.prbs, 51);
  EXPECT_NEAR(r1.occupied_mhz, 18.36, 1e-9);
}
TEST(Numerology, RowEdgeCases) {
  auto bad = numerology_row(7, 20.0);
  EXPECT_EQ(bad.mu, -1);
  auto nobw = numerology_row(2, 20.0);  // no table entry for (20 MHz, mu 2)
  EXPECT_EQ(nobw.mu, 2);
  EXPECT_EQ(nobw.scs_khz, 60);
  EXPECT_EQ(nobw.prbs, -1);
  EXPECT_LT(nobw.occupied_mhz, 0.0);
  auto fr2 = numerology_row(3, 100.0);
  EXPECT_EQ(fr2.prbs, 66);
  EXPECT_NEAR(fr2.occupied_mhz, 95.04, 1e-9);
  EXPECT_NEAR(fr2.slot_ms, 0.125, 1e-12);
}

// ---------------------------------------------------------------- LMR <-> MCPTT floor mapping (W 3.4)
TEST(Iwf, LmrEventsBecomeFloorMessages) {
  EXPECT_EQ(lmr_to_mcptt(LmrEvent::PttPressed), McpttMsg::FloorRequest);
  EXPECT_EQ(lmr_to_mcptt(LmrEvent::PttReleased), McpttMsg::FloorRelease);
  EXPECT_EQ(lmr_to_mcptt(LmrEvent::EmergencyPttPressed), McpttMsg::FloorRequestEmergency);
  EXPECT_EQ(lmr_to_mcptt(LmrEvent::ChannelBusy), McpttMsg::None);
}
TEST(Iwf, FloorOutcomesBecomeLmrActions) {
  EXPECT_EQ(mcptt_to_lmr(McpttFloor::Granted), LmrAction::TxPermit);
  EXPECT_EQ(mcptt_to_lmr(McpttFloor::Denied), LmrAction::BusyTone);
  EXPECT_EQ(mcptt_to_lmr(McpttFloor::Taken), LmrAction::BusyTone);
  EXPECT_EQ(mcptt_to_lmr(McpttFloor::Revoked), LmrAction::TxTerminate);
  EXPECT_EQ(mcptt_to_lmr(McpttFloor::Idle), LmrAction::ClearChannel);
}

// ---------------------------------------------------------------- talkgroup mapping (W 3.5)
TEST(Iwf, GroupUriRoundTrip) {
  EXPECT_EQ(make_group_uri(1234, "mcptt.example.org"), "sip:tg1234@mcptt.example.org");
  auto id = parse_group_uri("sip:tg1234@mcptt.example.org");
  ASSERT_TRUE(id.has_value());
  EXPECT_EQ(*id, 1234);
  EXPECT_EQ(*parse_group_uri(make_group_uri(65535, "x.y")), 65535);
  EXPECT_FALSE(parse_group_uri("sip:tg@x").has_value());
  EXPECT_FALSE(parse_group_uri("sip:tg12a@x").has_value());
  EXPECT_FALSE(parse_group_uri("sip:tg70000@x").has_value());
  EXPECT_FALSE(parse_group_uri("tel:+441234").has_value());
  EXPECT_FALSE(parse_group_uri("sip:tg12").has_value());
}
TEST(Iwf, TalkgroupMapIsOneToOne) {
  TalkgroupMap m;
  EXPECT_EQ(m.size(), 0u);
  EXPECT_TRUE(m.add(100, "sip:tg100@a.org"));
  EXPECT_TRUE(m.add(200, "sip:tg200@a.org"));
  EXPECT_FALSE(m.add(100, "sip:other@a.org"));    // LMR id already mapped
  EXPECT_FALSE(m.add(300, "sip:tg200@a.org"));    // URI already mapped
  EXPECT_EQ(m.size(), 2u);
  ASSERT_TRUE(m.to_mcptt(100).has_value());
  EXPECT_EQ(*m.to_mcptt(100), "sip:tg100@a.org");
  EXPECT_FALSE(m.to_mcptt(999).has_value());
  ASSERT_TRUE(m.to_lmr("sip:tg200@a.org").has_value());
  EXPECT_EQ(*m.to_lmr("sip:tg200@a.org"), 200);
  EXPECT_FALSE(m.to_lmr("sip:nope@a.org").has_value());
}
