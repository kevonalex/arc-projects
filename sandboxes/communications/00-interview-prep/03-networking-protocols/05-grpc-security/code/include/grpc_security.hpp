// STUB - 03-net/05-grpc-security. Implement every TODO. Tests: tests/test_grpc_security.cpp
// Pure C++17 - no gRPC/protobuf library needed for these tests (the point is to understand the bytes).
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

// ---- varint: 7 payload bits per byte, LEAST-significant group first, bit 7 = "more bytes follow" ----
inline Bytes encode_varint(uint64_t v) { (void)v; return {}; }                                  // TODO
// TODO: nullopt if truncated, longer than 10 bytes, or the 10th byte is > 1 (would overflow 64 bits). Set 'used'.
inline std::optional<uint64_t> decode_varint(const uint8_t* p, size_t n, size_t& used) { (void)p; (void)n; (void)used; return std::nullopt; }
inline uint64_t zigzag_encode(int64_t v) { (void)v; return 0; }                                 // TODO (v << 1) ^ (v >> 63)
inline int64_t zigzag_decode(uint64_t u) { (void)u; return 0; }                                 // TODO (u >> 1) ^ -(u & 1)

enum class WireType : uint8_t { Varint = 0, Fixed64 = 1, Len = 2, Fixed32 = 5 };
inline uint64_t make_tag(uint32_t field, WireType t) { (void)field; (void)t; return 0; }        // TODO (field << 3) | wiretype

// TODO: each method appends tag + value. Fixed-width values are LITTLE-endian. Len = tag, varint length, bytes.
class ProtoWriter {
public:
    void varint(uint32_t field, uint64_t v) { (void)field; (void)v; }
    void sint(uint32_t field, int64_t v) { (void)field; (void)v; }
    void str(uint32_t field, const std::string& s) { (void)field; (void)s; }
    void fixed64(uint32_t field, uint64_t v) { (void)field; (void)v; }
    void fixed32(uint32_t field, uint32_t v) { (void)field; (void)v; }
    void dbl(uint32_t field, double d) { (void)field; (void)d; }
    const Bytes& bytes() const { return buf_; }
private:
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
// TODO next(): decode tag (field 0 invalid), then by wire type: 0 varint, 1 8 bytes, 5 4 bytes, 2 length-delimited
// (declared length must fit the buffer). Wire types 3,4,6,7 and any truncation -> ok()==false and nullopt.
class ProtoReader {
public:
    ProtoReader(const uint8_t* p, size_t n) : p_(p), n_(n) {}
    std::optional<Field> next() { return std::nullopt; }
    bool ok() const { return ok_; }
private:
    const uint8_t* p_; size_t n_; bool ok_ = true;
};

// proto3 semantics: fields equal to their default (0, "", 0.0) are NOT serialised; unknown fields are skipped on decode.
struct StartCallRequest { std::string call_id; uint32_t talkgroup = 0; std::string caller; };            // 1 string, 2 uint32, 3 string
struct CallStats { std::string call_id; uint32_t packets = 0, lost = 0; double jitter_ms = 0; };          // 1 string, 2 uint32, 3 uint32, 4 double
inline Bytes encode(const StartCallRequest& m) { (void)m; return {}; }                                    // TODO
inline std::optional<StartCallRequest> decode_start_call(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }  // TODO (nullopt if reader !ok)
inline Bytes encode(const CallStats& m) { (void)m; return {}; }                                           // TODO
inline std::optional<CallStats> decode_call_stats(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }        // TODO

// ---- gRPC message framing: [compressed flag (1)] [length (4, big-endian)] [payload] ----
inline Bytes grpc_frame(const Bytes& payload, bool compressed = false) { (void)payload; (void)compressed; return {}; }          // TODO
struct GrpcUnframe {
    enum class Status { NeedMore, Ok, TooLarge, BadFlag } status = Status::NeedMore;
    bool compressed = false; Bytes payload; size_t consumed = 0;
};
// TODO: <5 bytes -> NeedMore; flag > 1 -> BadFlag; length > max_len -> TooLarge (before waiting for the body!);
// incomplete body -> NeedMore; else Ok with payload and consumed = 5 + length.
inline GrpcUnframe grpc_unframe(const uint8_t* p, size_t n, size_t max_len = 4 * 1024 * 1024) { (void)p; (void)n; (void)max_len; return {}; }

// ---- HTTP/2 frame header: length(24) type(8) flags(8) R(1) stream_id(31) ----
inline const char* kH2Preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
struct H2FrameHeader { uint32_t length = 0; uint8_t type = 0, flags = 0; uint32_t stream_id = 0; };
inline std::optional<H2FrameHeader> parse_h2_frame_header(const uint8_t* p, size_t n) { (void)p; (void)n; return std::nullopt; }  // TODO mask the reserved bit
inline Bytes build_h2_frame_header(const H2FrameHeader& h) { (void)h; return {}; }                        // TODO (9 bytes)
inline const char* h2_type_name(uint8_t t) { (void)t; return ""; }    // TODO 0 DATA 1 HEADERS 2 PRIORITY 3 RST_STREAM 4 SETTINGS 5 PUSH_PROMISE 6 PING 7 GOAWAY 8 WINDOW_UPDATE 9 CONTINUATION else UNKNOWN

// ---- grpc-timeout: 1..8 ASCII digits + unit H M S m u n ----
inline std::optional<int64_t> parse_grpc_timeout_ns(const std::string& s) { (void)s; return std::nullopt; }      // TODO (reject bad unit, non-digits, >8 digits)
// TODO: coarsest unit that is EXACT and fits 8 digits (100 ms -> "100m", 2 s -> "2S", 60 s -> "1M"); otherwise finest unit that fits, rounded up.
inline std::string format_grpc_timeout(int64_t ns) { (void)ns; return ""; }
inline int64_t remaining_ms(int64_t deadline_total_ms, int64_t elapsed_ms, int64_t margin_ms = 0) { (void)deadline_total_ms; (void)elapsed_ms; (void)margin_ms; return -1; }  // TODO clamp at 0

enum class GrpcStatus : int { Ok = 0, Cancelled = 1, Unknown = 2, InvalidArgument = 3, DeadlineExceeded = 4, NotFound = 5, AlreadyExists = 6,
    PermissionDenied = 7, ResourceExhausted = 8, FailedPrecondition = 9, Aborted = 10, OutOfRange = 11, Unimplemented = 12, Internal = 13,
    Unavailable = 14, DataLoss = 15, Unauthenticated = 16 };
// TODO: 400->Internal, 401->Unauthenticated, 403->PermissionDenied, 404->Unimplemented, 429/502/503/504->Unavailable, else Unknown
inline GrpcStatus grpc_status_from_http(int http) { (void)http; return GrpcStatus::Ok; }

// ---- SRTP (RFC 3711): 48-bit packet index = 65536*ROC + SEQ; only SEQ is on the wire ----
// TODO estimate(): if never initialised return seq. s_l = last seq. If s_l < 32768: v = (seq - s_l > 32768) ? (roc-1) mod 2^32 : roc
//      else: v = (s_l - 32768 > seq) ? roc+1 : roc.  index = (v << 16) | seq.
// TODO update(seq): first packet initialises s_l; afterwards advance (roc, s_l) only if the estimated index is HIGHER than the current one.
class SrtpIndex {
public:
    uint64_t estimate(uint16_t seq) const { (void)seq; return 0; }
    void update(uint16_t seq) { (void)seq; }
    uint64_t roc() const { return 0; }
};
// TODO 64-packet window. check(): true if new (> highest) or within the window and not yet seen. update(): slide / set bit.
class ReplayWindow {
public:
    bool check(uint64_t idx) const { (void)idx; return false; }
    void update(uint64_t idx) { (void)idx; }
};

// ---- CallControl service logic (the body of the gRPC handlers) ----
// start_call: empty call_id -> {false,"call_id required"}; talkgroup 0 -> {false,"talkgroup must be > 0"}; duplicate -> {false,"call already exists"}
// end_call: false if unknown; subscribers get ONE final message (final_message=true) and are removed; call sinks OUTSIDE the lock.
// subscribe: -1 if the call does not exist. publish(): send a snapshot to each subscriber of an existing call; remove subscribers whose sink returns false.
class CallControlService {
public:
    struct StartResult { bool accepted = false; std::string reason; };
    using StatsSink = std::function<bool(const CallStats&, bool final_message)>;
    StartResult start_call(const StartCallRequest& req) { (void)req; return {}; }
    bool end_call(const std::string& call_id) { (void)call_id; return false; }
    void record_packet(const std::string& call_id, bool lost, double jitter_ms) { (void)call_id; (void)lost; (void)jitter_ms; }
    std::optional<CallStats> snapshot(const std::string& call_id) const { (void)call_id; return std::nullopt; }
    int subscribe(const std::string& call_id, StatsSink sink) { (void)call_id; (void)sink; return -1; }
    void unsubscribe(int id) { (void)id; }
    void publish() {}
    size_t active_calls() const { return 0; }
    size_t subscriptions() const { return 0; }
};

}  // namespace ether
