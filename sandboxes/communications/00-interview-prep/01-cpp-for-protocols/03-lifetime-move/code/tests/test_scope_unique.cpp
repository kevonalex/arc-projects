#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <es_lifetime/probe.hpp>
#include <es_lifetime/scope_exit.hpp>
#include <es_lifetime/unique_box.hpp>

using namespace es::lifetime;

// ----------------------------------------------------------------------------- ScopeExit
TEST(ScopeExit, RunsAtEndOfScope) {
    int n = 0;
    {
        ScopeExit g([&] { ++n; });
        EXPECT_EQ(n, 0);  // not yet
    }
    EXPECT_EQ(n, 1);
}

TEST(ScopeExit, ReleaseCancelsTheAction) {
    int n = 0;
    {
        ScopeExit g([&] { ++n; });
        g.release();
    }
    EXPECT_EQ(n, 0);
}

TEST(ScopeExit, RunsOnEarlyReturn) {
    int n = 0;
    auto f = [&](bool early) {
        ScopeExit g([&] { ++n; });
        if (early) return 1;
        return 2;
    };
    EXPECT_EQ(f(true), 1);
    EXPECT_EQ(n, 1);
    EXPECT_EQ(f(false), 2);
    EXPECT_EQ(n, 2);
}

TEST(ScopeExit, RunsDuringExceptionUnwinding) {
    int n = 0;
    try {
        ScopeExit g([&] { ++n; });
        throw std::runtime_error("boom");
    } catch (const std::exception&) {
        EXPECT_EQ(n, 1) << "guard must have run BEFORE the catch block";
    }
}

TEST(ScopeExit, MultipleGuardsRunInReverseOrder) {
    std::string order;
    {
        ScopeExit a([&] { order += 'A'; });
        ScopeExit b([&] { order += 'B'; });
        ScopeExit c([&] { order += 'C'; });
    }
    EXPECT_EQ(order, "CBA");  // destroyed in reverse order of construction
}

// ----------------------------------------------------------------------------- UniqueBox
static_assert(!std::is_copy_constructible_v<UniqueBox<int>>);
static_assert(!std::is_copy_assignable_v<UniqueBox<int>>);
static_assert(std::is_nothrow_move_constructible_v<UniqueBox<int>>);
static_assert(sizeof(UniqueBox<int>) == sizeof(int*), "zero overhead over a raw pointer");

TEST(UniqueBox, DeletesExactlyOnce) {
    Probe::reset();
    {
        UniqueBox<Probe> b(new Probe(1));
        EXPECT_EQ(Probe::counts.dtor, 0);
    }
    EXPECT_EQ(Probe::counts.ctor, 1);
    EXPECT_EQ(Probe::counts.dtor, 1);
}

TEST(UniqueBox, MoveConstructTransfersOwnership) {
    Probe::reset();
    {
        UniqueBox<Probe> a(new Probe(5));
        Probe* raw = a.get();
        UniqueBox<Probe> b(std::move(a));
        EXPECT_EQ(b.get(), raw);
        EXPECT_EQ(a.get(), nullptr);  // NOLINT: moved-from state under test
        EXPECT_EQ(b->value, 5);
        EXPECT_EQ((*b).value, 5);
        EXPECT_EQ(Probe::counts.dtor, 0);
    }
    EXPECT_EQ(Probe::counts.dtor, 1) << "double delete or leak";
}

TEST(UniqueBox, MoveAssignDeletesPreviousObject) {
    Probe::reset();
    UniqueBox<Probe> a(new Probe(1));
    UniqueBox<Probe> b(new Probe(2));
    a = std::move(b);
    EXPECT_EQ(Probe::counts.dtor, 1) << "old target of `a` must be deleted immediately";
    EXPECT_EQ(a->value, 2);
    EXPECT_FALSE(static_cast<bool>(b));  // NOLINT
}

TEST(UniqueBox, SelfMoveAssignKeepsObject) {
    Probe::reset();
    UniqueBox<Probe> a(new Probe(4));
    UniqueBox<Probe>& alias = a;
    a = std::move(alias);
    EXPECT_TRUE(static_cast<bool>(a));
    EXPECT_EQ(a->value, 4);
    EXPECT_EQ(Probe::counts.dtor, 0);
}

TEST(UniqueBox, ResetDeletesOldAndOwnsNew) {
    Probe::reset();
    UniqueBox<Probe> a(new Probe(1));
    a.reset(new Probe(2));
    EXPECT_EQ(Probe::counts.dtor, 1);
    EXPECT_EQ(a->value, 2);
    a.reset();
    EXPECT_EQ(Probe::counts.dtor, 2);
    EXPECT_FALSE(static_cast<bool>(a));
}

TEST(UniqueBox, ReleaseGivesUpOwnershipWithoutDeleting) {
    Probe::reset();
    UniqueBox<Probe> a(new Probe(8));
    Probe* raw = a.release();
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(a.get(), nullptr);
    EXPECT_EQ(Probe::counts.dtor, 0);
    delete raw;  // we are the owner now
    EXPECT_EQ(Probe::counts.dtor, 1);
}

TEST(UniqueBox, MakeBoxForwardsArguments) {
    auto b = make_box<Probe>(42);
    EXPECT_EQ(b->value, 42);
}

TEST(UniqueBox, WorksInAVector) {
    Probe::reset();
    {
        std::vector<UniqueBox<Probe>> v;
        for (int i = 0; i < 10; ++i) v.push_back(make_box<Probe>(i));  // move into vector, move on growth
        EXPECT_EQ(v[7]->value, 7);
        EXPECT_EQ(Probe::counts.dtor, 0);
    }
    EXPECT_EQ(Probe::counts.dtor, 10);
}
