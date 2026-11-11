// Tests for variant + std::visit (worksheet Q20, Q21): to_event, describe, CallMachine::handle(Msg).
#include <es_sm/call_machine.hpp>
#include <es_sm/msg.hpp>

#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <variant>

using namespace es::sm;

TEST(Msg, VariantFacts) {
    // PROVIDED facts - these pass from the start and teach what the variant is.
    static_assert(std::variant_size_v<Msg> == 7, "seven message types");
    static_assert(std::is_trivially_copyable_v<Msg>, "plain data: safe to push through a lock-free queue");
    Msg m;  // default-constructed: holds the FIRST alternative
    EXPECT_TRUE(std::holds_alternative<PttPress>(m));
    EXPECT_EQ(m.index(), 0u);
    m = FloorGranted{42};
    EXPECT_EQ(m.index(), 2u);
    EXPECT_EQ(std::get<FloorGranted>(m).talker_id, 42u);
    EXPECT_EQ(std::get_if<FloorDenied>(&m), nullptr) << "wrong-type get_if returns nullptr, no exception";
}

TEST(Msg, ToEventCoversEveryAlternative) {
    EXPECT_EQ(to_event(Msg{PttPress{}}), Event::PttPress);
    EXPECT_EQ(to_event(Msg{PttRelease{}}), Event::PttRelease);
    EXPECT_EQ(to_event(Msg{FloorGranted{1}}), Event::FloorGranted);
    EXPECT_EQ(to_event(Msg{FloorDenied{2}}), Event::FloorDenied);
    EXPECT_EQ(to_event(Msg{RemoteTalkStart{3}}), Event::RemoteTalkStart);
    EXPECT_EQ(to_event(Msg{RemoteTalkEnd{}}), Event::RemoteTalkEnd);
    EXPECT_EQ(to_event(Msg{HangTimeout{}}), Event::HangTimeout);
}

TEST(Msg, Describe) {
    EXPECT_EQ(describe(Msg{PttPress{}}), "PttPress");
    EXPECT_EQ(describe(Msg{PttRelease{}}), "PttRelease");
    EXPECT_EQ(describe(Msg{FloorGranted{7}}), "FloorGranted(talker=7)");
    EXPECT_EQ(describe(Msg{FloorDenied{2}}), "FloorDenied(reason=2)");
    EXPECT_EQ(describe(Msg{RemoteTalkStart{9}}), "RemoteTalkStart(talker=9)");
    EXPECT_EQ(describe(Msg{RemoteTalkEnd{}}), "RemoteTalkEnd");
    EXPECT_EQ(describe(Msg{HangTimeout{}}), "HangTimeout");
}

TEST(Msg, DescribeReasonIsNumberNotCharacter) {
    // 65 is 'A'; a careless `std::string(1, reason)` or `os << uint8_t` would print a letter.
    EXPECT_EQ(describe(Msg{FloorDenied{65}}), "FloorDenied(reason=65)");
    EXPECT_EQ(describe(Msg{FloorDenied{255}}), "FloorDenied(reason=255)");
}

TEST(Msg, OverloadedPicksByType) {
    // PROVIDED helper check; passes from the start.
    const Msg m = RemoteTalkStart{5};
    const std::string s = std::visit(overloaded{[](const RemoteTalkStart& r) { return std::string("rx") + std::to_string(r.talker_id); },
                                                [](const auto&) { return std::string("other"); }},
                                     m);
    EXPECT_EQ(s, "rx5");
}

TEST(MachineMsg, TalkerRecordedFromPayloadAndClearedOnIdle) {
    CallMachine m;
    EXPECT_FALSE(m.talker().has_value());

    EXPECT_TRUE(m.handle(Msg{RemoteTalkStart{9}}).handled);
    EXPECT_EQ(m.state(), CallState::Listening);
    ASSERT_TRUE(m.talker().has_value());
    EXPECT_EQ(*m.talker(), 9u);

    m.handle(Msg{RemoteTalkEnd{}});
    EXPECT_EQ(m.state(), CallState::Hang);
    ASSERT_TRUE(m.talker().has_value()) << "talker is remembered during hang time";
    EXPECT_EQ(*m.talker(), 9u);

    m.handle(Msg{HangTimeout{}});
    EXPECT_EQ(m.state(), CallState::Idle);
    EXPECT_FALSE(m.talker().has_value()) << "cleared when back to Idle";
}

TEST(MachineMsg, IgnoredMessageDoesNotChangeTalker) {
    CallMachine m;
    m.handle(Msg{RemoteTalkStart{9}});
    // FloorGranted while Listening is ignored: its talker id must NOT overwrite the stored one.
    const Transition t = m.handle(Msg{FloorGranted{1234}});
    EXPECT_FALSE(t.handled);
    ASSERT_TRUE(m.talker().has_value());
    EXPECT_EQ(*m.talker(), 9u);
    EXPECT_EQ(m.ignored(), 1u);
}

TEST(MachineMsg, GrantedTalkerRecorded) {
    CallMachine m;
    m.handle(Msg{PttPress{}});
    EXPECT_EQ(m.state(), CallState::Requesting);
    EXPECT_TRUE(m.handle(Msg{FloorGranted{77}}).handled);
    EXPECT_EQ(m.state(), CallState::Talking);
    ASSERT_TRUE(m.talker().has_value());
    EXPECT_EQ(*m.talker(), 77u);
}

TEST(MachineMsg, DeniedGoesIdle) {
    CallMachine m;
    m.handle(Msg{PttPress{}});
    const Transition t = m.handle(Msg{FloorDenied{3}});
    EXPECT_EQ(t.action, Action::PlayBusyTone);
    EXPECT_EQ(m.state(), CallState::Idle);
}

TEST(MachineMsg, HandleAcceptsAlternativesDirectly) {
    CallMachine m;
    // Implicit conversion from an alternative to Msg picks the Msg overload (Event is a scoped enum).
    EXPECT_TRUE(m.handle(PttPress{}).handled);
    EXPECT_EQ(m.state(), CallState::Requesting);
}
