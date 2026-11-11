// gateway_core.hpp -- STUBS for Capstone M4 (student version). Reference: ../solutions/gateway_core.hpp
//
// The transport-independent core behind the gateway's gRPC API: validation, call table, floor arbitration, stats, retry rules.
// Header-only, C++17.  Synthetic traffic only.
#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace gw {

// Numeric values equal grpc::StatusCode so the adapter is a static_cast.   [VERIFY against grpc/status_code_enum.h]
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

  // Rules (W 3.1):
  //  - empty user or priority outside 0..7            -> Denied
  //  - floor idle                                      -> Granted          (event Granted)
  //  - requester is already the holder                 -> AlreadyHolder
  //  - priority STRICTLY higher than holder's          -> Preempted: event Revoked(old holder) then Granted(new); the old holder is NOT
  //                                                        re-queued; if the requester was waiting, remove it from the queue
  //  - otherwise already queued                        -> Queued (no duplicate entry, no new event)
  //  - queue full                                      -> Denied
  //  - otherwise append to the queue, event Queued     -> Queued
  FloorResult request([[maybe_unused]] const std::string& user, [[maybe_unused]] int priority) {
    return FloorResult::Denied;  // TODO W 3.1
  }
  // Holder releases: event Released, floor freed, then GRANT THE NEXT waiter (highest priority first, oldest first within a
  // priority) -> event Granted; if nobody waits -> event Idle.  A queued user may also withdraw (just removed, no events).
  // Returns false if the user neither holds nor awaits the floor.
  bool release([[maybe_unused]] const std::string& user) {
    return false;  // TODO W 3.2
  }
  // Talk-burst timer: holder_ has held the floor for `held_ms_ += ms`; once held_ms_ >= max_talk_ms: event TimedOut(holder), then
  // grant the next waiter exactly as release() does.  No holder => nothing happens.  A new grant resets held_ms_.
  void tick([[maybe_unused]] uint32_t ms) {
    // TODO W 3.3
  }

  std::optional<std::string> holder() const { return holder_; }
  size_t queue_size() const { return queue_.size(); }
  std::vector<std::string> queue_order() const {  // given: who is next, in grant order
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
  // Free to use in your implementation (members are yours to change):
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
  double ber = 0;            // bit_errors / bits_checked   (0 if nothing checked)
  double jitter_ms = 0;      // RFC 3550 style smoothed estimate
};

class GatewayService {
 public:
  explicit GatewayService(size_t max_calls = 8, uint32_t max_talk_ms = 30000, size_t floor_queue = 4)
      : max_calls_(max_calls), max_talk_ms_(max_talk_ms), floor_queue_(floor_queue) {}

  // InvalidArgument: empty / >64-char id, talkgroup outside 1..65535, empty caller (check in that order).
  // The IDENTICAL request repeated is Ok (idempotent - safe to retry); same id with different parameters: AlreadyExists.
  // A full table (calls_.size() >= max_calls_) for a NEW call: ResourceExhausted.  Create the call with its FloorController.
  Status start_call([[maybe_unused]] const std::string& call_id, [[maybe_unused]] uint32_t talkgroup, [[maybe_unused]] const std::string& caller) {
    return make_status(Code::Unavailable, "TODO");  // TODO W 3.4
  }
  Status end_call([[maybe_unused]] const std::string& call_id) { return make_status(Code::Unavailable, "TODO"); }  // TODO W 3.4: NotFound if absent

  // received: packets++, bits_checked += bits, bit_errors += errs, jitter += (sample - jitter)/16.   lost: lost++ only.   NotFound if no call.
  Status record_frame([[maybe_unused]] const std::string& call_id, [[maybe_unused]] bool received, [[maybe_unused]] uint32_t bits_checked,
                      [[maybe_unused]] uint32_t bit_errors, [[maybe_unused]] double jitter_sample_ms) {
    return make_status(Code::Unavailable, "TODO");  // TODO W 3.5
  }
  // Copy of the stored stats with loss_pct and ber DERIVED here (never stored).  nullopt if no such call.
  std::optional<CallStats> snapshot([[maybe_unused]] const std::string& call_id) const {
    return std::nullopt;  // TODO W 3.5
  }

  // ---- floor glue (given; works once FloorController does) ----
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
// Exponential backoff: min(cap, base * 2^attempt) ms, attempt counted from 0.  Must not overflow for attempt = 1000.
inline uint32_t retry_backoff_ms([[maybe_unused]] unsigned attempt, [[maybe_unused]] uint32_t base_ms, [[maybe_unused]] uint32_t cap_ms) {
  return 0;  // TODO W 3.6
}
// Unavailable: true.  DeadlineExceeded: only if `idempotent`.  Everything else: false.
inline bool is_retryable([[maybe_unused]] Code c, [[maybe_unused]] bool idempotent) {
  return false;  // TODO W 3.6
}

}  // namespace gw
