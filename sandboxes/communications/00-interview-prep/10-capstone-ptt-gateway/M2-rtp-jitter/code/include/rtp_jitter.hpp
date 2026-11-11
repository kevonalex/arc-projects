// rtp_jitter.hpp  --  STUBS for Capstone M2 (student version).  Reference: ../solutions/rtp_jitter.hpp
// Wire facts: RFC 3550 (RTP), RFC 3551 (PT 0 = PCMU), ITU-T G.711, RFC 2365 + RFC 1112 (multicast).
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace gw {

// ---- G.711 mu-law (W 3.1) ----
// Classic algorithm: bias 0x84, clip 32635, 3-bit exponent + 4-bit mantissa, result is bit-inverted.
// encode(0) = 0xFF, encode(32767) = 0x80, encode(-32768) = 0x00; decode(0xFF) = 0, decode(0x80) = 32124, decode(0x00) = -32124.
inline uint8_t mulaw_encode([[maybe_unused]] int16_t pcm) { return 0x55; }  // TODO W 3.1
inline int16_t mulaw_decode([[maybe_unused]] uint8_t u) { return 1234; }    // TODO W 3.1

// ---- RTP header (W 3.2) ----
struct RtpHeader {
  uint8_t version = 2;
  bool padding = false;
  bool extension = false;
  uint8_t cc = 0;  // CSRC count
  bool marker = false;
  uint8_t pt = 0;  // 0 = PCMU
  uint16_t seq = 0;
  uint32_t timestamp = 0;
  uint32_t ssrc = 0;
};
// 12-byte fixed header (V=2, P=0, X=0, CC=0; M and PT from h; seq, timestamp, ssrc big-endian) + payload.
inline std::vector<uint8_t> rtp_pack([[maybe_unused]] const RtpHeader& h, [[maybe_unused]] const uint8_t* payload,
                                     [[maybe_unused]] size_t n) {
  return {};  // TODO W 3.2
}
enum class RtpStatus { Ok, TooShort, BadVersion, Truncated };
// Parse; skip 4*CC CSRC bytes, a header extension (4 bytes + 4*length words) and trailing padding (last byte = count).
// TooShort: < 12 bytes.  BadVersion: V != 2.  Truncated: CSRC/extension/padding runs past the buffer (or pad count 0).
inline RtpStatus rtp_parse([[maybe_unused]] const uint8_t* d, [[maybe_unused]] size_t len, [[maybe_unused]] RtpHeader& h,
                           [[maybe_unused]] size_t& payload_off, [[maybe_unused]] size_t& payload_len) {
  return RtpStatus::TooShort;  // TODO W 3.2
}

// ---- sequence numbers and receiver statistics (W 3.3) ----
// RFC 1982 serial arithmetic: is a newer than b, even across the 65535 -> 0 wrap?  (a == b is not newer.)
inline bool seq_newer([[maybe_unused]] uint16_t a, [[maybe_unused]] uint16_t b) { return false; }  // TODO W 3.3
// Monotonic 64-bit count from 16-bit sequence numbers; first packet maps to its own value; reorder-safe (signed 16-bit delta).
class SeqUnwrapper {
 public:
  int64_t extend([[maybe_unused]] uint16_t seq) { return -1; }  // TODO W 3.3
};
// RFC 3550 A.3: expected = highest - first + 1; lost = expected - received.
class LossCounter {
 public:
  void on_packet([[maybe_unused]] uint16_t seq) {}  // TODO W 3.3
  uint32_t received() const { return 999; }          // TODO
  int64_t expected() const { return 999; }           // TODO
  int64_t lost() const { return 999; }               // TODO
  double loss_fraction() const { return 9.0; }       // TODO: max(lost,0) / expected
};
// RFC 3550 6.4.1: D = (Rj - Ri) - (Sj - Si) in RTP timestamp units; J += (|D| - J) / 16.  First update only records transit.
class JitterEstimator {
 public:
  void update([[maybe_unused]] uint32_t rtp_ts, [[maybe_unused]] uint32_t arrival_ts) {}  // TODO W 3.4
  double jitter() const { return -1.0; }                                                    // TODO W 3.4
  double jitter_ms([[maybe_unused]] int clock_hz = 8000) const { return -1.0; }             // TODO W 3.4
};

// ---- jitter buffer (W 3.4 - 3.5) ----
struct PlayoutFrame {
  bool present = false;    // a real packet was available for this 20 ms slot
  bool concealed = false;  // the packet was missing: PLC (repeat of the previous frame, or 160 x 0xFF silence)
  int64_t seq = -1;
  std::vector<uint8_t> data;
};
enum class PushResult { Accepted, Duplicate, Late, TooEarly };
// Rules (see worksheet W 3.5):
//  push():  unwrap the seq. Started and ext < next  -> Late (count). Already stored -> Duplicate.
//           ext - horizon >= max_frames -> TooEarly (horizon = next_ if started, else the smallest stored seq).
//           Otherwise store -> Accepted.
//  pop():   not started: wait until >= target_frames are stored, then start at the smallest stored seq; until then
//           return a default PlayoutFrame (present=false, concealed=false).  Started: play `next`; present if stored,
//           else concealed (count it); always advance next.
class JitterBuffer {
 public:
  JitterBuffer(int target_frames, int max_frames) : target_(target_frames), max_(max_frames) {}
  PushResult push([[maybe_unused]] uint16_t seq, [[maybe_unused]] std::vector<uint8_t> payload) { return PushResult::TooEarly; }  // TODO
  PlayoutFrame pop() { return PlayoutFrame{}; }                                                                                  // TODO
  void reset() {}                                                  // TODO: back to pre-buffering, counters cleared
  int depth() const { return -1; }                                  // TODO
  bool started() const { return true; }                             // TODO
  uint32_t received() const { return 999; }                         // TODO
  uint32_t late() const { return 999; }                             // TODO
  uint32_t duplicates() const { return 999; }                       // TODO
  uint32_t too_early() const { return 999; }                        // TODO
  uint32_t concealed() const { return 999; }                        // TODO

 private:
  [[maybe_unused]] int target_, max_;
};
// ceil(k * jitter / ptime) frames, never below min_frames.
inline int choose_target_frames([[maybe_unused]] double jitter_ms, [[maybe_unused]] double ptime_ms = 20.0,
                                [[maybe_unused]] double k = 3.0, [[maybe_unused]] int min_frames = 2) {
  return 0;  // TODO W 3.5
}

// ---- voice packet sizing (W 3.6) ----
inline uint32_t rtp_ts_increment([[maybe_unused]] double ptime_ms, [[maybe_unused]] int clock_hz) { return 0; }  // TODO W 3.6
// (payload + RTP 12 + UDP 8 + IPv4 20 + l2_bytes) * 8 per packet, one packet per ptime.
inline double stream_bitrate_bps([[maybe_unused]] int payload_bytes, [[maybe_unused]] double ptime_ms, [[maybe_unused]] int l2_bytes) {
  return 0.0;  // TODO W 3.6
}

// ---- multicast addressing (W 3.7) ----
inline uint32_t ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {  // given
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(c) << 8) | d;
}
inline bool is_ipv4_multicast([[maybe_unused]] uint32_t ip) { return false; }  // TODO W 3.7: 224.0.0.0/4
// Ethernet multicast MAC: 01:00:5E followed by the LOW 23 bits of the group address.
inline std::array<uint8_t, 6> multicast_mac([[maybe_unused]] uint32_t ip) { return {0, 0, 0, 0, 0, 0}; }  // TODO W 3.7
// Talkgroup N -> 239.192.(N >> 8).(N & 255)   (organisation-local scope, RFC 2365).
inline uint32_t talkgroup_group_addr([[maybe_unused]] uint16_t tg) { return 0; }  // TODO W 3.7
inline std::string ipv4_to_string([[maybe_unused]] uint32_t ip) { return ""; }     // TODO W 3.7: dotted quad

}  // namespace gw
