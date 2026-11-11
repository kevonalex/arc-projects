#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include <es_lifetime/call_registry.hpp>

using namespace es::lifetime;

namespace {
struct Counter : Listener {
    int hits = 0;
    int last = -1;
    void on_event(int code) override {
        ++hits;
        last = code;
    }
};

// Adds another listener to the registry the first time it is notified.
struct Adder : Listener {
    CallRegistry& reg;
    std::shared_ptr<Counter> to_add;
    bool done = false;
    int hits = 0;
    Adder(CallRegistry& r, std::shared_ptr<Counter> c) : reg(r), to_add(std::move(c)) {}
    void on_event(int) override {
        ++hits;
        if (!done) {
            done = true;
            reg.add(to_add);  // may reallocate the registry's storage while notify() is running
        }
    }
};
}  // namespace

TEST(CallRegistry, NotifiesAllLiveListeners) {
    CallRegistry reg;
    auto a = std::make_shared<Counter>();
    auto b = std::make_shared<Counter>();
    reg.add(a);
    reg.add(b);
    EXPECT_EQ(reg.notify(7), 2u);
    EXPECT_EQ(a->hits, 1);
    EXPECT_EQ(b->last, 7);
}

TEST(CallRegistry, DoesNotExtendListenerLifetime) {
    CallRegistry reg;
    auto a = std::make_shared<Counter>();
    reg.add(a);
    EXPECT_EQ(a.use_count(), 1) << "registry must hold a weak reference, not a shared one";
    std::weak_ptr<Counter> probe = a;
    a.reset();
    EXPECT_TRUE(probe.expired());
}

TEST(CallRegistry, SkipsAndPrunesExpiredListeners) {
    CallRegistry reg;
    auto a = std::make_shared<Counter>();
    auto b = std::make_shared<Counter>();
    reg.add(a);
    reg.add(b);
    ASSERT_EQ(reg.size(), 2u);
    b.reset();  // owner destroys the listener; the registry must not call it
    EXPECT_EQ(reg.notify(1), 1u);
    EXPECT_EQ(a->hits, 1);
    EXPECT_EQ(reg.size(), 1u) << "expired entry should have been pruned";
}

TEST(CallRegistry, EmptyRegistryNotifiesNobody) {
    CallRegistry reg;
    EXPECT_EQ(reg.notify(1), 0u);
}

TEST(CallRegistry, ListenerMayAddListenerDuringNotify) {
    CallRegistry reg;
    auto late = std::make_shared<Counter>();
    auto adder = std::make_shared<Adder>(reg, late);
    reg.add(adder);
    EXPECT_EQ(reg.notify(1), 1u);  // `late` was added during the round: not called in this round
    EXPECT_EQ(late->hits, 0);
    EXPECT_EQ(reg.notify(2), 2u);  // next round: both
    EXPECT_EQ(late->hits, 1);
    EXPECT_EQ(adder->hits, 2);
}
