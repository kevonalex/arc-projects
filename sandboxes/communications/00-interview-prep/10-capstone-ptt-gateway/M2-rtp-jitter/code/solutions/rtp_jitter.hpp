// rtp_jitter.hpp  --  REFERENCE SOLUTION for Capstone M2 (G.711 mu-law, RTP header, loss/jitter stats, jitter buffer,
// multicast addressing).  Wire facts: RFC 3550 (RTP), RFC 3551 (PT 0 = PCMU), ITU-T G.711, RFC 2365 + RFC 1112 (multicast).
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace gw {

// ------------------------------------------------------------------ G.711 mu-law (W 3.1)
inline uint8_t mulaw_encode(int16_t pcm) {
  const int kBias = 0x84, kClip = 32635;
  int s = pcm;
  const int sign = (s < 0) ? 0x80 : 0;
  if (s < 0) s = -s;
  if (s > kClip) s = kClip;
  s += kBias;
  int exponent = 7;
  for (int mask = 0x4000; (s & mask) == 0 && exponent > 0; mask >>= 1) --exponent;
  const int mantissa = (s >> (exponent + 3)) & 0x0F;
  return static_cast<uint8_t>(~(sign | (exponent << 4) | mantissa));
}
inline int16_t mulaw_decode(uint8_t u) {
  u = static_cast<uint8_t>(~u);
  const int sign = u & 0x80, exponent = (u >> 4) & 7, mantissa = u & 0x0F;
  int sample = (((mantissa << 3) + 0x84) << exponent) - 0x84;
  return static_cast<int16_t>(sign ? -sample : sample);
}

// ------------------------------------------------------------------ RTP header (W 3.2)
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
// 12-byte fixed header (no CSRCs / extension / padding are WRITTEN) followed by the payload.
inline std::vector<uint8_t> rtp_pack(const RtpHeader& h, const uint8_t* payload, size_t n) {
  std::vector<uint8_t> out;
  out.reserve(12 + n);
  out.push_back(static_cast<uint8_t>((2u << 6) | 0u));  // V=2, P=0, X=0, CC=0
  out.push_back(static_cast<uint8_t>((h.marker ? 0x80u : 0u) | (h.pt & 0x7Fu)));
  out.push_back(static_cast<uint8_t>(h.seq >> 8));
  out.push_back(static_cast<uint8_t>(h.seq));
  for (int sh = 24; sh >= 0; sh -= 8) out.push_back(static_cast<uint8_t>(h.timestamp >> sh));
  for (int sh = 24; sh >= 0; sh -= 8) out.push_back(static_cast<uint8_t>(h.ssrc >> sh));
  out.insert(out.end(), payload, payload + n);
  return out;
}
enum class RtpStatus { Ok, TooShort, BadVersion, Truncated };
// Parse, skipping CSRC entries, a header extension and trailing padding.  payload_off/len locate the media bytes.
inline RtpStatus rtp_parse(const uint8_t* d, size_t len, RtpHeader& h, size_t& payload_off, size_t& payload_len) {
  if (len < 12) return RtpStatus::TooShort;
  h.version = d[0] >> 6;
  if (h.version != 2) return RtpStatus::BadVersion;
  h.padding = (d[0] & 0x20) != 0;
  h.extension = (d[0] & 0x10) != 0;
  h.cc = d[0] & 0x0F;
  h.marker = (d[1] & 0x80) != 0;
  h.pt = d[1] & 0x7F;
  h.seq = static_cast<uint16_t>((d[2] << 8) | d[3]);
  h.timestamp = (static_cast<uint32_t>(d[4]) << 24) | (static_cast<uint32_t>(d[5]) << 16) | (static_cast<uint32_t>(d[6]) << 8) | d[7];
  h.ssrc = (static_cast<uint32_t>(d[8]) << 24) | (static_cast<uint32_t>(d[9]) << 16) | (static_cast<uint32_t>(d[10]) << 8) | d[11];
  size_t off = 12 + 4u * h.cc;
  if (off > len) return RtpStatus::Truncated;
  if (h.extension) {
    if (off + 4 > len) return RtpStatus::Truncated;
    const size_t words = static_cast<size_t>((d[off + 2] << 8) | d[off + 3]);
    off += 4 + 4 * words;
    if (off > len) return RtpStatus::Truncated;
  }
  size_t plen = len - off;
  if (h.padding) {
    if (plen == 0) return RtpStatus::Truncated;
    const size_t pad = d[len - 1];
    if (pad == 0 || pad > plen) return RtpStatus::Truncated;
    plen -= pad;
  }
  payload_off = off;
  payload_len = plen;
  return RtpStatus::Ok;
}

// ------------------------------------------------------------------ sequence numbers and receiver statistics (W 3.3)
// RFC 1982 serial arithmetic on 16 bits: is a "newer" than b even across the 65535 -> 0 wrap?
inline bool seq_newer(uint16_t a, uint16_t b) { return a != b && static_cast<uint16_t>(a - b) < 0x8000; }
// Turns 16-bit sequence numbers into a monotonic 64-bit count (first packet maps to its own value).
class SeqUnwrapper {
 public:
  int64_t extend(uint16_t seq) {
    if (!started_) {
      started_ = true;
      last16_ = seq;
      ext_ = seq;
      return ext_;
    }
    ext_ += static_cast<int16_t>(static_cast<uint16_t>(seq - last16_));  // signed 16-bit delta: reorder-safe
    last16_ = seq;
    return ext_;
  }

 private:
  bool started_ = false;
  uint16_t last16_ = 0;
  int64_t ext_ = 0;
};
// RFC 3550 A.3: expected = highest - first + 1; lost = expected - received.
class LossCounter {
 public:
  void on_packet(uint16_t seq) {
    const int64_t e = unwrap_.extend(seq);
    if (received_ == 0) {
      base_ = highest_ = e;
    } else if (e > highest_) {
      highest_ = e;
    }
    ++received_;
  }
  uint32_t received() const { return received_; }
  int64_t expected() const { return received_ == 0 ? 0 : highest_ - base_ + 1; }
  int64_t lost() const { return expected() - static_cast<int64_t>(received_); }  // negative if duplicates arrived
  double loss_fraction() const { return expected() <= 0 ? 0.0 : static_cast<double>(lost() < 0 ? 0 : lost()) / static_cast<double>(expected()); }

 private:
  SeqUnwrapper unwrap_;
  uint32_t received_ = 0;
  int64_t base_ = 0, highest_ = 0;
};
// RFC 3550 6.4.1 interarrival jitter, in RTP timestamp units: D = (Rj - Ri) - (Sj - Si); J += (|D| - J) / 16.
class JitterEstimator {
 public:
  void update(uint32_t rtp_ts, uint32_t arrival_ts) {
    const int32_t transit = static_cast<int32_t>(arrival_ts - rtp_ts);  // wrap-safe difference
    if (have_) {
      const int32_t d = transit - last_transit_;
      j_ += (std::fabs(static_cast<double>(d)) - j_) / 16.0;
    }
    last_transit_ = transit;
    have_ = true;
  }
  double jitter() const { return j_; }
  double jitter_ms(int clock_hz = 8000) const { return j_ * 1000.0 / clock_hz; }

 private:
  bool have_ = false;
  int32_t last_transit_ = 0;
  double j_ = 0.0;
};

// ------------------------------------------------------------------ jitter buffer (W 3.4 - 3.5)
struct PlayoutFrame {
  bool present = false;    // a real packet was available for this 20 ms slot
  bool concealed = false;  // the packet was missing: this is PLC (repeat of the previous frame / silence)
  int64_t seq = -1;
  std::vector<uint8_t> data;
};
enum class PushResult { Accepted, Duplicate, Late, TooEarly };
class JitterBuffer {
 public:
  // target_frames = playout delay in frames (start when this many are buffered); max_frames = how far ahead we accept.
  JitterBuffer(int target_frames, int max_frames) : target_(target_frames), max_(max_frames) {}
  PushResult push(uint16_t seq, std::vector<uint8_t> payload) {
    const int64_t e = unwrap_.extend(seq);
    if (started_ && e < next_) {
      ++late_;
      return PushResult::Late;
    }
    if (frames_.count(e)) {
      ++duplicates_;
      return PushResult::Duplicate;
    }
    const int64_t horizon = started_ ? next_ : (frames_.empty() ? e : frames_.begin()->first);
    if (e - horizon >= max_) {
      ++too_early_;
      return PushResult::TooEarly;
    }
    frames_[e] = std::move(payload);
    ++received_;
    return PushResult::Accepted;
  }
  // Call once per 20 ms playout tick.
  PlayoutFrame pop() {
    PlayoutFrame f;
    if (!started_) {
      if (static_cast<int>(frames_.size()) < target_) return f;  // still pre-buffering: nothing to play yet
      started_ = true;
      next_ = frames_.begin()->first;
    }
    f.seq = next_;
    auto it = frames_.find(next_);
    if (it != frames_.end()) {
      f.present = true;
      f.data = std::move(it->second);
      last_ = f.data;
      frames_.erase(it);
    } else {
      f.concealed = true;
      f.data = last_.empty() ? std::vector<uint8_t>(160, 0xFF) : last_;  // repeat previous frame, or mu-law silence
      ++concealed_;
    }
    ++next_;
    return f;
  }
  void reset() {  // end of talk-spurt: forget everything and pre-buffer again
    *this = JitterBuffer(target_, max_);
  }
  int depth() const { return static_cast<int>(frames_.size()); }
  bool started() const { return started_; }
  uint32_t received() const { return received_; }
  uint32_t late() const { return late_; }
  uint32_t duplicates() const { return duplicates_; }
  uint32_t too_early() const { return too_early_; }
  uint32_t concealed() const { return concealed_; }

 private:
  int target_, max_;
  SeqUnwrapper unwrap_;
  std::map<int64_t, std::vector<uint8_t>> frames_;
  std::vector<uint8_t> last_;
  bool started_ = false;
  int64_t next_ = 0;
  uint32_t received_ = 0, late_ = 0, duplicates_ = 0, too_early_ = 0, concealed_ = 0;
};
// Playout target from measured jitter: k * J rounded UP to whole frames, never below min_frames.
inline int choose_target_frames(double jitter_ms, double ptime_ms = 20.0, double k = 3.0, int min_frames = 2) {
  const int n = static_cast<int>(std::ceil(k * jitter_ms / ptime_ms));
  return n < min_frames ? min_frames : n;
}

// ------------------------------------------------------------------ voice packet sizing (W 3.6)
inline uint32_t rtp_ts_increment(double ptime_ms, int clock_hz) { return static_cast<uint32_t>(std::lround(clock_hz * ptime_ms / 1000.0)); }
// Bits per second on the wire for one voice stream: (payload + RTP 12 + UDP 8 + IPv4 20 + l2_bytes) * 8 per packet.
inline double stream_bitrate_bps(int payload_bytes, double ptime_ms, int l2_bytes) {
  return (payload_bytes + 12 + 8 + 20 + l2_bytes) * 8.0 / (ptime_ms / 1000.0);
}

// ------------------------------------------------------------------ multicast addressing (W 3.7)
inline uint32_t ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(c) << 8) | d;
}
inline bool is_ipv4_multicast(uint32_t ip) { return (ip >> 28) == 0xE; }  // 224.0.0.0/4
// Ethernet multicast MAC: 01:00:5E + the LOW 23 bits of the group address (so 32 groups share each MAC).
inline std::array<uint8_t, 6> multicast_mac(uint32_t ip) {
  return {0x01, 0x00, 0x5E, static_cast<uint8_t>((ip >> 16) & 0x7F), static_cast<uint8_t>(ip >> 8), static_cast<uint8_t>(ip)};
}
// Talkgroup N -> 239.192.(N>>8).(N&255): organisation-local scope (239.192.0.0/14, RFC 2365).
inline uint32_t talkgroup_group_addr(uint16_t tg) { return ipv4(239, 192, static_cast<uint8_t>(tg >> 8), static_cast<uint8_t>(tg)); }
inline std::string ipv4_to_string(uint32_t ip) {
  return std::to_string(ip >> 24) + "." + std::to_string((ip >> 16) & 255) + "." + std::to_string((ip >> 8) & 255) + "." + std::to_string(ip & 255);
}

}  // namespace gw
