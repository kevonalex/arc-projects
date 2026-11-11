#pragma once
// STUB - worksheet Q18, Q19. The call state machine as ENUMS + SWITCH.
//
// A push-to-talk group call, seen from one radio/console:
//
//   Idle        nothing happening; the channel is free
//   Requesting  we pressed PTT and asked for the floor; waiting for the answer
//   Talking     we hold the floor and are transmitting
//   Listening   somebody else holds the floor and we are receiving
//   Hang        the talker released; the channel stays reserved for a short "hang time" so a reply
//               needs no new call set-up (P25/DMR "hang time" - durations VERIFY per system config)
//
// The transition table (everything not listed is IGNORED: state unchanged, Action::None, handled=false):
//
//   state       event             -> next state   action
//   ---------   ---------------   -------------   -----------------
//   Idle        PttPress          -> Requesting   SendFloorRequest
//   Idle        RemoteTalkStart   -> Listening    StartRx
//   Requesting  FloorGranted      -> Talking      StartTx
//   Requesting  FloorDenied       -> Idle         PlayBusyTone
//   Requesting  PttRelease        -> Idle         CancelRequest
//   Talking     PttRelease        -> Hang         StopTxStartHang
//   Listening   RemoteTalkEnd     -> Hang         StopRxStartHang
//   Hang        PttPress          -> Requesting   SendFloorRequest
//   Hang        RemoteTalkStart   -> Listening    StartRx
//   Hang        HangTimeout       -> Idle         ReleaseChannel
//
// 10 transitions out of 5 x 7 = 35 (state, event) pairs; the other 25 are ignored.
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace es::sm {

enum class CallState : std::uint8_t { Idle, Requesting, Talking, Listening, Hang };

enum class Event : std::uint8_t {
    PttPress,
    PttRelease,
    FloorGranted,
    FloorDenied,
    RemoteTalkStart,
    RemoteTalkEnd,
    HangTimeout
};

enum class Action : std::uint8_t {
    None,
    SendFloorRequest,
    CancelRequest,
    PlayBusyTone,
    StartTx,
    StopTxStartHang,
    StartRx,
    StopRxStartHang,
    ReleaseChannel
};

inline constexpr std::size_t kNumStates = 5;
inline constexpr std::size_t kNumEvents = 7;

struct Transition {
    CallState next;
    Action action;
    bool handled;  // false: the event is not meaningful in this state (ignored)
};

// Names for logs: exactly the enumerator spelling, e.g. "Requesting", "PttPress", "StopTxStartHang".
// Write each as a switch WITHOUT a default: then -Wswitch (part of -Wall) warns if you add an enumerator
// and forget it here. Return "?" after the switch for out-of-range values.
constexpr std::string_view to_string(CallState s) noexcept {
    (void)s;
    return "?";  // TODO
}
constexpr std::string_view to_string(Event e) noexcept {
    (void)e;
    return "?";  // TODO
}
constexpr std::string_view to_string(Action a) noexcept {
    (void)a;
    return "?";  // TODO
}

// The pure transition function: no side effects, no stored state - just the table above.
// Switch on the state (no default), and inside each case switch on the event.
constexpr Transition step(CallState s, Event e) noexcept {
    (void)e;
    return {s, Action::None, false};  // TODO
}

}  // namespace es::sm
