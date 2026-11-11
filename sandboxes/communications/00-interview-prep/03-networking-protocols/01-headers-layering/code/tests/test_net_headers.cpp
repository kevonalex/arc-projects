#include <gtest/gtest.h>
#include "net_headers.hpp"

using namespace ether;

// Wikipedia "IPv4 header checksum" worked example (checksum field = b861). VERIFY against RFC 1071 section 3 style sums.
static const uint8_t kIpv4Hdr[20] = {0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                                     0xb8, 0x61, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7};

TEST(Endian, ReadWrite) {
    uint8_t b[4] = {0x12, 0x34, 0x56, 0x78};
    EXPECT_EQ(rd16(b), 0x1234);
    EXPECT_EQ(rd32(b), 0x12345678u);
    uint8_t o[4] = {};
    wr32(o, 0xDEADBEEF);
    EXPECT_EQ(o[0], 0xDE); EXPECT_EQ(o[3], 0xEF);
    wr16(o, 0xCAFE);
    EXPECT_EQ(o[0], 0xCA); EXPECT_EQ(o[1], 0xFE);
}

TEST(Checksum, Ipv4KnownVector) {
    EXPECT_EQ(internet_checksum(kIpv4Hdr, 20), 0);          // valid header sums to zero
    uint8_t copy[20]; for (int i = 0; i < 20; ++i) copy[i] = kIpv4Hdr[i];
    copy[10] = copy[11] = 0;
    EXPECT_EQ(internet_checksum(copy, 20), 0xb861);
}
TEST(Checksum, OddLengthPadsLowByte) {
    const uint8_t d[3] = {0x01, 0x02, 0x03};   // words 0x0102 + 0x0300 = 0x0402 -> ~ = 0xFBFD
    EXPECT_EQ(internet_checksum(d, 3), 0xFBFD);
}
TEST(Checksum, CarryFolding) {
    const uint8_t d[4] = {0xFF, 0xFF, 0x00, 0x01};  // 0xFFFF + 0x0001 = 0x10000 -> fold -> 0x0001 -> ~ = 0xFFFE
    EXPECT_EQ(internet_checksum(d, 4), 0xFFFE);
}

TEST(Ethernet, PlainAndVlan) {
    uint8_t f[18] = {1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15, 0x08, 0x00, 0, 0, 0, 0};
    auto h = parse_ethernet(f, 14);
    ASSERT_TRUE(h);
    EXPECT_EQ(h->ethertype, 0x0800); EXPECT_FALSE(h->has_vlan); EXPECT_EQ(h->header_len, 14u);
    EXPECT_EQ(h->src[0], 10);
    uint8_t v[18] = {1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15, 0x81, 0x00, 0xA0, 0x64, 0x86, 0xDD};
    auto hv = parse_ethernet(v, 18);
    ASSERT_TRUE(hv);
    EXPECT_TRUE(hv->has_vlan); EXPECT_EQ(hv->vlan_tci, 0xA064);   // PCP=5, VID=100
    EXPECT_EQ(hv->ethertype, 0x86DD); EXPECT_EQ(hv->header_len, 18u);
    EXPECT_FALSE(parse_ethernet(v, 17));
}

TEST(Ipv4, ParseKnownVector) {
    uint8_t pkt[0x73] = {};
    for (int i = 0; i < 20; ++i) pkt[i] = kIpv4Hdr[i];
    auto h = parse_ipv4(pkt, sizeof pkt);
    ASSERT_TRUE(h);
    EXPECT_EQ(h->ihl, 5); EXPECT_EQ(h->total_length, 0x73); EXPECT_TRUE(h->df); EXPECT_FALSE(h->mf);
    EXPECT_EQ(h->frag_offset, 0); EXPECT_EQ(h->ttl, 64); EXPECT_EQ(h->protocol, 17);
    EXPECT_EQ(h->src, 0xC0A80001u); EXPECT_EQ(h->dst, 0xC0A800C7u);
    EXPECT_TRUE(ipv4_checksum_ok(pkt, h->header_len()));
}
TEST(Ipv4, RejectsMalformed) {
    uint8_t pkt[0x73] = {};
    for (int i = 0; i < 20; ++i) pkt[i] = kIpv4Hdr[i];
    EXPECT_FALSE(parse_ipv4(pkt, 19));                 // too short
    EXPECT_FALSE(parse_ipv4(pkt, 50));                 // total_length (0x73) larger than buffer
    pkt[0] = 0x65; EXPECT_FALSE(parse_ipv4(pkt, sizeof pkt));   // version 6
    pkt[0] = 0x44; EXPECT_FALSE(parse_ipv4(pkt, sizeof pkt));   // ihl 4 < 5
    pkt[0] = 0x4F; EXPECT_FALSE(parse_ipv4(pkt, 40));           // ihl 15 -> 60 bytes > buffer
}
TEST(Ipv4, BuildRoundTrip) {
    Ipv4Header h;
    h.dscp = 46; h.total_length = 200; h.id = 0x1234; h.df = true; h.ttl = 64; h.protocol = 17;
    h.src = 0x0A000001; h.dst = 0xEF010101;     // 10.0.0.1 -> 239.1.1.1
    Bytes b = build_ipv4(h);
    ASSERT_EQ(b.size(), 20u);
    EXPECT_EQ(b[1], 0xB8);                       // DSCP 46 << 2 = 0xB8
    EXPECT_TRUE(ipv4_checksum_ok(b.data(), 20));
    b.resize(200);
    auto p = parse_ipv4(b.data(), b.size());
    ASSERT_TRUE(p);
    EXPECT_EQ(p->dscp, 46); EXPECT_EQ(p->id, 0x1234); EXPECT_TRUE(p->df); EXPECT_EQ(p->dst, 0xEF010101u);
}
TEST(Ipv4, WorksheetPartAPacket) {   // the RTP-over-multicast packet dissected in worksheet Part A
    Ipv4Header h;
    h.dscp = 46; h.total_length = 200; h.id = 0x1234; h.df = true; h.ttl = 64; h.protocol = 17;
    h.src = 0x0A000001; h.dst = 0xEF010101;
    Bytes b = build_ipv4(h);
    ASSERT_EQ(b.size(), 20u);
    EXPECT_EQ(rd16(&b[10]), 0x2D36);
}
TEST(Ipv4, FragmentFlagsAndOffset) {
    Ipv4Header h; h.total_length = 20; h.mf = true; h.frag_offset = 185;   // 185*8 = 1480
    Bytes b = build_ipv4(h);
    ASSERT_EQ(b.size(), 20u);
    EXPECT_EQ(b[6], 0x20); EXPECT_EQ(b[7], 185);
    auto p = parse_ipv4(b.data(), b.size());
    ASSERT_TRUE(p);
    EXPECT_TRUE(p->mf); EXPECT_FALSE(p->df); EXPECT_EQ(p->frag_offset, 185);
}

TEST(Ipv6, Parse) {
    uint8_t p[40] = {0x6B, 0xB1, 0x23, 0x45, 0x00, 0x00, 17, 32};   // TC=0xBB? see below
    // first word 0x6BB12345: version 6, TC = 0xBB, flow label = 0x12345
    p[0] = 0x6B; p[1] = 0xB1;
    p[8] = 0x20; p[9] = 0x01; p[23] = 1; p[24] = 0xFF; p[25] = 0x02; p[39] = 0x01;
    auto h = parse_ipv6(p, 40);
    ASSERT_TRUE(h);
    EXPECT_EQ(h->traffic_class, 0xBB); EXPECT_EQ(h->flow_label, 0x12345u);
    EXPECT_EQ(h->next_header, 17); EXPECT_EQ(h->hop_limit, 32);
    EXPECT_EQ(h->src[0], 0x20); EXPECT_EQ(h->dst[0], 0xFF); EXPECT_EQ(h->dst[15], 1);
    p[5] = 8;                                    // payload_length 8 but buffer is only 40
    EXPECT_FALSE(parse_ipv6(p, 40));
    p[0] = 0x4B;
    EXPECT_FALSE(parse_ipv6(p, 48));
}

TEST(Udp, ParseAndChecksum) {
    uint8_t seg[11] = {0x13, 0x8c, 0x13, 0x8e, 0, 11, 0, 0, 0x41, 0x42, 0x43};
    auto h = parse_udp(seg, sizeof seg);
    ASSERT_TRUE(h);
    EXPECT_EQ(h->src_port, 5004); EXPECT_EQ(h->dst_port, 5006); EXPECT_EQ(h->length, 11);
    // independent reference value computed with a throw-away Python script (see worksheet)
    EXPECT_EQ(udp_checksum_v4(0xC0A80001, 0xC0A80002, seg, sizeof seg), 0xd327);
    seg[4] = 0; seg[5] = 7;                       // length < 8 + payload is fine, but < 8 is not
    seg[5] = 4; EXPECT_FALSE(parse_udp(seg, sizeof seg));
    seg[5] = 200; EXPECT_FALSE(parse_udp(seg, sizeof seg));
}

TEST(Tcp, SynAndDataOffset) {
    uint8_t s[24] = {0x1F, 0x90, 0x00, 0x50, 0, 0, 0x03, 0xE8, 0, 0, 0, 0, 0x60, 0x02, 0xFF, 0xFF, 0, 0, 0, 0,
                     0x02, 0x04, 0x05, 0xB4};   // SYN with MSS=1460 option
    auto h = parse_tcp(s, sizeof s);
    ASSERT_TRUE(h);
    EXPECT_EQ(h->src_port, 8080); EXPECT_EQ(h->dst_port, 80); EXPECT_EQ(h->seq, 1000u);
    EXPECT_EQ(h->data_offset, 6); EXPECT_EQ(h->header_len(), 24u);
    EXPECT_TRUE(h->flags.syn); EXPECT_FALSE(h->flags.ack); EXPECT_FALSE(h->flags.fin);
    EXPECT_EQ(h->window, 65535);
    EXPECT_FALSE(parse_tcp(s, 23));               // options claim 24 bytes
    s[13] = 0x12; h = parse_tcp(s, sizeof s);     // SYN+ACK
    EXPECT_TRUE(h->flags.syn && h->flags.ack);
    s[12] = 0x40; EXPECT_FALSE(parse_tcp(s, sizeof s));   // data offset 4 < 5
}

TEST(Sctp, CommonAndChunks) {
    uint8_t p[16] = {0x0B, 0x59, 0x0B, 0x59, 0xAA, 0xBB, 0xCC, 0xDD, 1, 2, 3, 4, 0, 0x03, 0, 0x07};
    auto c = parse_sctp_common(p, 16);
    ASSERT_TRUE(c);
    EXPECT_EQ(c->src_port, 2905); EXPECT_EQ(c->verification_tag, 0xAABBCCDDu); EXPECT_EQ(c->checksum, 0x01020304u);
    uint8_t ch[8] = {0, 0x03, 0, 7, 1, 2, 3, 0};   // DATA-like chunk of length 7 -> 1 pad byte
    auto k = parse_sctp_chunk(ch, 8);
    ASSERT_TRUE(k);
    EXPECT_EQ(k->type, 0); EXPECT_EQ(k->flags, 3); EXPECT_EQ(k->length, 7);
    EXPECT_EQ(sctp_padded_len(7), 8u); EXPECT_EQ(sctp_padded_len(8), 8u); EXPECT_EQ(sctp_padded_len(4), 4u);
    ch[3] = 3; EXPECT_FALSE(parse_sctp_chunk(ch, 8));
    ch[3] = 9; EXPECT_FALSE(parse_sctp_chunk(ch, 8));
}
