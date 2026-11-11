#pragma once
// STUB - worksheet 06, Build D (Q11-Q12): the floor-control state machine of ONE talkgroup call.
// Pure logic and synthetic unit numbers - no radio.
//
// States:   Idle   - no channel held
//           Active - one talker holds the floor
//           Hang   - talker released; channel is held for `hang_ms` waiting for a reply
// Events:   press(unit, emergency)   release(unit)   tick(ms)
//
// RULES (the tests encode exactly these):
//  Idle  : press(u)              -> Active, talker = u, action Granted.   release/tick -> None.
//  Active: press(talker)         -> None (duplicate key-up).
//          press(v != talker)    -> if v is emergency AND the current call is NOT emergency: talker = v, action
//                                   Preempted (previous = old talker), call becomes emergency.
//                                   otherwise action Denied (talker unchanged).
//          release(talker)       -> Hang, hang timer = 0, talker = 0, action Released (previous = old talker).
//          release(other)        -> None.
//          tick(ms)              -> talk timer += ms; if talk timer >= talk_timeout_ms -> Hang, hang timer = 0,
//                                   talker = 0, action TalkTimeout (previous = old talker).
//  Hang  : press(v)              -> Active, talker = v, talk timer = 0, emergency = flag, action Granted
//                                   (a reply within hang time needs no new channel grant).
//          release(...)          -> None.
//          tick(ms)              -> hang timer += ms; if >= hang_ms -> Idle, action HangExpired.
//  Every grant/preempt resets the talk timer to 0. tick() performs AT MOST ONE transition per call
//  (the caller ticks in small steps, e.g. 10 ms).
#include <cstdint>

#include "lmr/todo.hpp"

namespace lmrb {

enum class FloorState { Idle, Active, Hang };

enum class FloorAction { None, Granted, Denied, Preempted, Released, TalkTimeout, HangExpired };

struct FloorOutcome {
  FloorAction action = FloorAction::None;
  std::uint32_t talker = 0;    // talker AFTER the event (0 = nobody)
  std::uint32_t previous = 0;  // talker BEFORE the event when it changed (Preempted/Released/TalkTimeout)
};

class FloorControl {
 public:
  FloorControl(std::uint32_t hang_ms, std::uint32_t talk_timeout_ms)
      : hang_ms_(hang_ms), talk_timeout_ms_(talk_timeout_ms) {}

  // TODO
  FloorOutcome press(std::uint32_t /*unit*/, bool /*emergency*/ = false) { lmr::todo("press"); }
  FloorOutcome release(std::uint32_t /*unit*/) { lmr::todo("release"); }
  FloorOutcome tick(std::uint32_t /*ms*/) { lmr::todo("tick"); }

  FloorState state() const { return state_; }
  std::uint32_t talker() const { return talker_; }
  bool emergency() const { return emergency_; }

 private:
  std::uint32_t hang_ms_;
  std::uint32_t talk_timeout_ms_;
  FloorState state_ = FloorState::Idle;
  std::uint32_t talker_ = 0;
  bool emergency_ = false;
  std::uint32_t talk_ms_ = 0;
  std::uint32_t hang_elapsed_ms_ = 0;
};

}  // namespace lmrb
