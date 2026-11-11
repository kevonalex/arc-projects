// Reference solution - 03-net/05-grpc-security
// protobuf wire format, gRPC message framing, HTTP/2 frame header, grpc-timeout, SRTP index/replay,
// and the pure-C++ CallControl service logic that a gRPC server would wrap.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ether {

using Bytes = std::vector<uint8_t>;

// ============================================================ protobuf varint (base-128, little-endian groups)
inline Bytes encode_varint(uint64_t v) {
    Bytes out;
    while (v >= 0x80) { out.push_back(static_cast<uint8_t>(v | 0x80)); v >>= 7; }   // 7 payload bits + continuation bit
    out.push_back(static_cast<uint8_t>(v));
    return out;
}
// Returns nullopt on truncation or a >10-byte / overflowing encoding. 'used' = bytes consumed.
inline std::optional<uint64_t> decode_varint(const uint8_t* p, size_t n, size_t& used) {
    uint64_t v = 0;
    for (size_t i = 0; i < n && i < 10; ++i) {
        uint8_t b = p[i];
        if (i == 9 && b > 1) return std::nullopt;                 // 10th byte may only supply bit 63
        v |= uint64_t{static_cast<uint8_t>(b & 0x7F)} << (7 * i);
        if (!(b & 0x80)) { used = i + 1; return v; }
    }
    return std::nullopt;
}
// ZigZag: small magnitudes (positive or negative) -> small unsigned numbers. 0,-1,1,-2,2 -> 0,1,2,3,4
inline uint64_t zigzag_encode(int64_t v) { return (static_cast<uint64_t>(v) << 1) ^ static_cast<uint64_t>(v >> 63); }
inline int64_t zigzag_decode(uint64_t u) { return static_cast<int64_t>(u >> 1) ^ -static_cast<int64_t>(u & 1); }

enum class WireType : uint8_t { Varint = 0, Fixed64 = 1, Len = 2, Fixed32 = 5 };
inline uint64_t make_tag(uint32_t field, WireType t) { return (uint64_t{field} << 3) | static_cast<uint64_t>(t); }

class ProtoWriter {
public:
    void varint(uint32_t field, uint64_t v) { tag(field, WireType::Varint); append(encode_varint(v)); }
    void sint(uint32_t field, int64_t v) { varint(field, zigzag_encode(v)); }
    void str(uint32_t field, const std::string& s) { tag(field, WireType::Len); append(encode_varint(s.size())); buf_.insert(buf_.end(), s.begin(), s.end()); }
    void fixed64(uint32_t field, uint64_t v) { tag(field, WireType::Fixed64); for (int i = 0; i < 8; ++i) buf_.push_back(uint8_t(v >> (8 * i))); }   // little-endian
    void fixed32(uint32_t field, uint32_t v) { tag(field, WireType::Fixed32); for (int i = 0; i < 4; ++i) buf_.push_back(uint8_t(v >> (8 * i))); }
    void dbl(uint32_t field, double d) { uint64_t u; std::memcpy(&u, &d, 8); fixed64(field, u); }
    const Bytes& bytes() const { return buf_; }
private:
    void tag(uint32_t field, WireType t) { append(encode_varint(make_tag(field, t))); }
    void append(const Bytes& b) { buf_.insert(buf_.end(), b.begin(), b.end()); }
    Bytes buf_;
};

struct Field {
    uint32_t number = 0;
    WireType type = WireType::Varint;
    uint64_t value = 0;                   // Varint / Fixed32 / Fixed64 (raw little-endian value)
    const uint8_t* data = nullptr;        // Len
    size_t len = 0;
    std::string as_string() const { return std::string(reinterpret_cast<const char*>(data), len); }
};
class ProtoReader {
public:
    ProtoReader(const uint8_t* p, size_t n) : p_(p), n_(n) {}
    // nullopt at end of input OR on malformed input (check ok()). Unknown fields are returned like any other: callers skip them.
    std::optional<Field> next() {
        if (pos_ >= n_ || !ok_) return std::nullopt;
        size_t used = 0;
        auto tag = decode_varint(p_ + pos_, n_ - pos_, used);
        if (!tag || (*tag >> 3) == 0 || (*tag >> 3) > 0x1FFFFFFF) { ok_ = false; return std::nullopt; }
        pos_ += used;
        Field f; f.number = static_cast<uint32_t>(*tag >> 3);
        switch (*tag & 7) {
        case 0: { auto v = decode_varint(p_ + pos_, n_ - pos_, used); if (!v) return fail(); f.type = WireType::Varint; f.value = *v; pos_ += used; break; }
        case 1: { if (n_ - pos_ < 8) return fail(); f.type = WireType::Fixed64; for (int i = 0; i < 8; ++i) f.value |= uint64_t{p_[pos_ + i]} << (8 * i); pos_ += 8; break; }
        case 5: { if (n_ - pos_ < 4) return fail(); f.type = WireType::Fixed32; for (int i = 0; i < 4; ++i) f.value |= uint64_t{p_[pos_ + i]} << (8 * i); pos_ += 4; break; }
        case 2: {
            auto l = decode_varint(p_ + pos_, n_ - pos_, used);
            if (!l) return fail();
            pos_ += used;
            if (*l > n_ - pos_) return fail();                    // declared length exceeds the buffer
            f.type = WireType::Len; f.data = p_ + pos_; f.len = static_cast<size_t>(*l); pos_ += f.len; break;
        }
        default: return fail();                                   // 3,4 = deprecated groups; 6,7 invalid
        }
        return f;
    }
    bool ok() const { return ok_; }
private:
    std::optional<Field> fail() { ok_ = false; return std::nullopt; }
    const uint8_t* p_; size_t n_; size_t pos_ = 0; bool ok_ = true;
};

// ---- messages from proto/callcontrol.proto (proto3: default values are NOT serialised) ----
struct StartCallRequest { std::string call_id; uint32_t talkgroup = 0; std::string caller; };
struct CallStats { std::string call_id; uint32_t packets = 0, lost = 0; double jitter_ms = 0; };

inline Bytes encode(const StartCallRequest& m) {
    ProtoWriter w;
    if (!m.call_id.empty()) w.str(1, m.call_id);
    if (m.talkgroup) w.varint(2, m.talkgroup);
    if (!m.caller.empty()) w.str(3, m.caller);
    return w.bytes();
}
inline std::optional<StartCallRequest> decode_start_call(const uint8_t* p, size_t n) {
    ProtoReader r(p, n); StartCallRequest m;
    while (auto f = r.next()) {
        if (f->number == 1 && f->type == WireType::Len) m.call_id = f->as_string();
        else if (f->number == 2 && f->type == WireType::Varint) m.talkgroup = static_cast<uint32_t>(f->value);   // uint32: truncate like protobuf
        else if (f->number == 3 && f->type == WireType::Len) m.caller = f->as_string();
        /* else: unknown field - skipped, which is how protobuf stays forward compatible */
    }
    if (!r.ok()) return std::nullopt;
    return m;
}
inline Bytes encode(const CallStats& m) {
    ProtoWriter w;
    if (!m.call_id.empty()) w.str(1, m.call_id);
    if (m.packets) w.varint(2, m.packets);
    if (m.lost) w.varint(3, m.lost);
    if (m.jitter_ms != 0.0) w.dbl(4, m.jitter_ms);
    return w.bytes();
}
inline std::optional<CallStats> decode_call_stats(const uint8_t* p, size_t n) {
    ProtoReader r(p, n); CallStats m;
    while (auto f = r.next()) {
        if (f->number == 1 && f->type == WireType::Len) m.call_id = f->as_string();
        else if (f->number == 2 && f->type == WireType::Varint) m.packets = static_cast<uint32_t>(f->value);
        else if (f->number == 3 && f->type == WireType::Varint) m.lost = static_cast<uint32_t>(f->value);
        else if (f->number == 4 && f->type == WireType::Fixed64) std::memcpy(&m.jitter_ms, &f->value, 8);
    }
    if (!r.ok()) return std::nullopt;
    return m;
}

// ============================================================ gRPC length-prefixed message (5-byte prefix)
inline Bytes grpc_frame(const Bytes& payload, bool compressed = false) {
    Bytes b; b.push_back(compressed ? 1 : 0);
    uint32_t n = static_cast<uint32_t>(payload.size());
    for (int s = 24; s >= 0; s -= 8) b.push_back(uint8_t(n >> s));
    b.insert(b.end(), payload.begin(), payload.end());
    return b;
}
struct GrpcUnframe {
    enum class Status { NeedMore, Ok, TooLarge, BadFlag } status = Status::NeedMore;
    bool compressed = false; Bytes payload; size_t consumed = 0;
};
// Incremental: call with whatever bytes have arrived. NeedMore => keep the bytes and call again after the next recv.
inline GrpcUnframe grpc_unframe(const uint8_t* p, size_t n, size_t max_len = 4 * 1024 * 1024) {
    GrpcUnframe r;
    if (n < 5) return r;
    if (p[0] > 1) { r.status = GrpcUnframe::Status::BadFlag; return r; }
    uint32_t len = (uint32_t{p[1]} << 24) | (uint32_t{p[2]} << 16) | (uint32_t{p[3]} << 8) | p[4];
    if (len > max_len) { r.status = GrpcUnframe::Status::TooLarge; return r; }   // refuse BEFORE waiting for/allocating the body
    if (n < 5 + size_t{len}) return r;
    r.status = GrpcUnframe::Status::Ok; r.compressed = p[0] == 1;
    r.payload.assign(p + 5, p + 5 + len); r.consumed = 5 + size_t{len};
    return r;
}

// ============================================================ HTTP/2 (RFC 9113)
inline const char* kH2Preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";   // 24 bytes the client sends first
struct H2FrameHeader { uint32_t length = 0; uint8_t type = 0, flags = 0; uint32_t stream_id = 0; };
inline std::optional<H2FrameHeader> parse_h2_frame_header(const uint8_t* p, size_t n) {
    if (n < 9) return std::nullopt;
    H2FrameHeader h;
    h.length = (uint32_t{p[0]} << 16) | (uint32_t{p[1]} << 8) | p[2];
    h.type = p[3]; h.flags = p[4];
    h.stream_id = ((uint32_t{p[5]} << 24) | (uint32_t{p[6]} << 16) | (uint32_t{p[7]} << 8) | p[8]) & 0x7FFFFFFFu;   // top bit reserved
    return h;
}
inline Bytes build_h2_frame_header(const H2FrameHeader& h) {
    return {uint8_t(h.length >> 16), uint8_t(h.length >> 8), uint8_t(h.length), h.type, h.flags,
            uint8_t((h.stream_id >> 24) & 0x7F), uint8_t(h.stream_id >> 16), uint8_t(h.stream_id >> 8), uint8_t(h.stream_id)};
}
inline const char* h2_type_name(uint8_t t) {
    static const char* n[] = {"DATA", "HEADERS", "PRIORITY", "RST_STREAM", "SETTINGS", "PUSH_PROMISE", "PING", "GOAWAY", "WINDOW_UPDATE", "CONTINUATION"};
    return t < 10 ? n[t] : "UNKNOWN";
}

// ============================================================ grpc-timeout header:  <1-8 digits><H|M|S|m|u|n>
inline std::optional<int64_t> parse_grpc_timeout_ns(const std::string& s) {
    if (s.size() < 2 || s.size() > 9) return std::nullopt;
    int64_t v = 0;
    for (size_t i = 0; i + 1 < s.size(); ++i) { if (s[i] < '0' || s[i] > '9') return std::nullopt; v = v * 10 + (s[i] - '0'); }
    switch (s.back()) {
        case 'H': return v * 3600LL * 1000000000LL;
        case 'M': return v * 60LL * 1000000000LL;
        case 'S': return v * 1000000000LL;
        case 'm': return v * 1000000LL;
        case 'u': return v * 1000LL;
        case 'n': return v;
        default: return std::nullopt;
    }
}
// Coarsest unit that represents the value exactly with <= 8 digits; otherwise the finest unit that fits (rounded up).
inline std::string format_grpc_timeout(int64_t ns) {
    struct U { char c; int64_t ns; };
    static const U units[] = {{'H', 3600000000000LL}, {'M', 60000000000LL}, {'S', 1000000000LL}, {'m', 1000000LL}, {'u', 1000LL}, {'n', 1LL}};
    for (const auto& u : units)
        if (ns % u.ns == 0 && ns / u.ns <= 99999999) return std::to_string(ns / u.ns) + u.c;
    for (int i = 5; i >= 0; --i) {                       // finest first: keeps the most precision that still fits
        int64_t v = (ns + units[i].ns - 1) / units[i].ns;
        if (v <= 99999999) return std::to_string(v) + units[i].c;
    }
    return "99999999H";
}
// Deadline propagation: a child call may only use what the parent has left (minus a safety margin).
inline int64_t remaining_ms(int64_t deadline_total_ms, int64_t elapsed_ms, int64_t margin_ms = 0) {
    int64_t r = deadline_total_ms - elapsed_ms - margin_ms;
    return r < 0 ? 0 : r;
}

enum class GrpcStatus : int { Ok = 0, Cancelled = 1, Unknown = 2, InvalidArgument = 3, DeadlineExceeded = 4, NotFound = 5, AlreadyExists = 6,
    PermissionDenied = 7, ResourceExhausted = 8, FailedPrecondition = 9, Aborted = 10, OutOfRange = 11, Unimplemented = 12, Internal = 13,
    Unavailable = 14, DataLoss = 15, Unauthenticated = 16 };
// HTTP status -> gRPC status for non-gRPC responses (gRPC over HTTP/2 spec table). VERIFY against the spec.
inline GrpcStatus grpc_status_from_http(int http) {
    switch (http) {
        case 400: return GrpcStatus::Internal;
        case 401: return GrpcStatus::Unauthenticated;
        case 403: return GrpcStatus::PermissionDenied;
        case 404: return GrpcStatus::Unimplemented;
        case 429: case 502: case 503: case 504: return GrpcStatus::Unavailable;
        default: return GrpcStatus::Unknown;
    }
}

// ============================================================ SRTP packet index + replay window (RFC 3711 3.3)
// SRTP uses a 48-bit packet index i = 2^16 * ROC + SEQ. Only SEQ travels in the packet; the receiver
// reconstructs ROC (rollover counter) by guessing the value closest to its last state.
class SrtpIndex {
public:
    uint64_t estimate(uint16_t seq) const {
        if (!init_) return seq;
        uint64_t v;
        if (s_l_ < 32768) v = (seq - s_l_ > 32768) ? ((roc_ - 1) & 0xFFFFFFFFu) : roc_;       // a very high seq just after init = old packet from the previous cycle
        else v = (s_l_ - 32768 > seq) ? roc_ + 1 : roc_;                    // a very low seq when s_l is high = next cycle
        return (v << 16) | seq;
    }
    // Call only AFTER the packet authenticated (otherwise an attacker can desynchronise ROC).
    void update(uint16_t seq) {
        uint64_t idx = estimate(seq);
        if (!init_) { init_ = true; roc_ = 0; s_l_ = seq; return; }
        if (idx > ((roc_ << 16) | s_l_)) { roc_ = idx >> 16; s_l_ = seq; }
    }
    uint64_t roc() const { return roc_; }
private:
    bool init_ = false; uint64_t roc_ = 0; uint16_t s_l_ = 0;
};
// 64-packet sliding replay window. check() BEFORE decrypting, update() only AFTER the auth tag verified.
class ReplayWindow {
public:
    bool check(uint64_t idx) const {
        if (!any_) return true;
        if (idx > highest_) return true;
        uint64_t diff = highest_ - idx;
        if (diff >= 64) return false;                      // too old
        return !((mask_ >> diff) & 1);                      // already seen
    }
    void update(uint64_t idx) {
        if (!any_) { any_ = true; highest_ = idx; mask_ = 1; return; }
        if (idx > highest_) { uint64_t sh = idx - highest_; mask_ = sh >= 64 ? 0 : (mask_ << sh); mask_ |= 1; highest_ = idx; return; }
        uint64_t diff = highest_ - idx;
        if (diff < 64) mask_ |= (uint64_t{1} << diff);
    }
private:
    bool any_ = false; uint64_t highest_ = 0, mask_ = 0;
};

// ============================================================ CallControl service logic (what a gRPC handler wraps)
class CallControlService {
public:
    struct StartResult { bool accepted = false; std::string reason; };
    using StatsSink = std::function<bool(const CallStats&, bool final_message)>;   // return false = client cancelled the stream

    StartResult start_call(const StartCallRequest& req) {
        if (req.call_id.empty()) return {false, "call_id required"};          // -> gRPC INVALID_ARGUMENT
        if (req.talkgroup == 0) return {false, "talkgroup must be > 0"};
        std::lock_guard<std::mutex> lk(m_);
        if (calls_.count(req.call_id)) return {false, "call already exists"};  // -> ALREADY_EXISTS
        CallStats s; s.call_id = req.call_id;
        calls_[req.call_id] = s;
        return {true, ""};
    }
    bool end_call(const std::string& call_id) {                                 // false -> NOT_FOUND
        std::vector<std::pair<StatsSink, CallStats>> notify;
        {
            std::lock_guard<std::mutex> lk(m_);
            auto it = calls_.find(call_id);
            if (it == calls_.end()) return false;
            for (auto sit = subs_.begin(); sit != subs_.end();)
                if (sit->second.call_id == call_id) { notify.emplace_back(sit->second.sink, it->second); sit = subs_.erase(sit); } else ++sit;
            calls_.erase(it);
        }
        for (auto& n : notify) n.first(n.second, true);                         // final message, outside the lock (sinks may call back in)
        return true;
    }
    void record_packet(const std::string& call_id, bool lost, double jitter_ms) {
        std::lock_guard<std::mutex> lk(m_);
        auto it = calls_.find(call_id);
        if (it == calls_.end()) return;
        ++it->second.packets; if (lost) ++it->second.lost; it->second.jitter_ms = jitter_ms;
    }
    std::optional<CallStats> snapshot(const std::string& call_id) const {
        std::lock_guard<std::mutex> lk(m_);
        auto it = calls_.find(call_id);
        if (it == calls_.end()) return std::nullopt;
        return it->second;
    }
    // Server-streaming WatchStats: returns a subscription id, or -1 if the call does not exist.
    int subscribe(const std::string& call_id, StatsSink sink) {
        std::lock_guard<std::mutex> lk(m_);
        if (!calls_.count(call_id)) return -1;
        int id = next_id_++;
        subs_[id] = {call_id, std::move(sink)};
        return id;
    }
    void unsubscribe(int id) { std::lock_guard<std::mutex> lk(m_); subs_.erase(id); }
    // Called by a timer every interval: push a snapshot to every subscriber; drop those whose sink returns false.
    void publish() {
        std::vector<std::tuple<int, StatsSink, CallStats>> work;
        {
            std::lock_guard<std::mutex> lk(m_);
            for (auto& [id, s] : subs_) { auto it = calls_.find(s.call_id); if (it != calls_.end()) work.emplace_back(id, s.sink, it->second); }
        }
        for (auto& [id, sink, st] : work) if (!sink(st, false)) unsubscribe(id);
    }
    size_t active_calls() const { std::lock_guard<std::mutex> lk(m_); return calls_.size(); }
    size_t subscriptions() const { std::lock_guard<std::mutex> lk(m_); return subs_.size(); }
private:
    struct Sub { std::string call_id; StatsSink sink; };
    mutable std::mutex m_;
    std::map<std::string, CallStats> calls_;
    std::map<int, Sub> subs_;
    int next_id_ = 1;
};

}  // namespace ether
