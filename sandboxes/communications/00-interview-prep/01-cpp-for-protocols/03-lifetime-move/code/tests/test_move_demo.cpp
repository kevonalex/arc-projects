// OBSERVATION tests: these use only the PROVIDED Probe type, so they pass from the start.
// Read them, predict the numbers BEFORE running (worksheet Q7-Q9), then confirm.
#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <es_lifetime/probe.hpp>

using namespace es::lifetime;

TEST(MoveDemo, StdMoveAloneMovesNothing) {
    Probe::reset();
    Probe a(5);
    Probe&& r = std::move(a);  // just a cast to rvalue reference
    (void)r;
    EXPECT_EQ(Probe::counts.move_ctor, 0);
    EXPECT_EQ(a.value, 5);  // untouched
}

TEST(MoveDemo, MoveConstructorDoesTheWork) {
    Probe::reset();
    Probe a(5);
    Probe b(std::move(a));
    EXPECT_EQ(Probe::counts.move_ctor, 1);
    EXPECT_EQ(Probe::counts.copy_ctor, 0);
    EXPECT_EQ(b.value, 5);
    EXPECT_EQ(a.value, -1);  // moved-from: valid but unspecified (our Probe marks it)
}

TEST(MoveDemo, ConstObjectCannotBeMovedFrom) {
    Probe::reset();
    const Probe c(7);
    Probe d(std::move(c));  // const Probe&& binds to const Probe& (the copy ctor)
    EXPECT_EQ(Probe::counts.move_ctor, 0);
    EXPECT_EQ(Probe::counts.copy_ctor, 1);
    EXPECT_EQ(d.value, 7);
}

TEST(MoveDemo, VectorGrowthMovesOnlyIfMoveIsNoexcept) {
    {
        std::vector<Probe> v;
        v.reserve(1);
        v.emplace_back();
        Probe::reset();
        v.emplace_back();  // capacity 1 -> reallocation: old element must be relocated
        EXPECT_EQ(Probe::counts.move_ctor, 1);
        EXPECT_EQ(Probe::counts.copy_ctor, 0);
    }
    {
        std::vector<SlowMoveProbe> v;
        v.reserve(1);
        v.emplace_back();
        SlowMoveProbe::reset();
        v.emplace_back();
        // If a move could throw halfway, the old storage would be half destroyed and the
        // strong guarantee lost, so the library copies instead (std::move_if_noexcept).
        EXPECT_EQ(SlowMoveProbe::counts.move_ctor, 0);
        EXPECT_EQ(SlowMoveProbe::counts.copy_ctor, 1);
    }
}

Probe make_probe() { return Probe(9); }  // prvalue

TEST(MoveDemo, GuaranteedElisionInCpp17) {
    Probe::reset();
    Probe x = make_probe();  // C++17: no temporary at all - constructed directly in x
    EXPECT_EQ(Probe::counts.ctor, 1);
    EXPECT_EQ(Probe::counts.move_ctor, 0);
    EXPECT_EQ(Probe::counts.copy_ctor, 0);
    EXPECT_EQ(x.value, 9);
}

TEST(MoveDemo, ReturnMovedLocalDefeatsElision) {
    // Anti-pattern: `return std::move(local);` prevents NRVO and forces a move.
    auto bad = [] {
        Probe p(3);
        return std::move(p);
    };
    Probe::reset();
    Probe x = bad();
    EXPECT_EQ(Probe::counts.move_ctor, 1);
    EXPECT_EQ(x.value, 3);
}

// Rule of zero: compose members that already manage themselves; write NO special members.
struct Message {
    std::string text;
    std::vector<int> ids;
};
static_assert(std::is_nothrow_move_constructible_v<Message>);
static_assert(std::is_copy_constructible_v<Message>);

TEST(RuleOfZero, CompilerGeneratedMembersAreCorrect) {
    Message a{"hello", {1, 2, 3}};
    Message b = a;             // deep copy for free
    Message c = std::move(a);  // steals both buffers for free
    EXPECT_EQ(b.ids.size(), 3u);
    EXPECT_EQ(c.text, "hello");
}
