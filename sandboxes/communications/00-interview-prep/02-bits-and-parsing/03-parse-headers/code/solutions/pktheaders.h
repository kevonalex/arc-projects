// pktheaders.h  --  REFERENCE SOLUTION for Pack 03.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pkt {

// ---------------------------------------------------------------- BitReader
class BitReader {
 public:
  BitReader(const uint8_t* data, size_t len_bytes) : data_(data), total_bits_(len_bytes * 8) {}

  bool read(unsigned nbits, uint32_t& out) {
    if (nbits > 32u || nbits > bits_remaining()) return false;
    uint32_t v = 0;
    for (unsigned i = 0; i < nbits; ++i) {
      const size_t p = pos_ + i;                               // absolute bit index, 0 = MSB of data[0]
      const unsigned bit = (data_[p >> 3] >> (7u - (p & 7u))) & 1u;  // bit 7 of the byte comes first
      v = (v << 1) | bit;                                      // append at the bottom
    }
    pos_ += nbits;
    out = v;
    return true;
  }
  bool skip(unsigned nbits) {
    if (nbits > bits_remaining()) return false;
    pos_ += nbits;
    return true;
  }
  bool align_to_byte() {
    const size_t rem = pos_ & 7u;
    if (rem == 0) return true;
    return skip(static_cast<unsigned>(8u - rem));
  }
  size_t bits_remaining() const { return total_bits_ - pos_; }
  size_t bit_position() const { return pos_; }

 private:
  const uint8_t* data_;
  size_t total_bits_;
  size_t pos_ = 0;
};

// ---------------------------------------------------------------- BitWriter
class BitWriter {
 public:
  void write(uint32_t value, unsigned nbits) {
    if (nbits > 32u) nbits = 32u;
    for (unsigned i = nbits; i-- > 0;) {  // MSB of the field first
      const unsigned bit = (value >> i) & 1u;
      if ((count_ & 7u) == 0) bytes_.push_back(0);
      bytes_.back() = static_cast<uint8_t>(bytes_.back() | (bit << (7u - (count_ & 7u))));
      ++count_;
    }
  }
  size_t bit_count() const { return count_; }
  const std::vector<uint8_t>& bytes() const { return bytes_; }

 private:
  std::vector<uint8_t> bytes_;
  size_t count_ = 0;
};

// ---------------------------------------------------------------- RTP
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
  size_t header_len = 0;
  size_t payload_len = 0;
};

inline bool parse_rtp(const uint8_t* d, size_t len, RtpHeader& out) {
  if (d == nullptr || len < 12) return false;  // check length BEFORE reading anything
  BitReader r(d, len);
  uint32_t v = 0, p = 0, x = 0, cc = 0, m = 0, pt = 0, seq = 0, ts = 0, ssrc = 0;
  r.read(2, v);
  r.read(1, p);
  r.read(1, x);
  r.read(4, cc);
  r.read(1, m);
  r.read(7, pt);
  r.read(16, seq);
  r.read(32, ts);
  r.read(32, ssrc);
  if (v != 2) return false;

  size_t hl = 12u + 4u * cc;  // CSRC list
  if (len < hl) return false;
  if (x != 0) {  // one extension header: 16-bit profile, 16-bit length in 32-bit words, then data
    if (len < hl + 4) return false;
    const size_t words = (static_cast<size_t>(d[hl + 2]) << 8) | d[hl + 3];
    hl += 4u + 4u * words;
    if (len < hl) return false;
  }
  size_t pad = 0;
  if (p != 0) {  // last octet = number of padding octets, counting itself
    pad = d[len - 1];
    if (pad == 0 || hl + pad > len) return false;
  }
  out.version = static_cast<uint8_t>(v);
  out.padding = p != 0;
  out.extension = x != 0;
  out.csrc_count = static_cast<uint8_t>(cc);
  out.marker = m != 0;
  out.payload_type = static_cast<uint8_t>(pt);
  out.seq = static_cast<uint16_t>(seq);
  out.timestamp = ts;
  out.ssrc = ssrc;
  out.header_len = hl;
  out.payload_len = len - hl - pad;
  return true;
}

inline std::vector<uint8_t> build_rtp(const RtpHeader& h) {
  BitWriter w;
  w.write(h.version, 2);
  w.write(h.padding ? 1u : 0u, 1);
  w.write(h.extension ? 1u : 0u, 1);
  w.write(h.csrc_count, 4);
  w.write(h.marker ? 1u : 0u, 1);
  w.write(h.payload_type, 7);
  w.write(h.seq, 16);
  w.write(h.timestamp, 32);
  w.write(h.ssrc, 32);
  return w.bytes();
}

// ---------------------------------------------------------------- IPv4
struct Ipv4Header {
  uint8_t version = 0;
  uint8_t ihl = 0;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t total_length = 0;
  uint16_t identification = 0;
  bool flag_df = false;
  bool flag_mf = false;
  uint16_t frag_offset = 0;
  uint8_t ttl = 0;
  uint8_t protocol = 0;
  uint16_t checksum = 0;
  uint32_t src = 0;
  uint32_t dst = 0;
  size_t header_len = 0;
};

inline bool parse_ipv4(const uint8_t* d, size_t len, Ipv4Header& out) {
  if (d == nullptr || len < 20) return false;
  BitReader r(d, len);
  uint32_t ver = 0, ihl = 0, dscp = 0, ecn = 0, tl = 0, id = 0, rsv = 0, df = 0, mf = 0, fo = 0;
  uint32_t ttl = 0, proto = 0, ck = 0, src = 0, dst = 0;
  r.read(4, ver);
  r.read(4, ihl);
  r.read(6, dscp);
  r.read(2, ecn);
  r.read(16, tl);
  r.read(16, id);
  r.read(1, rsv);  // reserved flag, must be 0; ignored
  r.read(1, df);
  r.read(1, mf);
  r.read(13, fo);
  r.read(8, ttl);
  r.read(8, proto);
  r.read(16, ck);
  r.read(32, src);
  r.read(32, dst);
  if (ver != 4 || ihl < 5) return false;
  const size_t hl = static_cast<size_t>(ihl) * 4u;
  if (len < hl || tl < hl || tl > len) return false;
  out.version = static_cast<uint8_t>(ver);
  out.ihl = static_cast<uint8_t>(ihl);
  out.dscp = static_cast<uint8_t>(dscp);
  out.ecn = static_cast<uint8_t>(ecn);
  out.total_length = static_cast<uint16_t>(tl);
  out.identification = static_cast<uint16_t>(id);
  out.flag_df = df != 0;
  out.flag_mf = mf != 0;
  out.frag_offset = static_cast<uint16_t>(fo);
  out.ttl = static_cast<uint8_t>(ttl);
  out.protocol = static_cast<uint8_t>(proto);
  out.checksum = static_cast<uint16_t>(ck);
  out.src = src;
  out.dst = dst;
  out.header_len = hl;
  return true;
}

// ---------------------------------------------------------------- UDP
struct UdpHeader {
  uint16_t src_port = 0;
  uint16_t dst_port = 0;
  uint16_t length = 0;
  uint16_t checksum = 0;
};

inline bool parse_udp(const uint8_t* d, size_t len, UdpHeader& out) {
  if (d == nullptr || len < 8) return false;
  BitReader r(d, len);
  uint32_t sp = 0, dp = 0, l = 0, ck = 0;
  r.read(16, sp);
  r.read(16, dp);
  r.read(16, l);
  r.read(16, ck);
  if (l < 8 || l > len) return false;
  out.src_port = static_cast<uint16_t>(sp);
  out.dst_port = static_cast<uint16_t>(dp);
  out.length = static_cast<uint16_t>(l);
  out.checksum = static_cast<uint16_t>(ck);
  return true;
}

inline std::vector<uint8_t> build_udp(const UdpHeader& h) {
  BitWriter w;
  w.write(h.src_port, 16);
  w.write(h.dst_port, 16);
  w.write(h.length, 16);
  w.write(h.checksum, 16);
  return w.bytes();
}

// ---------------------------------------------------------------- chain
struct Packet {
  Ipv4Header ip;
  UdpHeader udp;
  RtpHeader rtp;
  size_t rtp_offset = 0;
};

inline bool parse_ip_udp_rtp(const uint8_t* d, size_t len, Packet& out) {
  Packet p;
  if (!parse_ipv4(d, len, p.ip)) return false;
  if (p.ip.protocol != 17) return false;
  if (p.ip.flag_mf || p.ip.frag_offset != 0) return false;  // only unfragmented datagrams
  const size_t ip_payload = p.ip.total_length - p.ip.header_len;  // IP says how much is ours
  if (!parse_udp(d + p.ip.header_len, ip_payload, p.udp)) return false;
  const size_t udp_payload = p.udp.length - 8u;
  p.rtp_offset = p.ip.header_len + 8u;
  if (!parse_rtp(d + p.rtp_offset, udp_payload, p.rtp)) return false;
  out = p;
  return true;
}

}  // namespace pkt
