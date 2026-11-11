// Worksheet 06, Build D (Q11-Q12) - floor-control state machine. Synthetic unit numbers only.
#include <gtest/gtest.h>

#include "lmrb/floor_control.hpp"

using lmrb::FloorAction;
using lmrb::FloorControl;
using lmrb::FloorState;

TEST(FloorControl, IdleToActiveOnPress) {
  FloorControl f(3000, 60000);
  EXPECT_EQ(f.state(), FloorState::Idle);
  const auto o = f.press(7);
  EXPECT_EQ(o.action, FloorAction::Granted);
  EXPECT_EQ(o.talker, 7u);
  EXPECT_EQ(f.state(), FloorState::Active);
  EXPECT_EQ(f.talker(), 7u);
  EXPECT_FALSE(f.emergency());
}

TEST(FloorControl, IdleIgnoresReleaseAndTick) {
  FloorControl f(3000, 60000);
  EXPECT_EQ(f.release(1).action, FloorAction::None);
  EXPECT_EQ(f.tick(5000).action, FloorAction::None);
  EXPECT_EQ(f.state(), FloorState::Idle);
}

TEST(FloorControl, SecondTalkerIsDeniedAndDuplicateKeyUpIsIgnored) {
  FloorControl f(3000, 60000);
  f.press(1);
  EXPECT_EQ(f.press(1).action, FloorAction::None);
  const auto d = f.press(2);
  EXPECT_EQ(d.action, FloorAction::Denied);
  EXPECT_EQ(d.talker, 1u);
  EXPECT_EQ(f.talker(), 1u);
}

TEST(FloorControl, ReleaseByNonTalkerIsIgnored) {
  FloorControl f(3000, 60000);
  f.press(1);
  EXPECT_EQ(f.release(2).action, FloorAction::None);
  EXPECT_EQ(f.state(), FloorState::Active);
  EXPECT_EQ(f.talker(), 1u);
}

TEST(FloorControl, ReleaseEntersHangThenExpiresToIdle) {
  FloorControl f(3000, 60000);
  f.press(1);
  const auto r = f.release(1);
  EXPECT_EQ(r.action, FloorAction::Released);
  EXPECT_EQ(r.previous, 1u);
  EXPECT_EQ(r.talker, 0u);
  EXPECT_EQ(f.state(), FloorState::Hang);
  EXPECT_EQ(f.talker(), 0u);

  EXPECT_EQ(f.tick(2999).action, FloorAction::None);
  EXPECT_EQ(f.state(), FloorState::Hang);
  EXPECT_EQ(f.tick(1).action, FloorAction::HangExpired);
  EXPECT_EQ(f.state(), FloorState::Idle);
}

TEST(FloorControl, ReplyDuringHangNeedsNoNewGrantAndResetsTimers) {
  FloorControl f(3000, 60000);
  f.press(1);
  f.release(1);
  f.tick(2000);
  const auto o = f.press(2);  // a reply from another member of the group
  EXPECT_EQ(o.action, FloorAction::Granted);
  EXPECT_EQ(f.state(), FloorState::Active);
  EXPECT_EQ(f.talker(), 2u);
  f.release(2);
  // hang timer restarted from zero: 2999 ms later we are still in hang
  EXPECT_EQ(f.tick(2999).action, FloorAction::None);
  EXPECT_EQ(f.state(), FloorState::Hang);
  EXPECT_EQ(f.tick(1).action, FloorAction::HangExpired);
}

TEST(FloorControl, HangIgnoresRelease) {
  FloorControl f(3000, 60000);
  f.press(1);
  f.release(1);
  EXPECT_EQ(f.release(1).action, FloorAction::None);
  EXPECT_EQ(f.state(), FloorState::Hang);
}

TEST(FloorControl, EmergencyPreemptsANormalTalker) {
  FloorControl f(3000, 60000);
  f.press(1);
  const auto o = f.press(2, true);
  EXPECT_EQ(o.action, FloorAction::Preempted);
  EXPECT_EQ(o.previous, 1u);
  EXPECT_EQ(o.talker, 2u);
  EXPECT_EQ(f.talker(), 2u);
  EXPECT_TRUE(f.emergency());
  // another emergency cannot take it, nor can a normal press
  EXPECT_EQ(f.press(3, true).action, FloorAction::Denied);
  EXPECT_EQ(f.press(4, false).action, FloorAction::Denied);
  EXPECT_EQ(f.talker(), 2u);
}

TEST(FloorControl, NormalPressNeverPreempts) {
  FloorControl f(3000, 60000);
  f.press(1);
  EXPECT_EQ(f.press(2, false).action, FloorAction::Denied);
  EXPECT_FALSE(f.emergency());
}

TEST(FloorControl, EmergencyFlagDoesNotSurviveTheCall) {
  FloorControl f(3000, 60000);
  f.press(1);
  f.press(2, true);
  f.release(2);
  EXPECT_FALSE(f.emergency());
  f.press(3, false);
  EXPECT_FALSE(f.emergency());
}

TEST(FloorControl, TalkTimeoutForcesHang) {
  FloorControl f(3000, 1000);
  f.press(5);
  for (int i = 1; i <= 9; ++i) {
    EXPECT_EQ(f.tick(100).action, FloorAction::None) << i;  // 100..900 ms
  }
  const auto o = f.tick(100);  // 1000 ms reached
  EXPECT_EQ(o.action, FloorAction::TalkTimeout);
  EXPECT_EQ(o.previous, 5u);
  EXPECT_EQ(o.talker, 0u);
  EXPECT_EQ(f.state(), FloorState::Hang);
}

TEST(FloorControl, GrantAndPreemptionResetTheTalkTimer) {
  FloorControl f(3000, 1000);
  f.press(1);
  f.tick(900);
  f.press(2, true);  // preempt: new talker gets a full timeout
  EXPECT_EQ(f.tick(900).action, FloorAction::None);
  EXPECT_EQ(f.tick(100).action, FloorAction::TalkTimeout);
}

TEST(FloorControl, FullConversationScript) {
  FloorControl f(3000, 60000);
  EXPECT_EQ(f.press(1).action, FloorAction::Granted);
  f.tick(5000);
  EXPECT_EQ(f.release(1).action, FloorAction::Released);
  f.tick(1500);
  EXPECT_EQ(f.press(2).action, FloorAction::Granted);  // reply inside hang
  f.tick(2500);
  EXPECT_EQ(f.release(2).action, FloorAction::Released);
  f.tick(3000);
  EXPECT_EQ(f.state(), FloorState::Idle);
  EXPECT_EQ(f.press(3).action, FloorAction::Granted);  // a fresh call after hang expired
}
