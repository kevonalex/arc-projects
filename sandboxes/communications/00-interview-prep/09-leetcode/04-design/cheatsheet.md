# 04 · Design — Cheatsheet

**Core idea:** compose structures; name the invariant that ties them; keep every op O(1).

| LC | Structures | Key invariant / trick | Cost |
|----|-----------|-----------------------|------|
| 146 LRU | `unordered_map<key, list::iterator>` + `list<(k,v)>` | same key set; list MRU→LRU; `splice(begin, list, it)` | O(1) |
| 359 Logger | `unordered_map<msg, nextAllowed>` | print iff `t >= next`; only prints update `next = t+10` | O(1) |
| 362 Hits | 300 buckets `(stamp,count)` or deque of times | bucket `t%300` valid iff `stamp==t`; window `t-stamp < 300` | O(1) hit / O(W) read |
| 705 Set | `vector<list<int>>` B≈769 prime | `k % B`; add checks dup | O(1+α) |
| 706 Map | `vector<list<pair>>` | `put` scans chain to overwrite | O(1+α) |
| 155 MinStack | stack of `(val, minSoFar)` | pop = discard frame | O(1) |

## LRU template
```cpp
get:  find -> splice to front -> return val
put:  if found: set, splice; else { if full: index.erase(order.back().first), order.pop_back();
                                    order.emplace_front(k,v); index[k]=order.begin(); }
```

## Window-counter choices
* queue of timestamps: simple, memory ∝ traffic.
* ring of W buckets: memory ∝ W, constant read cost.
* decaying counter / token bucket: O(1) state, approximate.

## Hash table facts
Prime bucket count; load factor α = n/B; resize at α>1 (double, rehash, amortised O(1)); chain vs open addressing trade-offs; dense small key space ⇒ plain array/bitmap wins.

## Gotchas
* LRU: refresh on **update**; read victim key **before** `pop_back`.
* Logger: suppressed call must not move the window; `t == last+10` is allowed.
* Hit counter: strict `<` vs `<=` on the 300 boundary — verify against the example (`301` drops the hit at `1`).
* Hash: negative keys with `%` (cast / `unsigned`), duplicate keys on `put`.
* MinStack: duplicates of the minimum.
* Thread-safety: say "single-threaded; I'd add a mutex / shard by key".

## Script
"Clarify API + eviction rules → brute force scans O(n) → pair a map (lookup) with a list/array (order/expiry) → state invariant → code → trace eviction & boundary → O(1) per op, O(capacity) space."
