// Tests for es_sm/spsc_queue.hpp (worksheet Q24). Run again with -DENABLE_TSAN=ON: on x86 wrong memory
// orders often pass at run time; TSan checks the happens-before edges you wrote.
#include <es_sm/spsc_queue.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using es::sm::SpscQueue;
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

TEST(Spsc, EmptyAndFullSingleThread) {
    SpscQueue<int, 4> q;
    int out = -1;
    EXPECT_FALSE(q.try_pop(out));
    EXPECT_EQ(out, -1) << "failed pop must not modify out";
    EXPECT_EQ(q.size_approx(), 0u);
    for (int i = 0; i < 4; ++i) EXPECT_TRUE(q.try_push(i)) << i;
    EXPECT_FALSE(q.try_push(99)) << "capacity is exactly N";
    EXPECT_EQ(q.size_approx(), 4u);
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.try_pop(out));
        EXPECT_EQ(out, i);
    }
    EXPECT_FALSE(q.try_pop(out));
}

TEST(Spsc, WrapAroundSingleThread) {
    SpscQueue<int, 4> q;
    int out = 0, next_in = 0, next_out = 0;
    for (int round = 0; round < 500; ++round) {
        for (int i = 0; i < 3; ++i) {
            if (q.try_push(next_in)) ++next_in;
        }
        for (int i = 0; i < 2; ++i) {
            if (q.try_pop(out)) {
                ASSERT_EQ(out, next_out);
                ++next_out;
            }
        }
    }
    EXPECT_EQ(static_cast<std::size_t>(next_in - next_out), q.size_approx());
}

TEST(Spsc, IndicesLiveOnSeparateCacheLines) {
    // Layout fact: three alignas(64) members => the object is at least 3 lines and 64-byte aligned.
    EXPECT_GE(alignof(SpscQueue<int, 4>), 64u);
    EXPECT_GE(sizeof(SpscQueue<int, 4>), 3u * 64u);
}

TEST(Spsc, SizeIsLockFreeOnThisPlatform) {
    std::atomic<std::size_t> probe{0};
    EXPECT_TRUE(probe.is_lock_free()) << "an index that takes a lock defeats the purpose (VERIFY on new targets)";
}

namespace {
struct Msg {
    std::uint32_t seq = 0;
    std::uint32_t twice = 0;
    std::uint32_t thrice = 0;
    std::uint32_t pad = 0;
};
}  // namespace

TEST(Spsc, ProducerConsumerOrderAndIntegrity) {
    constexpr std::uint32_t kN = 200000;
    SpscQueue<Msg, 64> q;
    const auto deadline = Clock::now() + 10s;  // a broken queue fails after 10 s instead of hanging
    std::atomic<bool> timed_out{false};
    std::vector<Msg> got;
    got.reserve(kN);

    std::thread producer([&] {
        for (std::uint32_t i = 1; i <= kN; ++i) {
            const Msg m{i, 2 * i, 3 * i, 0};
            while (!q.try_push(m)) {
                if (Clock::now() > deadline) {
                    timed_out = true;
                    return;
                }
                std::this_thread::yield();
            }
        }
    });
    std::thread consumer([&] {
        Msg m;
        while (got.size() < kN) {
            if (q.try_pop(m)) {
                got.push_back(m);
            } else {
                if (Clock::now() > deadline) {
                    timed_out = true;
                    return;
                }
                std::this_thread::yield();
            }
        }
    });
    producer.join();
    consumer.join();

    EXPECT_FALSE(timed_out.load());
    ASSERT_EQ(got.size(), static_cast<std::size_t>(kN));
    for (std::uint32_t i = 0; i < kN; ++i) {
        ASSERT_EQ(got[i].seq, i + 1) << "lost / duplicated / reordered at " << i;
        ASSERT_EQ(got[i].twice, 2 * got[i].seq) << "torn element at " << i;
        ASSERT_EQ(got[i].thrice, 3 * got[i].seq) << "torn element at " << i;
    }
}

TEST(Spsc, SizeApproxNeverExceedsCapacityWhileRunning) {
    SpscQueue<int, 8> q;
    std::atomic<bool> stop{false};
    std::atomic<bool> bad{false};
    std::thread producer([&] {
        int i = 0;
        while (!stop.load(std::memory_order_acquire)) q.try_push(i++);
    });
    std::thread consumer([&] {
        int out;
        while (!stop.load(std::memory_order_acquire)) q.try_pop(out);
    });
    const auto until = Clock::now() + 200ms;
    while (Clock::now() < until) {
        if (q.size_approx() > q.capacity()) bad = true;
    }
    stop.store(true, std::memory_order_release);
    producer.join();
    consumer.join();
    EXPECT_FALSE(bad.load());
}
