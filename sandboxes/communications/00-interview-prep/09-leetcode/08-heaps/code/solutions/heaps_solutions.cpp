#include "heaps.hpp"

#include <functional>

namespace lc {

// 215 -- keep the k LARGEST seen so far in a MIN-heap; the root is the smallest of those = k-th largest.
// O(n log k) time, O(k) space.
int find_kth_largest(const std::vector<int>& nums, int k) {
    std::priority_queue<int, std::vector<int>, std::greater<int>> heap;
    for (int x : nums) {
        if (heap.size() < static_cast<std::size_t>(k)) {
            heap.push(x);
        } else if (x > heap.top()) {
            heap.pop();
            heap.push(x);
        }
    }
    return heap.top();
}

// 23 -- heap holds the current head of each non-empty list; pop the smallest, push its successor.
// O(N log k).
ListNode* merge_k_lists(const std::vector<ListNode*>& lists) {
    auto later = [](const ListNode* a, const ListNode* b) { return a->val > b->val; };
    std::priority_queue<ListNode*, std::vector<ListNode*>, decltype(later)> heap(later);
    for (ListNode* l : lists)
        if (l != nullptr) heap.push(l);
    ListNode dummy;
    ListNode* tail = &dummy;
    while (!heap.empty()) {
        ListNode* n = heap.top();
        heap.pop();
        tail->next = n;
        tail = n;
        if (n->next != nullptr) heap.push(n->next);
    }
    tail->next = nullptr;
    return dummy.next;
}

// 295 -- two heaps. Invariants: every element of lo_ <= every element of hi_,
// and lo_.size() == hi_.size() or lo_.size() == hi_.size() + 1.
void MedianFinder::addNum(int num) {
    if (lo_.empty() || num <= lo_.top()) lo_.push(num); else hi_.push(num);
    if (lo_.size() > hi_.size() + 1) {
        hi_.push(lo_.top());
        lo_.pop();
    } else if (hi_.size() > lo_.size()) {
        lo_.push(hi_.top());
        hi_.pop();
    }
}

double MedianFinder::findMedian() const {
    if (lo_.size() > hi_.size()) return static_cast<double>(lo_.top());
    return (static_cast<double>(lo_.top()) + static_cast<double>(hi_.top())) / 2.0;
}

}  // namespace lc
