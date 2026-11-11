// pktheaders.h  --  STUBS for Pack 03 (student version).
// Implement BitReader, BitWriter and the RTP / IPv4 / UDP parsers. Reference: ../solutions/pktheaders.h
// Rules of the road (W 3.1): never reinterpret_cast the buffer to a struct; read fields explicitly,
// check every length BEFORE touching bytes, and never trust a length field you have not bounded.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pkt {

// ---------------------------------------------------------------- BitReader (W 3.2)
// MSB-first: the first bit read is the most significant bit (bit 7) of data[0].
class BitReader {
 public:
  BitReader(const uint8_t* data, size_t len_bytes) : data_(data), total_bits_(len_bytes * 8) {}
  // Read nbits (0..32) into out, MSB first. Return false (and consume nothing) if nbits > 32
  // or fewer than nbits bits remain.
  bool read([[maybe_unused]] unsigned nbits, [[maybe_unused]] uint32_t& out) { return false; }  // TODO W 3.2
  bool skip([[maybe_unused]] unsigned nbits) { return false; }                                  // TODO
  bool align_to_byte() { return false; }  // skip to the next byte boundary (no-op if aligned)  TODO
  size_t bits_remaining() const { return 0; }                                                   // TODO
  size_t bit_position() const { return 999; }                                                   // TODO

 private:
  [[maybe_unused]] const uint8_t* data_;
  [[maybe_unused]] size_t total_bits_;
  [[maybe_unused]] size_t pos_ = 0;
};

// ---------------------------------------------------------------- BitWriter (W 3.2)
// MSB-first. write(value, n) appends the low n bits of value (n in 0..32), most significant first.
// Unused trailing bits of the last byte are 0.
class BitWriter {
 public:
  void write([[maybe_unused]] uint32_t value, [[maybe_unused]] unsigned nbits) {}  // TODO W 3.2
  size_t bit_count() const { return 999; }                                          // TODO
  const std::vector<uint8_t>& bytes() const { return bytes_; }

 private:
  std::vector<uint8_t> bytes_;
};

// ---------------------------------------------------------------- RTP (RFC 3550 5.1)  (W 3.4)
struct RtpHeader {
  uint8_t version = 0;
  bool padding = false;
  bool extension = false;
  uint8_t csrc_count = 0;
  bool marker = false;
  uint8_t payload_type = 0;
  uint16_t seq = 0;
  uint32_t timestamp = 0;
  uint32_t ssrc = 0;
  size_t header_len = 0;   // fixed 12 + 4*CC + extension (if X)
  size_t payload_len = 0;  // bytes after the header, minus padding
};
// Validate and parse. Must reject: len < 12, version != 2, header (CSRC list, extension) running
// past len, padding count of 0 or larger than the bytes available.
inline bool parse_rtp([[maybe_unused]] const uint8_t* data, [[maybe_unused]] size_t len,
                      [[maybe_unused]] RtpHeader& out) {
  return false;  // TODO W 3.4
}
// Serialise the 12-byte fixed header (no CSRC list/extension).
inline std::vector<uint8_t> build_rtp([[maybe_unused]] const RtpHeader& h) {
  return {};  // TODO W 3.4
}

// ---------------------------------------------------------------- IPv4 (RFC 791 3.1)  (W 3.5)
struct Ipv4Header {
  uint8_t version = 0;
  uint8_t ihl = 0;  // header length in 32-bit words
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t total_length = 0;
  uint16_t identification = 0;
  bool flag_df = false;
  bool flag_mf = false;
  uint16_t frag_offset = 0;  // in 8-byte units
  uint8_t ttl = 0;
  uint8_t protocol = 0;
  uint16_t checksum = 0;
  uint32_t src = 0;
  uint32_t dst = 0;
  size_t header_len = 0;  // ihl * 4
};
// Must reject: len < 20, version != 4, ihl < 5, len < ihl*4, total_length < ihl*4,
// total_length > len (truncated capture). Trailing bytes beyond total_length are allowed
// (Ethernet padding). Does NOT verify the checksum (see Pack 04).
inline bool parse_ipv4([[maybe_unused]] const uint8_t* data, [[maybe_unused]] size_t len,
                       [[maybe_unused]] Ipv4Header& out) {
  return false;  // TODO W 3.5
}

// ---------------------------------------------------------------- UDP (RFC 768)  (W 3.6)
struct UdpHeader {
  uint16_t src_port = 0;
  uint16_t dst_port = 0;
  uint16_t length = 0;  // header + payload, bytes
  uint16_t checksum = 0;
};
// Must reject: len < 8, length < 8, length > len.
inline bool parse_udp([[maybe_unused]] const uint8_t* data, [[maybe_unused]] size_t len,
                      [[maybe_unused]] UdpHeader& out) {
  return false;  // TODO W 3.6
}
inline std::vector<uint8_t> build_udp([[maybe_unused]] const UdpHeader& h) {
  return {};  // TODO W 3.6
}

// ---------------------------------------------------------------- IPv4 + UDP + RTP chain (W 4)
struct Packet {
  Ipv4Header ip;
  UdpHeader udp;
  RtpHeader rtp;
  size_t rtp_offset = 0;  // from the start of the IP packet
};
// Parse IPv4 -> UDP -> RTP. Reject non-UDP (protocol != 17) and fragments (MF set or offset != 0).
// Each layer sees only the bytes the previous layer says belong to it.
inline bool parse_ip_udp_rtp([[maybe_unused]] const uint8_t* data, [[maybe_unused]] size_t len,
                             [[maybe_unused]] Packet& out) {
  return false;  // TODO W 4
}

}  // namespace pkt
