// Reference solution - 03-net/04-rtp-sip : RTP/RTCP (RFC 3550), SDP offer/answer (RFC 4566/3264), SIP parsing (RFC 3261)
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
    uint8_t cc = 0;                      // CSRC count (== csrc.size())
    uint8_t pt = 0;                      // 7 bits
    uint16_t seq = 0;
    uint32_t timestamp = 0, ssrc = 0;
    std::vector<uint32_t> csrc;
    uint16_t ext_profile = 0;
    Bytes ext_data;                      // multiple of 4 bytes
};
struct RtpPacket { RtpHeader h; size_t payload_off = 0, payload_len = 0; };

inline std::optional<RtpPacket> parse_rtp(const uint8_t* p, size_t n) {
    if (n < 12) return std::nullopt;
    RtpPacket r;
    RtpHeader& h = r.h;
    h.version = p[0] >> 6;
    if (h.version != 2) return std::nullopt;                     // V=2 is the demux guard against other protocols
    h.padding = p[0] & 0x20; h.extension = p[0] & 0x10; h.cc = p[0] & 0x0F;
    h.marker = p[1] & 0x80; h.pt = p[1] & 0x7F;
    h.seq = get16(p + 2); h.timestamp = get32(p + 4); h.ssrc = get32(p + 8);
    size_t off = 12;
    if (n < off + size_t{h.cc} * 4) return std::nullopt;
    for (int i = 0; i < h.cc; ++i, off += 4) h.csrc.push_back(get32(p + off));
    if (h.extension) {
        if (n < off + 4) return std::nullopt;
        h.ext_profile = get16(p + off);
        size_t words = get16(p + off + 2);
        off += 4;
        if (n < off + words * 4) return std::nullopt;
        h.ext_data.assign(p + off, p + off + words * 4);
        off += words * 4;
    }
    size_t end = n;
    if (h.padding) {
        uint8_t pad = p[n - 1];
        if (pad == 0 || off + pad > n) return std::nullopt;      // count includes itself; must fit
        end = n - pad;
    }
    r.payload_off = off; r.payload_len = end - off;
    return r;
}
inline Bytes build_rtp(const RtpHeader& h, const uint8_t* payload, size_t len) {
    Bytes b;
    b.reserve(12 + h.csrc.size() * 4 + h.ext_data.size() + 4 + len);
    b.push_back(uint8_t((2 << 6) | (h.padding ? 0x20 : 0) | (h.extension ? 0x10 : 0) | (h.csrc.size() & 0x0F)));
    b.push_back(uint8_t((h.marker ? 0x80 : 0) | (h.pt & 0x7F)));
    put16(b, h.seq); put32(b, h.timestamp); put32(b, h.ssrc);
    for (uint32_t c : h.csrc) put32(b, c);
    if (h.extension) { put16(b, h.ext_profile); put16(b, uint16_t(h.ext_data.size() / 4)); b.insert(b.end(), h.ext_data.begin(), h.ext_data.end()); }
    b.insert(b.end(), payload, payload + len);
    return b;
}

// 16-bit sequence numbers wrap; "a is newer than b" uses serial-number arithmetic (RFC 1982): the signed difference.
inline bool seq_newer(uint16_t a, uint16_t b) { return static_cast<int16_t>(a - b) > 0; }
inline int32_t ts_diff(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b); }   // a - b with 32-bit wrap

// RFC 3550 A.1 sequence tracking: extended highest sequence = cycles*65536 + max_seq.
class SeqTracker {
public:
    static constexpr uint16_t kMaxDropout = 3000, kMaxMisorder = 100;
    // returns false if the packet is rejected as a wild jump (stream restart / foreign source)
    bool update(uint16_t seq) {
        if (!started_) { started_ = true; base_seq_ = max_seq_ = seq; received_ = 1; return true; }
        uint16_t udelta = static_cast<uint16_t>(seq - max_seq_);
        if (udelta < kMaxDropout) {                       // in order (possibly with a gap)
            if (seq < max_seq_) cycles_ += 65536;         // wrapped around
            max_seq_ = seq; ++received_; return true;
        }
        if (udelta <= 65536 - kMaxMisorder) return false; // very large jump
        ++received_;                                      // late / duplicate within misorder window
        return true;
    }
    uint32_t extended_max() const { return cycles_ + max_seq_; }
    uint32_t expected() const { return started_ ? extended_max() - base_seq_ + 1 : 0; }
    uint32_t received() const { return received_; }
    int64_t lost() const { return int64_t{expected()} - received_; }   // may be negative with duplicates
private:
    bool started_ = false; uint16_t max_seq_ = 0; uint32_t cycles_ = 0, base_seq_ = 0, received_ = 0;
};

// RFC 3550 A.8 interarrival jitter. All values in RTP timestamp units.
//   D(i-1,i) = (R_i - R_{i-1}) - (S_i - S_{i-1})    J += (|D| - J) / 16
class JitterEstimator {
public:
    void update(uint32_t arrival_units, uint32_t rtp_ts) {
        uint32_t transit = arrival_units - rtp_ts;
        if (have_prev_) {
            double d = std::fabs(static_cast<double>(static_cast<int32_t>(transit - prev_transit_)));
            jitter_ += (d - jitter_) / 16.0;
        }
        prev_transit_ = transit; have_prev_ = true;
    }
    double jitter_units() const { return jitter_; }
    double jitter_ms(double clock_hz) const { return jitter_ * 1000.0 / clock_hz; }
private:
    bool have_prev_ = false; uint32_t prev_transit_ = 0; double jitter_ = 0;
};

// fraction lost in 1/256 units over an interval (RFC 3550 6.4.1 / A.3)
inline uint8_t loss_fraction_256(uint32_t expected_interval, uint32_t received_interval) {
    if (expected_interval == 0 || received_interval >= expected_interval) return 0;
    uint32_t lost = expected_interval - received_interval;
    return static_cast<uint8_t>(std::min<uint64_t>(255, (uint64_t{lost} << 8) / expected_interval));   // 1.0 would be 256: clamp
}

// ===================================================================== RTCP
struct ReportBlock {
    uint32_t ssrc = 0; uint8_t fraction_lost = 0; int32_t cum_lost = 0;     // 24-bit signed
    uint32_t ext_highest_seq = 0, jitter = 0, lsr = 0, dlsr = 0;
};
struct RtcpPacket {
    uint8_t type = 200;                    // 200 = SR, 201 = RR
    uint32_t ssrc = 0;
    uint32_t ntp_msw = 0, ntp_lsw = 0, rtp_ts = 0, pkt_count = 0, octet_count = 0;   // SR only
    std::vector<ReportBlock> blocks;
};
inline Bytes build_rtcp(const RtcpPacket& r) {
    Bytes b;
    size_t words = 1 + (r.type == 200 ? 5 : 0) + r.blocks.size() * 6;      // length field = words after the first word
    b.push_back(uint8_t((2 << 6) | (r.blocks.size() & 0x1F)));
    b.push_back(r.type);
    put16(b, uint16_t(words));
    put32(b, r.ssrc);
    if (r.type == 200) { put32(b, r.ntp_msw); put32(b, r.ntp_lsw); put32(b, r.rtp_ts); put32(b, r.pkt_count); put32(b, r.octet_count); }
    for (const auto& k : r.blocks) {
        put32(b, k.ssrc);
        b.push_back(k.fraction_lost);
        uint32_t cl = static_cast<uint32_t>(k.cum_lost) & 0xFFFFFF;
        b.push_back(uint8_t(cl >> 16)); b.push_back(uint8_t(cl >> 8)); b.push_back(uint8_t(cl));
        put32(b, k.ext_highest_seq); put32(b, k.jitter); put32(b, k.lsr); put32(b, k.dlsr);
    }
    return b;
}
inline std::optional<RtcpPacket> parse_rtcp(const uint8_t* p, size_t n) {
    if (n < 8 || (p[0] >> 6) != 2) return std::nullopt;
    RtcpPacket r;
    uint8_t rc = p[0] & 0x1F; r.type = p[1];
    if (r.type != 200 && r.type != 201) return std::nullopt;
    size_t total = (size_t{get16(p + 2)} + 1) * 4;                         // length counts 32-bit words minus one
    if (total > n) return std::nullopt;
    size_t off = 4; r.ssrc = get32(p + off); off += 4;
    if (r.type == 200) {
        if (total < off + 20) return std::nullopt;
        r.ntp_msw = get32(p + off); r.ntp_lsw = get32(p + off + 4); r.rtp_ts = get32(p + off + 8);
        r.pkt_count = get32(p + off + 12); r.octet_count = get32(p + off + 16); off += 20;
    }
    if (total < off + size_t{rc} * 24) return std::nullopt;
    for (int i = 0; i < rc; ++i, off += 24) {
        ReportBlock k;
        k.ssrc = get32(p + off); k.fraction_lost = p[off + 4];
        uint32_t cl = (uint32_t{p[off + 5]} << 16) | (uint32_t{p[off + 6]} << 8) | p[off + 7];
        k.cum_lost = (cl & 0x800000) ? static_cast<int32_t>(cl | 0xFF000000u) : static_cast<int32_t>(cl);   // sign-extend 24 bits
        k.ext_highest_seq = get32(p + off + 8); k.jitter = get32(p + off + 12); k.lsr = get32(p + off + 16); k.dlsr = get32(p + off + 20);
        r.blocks.push_back(k);
    }
    return r;
}
// LSR/DLSR use the middle 32 bits of the 64-bit NTP timestamp (16.16 fixed point, units of 1/65536 s).
inline uint32_t ntp_middle32(uint32_t msw, uint32_t lsw) { return (msw << 16) | (lsw >> 16); }
// RTT = A - LSR - DLSR  (all 1/65536 s, wrap-safe), returned in ms.
inline double rtt_ms(uint32_t now_mid32, uint32_t lsr, uint32_t dlsr) {
    if (lsr == 0) return -1;                                               // no SR received yet
    uint32_t d = now_mid32 - lsr - dlsr;
    return d * 1000.0 / 65536.0;
}

// ===================================================================== SDP (RFC 4566) + offer/answer (RFC 3264)
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
// Static payload types (RFC 3551) have no a=rtpmap line.
inline std::optional<Rtpmap> static_rtpmap(int pt) {
    switch (pt) {
        case 0: return Rtpmap{"PCMU", 8000, 1};
        case 8: return Rtpmap{"PCMA", 8000, 1};
        case 9: return Rtpmap{"G722", 8000, 1};      // famous erratum: clock 8000 although 16 kHz sampling
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
inline std::optional<SdpSession> parse_sdp(const std::string& text) {
    SdpSession s; SdpMedia* cur = nullptr; bool saw_v = false;
    for (const auto& line : split_lines(text)) {
        if (line.size() < 2 || line[1] != '=') return std::nullopt;     // every SDP line is <type>=<value>
        char t = line[0]; std::string v = line.substr(2);
        if (t == 'v') { if (v != "0") return std::nullopt; saw_v = true; }
        else if (t == 'c') {
            std::istringstream is(v); std::string net, ipv, addr; is >> net >> ipv >> addr;
            addr = addr.substr(0, addr.find('/'));                       // strip TTL/count
            if (cur) cur->conn_addr = addr; else s.conn_addr = addr;
        } else if (t == 'm') {
            std::istringstream is(v); SdpMedia m; is >> m.type >> m.port >> m.proto;
            int f; while (is >> f) m.formats.push_back(f);
            s.media.push_back(m); cur = &s.media.back();
        } else if (t == 'a' && cur) {
            if (v.rfind("rtpmap:", 0) == 0) {
                std::istringstream is(v.substr(7)); int pt = 0; std::string enc; is >> pt >> enc;
                Rtpmap r; std::vector<std::string> parts; std::string tok; std::istringstream es(enc);
                while (std::getline(es, tok, '/')) parts.push_back(tok);
                if (parts.size() >= 2) { r.name = parts[0]; r.clock = std::atoi(parts[1].c_str()); if (parts.size() > 2) r.channels = std::atoi(parts[2].c_str()); cur->rtpmap[pt] = r; }
            } else if (v.rfind("ptime:", 0) == 0) cur->ptime = std::atoi(v.c_str() + 6);
            else if (v == "sendrecv" || v == "sendonly" || v == "recvonly" || v == "inactive") cur->direction = v;
        }
    }
    if (!saw_v) return std::nullopt;
    for (auto& m : s.media) if (m.conn_addr.empty()) m.conn_addr = s.conn_addr;   // media-level overrides session-level
    return s;
}
inline std::optional<Rtpmap> lookup_codec(const SdpMedia& m, int pt) {
    auto it = m.rtpmap.find(pt);
    if (it != m.rtpmap.end()) return it->second;
    return static_rtpmap(pt);
}
// Offer/answer: the answerer picks from the offered formats (in the OFFERER's order of preference) the first one it
// supports - same encoding name (case-insensitive) AND clock rate. Returns the payload type number to use.
inline std::optional<int> negotiate_codec(const SdpMedia& offer, const std::vector<Rtpmap>& local_supported) {
    for (int pt : offer.formats) {
        auto c = lookup_codec(offer, pt);
        if (!c) continue;
        for (const auto& l : local_supported)
            if (lower(l.name) == lower(c->name) && l.clock == c->clock) return pt;
    }
    return std::nullopt;
}
inline std::string build_sdp_answer(const SdpMedia& offer, int chosen_pt, const std::string& local_ip, int local_port, int ptime = 20) {
    auto c = lookup_codec(offer, chosen_pt);
    std::ostringstream o;
    o << "v=0\r\no=gw 1 1 IN IP4 " << local_ip << "\r\ns=-\r\nc=IN IP4 " << local_ip << "\r\nt=0 0\r\n";
    o << "m=audio " << local_port << " RTP/AVP " << chosen_pt << "\r\n";
    if (c && offer.rtpmap.count(chosen_pt))
        o << "a=rtpmap:" << chosen_pt << " " << c->name << "/" << c->clock << (c->channels > 1 ? "/" + std::to_string(c->channels) : "") << "\r\n";
    o << "a=ptime:" << ptime << "\r\na=sendrecv\r\n";
    return o.str();
}

// ===================================================================== SIP (RFC 3261) - parsing + call state machine
struct SipMessage {
    bool is_request = true;
    std::string method, uri;                          // request
    int status = 0; std::string reason;               // response
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
inline std::optional<SipMessage> parse_sip(const std::string& text) {
    size_t sep = text.find("\r\n\r\n");
    if (sep == std::string::npos) return std::nullopt;                     // header block must end with an empty line
    SipMessage m;
    auto lines = split_lines(text.substr(0, sep));
    if (lines.empty()) return std::nullopt;
    std::istringstream sl(lines[0]); std::string a, b;
    sl >> a >> b;
    if (a.rfind("SIP/2.0", 0) == 0) {
        m.is_request = false; m.status = std::atoi(b.c_str());
        if (m.status < 100 || m.status > 699) return std::nullopt;
        std::getline(sl, m.reason); m.reason = trim(m.reason);
    } else {
        std::string ver; sl >> ver;
        if (ver != "SIP/2.0" || a.empty() || b.empty()) return std::nullopt;
        m.method = a; m.uri = b;
    }
    for (size_t i = 1; i < lines.size(); ++i) {
        size_t c = lines[i].find(':');
        if (c == std::string::npos) return std::nullopt;
        m.headers[expand_compact(lower(trim(lines[i].substr(0, c))))] = trim(lines[i].substr(c + 1));
    }
    m.body = text.substr(sep + 4);
    std::string cl = m.header("content-length");
    if (!cl.empty()) { size_t n = std::strtoul(cl.c_str(), nullptr, 10); if (n > m.body.size()) return std::nullopt; m.body.resize(n); }
    return m;
}
// A response copies Via, From, To (adding a tag if the UAS creates a dialog), Call-ID and CSeq from the request.
inline std::string build_sip_response(const SipMessage& req, int status, const std::string& reason, const std::string& to_tag = "") {
    std::ostringstream o;
    o << "SIP/2.0 " << status << " " << reason << "\r\n";
    o << "Via: " << req.header("via") << "\r\n" << "From: " << req.header("from") << "\r\n";
    std::string to = req.header("to");
    if (!to_tag.empty() && to.find("tag=") == std::string::npos) to += ";tag=" + to_tag;
    o << "To: " << to << "\r\n" << "Call-ID: " << req.header("call-id") << "\r\n" << "CSeq: " << req.header("cseq") << "\r\n";
    o << "Content-Length: 0\r\n\r\n";
    return o.str();
}

enum class CallState { Idle, Calling, Proceeding, Established, Terminated };
enum class CallEvent { SendInvite, Recv1xx, Recv2xx, RecvFailure, SendAck, SendBye, RecvBye };
// UAC view of one INVITE dialog. Unknown event for a state: stay.
inline CallState call_next(CallState s, CallEvent e) {
    switch (s) {
        case CallState::Idle:       if (e == CallEvent::SendInvite) return CallState::Calling; break;
        case CallState::Calling:
        case CallState::Proceeding:
            if (e == CallEvent::Recv1xx) return CallState::Proceeding;
            if (e == CallEvent::Recv2xx) return CallState::Established;       // then the UAC MUST send ACK (not part of the INVITE transaction)
            if (e == CallEvent::RecvFailure) return CallState::Terminated;
            break;
        case CallState::Established:
            if (e == CallEvent::SendBye || e == CallEvent::RecvBye) return CallState::Terminated;
            break;
        case CallState::Terminated: break;
    }
    return s;
}

}  // namespace ether
