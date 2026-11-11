// 08-heaps  --  stubs for priority-queue problems.
//
// Same ownership rule as pack 07: merge_k_lists relinks existing nodes and never frees them.
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <queue>
#include <vector>

namespace lc {

struct ListNode {
    int val;
    ListNode* next;
    explicit ListNode(int v = 0, ListNode* n = nullptr) : val(v), next(n) {}
};

class ListArena {
public:
    ListNode* make(int v) {
        nodes_.push_back(std::make_unique<ListNode>(v));
        return nodes_.back().get();
    }
    ListNode* build(const std::vector<int>& values) {
        ListNode dummy;
        ListNode* tail = &dummy;
        for (int v : values) {
            tail->next = make(v);
            tail = tail->next;
        }
        return dummy.next;
    }

private:
    std::vector<std::unique_ptr<ListNode>> nodes_;
};

inline std::vector<int> to_vector(const ListNode* head) {
    std::vector<int> out;
    for (; head != nullptr; head = head->next) out.push_back(head->val);
    return out;
}

// 215  Kth Largest Element in an Array.  Context: k-th strongest RSSI among N cells (handover candidate threshold).
// 1 <= k <= nums.size(). Duplicates count separately.
int find_kth_largest(const std::vector<int>& nums, int k);

// 23   Merge k Sorted Lists.  Context: merge k per-antenna timestamped event logs into one global timeline.
ListNode* merge_k_lists(const std::vector<ListNode*>& lists);

// 295  Find Median from Data Stream.  Context: running median of RTT/jitter samples (robust to outliers).
class MedianFinder {
public:
    void addNum(int num);
    double findMedian() const;  // precondition: at least one number added

private:
    std::priority_queue<int> lo_;                                             // max-heap: smaller half
    std::priority_queue<int, std::vector<int>, std::greater<int>> hi_;        // min-heap: larger half
};

}  // namespace lc
