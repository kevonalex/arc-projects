#pragma once
// SOLUTION - attempt Q18, Q19 first. (Table in include/es_sm/call_state.hpp.)
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
    bool handled;
};

constexpr std::string_view to_string(CallState s) noexcept {
    switch (s) {  // no default: -Wswitch flags a forgotten enumerator
        case CallState::Idle: return "Idle";
        case CallState::Requesting: return "Requesting";
        case CallState::Talking: return "Talking";
        case CallState::Listening: return "Listening";
        case CallState::Hang: return "Hang";
    }
    return "?";  // out-of-range value (e.g. a corrupted byte cast to the enum)
}

constexpr std::string_view to_string(Event e) noexcept {
    switch (e) {
        case Event::PttPress: return "PttPress";
        case Event::PttRelease: return "PttRelease";
        case Event::FloorGranted: return "FloorGranted";
        case Event::FloorDenied: return "FloorDenied";
        case Event::RemoteTalkStart: return "RemoteTalkStart";
        case Event::RemoteTalkEnd: return "RemoteTalkEnd";
        case Event::HangTimeout: return "HangTimeout";
    }
    return "?";
}

constexpr std::string_view to_string(Action a) noexcept {
    switch (a) {
        case Action::None: return "None";
        case Action::SendFloorRequest: return "SendFloorRequest";
        case Action::CancelRequest: return "CancelRequest";
        case Action::PlayBusyTone: return "PlayBusyTone";
        case Action::StartTx: return "StartTx";
        case Action::StopTxStartHang: return "StopTxStartHang";
        case Action::StartRx: return "StartRx";
        case Action::StopRxStartHang: return "StopRxStartHang";
        case Action::ReleaseChannel: return "ReleaseChannel";
    }
    return "?";
}

constexpr Transition step(CallState s, Event e) noexcept {
    // Outer switch: NO default - every state must be considered (compiler checks).
    // Inner switches: `default` means "all other events are ignored in this state", which is the
    // protocol rule; the test AllTransitionsExhaustive guards against a new Event slipping through.
    switch (s) {
        case CallState::Idle:
            switch (e) {
                case Event::PttPress: return {CallState::Requesting, Action::SendFloorRequest, true};
                case Event::RemoteTalkStart: return {CallState::Listening, Action::StartRx, true};
                default: break;
            }
            break;
        case CallState::Requesting:
            switch (e) {
                case Event::FloorGranted: return {CallState::Talking, Action::StartTx, true};
                case Event::FloorDenied: return {CallState::Idle, Action::PlayBusyTone, true};
                case Event::PttRelease: return {CallState::Idle, Action::CancelRequest, true};
                default: break;
            }
            break;
        case CallState::Talking:
            switch (e) {
                case Event::PttRelease: return {CallState::Hang, Action::StopTxStartHang, true};
                default: break;
            }
            break;
        case CallState::Listening:
            switch (e) {
                case Event::RemoteTalkEnd: return {CallState::Hang, Action::StopRxStartHang, true};
                default: break;
            }
            break;
        case CallState::Hang:
            switch (e) {
                case Event::PttPress: return {CallState::Requesting, Action::SendFloorRequest, true};
                case Event::RemoteTalkStart: return {CallState::Listening, Action::StartRx, true};
                case Event::HangTimeout: return {CallState::Idle, Action::ReleaseChannel, true};
                default: break;
            }
            break;
    }
    return {s, Action::None, false};
}

}  // namespace es::sm
