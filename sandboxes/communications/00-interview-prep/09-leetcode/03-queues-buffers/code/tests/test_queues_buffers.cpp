#include <gtest/gtest.h>

#include <algorithm>
#include <queue>
#include <vector>

#include "queues_buffers.hpp"

using namespace lc;

// ---- 622 -------------------------------------------------------------------
TEST(Queue622, LeetCodeExample) {
    MyCircularQueue q(3);
    EXPECT_TRUE(q.enQueue(1));
    EXPECT_TRUE(q.enQueue(2));
    EXPECT_TRUE(q.enQueue(3));
    EXPECT_FALSE(q.enQueue(4));
    EXPECT_EQ(q.Rear(), 3);
    EXPECT_TRUE(q.isFull());
    EXPECT_TRUE(q.deQueue());
    EXPECT_TRUE(q.enQueue(4));
    EXPECT_EQ(q.Rear(), 4);
    EXPECT_EQ(q.Front(), 2);
}
TEST(Queue622, EmptyAndCapacityOne) {
    MyCircularQueue q(1);
    EXPECT_TRUE(q.isEmpty());
    EXPECT_FALSE(q.deQueue());
    EXPECT_EQ(q.Front(), -1);
    EXPECT_EQ(q.Rear(), -1);
    EXPECT_TRUE(q.enQueue(7));
    EXPECT_FALSE(q.enQueue(8));
    EXPECT_TRUE(q.isFull());
    EXPECT_TRUE(q.deQueue());
    EXPECT_TRUE(q.isEmpty());
}
TEST(Queue622, RandomAgainstStdQueue) {
    MyCircularQueue q(5);
    std::queue<int> ref;
    unsigned seed = 12345;
    for (int step = 0; step < 2000; ++step) {
        seed = seed * 1103515245u + 12345u;
        if ((seed >> 16) % 2 == 0) {
            const bool ok = q.enQueue(step);
            if (ref.size() < 5) { ASSERT_TRUE(ok); ref.push(step); } else { ASSERT_FALSE(ok); }
        } else {
            const bool ok = q.deQueue();
            if (!ref.empty()) { ASSERT_TRUE(ok); ref.pop(); } else { ASSERT_FALSE(ok); }
        }
        ASSERT_EQ(q.isEmpty(), ref.empty());
        if (!ref.empty()) { ASSERT_EQ(q.Front(), ref.front()); ASSERT_EQ(q.Rear(), ref.back()); }
    }
}

// ---- 641 -------------------------------------------------------------------
TEST(Deque641, LeetCodeExample) {
    MyCircularDeque d(3);
    EXPECT_TRUE(d.insertLast(1));
    EXPECT_TRUE(d.insertLast(2));
    EXPECT_TRUE(d.insertFront(3));
    EXPECT_FALSE(d.insertFront(4));
    EXPECT_EQ(d.getRear(), 2);
    EXPECT_TRUE(d.isFull());
    EXPECT_TRUE(d.deleteLast());
    EXPECT_TRUE(d.insertFront(4));
    EXPECT_EQ(d.getFront(), 4);
}
TEST(Deque641, WrapBackwardsFromZero) {
    MyCircularDeque d(4);
    EXPECT_TRUE(d.insertFront(1));  // head_ must wrap 0 -> 3, not underflow
    EXPECT_TRUE(d.insertFront(2));
    EXPECT_EQ(d.getFront(), 2);
    EXPECT_EQ(d.getRear(), 1);
    EXPECT_TRUE(d.deleteFront());
    EXPECT_TRUE(d.deleteLast());
    EXPECT_TRUE(d.isEmpty());
    EXPECT_FALSE(d.deleteLast());
    EXPECT_EQ(d.getFront(), -1);
}

// ---- 232 -------------------------------------------------------------------
TEST(Queue232, LeetCodeExample) {
    MyQueue q;
    q.push(1);
    q.push(2);
    EXPECT_EQ(q.peek(), 1);
    EXPECT_EQ(q.pop(), 1);
    EXPECT_FALSE(q.empty());
}
TEST(Queue232, InterleavedKeepsFifo) {
    MyQueue q;
    q.push(1);
    q.push(2);
    EXPECT_EQ(q.pop(), 1);  // out_ now holds {2}
    q.push(3);              // must NOT jump ahead of 2
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
    EXPECT_TRUE(q.empty());
}

// ---- 239 -------------------------------------------------------------------
TEST(Queue239, Normal) {
    EXPECT_EQ(max_sliding_window({1, 3, -1, -3, 5, 3, 6, 7}, 3), (std::vector<int>{3, 3, 5, 5, 6, 7}));
}
TEST(Queue239, Edges) {
    EXPECT_EQ(max_sliding_window({1}, 1), (std::vector<int>{1}));
    EXPECT_EQ(max_sliding_window({9, 8, 7, 6}, 2), (std::vector<int>{9, 8, 7}));   // decreasing: expiry matters
    EXPECT_EQ(max_sliding_window({1, 2, 3, 4}, 2), (std::vector<int>{2, 3, 4}));   // increasing: dominance matters
    EXPECT_EQ(max_sliding_window({5, 5, 5, 5}, 3), (std::vector<int>{5, 5}));      // equal values
    EXPECT_EQ(max_sliding_window({1, -1}, 1), (std::vector<int>{1, -1}));
    EXPECT_EQ(max_sliding_window({4, 2}, 2), (std::vector<int>{4}));               // k == n
}
TEST(Queue239, MatchesBruteForce) {
    std::vector<int> v;
    unsigned seed = 7;
    for (int i = 0; i < 200; ++i) { seed = seed * 1664525u + 1013904223u; v.push_back(static_cast<int>(seed >> 24) - 128); }
    for (int k : {1, 2, 5, 17, 200}) {
        std::vector<int> brute;
        for (std::size_t i = 0; i + static_cast<std::size_t>(k) <= v.size(); ++i)
            brute.push_back(*std::max_element(v.begin() + static_cast<long>(i), v.begin() + static_cast<long>(i) + k));
        EXPECT_EQ(max_sliding_window(v, k), brute) << "k=" << k;
    }
}

// ---- 346 -------------------------------------------------------------------
TEST(Queue346, LeetCodeExample) {
    MovingAverage m(3);
    EXPECT_DOUBLE_EQ(m.next(1), 1.0);
    EXPECT_DOUBLE_EQ(m.next(10), 5.5);
    EXPECT_NEAR(m.next(3), 14.0 / 3.0, 1e-9);
    EXPECT_DOUBLE_EQ(m.next(5), 6.0);  // (10+3+5)/3 -- the 1 was evicted
}
TEST(Queue346, SizeOneAndNegatives) {
    MovingAverage m(1);
    EXPECT_DOUBLE_EQ(m.next(5), 5.0);
    EXPECT_DOUBLE_EQ(m.next(-7), -7.0);
    MovingAverage big(2);
    EXPECT_DOUBLE_EQ(big.next(2147483647), 2147483647.0);
    EXPECT_DOUBLE_EQ(big.next(2147483647), 2147483647.0);  // sum needs 64 bits
}
