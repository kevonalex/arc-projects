// Tests for Pack 03. Same file is compiled against the stubs and the solutions.
// All packets below are synthetic; none came from a live system.
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "pktheaders.h"

using namespace pkt;

// ---------------------------------------------------------------- BitReader (W 3.2)
TEST(BitReader, NacDuidFromTwoBytes) {
  const uint8_t d[2] = {0x29, 0x35};
  BitReader r(d, 2);
  uint32_t nac = 0, duid = 0, extra = 0;
  EXPECT_TRUE(r.read(12, nac));
  EXPECT_TRUE(r.read(4, duid));
  EXPECT_EQ(nac, 0x293u);
  EXPECT_EQ(duid, 0x5u);
  EXPECT_EQ(r.bits_remaining(), 0u);
  EXPECT_FALSE(r.read(1, extra));  // nothing left
}
TEST(BitReader, FirstBitIsMostSignificant) {
  const uint8_t d[1] = {0xA3};  // 1010 0011
  BitReader r(d, 1);
  uint32_t a = 0, b = 0;
  EXPECT_TRUE(r.read(3, a));
  EXPECT_TRUE(r.read(5, b));
  EXPECT_EQ(a, 5u);  // 101
  EXPECT_EQ(b, 3u);  // 00011
}
TEST(BitReader, CrossesByteBoundaries) {
  const uint8_t d[3] = {0xAB, 0xCD, 0xEF};
  BitReader r(d, 3);
  uint32_t v = 0;
  EXPECT_TRUE(r.skip(4));
  EXPECT_TRUE(r.read(12, v));
  EXPECT_EQ(v, 0xBCDu);
  EXPECT_EQ(r.bit_position(), 16u);
  EXPECT_TRUE(r.read(8, v));
  EXPECT_EQ(v, 0xEFu);
}
TEST(BitReader, Reads32Bits) {
  const uint8_t d[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  BitReader r(d, 4);
  uint32_t v = 0;
  EXPECT_TRUE(r.read(32, v));
  EXPECT_EQ(v, 0xDEADBEEFu);
}
TEST(BitReader, ZeroBitReadSucceedsWithZero) {
  const uint8_t d[1] = {0xFF};
  BitReader r(d, 1);
  uint32_t v = 123;
  EXPECT_TRUE(r.read(0, v));
  EXPECT_EQ(v, 0u);
  EXPECT_EQ(r.bit_position(), 0u);
}
TEST(BitReader, FailedReadConsumesNothing) {
  const uint8_t d[2] = {0x12, 0x34};
  BitReader r(d, 2);
  uint32_t v = 0;
  EXPECT_FALSE(r.read(17, v));  // more than available
  EXPECT_FALSE(r.read(33, v));  // more than 32
  EXPECT_EQ(r.bit_position(), 0u);
  EXPECT_TRUE(r.read(16, v));
  EXPECT_EQ(v, 0x1234u);
}
TEST(BitReader, AlignToByte) {
  const uint8_t d[2] = {0xFF, 0x0F};
  BitReader r(d, 2);
  uint32_t v = 0;
  EXPECT_TRUE(r.align_to_byte());  // already aligned: no-op
  EXPECT_EQ(r.bit_position(), 0u);
  EXPECT_TRUE(r.read(3, v));
  EXPECT_TRUE(r.align_to_byte());
  EXPECT_EQ(r.bit_position(), 8u);
  EXPECT_TRUE(r.read(8, v));
  EXPECT_EQ(v, 0x0Fu);
}
TEST(BitReader, EmptyBuffer) {
  BitReader r(nullptr, 0);
  uint32_t v = 0;
  EXPECT_FALSE(r.read(1, v));
  EXPECT_EQ(r.bits_remaining(), 0u);
}

// ---------------------------------------------------------------- BitWriter (W 3.2)
TEST(BitWriter, ThreePlusFiveBits) {
  BitWriter w;
  w.write(5, 3);
  w.write(3, 5);
  ASSERT_EQ(w.bytes().size(), 1u);
  EXPECT_EQ(w.bytes()[0], 0xA3);
  EXPECT_EQ(w.bit_count(), 8u);
}
TEST(BitWriter, NacDuidIntoTwoBytes) {
  BitWriter w;
  w.write(0x293, 12);
  w.write(0x5, 4);
  ASSERT_EQ(w.bytes().size(), 2u);
  EXPECT_EQ(w.bytes()[0], 0x29);
  EXPECT_EQ(w.bytes()[1], 0x35);
}
TEST(BitWriter, PartialByteIsZeroPadded) {
  BitWriter w;
  w.write(1, 1);
  ASSERT_EQ(w.bytes().size(), 1u);
  EXPECT_EQ(w.bytes()[0], 0x80);
  EXPECT_EQ(w.bit_count(), 1u);
}
TEST(BitWriter, OversizedValueIsClippedToWidth) {
  BitWriter w;
  w.write(0xFF, 4);  // only low 4 bits
  w.write(0x0, 4);
  ASSERT_EQ(w.bytes().size(), 1u);
  EXPECT_EQ(w.bytes()[0], 0xF0);
}
TEST(BitWriter, ThirtyTwoBits) {
  BitWriter w;
  w.write(0xDEADBEEFu, 32);
  const std::vector<uint8_t> expect = {0xDE, 0xAD, 0xBE, 0xEF};
  EXPECT_EQ(w.bytes(), expect);
}
TEST(BitWriter, RoundTripWithReaderOnMixedWidths) {
  const unsigned widths[] = {1, 3, 7, 12, 16, 5, 32, 2, 9, 6};
  uint32_t vals[10];
  uint32_t s = 99;
  BitWriter w;
  for (int i = 0; i < 10; ++i) {
    s = s * 1664525u + 1013904223u;
    vals[i] = widths[i] == 32 ? s : (s & ((1u << widths[i]) - 1u));
    w.write(vals[i], widths[i]);
  }
  BitReader r(w.bytes().data(), w.bytes().size());
  for (int i = 0; i < 10; ++i) {
    uint32_t got = 0;
    ASSERT_TRUE(r.read(widths[i], got));
    EXPECT_EQ(got, vals[i]) << "field " << i;
  }
}

// ---------------------------------------------------------------- RTP (W 3.4)
static const uint8_t kRtp[12] = {0x80, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x12, 0x34, 0x56, 0x78};

TEST(Rtp, ParseFixedHeader) {
  RtpHeader h;
  ASSERT_TRUE(parse_rtp(kRtp, sizeof kRtp, h));
  EXPECT_EQ(h.version, 2);
  EXPECT_FALSE(h.padding);
  EXPECT_FALSE(h.extension);
  EXPECT_EQ(h.csrc_count, 0);
  EXPECT_TRUE(h.marker);
  EXPECT_EQ(h.payload_type, 0);
  EXPECT_EQ(h.seq, 1);
  EXPECT_EQ(h.timestamp, 160u);
  EXPECT_EQ(h.ssrc, 0x12345678u);
  EXPECT_EQ(h.header_len, 12u);
  EXPECT_EQ(h.payload_len, 0u);
}
TEST(Rtp, PayloadLength) {
  std::vector<uint8_t> v(kRtp, kRtp + 12);
  v.insert(v.end(), {0xDE, 0xAD, 0xBE, 0xEF});
  RtpHeader h;
  ASSERT_TRUE(parse_rtp(v.data(), v.size(), h));
  EXPECT_EQ(h.payload_len, 4u);
}
TEST(Rtp, MarkerAndPayloadTypeAreSeparateFields) {
  uint8_t b[12];
  std::copy(kRtp, kRtp + 12, b);
  b[1] = 0x08;  // M=0, PT=8 (PCMA)
  RtpHeader h;
  ASSERT_TRUE(parse_rtp(b, 12, h));
  EXPECT_FALSE(h.marker);
  EXPECT_EQ(h.payload_type, 8);
  b[1] = 0xE0;  // M=1, PT=0x60=96 (dynamic)
  ASSERT_TRUE(parse_rtp(b, 12, h));
  EXPECT_TRUE(h.marker);
  EXPECT_EQ(h.payload_type, 0x60);
}
TEST(Rtp, RejectsTooShort) {
  RtpHeader h;
  EXPECT_FALSE(parse_rtp(kRtp, 11, h));
  EXPECT_FALSE(parse_rtp(kRtp, 0, h));
  EXPECT_FALSE(parse_rtp(nullptr, 12, h));
}
TEST(Rtp, RejectsWrongVersion) {
  uint8_t b[12];
  std::copy(kRtp, kRtp + 12, b);
  b[0] = 0x40;  // version 1
  RtpHeader h;
  EXPECT_FALSE(parse_rtp(b, 12, h));
  b[0] = 0xC0;  // version 3
  EXPECT_FALSE(parse_rtp(b, 12, h));
}
TEST(Rtp, CsrcListLengthIsCheckedAgainstBuffer) {
  uint8_t b[12];
  std::copy(kRtp, kRtp + 12, b);
  b[0] = 0x82;  // CC = 2 -> needs 8 more bytes
  RtpHeader h;
  EXPECT_FALSE(parse_rtp(b, 12, h));
  std::vector<uint8_t> v(b, b + 12);
  v.resize(20 + 3, 0x55);  // 20-byte header + 3 payload bytes
  ASSERT_TRUE(parse_rtp(v.data(), v.size(), h));
  EXPECT_EQ(h.csrc_count, 2);
  EXPECT_EQ(h.header_len, 20u);
  EXPECT_EQ(h.payload_len, 3u);
}
TEST(Rtp, ExtensionHeader) {
  std::vector<uint8_t> v(kRtp, kRtp + 12);
  v[0] = 0x90;  // V=2 X=1
  v.insert(v.end(), {0xBE, 0xDE, 0x00, 0x01, 0xAA, 0xBB, 0xCC, 0xDD});  // profile, length=1 word, data
  v.insert(v.end(), {0x01, 0x02});                                      // payload
  RtpHeader h;
  ASSERT_TRUE(parse_rtp(v.data(), v.size(), h));
  EXPECT_TRUE(h.extension);
  EXPECT_EQ(h.header_len, 20u);
  EXPECT_EQ(h.payload_len, 2u);
}
TEST(Rtp, ExtensionLengthLyingIsRejected) {
  std::vector<uint8_t> v(kRtp, kRtp + 12);
  v[0] = 0x90;
  v.insert(v.end(), {0xBE, 0xDE, 0xFF, 0xFF});  // claims 65535 words follow; none do
  RtpHeader h;
  EXPECT_FALSE(parse_rtp(v.data(), v.size(), h));
  std::vector<uint8_t> w(kRtp, kRtp + 12);
  w[0] = 0x90;
  w.insert(w.end(), {0xBE, 0xDE});  // truncated before the length field even ends
  EXPECT_FALSE(parse_rtp(w.data(), w.size(), h));
}
TEST(Rtp, PaddingIsRemovedFromPayload) {
  std::vector<uint8_t> v(kRtp, kRtp + 12);
  v[0] = 0xA0;  // V=2 P=1
  v.insert(v.end(), {1, 2, 3});          // payload
  v.insert(v.end(), {0, 0, 0, 4});       // 4 padding octets, last one is the count
  RtpHeader h;
  ASSERT_TRUE(parse_rtp(v.data(), v.size(), h));
  EXPECT_TRUE(h.padding);
  EXPECT_EQ(h.payload_len, 3u);
}
TEST(Rtp, BadPaddingCountRejected) {
  std::vector<uint8_t> v(kRtp, kRtp + 12);
  v[0] = 0xA0;
  v.insert(v.end(), {1, 2, 3, 0});  // count 0 is invalid
  RtpHeader h;
  EXPECT_FALSE(parse_rtp(v.data(), v.size(), h));
  v.back() = 200;  // larger than the bytes available
  EXPECT_FALSE(parse_rtp(v.data(), v.size(), h));
  std::vector<uint8_t> hdr_only(kRtp, kRtp + 12);
  hdr_only[0] = 0xA0;  // padding flag set, but the packet is only the 12-byte header
  hdr_only[11] = 0x01;  // "last octet" = 1 padding byte, yet no bytes exist beyond the header
  EXPECT_FALSE(parse_rtp(hdr_only.data(), hdr_only.size(), h));  // header_len + pad (13) > len (12)
}
TEST(Rtp, BuildRoundTrip) {
  RtpHeader h;
  ASSERT_TRUE(parse_rtp(kRtp, sizeof kRtp, h));
  const std::vector<uint8_t> out = build_rtp(h);
  const std::vector<uint8_t> expect(kRtp, kRtp + 12);
  EXPECT_EQ(out, expect);
}
TEST(Rtp, BuildFromScratch) {
  RtpHeader h;
  h.version = 2;
  h.marker = false;
  h.payload_type = 8;
  h.seq = 0xFFFF;
  h.timestamp = 0x00000140;
  h.ssrc = 0xCAFEBABE;
  const std::vector<uint8_t> expect = {0x80, 0x08, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x40,
                                       0xCA, 0xFE, 0xBA, 0xBE};
  EXPECT_EQ(build_rtp(h), expect);
}

// ---------------------------------------------------------------- IPv4 (W 3.5)
static std::vector<uint8_t> wiki_ipv4_packet() {
  // 4500 0073 0000 4000 4011 b861 c0a8 0001 c0a8 00c7  (total length 0x73 = 115)
  std::vector<uint8_t> v = {0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                            0xb8, 0x61, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7};
  v.resize(115, 0);
  return v;
}
TEST(Ipv4, ParseKnownHeader) {
  const auto v = wiki_ipv4_packet();
  Ipv4Header h;
  ASSERT_TRUE(parse_ipv4(v.data(), v.size(), h));
  EXPECT_EQ(h.version, 4);
  EXPECT_EQ(h.ihl, 5);
  EXPECT_EQ(h.header_len, 20u);
  EXPECT_EQ(h.dscp, 0);
  EXPECT_EQ(h.ecn, 0);
  EXPECT_EQ(h.total_length, 115);
  EXPECT_EQ(h.identification, 0);
  EXPECT_TRUE(h.flag_df);
  EXPECT_FALSE(h.flag_mf);
  EXPECT_EQ(h.frag_offset, 0);
  EXPECT_EQ(h.ttl, 64);
  EXPECT_EQ(h.protocol, 17);
  EXPECT_EQ(h.checksum, 0xB861);
  EXPECT_EQ(h.src, 0xC0A80001u);  // 192.168.0.1
  EXPECT_EQ(h.dst, 0xC0A800C7u);  // 192.168.0.199
}
TEST(Ipv4, FragmentFieldsMfAndOffset) {
  const std::vector<uint8_t> v = {0x45, 0x00, 0x00, 0x14, 0x12, 0x34, 0x20, 0xB9, 0x40, 0x11,
                                  0x00, 0x00, 0x7F, 0x00, 0x00, 0x01, 0x7F, 0x00, 0x00, 0x01};
  Ipv4Header h;
  ASSERT_TRUE(parse_ipv4(v.data(), v.size(), h));
  EXPECT_EQ(h.identification, 0x1234);
  EXPECT_TRUE(h.flag_mf);
  EXPECT_FALSE(h.flag_df);
  EXPECT_EQ(h.frag_offset, 0xB9);
}
TEST(Ipv4, DscpEcnSplit) {
  std::vector<uint8_t> v = {0x45, 0xB9, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11,
                            0x00, 0x00, 0x0A, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x02};
  Ipv4Header h;
  ASSERT_TRUE(parse_ipv4(v.data(), v.size(), h));
  EXPECT_EQ(h.dscp, 0x2E);  // 0xB9 = 1011 1001 -> DSCP 101110 (46, EF), ECN 01
  EXPECT_EQ(h.ecn, 1);
}
TEST(Ipv4, OptionsExtendHeader) {
  const std::vector<uint8_t> v = {0x46, 0x00, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
                                  0x0A, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x02, 0x01, 0x01, 0x01, 0x00};
  Ipv4Header h;
  ASSERT_TRUE(parse_ipv4(v.data(), v.size(), h));
  EXPECT_EQ(h.ihl, 6);
  EXPECT_EQ(h.header_len, 24u);
  EXPECT_EQ(h.protocol, 1);
}
TEST(Ipv4, Rejections) {
  const auto good = wiki_ipv4_packet();
  Ipv4Header h;
  EXPECT_FALSE(parse_ipv4(good.data(), 19, h));  // truncated header
  auto bad = good;
  bad[0] = 0x65;  // version 6
  EXPECT_FALSE(parse_ipv4(bad.data(), bad.size(), h));
  bad = good;
  bad[0] = 0x44;  // IHL 4 (< 5)
  EXPECT_FALSE(parse_ipv4(bad.data(), bad.size(), h));
  bad = good;
  bad[0] = 0x4F;  // IHL 15 -> 60-byte header, but total length 115 fine; make the buffer too short:
  EXPECT_FALSE(parse_ipv4(bad.data(), 40, h));  // total_length 115 > len 40 (truncated)
  bad = good;
  bad[2] = 0x00;
  bad[3] = 0x10;  // total_length 16 < header 20
  EXPECT_FALSE(parse_ipv4(bad.data(), bad.size(), h));
  EXPECT_FALSE(parse_ipv4(good.data(), 100, h));  // total_length 115 > len 100
}
TEST(Ipv4, TrailingPaddingAllowed) {
  auto v = wiki_ipv4_packet();
  v.resize(130, 0xEE);  // e.g. Ethernet padding after the IP datagram
  Ipv4Header h;
  EXPECT_TRUE(parse_ipv4(v.data(), v.size(), h));
}

// ---------------------------------------------------------------- UDP (W 3.6)
static const uint8_t kUdp[8] = {0x9C, 0x40, 0x13, 0x8C, 0x00, 0x14, 0x00, 0x00};
TEST(Udp, ParseKnownHeader) {
  std::vector<uint8_t> v(kUdp, kUdp + 8);
  v.resize(20, 0);
  UdpHeader h;
  ASSERT_TRUE(parse_udp(v.data(), v.size(), h));
  EXPECT_EQ(h.src_port, 40000);
  EXPECT_EQ(h.dst_port, 5004);
  EXPECT_EQ(h.length, 20);
  EXPECT_EQ(h.checksum, 0);
}
TEST(Udp, Rejections) {
  std::vector<uint8_t> v(kUdp, kUdp + 8);
  v.resize(20, 0);
  UdpHeader h;
  EXPECT_FALSE(parse_udp(v.data(), 7, h));   // shorter than a UDP header
  EXPECT_FALSE(parse_udp(v.data(), 19, h));  // length field says 20, only 19 present
  v[4] = 0x00;
  v[5] = 0x07;  // length 7 < 8
  EXPECT_FALSE(parse_udp(v.data(), v.size(), h));
}
TEST(Udp, BuildRoundTrip) {
  UdpHeader h{40000, 5004, 20, 0};
  const auto out = build_udp(h);
  const std::vector<uint8_t> expect(kUdp, kUdp + 8);
  EXPECT_EQ(out, expect);
}

// ---------------------------------------------------------------- chain (W 4)
static std::vector<uint8_t> chain_packet() {
  std::vector<uint8_t> v = {0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                            0x00, 0x00, 0x0A, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x02};
  v.insert(v.end(), kUdp, kUdp + 8);
  v.insert(v.end(), kRtp, kRtp + 12);
  return v;  // 40 bytes
}
TEST(Chain, ParsesAllThreeLayers) {
  const auto v = chain_packet();
  ASSERT_EQ(v.size(), 40u);
  Packet p;
  ASSERT_TRUE(parse_ip_udp_rtp(v.data(), v.size(), p));
  EXPECT_EQ(p.ip.protocol, 17);
  EXPECT_EQ(p.udp.dst_port, 5004);
  EXPECT_EQ(p.rtp.seq, 1);
  EXPECT_EQ(p.rtp.ssrc, 0x12345678u);
  EXPECT_EQ(p.rtp_offset, 28u);
  EXPECT_EQ(p.rtp.payload_len, 0u);
}
TEST(Chain, IgnoresTrailingEthernetPadding) {
  auto v = chain_packet();
  v.resize(46, 0);  // pad to the 46-byte Ethernet minimum payload
  Packet p;
  EXPECT_TRUE(parse_ip_udp_rtp(v.data(), v.size(), p));
  EXPECT_EQ(p.rtp.payload_len, 0u);  // padding is NOT RTP payload: IP total_length bounds it
}
TEST(Chain, RejectsTruncated) {
  const auto v = chain_packet();
  Packet p;
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), 39, p));
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), 27, p));
}
TEST(Chain, RejectsNonUdpAndFragments) {
  auto v = chain_packet();
  Packet p;
  v[9] = 6;  // TCP
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), v.size(), p));
  v = chain_packet();
  v[6] = 0x20;  // MF set
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), v.size(), p));
  v = chain_packet();
  v[7] = 0x01;  // fragment offset 1
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), v.size(), p));
}
TEST(Chain, RejectsUdpLengthLargerThanIpPayload) {
  auto v = chain_packet();
  v[24] = 0x00;
  v[25] = 0x30;  // UDP length 48 but only 20 bytes of IP payload
  Packet p;
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), v.size(), p));
}
TEST(Chain, RejectsNonRtpPayloadVersion) {
  auto v = chain_packet();
  v[28] = 0x40;  // RTP version 1
  Packet p;
  EXPECT_FALSE(parse_ip_udp_rtp(v.data(), v.size(), p));
}
TEST(Chain, DoesNotReadPastBufferOnRandomGarbage) {
  // ASan/UBSan will complain if any parser reads beyond `len`.
  uint32_t s = 12345;
  for (int iter = 0; iter < 2000; ++iter) {
    std::vector<uint8_t> v(static_cast<size_t>(iter % 64));
    for (auto& b : v) {
      s = s * 1664525u + 1013904223u;
      b = static_cast<uint8_t>(s >> 24);
    }
    Packet p;
    (void)parse_ip_udp_rtp(v.data(), v.size(), p);
    RtpHeader r;
    (void)parse_rtp(v.data(), v.size(), r);
  }
}
