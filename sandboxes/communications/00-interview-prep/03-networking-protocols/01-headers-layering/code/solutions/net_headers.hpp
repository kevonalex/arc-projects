// Reference solution - 03-net/01-headers-layering
// Header-only parsers/builders for Ethernet II (+802.1Q), IPv4, IPv6, UDP, TCP, SCTP.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ether {

using Bytes = std::vector<uint8_t>;

// ---- big-endian ("network order") helpers: explicit shifts, never reinterpret_cast ----
inline uint16_t rd16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }
inline uint32_t rd32(const uint8_t* p) {
    return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | uint32_t{p[3]};
}
inline void wr16(uint8_t* p, uint16_t v) { p[0] = static_cast<uint8_t>(v >> 8); p[1] = static_cast<uint8_t>(v); }
inline void wr32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v >> 24); p[1] = static_cast<uint8_t>(v >> 16);
    p[2] = static_cast<uint8_t>(v >> 8);  p[3] = static_cast<uint8_t>(v);
}

// ---- RFC 1071 Internet checksum: one's-complement sum of 16-bit words, then complemented ----
// Property used everywhere: if the checksum field is INCLUDED in the data, a valid packet sums to 0.
inline uint16_t internet_checksum(const uint8_t* data, size_t len, uint32_t initial = 0) {
    uint32_t sum = initial;
    size_t i = 0;
    for (; i + 1 < len; i += 2) sum += rd16(data + i);
    if (i < len) sum += uint32_t{data[i]} << 8;     // odd trailing byte is padded with a zero LOW byte
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);  // fold carries back in (end-around carry)
    return static_cast<uint16_t>(~sum);
}

// ---- Ethernet II (+ one optional 802.1Q tag) ----
struct EthHeader {
    std::array<uint8_t, 6> dst{}, src{};
    uint16_t ethertype = 0;       // 0x0800 IPv4, 0x86DD IPv6, 0x0806 ARP
    bool has_vlan = false;
    uint16_t vlan_tci = 0;        // PCP(3) | DEI(1) | VID(12)
    size_t header_len = 14;
};
inline std::optional<EthHeader> parse_ethernet(const uint8_t* p, size_t n) {
    if (n < 14) return std::nullopt;
    EthHeader h;
    for (int i = 0; i < 6; ++i) { h.dst[i] = p[i]; h.src[i] = p[6 + i]; }
    uint16_t t = rd16(p + 12);
    if (t == 0x8100) {                       // 802.1Q: TPID then TCI then the real EtherType
        if (n < 18) return std::nullopt;
        h.has_vlan = true; h.vlan_tci = rd16(p + 14); h.ethertype = rd16(p + 16); h.header_len = 18;
    } else {
        h.ethertype = t;
    }
    return h;
}

// ---- IPv4 (RFC 791) ----
struct Ipv4Header {
    uint8_t version = 4, ihl = 5;        // ihl counts 32-bit words
    uint8_t dscp = 0, ecn = 0;
    uint16_t total_length = 0, id = 0;
    bool df = false, mf = false;
    uint16_t frag_offset = 0;            // in 8-byte units
    uint8_t ttl = 64, protocol = 0;
    uint16_t checksum = 0;
    uint32_t src = 0, dst = 0;
    size_t header_len() const { return size_t{ihl} * 4; }
};
// Returns nullopt for: short buffer, version != 4, ihl < 5, buffer shorter than ihl*4,
// or total_length not in [ihl*4, n].
inline std::optional<Ipv4Header> parse_ipv4(const uint8_t* p, size_t n) {
    if (n < 20) return std::nullopt;
    Ipv4Header h;
    h.version = p[0] >> 4; h.ihl = p[0] & 0x0F;
    if (h.version != 4 || h.ihl < 5 || n < h.header_len()) return std::nullopt;
    h.dscp = p[1] >> 2; h.ecn = p[1] & 3;
    h.total_length = rd16(p + 2);
    if (h.total_length < h.header_len() || h.total_length > n) return std::nullopt;
    h.id = rd16(p + 4);
    uint16_t ff = rd16(p + 6);
    h.df = ff & 0x4000; h.mf = ff & 0x2000; h.frag_offset = ff & 0x1FFF;
    h.ttl = p[8]; h.protocol = p[9]; h.checksum = rd16(p + 10);
    h.src = rd32(p + 12); h.dst = rd32(p + 16);
    return h;
}
inline bool ipv4_checksum_ok(const uint8_t* p, size_t hdr_len) { return internet_checksum(p, hdr_len) == 0; }
// Builds a 20-byte header (no options) and fills in the correct checksum.
inline Bytes build_ipv4(const Ipv4Header& h) {
    Bytes b(20);
    b[0] = static_cast<uint8_t>((4 << 4) | 5);
    b[1] = static_cast<uint8_t>((h.dscp << 2) | (h.ecn & 3));
    wr16(&b[2], h.total_length); wr16(&b[4], h.id);
    wr16(&b[6], static_cast<uint16_t>((h.df ? 0x4000 : 0) | (h.mf ? 0x2000 : 0) | (h.frag_offset & 0x1FFF)));
    b[8] = h.ttl; b[9] = h.protocol; wr16(&b[10], 0);
    wr32(&b[12], h.src); wr32(&b[16], h.dst);
    wr16(&b[10], internet_checksum(b.data(), 20));
    return b;
}

// ---- IPv6 (RFC 8200): fixed 40 bytes, no checksum, no fragmentation field in the base header ----
struct Ipv6Header {
    uint8_t traffic_class = 0;           // DSCP(6)|ECN(2), same meaning as IPv4 TOS
    uint32_t flow_label = 0;             // 20 bits
    uint16_t payload_length = 0;         // bytes AFTER the 40-byte header
    uint8_t next_header = 0, hop_limit = 64;
    std::array<uint8_t, 16> src{}, dst{};
};
inline std::optional<Ipv6Header> parse_ipv6(const uint8_t* p, size_t n) {
    if (n < 40 || (p[0] >> 4) != 6) return std::nullopt;
    Ipv6Header h;
    uint32_t w = rd32(p);
    h.traffic_class = static_cast<uint8_t>((w >> 20) & 0xFF);
    h.flow_label = w & 0xFFFFF;
    h.payload_length = rd16(p + 4); h.next_header = p[6]; h.hop_limit = p[7];
    for (int i = 0; i < 16; ++i) { h.src[i] = p[8 + i]; h.dst[i] = p[24 + i]; }
    if (size_t{h.payload_length} + 40 > n) return std::nullopt;
    return h;
}

// ---- UDP (RFC 768): 8 bytes ----
struct UdpHeader { uint16_t src_port = 0, dst_port = 0, length = 0, checksum = 0; };
inline std::optional<UdpHeader> parse_udp(const uint8_t* p, size_t n) {
    if (n < 8) return std::nullopt;
    UdpHeader h{rd16(p), rd16(p + 2), rd16(p + 4), rd16(p + 6)};
    if (h.length < 8 || h.length > n) return std::nullopt;
    return h;
}
// IPv4 pseudo-header = src(4) dst(4) zero(1) proto(1) udp_length(2). 'seg' is the whole UDP datagram
// with its checksum field set to 0. A computed 0 is transmitted as 0xFFFF (0 means "no checksum" in IPv4).
inline uint16_t udp_checksum_v4(uint32_t src, uint32_t dst, const uint8_t* seg, size_t len) {
    uint8_t ph[12];
    wr32(ph, src); wr32(ph + 4, dst); ph[8] = 0; ph[9] = 17; wr16(ph + 10, static_cast<uint16_t>(len));
    // checksum over pseudo-header then segment; pass pseudo-header sum as 'initial' by folding it first
    uint32_t sum = 0;
    for (int i = 0; i < 12; i += 2) sum += rd16(ph + i);
    uint16_t c = internet_checksum(seg, len, sum);
    return c == 0 ? 0xFFFF : c;
}

// ---- TCP (RFC 9293) ----
struct TcpFlags { bool cwr, ece, urg, ack, psh, rst, syn, fin; };
struct TcpHeader {
    uint16_t src_port = 0, dst_port = 0;
    uint32_t seq = 0, ack = 0;
    uint8_t data_offset = 5;             // 32-bit words
    TcpFlags flags{};
    uint16_t window = 0, checksum = 0, urgent = 0;
    size_t header_len() const { return size_t{data_offset} * 4; }
};
inline std::optional<TcpHeader> parse_tcp(const uint8_t* p, size_t n) {
    if (n < 20) return std::nullopt;
    TcpHeader h;
    h.src_port = rd16(p); h.dst_port = rd16(p + 2); h.seq = rd32(p + 4); h.ack = rd32(p + 8);
    h.data_offset = p[12] >> 4;
    if (h.data_offset < 5 || n < h.header_len()) return std::nullopt;
    uint8_t f = p[13];
    h.flags = {bool(f & 0x80), bool(f & 0x40), bool(f & 0x20), bool(f & 0x10),
               bool(f & 0x08), bool(f & 0x04), bool(f & 0x02), bool(f & 0x01)};
    h.window = rd16(p + 14); h.checksum = rd16(p + 16); h.urgent = rd16(p + 18);
    return h;
}

// ---- SCTP (RFC 4960 / 9260): 12-byte common header, then chunks ----
struct SctpCommon { uint16_t src_port = 0, dst_port = 0; uint32_t verification_tag = 0, checksum = 0; };
struct SctpChunk  { uint8_t type = 0, flags = 0; uint16_t length = 0; };   // length includes the 4-byte chunk header
inline std::optional<SctpCommon> parse_sctp_common(const uint8_t* p, size_t n) {
    if (n < 12) return std::nullopt;
    return SctpCommon{rd16(p), rd16(p + 2), rd32(p + 4), rd32(p + 8)};
}
inline std::optional<SctpChunk> parse_sctp_chunk(const uint8_t* p, size_t n) {
    if (n < 4) return std::nullopt;
    SctpChunk c{p[0], p[1], rd16(p + 2)};
    if (c.length < 4 || c.length > n) return std::nullopt;
    return c;
}
// Chunks are padded to a 4-byte boundary on the wire but 'length' excludes the padding.
inline size_t sctp_padded_len(uint16_t length) { return (size_t{length} + 3) & ~size_t{3}; }

}  // namespace ether
