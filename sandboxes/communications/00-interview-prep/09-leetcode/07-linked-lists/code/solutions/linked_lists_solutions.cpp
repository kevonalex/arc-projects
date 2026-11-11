#include "linked_lists.hpp"

namespace lc {

// 206 -- invariant: `prev` is the reversed prefix, `cur` is the untouched suffix.
ListNode* reverse_list(ListNode* head) {
    ListNode* prev = nullptr;
    ListNode* cur = head;
    while (cur != nullptr) {
        ListNode* next = cur->next;  // save before overwriting
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    return prev;
}

// 21 -- dummy head removes the "which node is first?" special case. `<=` keeps the merge stable.
ListNode* merge_two_lists(ListNode* a, ListNode* b) {
    ListNode dummy;
    ListNode* tail = &dummy;
    while (a != nullptr && b != nullptr) {
        if (a->val <= b->val) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = (a != nullptr) ? a : b;  // splice the leftover run in O(1)
    return dummy.next;
}

// 141 -- Floyd. Test `fast && fast->next` BEFORE advancing two steps.
bool has_cycle(const ListNode* head) {
    const ListNode* slow = head;
    const ListNode* fast = head;
    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}

// 19 -- two pointers n+1 apart; when `lead` falls off the end, `lag` sits just before the target.
ListNode* remove_nth_from_end(ListNode* head, int n) {
    ListNode dummy(0, head);
    ListNode* lead = &dummy;
    ListNode* lag = &dummy;
    for (int i = 0; i < n; ++i) lead = lead->next;  // gap of n
    while (lead->next != nullptr) {
        lead = lead->next;
        lag = lag->next;
    }
    lag->next = lag->next->next;  // unlink (no delete: caller owns nodes)
    return dummy.next;
}

}  // namespace lc
