// Tests for es_sm/ring_buffer.hpp (worksheet Q22, Q23).
#include <es_sm/ring_buffer.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

using es::sm::RingBuffer;

TEST(Ring, StartsEmpty) {
    RingBuffer<int, 4> r;
    EXPECT_TRUE(r.empty());
    EXPECT_FALSE(r.full());
    EXPECT_EQ(r.size(), 0u);
    EXPECT_EQ(r.capacity(), 4u);
    EXPECT_FALSE(r.pop().has_value());
}

TEST(Ring, FifoOrder) {
    RingBuffer<int, 4> r;
    EXPECT_TRUE(r.push(1));
    EXPECT_TRUE(r.push(2));
    EXPECT_TRUE(r.push(3));
    EXPECT_EQ(r.size(), 3u);
    EXPECT_EQ(r.pop().value_or(-1), 1);
    EXPECT_EQ(r.pop().value_or(-1), 2);
    EXPECT_EQ(r.pop().value_or(-1), 3);
    EXPECT_TRUE(r.empty());
}

TEST(Ring, FullRejectsPushAndKeepsContents) {
    RingBuffer<int, 4> r;
    for (int i = 0; i < 4; ++i) EXPECT_TRUE(r.push(i)) << i;
    EXPECT_TRUE(r.full());
    EXPECT_EQ(r.size(), 4u);
    EXPECT_FALSE(r.push(99));
    EXPECT_EQ(r.size(), 4u);
    EXPECT_EQ(r.pop().value_or(-1), 0) << "rejected push must not have overwritten the oldest";
}

TEST(Ring, WrapsAroundManyTimes) {
    RingBuffer<int, 8> r;
    int next_in = 0, next_out = 0;
    for (int round = 0; round < 1000; ++round) {
        // vary the fill level so head/tail wander around the buffer
        const int burst = 1 + (round % 8);
        for (int i = 0; i < burst; ++i) {
            if (r.push(next_in)) ++next_in;
        }
        const int take = 1 + ((round * 3) % 8);
        for (int i = 0; i < take; ++i) {
            auto v = r.pop();
            if (!v) break;
            ASSERT_EQ(*v, next_out) << "order broken at round " << round;
            ++next_out;
        }
    }
    EXPECT_EQ(static_cast<std::size_t>(next_in - next_out), r.size());
}

TEST(Ring, IndexSubscriptIsLogicalFromOldest) {
    RingBuffer<int, 4> r;
    for (int i = 0; i < 4; ++i) r.push(10 + i);
    r.pop();
    r.pop();
    r.push(14);
    r.push(15);  // physical layout now wrapped
    ASSERT_EQ(r.size(), 4u);
    EXPECT_EQ(r[0], 12);
    EXPECT_EQ(r[1], 13);
    EXPECT_EQ(r[2], 14);
    EXPECT_EQ(r[3], 15);
}

TEST(Ring, PushOverwriteDropsOldest) {
    RingBuffer<int, 4> r;
    for (int i = 1; i <= 6; ++i) r.push_overwrite(i);  // 1..6 into capacity 4 -> keeps 3,4,5,6
    EXPECT_EQ(r.size(), 4u);
    EXPECT_EQ(r[0], 3);
    EXPECT_EQ(r[3], 6);
    EXPECT_EQ(r.pop().value_or(-1), 3);
}

TEST(Ring, Clear) {
    RingBuffer<int, 4> r;
    r.push(1);
    r.push(2);
    r.clear();
    EXPECT_TRUE(r.empty());
    EXPECT_TRUE(r.push(7));
    EXPECT_EQ(r.pop().value_or(-1), 7);
}

TEST(Ring, WorksWithNonTrivialTypes) {
    RingBuffer<std::string, 4> r;
    r.push("a fairly long string that does not fit any small-string buffer");
    r.push("second");
    auto s = r.pop();
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(*s, "a fairly long string that does not fit any small-string buffer");
    EXPECT_EQ(r.pop().value_or(""), "second");
}

// The point of `Index`: with 8-bit counters the wrap-around (256) happens quickly, so a bug in the
// head/tail arithmetic (integer promotion! pack 02) shows up after only 256 pushes.
TEST(Ring, EightBitCountersSurviveWrapAround) {
    RingBuffer<std::uint16_t, 8, std::uint8_t> r;
    std::uint16_t next_in = 0, next_out = 0;
    for (int step = 0; step < 5000; ++step) {
        // push 3, pop 3 -> counters advance by 3 each step; wrap every ~85 steps
        for (int i = 0; i < 3; ++i) {
            ASSERT_TRUE(r.push(next_in)) << "step " << step;
            ++next_in;
        }
        EXPECT_EQ(r.size(), 3u) << "size() wrong after counter wrap at step " << step;
        for (int i = 0; i < 3; ++i) {
            auto v = r.pop();
            ASSERT_TRUE(v.has_value()) << "step " << step;
            ASSERT_EQ(*v, next_out) << "step " << step;
            ++next_out;
        }
        ASSERT_TRUE(r.empty());
    }
}

TEST(Ring, EightBitCountersFullAndEmptyAcrossWrap) {
    RingBuffer<int, 4, std::uint8_t> r;
    // advance counters to just below the wrap point: 254 pushes+pops
    for (int i = 0; i < 254; ++i) {
        ASSERT_TRUE(r.push(i));
        ASSERT_TRUE(r.pop().has_value());
    }
    for (int i = 0; i < 4; ++i) ASSERT_TRUE(r.push(1000 + i));  // head wraps 254 -> 258 mod 256
    EXPECT_TRUE(r.full());
    EXPECT_EQ(r.size(), 4u);
    EXPECT_FALSE(r.push(1));
    for (int i = 0; i < 4; ++i) EXPECT_EQ(r.pop().value_or(-1), 1000 + i);
    EXPECT_TRUE(r.empty());
}

// ALWAYS PASSES - a demonstration for worksheet 3.4: why N must be a power of two for free-running counters.
TEST(RingDemo, ModuloIndexSkipsSlotsAtCounterWrapWhenNIsNotPowerOfTwo) {
    // N = 6, 8-bit counter. Slots visited by consecutive counter values 254, 255, 0, 1:
    const unsigned n = 6;
    const std::uint8_t c254 = 254, c255 = 255, c0 = static_cast<std::uint8_t>(c255 + 1);  // wraps to 0
    EXPECT_EQ(c254 % n, 2u);
    EXPECT_EQ(c255 % n, 3u);
    EXPECT_EQ(c0 % n, 0u) << "after slot 3 the ring jumps to slot 0: slots 4 and 5 were skipped";
    // With N = 8 (power of two): 254 & 7 = 6, 255 & 7 = 7, 0 & 7 = 0: continuous, nothing skipped.
    EXPECT_EQ(c254 & 7u, 6u);
    EXPECT_EQ(c255 & 7u, 7u);
    EXPECT_EQ(c0 & 7u, 0u);
}
