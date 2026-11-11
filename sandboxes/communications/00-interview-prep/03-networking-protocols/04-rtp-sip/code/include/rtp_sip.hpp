// STUB - 03-net/04-rtp-sip. Implement every TODO. Tests: tests/test_rtp_sip.cpp
// Utilities (get/put, lower, trim, split_lines, static_rtpmap, lookup_codec, expand_compact) are provided.
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ether {

using Bytes = std::vector<uint8_t>;
inline uint16_t get16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }
inline uint32_t get32(const uint8_t* p) { return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | p[3]; }
inline void put16(Bytes& b, uint16_t v) { b.push_back(uint8_t(v >> 8)); b.push_back(uint8_t(v)); }
inline void put32(Bytes& b, uint32_t v) { for (int s = 24; s >= 0; s -= 8) b.push_back(uint8_t(v >> s)); }

// ===================================================================== RTP
struct RtpHeader {
    uint8_t version = 2;
    bool padding = false, extension = false, marker = false;
    uint8_t cc = 0;
    uint8_t pt = 0;
    uint16_t seq = 0;
    uint32_t timestamp = 0, ssrc = 0;
    std::vector<uint32_t> csrc;
    uint16_t ext_profile = 0;
    Bytes ext_data;                      // multiple of 4 bytes
};
struct RtpPacket { RtpHeader h; size_t payload_off = 0, payload_len = 0; };

// TODO: V must be 2; 12-byte fixed header; CSRC list (cc*4); optional extension (profile16, length16 in WORDS, data);
// padding (last byte = pad count incl. itself, must be >=1 and fit) is excluded from payload_len. Reject truncation.
inline std::optional<RtpPacket> parse_rtp(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }
// TODO: inverse of parse_rtp (cc taken from csrc.size(); ext_data size/4 into the length field)
inline Bytes build_rtp(const RtpHeader& h, const uint8_t* payload, size_t len) { (void)h; (void)payload; (void)len; return {}; }

inline bool seq_newer(uint16_t a, uint16_t b) { (void)a; (void)b; return false; }       // TODO serial arithmetic: (int16_t)(a-b) > 0
inline int32_t ts_diff(uint32_t a, uint32_t b) { (void)a; (void)b; return 0; }          // TODO (int32_t)(a-b)

// TODO RFC 3550 A.1: first packet sets base. udelta = (uint16_t)(seq - max_seq).
//   udelta < 3000 -> in order (if seq < max_seq: cycles += 65536); max_seq = seq; received++ ; return true
//   udelta <= 65536 - 100 -> wild jump: return false (no state change)
//   else late/duplicate within the misorder window: received++ ; return true
class SeqTracker {
public:
    bool update(uint16_t seq) { (void)seq; return false; }
    uint32_t extended_max() const { return 0; }     // cycles + max_seq
    uint32_t expected() const { return 0; }         // extended_max - base + 1
    uint32_t received() const { return 0; }
    int64_t lost() const { return 0; }              // expected - received
};

// TODO RFC 3550 A.8:  transit = arrival - ts;  D = |transit - prev_transit| (as int32);  J += (D - J)/16. First packet only stores transit.
class JitterEstimator {
public:
    void update(uint32_t arrival_units, uint32_t rtp_ts) { (void)arrival_units; (void)rtp_ts; }
    double jitter_units() const { return -1; }
    double jitter_ms(double clock_hz) const { (void)clock_hz; return -1; }
};
inline uint8_t loss_fraction_256(uint32_t expected_interval, uint32_t received_interval) { (void)expected_interval; (void)received_interval; return 0; }  // TODO (lost<<8)/expected, 0 if none lost

// ===================================================================== RTCP
struct ReportBlock {
    uint32_t ssrc = 0; uint8_t fraction_lost = 0; int32_t cum_lost = 0;     // 24-bit signed on the wire
    uint32_t ext_highest_seq = 0, jitter = 0, lsr = 0, dlsr = 0;
};
struct RtcpPacket {
    uint8_t type = 200;                    // 200 = SR, 201 = RR
    uint32_t ssrc = 0;
    uint32_t ntp_msw = 0, ntp_lsw = 0, rtp_ts = 0, pkt_count = 0, octet_count = 0;   // SR only
    std::vector<ReportBlock> blocks;
};
// TODO: header (V=2, P=0, RC=blocks, PT, length = total 32-bit words - 1), SSRC, [SR info: 5 words], 6-word report blocks
inline Bytes build_rtcp(const RtcpPacket& r) { (void)r; return {}; }
// TODO: validate V=2, PT 200/201, length*4+4 <= n, enough bytes for RC blocks; sign-extend the 24-bit cumulative lost
inline std::optional<RtcpPacket> parse_rtcp(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }
inline uint32_t ntp_middle32(uint32_t msw, uint32_t lsw) { (void)msw; (void)lsw; return 0; }   // TODO (msw<<16)|(lsw>>16)
inline double rtt_ms(uint32_t now_mid32, uint32_t lsr, uint32_t dlsr) { (void)now_mid32; (void)lsr; (void)dlsr; return 0; }  // TODO (A-LSR-DLSR)/65536 s in ms; -1 if lsr==0

// ===================================================================== SDP / offer-answer
struct Rtpmap { std::string name; int clock = 0; int channels = 1; };
struct SdpMedia {
    std::string type, proto; int port = 0;
    std::vector<int> formats;
    std::map<int, Rtpmap> rtpmap;
    std::string conn_addr, direction = "sendrecv";
    int ptime = 0;
};
struct SdpSession { std::string conn_addr; std::vector<SdpMedia> media; };

inline std::string lower(std::string s) { for (auto& c : s) c = char(std::tolower((unsigned char)c)); return s; }
inline std::optional<Rtpmap> static_rtpmap(int pt) {
    switch (pt) {
        case 0: return Rtpmap{"PCMU", 8000, 1};
        case 8: return Rtpmap{"PCMA", 8000, 1};
        case 9: return Rtpmap{"G722", 8000, 1};
        case 18: return Rtpmap{"G729", 8000, 1};
        case 13: return Rtpmap{"CN", 8000, 1};
        default: return std::nullopt;
    }
}
inline std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> v; std::string cur;
    for (char c : s) { if (c == '\n') { if (!cur.empty() && cur.back() == '\r') cur.pop_back(); v.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) { if (cur.back() == '\r') cur.pop_back(); v.push_back(cur); }
    return v;
}
// TODO: v= must be 0; every line "<char>=<value>"; c=IN IP4 addr (session or media level; media-level wins);
// m=<type> <port> <proto> <pt...>; a=rtpmap:<pt> <name>/<clock>[/<ch>]; a=ptime:; a=sendrecv|sendonly|recvonly|inactive.
inline std::optional<SdpSession> parse_sdp(const std::string& text) { (void)text; return std::nullopt; }
inline std::optional<Rtpmap> lookup_codec(const SdpMedia& m, int pt) {
    auto it = m.rtpmap.find(pt);
    if (it != m.rtpmap.end()) return it->second;
    return static_rtpmap(pt);
}
// TODO RFC 3264: walk the OFFER's formats in order; pick the first whose (name, clock) matches a local codec (case-insensitive).
inline std::optional<int> negotiate_codec(const SdpMedia& offer, const std::vector<Rtpmap>& local_supported) { (void)offer; (void)local_supported; return std::nullopt; }
// TODO: v=0, o=, s=-, c=IN IP4 <ip>, t=0 0, m=audio <port> RTP/AVP <pt>, a=rtpmap only if the offer had one for pt, a=ptime, a=sendrecv  (CRLF)
inline std::string build_sdp_answer(const SdpMedia& offer, int chosen_pt, const std::string& local_ip, int local_port, int ptime = 20) {
    (void)offer; (void)chosen_pt; (void)local_ip; (void)local_port; (void)ptime; return "";
}

// ===================================================================== SIP
struct SipMessage {
    bool is_request = true;
    std::string method, uri;
    int status = 0; std::string reason;
    std::map<std::string, std::string> headers;       // lower-case names, compact forms expanded
    std::string body;
    std::string header(const std::string& n) const { auto it = headers.find(lower(n)); return it == headers.end() ? "" : it->second; }
};
inline std::string expand_compact(const std::string& n) {
    static const std::map<std::string, std::string> m{{"v", "via"}, {"f", "from"}, {"t", "to"}, {"i", "call-id"}, {"m", "contact"}, {"l", "content-length"}, {"c", "content-type"}};
    auto it = m.find(n); return it == m.end() ? n : it->second;
}
inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
// TODO: need "\r\n\r\n"; start line "METHOD uri SIP/2.0" or "SIP/2.0 code reason"; headers "Name: value" (lower-case key, expand
// compact names); body = rest, cut to Content-Length (reject if the body is shorter than declared).
inline std::optional<SipMessage> parse_sip(const std::string& text) { (void)text; return std::nullopt; }
// TODO: status line; copy Via, From, To (+ ";tag=<to_tag>" if given and To has no tag), Call-ID, CSeq; Content-Length: 0; blank line.
inline std::string build_sip_response(const SipMessage& req, int status, const std::string& reason, const std::string& to_tag = "") {
    (void)req; (void)status; (void)reason; (void)to_tag; return "";
}

enum class CallState { Idle, Calling, Proceeding, Established, Terminated };
enum class CallEvent { SendInvite, Recv1xx, Recv2xx, RecvFailure, SendAck, SendBye, RecvBye };
// TODO: UAC view. Idle -SendInvite-> Calling; Calling/Proceeding -1xx-> Proceeding, -2xx-> Established, -failure-> Terminated;
// Established -SendBye/RecvBye-> Terminated; SendAck keeps Established; anything else: stay.
inline CallState call_next(CallState s, CallEvent e) { (void)e; return s; }

}  // namespace ether
