// Tests for 03-net/06-cellular-encodings. Same file builds against the student stubs and the solutions.
#include <gtest/gtest.h>
#include <climits>
#include <initializer_list>
#include "cellenc.hpp"

using namespace ether;
static Bytes B(std::initializer_list<int> l) { Bytes b; for (int x : l) b.push_back(static_cast<uint8_t>(x)); return b; }

// ============================================================ GTP-U
TEST(Gtpu, PlainGPdu) {
    Bytes w = B({0x30, 0xFF, 0x00, 0x04, 0x12, 0x34, 0x56, 0x78, 0xDE, 0xAD, 0xBE, 0xEF});
    auto g = parse_gtpu(w.data(), w.size());
    ASSERT_TRUE(g);
    EXPECT_EQ(g->version, 1); EXPECT_TRUE(g->pt);
    EXPECT_FALSE(g->e); EXPECT_FALSE(g->s); EXPECT_FALSE(g->pn);
    EXPECT_EQ(g->msg_type, kGtpuGPdu); EXPECT_STREQ(gtpu_msg_name(g->msg_type), "G-PDU");
    EXPECT_EQ(g->length, 4u); EXPECT_EQ(g->teid, 0x12345678u);
    EXPECT_EQ(g->header_len, 8u);
    EXPECT_EQ(g->payload, B({0xDE, 0xAD, 0xBE, 0xEF}));
}
TEST(Gtpu, EchoRequestWithSequence) {
    // Echo Request: S flag, length counts the 4 optional bytes even though there is no payload.
    Bytes w = B({0x32, 0x01, 0x00, 0x04, 0, 0, 0, 0, 0x00, 0x2A, 0x00, 0x00});
    auto g = parse_gtpu(w.data(), w.size());
    ASSERT_TRUE(g);
    EXPECT_TRUE(g->s); EXPECT_FALSE(g->e);
    EXPECT_EQ(g->seq, 42u); EXPECT_EQ(g->teid, 0u);          // TEID 0 is used for path management messages
    EXPECT_EQ(g->header_len, 12u); EXPECT_TRUE(g->payload.empty());
    EXPECT_STREQ(gtpu_msg_name(g->msg_type), "Echo Request");
}
TEST(Gtpu, ExtensionHeaderPduSessionContainer) {
    // 5G N3: E flag, one extension (type 0x85, 1 unit = 4 bytes: len, 2 content bytes, next=0), then the user packet.
    Bytes w = B({0x34, 0xFF, 0x00, 0x0C, 0xCA, 0xFE, 0x00, 0x01,
                 0x00, 0x00, 0x00, 0x85,
                 0x01, 0x00, 0x05, 0x00,
                 0xAA, 0xBB, 0xCC, 0xDD});
    auto g = parse_gtpu(w.data(), w.size());
    ASSERT_TRUE(g);
    EXPECT_TRUE(g->e);
    ASSERT_EQ(g->exts.size(), 1u);
    EXPECT_EQ(g->exts[0].type, kGtpuExtPduSessionContainer);
    EXPECT_EQ(g->exts[0].content, B({0x00, 0x05}));          // QFI 5 in the low 6 bits of byte 2 of the container
    EXPECT_EQ(g->header_len, 16u);
    EXPECT_EQ(g->payload, B({0xAA, 0xBB, 0xCC, 0xDD}));
}
TEST(Gtpu, TwoChainedExtensions) {
    Bytes w = B({0x34, 0xFF, 0x00, 0x0C, 0, 0, 0, 9,
                 0x00, 0x00, 0x00, 0xC0,
                 0x01, 0x11, 0x22, 0x85,          // ext type 0xC0, next = 0x85
                 0x01, 0x33, 0x44, 0x00});        // ext type 0x85, next = none
    auto g = parse_gtpu(w.data(), w.size());
    ASSERT_TRUE(g);
    ASSERT_EQ(g->exts.size(), 2u);
    EXPECT_EQ(g->exts[0].type, 0xC0); EXPECT_EQ(g->exts[0].content, B({0x11, 0x22}));
    EXPECT_EQ(g->exts[1].type, 0x85); EXPECT_EQ(g->exts[1].content, B({0x33, 0x44}));
    EXPECT_TRUE(g->payload.empty());
}
TEST(Gtpu, RejectsMalformed) {
    auto bad = [](const Bytes& w) { return !parse_gtpu(w.data(), w.size()); };
    EXPECT_TRUE(bad(B({0x30, 0xFF, 0x00, 0x00, 0, 0, 0})));                       // 7 bytes
    EXPECT_TRUE(bad(B({0x50, 0xFF, 0x00, 0x00, 0, 0, 0, 1})));                    // version 2
    EXPECT_TRUE(bad(B({0x20, 0xFF, 0x00, 0x00, 0, 0, 0, 1})));                    // PT=0 is GTP', not GTP
    EXPECT_TRUE(bad(B({0x30, 0xFF, 0x00, 0x09, 0, 0, 0, 1, 1, 2, 3, 4})));        // length longer than datagram
    EXPECT_TRUE(bad(B({0x32, 0xFF, 0x00, 0x02, 0, 0, 0, 1, 0, 0})));              // S flag but optional part truncated
    EXPECT_TRUE(bad(B({0x34, 0xFF, 0x00, 0x08, 0, 0, 0, 1, 0, 0, 0, 0x85, 0x00, 0x00, 0x00, 0x00})));   // extension length 0
    EXPECT_TRUE(bad(B({0x34, 0xFF, 0x00, 0x08, 0, 0, 0, 1, 0, 0, 0, 0x85, 0x03, 0x00, 0x00, 0x00})));   // extension overruns
    Bytes loop = B({0x34, 0xFF, 0x00, 0x00, 0, 0, 0, 1, 0, 0, 0, 0x85});
    for (int i = 0; i < 12; ++i) { loop.push_back(0x01); loop.push_back(0); loop.push_back(0); loop.push_back(0x85); }
    loop[3] = static_cast<uint8_t>(loop.size() - 8);
    EXPECT_TRUE(bad(loop));                                                        // endless extension chain
}
TEST(Gtpu, TrailingBytesBeyondLengthAreIgnored) {
    Bytes w = B({0x30, 0xFF, 0x00, 0x02, 0, 0, 0, 7, 0x11, 0x22, 0x99, 0x99});
    auto g = parse_gtpu(w.data(), w.size());
    ASSERT_TRUE(g);
    EXPECT_EQ(g->payload, B({0x11, 0x22}));
}
TEST(Gtpu, BuildAndRoundTrip) {
    Bytes inner = B({0x45, 0, 0, 20, 1, 2, 3, 4});
    Bytes w = build_gtpu_gpdu(0xA1B2C3D4, inner);
    EXPECT_EQ(w, B({0x30, 0xFF, 0x00, 0x08, 0xA1, 0xB2, 0xC3, 0xD4, 0x45, 0, 0, 20, 1, 2, 3, 4}));
    Bytes ws = build_gtpu_gpdu(1, inner, 513);
    ASSERT_GE(ws.size(), 12u);
    EXPECT_EQ(ws[0], 0x32); EXPECT_EQ(ws[3], 12);
    EXPECT_EQ(ws[8], 0x02); EXPECT_EQ(ws[9], 0x01);
    auto g = parse_gtpu(ws.data(), ws.size());
    ASSERT_TRUE(g);
    EXPECT_EQ(g->seq, 513u); EXPECT_EQ(g->payload, inner);
    std::vector<GtpuExt> ex{{0x85, B({0x00, 0x09})}};
    Bytes we = build_gtpu_gpdu(77, inner, std::nullopt, ex);
    ASSERT_GE(we.size(), 12u);
    EXPECT_EQ(we[0], 0x34);
    auto g2 = parse_gtpu(we.data(), we.size());
    ASSERT_TRUE(g2);
    ASSERT_EQ(g2->exts.size(), 1u);
    EXPECT_EQ(g2->exts[0].content, B({0x00, 0x09})); EXPECT_EQ(g2->payload, inner); EXPECT_EQ(g2->teid, 77u);
}
TEST(Gtpu, OverheadAndInnerMtu) {
    EXPECT_EQ(gtpu_overhead(), 36u);                 // IPv4 20 + UDP 8 + GTP-U 8
    EXPECT_EQ(gtpu_overhead(true), 40u);
    EXPECT_EQ(gtpu_overhead(true, 4), 44u);
    EXPECT_EQ(gtpu_inner_mtu(1500), 1464u);
    EXPECT_EQ(gtpu_inner_mtu(1500, true, 4), 1456u);
    EXPECT_EQ(gtpu_inner_mtu(30), 0u);
    EXPECT_EQ(kGtpuPort, 2152);
}

// ============================================================ Diameter
TEST(Diameter, ParseHeader) {
    Bytes w = B({0x01, 0x00, 0x00, 0x14, 0x80, 0x00, 0x01, 0x01,
                 0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
    auto h = parse_diameter_header(w.data(), w.size());
    ASSERT_TRUE(h);
    EXPECT_EQ(h->version, 1); EXPECT_EQ(h->length, 20u); EXPECT_TRUE(h->request());
    EXPECT_EQ(h->flags & kDiaP, 0);
    EXPECT_EQ(h->command, 257u); EXPECT_EQ(diameter_command_name(h->command, h->request()), "Capabilities-Exchange-Request");
    EXPECT_EQ(diameter_command_name(272, false), "Credit-Control-Answer");
    EXPECT_EQ(h->app_id, 0u); EXPECT_EQ(h->hop_by_hop, 0x11223344u); EXPECT_EQ(h->end_to_end, 0x55667788u);
}
TEST(Diameter, HeaderRejects) {
    auto bad = [](Bytes w) { return !parse_diameter_header(w.data(), w.size()); };
    Bytes ok = B({0x01, 0, 0, 0x14, 0x80, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    EXPECT_FALSE(bad(ok));
    Bytes v2 = ok; v2[0] = 2; EXPECT_TRUE(bad(v2));
    Bytes odd = ok; odd[3] = 0x15; odd.push_back(0); EXPECT_TRUE(bad(odd));                  // length not a multiple of 4
    Bytes tiny = ok; tiny[3] = 0x10; EXPECT_TRUE(bad(tiny));                                 // length < 20
    Bytes longer = ok; longer[3] = 0x18; EXPECT_TRUE(bad(longer));                           // claims more than we have
    EXPECT_TRUE(bad(Bytes(19, 1)));
}
TEST(Diameter, AvpBytesAndPadding) {
    Bytes a = avp_u32(266, kAvpM, 10415);                                                   // Vendor-Id
    EXPECT_EQ(a, B({0x00, 0x00, 0x01, 0x0A, 0x40, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x28, 0xAF}));
    Bytes s = avp_utf8(264, kAvpM, "abc");                                                  // Origin-Host
    EXPECT_EQ(s, B({0x00, 0x00, 0x01, 0x08, 0x40, 0x00, 0x00, 0x0B, 'a', 'b', 'c', 0x00}));  // length 11, padded to 12 (pad NOT counted)
    Bytes v = build_avp(1, kAvpV | kAvpM, B({1, 2, 3, 4}), 10415);
    ASSERT_EQ(v.size(), 16u); EXPECT_EQ(v[7], 16);
    EXPECT_EQ(v, B({0, 0, 0, 1, 0xC0, 0, 0, 16, 0, 0, 0x28, 0xAF, 1, 2, 3, 4}));
}
TEST(Diameter, BuildAndParseCer) {
    std::vector<Bytes> avps{avp_utf8(264, kAvpM, "gw.example.org"), avp_utf8(296, kAvpM, "example.org"),
                            avp_u32(266, kAvpM, 10415), avp_utf8(269, 0, "etherstack-gw")};
    Bytes m = build_diameter(257, true, 0, 0xAABBCCDD, 0x01020304, avps);
    EXPECT_EQ(m.size() % 4, 0u);
    auto h = parse_diameter_header(m.data(), m.size());
    ASSERT_TRUE(h);
    EXPECT_EQ(h->length, m.size()); EXPECT_TRUE(h->request()); EXPECT_EQ(h->flags & kDiaP, kDiaP);
    auto list = parse_diameter_avps(m.data(), m.size(), *h);
    ASSERT_TRUE(list);
    ASSERT_EQ(list->size(), 4u);
    EXPECT_EQ((*list)[0].code, 264u); EXPECT_EQ(avp_as_string((*list)[0]), "gw.example.org");   // padding stripped
    EXPECT_EQ(avp_as_string((*list)[1]), "example.org");
    EXPECT_EQ(avp_as_u32((*list)[2]).value_or(0), 10415u);
    EXPECT_EQ((*list)[3].flags & kAvpM, 0);                                                     // Product-Name: M clear
    EXPECT_FALSE(avp_as_u32((*list)[0]));                                                       // 14 bytes is not a u32
}
TEST(Diameter, VendorAvpAndGrouped) {
    // Vendor-Specific-Application-Id (260) { Vendor-Id (266), Auth-Application-Id (258) } - S6a is app 16777251
    Bytes inner = avp_u32(266, kAvpM, 10415);
    Bytes b2 = avp_u32(258, kAvpM, 16777251);
    inner.insert(inner.end(), b2.begin(), b2.end());
    Bytes grouped = build_avp(260, kAvpM, inner);
    auto outer = parse_avps(grouped.data(), grouped.size());
    ASSERT_TRUE(outer);
    ASSERT_EQ(outer->size(), 1u); EXPECT_EQ((*outer)[0].code, 260u);
    auto kids = parse_avps((*outer)[0].data.data(), (*outer)[0].data.size());
    ASSERT_TRUE(kids);
    ASSERT_EQ(kids->size(), 2u);
    EXPECT_EQ(avp_as_u32((*kids)[1]).value_or(0), 16777251u);

    Bytes vend = build_avp(1400, kAvpV | kAvpM, B({0, 0, 0, 5}), 10415);                         // 3GPP vendor AVP
    auto l = parse_avps(vend.data(), vend.size());
    ASSERT_TRUE(l);
    EXPECT_EQ((*l)[0].vendor, 10415u); EXPECT_EQ((*l)[0].data, B({0, 0, 0, 5}));
}
TEST(Diameter, AvpRejects) {
    auto bad = [](const Bytes& w) { return !parse_avps(w.data(), w.size()); };
    EXPECT_TRUE(bad(B({0, 0, 1, 8, 0x40, 0, 0})));                                              // < 8 bytes
    EXPECT_TRUE(bad(B({0, 0, 1, 8, 0x40, 0, 0, 7, 0})));                                        // length < header
    EXPECT_TRUE(bad(B({0, 0, 1, 8, 0x40, 0, 0, 0x20, 1, 2, 3, 4})));                            // length beyond buffer
    EXPECT_TRUE(bad(B({0, 0, 1, 8, 0xC0, 0, 0, 10, 0, 0, 0, 1})));                              // V flag but length 10 < 12
    EXPECT_TRUE(bad(B({0, 0, 1, 8, 0x40, 0, 0, 9, 1, 0, 0})));                                  // padding missing (needs 12 bytes)
    EXPECT_FALSE(bad(B({0, 0, 1, 8, 0x40, 0, 0, 9, 1, 0, 0, 0})));
    EXPECT_FALSE(bad(Bytes{}));                                                                 // zero AVPs is fine
}

// ============================================================ bit packing
TEST(Bits, WriterMsbFirst) {
    BitWriter w;
    w.put(0b101, 3); w.put(1, 1); w.put(0b0011, 4);
    EXPECT_EQ(w.bit_count(), 8u);
    EXPECT_EQ(w.bytes(), B({0xB3}));
    w.put(0x3FF, 10);                                      // 10 more bits -> 2 more bytes, zero padded
    EXPECT_EQ(w.bit_count(), 18u);
    EXPECT_EQ(w.bytes(), B({0xB3, 0xFF, 0xC0}));
    BitWriter z; z.put(0xFF, 0);                           // zero bits writes nothing
    EXPECT_EQ(z.bit_count(), 0u); EXPECT_TRUE(z.bytes().empty());
    BitWriter big; big.put(0x0123456789ABCDEFull, 64);
    EXPECT_EQ(big.bytes(), B({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}));
}
TEST(Bits, ValueIsMaskedToWidth) {
    BitWriter w; w.put(0xFF, 3);                           // only the low 3 bits are written
    EXPECT_EQ(w.bit_count(), 3u); EXPECT_EQ(w.bytes(), B({0xE0}));
}
TEST(Bits, ReaderRoundTripAndBounds) {
    Bytes d = B({0xB3, 0xFF, 0xC0});
    BitReader r(d.data(), d.size());
    EXPECT_EQ(r.get(3).value(), 0b101u);
    EXPECT_EQ(r.get(1).value(), 1u);
    EXPECT_EQ(r.get(4).value(), 0b0011u);
    EXPECT_EQ(r.get(10).value(), 0x3FFu);
    EXPECT_EQ(r.bits_left(), 6u);
    EXPECT_FALSE(r.get(7));                                // only 6 left: must fail, and must not consume
    EXPECT_EQ(r.bits_left(), 6u);
    EXPECT_EQ(r.get(6).value(), 0u);
    EXPECT_EQ(r.get(0).value(), 0u);                       // zero-width read at the end is fine
    EXPECT_FALSE(r.get(1));
}

// ============================================================ ASN.1 UPER basics
TEST(Uper, CeilLog2) {
    EXPECT_EQ(ceil_log2(1), 0u); EXPECT_EQ(ceil_log2(2), 1u); EXPECT_EQ(ceil_log2(3), 2u); EXPECT_EQ(ceil_log2(4), 2u);
    EXPECT_EQ(ceil_log2(5), 3u); EXPECT_EQ(ceil_log2(8), 3u); EXPECT_EQ(ceil_log2(9), 4u);
    EXPECT_EQ(ceil_log2(255), 8u); EXPECT_EQ(ceil_log2(256), 8u); EXPECT_EQ(ceil_log2(257), 9u);
    EXPECT_EQ(ceil_log2(1000), 10u); EXPECT_EQ(ceil_log2(uint64_t{1} << 32), 32u);
}
TEST(Uper, ConstrainedIntegerWidths) {
    auto bits = [](int64_t v, int64_t lb, int64_t ub) { BitWriter w; EXPECT_TRUE(uper_put_constrained(w, v, lb, ub)); return w.bit_count(); };
    EXPECT_EQ(bits(5, 5, 5), 0u);                          // a single permitted value carries no information
    EXPECT_EQ(bits(1, 0, 1), 1u);
    EXPECT_EQ(bits(2, 0, 2), 2u);
    EXPECT_EQ(bits(7, 0, 7), 3u);
    EXPECT_EQ(bits(1234, 1000, 1999), 10u);
    EXPECT_EQ(bits(-5, -5, 5), 4u);                        // 11 values
    EXPECT_EQ(bits(0, 0, 255), 8u);
    EXPECT_EQ(bits(0, 0, 4294967295LL), 32u);              // UPER keeps minimum bits even for big ranges (aligned PER differs)
    EXPECT_EQ(bits(INT64_MIN, INT64_MIN, INT64_MAX), 64u);
}
TEST(Uper, ConstrainedIntegerValues) {
    BitWriter w;
    EXPECT_TRUE(uper_put_constrained(w, 1234, 1000, 1999));      // 234 = 0b0011101010
    EXPECT_EQ(w.bytes(), B({0x3A, 0x80}));
    EXPECT_FALSE(uper_put_constrained(w, 2000, 1000, 1999));     // out of range is refused, writes nothing
    EXPECT_FALSE(uper_put_constrained(w, 999, 1000, 1999));
    EXPECT_FALSE(uper_put_constrained(w, 0, 5, 4));
    EXPECT_EQ(w.bit_count(), 10u);
    BitReader r(w.bytes().data(), w.bytes().size());
    EXPECT_EQ(uper_get_constrained(r, 1000, 1999).value(), 1234);
    BitWriter n; uper_put_constrained(n, -5, -5, 5); uper_put_constrained(n, 5, -5, 5);
    BitReader rn(n.bytes().data(), n.bytes().size());
    EXPECT_EQ(uper_get_constrained(rn, -5, 5).value(), -5);
    EXPECT_EQ(uper_get_constrained(rn, -5, 5).value(), 5);
}
TEST(Uper, DecodeRejectsOutOfRangeCodepoint) {
    Bytes d = B({0xC0});                                   // bits "11" in a 0..2 field: 3 is not a legal value
    BitReader r(d.data(), d.size());
    EXPECT_FALSE(uper_get_constrained(r, 0, 2));
    Bytes e = B({0xFF});                                   // 1000..1999 needs 10 bits; only 8 supplied
    BitReader r2(e.data(), e.size());
    EXPECT_FALSE(uper_get_constrained(r2, 1000, 1999));
    Bytes f = B({0xFF, 0xC0});                             // 1023 is a 10-bit pattern but 1023 > 999 (range 1000)
    BitReader r3(f.data(), f.size());
    EXPECT_FALSE(uper_get_constrained(r3, 1000, 1999));
}
TEST(Uper, LengthDeterminant) {
    auto enc = [](size_t n) { BitWriter w; EXPECT_TRUE(uper_put_length(w, n)); return w.bytes(); };
    EXPECT_EQ(enc(0), B({0x00}));
    EXPECT_EQ(enc(5), B({0x05}));
    EXPECT_EQ(enc(127), B({0x7F}));
    EXPECT_EQ(enc(128), B({0x80, 0x80}));
    EXPECT_EQ(enc(200), B({0x80, 0xC8}));
    EXPECT_EQ(enc(16383), B({0xBF, 0xFF}));
    BitWriter w; EXPECT_FALSE(uper_put_length(w, 16384));
    for (size_t n : {size_t{0}, size_t{1}, size_t{127}, size_t{128}, size_t{300}, size_t{16383}}) {
        Bytes b = enc(n);
        BitReader r(b.data(), b.size());
        EXPECT_EQ(uper_get_length(r).value(), n);
    }
    Bytes frag = B({0xC1});                                // fragmentation prefix: unsupported -> nullopt
    BitReader rf(frag.data(), frag.size());
    EXPECT_FALSE(uper_get_length(rf));
    Bytes trunc = B({0x80});
    BitReader rt(trunc.data(), trunc.size());
    EXPECT_FALSE(uper_get_length(rt));
}
TEST(Uper, DemoSequenceOptionalAbsent) {
    DemoMsg m; m.a = 5; m.b = true; m.d = 1;
    Bytes e = encode_demo(m);
    // bits: 0 | 101 | 1 | 01  = 0101101 -> padded 01011010
    EXPECT_EQ(e, B({0x5A}));
    auto d = decode_demo(e.data(), e.size());
    ASSERT_TRUE(d);
    EXPECT_EQ(d->a, 5); EXPECT_TRUE(d->b); EXPECT_FALSE(d->c.has_value()); EXPECT_EQ(d->d, 1);
}
TEST(Uper, DemoSequenceOptionalPresent) {
    DemoMsg m; m.a = 5; m.b = true; m.c = 1234; m.d = 2;
    Bytes e = encode_demo(m);
    // bits: 1 | 101 | 1 | 0011101010 | 10  (17 bits)
    EXPECT_EQ(e, B({0xD9, 0xD5, 0x00}));
    auto d = decode_demo(e.data(), e.size());
    ASSERT_TRUE(d);
    EXPECT_EQ(d->a, 5); EXPECT_TRUE(d->b); ASSERT_TRUE(d->c); EXPECT_EQ(*d->c, 1234); EXPECT_EQ(d->d, 2);
    EXPECT_FALSE(decode_demo(e.data(), 1));                // truncated
    EXPECT_FALSE(decode_demo(e.data(), 0));
}
TEST(Uper, DemoExhaustiveRoundTrip) {
    int count = 0;
    for (int a = 0; a < 8; ++a) for (int b = 0; b < 2; ++b) for (int d = 0; d < 3; ++d)
        for (int c = 999; c <= 2000; c += 7) {
            DemoMsg m; m.a = a; m.b = b; m.d = d;
            if (c >= 1000 && c <= 1999) m.c = c;
            Bytes e = encode_demo(m);
            auto r = decode_demo(e.data(), e.size());
            ASSERT_TRUE(r);
            EXPECT_EQ(r->a, a); EXPECT_EQ(r->b, b != 0); EXPECT_EQ(r->d, d); EXPECT_EQ(r->c, m.c);
            ++count;
        }
    EXPECT_GT(count, 1000);
}
TEST(Uper, OctetStringBitPackedNoPadding) {
    BitWriter w;
    EXPECT_TRUE(uper_put_octets(w, B({0xAB, 0xCD}), 1, 8));    // length-1 = 1 in 3 bits, then the bytes unaligned
    EXPECT_EQ(w.bit_count(), 19u);
    EXPECT_EQ(w.bytes(), B({0x35, 0x79, 0xA0}));
    BitReader r(w.bytes().data(), w.bytes().size());
    auto d = uper_get_octets(r, 1, 8);
    ASSERT_TRUE(d); EXPECT_EQ(*d, B({0xAB, 0xCD}));
    BitWriter fx;                                               // fixed size: no length at all
    EXPECT_TRUE(uper_put_octets(fx, B({1, 2, 3, 4}), 4, 4));
    EXPECT_EQ(fx.bit_count(), 32u);
    BitWriter bad;
    EXPECT_FALSE(uper_put_octets(bad, B({1, 2, 3}), 4, 4));
    EXPECT_FALSE(uper_put_octets(bad, Bytes(9, 0), 1, 8));
    EXPECT_FALSE(uper_put_octets(bad, B({}), 1, 8));
    Bytes trunc = B({0x35, 0x79});                              // 16 bits < 19 needed
    BitReader rt(trunc.data(), trunc.size());
    EXPECT_FALSE(uper_get_octets(rt, 1, 8));
}
