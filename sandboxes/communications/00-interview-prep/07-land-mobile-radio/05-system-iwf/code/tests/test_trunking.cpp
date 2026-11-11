// Worksheet 05, Q7 / Q11 - trunking controller. Synthetic IDs only.
#include <gtest/gtest.h>

#include "lmrsys/trunking.hpp"

using lmrsys::GrantKind;
using lmrsys::TrunkController;

TEST(Affiliation, UnitHasAtMostOneGroup) {
  TrunkController c(2);
  c.affiliate(10, 1);
  c.affiliate(11, 1);
  EXPECT_EQ(c.affiliated_count(1), 2u);
  c.affiliate(10, 2);  // moves unit 10
  EXPECT_EQ(c.affiliated_count(1), 1u);
  EXPECT_EQ(c.affiliated_count(2), 1u);
  EXPECT_TRUE(c.is_affiliated(10, 2));
  EXPECT_FALSE(c.is_affiliated(10, 1));
  c.deaffiliate(11);
  EXPECT_EQ(c.affiliated_count(1), 0u);
}

TEST(Grants, DeniedWhenNotAffiliated) {
  TrunkController c(2);
  c.affiliate(1, 100);
  EXPECT_EQ(c.request_call(1, 200, false).kind, GrantKind::DeniedNotAffiliated);
  EXPECT_EQ(c.request_call(99, 100, false).kind, GrantKind::DeniedNotAffiliated);
  EXPECT_EQ(c.free_channels(), 2);
}

TEST(Grants, GrantQueueThenPromoteOnRelease) {
  TrunkController c(2);
  c.affiliate(1, 100);
  c.affiliate(2, 200);
  c.affiliate(3, 300);
  auto a = c.request_call(1, 100, false);
  auto b = c.request_call(2, 200, false);
  EXPECT_EQ(a.kind, GrantKind::Granted);
  EXPECT_EQ(a.channel, 0);
  EXPECT_EQ(b.kind, GrantKind::Granted);
  EXPECT_EQ(b.channel, 1);
  EXPECT_EQ(c.request_call(3, 300, false).kind, GrantKind::Queued);
  EXPECT_EQ(c.queue_length(), 1u);
  EXPECT_EQ(c.free_channels(), 0);

  auto promoted = c.release_call(100);
  ASSERT_TRUE(promoted.has_value());
  EXPECT_EQ(promoted->group, 300u);
  EXPECT_EQ(promoted->channel, 0);
  EXPECT_EQ(c.channel_of(300), 0);
  EXPECT_EQ(c.channel_of(100), -1);
  EXPECT_EQ(c.queue_length(), 0u);
}

TEST(Grants, SecondTalkerOnActiveGroupJoinsSameChannel) {
  TrunkController c(2);
  c.affiliate(1, 100);
  c.affiliate(4, 100);
  auto a = c.request_call(1, 100, false);
  auto j = c.request_call(4, 100, false);
  EXPECT_EQ(j.kind, GrantKind::Joined);
  EXPECT_EQ(j.channel, a.channel);
  EXPECT_EQ(c.free_channels(), 1);
}

TEST(Grants, LowestFreeChannelIsReused) {
  TrunkController c(3);
  c.affiliate(1, 1);
  c.affiliate(2, 2);
  c.affiliate(3, 3);
  c.request_call(1, 1, false);                 // ch 0
  c.request_call(2, 2, false);                 // ch 1
  c.release_call(1);                           // ch 0 free again
  auto g = c.request_call(3, 3, false);
  EXPECT_EQ(g.kind, GrantKind::Granted);
  EXPECT_EQ(g.channel, 0);
}

TEST(Emergency, PreemptsNonEmergencyCallWhenFull) {
  TrunkController c(1);
  c.affiliate(1, 100);
  c.affiliate(2, 200);
  EXPECT_EQ(c.request_call(1, 100, false).kind, GrantKind::Granted);
  auto r = c.request_call(2, 200, true);
  EXPECT_EQ(r.kind, GrantKind::Preempted);
  EXPECT_EQ(r.channel, 0);
  EXPECT_EQ(r.preempted_group, 100u);
  EXPECT_EQ(c.channel_of(100), -1);
  EXPECT_EQ(c.channel_of(200), 0);
}

TEST(Emergency, VictimIsLowestNonEmergencyGroupId) {
  TrunkController c(2);
  c.affiliate(1, 500);
  c.affiliate(2, 300);
  c.affiliate(3, 900);
  c.request_call(1, 500, false);  // ch 0
  c.request_call(2, 300, false);  // ch 1
  auto r = c.request_call(3, 900, true);
  EXPECT_EQ(r.kind, GrantKind::Preempted);
  EXPECT_EQ(r.preempted_group, 300u);
  EXPECT_EQ(r.channel, 1);
}

TEST(Emergency, CannotPreemptAnotherEmergencyAndIsQueued) {
  TrunkController c(1);
  c.affiliate(1, 100);
  c.affiliate(2, 200);
  EXPECT_EQ(c.request_call(1, 100, true).kind, GrantKind::Granted);
  EXPECT_EQ(c.request_call(2, 200, true).kind, GrantKind::Queued);
  EXPECT_EQ(c.channel_of(100), 0);
}

TEST(Queue, GroupIsQueuedOnceAndEmergencyEntryJumpsTheLine) {
  TrunkController c(1);
  c.affiliate(1, 1);
  c.affiliate(2, 2);
  c.affiliate(3, 3);
  EXPECT_EQ(c.request_call(1, 1, true).kind, GrantKind::Granted);   // emergency holds the channel
  EXPECT_EQ(c.request_call(2, 2, false).kind, GrantKind::Queued);
  EXPECT_EQ(c.request_call(2, 2, false).kind, GrantKind::Queued);   // duplicate: still one entry
  EXPECT_EQ(c.queue_length(), 1u);
  EXPECT_EQ(c.request_call(3, 3, true).kind, GrantKind::Queued);    // cannot preempt an emergency
  EXPECT_EQ(c.queue_length(), 2u);
  auto p = c.release_call(1);
  ASSERT_TRUE(p.has_value());
  EXPECT_EQ(p->group, 3u);  // emergency entry first, although group 2 waited longer
}

TEST(Queue, ReleasingAQueuedGroupJustRemovesIt) {
  TrunkController c(1);
  c.affiliate(1, 1);
  c.affiliate(2, 2);
  c.request_call(1, 1, false);
  c.request_call(2, 2, false);
  EXPECT_EQ(c.queue_length(), 1u);
  EXPECT_FALSE(c.release_call(2).has_value());
  EXPECT_EQ(c.queue_length(), 0u);
  EXPECT_EQ(c.channel_of(1), 0);
}

TEST(Queue, ReleasingUnknownGroupIsHarmless) {
  TrunkController c(1);
  EXPECT_FALSE(c.release_call(12345).has_value());
  EXPECT_EQ(c.free_channels(), 1);
}
