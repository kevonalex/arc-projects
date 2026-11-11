#include <gtest/gtest.h>
#include <limits>
#include <string>
#include "grpc_security.hpp"

using namespace ether;
static Bytes B(std::initializer_list<int> l) { Bytes b; for (int x : l) b.push_back(uint8_t(x)); return b; }

// ============================================================ varint, bit by bit (worksheet Part B)
TEST(Varint, EncodeKnownValues) {
    EXPECT_EQ(encode_varint(0), B({0x00}));
    EXPECT_EQ(encode_varint(1), B({0x01}));
    EXPECT_EQ(encode_varint(127), B({0x7F}));
    EXPECT_EQ(encode_varint(128), B({0x80, 0x01}));
    EXPECT_EQ(encode_varint(150), B({0x96, 0x01}));            // the protobuf docs' example
    EXPECT_EQ(encode_varint(300), B({0xAC, 0x02}));
    EXPECT_EQ(encode_varint(UINT64_MAX), B({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}));
}
TEST(Varint, NegativeInt32AsPlainVarintCostsTenBytes) {
    EXPECT_EQ(encode_varint(static_cast<uint64_t>(int64_t{-1})).size(), 10u);   // why sint32/zigzag exists
    EXPECT_EQ(encode_varint(zigzag_encode(-1)), B({0x01}));
}
TEST(Varint, DecodeAndRoundTrip) {
    size_t used = 0;
    Bytes b = B({0x96, 0x01, 0xFF});
    auto v = decode_varint(b.data(), b.size(), used);
    ASSERT_TRUE(v); EXPECT_EQ(*v, 150u); EXPECT_EQ(used, 2u);   // stops at the first byte without the continuation bit
    for (uint64_t x : std::initializer_list<uint64_t>{0, 1, 127, 128, 16383, 16384, 0xFFFFFFFFull, 0x123456789ABCDEFull, UINT64_MAX}) {
        Bytes e = encode_varint(x);
        auto d = decode_varint(e.data(), e.size(), used);
        ASSERT_TRUE(d); EXPECT_EQ(*d, x); EXPECT_EQ(used, e.size());
    }
}
TEST(Varint, DecodeRejectsMalformed) {
    size_t used = 0;
    Bytes trunc = B({0x96});
    EXPECT_FALSE(decode_varint(trunc.data(), trunc.size(), used));          // continuation bit but no next byte
    EXPECT_FALSE(decode_varint(trunc.data(), 0, used));
    Bytes eleven(11, 0x80); eleven.back() = 0x00;
    EXPECT_FALSE(decode_varint(eleven.data(), eleven.size(), used));        // longer than 10 bytes
    Bytes over = B({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02});
    EXPECT_FALSE(decode_varint(over.data(), over.size(), used));            // 10th byte > 1 overflows 64 bits
}
TEST(Varint, ZigZag) {
    EXPECT_EQ(zigzag_encode(0), 0u); EXPECT_EQ(zigzag_encode(-1), 1u); EXPECT_EQ(zigzag_encode(1), 2u);
    EXPECT_EQ(zigzag_encode(-2), 3u); EXPECT_EQ(zigzag_encode(2), 4u);
    EXPECT_EQ(zigzag_encode(2147483647), 4294967294u);
    EXPECT_EQ(zigzag_encode(-2147483648LL), 4294967295u);
    for (int64_t x : {int64_t{0}, int64_t{-1}, int64_t{1}, int64_t{-123456789}, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()})
        EXPECT_EQ(zigzag_decode(zigzag_encode(x)), x);
}

// ============================================================ protobuf wire format
TEST(Proto, TagsAndWireExamples) {
    EXPECT_EQ(make_tag(1, WireType::Varint), 0x08u);
    EXPECT_EQ(make_tag(2, WireType::Len), 0x12u);
    EXPECT_EQ(make_tag(4, WireType::Fixed64), 0x21u);
    ProtoWriter a; a.varint(1, 150);
    EXPECT_EQ(a.bytes(), B({0x08, 0x96, 0x01}));                            // docs: message { int32 a = 1; } with a = 150
    ProtoWriter b; b.str(2, "testing");
    EXPECT_EQ(b.bytes(), B({0x12, 0x07, 't', 'e', 's', 't', 'i', 'n', 'g'}));
    ProtoWriter c; c.fixed32(5, 0x01020304); c.sint(6, -1);
    EXPECT_EQ(c.bytes(), B({0x2D, 0x04, 0x03, 0x02, 0x01, 0x30, 0x01}));    // fixed32 is LITTLE-endian; sint -1 -> 1
}
TEST(Proto, ReaderWalksFieldsIncludingUnknown) {
    Bytes m = B({0x08, 0x96, 0x01, 0x12, 0x03, 'a', 'b', 'c', 0x21, 1, 0, 0, 0, 0, 0, 0, 0});
    ProtoReader r(m.data(), m.size());
    auto f1 = r.next(); ASSERT_TRUE(f1);
    EXPECT_EQ(f1->number, 1u); EXPECT_EQ(f1->type, WireType::Varint); EXPECT_EQ(f1->value, 150u);
    auto f2 = r.next(); ASSERT_TRUE(f2);
    EXPECT_EQ(f2->number, 2u); EXPECT_EQ(f2->type, WireType::Len); EXPECT_EQ(f2->as_string(), "abc");
    auto f4 = r.next(); ASSERT_TRUE(f4);
    EXPECT_EQ(f4->number, 4u); EXPECT_EQ(f4->type, WireType::Fixed64); EXPECT_EQ(f4->value, 1u);
    EXPECT_FALSE(r.next());
    EXPECT_TRUE(r.ok());                                                    // clean end, not an error
}
TEST(Proto, ReaderRejectsMalformed) {
    auto bad = [](Bytes b) { ProtoReader r(b.data(), b.size()); while (r.next()) {} return !r.ok(); };
    EXPECT_TRUE(bad(B({0x08})));                           // tag without value
    EXPECT_TRUE(bad(B({0x08, 0x96})));                     // truncated varint
    EXPECT_TRUE(bad(B({0x12, 0x05, 'a', 'b'})));           // declared length 5, only 2 bytes
    EXPECT_TRUE(bad(B({0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F})));   // huge length must not overflow pointer arithmetic
    EXPECT_TRUE(bad(B({0x0B})));                           // wire type 3 (start group) unsupported
    EXPECT_TRUE(bad(B({0x00, 0x01})));                     // field number 0 is invalid
    EXPECT_TRUE(bad(B({0x21, 1, 2, 3})));                  // fixed64 with 3 bytes
}

// ============================================================ the CallControl messages
TEST(Messages, StartCallRequestBytes) {
    StartCallRequest m; m.call_id = "c1"; m.talkgroup = 7; m.caller = "gw";
    EXPECT_EQ(encode(m), B({0x0A, 0x02, 'c', '1', 0x10, 0x07, 0x1A, 0x02, 'g', 'w'}));
    StartCallRequest empty;
    EXPECT_TRUE(encode(empty).empty());                    // proto3: defaults are not serialised
    m.talkgroup = 300;
    Bytes e = encode(m);
    auto d = decode_start_call(e.data(), e.size());
    ASSERT_TRUE(d);
    EXPECT_EQ(d->call_id, "c1"); EXPECT_EQ(d->talkgroup, 300u); EXPECT_EQ(d->caller, "gw");
}
TEST(Messages, DecodeSkipsUnknownFieldsAndDefaultsMissing) {
    Bytes e = B({0x10, 0x09, 0x28, 0x01, 0x32, 0x02, 'x', 'y'});   // talkgroup=9, unknown field 5 (varint), unknown field 6 (len)
    auto d = decode_start_call(e.data(), e.size());
    ASSERT_TRUE(d);
    EXPECT_EQ(d->talkgroup, 9u); EXPECT_TRUE(d->call_id.empty());  // forward compatibility: old reader, new writer
    Bytes trunc = B({0x0A, 0x05, 'c'});
    EXPECT_FALSE(decode_start_call(trunc.data(), trunc.size()));
}
TEST(Messages, LastValueWinsOnRepeatedScalar) {
    Bytes e = B({0x10, 0x01, 0x10, 0x02});
    auto d = decode_start_call(e.data(), e.size());
    ASSERT_TRUE(d); EXPECT_EQ(d->talkgroup, 2u);
}
TEST(Messages, CallStatsWithDouble) {
    CallStats s; s.call_id = "c1"; s.packets = 300; s.lost = 3; s.jitter_ms = 1.5;
    Bytes e = encode(s);
    EXPECT_EQ(e, B({0x0A, 0x02, 'c', '1', 0x10, 0xAC, 0x02, 0x18, 0x03, 0x21, 0, 0, 0, 0, 0, 0, 0xF8, 0x3F}));   // 1.5 = 0x3FF8000000000000, LE
    auto d = decode_call_stats(e.data(), e.size());
    ASSERT_TRUE(d);
    EXPECT_EQ(d->call_id, "c1"); EXPECT_EQ(d->packets, 300u); EXPECT_EQ(d->lost, 3u); EXPECT_DOUBLE_EQ(d->jitter_ms, 1.5);
    EXPECT_FALSE(decode_call_stats(e.data(), e.size() - 1));       // truncated double
}

// ============================================================ gRPC framing
TEST(GrpcFrame, BuildAndParse) {
    Bytes f = grpc_frame(B({'a', 'b', 'c'}));
    EXPECT_EQ(f, B({0, 0, 0, 0, 3, 'a', 'b', 'c'}));
    Bytes cf = grpc_frame(B({1}), true);
    ASSERT_FALSE(cf.empty());
    EXPECT_EQ(cf[0], 1);
    auto r = grpc_unframe(f.data(), f.size());
    ASSERT_EQ(r.status, GrpcUnframe::Status::Ok);
    EXPECT_EQ(r.payload, B({'a', 'b', 'c'})); EXPECT_FALSE(r.compressed); EXPECT_EQ(r.consumed, 8u);
}
TEST(GrpcFrame, IncrementalArrival) {
    Bytes f = grpc_frame(Bytes(100, 7));
    EXPECT_EQ(grpc_unframe(f.data(), 4).status, GrpcUnframe::Status::NeedMore);       // not even the prefix
    EXPECT_EQ(grpc_unframe(f.data(), 50).status, GrpcUnframe::Status::NeedMore);      // prefix but partial body
    Bytes two = f; two.insert(two.end(), f.begin(), f.end());                          // two messages in one recv
    auto a = grpc_unframe(two.data(), two.size());
    ASSERT_EQ(a.status, GrpcUnframe::Status::Ok); EXPECT_EQ(a.consumed, 105u);
    auto b = grpc_unframe(two.data() + a.consumed, two.size() - a.consumed);
    ASSERT_EQ(b.status, GrpcUnframe::Status::Ok); EXPECT_EQ(b.payload.size(), 100u);
}
TEST(GrpcFrame, RefusesHugeAndBadFlag) {
    Bytes hdr = B({0, 0xFF, 0xFF, 0xFF, 0xFF});
    EXPECT_EQ(grpc_unframe(hdr.data(), hdr.size()).status, GrpcUnframe::Status::TooLarge);   // decided from the header alone
    EXPECT_EQ(grpc_unframe(hdr.data(), hdr.size(), 0xFFFFFFFFull).status, GrpcUnframe::Status::NeedMore);
    Bytes flag = B({2, 0, 0, 0, 0});
    EXPECT_EQ(grpc_unframe(flag.data(), flag.size()).status, GrpcUnframe::Status::BadFlag);
}

// ============================================================ HTTP/2
TEST(Http2, FrameHeaderRoundTrip) {
    Bytes settings = B({0, 0, 0, 4, 0, 0, 0, 0, 0});
    auto h = parse_h2_frame_header(settings.data(), settings.size());
    ASSERT_TRUE(h);
    EXPECT_EQ(h->length, 0u); EXPECT_EQ(h->type, 4); EXPECT_EQ(h->stream_id, 0u);
    EXPECT_STREQ(h2_type_name(h->type), "SETTINGS");
    Bytes hdrs = B({0x00, 0x00, 0x0A, 0x01, 0x04, 0x80, 0x00, 0x00, 0x01});           // R bit set must be ignored
    h = parse_h2_frame_header(hdrs.data(), hdrs.size());
    ASSERT_TRUE(h);
    EXPECT_EQ(h->length, 10u); EXPECT_STREQ(h2_type_name(h->type), "HEADERS"); EXPECT_EQ(h->flags & 0x04, 0x04); EXPECT_EQ(h->stream_id, 1u);
    H2FrameHeader o; o.length = 0x010203; o.type = 0; o.flags = 1; o.stream_id = 0x7FFFFFFF;
    Bytes w = build_h2_frame_header(o);
    EXPECT_EQ(w, B({0x01, 0x02, 0x03, 0, 1, 0x7F, 0xFF, 0xFF, 0xFF}));
    EXPECT_FALSE(parse_h2_frame_header(w.data(), 8));
    EXPECT_STREQ(h2_type_name(99), "UNKNOWN");
    EXPECT_EQ(std::string(kH2Preface).size(), 24u);
}

// ============================================================ deadlines
TEST(Deadline, ParseGrpcTimeout) {
    EXPECT_EQ(parse_grpc_timeout_ns("100m"), 100000000LL);
    EXPECT_EQ(parse_grpc_timeout_ns("2S"), 2000000000LL);
    EXPECT_EQ(parse_grpc_timeout_ns("1H"), 3600000000000LL);
    EXPECT_EQ(parse_grpc_timeout_ns("5M"), 300000000000LL);
    EXPECT_EQ(parse_grpc_timeout_ns("250u"), 250000LL);
    EXPECT_EQ(parse_grpc_timeout_ns("7n"), 7LL);
    for (const char* bad : {"", "m", "100", "100x", "12 3m", "123456789m", "-5S", "1.5S"})
        EXPECT_FALSE(parse_grpc_timeout_ns(bad)) << bad;
}
TEST(Deadline, FormatGrpcTimeout) {
    EXPECT_EQ(format_grpc_timeout(100000000LL), "100m");
    EXPECT_EQ(format_grpc_timeout(2000000000LL), "2S");
    EXPECT_EQ(format_grpc_timeout(60000000000LL), "1M");
    EXPECT_EQ(format_grpc_timeout(3600000000000LL), "1H");
    EXPECT_EQ(format_grpc_timeout(1500000000LL), "1500m");
    EXPECT_EQ(format_grpc_timeout(100000LL), "100u");
    EXPECT_EQ(format_grpc_timeout(123456789LL), "123457u");        // not exact in any unit with <= 8 digits -> round up
    for (int64_t ns : {int64_t{250000000}, int64_t{3000000000}, int64_t{42000}})
        EXPECT_EQ(parse_grpc_timeout_ns(format_grpc_timeout(ns)), ns);
}
TEST(Deadline, PropagationShrinks) {
    EXPECT_EQ(remaining_ms(5000, 1200, 100), 3700);
    EXPECT_EQ(remaining_ms(5000, 5000), 0);
    EXPECT_EQ(remaining_ms(100, 400), 0);                            // never negative: 0 means "already expired"
}
TEST(Deadline, HttpToGrpcStatus) {
    EXPECT_EQ(grpc_status_from_http(503), GrpcStatus::Unavailable);
    EXPECT_EQ(grpc_status_from_http(401), GrpcStatus::Unauthenticated);
    EXPECT_EQ(grpc_status_from_http(404), GrpcStatus::Unimplemented);
    EXPECT_EQ(grpc_status_from_http(400), GrpcStatus::Internal);
    EXPECT_EQ(grpc_status_from_http(418), GrpcStatus::Unknown);
    EXPECT_EQ(static_cast<int>(GrpcStatus::DeadlineExceeded), 4);
}

// ============================================================ SRTP index + replay (worksheet Part F)
TEST(Srtp, IndexRolloverEstimation) {
    SrtpIndex s;
    s.update(65530);                                                 // first packet: roc 0
    EXPECT_EQ(s.estimate(65535), 65535u);
    s.update(65535);
    EXPECT_EQ(s.estimate(0), 65536u);                                // wrapped: next cycle, ROC 1
    s.update(0);
    EXPECT_EQ(s.roc(), 1u);
    EXPECT_EQ(s.estimate(1), 65537u);
    EXPECT_EQ(s.estimate(65534), 65534u);                            // late packet from the previous cycle keeps ROC 0
    s.update(65534);
    EXPECT_EQ(s.roc(), 1u);                                          // a late packet must not move the state back
    EXPECT_EQ(s.estimate(2), 65538u);
}
TEST(Srtp, IndexHighSeqCases) {
    SrtpIndex s; s.update(40000);
    EXPECT_EQ(s.estimate(100), (uint64_t{1} << 16) | 100);           // s_l >= 32768 and tiny seq -> next cycle
    EXPECT_EQ(s.estimate(20000), 20000u);                            // moderately older: same cycle
    SrtpIndex t; t.update(5);
    EXPECT_EQ(t.estimate(65000) >> 16, 0xFFFFFFFFull);               // just after start, huge seq = previous cycle (mod 2^32)
}
TEST(Srtp, ReplayWindow) {
    ReplayWindow w;
    EXPECT_TRUE(w.check(100)); w.update(100);
    EXPECT_FALSE(w.check(100));                                      // duplicate
    EXPECT_TRUE(w.check(99)); w.update(99);
    EXPECT_FALSE(w.check(99));
    EXPECT_TRUE(w.check(101)); w.update(101);
    EXPECT_FALSE(w.check(100));                                      // still remembered after sliding by 1
    w.update(200);
    EXPECT_FALSE(w.check(100));                                      // 100 slots behind: too old
    EXPECT_TRUE(w.check(137));                                       // 200 - 137 = 63: inside the window, unseen
    EXPECT_FALSE(w.check(136));                                      // 64 behind: outside
    w.update(137);
    EXPECT_FALSE(w.check(137));
    EXPECT_TRUE(w.check(150));
    w.update(1000);                                                  // jump > 64: window resets
    EXPECT_FALSE(w.check(200));
    EXPECT_FALSE(w.check(1000));
}
TEST(Srtp, CheckDoesNotConsume) {
    ReplayWindow w; w.update(10);
    EXPECT_TRUE(w.check(11)); EXPECT_TRUE(w.check(11));              // check is const: auth might still fail
    w.update(11);
    EXPECT_FALSE(w.check(11));
}

// ============================================================ CallControl service logic
TEST(CallControl, StartAndEnd) {
    CallControlService svc;
    StartCallRequest r; r.call_id = "c1"; r.talkgroup = 5; r.caller = "radio-7";
    auto ok = svc.start_call(r);
    EXPECT_TRUE(ok.accepted);
    EXPECT_EQ(svc.active_calls(), 1u);
    auto dup = svc.start_call(r);
    EXPECT_FALSE(dup.accepted); EXPECT_EQ(dup.reason, "call already exists");
    StartCallRequest bad; bad.talkgroup = 5;
    EXPECT_EQ(svc.start_call(bad).reason, "call_id required");
    StartCallRequest tg0; tg0.call_id = "c2";
    EXPECT_EQ(svc.start_call(tg0).reason, "talkgroup must be > 0");
    EXPECT_TRUE(svc.end_call("c1"));
    EXPECT_FALSE(svc.end_call("c1"));                                // second end: NOT_FOUND
    EXPECT_EQ(svc.active_calls(), 0u);
}
TEST(CallControl, StatsAccumulate) {
    CallControlService svc;
    StartCallRequest r; r.call_id = "c1"; r.talkgroup = 1; svc.start_call(r);
    svc.record_packet("c1", false, 1.0); svc.record_packet("c1", true, 2.5); svc.record_packet("c1", false, 2.0);
    svc.record_packet("nope", false, 9.0);                           // ignored, must not crash
    auto s = svc.snapshot("c1");
    ASSERT_TRUE(s);
    EXPECT_EQ(s->packets, 3u); EXPECT_EQ(s->lost, 1u); EXPECT_DOUBLE_EQ(s->jitter_ms, 2.0);
    EXPECT_FALSE(svc.snapshot("nope"));
}
TEST(CallControl, WatchStatsStreaming) {
    CallControlService svc;
    StartCallRequest r; r.call_id = "c1"; r.talkgroup = 1; svc.start_call(r);
    EXPECT_EQ(svc.subscribe("nope", [](const CallStats&, bool) { return true; }), -1);
    int updates = 0, finals = 0;
    int id = svc.subscribe("c1", [&](const CallStats& s, bool fin) { EXPECT_EQ(s.call_id, "c1"); (fin ? finals : updates)++; return true; });
    EXPECT_GT(id, 0);
    svc.record_packet("c1", false, 1.0);
    svc.publish(); svc.publish();
    EXPECT_EQ(updates, 2); EXPECT_EQ(finals, 0);
    svc.end_call("c1");                                              // ONE final message, then the stream is closed
    EXPECT_EQ(finals, 1);
    EXPECT_EQ(svc.subscriptions(), 0u);
    svc.publish();
    EXPECT_EQ(updates, 2);
}
TEST(CallControl, ClientCancelRemovesSubscription) {
    CallControlService svc;
    StartCallRequest r; r.call_id = "c1"; r.talkgroup = 1; svc.start_call(r);
    int n = 0;
    svc.subscribe("c1", [&](const CallStats&, bool) { return ++n < 2; });   // cancels on the 2nd update
    svc.publish(); svc.publish(); svc.publish();
    EXPECT_EQ(n, 2);                                                  // third publish did not reach the cancelled client
    EXPECT_EQ(svc.subscriptions(), 0u);
    int id = svc.subscribe("c1", [](const CallStats&, bool) { return true; });
    svc.unsubscribe(id);
    EXPECT_EQ(svc.subscriptions(), 0u);
}
TEST(CallControl, SinkMayCallBackIntoService) {
    CallControlService svc;                                           // a sink that ends the call must not deadlock
    StartCallRequest r; r.call_id = "c1"; r.talkgroup = 1; svc.start_call(r);
    svc.subscribe("c1", [&](const CallStats&, bool fin) { if (!fin) svc.end_call("c1"); return true; });
    svc.publish();
    EXPECT_EQ(svc.active_calls(), 0u);
}
