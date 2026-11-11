// STUB - 03-net/01-headers-layering. Implement every TODO, then run the tests.
// Rule: no reinterpret_cast of packet bytes onto structs; use explicit shifts (see worksheet Part B).
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ether {

using Bytes = std::vector<uint8_t>;

// TODO: big-endian read/write helpers
inline uint16_t rd16(const uint8_t* p) { (void)p; return 0; }
inline uint32_t rd32(const uint8_t* p) { (void)p; return 0; }
inline void wr16(uint8_t* p, uint16_t v) { (void)p; (void)v; }
inline void wr32(uint8_t* p, uint32_t v) { (void)p; (void)v; }

// TODO: RFC 1071 checksum (sum 16-bit big-endian words, pad odd byte with zero LOW byte,
// fold carries, complement). 'initial' lets the caller pre-load a pseudo-header sum.
inline uint16_t internet_checksum(const uint8_t* data, size_t len, uint32_t initial = 0) {
    (void)data; (void)len; (void)initial; return 0;
}

struct EthHeader {
    std::array<uint8_t, 6> dst{}, src{};
    uint16_t ethertype = 0;
    bool has_vlan = false;
    uint16_t vlan_tci = 0;
    size_t header_len = 14;
};
// TODO: handle the optional 802.1Q tag (TPID 0x8100 -> TCI -> real EtherType; header_len 18)
inline std::optional<EthHeader> parse_ethernet(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }

struct Ipv4Header {
    uint8_t version = 4, ihl = 5;
    uint8_t dscp = 0, ecn = 0;
    uint16_t total_length = 0, id = 0;
    bool df = false, mf = false;
    uint16_t frag_offset = 0;            // 8-byte units
    uint8_t ttl = 64, protocol = 0;
    uint16_t checksum = 0;
    uint32_t src = 0, dst = 0;
    size_t header_len() const { return size_t{ihl} * 4; }
};
// TODO: reject short buffers, version != 4, ihl < 5, n < ihl*4, total_length outside [ihl*4, n]
inline std::optional<Ipv4Header> parse_ipv4(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }
// TODO: a valid header (checksum field included) sums to 0
inline bool ipv4_checksum_ok(const uint8_t* p, size_t hdr_len) { (void)p; (void)hdr_len; return false; }
// TODO: 20-byte header, computed checksum
inline Bytes build_ipv4(const Ipv4Header& h) { (void)h; return {}; }

struct Ipv6Header {
    uint8_t traffic_class = 0;
    uint32_t flow_label = 0;
    uint16_t payload_length = 0;
    uint8_t next_header = 0, hop_limit = 64;
    std::array<uint8_t, 16> src{}, dst{};
};
// TODO: version 6, 40 bytes, payload_length must fit in the buffer
inline std::optional<Ipv6Header> parse_ipv6(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }

struct UdpHeader { uint16_t src_port = 0, dst_port = 0, length = 0, checksum = 0; };
// TODO: reject n < 8 and length outside [8, n]
inline std::optional<UdpHeader> parse_udp(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }
// TODO: IPv4 pseudo-header (src,dst,0,17,udp_len) + segment (checksum field zeroed); 0 -> 0xFFFF
inline uint16_t udp_checksum_v4(uint32_t src, uint32_t dst, const uint8_t* seg, size_t len) {
    (void)src; (void)dst; (void)seg; (void)len; return 0;
}

struct TcpFlags { bool cwr, ece, urg, ack, psh, rst, syn, fin; };
struct TcpHeader {
    uint16_t src_port = 0, dst_port = 0;
    uint32_t seq = 0, ack = 0;
    uint8_t data_offset = 5;
    TcpFlags flags{};
    uint16_t window = 0, checksum = 0, urgent = 0;
    size_t header_len() const { return size_t{data_offset} * 4; }
};
// TODO: flags byte is CWR ECE URG ACK PSH RST SYN FIN (MSB..LSB); data_offset >= 5
inline std::optional<TcpHeader> parse_tcp(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }

struct SctpCommon { uint16_t src_port = 0, dst_port = 0; uint32_t verification_tag = 0, checksum = 0; };
struct SctpChunk  { uint8_t type = 0, flags = 0; uint16_t length = 0; };
inline std::optional<SctpCommon> parse_sctp_common(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }
// TODO: length >= 4 and <= n
inline std::optional<SctpChunk> parse_sctp_chunk(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }
// TODO: round up to a multiple of 4
inline size_t sctp_padded_len(uint16_t length) { (void)length; return 0; }

}  // namespace ether
