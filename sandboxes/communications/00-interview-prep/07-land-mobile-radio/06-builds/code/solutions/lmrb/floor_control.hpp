#pragma once
// SOLUTION - worksheet 06, Build D.
#include <cstdint>

#include "lmr/todo.hpp"

namespace lmrb {

enum class FloorState { Idle, Active, Hang };

enum class FloorAction { None, Granted, Denied, Preempted, Released, TalkTimeout, HangExpired };

struct FloorOutcome {
  FloorAction action = FloorAction::None;
  std::uint32_t talker = 0;
  std::uint32_t previous = 0;
};

class FloorControl {
 public:
  FloorControl(std::uint32_t hang_ms, std::uint32_t talk_timeout_ms)
      : hang_ms_(hang_ms), talk_timeout_ms_(talk_timeout_ms) {}

  FloorOutcome press(std::uint32_t unit, bool emergency = false) {
    FloorOutcome o;
    switch (state_) {
      case FloorState::Idle:
      case FloorState::Hang:
        state_ = FloorState::Active;
        talker_ = unit;
        emergency_ = emergency;
        talk_ms_ = 0;
        o.action = FloorAction::Granted;
        o.talker = talker_;
        return o;
      case FloorState::Active:
        if (unit == talker_) {
          o.talker = talker_;
          return o;  // duplicate key-up: None
        }
        if (emergency && !emergency_) {
          o.previous = talker_;
          talker_ = unit;
          emergency_ = true;
          talk_ms_ = 0;
          o.action = FloorAction::Preempted;
          o.talker = talker_;
          return o;
        }
        o.action = FloorAction::Denied;
        o.talker = talker_;
        return o;
    }
    return o;
  }

  FloorOutcome release(std::uint32_t unit) {
    FloorOutcome o;
    o.talker = talker_;
    if (state_ == FloorState::Active && unit == talker_) {
      o.previous = talker_;
      enter_hang();
      o.action = FloorAction::Released;
      o.talker = 0;
    }
    return o;
  }

  FloorOutcome tick(std::uint32_t ms) {
    FloorOutcome o;
    o.talker = talker_;
    if (state_ == FloorState::Active) {
      talk_ms_ += ms;
      if (talk_ms_ >= talk_timeout_ms_) {
        o.previous = talker_;
        enter_hang();
        o.action = FloorAction::TalkTimeout;
        o.talker = 0;
      }
    } else if (state_ == FloorState::Hang) {
      hang_elapsed_ms_ += ms;
      if (hang_elapsed_ms_ >= hang_ms_) {
        state_ = FloorState::Idle;
        emergency_ = false;
        hang_elapsed_ms_ = 0;
        o.action = FloorAction::HangExpired;
      }
    }
    return o;
  }

  FloorState state() const { return state_; }
  std::uint32_t talker() const { return talker_; }
  bool emergency() const { return emergency_; }

 private:
  void enter_hang() {
    state_ = FloorState::Hang;
    talker_ = 0;
    emergency_ = false;  // nobody is talking: the emergency flag belongs to the talker
    hang_elapsed_ms_ = 0;
    talk_ms_ = 0;
  }

  std::uint32_t hang_ms_;
  std::uint32_t talk_timeout_ms_;
  FloorState state_ = FloorState::Idle;
  std::uint32_t talker_ = 0;
  bool emergency_ = false;
  std::uint32_t talk_ms_ = 0;
  std::uint32_t hang_elapsed_ms_ = 0;
};

}  // namespace lmrb
