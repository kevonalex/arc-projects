// Tests for Capstone M4. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "gateway_core.hpp"

using namespace gw;
using K = FloorEventKind;

namespace {
std::vector<FloorEvent> ev(std::initializer_list<FloorEvent> l) { return std::vector<FloorEvent>(l); }
std::string who(const FloorController& f) { return f.holder() ? *f.holder() : std::string("<none>"); }
}  // namespace

// ---------------------------------------------------------------- floor control (W 3.1 - 3.3)
TEST(Floor, IdleFloorIsGrantedAndSecondTalkerQueues) {
  FloorController f(30000, 4);
  EXPECT_EQ(f.request("alice", 0), FloorResult::Granted);
  ASSERT_TRUE(f.holder().has_value());
  EXPECT_EQ(who(f), "alice");
  EXPECT_EQ(f.request("alice", 0), FloorResult::AlreadyHolder);
  EXPECT_EQ(f.request("bob", 0), FloorResult::Queued);
  EXPECT_EQ(f.request("bob", 0), FloorResult::Queued);  // idempotent
  EXPECT_EQ(f.queue_size(), 1u);
  EXPECT_EQ(f.drain_events(), ev({{K::Granted, "alice"}, {K::Queued, "bob"}}));
}
TEST(Floor, InvalidRequestsAreDenied) {
  FloorController f(30000, 1);
  EXPECT_EQ(f.request("", 0), FloorResult::Denied);
  EXPECT_EQ(f.request("a", -1), FloorResult::Denied);
  EXPECT_EQ(f.request("a", 8), FloorResult::Denied);
  EXPECT_FALSE(f.holder().has_value());
  EXPECT_EQ(f.request("a", 0), FloorResult::Granted);
  EXPECT_EQ(f.request("b", 0), FloorResult::Queued);
  EXPECT_EQ(f.request("c", 0), FloorResult::Denied);  // queue capacity 1
}
TEST(Floor, ReleaseGrantsHighestPriorityThenOldest) {
  FloorController f(30000, 8);
  ASSERT_EQ(f.request("holder", 7), FloorResult::Granted);  // emergency holder: nobody below can preempt
  f.request("n1", 0);
  f.request("n2", 0);
  f.request("p3", 3);
  f.request("n3", 0);
  f.request("p3b", 3);
  EXPECT_EQ(f.queue_order(), (std::vector<std::string>{"p3", "p3b", "n1", "n2", "n3"}));
  f.drain_events();
  EXPECT_TRUE(f.release("holder"));
  ASSERT_TRUE(f.holder().has_value());
  EXPECT_EQ(who(f), "p3");
  EXPECT_EQ(f.drain_events(), ev({{K::Released, "holder"}, {K::Granted, "p3"}}));
  EXPECT_TRUE(f.release("p3"));
  EXPECT_EQ(who(f), "p3b");
}
TEST(Floor, ReleaseWithNobodyWaitingGoesIdle) {
  FloorController f(30000, 4);
  f.request("a", 0);
  f.drain_events();
  EXPECT_TRUE(f.release("a"));
  EXPECT_FALSE(f.holder().has_value());
  EXPECT_EQ(f.drain_events(), ev({{K::Released, "a"}, {K::Idle, ""}}));
  EXPECT_FALSE(f.release("a"));  // not holding any more
}
TEST(Floor, WaiterCanWithdrawSilently) {
  FloorController f(30000, 4);
  f.request("a", 0);
  f.request("b", 0);
  f.drain_events();
  EXPECT_TRUE(f.release("b"));
  EXPECT_EQ(f.queue_size(), 0u);
  EXPECT_TRUE(f.drain_events().empty());
  EXPECT_FALSE(f.release("nobody"));
}
TEST(Floor, HigherPriorityPreemptsAndOldHolderIsNotRequeued) {
  FloorController f(30000, 4);
  f.request("a", 1);
  f.request("b", 1);  // equal priority does NOT preempt
  ASSERT_EQ(who(f), "a");
  f.drain_events();
  EXPECT_EQ(f.request("c", 7), FloorResult::Preempted);
  ASSERT_TRUE(f.holder().has_value());
  EXPECT_EQ(who(f), "c");
  EXPECT_EQ(f.drain_events(), ev({{K::Revoked, "a"}, {K::Granted, "c"}}));
  EXPECT_EQ(f.queue_order(), (std::vector<std::string>{"b"}));  // a is gone: it must ask again
  EXPECT_EQ(f.request("d", 7), FloorResult::Queued);            // emergency does not preempt emergency
}
TEST(Floor, PreemptingWaiterLeavesTheQueue) {
  FloorController f(30000, 4);
  f.request("a", 0);
  f.request("b", 0);
  ASSERT_EQ(f.queue_size(), 1u);
  EXPECT_EQ(f.request("b", 5), FloorResult::Preempted);
  EXPECT_EQ(f.queue_size(), 0u);
}
TEST(Floor, TalkBurstTimerRevokesAndPassesTheFloor) {
  FloorController f(1000, 4);
  f.request("a", 0);
  f.request("b", 0);
  f.drain_events();
  f.tick(600);
  ASSERT_TRUE(f.holder().has_value());
  EXPECT_EQ(who(f), "a");
  EXPECT_TRUE(f.drain_events().empty());
  f.tick(400);  // total 1000 ms: limit reached (>=)
  ASSERT_TRUE(f.holder().has_value());
  EXPECT_EQ(who(f), "b");
  EXPECT_EQ(f.drain_events(), ev({{K::TimedOut, "a"}, {K::Granted, "b"}}));
  f.tick(999);  // the new talker's timer restarted from zero
  EXPECT_EQ(who(f), "b");
  f.tick(1);
  EXPECT_FALSE(f.holder().has_value());
  EXPECT_EQ(f.drain_events(), ev({{K::TimedOut, "b"}, {K::Idle, ""}}));
  f.tick(100000);  // idle floor: nothing happens
  EXPECT_TRUE(f.drain_events().empty());
}

// ---------------------------------------------------------------- call table (W 3.4)
TEST(Calls, ValidationOrderAndCodes) {
  GatewayService g;
  EXPECT_EQ(g.start_call("", 100, "alice").code, Code::InvalidArgument);
  EXPECT_EQ(g.start_call(std::string(65, 'x'), 100, "alice").code, Code::InvalidArgument);
  EXPECT_EQ(g.start_call("c1", 0, "alice").code, Code::InvalidArgument);
  EXPECT_EQ(g.start_call("c1", 65536, "alice").code, Code::InvalidArgument);
  EXPECT_EQ(g.start_call("c1", 100, "").code, Code::InvalidArgument);
  EXPECT_EQ(g.call_count(), 0u);
  EXPECT_TRUE(g.start_call("c1", 100, "alice").ok());
  EXPECT_TRUE(g.start_call(std::string(64, 'x'), 65535, "bob").ok());
  EXPECT_EQ(g.call_count(), 2u);
}
TEST(Calls, DuplicateStartIsIdempotentOnlyWhenIdentical) {
  GatewayService g;
  ASSERT_TRUE(g.start_call("c1", 100, "alice").ok());
  EXPECT_TRUE(g.start_call("c1", 100, "alice").ok());                          // retry-safe
  EXPECT_EQ(g.start_call("c1", 101, "alice").code, Code::AlreadyExists);        // conflict
  EXPECT_EQ(g.start_call("c1", 100, "mallory").code, Code::AlreadyExists);
  EXPECT_EQ(g.call_count(), 1u);
}
TEST(Calls, CapacityAndEndCall) {
  GatewayService g(2);
  ASSERT_TRUE(g.start_call("a", 1, "u").ok());
  ASSERT_TRUE(g.start_call("b", 1, "u").ok());
  EXPECT_EQ(g.start_call("c", 1, "u").code, Code::ResourceExhausted);
  EXPECT_TRUE(g.start_call("a", 1, "u").ok());  // existing identical call is not "new": still fine when full
  EXPECT_TRUE(g.end_call("a").ok());
  EXPECT_EQ(g.end_call("a").code, Code::NotFound);
  EXPECT_TRUE(g.start_call("c", 1, "u").ok());
}

// ---------------------------------------------------------------- stats (W 3.5)
TEST(Stats, CountersLossBerAndJitter) {
  GatewayService g;
  ASSERT_TRUE(g.start_call("c", 7, "alice").ok());
  auto s0 = g.snapshot("c");
  ASSERT_TRUE(s0.has_value());
  EXPECT_EQ(s0->packets, 0u);
  EXPECT_DOUBLE_EQ(s0->ber, 0.0);       // no division by zero
  EXPECT_DOUBLE_EQ(s0->loss_pct, 0.0);
  EXPECT_EQ(s0->talkgroup, 7u);
  EXPECT_EQ(s0->caller, "alice");
  ASSERT_TRUE(g.record_frame("c", true, 1000, 10, 16.0).ok());
  ASSERT_TRUE(g.record_frame("c", true, 1000, 0, 0.0).ok());
  ASSERT_TRUE(g.record_frame("c", false, 0, 0, 0.0).ok());
  ASSERT_TRUE(g.record_frame("c", true, 1000, 5, 0.0).ok());
  auto s = g.snapshot("c");
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->call_id, "c");
  EXPECT_EQ(s->packets, 3u);
  EXPECT_EQ(s->lost, 1u);
  EXPECT_NEAR(s->loss_pct, 25.0, 1e-9);
  EXPECT_EQ(s->bits_checked, 3000u);
  EXPECT_EQ(s->bit_errors, 15u);
  EXPECT_NEAR(s->ber, 0.005, 1e-12);
  // jitter: 0 -> 1.0 (16/16) -> 0.9375 -> (lost frame untouched) -> 0.87890625
  EXPECT_NEAR(s->jitter_ms, 0.87890625, 1e-9);
}
TEST(Stats, UnknownCall) {
  GatewayService g;
  EXPECT_EQ(g.record_frame("nope", true, 1, 0, 0).code, Code::NotFound);
  EXPECT_FALSE(g.snapshot("nope").has_value());
  g.start_call("c", 1, "u");
  g.end_call("c");
  EXPECT_FALSE(g.snapshot("c").has_value());
}

// ---------------------------------------------------------------- service-level floor glue (W 3.3)
TEST(ServiceFloor, EndToEndThroughTheCallTable) {
  GatewayService g(8, 500, 2);
  FloorResult r{};
  EXPECT_EQ(g.floor_request("none", "a", 0, &r).code, Code::NotFound);
  ASSERT_TRUE(g.start_call("tg", 1, "dispatcher").ok());
  ASSERT_TRUE(g.floor_request("tg", "a", 0, &r).ok());
  EXPECT_EQ(r, FloorResult::Granted);
  ASSERT_TRUE(g.floor_request("tg", "b", 0, &r).ok());
  EXPECT_EQ(r, FloorResult::Queued);
  EXPECT_EQ(g.floor_release("tg", "zzz").code, Code::FailedPrecondition);
  g.tick(500);
  ASSERT_TRUE(g.floor_holder("tg").has_value());
  EXPECT_EQ(*g.floor_holder("tg"), "b");
  EXPECT_EQ(g.floor_request("tg", "", 0, &r).code, Code::ResourceExhausted);  // Denied maps to ResourceExhausted
  auto events = g.floor_events("tg");
  ASSERT_GE(events.size(), 3u);
  EXPECT_TRUE(g.floor_release("tg", "b").ok());
}

// ---------------------------------------------------------------- resilience (W 3.6)
TEST(Retry, ExponentialBackoffWithCap) {
  EXPECT_EQ(retry_backoff_ms(0, 100, 5000), 100u);
  EXPECT_EQ(retry_backoff_ms(1, 100, 5000), 200u);
  EXPECT_EQ(retry_backoff_ms(2, 100, 5000), 400u);
  EXPECT_EQ(retry_backoff_ms(5, 100, 5000), 3200u);
  EXPECT_EQ(retry_backoff_ms(6, 100, 5000), 5000u);
  EXPECT_EQ(retry_backoff_ms(1000, 100, 5000), 5000u);  // no overflow, no UB
  EXPECT_EQ(retry_backoff_ms(3, 100, 50), 50u);         // cap below base still caps
}
TEST(Retry, WhichFailuresMayBeRetried) {
  EXPECT_TRUE(is_retryable(Code::Unavailable, false));
  EXPECT_TRUE(is_retryable(Code::Unavailable, true));
  EXPECT_FALSE(is_retryable(Code::DeadlineExceeded, false));
  EXPECT_TRUE(is_retryable(Code::DeadlineExceeded, true));
  for (Code c : {Code::InvalidArgument, Code::NotFound, Code::AlreadyExists, Code::ResourceExhausted, Code::FailedPrecondition, Code::Ok})
    EXPECT_FALSE(is_retryable(c, true)) << static_cast<int>(c);
}
TEST(Retry, StatusCodeNumbersMatchGrpc) {
  EXPECT_EQ(static_cast<int>(Code::InvalidArgument), 3);
  EXPECT_EQ(static_cast<int>(Code::DeadlineExceeded), 4);
  EXPECT_EQ(static_cast<int>(Code::NotFound), 5);
  EXPECT_EQ(static_cast<int>(Code::AlreadyExists), 6);
  EXPECT_EQ(static_cast<int>(Code::ResourceExhausted), 8);
  EXPECT_EQ(static_cast<int>(Code::FailedPrecondition), 9);
  EXPECT_EQ(static_cast<int>(Code::Unavailable), 14);
}
