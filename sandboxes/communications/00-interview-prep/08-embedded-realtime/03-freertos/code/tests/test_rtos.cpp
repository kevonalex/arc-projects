// Tests for Pack 08/03. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <vector>

#include "rtos_sim.hpp"

using namespace rtos;

namespace {
// Mars-Pathfinder-shaped scenario. Task indices: 0 = H (prio 3), 1 = M (prio 2), 2 = L (prio 1). Mutex 0 = bus lock.
//   L (t=0): lock, run 4, unlock.     H (t=1): lock, run 1, unlock.     M (t=2): run 6 (CPU-bound, no lock).
std::vector<TaskSpec> pathfinder() {
  return {
      {"H", 3, 1, {{Op::Lock, 0}, {Op::Run, 1}, {Op::Unlock, 0}}},
      {"M", 2, 2, {{Op::Run, 6}}},
      {"L", 1, 0, {{Op::Lock, 0}, {Op::Run, 4}, {Op::Unlock, 0}}},
  };
}
}  // namespace

// ---------------------------------------------------------------- scheduler policy (W 3.5)
TEST(Policy, HighestPriorityReadyTaskWins) {
  Sched sc;
  sc.owner.assign(1, -1);
  sc.t.resize(3);
  sc.t[0].prio = 1;
  sc.t[1].prio = 5;
  sc.t[2].prio = 5;
  EXPECT_EQ(pick_next(sc, 0), 1);  // tie between 1 and 2 -> lowest index
  sc.t[1].blocked_on = 0;          // blocked tasks are not candidates
  EXPECT_EQ(pick_next(sc, 0), 2);
  sc.t[2].wake = 10;               // sleeping tasks are not candidates
  EXPECT_EQ(pick_next(sc, 3), 0);
  EXPECT_EQ(pick_next(sc, 10), 2);
  sc.t[0].done = true;
  sc.t[2].done = true;
  EXPECT_EQ(pick_next(sc, 99), -1);  // nobody ready: idle
}
TEST(Policy, NotYetReleasedTasksAreNotReady) {
  Sched sc;
  sc.t.resize(2);
  sc.t[0].prio = 1;
  sc.t[1].prio = 9;
  sc.t[1].start = 5;
  EXPECT_EQ(pick_next(sc, 2), 0);
  EXPECT_EQ(pick_next(sc, 5), 1);
}

// ---------------------------------------------------------------- inheritance (W 3.6)
TEST(Inheritance, OneHop) {
  Sched sc;
  sc.owner = {0};  // task 0 (prio 1) owns mutex 0
  sc.t.resize(2);
  sc.t[0].prio = 1;
  sc.t[1].prio = 3;
  sc.t[1].blocked_on = 0;
  sc.inheritance = false;
  EXPECT_EQ(effective_priority(sc, 0), 1);
  sc.inheritance = true;
  EXPECT_EQ(effective_priority(sc, 0), 3);
  EXPECT_EQ(effective_priority(sc, 1), 3);
}
TEST(Inheritance, TransitiveChain) {
  // T3(prio 3) waits on B owned by T2(prio 2), which waits on A owned by T1(prio 1): T1 must run at 3.
  Sched sc;
  sc.inheritance = true;
  sc.owner = {0, 1};  // mutex 0 (A) owned by task 0 (T1); mutex 1 (B) owned by task 1 (T2)
  sc.t.resize(3);
  sc.t[0].prio = 1;
  sc.t[1].prio = 2;
  sc.t[2].prio = 3;
  sc.t[1].blocked_on = 0;
  sc.t[2].blocked_on = 1;
  EXPECT_EQ(effective_priority(sc, 0), 3);
  EXPECT_EQ(effective_priority(sc, 1), 3);
  sc.inheritance = false;
  EXPECT_EQ(effective_priority(sc, 0), 1);
}
TEST(Inheritance, FinishedWaitersDoNotBoost) {
  Sched sc;
  sc.inheritance = true;
  sc.owner = {0};
  sc.t.resize(2);
  sc.t[0].prio = 1;
  sc.t[1].prio = 7;
  sc.t[1].blocked_on = 0;
  sc.t[1].done = true;
  EXPECT_EQ(effective_priority(sc, 0), 1);
}
TEST(Inheritance, DeadlockCycleTerminates) {
  Sched sc;
  sc.inheritance = true;
  sc.owner = {0, 1};
  sc.t.resize(2);
  sc.t[0].prio = 1;
  sc.t[1].prio = 2;
  sc.t[0].blocked_on = 1;  // T0 waits for B (owned by T1)
  sc.t[1].blocked_on = 0;  // T1 waits for A (owned by T0): classic deadlock
  EXPECT_GE(effective_priority(sc, 0), 2);  // must return, not recurse forever
}

// ---------------------------------------------------------------- Mars Pathfinder (W 3.5)
TEST(Pathfinder, WithoutInheritanceTheMediumTaskStarvesTheHighTask) {
  auto specs = pathfinder();
  Result r = simulate(specs, 1, false, 50);
  const std::vector<int> want = {2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 0};  // L L M M M M M M L L H
  EXPECT_EQ(r.trace, want);
  ASSERT_EQ(r.finish.size(), 3u);
  EXPECT_EQ(r.finish[0], 11);  // H
  EXPECT_EQ(r.finish[1], 8);   // M finishes BEFORE the higher-priority H: priority inversion
  EXPECT_EQ(r.finish[2], 10);  // L
  EXPECT_EQ(response_ticks(r, specs, 0), 10);  // H needed 1 tick of CPU but took 10 ticks end to end
}
TEST(Pathfinder, WithInheritanceTheLockHolderIsBoosted) {
  auto specs = pathfinder();
  Result r = simulate(specs, 1, true, 50);
  const std::vector<int> want = {2, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1};  // L L L L H M M M M M M
  EXPECT_EQ(r.trace, want);
  EXPECT_EQ(r.finish[2], 4);   // L
  EXPECT_EQ(r.finish[0], 5);   // H
  EXPECT_EQ(r.finish[1], 11);  // M waits; it was never entitled to run ahead of H
  EXPECT_EQ(response_ticks(r, specs, 0), 4);
  EXPECT_LT(response_ticks(r, specs, 0), 10);
}
TEST(Pathfinder, DeadlineOfSixTicks) {
  const int deadline = 6;  // the watchdog resets the system if H has not finished 6 ticks after release
  auto specs = pathfinder();
  EXPECT_GT(response_ticks(simulate(specs, 1, false, 50), specs, 0), deadline);   // reset!
  EXPECT_LE(response_ticks(simulate(specs, 1, true, 50), specs, 0), deadline);    // rover keeps driving
}
TEST(Engine, DelayAndIdleTicks) {
  std::vector<TaskSpec> specs = {{"T", 1, 0, {{Op::Delay, 3}, {Op::Run, 2}}}};
  Result r = simulate(specs, 0, false, 20);
  const std::vector<int> want = {-1, -1, -1, 0, 0};
  EXPECT_EQ(r.trace, want);
  EXPECT_EQ(r.finish[0], 5);
}

// ---------------------------------------------------------------- primitives (W 3.2 - 3.4)
TEST(Queue, BoundedFifoCopyInCopyOut) {
  BoundedQueue<int> q(2);
  EXPECT_TRUE(q.send(10));
  EXPECT_TRUE(q.send(20));
  EXPECT_FALSE(q.send(30));  // full: the sender would block (or drop) here
  EXPECT_EQ(q.size(), 2u);
  int v = 0;
  EXPECT_TRUE(q.receive(v));
  EXPECT_EQ(v, 10);
  EXPECT_TRUE(q.send(30));
  EXPECT_TRUE(q.receive(v));
  EXPECT_EQ(v, 20);
  EXPECT_TRUE(q.receive(v));
  EXPECT_EQ(v, 30);
  EXPECT_FALSE(q.receive(v));  // empty
  EXPECT_EQ(q.size(), 0u);
}
TEST(Semaphore, BinaryVersusCounting) {
  Semaphore bin(0, 1);
  EXPECT_FALSE(bin.take());   // an ISR has not signalled yet
  EXPECT_TRUE(bin.give());    // ISR: "data ready"
  EXPECT_FALSE(bin.give());   // a second give while pending is absorbed: events can be LOST
  EXPECT_EQ(bin.count(), 1);
  EXPECT_TRUE(bin.take());
  EXPECT_FALSE(bin.take());
  Semaphore cnt(0, 4);        // counting: remembers up to 4 events
  for (int i = 0; i < 4; ++i) EXPECT_TRUE(cnt.give());
  EXPECT_FALSE(cnt.give());
  EXPECT_EQ(cnt.count(), 4);
}
TEST(Mutex, OnlyTheOwnerMayUnlock) {
  Mutex m;
  EXPECT_EQ(m.owner(), -1);
  EXPECT_TRUE(m.lock(7));
  EXPECT_EQ(m.owner(), 7);
  EXPECT_FALSE(m.lock(8));    // taken
  EXPECT_FALSE(m.lock(7));    // non-recursive: owner locking again fails
  EXPECT_FALSE(m.unlock(8));  // a non-owner cannot release it (a semaphore would allow this)
  EXPECT_TRUE(m.unlock(7));
  EXPECT_EQ(m.owner(), -1);
  EXPECT_TRUE(m.lock(8));
}
