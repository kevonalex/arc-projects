// Tests for es_sm/call_state.hpp and the CallMachine event path (worksheet Q18, Q19, Q21).
#include <es_sm/call_machine.hpp>
#include <es_sm/call_state.hpp>

#include <gtest/gtest.h>

#include <array>
#include <queue>
#include <set>
#include <string>
#include <vector>

using namespace es::sm;

namespace {
constexpr std::array<CallState, kNumStates> kStates = {CallState::Idle, CallState::Requesting, CallState::Talking,
                                                       CallState::Listening, CallState::Hang};
constexpr std::array<Event, kNumEvents> kEvents = {Event::PttPress,       Event::PttRelease,   Event::FloorGranted,
                                                   Event::FloorDenied,    Event::RemoteTalkStart,
                                                   Event::RemoteTalkEnd,  Event::HangTimeout};
}  // namespace

TEST(Names, StateNames) {
    EXPECT_EQ(to_string(CallState::Idle), "Idle");
    EXPECT_EQ(to_string(CallState::Requesting), "Requesting");
    EXPECT_EQ(to_string(CallState::Talking), "Talking");
    EXPECT_EQ(to_string(CallState::Listening), "Listening");
    EXPECT_EQ(to_string(CallState::Hang), "Hang");
}

TEST(Names, EventNames) {
    EXPECT_EQ(to_string(Event::PttPress), "PttPress");
    EXPECT_EQ(to_string(Event::PttRelease), "PttRelease");
    EXPECT_EQ(to_string(Event::FloorGranted), "FloorGranted");
    EXPECT_EQ(to_string(Event::FloorDenied), "FloorDenied");
    EXPECT_EQ(to_string(Event::RemoteTalkStart), "RemoteTalkStart");
    EXPECT_EQ(to_string(Event::RemoteTalkEnd), "RemoteTalkEnd");
    EXPECT_EQ(to_string(Event::HangTimeout), "HangTimeout");
}

TEST(Names, ActionNames) {
    EXPECT_EQ(to_string(Action::None), "None");
    EXPECT_EQ(to_string(Action::SendFloorRequest), "SendFloorRequest");
    EXPECT_EQ(to_string(Action::CancelRequest), "CancelRequest");
    EXPECT_EQ(to_string(Action::PlayBusyTone), "PlayBusyTone");
    EXPECT_EQ(to_string(Action::StartTx), "StartTx");
    EXPECT_EQ(to_string(Action::StopTxStartHang), "StopTxStartHang");
    EXPECT_EQ(to_string(Action::StartRx), "StartRx");
    EXPECT_EQ(to_string(Action::StopRxStartHang), "StopRxStartHang");
    EXPECT_EQ(to_string(Action::ReleaseChannel), "ReleaseChannel");
}

TEST(Names, OutOfRangeValueIsQuestionMark) {
    // A corrupted byte cast to the enum: the switch falls through to the return after it.
    EXPECT_EQ(to_string(static_cast<CallState>(200)), "?");
    EXPECT_EQ(to_string(static_cast<Event>(200)), "?");
    EXPECT_EQ(to_string(static_cast<Action>(200)), "?");
}

struct Row {
    CallState from;
    Event ev;
    CallState to;
    Action action;
};

TEST(Step, EveryDocumentedTransition) {
    const Row rows[] = {
        {CallState::Idle, Event::PttPress, CallState::Requesting, Action::SendFloorRequest},
        {CallState::Idle, Event::RemoteTalkStart, CallState::Listening, Action::StartRx},
        {CallState::Requesting, Event::FloorGranted, CallState::Talking, Action::StartTx},
        {CallState::Requesting, Event::FloorDenied, CallState::Idle, Action::PlayBusyTone},
        {CallState::Requesting, Event::PttRelease, CallState::Idle, Action::CancelRequest},
        {CallState::Talking, Event::PttRelease, CallState::Hang, Action::StopTxStartHang},
        {CallState::Listening, Event::RemoteTalkEnd, CallState::Hang, Action::StopRxStartHang},
        {CallState::Hang, Event::PttPress, CallState::Requesting, Action::SendFloorRequest},
        {CallState::Hang, Event::RemoteTalkStart, CallState::Listening, Action::StartRx},
        {CallState::Hang, Event::HangTimeout, CallState::Idle, Action::ReleaseChannel},
    };
    for (const Row& r : rows) {
        const Transition t = step(r.from, r.ev);
        EXPECT_TRUE(t.handled) << to_string(r.from) << " + " << to_string(r.ev);
        EXPECT_EQ(t.next, r.to) << to_string(r.from) << " + " << to_string(r.ev);
        EXPECT_EQ(t.action, r.action) << to_string(r.from) << " + " << to_string(r.ev);
    }
}

TEST(Step, AllTransitionsExhaustive) {
    // 5 states x 7 events = 35 pairs: exactly 10 are meaningful, 25 are ignored.
    int handled = 0, ignored = 0;
    for (CallState s : kStates) {
        for (Event e : kEvents) {
            const Transition t = step(s, e);
            if (t.handled) {
                ++handled;
            } else {
                ++ignored;
                EXPECT_EQ(t.next, s) << "ignored event must not change state: " << to_string(s) << " + " << to_string(e);
                EXPECT_EQ(t.action, Action::None) << to_string(s) << " + " << to_string(e);
            }
        }
    }
    EXPECT_EQ(handled, 10);
    EXPECT_EQ(ignored, 25);
}

TEST(Step, ConstexprEvaluable) {
    static_assert(step(CallState::Idle, Event::PttPress).next == CallState::Requesting ||
                      step(CallState::Idle, Event::PttPress).next == CallState::Idle,
                  "step must be usable in constant expressions");
    constexpr auto t = step(CallState::Hang, Event::HangTimeout);
    EXPECT_EQ(t.next, CallState::Idle);
}

TEST(Graph, EveryStateReachableFromIdle) {
    std::set<CallState> seen{CallState::Idle};
    std::queue<CallState> todo;
    todo.push(CallState::Idle);
    while (!todo.empty()) {
        const CallState s = todo.front();
        todo.pop();
        for (Event e : kEvents) {
            const Transition t = step(s, e);
            if (t.handled && seen.insert(t.next).second) todo.push(t.next);
        }
    }
    EXPECT_EQ(seen.size(), kNumStates);
}

TEST(Graph, NoTrapStates_EveryStateCanReachIdle) {
    for (CallState start : kStates) {
        std::set<CallState> seen{start};
        std::queue<CallState> todo;
        todo.push(start);
        bool reached_idle = (start == CallState::Idle);
        while (!todo.empty() && !reached_idle) {
            const CallState s = todo.front();
            todo.pop();
            for (Event e : kEvents) {
                const Transition t = step(s, e);
                if (!t.handled) continue;
                if (t.next == CallState::Idle) reached_idle = true;
                if (seen.insert(t.next).second) todo.push(t.next);
            }
        }
        EXPECT_TRUE(reached_idle) << "stuck in/after " << to_string(start);
    }
}

TEST(Graph, TalkingOnlyReachableViaFloorGranted) {
    for (CallState s : kStates) {
        for (Event e : kEvents) {
            const Transition t = step(s, e);
            if (t.handled && t.next == CallState::Talking) {
                EXPECT_EQ(s, CallState::Requesting);
                EXPECT_EQ(e, Event::FloorGranted);
            }
        }
    }
}

TEST(Machine, HappyPathTalk) {
    CallMachine m;
    EXPECT_EQ(m.state(), CallState::Idle);
    EXPECT_EQ(m.handle(Event::PttPress).action, Action::SendFloorRequest);
    EXPECT_EQ(m.state(), CallState::Requesting);
    EXPECT_EQ(m.handle(Event::FloorGranted).action, Action::StartTx);
    EXPECT_EQ(m.state(), CallState::Talking);
    EXPECT_EQ(m.handle(Event::PttRelease).action, Action::StopTxStartHang);
    EXPECT_EQ(m.state(), CallState::Hang);
    EXPECT_EQ(m.handle(Event::HangTimeout).action, Action::ReleaseChannel);
    EXPECT_EQ(m.state(), CallState::Idle);
    EXPECT_EQ(m.ignored(), 0u);
}

TEST(Machine, CountsIgnoredAndKeepsState) {
    CallMachine m;
    EXPECT_FALSE(m.handle(Event::FloorGranted).handled);  // nobody asked for the floor
    EXPECT_FALSE(m.handle(Event::HangTimeout).handled);
    EXPECT_EQ(m.state(), CallState::Idle);
    EXPECT_EQ(m.ignored(), 2u);
    m.handle(Event::RemoteTalkStart);
    EXPECT_EQ(m.state(), CallState::Listening);
    EXPECT_FALSE(m.handle(Event::PttPress).handled) << "PTT while receiving: floor is busy";
    EXPECT_EQ(m.state(), CallState::Listening);
    EXPECT_EQ(m.ignored(), 3u);
}

TEST(Machine, ReplyDuringHangNeedsNoNewCallSetupButStillRequestsFloor) {
    CallMachine m;
    m.handle(Event::RemoteTalkStart);
    m.handle(Event::RemoteTalkEnd);
    ASSERT_EQ(m.state(), CallState::Hang);
    const Transition t = m.handle(Event::PttPress);
    EXPECT_EQ(t.next, CallState::Requesting);
    EXPECT_EQ(t.action, Action::SendFloorRequest);
}
