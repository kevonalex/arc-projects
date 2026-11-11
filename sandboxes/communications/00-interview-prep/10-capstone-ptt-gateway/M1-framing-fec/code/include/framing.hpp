// framing.hpp  --  STUBS for Capstone M1 (student version).  Reference: ../solutions/framing.hpp
//
// OUR toy over-the-air format (not P25/DMR/TETRA).  One 20 ms voice frame, MSB first, one bit per vector element:
//   [ sync 24 | Golay(23,12) x2 = header 46 | Hamming(7,4) over (payload 160 B + CRC-16 2 B) = 2268 ]  = 2338 bits
// The CRC covers the 3 header bytes AND the payload (so a mis-"corrected" header is still caught).
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gw {

using Bits = std::vector<uint8_t>;  // each element is 0 or 1

constexpr size_t kPayloadBytes = 160;  // 20 ms of 8 kHz mu-law (stand-in for a vocoder frame)
constexpr int kSyncBits = 24;
constexpr uint32_t kSyncWord = 0x02B31E;  // worst aperiodic autocorrelation sidelobe = 3 (exhaustive search)
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

// ---- CRC-16/CCITT-FALSE (W 3.1): poly 0x1021, init 0xFFFF, no reflection, no final XOR.  "123456789" -> 0x29B1 ----
inline uint16_t crc16_ccitt([[maybe_unused]] const uint8_t* d, [[maybe_unused]] size_t n) { return 0; }  // TODO W 3.1

// ---- Golay(23,12,7) (W 3.2): codeword = (d12 << 11) | ((d12 << 11) mod g), g = 0xC75 ----
inline uint32_t golay23_encode([[maybe_unused]] uint16_t d12) { return 0; }  // TODO W 3.2
struct GolayDecode {
  uint16_t data;
  int errors;  // bits corrected (0..3).  The code is PERFECT: it always "corrects"; 4+ errors miscorrect silently.
};
// Nearest codeword (brute force over all 4096 is fine).
inline GolayDecode golay23_decode([[maybe_unused]] uint32_t rx) { return {0xFFFF, -1}; }  // TODO W 3.2

// ---- Hamming(7,4) (W 3.3): position k (1-based) is bit k-1; layout p1 p2 d1 p4 d2 d3 d4; nibble bit0..3 -> pos 3,5,6,7 ----
inline uint8_t hamming74_encode([[maybe_unused]] uint8_t nib) { return 0xFF; }  // TODO W 3.3
struct Ham74Decode {
  uint8_t nibble;
  bool corrected;
};
inline Ham74Decode hamming74_decode([[maybe_unused]] uint8_t cw) { return {0xFF, true}; }  // TODO W 3.3

// ---- header + frame (W 3.4 - 3.5) ----
struct Header {
  uint8_t version = 1;    // 2 bits
  uint8_t type = 0;       // 2 bits: 0 voice, 1 end-of-call, 2 data, 3 reserved
  uint8_t talkgroup = 0;  // 8 bits
  uint16_t seq = 0;       // 12 bits
};
inline bool operator==(const Header& a, const Header& b) {
  return a.version == b.version && a.type == b.type && a.talkgroup == b.talkgroup && a.seq == b.seq;
}
// 24-bit word, MSB first: version(2) type(2) talkgroup(8) seq(12).
inline uint32_t header_to_word24([[maybe_unused]] const Header& h) { return 0xFFFFFF; }  // TODO W 3.4
inline Header word24_to_header([[maybe_unused]] uint32_t w) { return Header{3, 3, 255, 4095}; }  // TODO W 3.4
// CRC-16 over the 3 header bytes (big-endian word24) followed by the payload.
inline uint16_t frame_crc([[maybe_unused]] const Header& h, [[maybe_unused]] const uint8_t* payload) { return 0; }  // TODO W 3.4
// sync | golay(word24 >> 12) | golay(word24 & 0xFFF) | for each byte of payload+crc: hamming(high nibble), hamming(low nibble)
inline Bits build_frame_bits([[maybe_unused]] const Header& h, [[maybe_unused]] const uint8_t* payload) { return {}; }  // TODO W 3.5

enum class Status { Ok, Truncated, SyncFail, BadHeader, CrcFail };
struct ParseResult {
  Status status = Status::Truncated;
  Header header;
  std::vector<uint8_t> payload;  // empty unless status == Ok
  int header_corrections = 0;
  int payload_corrections = 0;
};
// `bits` starts at the first sync bit.  Order of checks: size (Truncated) -> sync within kSyncTolerance (SyncFail) ->
// Golay-decode both header words, version must be 1 (BadHeader) -> Hamming-decode body -> CRC (CrcFail).
// Count header_corrections (sum of Golay errors) and payload_corrections (number of Hamming words that fixed a bit).
inline ParseResult parse_frame_bits([[maybe_unused]] const Bits& bits) {
  ParseResult r;
  r.status = Status::SyncFail;
  return r;  // TODO W 3.5
}

// ---- sync search (W 3.6) ----
// Slide the sync word over the stream; return the offset with the FEWEST mismatching bits provided it is
// <= max_mismatch (first offset on ties), else -1.
inline int find_sync([[maybe_unused]] const Bits& stream, [[maybe_unused]] int max_mismatch) { return -2; }  // TODO W 3.6

}  // namespace gw
