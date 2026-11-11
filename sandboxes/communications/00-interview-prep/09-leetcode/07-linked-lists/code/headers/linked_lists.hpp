// 07-linked-lists  --  stubs for pointer-surgery problems.
//
// OWNERSHIP RULE for this pack: the functions below NEVER delete nodes. The caller
// (here: ListArena in the tests) owns every node -- exactly like intrusive lists whose
// nodes live in a packet-buffer pool. That also makes cyclic lists safe to build in tests.
#pragma once

#include <memory>
#include <vector>

namespace lc {

struct ListNode {
    int val;
    ListNode* next;
    explicit ListNode(int v = 0, ListNode* n = nullptr) : val(v), next(n) {}
};

// Owns nodes; frees them all on destruction (works even if the list has a cycle).
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

// Walks at most `limit` nodes so a broken (cyclic) list cannot hang a test.
inline std::vector<int> to_vector(const ListNode* head, std::size_t limit = 100000) {
    std::vector<int> out;
    for (; head != nullptr && out.size() < limit; head = head->next) out.push_back(head->val);
    return out;
}

// 206  Reverse Linked List.  Context: reverse a chain of RLC segments / buffer descriptors in place.
ListNode* reverse_list(ListNode* head);

// 21   Merge Two Sorted Lists.  Context: merge two timestamp-ordered event queues from two radios.
ListNode* merge_two_lists(ListNode* a, ListNode* b);

// 141  Linked List Cycle.  Context: detect a corrupted descriptor ring (a free-list that loops) with O(1) memory.
bool has_cycle(const ListNode* head);

// 19   Remove Nth Node From End.  Context: drop the n-th oldest entry from a singly linked retransmit list in one pass.
// Precondition: 1 <= n <= length. The removed node is unlinked, not freed.
ListNode* remove_nth_from_end(ListNode* head, int n);

}  // namespace lc
