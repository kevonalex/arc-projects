#include <gtest/gtest.h>

#include <climits>
#include <map>

#include "design.hpp"

using namespace lc;

// ---- 146 -------------------------------------------------------------------
TEST(Design146, LeetCodeExample) {
    LRUCache c(2);
    c.put(1, 1);
    c.put(2, 2);
    EXPECT_EQ(c.get(1), 1);
    c.put(3, 3);  // evicts 2
    EXPECT_EQ(c.get(2), -1);
    c.put(4, 4);  // evicts 1
    EXPECT_EQ(c.get(1), -1);
    EXPECT_EQ(c.get(3), 3);
    EXPECT_EQ(c.get(4), 4);
}
TEST(Design146, UpdateRefreshesRecency) {
    LRUCache c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.put(1, 10);  // update, 1 becomes MRU, no eviction
    c.put(3, 3);   // evicts 2, not 1
    EXPECT_EQ(c.get(1), 10);
    EXPECT_EQ(c.get(2), -1);
    EXPECT_EQ(c.get(3), 3);
}
TEST(Design146, CapacityOne) {
    LRUCache c(1);
    c.put(1, 1);
    c.put(2, 2);
    EXPECT_EQ(c.get(1), -1);
    EXPECT_EQ(c.get(2), 2);
    c.put(2, 3);
    EXPECT_EQ(c.get(2), 3);
}

// ---- 359 -------------------------------------------------------------------
TEST(Design359, LeetCodeExample) {
    Logger l;
    EXPECT_TRUE(l.shouldPrintMessage(1, "foo"));
    EXPECT_TRUE(l.shouldPrintMessage(2, "bar"));
    EXPECT_FALSE(l.shouldPrintMessage(3, "foo"));
    EXPECT_FALSE(l.shouldPrintMessage(8, "bar"));
    EXPECT_FALSE(l.shouldPrintMessage(10, "foo"));
    EXPECT_TRUE(l.shouldPrintMessage(11, "foo"));
}
TEST(Design359, SuppressedAttemptDoesNotExtendWindow) {
    Logger l;
    EXPECT_TRUE(l.shouldPrintMessage(0, "x"));
    EXPECT_FALSE(l.shouldPrintMessage(9, "x"));
    EXPECT_TRUE(l.shouldPrintMessage(10, "x"));  // window measured from the last PRINT (0)
}

// ---- 362 -------------------------------------------------------------------
TEST(Design362, LeetCodeExample) {
    HitCounter h;
    h.hit(1);
    h.hit(2);
    h.hit(3);
    EXPECT_EQ(h.getHits(4), 3);
    h.hit(300);
    EXPECT_EQ(h.getHits(300), 4);
    EXPECT_EQ(h.getHits(301), 3);  // hit at t=1 expired (301-1 = 300, window is 300 wide)
}
TEST(Design362, BucketReuseAndSameSecond) {
    HitCounter h;
    h.hit(5);
    h.hit(5);
    h.hit(5);
    EXPECT_EQ(h.getHits(5), 3);
    h.hit(305);  // same bucket as 5; old count must be discarded
    EXPECT_EQ(h.getHits(305), 1);
    EXPECT_EQ(h.getHits(1000), 0);
}

// ---- 705 / 706 -------------------------------------------------------------
TEST(Design705, LeetCodeExample) {
    MyHashSet s;
    s.add(1);
    s.add(2);
    EXPECT_TRUE(s.contains(1));
    EXPECT_FALSE(s.contains(3));
    s.add(2);
    EXPECT_TRUE(s.contains(2));
    s.remove(2);
    EXPECT_FALSE(s.contains(2));
    s.remove(99);  // removing a missing key is a no-op
}
TEST(Design705, CollisionsAndZero) {
    MyHashSet s;
    s.add(0);
    s.add(769);  // same bucket as 0
    s.remove(0);
    EXPECT_FALSE(s.contains(0));
    EXPECT_TRUE(s.contains(769));
    s.add(1000000);
    EXPECT_TRUE(s.contains(1000000));
}
TEST(Design706, LeetCodeExample) {
    MyHashMap m;
    m.put(1, 1);
    m.put(2, 2);
    EXPECT_EQ(m.get(1), 1);
    EXPECT_EQ(m.get(3), -1);
    m.put(2, 1);
    EXPECT_EQ(m.get(2), 1);
    m.remove(2);
    EXPECT_EQ(m.get(2), -1);
}
TEST(Design706, CollisionChain) {
    MyHashMap m;
    m.put(5, 50);
    m.put(5 + 769, 60);
    m.put(5 + 2 * 769, 70);
    m.remove(5 + 769);
    EXPECT_EQ(m.get(5), 50);
    EXPECT_EQ(m.get(5 + 769), -1);
    EXPECT_EQ(m.get(5 + 2 * 769), 70);
}

// ---- 155 -------------------------------------------------------------------
TEST(Design155, LeetCodeExample) {
    MinStack s;
    s.push(-2);
    s.push(0);
    s.push(-3);
    EXPECT_EQ(s.getMin(), -3);
    s.pop();
    EXPECT_EQ(s.top(), 0);
    EXPECT_EQ(s.getMin(), -2);
}
TEST(Design155, DuplicateMinimum) {
    MinStack s;
    s.push(1);
    s.push(1);
    s.pop();
    EXPECT_EQ(s.getMin(), 1);  // a naive single 'min' variable fails here
    s.push(INT_MIN);
    s.push(INT_MAX);
    EXPECT_EQ(s.getMin(), INT_MIN);
    s.pop();
    s.pop();
    EXPECT_EQ(s.getMin(), 1);
}
