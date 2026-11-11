#pragma once
// STUB - worksheet Q20, Q21. Messages (std::variant) -> events (enum) with std::visit, and the stateful
// CallMachine that wraps the pure step() function from call_state.hpp.
#include <es_sm/call_state.hpp>
#include <es_sm/msg.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace es::sm {

// Map a message to the plain Event used by step(). Use std::visit with `overloaded` - one lambda per
// alternative, NO default/catch-all lambda, so adding a message type without updating this is a compile error.
inline Event to_event(const Msg& m) {
    (void)m;
    return Event::PttPress;  // TODO
}

// Human-readable text for logs, produced with std::visit:
//   PttPress -> "PttPress"            PttRelease -> "PttRelease"
//   FloorGranted{7} -> "FloorGranted(talker=7)"        FloorDenied{2} -> "FloorDenied(reason=2)"
//   RemoteTalkStart{9} -> "RemoteTalkStart(talker=9)"  RemoteTalkEnd -> "RemoteTalkEnd"   HangTimeout -> "HangTimeout"
// Careful: print the uint8_t reason as a NUMBER (cast to unsigned), not as a character (pack 02).
inline std::string describe(const Msg& m) {
    (void)m;
    return "";  // TODO
}

class CallMachine {
public:
    CallState state() const noexcept { return state_; }

    // Remote (or granted) talker id carried by the last handled FloorGranted / RemoteTalkStart message;
    // cleared whenever the machine returns to Idle.
    std::optional<std::uint32_t> talker() const noexcept { return talker_; }

    // How many events/messages were ignored (handled == false) since construction.
    std::size_t ignored() const noexcept { return ignored_; }

    // Apply an event: state_ = step(state_, e).next; count ignored events; clear talker_ when the new state is Idle.
    Transition handle(Event e) {
        (void)e;
        return {state_, Action::None, false};  // TODO
    }

    // Apply a message: convert with to_event(), call handle(Event); if it was handled, record the talker id
    // carried by FloorGranted / RemoteTalkStart (use std::get_if or std::visit).
    Transition handle(const Msg& m) {
        (void)m;
        return {state_, Action::None, false};  // TODO
    }

private:
    CallState state_ = CallState::Idle;
    std::optional<std::uint32_t> talker_;
    std::size_t ignored_ = 0;
};

}  // namespace es::sm
