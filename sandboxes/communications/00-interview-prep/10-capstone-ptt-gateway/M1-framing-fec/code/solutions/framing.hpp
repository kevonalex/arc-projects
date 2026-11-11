// framing.hpp  --  REFERENCE SOLUTION for Capstone M1 (frame layout, CRC-16, Golay header, Hamming payload).
//
// This is OUR toy over-the-air format, not P25/DMR/TETRA.  Frame (one 20 ms voice frame), MSB first, one bit per element:
//   [ sync 24 | Golay(23,12) x2 = header 46 | Hamming(7,4) over (payload 160 B + CRC-16 2 B) = 2268 ]  = 2338 bits
// The CRC covers the 3 header bytes AND the payload, so a header the Golay decoder "corrected" wrongly is still caught.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gw {

using Bits = std::vector<uint8_t>;  // each element is 0 or 1

constexpr size_t kPayloadBytes = 160;  // 20 ms of 8 kHz mu-law (stand-in for a vocoder frame; see worksheet)
constexpr int kSyncBits = 24;
constexpr uint32_t kSyncWord = 0x02B31E;  // worst aperiodic autocorrelation sidelobe = 3 (found by exhaustive search)
constexpr int kSyncTolerance = 3;         // accept up to this many mismatching sync bits
constexpr size_t kHeaderBits = 46;
constexpr size_t kBodyBytes = kPayloadBytes + 2;
constexpr size_t kBodyBits = kBodyBytes * 2 * 7;
constexpr size_t kFrameBits = kSyncBits + kHeaderBits + kBodyBits;  // 2338

// ---- given helpers (not stubs): bit-vector plumbing ----
inline void put_bits(Bits& b, uint32_t value, int nbits) {
  for (int i = nbits - 1; i >= 0; --i) b.push_back(static_cast<uint8_t>((value >> i) & 1u));
}
inline uint32_t get_bits(const Bits& b, size_t pos, int nbits) {
  uint32_t v = 0;
  for (int i = 0; i < nbits; ++i) v = (v << 1) | (b[pos + static_cast<size_t>(i)] & 1u);
  return v;
}

// ------------------------------------------------------------------ CRC-16/CCITT-FALSE (W 3.1)
// poly 0x1021, init 0xFFFF, no reflection, no final XOR.  crc16("123456789") == 0x29B1.
inline uint16_t crc16_ccitt(const uint8_t* d, size_t n) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < n; ++i) {
    crc = static_cast<uint16_t>(crc ^ (static_cast<uint16_t>(d[i]) << 8));
    for (int k = 0; k < 8; ++k) crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}

// ------------------------------------------------------------------ Golay(23,12,7) (W 3.2)
// Systematic: codeword = (data12 << 11) | ((data12 << 11) mod g), g = x^11+x^10+x^6+x^5+x^4+x^2+1 = 0xC75.
inline uint32_t golay23_encode(uint16_t d12) {
  const uint32_t v = static_cast<uint32_t>(d12 & 0xFFFu) << 11;
  uint32_t rem = v;
  for (int i = 22; i >= 11; --i)
    if ((rem >> i) & 1u) rem ^= (0xC75u << (i - 11));
  return v | (rem & 0x7FFu);
}
struct GolayDecode {
  uint16_t data;
  int errors;  // bits corrected (0..3).  23-bit Golay is PERFECT: it always "corrects" - 4+ errors miscorrect silently.
};
inline GolayDecode golay23_decode(uint32_t rx) {
  static const std::array<uint32_t, 4096> table = [] {
    std::array<uint32_t, 4096> t{};
    for (uint32_t d = 0; d < 4096; ++d) t[d] = golay23_encode(static_cast<uint16_t>(d));
    return t;
  }();
  int best = 99;
  uint16_t best_d = 0;
  for (uint32_t d = 0; d < 4096; ++d) {
    int dist = 0;
    for (uint32_t x = (rx ^ table[d]) & 0x7FFFFFu; x; x &= x - 1) ++dist;
    if (dist < best) {
      best = dist;
      best_d = static_cast<uint16_t>(d);
    }
  }
  return {best_d, best};
}

// ------------------------------------------------------------------ Hamming(7,4) (W 3.3)
// Position k (1-based) is bit k-1.  Positions: p1 p2 d1 p4 d2 d3 d4.  Nibble bit0..3 -> positions 3,5,6,7.
inline uint8_t hamming74_encode(uint8_t nib) {
  const unsigned d1 = nib & 1u, d2 = (nib >> 1) & 1u, d3 = (nib >> 2) & 1u, d4 = (nib >> 3) & 1u;
  const unsigned p1 = d1 ^ d2 ^ d4, p2 = d1 ^ d3 ^ d4, p4 = d2 ^ d3 ^ d4;
  return static_cast<uint8_t>(p1 | (p2 << 1) | (d1 << 2) | (p4 << 3) | (d2 << 4) | (d3 << 5) | (d4 << 6));
}
struct Ham74Decode {
  uint8_t nibble;
  bool corrected;
};
inline Ham74Decode hamming74_decode(uint8_t cw) {
  auto b = [&](int pos) { return (cw >> (pos - 1)) & 1u; };
  const unsigned s1 = b(1) ^ b(3) ^ b(5) ^ b(7), s2 = b(2) ^ b(3) ^ b(6) ^ b(7), s4 = b(4) ^ b(5) ^ b(6) ^ b(7);
  const unsigned syn = s1 | (s2 << 1) | (s4 << 2);  // = position of the single wrong bit, 0 = none
  if (syn) cw = static_cast<uint8_t>(cw ^ (1u << (syn - 1)));
  const unsigned nib = ((cw >> 2) & 1u) | (((cw >> 4) & 1u) << 1) | (((cw >> 5) & 1u) << 2) | (((cw >> 6) & 1u) << 3);
  return {static_cast<uint8_t>(nib), syn != 0};
}

// ------------------------------------------------------------------ header + frame (W 3.4 - 3.5)
struct Header {
  uint8_t version = 1;  // 2 bits
  uint8_t type = 0;     // 2 bits: 0 voice, 1 end-of-call, 2 data, 3 reserved
  uint8_t talkgroup = 0;  // 8 bits
  uint16_t seq = 0;       // 12 bits: frame counter (20 ms * 4096 = 81.9 s before wrap)
};
inline bool operator==(const Header& a, const Header& b) {
  return a.version == b.version && a.type == b.type && a.talkgroup == b.talkgroup && a.seq == b.seq;
}
// 24-bit header word: version(2) type(2) talkgroup(8) seq(12), MSB first.
inline uint32_t header_to_word24(const Header& h) {
  return (static_cast<uint32_t>(h.version & 3u) << 22) | (static_cast<uint32_t>(h.type & 3u) << 20) |
         (static_cast<uint32_t>(h.talkgroup) << 12) | (h.seq & 0xFFFu);
}
inline Header word24_to_header(uint32_t w) {
  Header h;
  h.version = static_cast<uint8_t>((w >> 22) & 3u);
  h.type = static_cast<uint8_t>((w >> 20) & 3u);
  h.talkgroup = static_cast<uint8_t>((w >> 12) & 0xFFu);
  h.seq = static_cast<uint16_t>(w & 0xFFFu);
  return h;
}
// CRC over the 3 header bytes followed by the payload.
inline uint16_t frame_crc(const Header& h, const uint8_t* payload) {
  std::vector<uint8_t> v;
  const uint32_t w = header_to_word24(h);
  v.push_back(static_cast<uint8_t>(w >> 16));
  v.push_back(static_cast<uint8_t>(w >> 8));
  v.push_back(static_cast<uint8_t>(w));
  v.insert(v.end(), payload, payload + kPayloadBytes);
  return crc16_ccitt(v.data(), v.size());
}
inline Bits build_frame_bits(const Header& h, const uint8_t* payload) {
  Bits b;
  b.reserve(kFrameBits);
  put_bits(b, kSyncWord, kSyncBits);
  const uint32_t w = header_to_word24(h);
  put_bits(b, golay23_encode(static_cast<uint16_t>(w >> 12)), 23);
  put_bits(b, golay23_encode(static_cast<uint16_t>(w & 0xFFFu)), 23);
  std::vector<uint8_t> body(payload, payload + kPayloadBytes);
  const uint16_t crc = frame_crc(h, payload);
  body.push_back(static_cast<uint8_t>(crc >> 8));
  body.push_back(static_cast<uint8_t>(crc));
  for (uint8_t byte : body) {
    put_bits(b, hamming74_encode(static_cast<uint8_t>(byte >> 4)), 7);   // high nibble first
    put_bits(b, hamming74_encode(static_cast<uint8_t>(byte & 0xF)), 7);
  }
  return b;
}

enum class Status { Ok, Truncated, SyncFail, BadHeader, CrcFail };
struct ParseResult {
  Status status = Status::Truncated;
  Header header;
  std::vector<uint8_t> payload;  // empty unless status == Ok
  int header_corrections = 0;
  int payload_corrections = 0;
};
// `bits` must START at the first sync bit.
inline ParseResult parse_frame_bits(const Bits& bits) {
  ParseResult r;
  if (bits.size() < kFrameBits) return r;
  int mism = 0;
  for (int i = 0; i < kSyncBits; ++i)
    if (bits[static_cast<size_t>(i)] != ((kSyncWord >> (kSyncBits - 1 - i)) & 1u)) ++mism;
  if (mism > kSyncTolerance) {
    r.status = Status::SyncFail;
    return r;
  }
  size_t pos = kSyncBits;
  const GolayDecode g0 = golay23_decode(get_bits(bits, pos, 23));
  const GolayDecode g1 = golay23_decode(get_bits(bits, pos + 23, 23));
  pos += 46;
  r.header_corrections = g0.errors + g1.errors;
  r.header = word24_to_header((static_cast<uint32_t>(g0.data) << 12) | g1.data);
  if (r.header.version != 1) {
    r.status = Status::BadHeader;
    return r;
  }
  std::vector<uint8_t> body(kBodyBytes);
  for (size_t i = 0; i < kBodyBytes; ++i) {
    const Ham74Decode hi = hamming74_decode(static_cast<uint8_t>(get_bits(bits, pos, 7)));
    const Ham74Decode lo = hamming74_decode(static_cast<uint8_t>(get_bits(bits, pos + 7, 7)));
    pos += 14;
    r.payload_corrections += (hi.corrected ? 1 : 0) + (lo.corrected ? 1 : 0);
    body[i] = static_cast<uint8_t>((hi.nibble << 4) | lo.nibble);
  }
  const uint16_t rx_crc = static_cast<uint16_t>((body[kPayloadBytes] << 8) | body[kPayloadBytes + 1]);
  if (rx_crc != frame_crc(r.header, body.data())) {
    r.status = Status::CrcFail;
    return r;
  }
  r.payload.assign(body.begin(), body.begin() + kPayloadBytes);
  r.status = Status::Ok;
  return r;
}

// ------------------------------------------------------------------ sync search (W 3.6)
// Slide the 24-bit sync over `stream`; return the offset with the FEWEST mismatches provided that is <= max_mismatch
// (first such offset on ties); -1 if none.  The frame starts at the returned offset.
inline int find_sync(const Bits& stream, int max_mismatch) {
  int best_off = -1, best = max_mismatch + 1;
  if (stream.size() < static_cast<size_t>(kSyncBits)) return -1;
  for (size_t off = 0; off + kSyncBits <= stream.size(); ++off) {
    int mism = 0;
    for (int i = 0; i < kSyncBits && mism < best; ++i)
      if (stream[off + static_cast<size_t>(i)] != ((kSyncWord >> (kSyncBits - 1 - i)) & 1u)) ++mism;
    if (mism < best) {
      best = mism;
      best_off = static_cast<int>(off);
    }
  }
  return best_off;
}

}  // namespace gw
