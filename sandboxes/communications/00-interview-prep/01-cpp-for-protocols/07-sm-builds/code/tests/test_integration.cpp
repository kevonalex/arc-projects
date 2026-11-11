// Integration (worksheet Q27): the whole chain of pack 07 plus pack 06's ideas.
//   RX thread --Msg--> SpscQueue<Msg,16> --Msg--> protocol thread --> CallMachine --> FixedVector<Action,..> log
// No locks and no heap on the data path between the two threads.
#include <es_sm/call_machine.hpp>
#include <es_sm/fixed_vector.hpp>
#include <es_sm/ring_buffer.hpp>
#include <es_sm/spsc_queue.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <thread>
#include <vector>

using namespace es::sm;
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

TEST(Integration, ScriptedCallThroughSpscIntoMachine) {
    SpscQueue<Msg, 16> q;
    CallMachine machine;
    FixedVector<Action, 16> actions;
    const auto deadline = Clock::now() + 10s;
    bool timed_out = false;

    const std::vector<Msg> script = {
        PttPress{},          // Idle -> Requesting      SendFloorRequest
        FloorGranted{7},     // Requesting -> Talking   StartTx
        RemoteTalkStart{9},  // ignored: we hold the floor
        PttRelease{},        // Talking -> Hang         StopTxStartHang
        RemoteTalkStart{9},  // Hang -> Listening       StartRx
        RemoteTalkEnd{},     // Listening -> Hang       StopRxStartHang
        HangTimeout{},       // Hang -> Idle            ReleaseChannel
    };

    std::thread rx([&] {  // producer: the "radio" side
        for (const Msg& m : script) {
            while (!q.try_push(m)) {
                if (Clock::now() > deadline) return;
                std::this_thread::yield();
            }
        }
    });
    std::thread proto([&] {  // consumer: owns the state machine; nobody else touches it
        std::size_t processed = 0;
        Msg m;
        while (processed < script.size()) {
            if (q.try_pop(m)) {
                const Transition t = machine.handle(m);
                if (t.handled) actions.push_back(t.action);
                ++processed;
            } else {
                if (Clock::now() > deadline) {
                    timed_out = true;
                    return;
                }
                std::this_thread::yield();
            }
        }
    });
    rx.join();
    proto.join();

    ASSERT_FALSE(timed_out);
    EXPECT_EQ(machine.state(), CallState::Idle);
    EXPECT_EQ(machine.ignored(), 1u);
    EXPECT_FALSE(machine.talker().has_value());

    const Action expected[] = {Action::SendFloorRequest, Action::StartTx,       Action::StopTxStartHang,
                               Action::StartRx,          Action::StopRxStartHang, Action::ReleaseChannel};
    ASSERT_EQ(actions.size(), std::size(expected));
    for (std::size_t i = 0; i < actions.size(); ++i) {
        EXPECT_EQ(actions[i], expected[i]) << "action " << i << ": " << to_string(actions[i]) << " vs "
                                           << to_string(expected[i]);
    }
}

TEST(Integration, TransitionLogRingKeepsOnlyTheLastEntries) {
    // A diagnostic trail: remember the last 8 handled transitions, overwriting the oldest.
    RingBuffer<Action, 8> trail;
    CallMachine m;
    for (int call = 0; call < 5; ++call) {  // 5 full calls x 4 transitions = 20 entries
        for (Event e : {Event::PttPress, Event::FloorGranted, Event::PttRelease, Event::HangTimeout}) {
            const Transition t = m.handle(e);
            if (t.handled) trail.push_overwrite(t.action);
        }
    }
    ASSERT_EQ(trail.size(), 8u);
    // the last two calls: [SendFloorRequest, StartTx, StopTxStartHang, ReleaseChannel] x 2
    EXPECT_EQ(trail[0], Action::SendFloorRequest);
    EXPECT_EQ(trail[3], Action::ReleaseChannel);
    EXPECT_EQ(trail[4], Action::SendFloorRequest);
    EXPECT_EQ(trail[7], Action::ReleaseChannel);
}
