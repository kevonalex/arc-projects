// Tests for es_rt/mailbox.hpp and es_rt/counters.hpp (worksheet Q21, Q22).
// NOTE: on x86 a relaxed/incorrect ordering often still "passes" - x86 is a strong memory model.
// Run once with -DENABLE_TSAN=ON (see README): ThreadSanitizer checks the happens-before edges you wrote
// instead of relying on what your CPU happens to do.
#include <es_rt/counters.hpp>
#include <es_rt/mailbox.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

struct Msg {
    std::uint32_t seq = 0;
    std::uint32_t twice = 0;   // invariant: twice == 2 * seq
    std::uint32_t thrice = 0;  // invariant: thrice == 3 * seq
};

using Clock = std::chrono::steady_clock;

}  // namespace

TEST(Mailbox, SingleThreadSemantics) {
    es::rt::Mailbox<int> mb;
    int out = -1;
    EXPECT_FALSE(mb.try_take(out)) << "empty mailbox";
    EXPECT_EQ(out, -1) << "failed take must not modify out";
    EXPECT_TRUE(mb.try_publish(5));
    EXPECT_FALSE(mb.try_publish(6)) << "slot still full: the old value must not be overwritten";
    EXPECT_TRUE(mb.try_take(out));
    EXPECT_EQ(out, 5);
    EXPECT_FALSE(mb.try_take(out)) << "taken once only";
    EXPECT_TRUE(mb.try_publish(7)) << "slot is free again";
    EXPECT_TRUE(mb.try_take(out));
    EXPECT_EQ(out, 7);
}

TEST(Mailbox, ProducerConsumerNoTornOrLostMessages) {
    constexpr std::uint32_t kN = 50000;
    es::rt::Mailbox<Msg> mb;
    std::atomic<bool> producer_timed_out{false};
    std::atomic<bool> consumer_timed_out{false};
    std::vector<Msg> received;
    received.reserve(kN);

    const auto deadline = Clock::now() + 10s;  // bound the whole test: failure, never a hang

    std::thread producer([&] {
        for (std::uint32_t i = 1; i <= kN; ++i) {
            const Msg m{i, 2 * i, 3 * i};
            while (!mb.try_publish(m)) {
                if (Clock::now() > deadline) {
                    producer_timed_out = true;
                    return;
                }
                std::this_thread::yield();
            }
        }
    });
    std::thread consumer([&] {
        while (received.size() < kN) {
            Msg m;
            if (mb.try_take(m)) {
                received.push_back(m);
            } else {
                if (Clock::now() > deadline) {
                    consumer_timed_out = true;
                    return;
                }
                std::this_thread::yield();
            }
        }
    });
    producer.join();
    consumer.join();

    EXPECT_FALSE(producer_timed_out.load());
    EXPECT_FALSE(consumer_timed_out.load());
    ASSERT_EQ(received.size(), static_cast<std::size_t>(kN));
    for (std::uint32_t i = 0; i < kN; ++i) {
        const Msg& m = received[i];
        ASSERT_EQ(m.seq, i + 1) << "lost / duplicated / reordered message at index " << i;
        ASSERT_EQ(m.twice, 2 * m.seq) << "torn message: payload not fully visible";
        ASSERT_EQ(m.thrice, 3 * m.seq) << "torn message: payload not fully visible";
    }
}

TEST(EventCounter, ManyThreadsNoLostUpdates) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 100000;
    es::rt::EventCounter c;
    std::vector<std::thread> ts;
    for (int t = 0; t < kThreads; ++t) {
        ts.emplace_back([&] {
            for (int i = 0; i < kPerThread; ++i) c.add();
        });
    }
    for (auto& t : ts) t.join();
    EXPECT_EQ(c.load(), static_cast<std::uint64_t>(kThreads) * kPerThread);
}

TEST(EventCounter, AddByN) {
    es::rt::EventCounter c;
    c.add(5);
    c.add(7);
    c.add();
    EXPECT_EQ(c.load(), 13u);
}

TEST(StopFlag, StartsClearThenSticks) {
    es::rt::StopFlag f;
    EXPECT_FALSE(f.requested());
    f.request();
    EXPECT_TRUE(f.requested());
    EXPECT_TRUE(f.requested());
}

TEST(StopFlag, WorkerSeesDataWrittenBeforeRequest) {
    es::rt::StopFlag f;
    int final_config = 0;  // plain int: published by the release/acquire pair, NOT by being atomic
    int seen = -1;
    bool timed_out = false;

    std::thread worker([&] {
        const auto deadline = Clock::now() + 5s;
        while (!f.requested()) {
            if (Clock::now() > deadline) {
                timed_out = true;
                return;
            }
            std::this_thread::yield();
        }
        seen = final_config;  // must observe 99
    });
    final_config = 99;
    f.request();
    worker.join();
    EXPECT_FALSE(timed_out);
    EXPECT_EQ(seen, 99);
}

// Always passes - a demonstration for worksheet 3.3: which atomics are lock-free is a PLATFORM fact.
TEST(AtomicFacts, PrintLockFreeness) {
    std::atomic<std::uint8_t> a8{0};
    std::atomic<std::uint32_t> a32{0};
    std::atomic<std::uint64_t> a64{0};
    std::cout << "[facts] atomic<u8> lock-free: " << a8.is_lock_free()
              << ", u32: " << a32.is_lock_free()
              << ", u64: " << a64.is_lock_free() << "\n";
    SUCCEED();
}
