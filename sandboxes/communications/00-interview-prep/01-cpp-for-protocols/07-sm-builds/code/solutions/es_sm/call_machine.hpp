#pragma once
// SOLUTION - attempt Q20, Q21 first.
#include <es_sm/call_state.hpp>
#include <es_sm/msg.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace es::sm {

inline Event to_event(const Msg& m) {
    // One lambda per alternative and NO catch-all: add a struct to Msg and this stops compiling
    // ("no matching function for call") until you decide what it means.
    return std::visit(
        overloaded{
            [](const PttPress&) { return Event::PttPress; },
            [](const PttRelease&) { return Event::PttRelease; },
            [](const FloorGranted&) { return Event::FloorGranted; },
            [](const FloorDenied&) { return Event::FloorDenied; },
            [](const RemoteTalkStart&) { return Event::RemoteTalkStart; },
            [](const RemoteTalkEnd&) { return Event::RemoteTalkEnd; },
            [](const HangTimeout&) { return Event::HangTimeout; },
        },
        m);
}

inline std::string describe(const Msg& m) {
    return std::visit(
        overloaded{
            [](const PttPress&) -> std::string { return "PttPress"; },
            [](const PttRelease&) -> std::string { return "PttRelease"; },
            [](const FloorGranted& x) -> std::string {
                return "FloorGranted(talker=" + std::to_string(x.talker_id) + ")";
            },
            [](const FloorDenied& x) -> std::string {
                // uint8_t would print as a character through iostreams; std::to_string(unsigned) is a number.
                return "FloorDenied(reason=" + std::to_string(static_cast<unsigned>(x.reason)) + ")";
            },
            [](const RemoteTalkStart& x) -> std::string {
                return "RemoteTalkStart(talker=" + std::to_string(x.talker_id) + ")";
            },
            [](const RemoteTalkEnd&) -> std::string { return "RemoteTalkEnd"; },
            [](const HangTimeout&) -> std::string { return "HangTimeout"; },
        },
        m);
}

class CallMachine {
public:
    CallState state() const noexcept { return state_; }
    std::optional<std::uint32_t> talker() const noexcept { return talker_; }
    std::size_t ignored() const noexcept { return ignored_; }

    Transition handle(Event e) {
        const Transition t = step(state_, e);
        if (!t.handled) {
            ++ignored_;
            return t;  // state unchanged
        }
        state_ = t.next;
        if (state_ == CallState::Idle) talker_.reset();
        return t;
    }

    Transition handle(const Msg& m) {
        const Transition t = handle(to_event(m));
        if (t.handled) {
            if (const auto* g = std::get_if<FloorGranted>(&m)) {
                talker_ = g->talker_id;
            } else if (const auto* r = std::get_if<RemoteTalkStart>(&m)) {
                talker_ = r->talker_id;
            }
        }
        return t;
    }

private:
    CallState state_ = CallState::Idle;
    std::optional<std::uint32_t> talker_;
    std::size_t ignored_ = 0;
};

}  // namespace es::sm
