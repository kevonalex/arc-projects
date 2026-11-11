#include <gtest/gtest.h>

#include <vector>

#include "linked_lists.hpp"

using namespace lc;
using V = std::vector<int>;

// ---- 206 -------------------------------------------------------------------
TEST(List206, Normal) {
    ListArena a;
    EXPECT_EQ(to_vector(reverse_list(a.build({1, 2, 3, 4, 5}))), (V{5, 4, 3, 2, 1}));
    EXPECT_EQ(to_vector(reverse_list(a.build({1, 2}))), (V{2, 1}));
}
TEST(List206, Edges) {
    ListArena a;
    EXPECT_EQ(reverse_list(nullptr), nullptr);
    EXPECT_EQ(to_vector(reverse_list(a.build({7}))), (V{7}));
}

// ---- 21 --------------------------------------------------------------------
TEST(List21, Normal) {
    ListArena a;
    EXPECT_EQ(to_vector(merge_two_lists(a.build({1, 2, 4}), a.build({1, 3, 4}))), (V{1, 1, 2, 3, 4, 4}));
}
TEST(List21, Edges) {
    ListArena a;
    EXPECT_EQ(merge_two_lists(nullptr, nullptr), nullptr);
    EXPECT_EQ(to_vector(merge_two_lists(nullptr, a.build({0}))), (V{0}));
    EXPECT_EQ(to_vector(merge_two_lists(a.build({5}), nullptr)), (V{5}));
    EXPECT_EQ(to_vector(merge_two_lists(a.build({1, 2, 3}), a.build({7, 8}))), (V{1, 2, 3, 7, 8}));  // leftover splice
    EXPECT_EQ(to_vector(merge_two_lists(a.build({7, 8}), a.build({1, 2, 3}))), (V{1, 2, 3, 7, 8}));
    EXPECT_EQ(to_vector(merge_two_lists(a.build({-3, -3}), a.build({-3}))), (V{-3, -3, -3}));
}
TEST(List21, StableOnTies) {
    ListArena a;
    ListNode* x = a.build({1});
    ListNode* y = a.build({1});
    ListNode* m = merge_two_lists(x, y);
    EXPECT_EQ(m, x);  // equal keys: first list's node comes first
    EXPECT_EQ(m->next, y);
}

// ---- 141 -------------------------------------------------------------------
TEST(List141, NoCycle) {
    ListArena a;
    EXPECT_FALSE(has_cycle(nullptr));
    EXPECT_FALSE(has_cycle(a.build({1})));
    EXPECT_FALSE(has_cycle(a.build({1, 2})));
    EXPECT_FALSE(has_cycle(a.build({1, 2, 3, 4, 5, 6})));
}
TEST(List141, Cycles) {
    ListArena a;
    {
        ListNode* h = a.build({1});
        h->next = h;  // self loop
        EXPECT_TRUE(has_cycle(h));
    }
    {
        ListNode* h = a.build({3, 2, 0, -4});
        h->next->next->next->next = h->next;  // tail -> second node (LeetCode example 1)
        EXPECT_TRUE(has_cycle(h));
    }
    {
        ListNode* h = a.build({1, 2});
        h->next->next = h;  // full ring
        EXPECT_TRUE(has_cycle(h));
    }
    {
        // long tail, short loop: tortoise and hare must still meet
        ListNode* h = a.build(V(1000, 1));
        ListNode* t = h;
        while (t->next) t = t->next;
        ListNode* loop_start = h;
        for (int i = 0; i < 997; ++i) loop_start = loop_start->next;
        t->next = loop_start;
        EXPECT_TRUE(has_cycle(h));
    }
}

// ---- 19 --------------------------------------------------------------------
TEST(List19, Normal) {
    ListArena a;
    EXPECT_EQ(to_vector(remove_nth_from_end(a.build({1, 2, 3, 4, 5}), 2)), (V{1, 2, 3, 5}));
    EXPECT_EQ(to_vector(remove_nth_from_end(a.build({1, 2}), 1)), (V{1}));
}
TEST(List19, Edges) {
    ListArena a;
    EXPECT_EQ(remove_nth_from_end(a.build({1}), 1), nullptr);                       // single node
    EXPECT_EQ(to_vector(remove_nth_from_end(a.build({1, 2}), 2)), (V{2}));          // remove the head
    EXPECT_EQ(to_vector(remove_nth_from_end(a.build({1, 2, 3}), 3)), (V{2, 3}));    // n == length
    EXPECT_EQ(to_vector(remove_nth_from_end(a.build({1, 2, 3}), 1)), (V{1, 2}));    // remove the tail
}
