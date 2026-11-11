// Tests for Capstone M2. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "rtp_jitter.hpp"

using namespace gw;

// ---------------------------------------------------------------- mu-law (W 3.1)
TEST(Mulaw, KnownCodes) {
  EXPECT_EQ(mulaw_encode(0), 0xFF);
  EXPECT_EQ(mulaw_encode(32767), 0x80);
  EXPECT_EQ(mulaw_encode(-32768), 0x00);
  EXPECT_EQ(mulaw_encode(-1), 0x7F);
  EXPECT_EQ(mulaw_decode(0xFF), 0);
  EXPECT_EQ(mulaw_decode(0x7F), 0);  // "negative zero" decodes to 0 as well
  EXPECT_EQ(mulaw_decode(0x80), 32124);
  EXPECT_EQ(mulaw_decode(0x00), -32124);
}
TEST(Mulaw, RoundTripErrorIsProportionalToLevel) {
  for (int v = -32768; v <= 32767; v += 7) {
    const int16_t pcm = static_cast<int16_t>(v);
    const int back = mulaw_decode(mulaw_encode(pcm));
    const int err = std::abs(back - std::min(std::max(v, -32124), 32124));
    // companding: step size grows with amplitude, so the allowed error does too (about 3 % of the level + 1 LSB-ish)
    EXPECT_LE(err, std::abs(v) / 16 + 8) << "pcm " << v;
  }
}
TEST(Mulaw, MonotonicAndSignSymmetric) {
  int prev = -40000;
  for (int v = -32768; v <= 32767; v += 13) {
    const int d = mulaw_decode(mulaw_encode(static_cast<int16_t>(v)));
    EXPECT_GE(d, prev);
    prev = d;
  }
  for (int v : {1, 100, 1000, 12345, 30000}) {
    EXPECT_EQ(mulaw_decode(mulaw_encode(static_cast<int16_t>(-v))), -mulaw_decode(mulaw_encode(static_cast<int16_t>(v))));
  }
}

// ---------------------------------------------------------------- RTP header (W 3.2)
TEST(Rtp, PackKnownBytes) {
  RtpHeader h;
  h.pt = 0;
  h.seq = 1;
  h.timestamp = 160;
  h.ssrc = 0xDEADBEEF;
  const uint8_t pl[2] = {0xAA, 0xBB};
  auto b = rtp_pack(h, pl, 2);
  const std::vector<uint8_t> want = {0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xBB};
  EXPECT_EQ(b, want);
  h.marker = true;
  auto m = rtp_pack(h, pl, 0);
  ASSERT_EQ(m.size(), 12u);
  EXPECT_EQ(m[1], 0x80);
}
TEST(Rtp, ParseRoundTrip) {
  RtpHeader h;
  h.marker = true;
  h.pt = 96;
  h.seq = 0xFFFE;
  h.timestamp = 0x01020304;
  h.ssrc = 0xCAFEBABE;
  std::vector<uint8_t> pl(160, 0x7E);
  auto b = rtp_pack(h, pl.data(), pl.size());
  RtpHeader g;
  size_t off = 0, len = 0;
  ASSERT_EQ(rtp_parse(b.data(), b.size(), g, off, len), RtpStatus::Ok);
  EXPECT_EQ(g.version, 2);
  EXPECT_TRUE(g.marker);
  EXPECT_EQ(g.pt, 96);
  EXPECT_EQ(g.seq, 0xFFFE);
  EXPECT_EQ(g.timestamp, 0x01020304u);
  EXPECT_EQ(g.ssrc, 0xCAFEBABEu);
  EXPECT_EQ(off, 12u);
  EXPECT_EQ(len, 160u);
}
TEST(Rtp, CsrcExtensionAndPaddingAreSkipped) {
  // V=2, P=1, X=1, CC=2; M=0 PT=0; seq 5; ts 0; ssrc 1; 2 CSRCs; ext (id 0xBEDE, length 1 word); payload 3 bytes; padding 4 bytes
  std::vector<uint8_t> b = {0xB2, 0x00, 0x00, 0x05, 0, 0, 0, 0, 0, 0, 0, 1,
                            0, 0, 0, 10, 0, 0, 0, 11,                         // CSRCs
                            0xBE, 0xDE, 0x00, 0x01, 0x11, 0x22, 0x33, 0x44,   // extension header + 1 word
                            0xAA, 0xBB, 0xCC,                                 // payload
                            0x00, 0x00, 0x00, 0x04};                          // padding (last byte = count)
  RtpHeader g;
  size_t off = 0, len = 0;
  ASSERT_EQ(rtp_parse(b.data(), b.size(), g, off, len), RtpStatus::Ok);
  EXPECT_EQ(g.cc, 2);
  EXPECT_TRUE(g.extension);
  EXPECT_TRUE(g.padding);
  EXPECT_EQ(off, 12u + 8u + 8u);
  EXPECT_EQ(len, 3u);
  EXPECT_EQ(b[off], 0xAA);
}
TEST(Rtp, MalformedPacketsAreRejected) {
  RtpHeader g;
  size_t off = 0, len = 0;
  std::vector<uint8_t> shorty(11, 0x80);
  EXPECT_EQ(rtp_parse(shorty.data(), shorty.size(), g, off, len), RtpStatus::TooShort);
  std::vector<uint8_t> v1 = {0x40, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};
  EXPECT_EQ(rtp_parse(v1.data(), v1.size(), g, off, len), RtpStatus::BadVersion);
  std::vector<uint8_t> cc = {0x8F, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};  // CC=15 but no CSRC bytes follow
  EXPECT_EQ(rtp_parse(cc.data(), cc.size(), g, off, len), RtpStatus::Truncated);
  std::vector<uint8_t> pad = {0xA0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0x01, 0x09};  // padding count 9 > payload 2
  EXPECT_EQ(rtp_parse(pad.data(), pad.size(), g, off, len), RtpStatus::Truncated);
  std::vector<uint8_t> ext = {0x90, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0xBE, 0xDE, 0x00, 0x09};  // ext claims 9 words
  EXPECT_EQ(rtp_parse(ext.data(), ext.size(), g, off, len), RtpStatus::Truncated);
}

// ---------------------------------------------------------------- sequence numbers + loss + jitter (W 3.3 - 3.4)
TEST(Seq, SerialArithmetic) {
  EXPECT_TRUE(seq_newer(5, 4));
  EXPECT_FALSE(seq_newer(4, 5));
  EXPECT_FALSE(seq_newer(7, 7));
  EXPECT_TRUE(seq_newer(0, 65535));     // wrap: 0 is newer than 65535
  EXPECT_FALSE(seq_newer(65535, 0));
  EXPECT_TRUE(seq_newer(100, 65000));   // 636 steps ahead across the wrap
  EXPECT_FALSE(seq_newer(0x8000, 0));   // exactly half the space away: by convention not newer
}
TEST(Seq, UnwrapperIsMonotonicAcrossTheWrapAndToleratesReorder) {
  SeqUnwrapper u;
  EXPECT_EQ(u.extend(65534), 65534);
  EXPECT_EQ(u.extend(65535), 65535);
  EXPECT_EQ(u.extend(0), 65536);
  EXPECT_EQ(u.extend(1), 65537);
  EXPECT_EQ(u.extend(65535), 65535);  // a late straggler from before the wrap goes BACK, not forward 65536
  EXPECT_EQ(u.extend(2), 65538);
}
TEST(Loss, ExpectedReceivedLost) {
  LossCounter c;
  for (uint16_t s : {10, 11, 12, 15, 16}) c.on_packet(s);  // 13 and 14 never arrived
  EXPECT_EQ(c.received(), 5u);
  EXPECT_EQ(c.expected(), 7);
  EXPECT_EQ(c.lost(), 2);
  EXPECT_NEAR(c.loss_fraction(), 2.0 / 7.0, 1e-12);
}
TEST(Loss, WrapAndDuplicate) {
  LossCounter c;
  for (uint16_t s : {65534, 65535, 0, 1, 1}) c.on_packet(s);  // duplicate of 1
  EXPECT_EQ(c.expected(), 4);
  EXPECT_EQ(c.received(), 5u);
  EXPECT_EQ(c.lost(), -1);                 // duplicates make "lost" negative (RFC 3550 allows this)
  EXPECT_DOUBLE_EQ(c.loss_fraction(), 0.0);  // reported loss fraction is clamped at zero
}
TEST(Jitter, Rfc3550Estimator) {
  // 20 ms packets (160 ticks at 8 kHz); the 3rd packet arrives 80 ticks (10 ms) late; others on time.
  const uint32_t delay[5] = {0, 0, 80, 0, 0};
  JitterEstimator j;
  for (uint32_t i = 0; i < 5; ++i) j.update(1000 + 160 * i, 5000 + 160 * i + delay[i]);
  // D = 0, 80, -80, 0 -> J: 0, 5, 9.6875, 9.08203125
  EXPECT_NEAR(j.jitter(), 9.08203125, 1e-9);
  EXPECT_NEAR(j.jitter_ms(8000), 1.13525390625, 1e-9);
}
TEST(Jitter, ConstantDelayIsZeroJitter) {
  JitterEstimator j;
  for (uint32_t i = 0; i < 50; ++i) j.update(160 * i, 160 * i + 777);  // a big constant delay is NOT jitter
  EXPECT_DOUBLE_EQ(j.jitter(), 0.0);
}
TEST(Jitter, SurvivesTimestampWrap) {
  JitterEstimator j;
  const uint32_t start = 0xFFFFFF00u;
  for (uint32_t i = 0; i < 20; ++i) j.update(start + 160 * i, start + 160 * i + 40);
  EXPECT_DOUBLE_EQ(j.jitter(), 0.0);
}

// ---------------------------------------------------------------- jitter buffer (W 3.4 - 3.5)
namespace {
std::vector<uint8_t> frame(uint8_t tag) { return std::vector<uint8_t>(160, tag); }
}  // namespace
TEST(JitterBuffer, PrebuffersThenPlaysInOrder) {
  JitterBuffer jb(2, 8);
  EXPECT_EQ(jb.push(0, frame(0)), PushResult::Accepted);
  auto f = jb.pop();  // only 1 frame stored, target is 2
  EXPECT_FALSE(f.present);
  EXPECT_FALSE(f.concealed);
  EXPECT_FALSE(jb.started());
  jb.push(1, frame(1));
  f = jb.pop();
  ASSERT_TRUE(f.present);
  EXPECT_EQ(f.seq, 0);
  EXPECT_EQ(f.data, frame(0));
  EXPECT_TRUE(jb.started());
  jb.push(2, frame(2));
  f = jb.pop();
  ASSERT_TRUE(f.present);
  EXPECT_EQ(f.data, frame(1));
  EXPECT_EQ(jb.concealed(), 0u);
}
TEST(JitterBuffer, ReorderingWithinTheTargetIsAbsorbed) {
  JitterBuffer jb(2, 8);
  jb.push(0, frame(0));
  jb.push(2, frame(2));  // seq 1 is late
  auto f0 = jb.pop();
  ASSERT_TRUE(f0.present);
  EXPECT_EQ(f0.seq, 0);
  jb.push(1, frame(1));  // arrives before its slot
  auto f1 = jb.pop();
  ASSERT_TRUE(f1.present);
  EXPECT_EQ(f1.seq, 1);
  EXPECT_EQ(f1.data, frame(1));
  EXPECT_EQ(jb.concealed(), 0u);
  EXPECT_EQ(jb.late(), 0u);
}
TEST(JitterBuffer, TooLateMeansConcealmentAndALateDrop) {
  JitterBuffer jb(2, 8);
  jb.push(0, frame(0));
  jb.push(2, frame(2));
  EXPECT_TRUE(jb.pop().present);          // plays 0
  auto miss = jb.pop();                   // slot of seq 1: nothing there
  EXPECT_TRUE(miss.concealed);
  EXPECT_FALSE(miss.present);
  EXPECT_EQ(miss.seq, 1);
  EXPECT_EQ(miss.data, frame(0));         // PLC = repeat the previous frame
  EXPECT_EQ(jb.push(1, frame(1)), PushResult::Late);  // arrives after its slot passed: useless
  EXPECT_EQ(jb.concealed(), 1u);
  EXPECT_EQ(jb.late(), 1u);
  auto f2 = jb.pop();
  ASSERT_TRUE(f2.present);
  EXPECT_EQ(f2.seq, 2);
}
TEST(JitterBuffer, DuplicatesAndFarFutureAreRejected) {
  JitterBuffer jb(2, 4);
  EXPECT_EQ(jb.push(10, frame(1)), PushResult::Accepted);
  EXPECT_EQ(jb.push(10, frame(1)), PushResult::Duplicate);
  EXPECT_EQ(jb.push(14, frame(9)), PushResult::TooEarly);  // 4 ahead of the oldest >= max_frames
  EXPECT_EQ(jb.push(13, frame(3)), PushResult::Accepted);
  EXPECT_EQ(jb.duplicates(), 1u);
  EXPECT_EQ(jb.too_early(), 1u);
  EXPECT_EQ(jb.received(), 2u);
  EXPECT_EQ(jb.depth(), 2);
}
TEST(JitterBuffer, SequenceWrapIsTransparent) {
  JitterBuffer jb(2, 8);
  for (uint16_t s : {65534, 65535, 0, 1}) jb.push(s, frame(static_cast<uint8_t>(s)));
  std::vector<int64_t> played;
  for (int i = 0; i < 4; ++i) {
    auto f = jb.pop();
    ASSERT_TRUE(f.present) << i;
    played.push_back(f.seq);
  }
  EXPECT_EQ(played, (std::vector<int64_t>{65534, 65535, 65536, 65537}));
  EXPECT_EQ(jb.concealed(), 0u);
}
TEST(JitterBuffer, MissingFirstFrameIsSilenceAndResetRestarts) {
  JitterBuffer jb(2, 8);
  jb.push(5, frame(5));
  jb.push(7, frame(7));
  EXPECT_TRUE(jb.pop().present);  // 5
  auto f = jb.pop();              // 6 is missing
  EXPECT_TRUE(f.concealed);
  jb.reset();
  EXPECT_FALSE(jb.started());
  EXPECT_EQ(jb.depth(), 0);
  EXPECT_EQ(jb.concealed(), 0u);
  EXPECT_EQ(jb.push(100, frame(1)), PushResult::Accepted);  // a new talk-spurt may start anywhere
  jb.push(101, frame(2));
  EXPECT_EQ(jb.pop().seq, 100);
}
TEST(JitterBuffer, SimulatedChannelWithJitterAndLoss) {
  // 100 packets, 20 ms apart. Each packet gets a deterministic pseudo-random network delay of 0..59 ms; 5 are dropped.
  const int n = 100;
  struct Arr { int tick; uint16_t seq; };
  std::vector<Arr> arrivals;
  uint32_t s = 7;
  for (int i = 0; i < n; ++i) {
    s = s * 1664525u + 1013904223u;
    const int delay_ms = static_cast<int>((s >> 16) % 60);
    if (i % 20 == 13) continue;  // lost in the network
    arrivals.push_back({(i * 20 + delay_ms) / 20, static_cast<uint16_t>(i)});
  }
  // generous target: 4 frames = 80 ms covers the 60 ms spread
  JitterBuffer jb(4, 32);
  int present = 0, slots = 0;
  for (int tick = 0; tick < 4 * n && slots < n; ++tick) {  // stop after exactly n playout slots (the talk-spurt is over)
    for (const auto& a : arrivals)
      if (a.tick == tick) jb.push(a.seq, frame(static_cast<uint8_t>(a.seq)));
    auto f = jb.pop();
    if (f.present) ++present;
    if (f.present || f.concealed) ++slots;
  }
  EXPECT_EQ(jb.late(), 0u);                 // a deep enough buffer: every surviving packet made its slot
  EXPECT_EQ(present, n - 5);                // exactly the 5 network losses are missing
  EXPECT_EQ(jb.concealed(), 5u);
}
TEST(JitterBuffer, TargetSizing) {
  EXPECT_EQ(choose_target_frames(0.0), 2);
  EXPECT_EQ(choose_target_frames(5.0), 2);   // ceil(15/20) = 1 -> clamped to the 2-frame floor
  EXPECT_EQ(choose_target_frames(15.0), 3);  // ceil(45/20)
  EXPECT_EQ(choose_target_frames(40.0), 6);  // ceil(120/20)
  EXPECT_EQ(choose_target_frames(10.0, 20.0, 2.0), 2);
  EXPECT_EQ(choose_target_frames(30.0, 20.0, 2.0, 1), 3);
}

// ---------------------------------------------------------------- packet sizing (W 3.6)
TEST(Sizing, VoicePacketNumbers) {
  EXPECT_EQ(rtp_ts_increment(20.0, 8000), 160u);
  EXPECT_EQ(rtp_ts_increment(10.0, 8000), 80u);
  EXPECT_EQ(rtp_ts_increment(20.0, 16000), 320u);
  EXPECT_NEAR(stream_bitrate_bps(160, 20.0, 0), 80000.0, 1e-9);     // IP level: 200 B per 20 ms
  EXPECT_NEAR(stream_bitrate_bps(160, 20.0, 38), 95200.0, 1e-9);    // + Ethernet 14+4+8+12 = 38 B on the wire
  EXPECT_NEAR(stream_bitrate_bps(80, 10.0, 0), 96000.0, 1e-9);      // halving ptime: SAME payload rate, 20% more total
  EXPECT_NEAR(stream_bitrate_bps(320, 40.0, 0), 72000.0, 1e-9);     // doubling ptime: 40 B of headers amortised over 2x
}

// ---------------------------------------------------------------- multicast (W 3.7)
TEST(Multicast, AddressClassAndMac) {
  EXPECT_TRUE(is_ipv4_multicast(ipv4(224, 0, 0, 1)));
  EXPECT_TRUE(is_ipv4_multicast(ipv4(239, 255, 255, 255)));
  EXPECT_FALSE(is_ipv4_multicast(ipv4(223, 255, 255, 255)));
  EXPECT_FALSE(is_ipv4_multicast(ipv4(240, 0, 0, 0)));
  EXPECT_FALSE(is_ipv4_multicast(ipv4(192, 168, 1, 1)));
  EXPECT_EQ(multicast_mac(ipv4(224, 0, 0, 251)), (std::array<uint8_t, 6>{0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB}));
  EXPECT_EQ(multicast_mac(ipv4(239, 192, 0, 1)), (std::array<uint8_t, 6>{0x01, 0x00, 0x5E, 0x40, 0x00, 0x01}));
}
TEST(Multicast, ThirtyTwoGroupsShareOneMac) {
  // 239.1.1.1 and 239.129.1.1 differ only in a bit that the MAC mapping discards (the top bit of the 2nd octet)
  EXPECT_EQ(multicast_mac(ipv4(239, 1, 1, 1)), multicast_mac(ipv4(239, 129, 1, 1)));
  EXPECT_NE(multicast_mac(ipv4(239, 1, 1, 1)), multicast_mac(ipv4(239, 2, 1, 1)));
}
TEST(Multicast, TalkgroupToGroup) {
  EXPECT_EQ(ipv4_to_string(talkgroup_group_addr(1)), "239.192.0.1");
  EXPECT_EQ(ipv4_to_string(talkgroup_group_addr(0x1234)), "239.192.18.52");
  EXPECT_TRUE(is_ipv4_multicast(talkgroup_group_addr(65535)));
  EXPECT_NE(talkgroup_group_addr(1), talkgroup_group_addr(2));
  EXPECT_EQ(ipv4_to_string(ipv4(10, 0, 255, 7)), "10.0.255.7");
}
