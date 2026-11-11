// gateway_core.hpp -- REFERENCE SOLUTION for Capstone M4: the transport-independent core behind the gateway's gRPC API.
//
// Why a "core" separate from gRPC?  Everything that can go wrong in a control plane (validation, duplicate calls,
// capacity, floor arbitration, stats arithmetic, retry rules) is plain logic.  gRPC only adds a wire.  We unit-test the
// logic hermetically and keep the gRPC adapter in grpc/ so thin that it has nothing left to get wrong.
// Header-only, C++17.  Synthetic traffic only - nothing here touches a radio.
#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace gw {

// ===================================================================== status codes
// Numeric values deliberately equal grpc::StatusCode so the adapter is a static_cast.   [VERIFY against grpc/status_code_enum.h]
enum class Code : int {
  Ok = 0,
  InvalidArgument = 3,
  DeadlineExceeded = 4,
  NotFound = 5,
  AlreadyExists = 6,
  ResourceExhausted = 8,
  FailedPrecondition = 9,
  Unavailable = 14,
};
struct Status {
  Code code = Code::Ok;
  std::string message;
  bool ok() const { return code == Code::Ok; }
};
inline Status make_status(Code c, std::string m = {}) { return Status{c, std::move(m)}; }

// ===================================================================== floor control (W 3.1 - 3.3)
enum class FloorResult { Granted, Queued, Denied, AlreadyHolder, Preempted };
enum class FloorEventKind { Granted, Revoked, Released, TimedOut, Queued, Idle };
struct FloorEvent {
  FloorEventKind kind;
  std::string user;
  bool operator==(const FloorEvent& o) const { return kind == o.kind && user == o.user; }
};
constexpr int kPriorityMin = 0;
constexpr int kPriorityNormal = 0;
constexpr int kPriorityEmergency = 7;

// One talkgroup's floor: exactly one talker at a time.
class FloorController {
 public:
  FloorController(uint32_t max_talk_ms, size_t queue_capacity) : max_talk_ms_(max_talk_ms), cap_(queue_capacity) {}

  FloorResult request(const std::string& user, int priority) {
    if (user.empty() || priority < kPriorityMin || priority > kPriorityEmergency) return FloorResult::Denied;
    if (!holder_) {
      grant(user, priority);
      return FloorResult::Granted;
    }
    if (*holder_ == user) return FloorResult::AlreadyHolder;
    if (priority > holder_prio_) {  // strictly higher priority takes the floor; the old talker is NOT queued
      events_.push_back({FloorEventKind::Revoked, *holder_});
      erase_waiter(user);
      grant(user, priority);
      return FloorResult::Preempted;
    }
    for (const auto& w : queue_)
      if (w.user == user) return FloorResult::Queued;  // idempotent: no duplicate entries
    if (queue_.size() >= cap_) return FloorResult::Denied;
    queue_.push_back({user, priority, order_++});
    events_.push_back({FloorEventKind::Queued, user});
    return FloorResult::Queued;
  }

  // Holder releases -> next waiter (highest priority, then oldest) gets the floor.  A waiter may also withdraw.
  bool release(const std::string& user) {
    if (holder_ && *holder_ == user) {
      events_.push_back({FloorEventKind::Released, user});
      holder_.reset();
      grant_next();
      return true;
    }
    return erase_waiter(user);
  }

  // Talk-burst timer: a stuck/forgotten PTT must not hold the group forever.
  void tick(uint32_t ms) {
    if (!holder_) return;
    held_ms_ += ms;
    if (held_ms_ >= max_talk_ms_) {
      events_.push_back({FloorEventKind::TimedOut, *holder_});
      holder_.reset();
      grant_next();
    }
  }

  std::optional<std::string> holder() const { return holder_; }
  size_t queue_size() const { return queue_.size(); }
  std::vector<std::string> queue_order() const {  // who is next, in grant order
    auto q = queue_;
    std::stable_sort(q.begin(), q.end(), [](const Waiter& a, const Waiter& b) { return a.prio != b.prio ? a.prio > b.prio : a.order < b.order; });
    std::vector<std::string> out;
    for (const auto& w : q) out.push_back(w.user);
    return out;
  }
  std::vector<FloorEvent> drain_events() {
    std::vector<FloorEvent> e;
    e.swap(events_);
    return e;
  }

 private:
  struct Waiter {
    std::string user;
    int prio;
    uint64_t order;
  };
  void grant(const std::string& user, int prio) {
    holder_ = user;
    holder_prio_ = prio;
    held_ms_ = 0;
    events_.push_back({FloorEventKind::Granted, user});
  }
  void grant_next() {
    if (queue_.empty()) {
      events_.push_back({FloorEventKind::Idle, ""});
      return;
    }
    auto best = std::min_element(queue_.begin(), queue_.end(), [](const Waiter& a, const Waiter& b) { return a.prio != b.prio ? a.prio > b.prio : a.order < b.order; });
    Waiter w = *best;
    queue_.erase(best);
    grant(w.user, w.prio);
  }
  bool erase_waiter(const std::string& user) {
    for (auto it = queue_.begin(); it != queue_.end(); ++it)
      if (it->user == user) {
        queue_.erase(it);
        return true;
      }
    return false;
  }
  uint32_t max_talk_ms_;
  size_t cap_;
  std::optional<std::string> holder_;
  int holder_prio_ = 0;
  uint32_t held_ms_ = 0;
  std::vector<Waiter> queue_;
  uint64_t order_ = 0;
  std::vector<FloorEvent> events_;
};

// ===================================================================== call table + stats (W 3.4 - 3.5)
struct CallStats {
  std::string call_id;
  uint32_t talkgroup = 0;
  std::string caller;
  uint32_t packets = 0;      // frames received
  uint32_t lost = 0;         // frames declared lost
  double loss_pct = 0;       // lost / (packets + lost) * 100
  uint64_t bits_checked = 0;
  uint64_t bit_errors = 0;
  double ber = 0;            // bit_errors / bits_checked
  double jitter_ms = 0;      // RFC 3550 style smoothed estimate
};

class GatewayService {
 public:
  explicit GatewayService(size_t max_calls = 8, uint32_t max_talk_ms = 30000, size_t floor_queue = 4)
      : max_calls_(max_calls), max_talk_ms_(max_talk_ms), floor_queue_(floor_queue) {}

  // InvalidArgument: empty / >64-char id, talkgroup outside 1..65535, empty caller.
  // Retrying the IDENTICAL request is Ok (idempotent); the same id with different parameters is AlreadyExists.
  // A full table is ResourceExhausted.
  Status start_call(const std::string& call_id, uint32_t talkgroup, const std::string& caller) {
    if (call_id.empty() || call_id.size() > 64) return make_status(Code::InvalidArgument, "call_id must be 1..64 chars");
    if (talkgroup < 1 || talkgroup > 65535) return make_status(Code::InvalidArgument, "talkgroup must be 1..65535");
    if (caller.empty()) return make_status(Code::InvalidArgument, "caller required");
    auto it = calls_.find(call_id);
    if (it != calls_.end()) {
      if (it->second.stats.talkgroup == talkgroup && it->second.stats.caller == caller) return make_status(Code::Ok, "already started");
      return make_status(Code::AlreadyExists, "call_id in use with different parameters");
    }
    if (calls_.size() >= max_calls_) return make_status(Code::ResourceExhausted, "call table full");
    Call c{CallStats{}, FloorController(max_talk_ms_, floor_queue_)};
    c.stats.call_id = call_id;
    c.stats.talkgroup = talkgroup;
    c.stats.caller = caller;
    calls_.emplace(call_id, std::move(c));
    return make_status(Code::Ok);
  }
  Status end_call(const std::string& call_id) {
    return calls_.erase(call_id) ? make_status(Code::Ok) : make_status(Code::NotFound, "no such call");
  }

  // One received/lost frame.  received: packets++, bits += bits_checked, errors += bit_errors,
  //   jitter += (jitter_sample_ms - jitter) / 16        (RFC 3550 section 6.4.1 smoothing)
  // lost: lost++ only.
  Status record_frame(const std::string& call_id, bool received, uint32_t bits_checked, uint32_t bit_errors, double jitter_sample_ms) {
    auto it = calls_.find(call_id);
    if (it == calls_.end()) return make_status(Code::NotFound, "no such call");
    CallStats& s = it->second.stats;
    if (received) {
      ++s.packets;
      s.bits_checked += bits_checked;
      s.bit_errors += bit_errors;
      s.jitter_ms += (jitter_sample_ms - s.jitter_ms) / 16.0;
    } else {
      ++s.lost;
    }
    return make_status(Code::Ok);
  }
  // Derived fields (loss_pct, ber) are computed here, never stored: one source of truth.  nullopt if no such call.
  std::optional<CallStats> snapshot(const std::string& call_id) const {
    auto it = calls_.find(call_id);
    if (it == calls_.end()) return std::nullopt;
    CallStats s = it->second.stats;
    const uint64_t total = static_cast<uint64_t>(s.packets) + s.lost;
    s.loss_pct = total ? 100.0 * static_cast<double>(s.lost) / static_cast<double>(total) : 0.0;
    s.ber = s.bits_checked ? static_cast<double>(s.bit_errors) / static_cast<double>(s.bits_checked) : 0.0;
    return s;
  }

  // ---- floor glue (given) ----
  Status floor_request(const std::string& call_id, const std::string& user, int priority, FloorResult* out = nullptr) {
    auto it = calls_.find(call_id);
    if (it == calls_.end()) return make_status(Code::NotFound, "no such call");
    FloorResult r = it->second.floor.request(user, priority);
    if (out) *out = r;
    return r == FloorResult::Denied ? make_status(Code::ResourceExhausted, "floor request denied") : make_status(Code::Ok);
  }
  Status floor_release(const std::string& call_id, const std::string& user) {
    auto it = calls_.find(call_id);
    if (it == calls_.end()) return make_status(Code::NotFound, "no such call");
    return it->second.floor.release(user) ? make_status(Code::Ok) : make_status(Code::FailedPrecondition, "user neither holds nor awaits the floor");
  }
  void tick(uint32_t ms) {
    for (auto& kv : calls_) kv.second.floor.tick(ms);
  }
  std::optional<std::string> floor_holder(const std::string& call_id) const {
    auto it = calls_.find(call_id);
    return it == calls_.end() ? std::nullopt : it->second.floor.holder();
  }
  std::vector<FloorEvent> floor_events(const std::string& call_id) {
    auto it = calls_.find(call_id);
    return it == calls_.end() ? std::vector<FloorEvent>{} : it->second.floor.drain_events();
  }
  size_t call_count() const { return calls_.size(); }

 private:
  struct Call {
    CallStats stats;
    FloorController floor;
  };
  size_t max_calls_;
  uint32_t max_talk_ms_;
  size_t floor_queue_;
  std::map<std::string, Call> calls_;
};

// ===================================================================== client-side resilience (W 3.6)
// Exponential backoff: min(cap, base * 2^attempt) milliseconds, attempt counted from 0.  Must not overflow for big attempts.
inline uint32_t retry_backoff_ms(unsigned attempt, uint32_t base_ms, uint32_t cap_ms) {
  uint64_t d = base_ms;
  for (unsigned i = 0; i < attempt && d < cap_ms; ++i) d *= 2;
  return static_cast<uint32_t>(std::min<uint64_t>(d, cap_ms));
}
// Which failures may a client retry automatically?  Unavailable: yes (nothing was processed).  DeadlineExceeded: only if the
// RPC is idempotent (the server may have done the work).  Everything else is a verdict, not a hiccup: no.
inline bool is_retryable(Code c, bool idempotent) {
  if (c == Code::Unavailable) return true;
  if (c == Code::DeadlineExceeded) return idempotent;
  return false;
}

}  // namespace gw
