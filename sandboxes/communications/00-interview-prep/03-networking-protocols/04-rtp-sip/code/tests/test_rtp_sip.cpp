#include <gtest/gtest.h>
#include <string>
#include "rtp_sip.hpp"

using namespace ether;

// ============================================================ RTP header
// The exact header from pack 01 Part A: V=2 PT=0 seq=1 ts=160 ssrc=0x11223344
static const uint8_t kRtp[12] = {0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x11, 0x22, 0x33, 0x44};

TEST(Rtp, ParseMinimal) {
    Bytes pkt(kRtp, kRtp + 12);
    pkt.insert(pkt.end(), 160, 0xFF);
    auto r = parse_rtp(pkt.data(), pkt.size());
    ASSERT_TRUE(r);
    EXPECT_EQ(r->h.version, 2); EXPECT_FALSE(r->h.padding); EXPECT_FALSE(r->h.extension); EXPECT_FALSE(r->h.marker);
    EXPECT_EQ(r->h.cc, 0); EXPECT_EQ(r->h.pt, 0); EXPECT_EQ(r->h.seq, 1);
    EXPECT_EQ(r->h.timestamp, 160u); EXPECT_EQ(r->h.ssrc, 0x11223344u);
    EXPECT_EQ(r->payload_off, 12u); EXPECT_EQ(r->payload_len, 160u);
}
TEST(Rtp, MarkerAndDynamicPayloadType) {
    uint8_t b[12] = {0x80, 0x80 | 96, 0, 5, 0, 0, 0, 0, 0, 0, 0, 1};   // M=1, PT=96
    auto r = parse_rtp(b, 12);
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->h.marker); EXPECT_EQ(r->h.pt, 96);
    EXPECT_EQ(r->payload_len, 0u);
}
TEST(Rtp, CsrcExtensionAndPadding) {
    RtpHeader h; h.pt = 8; h.seq = 0xFFFF; h.timestamp = 0xFFFFFF00u; h.ssrc = 7;
    h.csrc = {0xAAAAAAAA, 0xBBBBBBBB};
    h.extension = true; h.ext_profile = 0xBEDE; h.ext_data = {1, 2, 3, 4, 5, 6, 7, 8};
    Bytes payload = {9, 9, 9};
    Bytes pkt = build_rtp(h, payload.data(), payload.size());
    ASSERT_EQ(pkt.size(), 12u + 8 + 4 + 8 + 3);
    EXPECT_EQ(pkt[0], 0x80 | 0x10 | 2);                 // V=2, X=1, CC=2
    auto r = parse_rtp(pkt.data(), pkt.size());
    ASSERT_TRUE(r);
    EXPECT_EQ(r->h.csrc.size(), 2u); EXPECT_EQ(r->h.csrc[1], 0xBBBBBBBBu);
    EXPECT_EQ(r->h.ext_profile, 0xBEDE); EXPECT_EQ(r->h.ext_data.size(), 8u);
    EXPECT_EQ(r->payload_off, 32u); EXPECT_EQ(r->payload_len, 3u);
    // padding: 4 pad bytes, last byte = 4
    pkt[0] |= 0x20; pkt.insert(pkt.end(), {0, 0, 0, 4});
    r = parse_rtp(pkt.data(), pkt.size());
    ASSERT_TRUE(r);
    EXPECT_EQ(r->payload_len, 3u);                      // padding excluded
}
TEST(Rtp, RejectsMalformed) {
    uint8_t b[16] = {0x80, 0, 0, 1, 0, 0, 0, 0xA0, 0, 0, 0, 1, 0, 0, 0, 0};
    EXPECT_FALSE(parse_rtp(b, 11));                     // short
    b[0] = 0x40; EXPECT_FALSE(parse_rtp(b, 16));        // version 1
    b[0] = 0x8F; EXPECT_FALSE(parse_rtp(b, 16));        // CC=15 needs 60 bytes of CSRC
    b[0] = 0x90; EXPECT_FALSE(parse_rtp(b, 14));        // extension header truncated
    b[0] = 0x90; b[14] = 0; b[15] = 5; EXPECT_FALSE(parse_rtp(b, 16));   // ext length 5 words > buffer
    b[0] = 0xA0; b[15] = 0; EXPECT_FALSE(parse_rtp(b, 16));              // padding count 0 is illegal
    b[15] = 200; EXPECT_FALSE(parse_rtp(b, 16));                         // padding larger than packet
}
TEST(Rtp, SerialArithmetic) {
    EXPECT_TRUE(seq_newer(2, 1));
    EXPECT_TRUE(seq_newer(0, 65535));                   // wrap: 0 comes after 65535
    EXPECT_FALSE(seq_newer(65535, 0));
    EXPECT_FALSE(seq_newer(7, 7));
    EXPECT_EQ(ts_diff(100, 40), 60);
    EXPECT_EQ(ts_diff(10, 0xFFFFFFF0u), 26);            // 32-bit timestamp wrap
    EXPECT_EQ(ts_diff(0xFFFFFFF0u, 10), -26);
}
TEST(Rtp, SequenceTrackerWrapAndLoss) {
    SeqTracker t;
    for (uint16_t s : {65534, 65535, 0, 1}) EXPECT_TRUE(t.update(s));
    EXPECT_EQ(t.extended_max(), 65537u);
    EXPECT_EQ(t.expected(), 4u); EXPECT_EQ(t.received(), 4u); EXPECT_EQ(t.lost(), 0);
    EXPECT_TRUE(t.update(3));                           // 2 never arrived
    EXPECT_EQ(t.expected(), 6u); EXPECT_EQ(t.lost(), 1);
    EXPECT_TRUE(t.update(2));                           // late arrival repairs the loss count
    EXPECT_EQ(t.lost(), 0);
    EXPECT_EQ(t.extended_max(), 65539u);                // max unchanged by a late packet
}
TEST(Rtp, SequenceTrackerRejectsWildJump) {
    SeqTracker t;
    t.update(100); t.update(101);
    EXPECT_FALSE(t.update(40000));                      // not a gap of < 3000: stream restart / foreign source
    EXPECT_EQ(t.extended_max(), 101u);
}
TEST(Rtp, JitterEstimator) {
    JitterEstimator j;
    // sent every 160 units (20 ms @ 8 kHz); the 3rd packet arrives 80 units (10 ms) late
    j.update(0, 0); j.update(160, 160); EXPECT_DOUBLE_EQ(j.jitter_units(), 0.0);
    j.update(400, 320);                                 // D = 80 -> J = 80/16 = 5
    EXPECT_DOUBLE_EQ(j.jitter_units(), 5.0);
    j.update(480, 480);                                 // D = 80 again -> J = 5 + (80-5)/16
    EXPECT_NEAR(j.jitter_units(), 9.6875, 1e-12);
    EXPECT_NEAR(j.jitter_ms(8000), 9.6875 / 8.0, 1e-12);
}
TEST(Rtp, JitterSurvivesTimestampWrap) {
    JitterEstimator j;
    j.update(0xFFFFFF60u, 0xFFFFFF60u);
    j.update(0x00000000u + 0x00, 0x00000000u);          // both wrapped identically: transit unchanged
    EXPECT_DOUBLE_EQ(j.jitter_units(), 0.0);
}
TEST(Rtp, LossFraction) {
    EXPECT_EQ(loss_fraction_256(100, 95), 12);          // 5/100 * 256 = 12.8 -> 12 (truncated)
    EXPECT_EQ(loss_fraction_256(100, 100), 0);
    EXPECT_EQ(loss_fraction_256(100, 105), 0);          // duplicates: never negative
    EXPECT_EQ(loss_fraction_256(0, 0), 0);
    EXPECT_EQ(loss_fraction_256(4, 0), 255);            // everything lost: (4<<8)/4 = 256 would overflow; clamp
}

// ============================================================ RTCP
TEST(Rtcp, SenderReportLayout) {
    RtcpPacket sr; sr.type = 200; sr.ssrc = 0x11223344;
    sr.ntp_msw = 0xE0000000u; sr.ntp_lsw = 0x80000000u; sr.rtp_ts = 160000; sr.pkt_count = 1000; sr.octet_count = 160000;
    ReportBlock b; b.ssrc = 0xAABBCCDD; b.fraction_lost = 12; b.cum_lost = -1; b.ext_highest_seq = 65537; b.jitter = 9; b.lsr = 0x1234; b.dlsr = 0x10000;
    sr.blocks.push_back(b);
    Bytes w = build_rtcp(sr);
    ASSERT_EQ(w.size(), 52u);                           // 4 hdr + 4 ssrc + 20 sender info + 24 block
    EXPECT_EQ(w[0], 0x81);                              // V=2 P=0 RC=1
    EXPECT_EQ(w[1], 200);
    EXPECT_EQ((w[2] << 8) | w[3], 12);                  // length = 52/4 - 1
    EXPECT_EQ(w[4 + 4 + 20 + 4], 12); EXPECT_EQ(w[4 + 4 + 20 + 5], 0xFF);   // fraction; cum_lost = -1 -> 0xFFFFFF
    auto p = parse_rtcp(w.data(), w.size());
    ASSERT_TRUE(p);
    EXPECT_EQ(p->type, 200); EXPECT_EQ(p->ssrc, 0x11223344u); EXPECT_EQ(p->pkt_count, 1000u); EXPECT_EQ(p->octet_count, 160000u);
    ASSERT_EQ(p->blocks.size(), 1u);
    EXPECT_EQ(p->blocks[0].cum_lost, -1);               // sign extension of the 24-bit field
    EXPECT_EQ(p->blocks[0].ext_highest_seq, 65537u); EXPECT_EQ(p->blocks[0].dlsr, 0x10000u);
}
TEST(Rtcp, ReceiverReportEmptyAndRejects) {
    RtcpPacket rr; rr.type = 201; rr.ssrc = 5;
    Bytes w = build_rtcp(rr);
    ASSERT_EQ(w.size(), 8u);
    EXPECT_EQ((w[2] << 8) | w[3], 1);
    auto p = parse_rtcp(w.data(), w.size());
    ASSERT_TRUE(p); EXPECT_EQ(p->type, 201); EXPECT_TRUE(p->blocks.empty());
    EXPECT_FALSE(parse_rtcp(w.data(), 7));
    Bytes bad = w; bad[1] = 202;                        // SDES: not handled here
    EXPECT_FALSE(parse_rtcp(bad.data(), bad.size()));
    bad = w; bad[0] = 0x81;                             // claims 1 report block but has none
    EXPECT_FALSE(parse_rtcp(bad.data(), bad.size()));
    bad = w; bad[3] = 50;                               // length field larger than the buffer
    EXPECT_FALSE(parse_rtcp(bad.data(), bad.size()));
}
TEST(Rtcp, RttFromLsrDlsr) {
    EXPECT_EQ(ntp_middle32(0x11112222u, 0x33334444u), 0x22223333u);
    // A = 2.0 s, LSR = 1.0 s, DLSR = 0.5 s  ->  RTT = 0.5 s
    EXPECT_NEAR(rtt_ms(0x00020000u, 0x00010000u, 0x00008000u), 500.0, 1e-9);
    EXPECT_LT(rtt_ms(0x00020000u, 0, 0), 0);            // no SR yet
    // wrap-safe: A has wrapped past 2^32
    EXPECT_NEAR(rtt_ms(0x00004000u, 0xFFFFC000u, 0), 500.0, 1e-9);   // 0x8000/65536 s
}

// ============================================================ SDP
static const char* kOffer =
    "v=0\r\no=alice 2890844526 2890844526 IN IP4 10.0.0.1\r\ns=-\r\nc=IN IP4 10.0.0.1\r\nt=0 0\r\n"
    "m=audio 49170 RTP/AVP 96 0 8 101\r\na=rtpmap:96 opus/48000/2\r\na=rtpmap:101 telephone-event/8000\r\n"
    "a=ptime:20\r\na=sendrecv\r\n";

TEST(Sdp, ParseOffer) {
    auto s = parse_sdp(kOffer);
    ASSERT_TRUE(s);
    ASSERT_EQ(s->media.size(), 1u);
    const SdpMedia& m = s->media[0];
    EXPECT_EQ(m.type, "audio"); EXPECT_EQ(m.port, 49170); EXPECT_EQ(m.proto, "RTP/AVP");
    EXPECT_EQ(m.formats, (std::vector<int>{96, 0, 8, 101}));
    EXPECT_EQ(m.conn_addr, "10.0.0.1");
    EXPECT_EQ(m.ptime, 20); EXPECT_EQ(m.direction, "sendrecv");
    ASSERT_TRUE(m.rtpmap.count(96));
    EXPECT_EQ(m.rtpmap.at(96).name, "opus"); EXPECT_EQ(m.rtpmap.at(96).clock, 48000); EXPECT_EQ(m.rtpmap.at(96).channels, 2);
    EXPECT_FALSE(m.rtpmap.count(0));                    // static PT: no rtpmap line
    auto c0 = lookup_codec(m, 0);
    ASSERT_TRUE(c0); EXPECT_EQ(c0->name, "PCMU");
}
TEST(Sdp, MediaLevelConnectionOverridesSession) {
    auto s = parse_sdp("v=0\r\nc=IN IP4 1.1.1.1\r\nm=audio 4000 RTP/AVP 0\r\nc=IN IP4 2.2.2.2\r\n");
    ASSERT_TRUE(s);
    EXPECT_EQ(s->conn_addr, "1.1.1.1"); EXPECT_EQ(s->media[0].conn_addr, "2.2.2.2");
    auto t = parse_sdp("v=0\r\nc=IN IP4 224.2.1.1/127\r\nm=audio 4000 RTP/AVP 0\r\n");
    ASSERT_TRUE(t); EXPECT_EQ(t->media[0].conn_addr, "224.2.1.1");   // TTL suffix stripped
}
TEST(Sdp, RejectsGarbage) {
    EXPECT_FALSE(parse_sdp("hello"));
    EXPECT_FALSE(parse_sdp("v=1\r\n"));                 // only v=0 exists
    EXPECT_FALSE(parse_sdp("m=audio 4000 RTP/AVP 0\r\n"));   // no v=
}
TEST(Sdp, NegotiationFollowsOfferersOrderAndMatchesClock) {
    auto s = parse_sdp(kOffer); ASSERT_TRUE(s);
    const SdpMedia& m = s->media[0];
    EXPECT_EQ(negotiate_codec(m, {{"PCMU", 8000, 1}, {"PCMA", 8000, 1}}), 0);          // opus unsupported -> first match PCMU
    EXPECT_EQ(negotiate_codec(m, {{"PCMA", 8000, 1}, {"OPUS", 48000, 2}}), 96);        // offerer prefers 96; case-insensitive
    EXPECT_EQ(negotiate_codec(m, {{"PCMA", 8000, 1}, {"PCMU", 8000, 1}}), 0);          // 0 before 8 in the offer
    EXPECT_FALSE(negotiate_codec(m, {{"G729", 8000, 1}}));
    EXPECT_FALSE(negotiate_codec(m, {{"opus", 16000, 1}}));                              // same name, wrong clock = no match
}
TEST(Sdp, AnswerRoundTrip) {
    auto s = parse_sdp(kOffer); ASSERT_TRUE(s);
    std::string ans = build_sdp_answer(s->media[0], 96, "10.0.0.2", 5004);
    auto a = parse_sdp(ans);
    ASSERT_TRUE(a) << ans;
    ASSERT_EQ(a->media.size(), 1u);
    EXPECT_EQ(a->media[0].port, 5004); EXPECT_EQ(a->media[0].formats, (std::vector<int>{96}));
    EXPECT_EQ(a->media[0].conn_addr, "10.0.0.2");
    EXPECT_EQ(a->media[0].rtpmap.at(96).clock, 48000);
    auto b = parse_sdp(build_sdp_answer(s->media[0], 0, "10.0.0.2", 5006));
    ASSERT_TRUE(b);
    EXPECT_EQ(b->media[0].formats, (std::vector<int>{0}));
    EXPECT_TRUE(b->media[0].rtpmap.empty());            // static PT: no a=rtpmap needed
}

// ============================================================ SIP
static std::string invite(const std::string& body) {
    return "INVITE sip:bob@example.com SIP/2.0\r\n"
           "Via: SIP/2.0/UDP pc33.example.com;branch=z9hG4bKnashds8\r\n"
           "f: Alice <sip:alice@example.com>;tag=1928301774\r\n"
           "To: Bob <sip:bob@example.com>\r\n"
           "i: a84b4c76e66710@pc33.example.com\r\n"
           "CSeq: 314159 INVITE\r\n"
           "Content-Type: application/sdp\r\n"
           "l: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}
TEST(Sip, ParseInviteWithCompactHeadersAndBody) {
    auto m = parse_sip(invite(kOffer));
    ASSERT_TRUE(m);
    EXPECT_TRUE(m->is_request); EXPECT_EQ(m->method, "INVITE"); EXPECT_EQ(m->uri, "sip:bob@example.com");
    EXPECT_EQ(m->header("From"), "Alice <sip:alice@example.com>;tag=1928301774");   // compact 'f' expanded
    EXPECT_EQ(m->header("call-id"), "a84b4c76e66710@pc33.example.com");               // compact 'i'
    EXPECT_EQ(m->header("CSEQ"), "314159 INVITE");                                    // case-insensitive lookup
    EXPECT_EQ(m->body, std::string(kOffer));
    auto sdp = parse_sdp(m->body);
    ASSERT_TRUE(sdp); EXPECT_EQ(sdp->media[0].port, 49170);                          // SIP carries SDP as an opaque body
}
TEST(Sip, ContentLengthCutsTrailingBytesAndRejectsShortBody) {
    std::string msg = invite("abc") + "EXTRA";
    auto m = parse_sip(msg);
    ASSERT_TRUE(m); EXPECT_EQ(m->body, "abc");
    std::string shortmsg = invite("abc");
    shortmsg.pop_back();
    EXPECT_FALSE(parse_sip(shortmsg));                  // body shorter than Content-Length
}
TEST(Sip, RejectsMalformed) {
    EXPECT_FALSE(parse_sip("INVITE sip:bob@example.com SIP/2.0\r\nVia: x\r\n"));        // no blank line
    EXPECT_FALSE(parse_sip("INVITE sip:bob@example.com HTTP/1.1\r\n\r\n"));
    EXPECT_FALSE(parse_sip("SIP/2.0 99 Weird\r\n\r\n"));                                // status out of range
    EXPECT_FALSE(parse_sip("INVITE sip:x SIP/2.0\r\nthis line has no colon\r\n\r\n"));
}
TEST(Sip, ResponseCopiesRequiredHeadersAndAddsTag) {
    auto req = parse_sip(invite(""));
    ASSERT_TRUE(req);
    std::string r = build_sip_response(*req, 180, "Ringing", "314159");
    auto m = parse_sip(r);
    ASSERT_TRUE(m) << r;
    EXPECT_FALSE(m->is_request); EXPECT_EQ(m->status, 180); EXPECT_EQ(m->reason, "Ringing");
    EXPECT_EQ(m->header("via"), req->header("via"));
    EXPECT_EQ(m->header("from"), req->header("from"));
    EXPECT_EQ(m->header("to"), "Bob <sip:bob@example.com>;tag=314159");
    EXPECT_EQ(m->header("call-id"), req->header("call-id"));
    EXPECT_EQ(m->header("cseq"), "314159 INVITE");
}
TEST(Sip, ResponseDoesNotOverwriteExistingToTag) {
    std::string req_text =
        "BYE sip:alice@example.com SIP/2.0\r\nVia: SIP/2.0/UDP h;branch=z9hG4bK1\r\nFrom: B <sip:b@x>;tag=b1\r\n"
        "To: A <sip:a@x>;tag=a1\r\nCall-ID: c1\r\nCSeq: 2 BYE\r\n\r\n";
    auto req = parse_sip(req_text); ASSERT_TRUE(req);
    auto m = parse_sip(build_sip_response(*req, 200, "OK", "newtag"));
    ASSERT_TRUE(m);
    EXPECT_EQ(m->header("to"), "A <sip:a@x>;tag=a1");   // in-dialog request: tag already present
}
TEST(Sip, CallFlowHappyPathAndFailure) {
    CallState s = CallState::Idle;
    s = call_next(s, CallEvent::SendInvite); EXPECT_EQ(s, CallState::Calling);
    s = call_next(s, CallEvent::Recv1xx);    EXPECT_EQ(s, CallState::Proceeding);   // 100 Trying / 180 Ringing
    s = call_next(s, CallEvent::Recv1xx);    EXPECT_EQ(s, CallState::Proceeding);
    s = call_next(s, CallEvent::Recv2xx);    EXPECT_EQ(s, CallState::Established);  // 200 OK
    s = call_next(s, CallEvent::SendAck);    EXPECT_EQ(s, CallState::Established);
    s = call_next(s, CallEvent::SendBye);    EXPECT_EQ(s, CallState::Terminated);
    EXPECT_EQ(call_next(CallState::Calling, CallEvent::RecvFailure), CallState::Terminated);   // 486 Busy etc.
    EXPECT_EQ(call_next(CallState::Calling, CallEvent::Recv2xx), CallState::Established);      // no 180 is fine
    EXPECT_EQ(call_next(CallState::Idle, CallEvent::RecvBye), CallState::Idle);
    EXPECT_EQ(call_next(CallState::Established, CallEvent::RecvBye), CallState::Terminated);
    EXPECT_EQ(call_next(CallState::Terminated, CallEvent::SendInvite), CallState::Terminated);
}
