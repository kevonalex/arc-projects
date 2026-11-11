// Tests for es_sm/fixed_vector.hpp (worksheet Q25, Q26). Probe is NOT default constructible and counts
// every construction/copy/move/destruction, so wrong lifetimes are caught exactly.
#include <es_sm/fixed_vector.hpp>

#include "probe.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>

using es::sm::FixedVector;
using probe::Probe;
using probe::counters;

class FixedVectorTest : public ::testing::Test {
protected:
    void SetUp() override { probe::reset(); }
    void TearDown() override {
        EXPECT_EQ(counters().live(), 0) << "leaked or double-destroyed elements (live != 0)";
    }
};

TEST_F(FixedVectorTest, EmptyConstructsNothing) {
    {
        FixedVector<Probe, 4> v;
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(v.size(), 0u);
        EXPECT_EQ(v.capacity(), 4u);
        EXPECT_EQ(v.data(), nullptr);
    }
    EXPECT_EQ(counters().ctors + counters().copies + counters().moves, 0) << "no slot may be constructed up front";
    EXPECT_EQ(counters().dtors, 0);
}

TEST_F(FixedVectorTest, EmplaceBackConstructsInPlaceWithoutCopies) {
    FixedVector<Probe, 4> v;
    Probe* p = v.emplace_back(7);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->v, 7);
    EXPECT_EQ(&v[0], p);
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(counters().ctors, 1);
    EXPECT_EQ(counters().copies, 0);
    EXPECT_EQ(counters().moves, 0);
}

TEST_F(FixedVectorTest, PushBackCopiesLvalueAndMovesRvalue) {
    FixedVector<Probe, 4> v;
    Probe a(1);  // counts as a ctor
    EXPECT_TRUE(v.push_back(a));
    EXPECT_EQ(counters().copies, 1);
    EXPECT_EQ(a.v, 1) << "lvalue untouched";
    EXPECT_TRUE(v.push_back(Probe(2)));  // temporary: moved in
    EXPECT_EQ(counters().moves, 1);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].v, 1);
    EXPECT_EQ(v[1].v, 2);
}

TEST_F(FixedVectorTest, FullRejectsAndConstructsNothing) {
    FixedVector<Probe, 2> v;
    EXPECT_NE(v.emplace_back(1), nullptr);
    EXPECT_NE(v.emplace_back(2), nullptr);
    EXPECT_TRUE(v.full());
    const int before = counters().ctors + counters().copies + counters().moves;
    EXPECT_EQ(v.emplace_back(3), nullptr);
    EXPECT_FALSE(v.push_back(Probe(4)));  // this temporary is constructed by the TEST, not the container
    EXPECT_EQ(counters().ctors + counters().copies + counters().moves, before + 1)
        << "only the test's temporary Probe(4) may have been constructed";
    EXPECT_EQ(v.size(), 2u);
}

TEST_F(FixedVectorTest, PopBackDestroysExactlyOne) {
    FixedVector<Probe, 4> v;
    v.emplace_back(1);
    v.emplace_back(2);
    v.emplace_back(3);
    v.pop_back();
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(counters().dtors, 1);
    EXPECT_EQ(v[1].v, 2);
    v.pop_back();
    v.pop_back();
    v.pop_back();  // already empty: no-op
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(counters().dtors, 3);
}

TEST_F(FixedVectorTest, DestructorDestroysOnlyLiveElements) {
    {
        FixedVector<Probe, 8> v;
        v.emplace_back(1);
        v.emplace_back(2);
        EXPECT_EQ(counters().dtors, 0);
    }
    EXPECT_EQ(counters().dtors, 2) << "2 live elements, 6 never-constructed slots";
}

TEST_F(FixedVectorTest, ClearDestroysInReverseAndIsReusable) {
    FixedVector<Probe, 4> v;
    v.emplace_back(1);
    v.emplace_back(2);
    v.clear();
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(counters().dtors, 2);
    ASSERT_NE(v.emplace_back(9), nullptr);
    EXPECT_EQ(v[0].v, 9);
}

TEST_F(FixedVectorTest, CopyConstructorCopiesEachElement) {
    FixedVector<Probe, 4> a;
    a.emplace_back(1);
    a.emplace_back(2);
    ASSERT_EQ(a.size(), 2u);
    const int copies_before = counters().copies;
    {
        FixedVector<Probe, 4> b(a);
        ASSERT_EQ(b.size(), 2u);
        EXPECT_EQ(counters().copies - copies_before, 2);
        EXPECT_EQ(b[0].v, 1);
        EXPECT_EQ(b[1].v, 2);
        EXPECT_NE(&a[0], &b[0]);
        EXPECT_EQ(a.size(), 2u) << "source unchanged";
    }
}

TEST_F(FixedVectorTest, MoveConstructorMovesAndEmptiesSource) {
    FixedVector<Probe, 4> a;
    a.emplace_back(1);
    a.emplace_back(2);
    const int dtors_before = counters().dtors;
    FixedVector<Probe, 4> b(std::move(a));
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0].v, 1);
    EXPECT_EQ(b[1].v, 2);
    EXPECT_EQ(counters().moves, 2);
    EXPECT_EQ(counters().copies, 0);
    EXPECT_EQ(a.size(), 0u) << "moved-from FixedVector is empty";
    EXPECT_EQ(counters().dtors - dtors_before, 2) << "the two moved-from elements of `a` were destroyed";
}

TEST_F(FixedVectorTest, MoveConstructorIsNoexceptForNothrowMovableT) {
    static_assert(std::is_nothrow_move_constructible_v<FixedVector<Probe, 4>>, "Probe moves are noexcept");
    SUCCEED();
}

// Elements need not be default constructible, and non-trivial types (heap inside) must not leak (ASan).
TEST_F(FixedVectorTest, NonTrivialElementsStrings) {
    FixedVector<std::string, 3> v;
    EXPECT_TRUE(v.push_back(std::string("a string long enough to live on the heap, not in SSO")));
    EXPECT_TRUE(v.push_back("second"));
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], "a string long enough to live on the heap, not in SSO");
    FixedVector<std::string, 3> w(v);
    ASSERT_EQ(w.size(), 2u);
    EXPECT_EQ(w[1], "second");
    FixedVector<std::string, 3> x(std::move(v));
    ASSERT_EQ(x.size(), 2u);
    EXPECT_EQ(x[0], "a string long enough to live on the heap, not in SSO");
    EXPECT_TRUE(v.empty());
}

TEST_F(FixedVectorTest, RangeForOverLiveElements) {
    FixedVector<Probe, 4> v;
    v.emplace_back(10);
    v.emplace_back(20);
    v.emplace_back(30);
    int sum = 0;
    for (const Probe& p : v) sum += p.v;
    EXPECT_EQ(sum, 60);
}

namespace {
struct alignas(16) Wide {
    std::uint8_t b[16];
};
}  // namespace

TEST(FixedVectorLayout, StorageIsAlignedForT) {
    FixedVector<Wide, 3> v;
    EXPECT_GE(alignof(FixedVector<Wide, 3>), 16u);
    EXPECT_TRUE(v.push_back(Wide{}));
    EXPECT_TRUE(v.push_back(Wide{}));
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&v[0]) % 16u, 0u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&v[1]) % 16u, 0u);
    EXPECT_EQ(reinterpret_cast<const char*>(&v[1]) - reinterpret_cast<const char*>(&v[0]),
              static_cast<std::ptrdiff_t>(sizeof(Wide)));
}

TEST(FixedVectorLayout, NoHeapObjectSizeIsStoragePlusCount) {
    // 4 doubles inline + a size counter, plus padding: bigger than the payload alone, and independent of use.
    EXPECT_GE(sizeof(FixedVector<double, 4>), 4 * sizeof(double) + sizeof(std::size_t));
}
