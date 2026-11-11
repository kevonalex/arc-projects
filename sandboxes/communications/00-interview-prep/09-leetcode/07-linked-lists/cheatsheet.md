# 07 · Linked Lists — Cheatsheet

**Core idea:** pointer surgery with an invariant over named pointers + a dummy head; never lose the rest of the list.

## Templates
```cpp
// reverse
ListNode *prev=nullptr, *cur=head;
while (cur) { auto nx=cur->next; cur->next=prev; prev=cur; cur=nx; }
return prev;

// merge (dummy head)
ListNode d; auto* t=&d;
while (a && b) { if (a->val <= b->val) { t->next=a; a=a->next; } else { t->next=b; b=b->next; } t=t->next; }
t->next = a ? a : b; return d.next;

// Floyd
auto *s=head,*f=head;
while (f && f->next) { s=s->next; f=f->next->next; if (s==f) return true; }

// nth from end
ListNode d(0,head); auto *lead=&d,*lag=&d;
for(i<n) lead=lead->next;
while (lead->next) { lead=lead->next; lag=lag->next; }
lag->next = lag->next->next;
```

## Facts
* Floyd meets within `μ+λ` steps: first `t ≥ μ` with `λ | t`, i.e. `t = λ⌈μ/λ⌉`. Gap changes by exactly 1 per step ⇒ can't skip 0. Fast must be exactly 2× (difference 1).
* Cycle start (142): after meeting, reset one pointer to head; step both by 1; they meet at the entry.
* Middle node: slow/fast, slow is at the middle when fast ends.
* Dummy head ⇒ no special case for first node.

| LC | Time / Space |
|----|--------------|
| 206 | O(n) / O(1) |
| 21 | O(n+m) / O(1) |
| 141 | O(n) / O(1) |
| 19 | O(n) one pass / O(1) |

## Gotchas
* Save `next` before overwriting.
* Test `fast` **then** `fast->next`.
* Ownership: who frees removed nodes? (here: arena / caller)
* `<=` for stable merge; splice leftovers.
* Don't compare values for cycles — compare pointers.
* Recursion on 10⁵ nodes overflows the stack.
* Intrusive lists in RT code: prefer sentinel nodes, `std::list`/`splice` where allowed.

## Script
"Clarify null/one-node/ownership → brute force copies into a vector (O(n) space) → relink in place; invariant: ___ → code with dummy → trace `1→2→3` and `∅` → O(n)/O(1)."
