// mcptt.hpp  --  STUBS for Pack 06/04 (student version).  Reference: ../solutions/mcptt.hpp
// GTP-U layout is from TS 29.281 (VERIFY every bit position against the spec).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace iwf {

// ---- GTP-U (W 3.1 - 3.2) ----
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
// Parse flags byte (version 3 bits, PT 1 bit, spare, E, S, PN), msg type, length, TEID, then the optional
// 4 bytes (seq, N-PDU, next-ext-type) if any of E/S/PN is set, then walk the extension-header chain
// (each: length in 4-byte units, content, last byte = next type, 0 = end).  Never read past `len`.
// Return TooShort (<8 bytes), BadVersion (!=1), NotGtpu (PT==0), LengthMismatch (buffer shorter than
// 8+length), BadExtension (zero-length or overrunning extension).
inline GtpuStatus parse_gtpu([[maybe_unused]] const uint8_t* d, [[maybe_unused]] size_t len, GtpuHeader& h) {
  h = GtpuHeader{};
  return GtpuStatus::TooShort;  // TODO W 3.1
}
// Build a G-PDU (msg type 0xFF); with_seq => set S flag and add 4 optional bytes (seq, N-PDU=0, next=0).
inline std::vector<uint8_t> build_gtpu([[maybe_unused]] uint32_t teid, [[maybe_unused]] const uint8_t* payload,
                                       [[maybe_unused]] size_t n, [[maybe_unused]] bool with_seq = false,
                                       [[maybe_unused]] uint16_t seq = 0) {
  return {};  // TODO W 3.2
}

// ---- NR numerology calculator (W 3.3) ----
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
// mu outside 0..4 -> row with mu = -1.  occupied_mhz stays -1 when the PRB lookup fails.
inline NumerologyRow numerology_row([[maybe_unused]] int mu, [[maybe_unused]] double bw_mhz) {
  return NumerologyRow{};  // TODO W 3.3
}
// Given helper (not a stub).
inline std::string format_row(const NumerologyRow& r) {
  char buf[160];
  std::snprintf(buf, sizeof buf, "mu=%d  SCS=%d kHz  slots/subframe=%d  slot=%.4f ms  symbol=%.3f us  PRBs=%d  occupied=%.2f MHz", r.mu,
                r.scs_khz, r.slots_per_subframe, r.slot_ms, r.symbol_useful_us, r.prbs, r.occupied_mhz);
  return buf;
}

// ---- LMR <-> MCPTT interworking (W 3.4 - 3.5) ----
enum class LmrEvent { PttPressed, PttReleased, ChannelBusy, EmergencyPttPressed };
enum class McpttMsg { None, FloorRequest, FloorRelease, FloorRequestEmergency };
inline McpttMsg lmr_to_mcptt([[maybe_unused]] LmrEvent e) { return McpttMsg::FloorRelease; }  // TODO W 3.4
enum class McpttFloor { Granted, Denied, Taken, Idle, Revoked };
enum class LmrAction { TxPermit, BusyTone, TxTerminate, ClearChannel };
inline LmrAction mcptt_to_lmr([[maybe_unused]] McpttFloor f) { return LmrAction::ClearChannel; }  // TODO W 3.4
// sip:tg<ID>@<domain>  (a model; VERIFY the real MCPTT group-identity conventions in TS 23.179).
inline std::string make_group_uri([[maybe_unused]] uint16_t tgid, [[maybe_unused]] const std::string& domain) { return ""; }  // TODO W 3.5
// Inverse of make_group_uri; nullopt on any malformed input (no prefix, no '@', non-digits, > 65535).
inline std::optional<uint16_t> parse_group_uri([[maybe_unused]] const std::string& uri) { return 0; }  // TODO W 3.5
// One-to-one mapping: add() must refuse a duplicate LMR id OR a duplicate URI.
class TalkgroupMap {
 public:
  bool add([[maybe_unused]] uint16_t lmr_tgid, [[maybe_unused]] const std::string& group_uri) { return true; }  // TODO W 3.5
  std::optional<std::string> to_mcptt([[maybe_unused]] uint16_t tg) const { return std::string("?"); }          // TODO
  std::optional<uint16_t> to_lmr([[maybe_unused]] const std::string& uri) const { return 0; }                    // TODO
  size_t size() const { return 99; }                                                                               // TODO
};

}  // namespace iwf
