#include <gtest/gtest.h>

#include <type_traits>
#include <utility>
#include <vector>

#include <es_lifetime/buffer.hpp>

using es::lifetime::Buffer;

static_assert(std::is_nothrow_move_constructible_v<Buffer>, "vector must be able to MOVE Buffers");
static_assert(std::is_nothrow_move_assignable_v<Buffer>);

class BufferTest : public ::testing::Test {
protected:
    void SetUp() override { base_ = Buffer::live_blocks(); }
    long live() const { return Buffer::live_blocks() - base_; }  // blocks alive relative to test start
    long base_ = 0;
};

TEST_F(BufferTest, DefaultIsEmpty) {
    Buffer b;
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(b.data(), nullptr);
    EXPECT_EQ(live(), 0);
}

TEST_F(BufferTest, SizedIsZeroFilled) {
    Buffer b(4);
    EXPECT_EQ(b.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i) EXPECT_EQ(b[i], 0);
    EXPECT_EQ(live(), 1);
}

TEST_F(BufferTest, DestructorFreesTheBlock) {
    {
        Buffer b(16);
        EXPECT_EQ(live(), 1);
    }
    EXPECT_EQ(live(), 0);
}

TEST_F(BufferTest, CopyIsIndependentDeepCopy) {
    Buffer a(3);
    a[0] = 1; a[1] = 2; a[2] = 3;
    Buffer b(a);
    EXPECT_NE(a.data(), b.data());
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b[1], 2);
    b[1] = 99;
    EXPECT_EQ(a[1], 2) << "modifying the copy changed the original: shallow copy";
    EXPECT_EQ(live(), 2);
}

TEST_F(BufferTest, CopyOfEmptyIsEmpty) {
    Buffer a;
    Buffer b(a);
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(b.data(), nullptr);
}

TEST_F(BufferTest, CopyAssignDeepAndFreesOldBlock) {
    Buffer a(8);
    Buffer b(4);
    b[0] = 7;
    a = b;
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[0], 7);
    EXPECT_NE(a.data(), b.data());
    EXPECT_EQ(live(), 2) << "old block of `a` leaked, or a block was shared";
}

TEST_F(BufferTest, SelfCopyAssignmentIsSafe) {
    Buffer a(3);
    a[2] = 42;
    Buffer& alias = a;
    a = alias;
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[2], 42);
    EXPECT_EQ(live(), 1);
}

TEST_F(BufferTest, MoveConstructorStealsAndEmptiesSource) {
    Buffer a(5);
    a[4] = 9;
    const std::uint8_t* raw = a.data();
    Buffer b(std::move(a));
    EXPECT_EQ(b.data(), raw);          // same block: NOTHING was copied
    EXPECT_EQ(b[4], 9);
    EXPECT_EQ(a.data(), nullptr);      // NOLINT(bugprone-use-after-move): moved-from state is what we test
    EXPECT_EQ(a.size(), 0u);
    EXPECT_EQ(live(), 1) << "a move must not allocate";
}

TEST_F(BufferTest, MoveAssignFreesOldAndEmptiesSource) {
    Buffer a(8);
    Buffer b(4);
    const std::uint8_t* raw = b.data();
    a = std::move(b);
    EXPECT_EQ(a.data(), raw);
    EXPECT_EQ(a.size(), 4u);
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(live(), 1) << "the 8-byte block of `a` must have been freed";
}

TEST_F(BufferTest, SelfMoveAssignmentKeepsTheBlock) {
    Buffer a(6);
    a[5] = 1;
    Buffer& alias = a;
    a = std::move(alias);
    EXPECT_EQ(a.size(), 6u);
    EXPECT_EQ(a[5], 1);
    EXPECT_EQ(live(), 1);
}

TEST_F(BufferTest, MovedFromObjectCanBeReassignedAndDestroyed) {
    Buffer a(2);
    Buffer b(std::move(a));
    a = Buffer(3);  // assigning to a moved-from object is always legal
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(live(), 2);
}

TEST_F(BufferTest, VectorOfBuffersGrowsWithoutLeaksOrCopies) {
    {
        std::vector<Buffer> v;
        for (int i = 0; i < 50; ++i) v.emplace_back(8);
        EXPECT_EQ(live(), 50);
        for (const auto& b : v) EXPECT_EQ(b.size(), 8u);
    }
    EXPECT_EQ(live(), 0);
}
