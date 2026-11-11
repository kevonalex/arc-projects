// mcptt.hpp  --  REFERENCE SOLUTION for Pack 06/04 (GTP-U parser, NR numerology row, IWF mapping).
// GTP-U layout is from TS 29.281 (VERIFY every bit position against the spec before relying on it).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace iwf {

// ------------------------------------------------------------------ GTP-U (W 3.1 - 3.2)
enum class GtpuStatus { Ok, TooShort, BadVersion, NotGtpu, LengthMismatch, BadExtension };
struct GtpuHeader {
  uint8_t version = 0;
  uint8_t pt = 0;
  bool e = false, s = false, pn = false;
  uint8_t msg_type = 0;
  uint16_t length = 0;  // bytes after the first 8 (optional fields + extensions + payload)
  uint32_t teid = 0;
  uint16_t seq = 0;
  uint8_t npdu = 0;
  std::vector<uint8_t> ext_types;  // next-extension-header types seen, in order (0x85 = PDU session container)
  size_t header_len = 0;           // offset of the payload (T-PDU) from the start of the buffer
};
inline GtpuStatus parse_gtpu(const uint8_t* d, size_t len, GtpuHeader& h) {
  h = GtpuHeader{};
  if (len < 8) return GtpuStatus::TooShort;
  h.version = d[0] >> 5;
  h.pt = (d[0] >> 4) & 1;
  if (h.version != 1) return GtpuStatus::BadVersion;
  if (h.pt != 1) return GtpuStatus::NotGtpu;  // PT=0 is GTP' (charging), not user plane
  h.e = d[0] & 0x04;
  h.s = d[0] & 0x02;
  h.pn = d[0] & 0x01;
  h.msg_type = d[1];
  h.length = static_cast<uint16_t>((d[2] << 8) | d[3]);
  h.teid = (static_cast<uint32_t>(d[4]) << 24) | (static_cast<uint32_t>(d[5]) << 16) | (static_cast<uint32_t>(d[6]) << 8) | d[7];
  const size_t end = 8 + static_cast<size_t>(h.length);
  if (len < end) return GtpuStatus::LengthMismatch;
  h.header_len = 8;
  if (h.e || h.s || h.pn) {
    if (h.length < 4) return GtpuStatus::LengthMismatch;
    h.seq = static_cast<uint16_t>((d[8] << 8) | d[9]);
    h.npdu = d[10];
    uint8_t next = d[11];
    h.header_len = 12;
    while (next != 0) {
      h.ext_types.push_back(next);
      if (h.header_len + 1 > end) return GtpuStatus::BadExtension;
      const size_t ext_len = static_cast<size_t>(d[h.header_len]) * 4;  // length is in 4-octet units
      if (ext_len == 0 || h.header_len + ext_len > end) return GtpuStatus::BadExtension;
      next = d[h.header_len + ext_len - 1];
      h.header_len += ext_len;
    }
  }
  return GtpuStatus::Ok;
}
// Build a G-PDU (message type 0xFF) carrying `n` payload bytes; optional 16-bit sequence number.
inline std::vector<uint8_t> build_gtpu(uint32_t teid, const uint8_t* payload, size_t n, bool with_seq = false, uint16_t seq = 0) {
  std::vector<uint8_t> out;
  const size_t length = n + (with_seq ? 4 : 0);
  out.push_back(static_cast<uint8_t>(0x30 | (with_seq ? 0x02 : 0x00)));
  out.push_back(0xFF);
  out.push_back(static_cast<uint8_t>(length >> 8));
  out.push_back(static_cast<uint8_t>(length));
  for (int sh = 24; sh >= 0; sh -= 8) out.push_back(static_cast<uint8_t>(teid >> sh));
  if (with_seq) {
    out.push_back(static_cast<uint8_t>(seq >> 8));
    out.push_back(static_cast<uint8_t>(seq));
    out.push_back(0);  // N-PDU number
    out.push_back(0);  // next extension header type: none
  }
  out.insert(out.end(), payload, payload + n);
  return out;
}

// ------------------------------------------------------------------ NR numerology calculator (W 3.3)
// Given helper (not a stub): (bw MHz, mu) -> max PRBs, -1 if not in the small table (VERIFY TS 38.101).
inline int nr_prb_count(double bw_mhz, int mu) {
  struct E { double bw; int mu; int n; };
  static const E t[] = {{10, 0, 52}, {20, 0, 106}, {20, 1, 51}, {40, 1, 106}, {100, 1, 273}, {100, 3, 66}, {400, 3, 264}};
  for (const E& e : t)
    if (e.bw == bw_mhz && e.mu == mu) return e.n;
  return -1;
}
struct NumerologyRow {
  int mu = -1;
  int scs_khz = 0;
  int slots_per_subframe = 0;
  double slot_ms = 0;
  double symbol_useful_us = 0;
  int prbs = -1;
  double occupied_mhz = -1;  // prbs * 12 subcarriers * SCS
};
inline NumerologyRow numerology_row(int mu, double bw_mhz) {
  NumerologyRow r;
  if (mu < 0 || mu > 4) return r;
  r.mu = mu;
  r.scs_khz = 15 << mu;
  r.slots_per_subframe = 1 << mu;
  r.slot_ms = 1.0 / (1 << mu);
  r.symbol_useful_us = 1000.0 / r.scs_khz;
  r.prbs = nr_prb_count(bw_mhz, mu);
  if (r.prbs > 0) r.occupied_mhz = r.prbs * 12.0 * r.scs_khz / 1000.0;
  return r;
}
inline std::string format_row(const NumerologyRow& r) {
  char buf[160];
  std::snprintf(buf, sizeof buf, "mu=%d  SCS=%d kHz  slots/subframe=%d  slot=%.4f ms  symbol=%.3f us  PRBs=%d  occupied=%.2f MHz", r.mu,
                r.scs_khz, r.slots_per_subframe, r.slot_ms, r.symbol_useful_us, r.prbs, r.occupied_mhz);
  return buf;
}

// ------------------------------------------------------------------ LMR <-> MCPTT interworking (W 3.4 - 3.5)
enum class LmrEvent { PttPressed, PttReleased, ChannelBusy, EmergencyPttPressed };
enum class McpttMsg { None, FloorRequest, FloorRelease, FloorRequestEmergency };
inline McpttMsg lmr_to_mcptt(LmrEvent e) {
  switch (e) {
    case LmrEvent::PttPressed: return McpttMsg::FloorRequest;
    case LmrEvent::PttReleased: return McpttMsg::FloorRelease;
    case LmrEvent::EmergencyPttPressed: return McpttMsg::FloorRequestEmergency;
    case LmrEvent::ChannelBusy: return McpttMsg::None;  // a local observation, nothing to send
  }
  return McpttMsg::None;
}
enum class McpttFloor { Granted, Denied, Taken, Idle, Revoked };
enum class LmrAction { TxPermit, BusyTone, TxTerminate, ClearChannel };
inline LmrAction mcptt_to_lmr(McpttFloor f) {
  switch (f) {
    case McpttFloor::Granted: return LmrAction::TxPermit;
    case McpttFloor::Denied:
    case McpttFloor::Taken: return LmrAction::BusyTone;
    case McpttFloor::Revoked: return LmrAction::TxTerminate;
    case McpttFloor::Idle: return LmrAction::ClearChannel;
  }
  return LmrAction::ClearChannel;
}
// A MCPTT group identity is a SIP URI; we model it as sip:tg<ID>@<domain> (VERIFY the real URI conventions in TS 23.179).
inline std::string make_group_uri(uint16_t tgid, const std::string& domain) { return "sip:tg" + std::to_string(tgid) + "@" + domain; }
inline std::optional<uint16_t> parse_group_uri(const std::string& uri) {
  const std::string pre = "sip:tg";
  if (uri.compare(0, pre.size(), pre) != 0) return std::nullopt;
  const size_t at = uri.find('@', pre.size());
  if (at == std::string::npos || at == pre.size()) return std::nullopt;
  unsigned long v = 0;
  for (size_t i = pre.size(); i < at; ++i) {
    if (uri[i] < '0' || uri[i] > '9') return std::nullopt;
    v = v * 10 + static_cast<unsigned long>(uri[i] - '0');
    if (v > 0xFFFF) return std::nullopt;
  }
  return static_cast<uint16_t>(v);
}
// One-to-one talkgroup mapping; both directions must be unambiguous or floor control cannot be bridged.
class TalkgroupMap {
 public:
  bool add(uint16_t lmr_tgid, const std::string& group_uri) {
    if (fwd_.count(lmr_tgid) || rev_.count(group_uri)) return false;
    fwd_[lmr_tgid] = group_uri;
    rev_[group_uri] = lmr_tgid;
    return true;
  }
  std::optional<std::string> to_mcptt(uint16_t tg) const {
    auto it = fwd_.find(tg);
    return it == fwd_.end() ? std::nullopt : std::optional<std::string>(it->second);
  }
  std::optional<uint16_t> to_lmr(const std::string& uri) const {
    auto it = rev_.find(uri);
    return it == rev_.end() ? std::nullopt : std::optional<uint16_t>(it->second);
  }
  size_t size() const { return fwd_.size(); }

 private:
  std::map<uint16_t, std::string> fwd_;
  std::map<std::string, uint16_t> rev_;
};

}  // namespace iwf
