// Tests for es_rt/pool.hpp (worksheet Q23). Heap-freedom is checked separately in test_rt_alloc.cpp.
#include <es_rt/pool.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <set>

namespace {
struct Frame {
    std::uint8_t data[8];
    int len;
};
}  // namespace

TEST(FixedPool, CapacityAndAvailable) {
    es::rt::FixedPool<Frame, 4> pool;
    EXPECT_EQ(pool.capacity(), 4u);
    EXPECT_EQ(pool.available(), 4u);
    Frame* a = pool.acquire();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(pool.available(), 3u);
}

TEST(FixedPool, AcquiresDistinctElementsThenExhausts) {
    es::rt::FixedPool<Frame, 3> pool;
    std::set<Frame*> seen;
    for (int i = 0; i < 3; ++i) {
        Frame* f = pool.acquire();
        ASSERT_NE(f, nullptr) << "acquire #" << i;
        EXPECT_TRUE(seen.insert(f).second) << "same element handed out twice";
    }
    EXPECT_EQ(pool.available(), 0u);
    EXPECT_EQ(pool.acquire(), nullptr) << "exhausted pool returns nullptr (no throw, no allocation)";
}

TEST(FixedPool, FirstAcquireIsFirstElementAndAscending) {
    es::rt::FixedPool<Frame, 3> pool;
    Frame* a = pool.acquire();
    Frame* b = pool.acquire();
    Frame* c = pool.acquire();
    ASSERT_TRUE(a && b && c);
    EXPECT_LT(a, b);
    EXPECT_LT(b, c);
    EXPECT_EQ(b - a, 1);
    EXPECT_EQ(c - b, 1);
}

TEST(FixedPool, ReleaseThenAcquireReusesMostRecent) {
    es::rt::FixedPool<Frame, 3> pool;
    Frame* a = pool.acquire();
    Frame* b = pool.acquire();
    ASSERT_TRUE(a && b);
    EXPECT_TRUE(pool.release(a));
    EXPECT_EQ(pool.available(), 2u);
    EXPECT_EQ(pool.acquire(), a) << "LIFO reuse";
    EXPECT_TRUE(pool.release(b));
    EXPECT_TRUE(pool.release(a));
    EXPECT_EQ(pool.available(), 3u);
}

TEST(FixedPool, RejectsBadReleases) {
    es::rt::FixedPool<Frame, 3> pool;
    Frame* a = pool.acquire();
    ASSERT_NE(a, nullptr);

    Frame outside{};
    EXPECT_FALSE(pool.release(&outside)) << "pointer from elsewhere";
    EXPECT_FALSE(pool.release(nullptr));

    auto* misaligned = reinterpret_cast<Frame*>(reinterpret_cast<std::uint8_t*>(a) + 1);
    EXPECT_FALSE(pool.release(misaligned)) << "points into the middle of an element";

    EXPECT_TRUE(pool.release(a));
    EXPECT_FALSE(pool.release(a)) << "double release must be detected";
    EXPECT_EQ(pool.available(), 3u) << "failed releases must not change the pool";
}

TEST(FixedPool, ElementsAreUsableStorage) {
    es::rt::FixedPool<Frame, 2> pool;
    Frame* a = pool.acquire();
    Frame* b = pool.acquire();
    ASSERT_TRUE(a && b);
    a->len = 3;
    b->len = 5;
    a->data[7] = 0xAA;
    b->data[0] = 0xBB;
    EXPECT_EQ(a->len, 3);
    EXPECT_EQ(b->len, 5);
    EXPECT_EQ(a->data[7], 0xAA);
    EXPECT_EQ(b->data[0], 0xBB);
}

TEST(FixedPool, HotPathOperationsAreNoexcept) {
    es::rt::FixedPool<Frame, 2> pool;
    static_assert(noexcept(pool.acquire()), "acquire must be noexcept");
    static_assert(noexcept(pool.release(nullptr)), "release must be noexcept");
    SUCCEED();
}
