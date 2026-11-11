// STUB - 03-net/06-cellular-encodings. Implement every TODO. Tests: tests/test_cellenc.cpp
// Pure C++17, header-only. Everything is synthetic / textbook vectors: no live traffic is decoded.
// Read worksheet Parts B, C and D first; the byte layouts are drawn there.
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
// PART 1 - GTP-U (3GPP TS 29.281)
//   byte 0 : Version(3)=1 | PT(1)=1 | spare(1) | E(1) | S(1) | PN(1)
//   byte 1 : message type (0xFF G-PDU, 0x01 Echo Request, 0x02 Echo Response, 0x1A Error Indication, 0xFE End Marker)
//   byte 2-3: length = bytes AFTER the first 8        byte 4-7: TEID
//   if any of E,S,PN: 4 more bytes = sequence(16) | N-PDU(8) | next extension type(8)
//   extension: length(8, units of 4 bytes INCLUDING the length and next-type bytes) | content | next type(8)
// ============================================================================================
constexpr uint16_t kGtpuPort = 2152;
constexpr uint8_t kGtpuGPdu = 0xFF, kGtpuEchoReq = 0x01, kGtpuEchoResp = 0x02, kGtpuErrorInd = 0x1A, kGtpuEndMarker = 0xFE;
constexpr uint8_t kGtpuExtPduSessionContainer = 0x85;

struct GtpuExt { uint8_t type = 0; Bytes content; };       // content excludes the length byte and the next-type byte

struct GtpuPacket {
    uint8_t version = 0; bool pt = false; bool e = false, s = false, pn = false;
    uint8_t msg_type = 0; uint16_t length = 0; uint32_t teid = 0;
    uint16_t seq = 0; uint8_t npdu = 0;
    std::vector<GtpuExt> exts;
    size_t header_len = 0;      // 8, or 12 + extension bytes
    Bytes payload;
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

// TODO: reject (nullopt): <8 bytes, version != 1, PT != 1, 8+length > n, E/S/PN set but < 4 optional bytes inside 'length',
// an extension with length 0 or overrunning the packet, or more than 8 chained extensions.
// Bytes after 8+length are ignored. payload = bytes between header_len and 8+length.
inline std::optional<GtpuPacket> parse_gtpu(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }

// TODO: build a G-PDU. If 'seq' is given set S and add the 4 optional bytes. If 'exts' is non-empty set E; the chain's first type
// goes in the optional part's next-type byte, each extension's own last byte names the following type (0 at the end).
// Every extension content size + 2 must be a multiple of 4.
inline Bytes build_gtpu_gpdu(uint32_t teid, const Bytes& inner, std::optional<uint16_t> seq = std::nullopt,
                             const std::vector<GtpuExt>& exts = {}) {
    (void)teid; (void)inner; (void)seq; (void)exts; return {};
}

// TODO: outer IPv4 (20) + UDP (8) + GTP-U (8, or 12 with optional fields) + extension bytes
inline size_t gtpu_overhead(bool optional_fields = false, size_t ext_bytes = 0) { (void)optional_fields; (void)ext_bytes; return 0; }
// TODO: transport_mtu - overhead, never negative (return 0 if it does not fit)
inline size_t gtpu_inner_mtu(size_t transport_mtu, bool optional_fields = false, size_t ext_bytes = 0) {
    (void)transport_mtu; (void)optional_fields; (void)ext_bytes; return 0;
}

// ============================================================================================
// PART 2 - Diameter (RFC 6733)
//   header (20 B): Version(8)=1 | Length(24, whole message, multiple of 4) | Flags(8) R P E T rrrr | Command(24) |
//                  Application-ID(32) | Hop-by-Hop(32) | End-to-End(32)
//   AVP: Code(32) | Flags(8) V M P rrrrr | Length(24 = header+data, NOT padding) | [Vendor-ID(32) if V] | Data | pad to 4
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

// TODO: nullopt if n < 20, version != 1, length < 20, length not a multiple of 4, or length > n.
inline std::optional<DiameterHeader> parse_diameter_header(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }

// TODO: parse a run of AVPs (also used for the body of a Grouped AVP). The AVP length excludes padding, so advance by
// the length rounded UP to a multiple of 4 - and require that padding to be present. nullopt on: <8 bytes left,
// length < header size (8, or 12 with V), length > remaining, or missing padding. 'data' excludes header, vendor id and padding.
inline std::optional<std::vector<Avp>> parse_avps(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }

inline std::optional<std::vector<Avp>> parse_diameter_avps(const uint8_t* msg, size_t n, const DiameterHeader& h) {
    if (h.length > n) return std::nullopt;
    return parse_avps(msg + 20, h.length - 20);
}

// TODO: code | flags | 24-bit length (8, or 12 with V, plus data size - padding NOT included) | [vendor] | data | zero padding to 4
inline Bytes build_avp(uint32_t code, uint8_t flags, const Bytes& data, uint32_t vendor = 0) {
    (void)code; (void)flags; (void)data; (void)vendor; return {};
}
inline Bytes avp_u32(uint32_t code, uint8_t flags, uint32_t value, uint32_t vendor = 0) {
    Bytes d; detail::put32(d, value); return build_avp(code, flags, d, vendor);
}
inline Bytes avp_utf8(uint32_t code, uint8_t flags, const std::string& s, uint32_t vendor = 0) {
    return build_avp(code, flags, Bytes(s.begin(), s.end()), vendor);
}
inline std::optional<uint32_t> avp_as_u32(const Avp& a) { return a.data.size() == 4 ? std::optional<uint32_t>(detail::be32(a.data.data())) : std::nullopt; }
inline std::string avp_as_string(const Avp& a) { return std::string(a.data.begin(), a.data.end()); }

// TODO: 20-byte header (version 1, total length = 20 + all AVP bytes, flags R if request and P if proxiable) + AVPs.
inline Bytes build_diameter(uint32_t command, bool request, uint32_t app_id, uint32_t hbh, uint32_t e2e, const std::vector<Bytes>& avps, bool proxiable = true) {
    (void)command; (void)request; (void)app_id; (void)hbh; (void)e2e; (void)avps; (void)proxiable; return {};
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
        case 316: return "Update-Location";
        case 318: return "Authentication-Information";
        default: return "Unknown";
    }
}
inline std::string diameter_command_name(uint32_t code, bool request) {
    return std::string(diameter_command_base(code)) + (request ? "-Request" : "-Answer");
}

// ============================================================================================
// PART 3 - bit packing and ASN.1 UNALIGNED PER (X.691 UPER) basics
// ============================================================================================
class BitWriter {
public:
    // TODO: append the low 'nbits' of value, MOST-significant bit first (so bit 7 of byte 0 is written first). nbits in [0,64].
    void put(uint64_t value, unsigned nbits) { (void)value; (void)nbits; }
    size_t bit_count() const { return bit_; }
    const Bytes& bytes() const { return buf_; }      // last byte zero-padded
private:
    Bytes buf_; size_t bit_ = 0;
};

class BitReader {
public:
    BitReader(const uint8_t* p, size_t n) : p_(p), nbits_(n * 8) {}
    // TODO: nullopt if fewer than nbits remain (and consume nothing in that case)
    std::optional<uint64_t> get(unsigned nbits) { (void)nbits; return std::nullopt; }
    size_t bits_left() const { return nbits_ - pos_; }
private:
    const uint8_t* p_; size_t nbits_; size_t pos_ = 0;
};

// TODO: smallest b with 2^b >= range.  1 -> 0, 2 -> 1, 3,4 -> 2, 5..8 -> 3, 256 -> 8, 257 -> 9
inline unsigned ceil_log2(uint64_t range) { (void)range; return 99; }
// TODO: INTEGER (lb..ub): write (value-lb) in ceil_log2(ub-lb+1) bits; refuse (false, write nothing) if value outside or ub<lb.
// Careful: ub-lb+1 overflows to 0 for the full 64-bit range - that means 64 bits.
inline bool uper_put_constrained(BitWriter& w, int64_t value, int64_t lb, int64_t ub) { (void)w; (void)value; (void)lb; (void)ub; return false; }
// TODO: inverse; nullopt on truncation or if the decoded offset is >= range (illegal code point)
inline std::optional<int64_t> uper_get_constrained(BitReader& r, int64_t lb, int64_t ub) { (void)r; (void)lb; (void)ub; return std::nullopt; }
// TODO: unconstrained length determinant, n < 128: 0nnnnnnn (8 bits); n < 16384: 10nnnnnn nnnnnnnn (16 bits); else false.
inline bool uper_put_length(BitWriter& w, size_t n) { (void)w; (void)n; return false; }
// TODO: inverse. A first byte starting with 11 (fragmentation) is not supported -> nullopt.
inline std::optional<size_t> uper_get_length(BitReader& r) { (void)r; return std::nullopt; }
// TODO: OCTET STRING (SIZE(lb..ub)), ub < 65536: if lb != ub write size-lb in ceil_log2(ub-lb+1) bits, then each byte as 8 bits with NO padding.
inline bool uper_put_octets(BitWriter& w, const Bytes& d, size_t lb, size_t ub) { (void)w; (void)d; (void)lb; (void)ub; return false; }
inline std::optional<Bytes> uper_get_octets(BitReader& r, size_t lb, size_t ub) { (void)r; (void)lb; (void)ub; return std::nullopt; }

// Worked example (worksheet Part D):
//   Msg ::= SEQUENCE { a INTEGER (0..7), b BOOLEAN, c INTEGER (1000..1999) OPTIONAL, d ENUMERATED { x, y, z } }
// UPER layout: [preamble: 1 bit = c present] a(3) b(1) [c(10)] d(2)
struct DemoMsg { int a = 0; bool b = false; std::optional<int> c; int d = 0; };
inline Bytes encode_demo(const DemoMsg& m) { (void)m; return {}; }                                  // TODO
inline std::optional<DemoMsg> decode_demo(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }   // TODO

}  // namespace ether
