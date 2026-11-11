// SOLUTION - 03-net/06-cellular-encodings. Pure C++17, header-only. Tests: tests/test_cellenc.cpp
// Everything here is synthetic / textbook-vector based. No live traffic is decoded.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ether {

using Bytes = std::vector<uint8_t>;

// ============================================================================================
// PART 1 - GTP-U (3GPP TS 29.281): the user-plane tunnel between eNodeB/gNB and the core (S1-U / N3 / S5-S8 user plane)
//
//   byte 0 : Version(3)=1 | PT(1)=1 | spare(1) | E(1) | S(1) | PN(1)
//   byte 1 : message type  (0xFF = G-PDU: a tunnelled user packet; 1/2 = Echo req/resp; 0x1A Error Indication; 0xFE End Marker)
//   byte 2-3: length = number of bytes AFTER the first 8 mandatory bytes
//   byte 4-7: TEID  (tunnel endpoint id - chosen by the RECEIVER, it is a local index, not a global id)
//   if any of E,S,PN set: 4 more bytes = sequence(16) | N-PDU number(8) | next extension header type(8)
//   extension header: length(8, in units of 4 bytes, INCLUDING the length and next-type bytes) | content | next type(8)
// ============================================================================================
constexpr uint16_t kGtpuPort = 2152;
constexpr uint8_t kGtpuGPdu = 0xFF, kGtpuEchoReq = 0x01, kGtpuEchoResp = 0x02, kGtpuErrorInd = 0x1A, kGtpuEndMarker = 0xFE;
constexpr uint8_t kGtpuExtPduSessionContainer = 0x85;   // 5G QFI carrier  (VERIFY code point)

struct GtpuExt { uint8_t type = 0; Bytes content; };       // content excludes length byte and next-type byte

struct GtpuPacket {
    uint8_t version = 0;        // must be 1
    bool pt = false;            // protocol type: 1 = GTP, 0 = GTP' (charging)
    bool e = false, s = false, pn = false;
    uint8_t msg_type = 0;
    uint16_t length = 0;        // as on the wire
    uint32_t teid = 0;
    uint16_t seq = 0;           // valid only if e||s||pn
    uint8_t npdu = 0;
    std::vector<GtpuExt> exts;
    size_t header_len = 0;      // 8, or 12 + extension bytes
    Bytes payload;              // the tunnelled user packet for G-PDU
};

inline const char* gtpu_msg_name(uint8_t t) {
    switch (t) {
        case 0x01: return "Echo Request";
        case 0x02: return "Echo Response";
        case 0x1A: return "Error Indication";
        case 0x1F: return "Supported Extension Headers Notification";
        case 0xFE: return "End Marker";
        case 0xFF: return "G-PDU";
        default: return "other";
    }
}

// Reject: <8 bytes, version != 1, PT != 1, length that does not fit the datagram, optional part missing when a flag
// demands it, extension headers that overrun, a zero extension length, or more than 8 chained extensions.
inline std::optional<GtpuPacket> parse_gtpu(const uint8_t* p, size_t n) {
    if (n < 8) return std::nullopt;
    GtpuPacket g;
    g.version = p[0] >> 5;
    g.pt = (p[0] >> 4) & 1;
    g.e = (p[0] >> 2) & 1; g.s = (p[0] >> 1) & 1; g.pn = p[0] & 1;
    g.msg_type = p[1];
    g.length = static_cast<uint16_t>((p[2] << 8) | p[3]);
    g.teid = (uint32_t{p[4]} << 24) | (uint32_t{p[5]} << 16) | (uint32_t{p[6]} << 8) | p[7];
    if (g.version != 1 || !g.pt) return std::nullopt;
    if (8 + size_t{g.length} > n) return std::nullopt;
    size_t end = 8 + size_t{g.length};
    size_t pos = 8;
    if (g.e || g.s || g.pn) {
        if (end - pos < 4) return std::nullopt;
        g.seq = static_cast<uint16_t>((p[8] << 8) | p[9]);
        g.npdu = p[10];
        uint8_t next = p[11];
        pos = 12;
        int guard = 0;
        while (next != 0) {
            if (++guard > 8 || pos >= end) return std::nullopt;
            size_t units = p[pos];
            if (units == 0 || pos + units * 4 > end) return std::nullopt;
            GtpuExt x; x.type = next;
            x.content.assign(p + pos + 1, p + pos + units * 4 - 1);
            next = p[pos + units * 4 - 1];
            pos += units * 4;
            g.exts.push_back(std::move(x));
        }
    }
    g.header_len = pos;
    g.payload.assign(p + pos, p + end);
    return g;
}

// Build a G-PDU. If 'seq' is given the S flag is set (adds the 4 optional bytes). 'exts' sets E (content length+2 must be a multiple of 4).
inline Bytes build_gtpu_gpdu(uint32_t teid, const Bytes& inner, std::optional<uint16_t> seq = std::nullopt,
                             const std::vector<GtpuExt>& exts = {}) {
    bool opt = seq.has_value() || !exts.empty();
    Bytes ext_bytes;
    for (size_t i = 0; i < exts.size(); ++i) {
        size_t total = exts[i].content.size() + 2;
        ext_bytes.push_back(static_cast<uint8_t>(total / 4));
        ext_bytes.insert(ext_bytes.end(), exts[i].content.begin(), exts[i].content.end());
        ext_bytes.push_back(i + 1 < exts.size() ? exts[i + 1].type : 0);
    }
    uint16_t len = static_cast<uint16_t>((opt ? 4 : 0) + ext_bytes.size() + inner.size());
    Bytes b;
    b.push_back(static_cast<uint8_t>((1 << 5) | (1 << 4) | (exts.empty() ? 0 : 4) | (seq ? 2 : 0)));
    b.push_back(kGtpuGPdu);
    b.push_back(static_cast<uint8_t>(len >> 8)); b.push_back(static_cast<uint8_t>(len));
    for (int sh = 24; sh >= 0; sh -= 8) b.push_back(static_cast<uint8_t>(teid >> sh));
    if (opt) {
        uint16_t sq = seq.value_or(0);
        b.push_back(static_cast<uint8_t>(sq >> 8)); b.push_back(static_cast<uint8_t>(sq));
        b.push_back(0);                                                     // N-PDU number
        b.push_back(exts.empty() ? 0 : exts[0].type);
    }
    b.insert(b.end(), ext_bytes.begin(), ext_bytes.end());
    b.insert(b.end(), inner.begin(), inner.end());
    return b;
}

// Encapsulation overhead on the transport network: outer IPv4 (20) + UDP (8) + GTP-U (8, or 12 with optional fields) [+ extension bytes].
inline size_t gtpu_overhead(bool optional_fields = false, size_t ext_bytes = 0) { return 20 + 8 + 8 + (optional_fields ? 4 : 0) + ext_bytes; }
// Largest inner IP packet that fits one outer packet without fragmenting the tunnel.
inline size_t gtpu_inner_mtu(size_t transport_mtu, bool optional_fields = false, size_t ext_bytes = 0) {
    size_t o = gtpu_overhead(optional_fields, ext_bytes);
    return transport_mtu > o ? transport_mtu - o : 0;
}

// ============================================================================================
// PART 2 - Diameter (RFC 6733): AAA/signalling between core nodes (S6a HSS<->MME, Gx PCRF<->PCEF, Gy/Ro charging, Rx)
//
//   header (20 B): Version(8)=1 | Message Length(24, whole message, multiple of 4) | Flags(8) R P E T r r r r |
//                  Command Code(24) | Application-ID(32) | Hop-by-Hop ID(32) | End-to-End ID(32)
//   AVP: Code(32) | Flags(8) V M P rrrrr | Length(24, header+data, NOT padding) | [Vendor-ID(32) if V] | Data | pad to 4 bytes
// ============================================================================================
constexpr uint8_t kDiaR = 0x80, kDiaP = 0x40, kDiaE = 0x20, kDiaT = 0x10;
constexpr uint8_t kAvpV = 0x80, kAvpM = 0x40, kAvpP = 0x20;

struct DiameterHeader {
    uint8_t version = 0; uint32_t length = 0; uint8_t flags = 0; uint32_t command = 0;
    uint32_t app_id = 0, hop_by_hop = 0, end_to_end = 0;
    bool request() const { return flags & kDiaR; }
};
struct Avp { uint32_t code = 0; uint8_t flags = 0; uint32_t vendor = 0; Bytes data; };

namespace detail {
inline uint32_t be32(const uint8_t* p) { return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | p[3]; }
inline uint32_t be24(const uint8_t* p) { return (uint32_t{p[0]} << 16) | (uint32_t{p[1]} << 8) | p[2]; }
inline void put32(Bytes& b, uint32_t v) { for (int s = 24; s >= 0; s -= 8) b.push_back(static_cast<uint8_t>(v >> s)); }
inline void put24(Bytes& b, uint32_t v) { for (int s = 16; s >= 0; s -= 8) b.push_back(static_cast<uint8_t>(v >> s)); }
}  // namespace detail

inline std::optional<DiameterHeader> parse_diameter_header(const uint8_t* p, size_t n) {
    if (n < 20) return std::nullopt;
    DiameterHeader h;
    h.version = p[0];
    h.length = detail::be24(p + 1);
    h.flags = p[4];
    h.command = detail::be24(p + 5);
    h.app_id = detail::be32(p + 8);
    h.hop_by_hop = detail::be32(p + 12);
    h.end_to_end = detail::be32(p + 16);
    if (h.version != 1 || h.length < 20 || (h.length & 3) != 0 || h.length > n) return std::nullopt;
    return h;
}

// Parse a run of AVPs (also used for the body of a Grouped AVP). nullopt on any malformed AVP.
inline std::optional<std::vector<Avp>> parse_avps(const uint8_t* p, size_t n) {
    std::vector<Avp> out;
    size_t pos = 0;
    while (pos < n) {
        if (n - pos < 8) return std::nullopt;
        Avp a;
        a.code = detail::be32(p + pos);
        a.flags = p[pos + 4];
        size_t len = detail::be24(p + pos + 5);
        size_t hdr = (a.flags & kAvpV) ? 12 : 8;
        if (len < hdr || len > n - pos) return std::nullopt;
        if (a.flags & kAvpV) a.vendor = detail::be32(p + pos + 8);
        a.data.assign(p + pos + hdr, p + pos + len);
        size_t padded = (len + 3) & ~size_t{3};
        if (padded > n - pos) {
            // the last AVP may be missing its padding only if it ends exactly at n: be strict - padding must be present
            return std::nullopt;
        }
        pos += padded;
        out.push_back(std::move(a));
    }
    return out;
}

inline std::optional<std::vector<Avp>> parse_diameter_avps(const uint8_t* msg, size_t n, const DiameterHeader& h) {
    if (h.length > n) return std::nullopt;
    return parse_avps(msg + 20, h.length - 20);
}

inline Bytes build_avp(uint32_t code, uint8_t flags, const Bytes& data, uint32_t vendor = 0) {
    Bytes b;
    bool v = flags & kAvpV;
    detail::put32(b, code);
    b.push_back(flags);
    detail::put24(b, static_cast<uint32_t>((v ? 12 : 8) + data.size()));
    if (v) detail::put32(b, vendor);
    b.insert(b.end(), data.begin(), data.end());
    while (b.size() % 4) b.push_back(0);
    return b;
}
inline Bytes avp_u32(uint32_t code, uint8_t flags, uint32_t value, uint32_t vendor = 0) {
    Bytes d; detail::put32(d, value); return build_avp(code, flags, d, vendor);
}
inline Bytes avp_utf8(uint32_t code, uint8_t flags, const std::string& s, uint32_t vendor = 0) {
    return build_avp(code, flags, Bytes(s.begin(), s.end()), vendor);
}
inline std::optional<uint32_t> avp_as_u32(const Avp& a) { return a.data.size() == 4 ? std::optional<uint32_t>(detail::be32(a.data.data())) : std::nullopt; }
inline std::string avp_as_string(const Avp& a) { return std::string(a.data.begin(), a.data.end()); }

inline Bytes build_diameter(uint32_t command, bool request, uint32_t app_id, uint32_t hbh, uint32_t e2e, const std::vector<Bytes>& avps, bool proxiable = true) {
    size_t body = 0;
    for (const auto& a : avps) body += a.size();
    Bytes b;
    b.push_back(1);
    detail::put24(b, static_cast<uint32_t>(20 + body));
    b.push_back(static_cast<uint8_t>((request ? kDiaR : 0) | (proxiable ? kDiaP : 0)));
    detail::put24(b, command);
    detail::put32(b, app_id); detail::put32(b, hbh); detail::put32(b, e2e);
    for (const auto& a : avps) b.insert(b.end(), a.begin(), a.end());
    return b;
}

inline const char* diameter_command_base(uint32_t code) {
    switch (code) {
        case 257: return "Capabilities-Exchange";
        case 258: return "Re-Auth";
        case 271: return "Accounting";
        case 272: return "Credit-Control";
        case 274: return "Abort-Session";
        case 275: return "Session-Termination";
        case 280: return "Device-Watchdog";
        case 282: return "Disconnect-Peer";
        case 316: return "Update-Location";               // S6a (app 16777251) - VERIFY
        case 318: return "Authentication-Information";    // S6a - VERIFY
        default: return "Unknown";
    }
}
inline std::string diameter_command_name(uint32_t code, bool request) {
    return std::string(diameter_command_base(code)) + (request ? "-Request" : "-Answer");
}

// ============================================================================================
// PART 3 - bit packing and ASN.1 UNALIGNED PER (X.691 UPER) basics. LTE RRC uses UPER; NGAP/S1AP use ALIGNED PER (VERIFY).
// ============================================================================================
class BitWriter {
public:
    // Append the low 'nbits' of value, most-significant bit first. nbits in [0,64].
    void put(uint64_t value, unsigned nbits) {
        for (unsigned i = nbits; i-- > 0;) {
            if (bit_ % 8 == 0) buf_.push_back(0);
            if ((value >> i) & 1) buf_.back() |= static_cast<uint8_t>(0x80 >> (bit_ % 8));
            ++bit_;
        }
    }
    size_t bit_count() const { return bit_; }
    const Bytes& bytes() const { return buf_; }      // last byte zero-padded
private:
    Bytes buf_; size_t bit_ = 0;
};

class BitReader {
public:
    BitReader(const uint8_t* p, size_t n) : p_(p), nbits_(n * 8) {}
    // nullopt if fewer than nbits remain.
    std::optional<uint64_t> get(unsigned nbits) {
        if (nbits > nbits_ - pos_) return std::nullopt;
        uint64_t v = 0;
        for (unsigned i = 0; i < nbits; ++i, ++pos_) v = (v << 1) | ((p_[pos_ / 8] >> (7 - pos_ % 8)) & 1);
        return v;
    }
    size_t bits_left() const { return nbits_ - pos_; }
private:
    const uint8_t* p_; size_t nbits_; size_t pos_ = 0;
};

// ceil(log2(range)) for range >= 1:  1 -> 0, 2 -> 1, 3,4 -> 2, 5..8 -> 3, 256 -> 8, 257 -> 9
inline unsigned ceil_log2(uint64_t range) {
    unsigned b = 0;
    while (b < 64 && (uint64_t{1} << b) < range) ++b;
    return b;
}
// Constrained whole number INTEGER (lb..ub): (value - lb) in ceil(log2(ub-lb+1)) bits. A single-value range takes 0 bits.
inline bool uper_put_constrained(BitWriter& w, int64_t value, int64_t lb, int64_t ub) {
    if (ub < lb || value < lb || value > ub) return false;
    uint64_t range = static_cast<uint64_t>(ub) - static_cast<uint64_t>(lb) + 1;
    w.put(static_cast<uint64_t>(value) - static_cast<uint64_t>(lb), range ? ceil_log2(range) : 64);   // range==0 <=> 2^64 values
    return true;
}
inline std::optional<int64_t> uper_get_constrained(BitReader& r, int64_t lb, int64_t ub) {
    if (ub < lb) return std::nullopt;
    uint64_t range = static_cast<uint64_t>(ub) - static_cast<uint64_t>(lb) + 1;
    auto v = r.get(range ? ceil_log2(range) : 64);
    if (!v || (range && *v >= range)) return std::nullopt;                         // value outside the declared range = malformed
    return static_cast<int64_t>(static_cast<uint64_t>(lb) + *v);
}
// Unconstrained length determinant (X.691 11.9), lengths < 16384: 0nnnnnnn  or  10nnnnnn nnnnnnnn.
inline bool uper_put_length(BitWriter& w, size_t n) {
    if (n < 128) { w.put(n, 8); return true; }
    if (n < 16384) { w.put(0x8000u | n, 16); return true; }
    return false;
}
inline std::optional<size_t> uper_get_length(BitReader& r) {
    auto b = r.get(8);
    if (!b) return std::nullopt;
    if ((*b & 0x80) == 0) return static_cast<size_t>(*b);
    if ((*b & 0xC0) == 0x80) { auto c = r.get(8); if (!c) return std::nullopt; return static_cast<size_t>(((*b & 0x3F) << 8) | *c); }
    return std::nullopt;                                                  // 11xxxxxx = fragmentation: not supported here
}
// OCTET STRING (SIZE(lb..ub)) with ub < 65536: length-lb in ceil(log2(ub-lb+1)) bits, then the bytes packed with NO alignment padding.
inline bool uper_put_octets(BitWriter& w, const Bytes& d, size_t lb, size_t ub) {
    if (d.size() < lb || d.size() > ub || ub >= 65536) return false;
    if (ub != lb) w.put(d.size() - lb, ceil_log2(ub - lb + 1));
    for (uint8_t c : d) w.put(c, 8);
    return true;
}
inline std::optional<Bytes> uper_get_octets(BitReader& r, size_t lb, size_t ub) {
    if (ub < lb || ub >= 65536) return std::nullopt;
    size_t n = lb;
    if (ub != lb) {
        auto v = r.get(ceil_log2(ub - lb + 1));
        if (!v) return std::nullopt;
        n = lb + *v;
        if (n > ub) return std::nullopt;
    }
    if (r.bits_left() < n * 8) return std::nullopt;
    Bytes out;
    for (size_t i = 0; i < n; ++i) out.push_back(static_cast<uint8_t>(*r.get(8)));
    return out;
}

// Worked example used by the worksheet:
//   Msg ::= SEQUENCE { a INTEGER (0..7), b BOOLEAN, c INTEGER (1000..1999) OPTIONAL, d ENUMERATED { x, y, z } }
// UPER layout: [preamble: 1 bit presence of c] a(3) b(1) [c(10)] d(2)
struct DemoMsg { int a = 0; bool b = false; std::optional<int> c; int d = 0; };
inline Bytes encode_demo(const DemoMsg& m) {
    BitWriter w;
    w.put(m.c ? 1 : 0, 1);
    uper_put_constrained(w, m.a, 0, 7);
    w.put(m.b ? 1 : 0, 1);
    if (m.c) uper_put_constrained(w, *m.c, 1000, 1999);
    uper_put_constrained(w, m.d, 0, 2);
    return w.bytes();
}
inline std::optional<DemoMsg> decode_demo(const uint8_t* p, size_t n) {
    BitReader r(p, n);
    DemoMsg m;
    auto pre = r.get(1); if (!pre) return std::nullopt;
    auto a = uper_get_constrained(r, 0, 7); if (!a) return std::nullopt;
    auto b = r.get(1); if (!b) return std::nullopt;
    m.a = static_cast<int>(*a); m.b = *b;
    if (*pre) { auto c = uper_get_constrained(r, 1000, 1999); if (!c) return std::nullopt; m.c = static_cast<int>(*c); }
    auto d = uper_get_constrained(r, 0, 2); if (!d) return std::nullopt;
    m.d = static_cast<int>(*d);
    return m;
}

}  // namespace ether
