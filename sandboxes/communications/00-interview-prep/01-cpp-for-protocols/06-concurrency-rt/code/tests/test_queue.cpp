// Tests for es_rt/bounded_queue.hpp (worksheet Q20). Every blocking test is bounded: a broken queue
// must FAIL, not hang (ctest also enforces a 60 s TIMEOUT).
#include <es_rt/bounded_queue.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using es::rt::BoundedQueue;
using namespace std::chrono_literals;

TEST(BoundedQueue, FifoSingleThread) {
    BoundedQueue<int> q(4);
    EXPECT_TRUE(q.push(10));
    EXPECT_TRUE(q.push(20));
    EXPECT_TRUE(q.push(30));
    EXPECT_EQ(q.size(), 3u);
    auto a = q.pop();
    auto b = q.pop();
    auto c = q.pop();
    ASSERT_TRUE(a && b && c);
    EXPECT_EQ(*a, 10);
    EXPECT_EQ(*b, 20);
    EXPECT_EQ(*c, 30);
    EXPECT_EQ(q.size(), 0u);
}

TEST(BoundedQueue, CloseDrainsThenReportsEnd) {
    BoundedQueue<int> q(4);
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    q.close();
    EXPECT_FALSE(q.push(3)) << "push after close must fail";
    auto a = q.pop();
    auto b = q.pop();
    auto c = q.pop();
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*a, 1);
    EXPECT_EQ(*b, 2);
    EXPECT_FALSE(c.has_value()) << "closed AND drained -> nullopt";
}

TEST(BoundedQueue, PushBlocksWhenFullUntilPop) {
    BoundedQueue<int> q(2);
    std::atomic<bool> third_done{false};
    std::thread producer([&] {
        q.push(1);
        q.push(2);
        q.push(3);  // must block: capacity is 2
        third_done = true;
    });
    // Give the producer ample time to (wrongly) get through if push does not block.
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(third_done.load()) << "third push should still be blocked";
    EXPECT_LE(q.size(), 2u) << "queue must never exceed its capacity";
    auto x = q.pop();  // frees a slot: producer can finish
    q.close();         // safety net so the test cannot hang if the implementation is broken
    producer.join();
    ASSERT_TRUE(x.has_value());
    EXPECT_EQ(*x, 1);
}

TEST(BoundedQueue, CloseWakesBlockedConsumer) {
    BoundedQueue<int> q(2);
    std::atomic<bool> got_end{false};
    std::thread consumer([&] {
        auto v = q.pop();  // blocks: queue is empty
        got_end = !v.has_value();
    });
    std::this_thread::sleep_for(50ms);
    q.close();
    consumer.join();
    EXPECT_TRUE(got_end.load());
}

TEST(BoundedQueue, CloseWakesBlockedProducer) {
    BoundedQueue<int> q(1);
    ASSERT_TRUE(q.push(1));
    std::atomic<int> result{-1};  // 0 = push failed (expected), 1 = push succeeded
    std::thread producer([&] { result = q.push(2) ? 1 : 0; });  // blocks: full
    std::this_thread::sleep_for(50ms);
    q.close();
    producer.join();
    EXPECT_EQ(result.load(), 0);
}

TEST(BoundedQueue, OneProducerOneConsumerDeliversEverythingInOrder) {
    constexpr int kN = 20000;
    BoundedQueue<int> q(8);
    std::vector<int> received;
    received.reserve(kN);
    std::thread consumer([&] {
        while (auto v = q.pop()) received.push_back(*v);
    });
    std::thread producer([&] {
        for (int i = 0; i < kN; ++i) {
            if (!q.push(i)) break;
        }
        q.close();
    });
    producer.join();
    consumer.join();
    ASSERT_EQ(received.size(), static_cast<std::size_t>(kN));
    for (int i = 0; i < kN; ++i) {
        ASSERT_EQ(received[static_cast<std::size_t>(i)], i) << "FIFO order broken at " << i;
    }
}

TEST(BoundedQueue, ManyProducersManyConsumersNothingLostNothingDuplicated) {
    constexpr int kProducers = 4, kConsumers = 3, kPerProducer = 5000;
    BoundedQueue<std::uint64_t> q(16);
    std::atomic<std::uint64_t> sum{0};
    std::atomic<std::uint64_t> count{0};

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&] {
            while (auto v = q.pop()) {
                sum += *v;
                ++count;
            }
        });
    }
    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&, p] {
            for (int i = 1; i <= kPerProducer; ++i) {
                if (!q.push(static_cast<std::uint64_t>(p) * 1000000u + static_cast<std::uint64_t>(i))) return;
            }
        });
    }
    for (auto& t : producers) t.join();
    q.close();  // all producers finished: consumers drain, then see nullopt
    for (auto& t : consumers) t.join();

    std::uint64_t expected = 0;
    for (int p = 0; p < kProducers; ++p)
        for (int i = 1; i <= kPerProducer; ++i)
            expected += static_cast<std::uint64_t>(p) * 1000000u + static_cast<std::uint64_t>(i);
    EXPECT_EQ(count.load(), static_cast<std::uint64_t>(kProducers) * kPerProducer);
    EXPECT_EQ(sum.load(), expected);
}

TEST(BoundedQueue, MoveOnlyPayload) {
    BoundedQueue<std::unique_ptr<int>> q(2);
    EXPECT_TRUE(q.push(std::make_unique<int>(7)));
    auto v = q.pop();
    ASSERT_TRUE(v.has_value());
    ASSERT_NE(v->get(), nullptr);
    EXPECT_EQ(**v, 7);
}
