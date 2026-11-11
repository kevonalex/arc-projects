# 04 · Design — Pattern Worksheet

**Pattern:** *compose* two simple structures so each one covers the other's weakness, and state the **cross-structure invariant** that keeps them consistent. Goal: every public operation O(1) (or amortised O(1)).
**Why it matters at a radio/protocol shop:** UE/bearer context caches, rate limiters on alarms and signalling, per-window counters, dedup tables — all "small, hot, bounded-memory" data structures.
**Coded:** 146, 359, 362, 705, 706, 155 — `code/headers/design.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (API, capacity, key/value ranges, time units, thread-safety, what "recent" means) → 2 **Brute force + complexity** → 3 **Optimise** (name the two structures and the invariant tying them) → 4 **Code** → 5 **Hand-trace** normal + edge (eviction, duplicate key, boundary timestamp) → 6 **Final complexity** per operation.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the techniques work

### A1. Hash map + doubly linked list = LRU (146)
* Hash map: key → *handle* to the list node: **O(1) expected** find.
* Doubly linked list ordered by recency: given a handle, unlink and relink at the front in **O(1)**; the back is the LRU victim in O(1).
**Cross-structure invariant I:** `index` and `order` contain **the same set of keys**, `index[k]` points at the list node holding `k`, and the list is sorted most-recent → least-recent. Every operation (get-hit, put-new, put-update, evict) is a constant number of map ops + list splices and restores I, so by induction I always holds and the victim at `order.back()` really is the least recently used.
Why a *list* and not a vector: removing from the middle of a vector is O(n). Why `std::list::splice`: it moves a node without invalidating iterators, so `index` entries stay valid — **iterator stability** is the property being used.

### A2. Time-window counters
Two canonical designs for "events in the last W seconds":
* **Queue of timestamps** — O(1) amortised update, O(#events) memory; evict from the front when `now − t ≥ W`.
* **Circular array of W buckets** indexed by `t mod W` — O(1) update, **O(W) memory independent of traffic**. Each bucket stores `(stamp, count)`; a bucket whose `stamp` is older than `W` belongs to an expired second and is *lazily* reset on write / ignored on read.
Invariant B: for each live second `t ∈ (now−W, now]`, `bucket[t mod W] = (t, #hits at t)` — distinct live seconds map to distinct buckets because they differ by `< W`.

### A3. Rate limiting by "next allowed time"
Store, per message key, the earliest timestamp at which it may print again: `next[m] = lastPrinted[m] + 10`. Decision: print iff `t ≥ next[m]`. **Only a printed message updates `next`** — a suppressed attempt must not extend the quiet period (that would be a leaky "debounce", a different spec). Memory grows with the number of distinct keys: in production add expiry (periodic sweep or a bucketed ring) — say so.

### A4. Hash tables from scratch (705/706)
`B` buckets (prefer a **prime** `B` so poor hash functions of the key still spread), each a short list. `bucket(k) = k mod B`. With load `α = n/B` and a uniform hash, expected cost per op is `1 + α` ⇒ O(1) if you **resize** when `α` exceeds a bound (double `B`, rehash: amortised O(1), same argument as `vector::push_back`). Remove must find *and unlink* the node; `put` on an existing key must **overwrite**, not append a duplicate.

### A5. "Augment the state, not the algorithm" (155)
To answer `getMin` in O(1) under push/pop, store with every element the minimum **of the stack up to and including it**. **Invariant:** `st[i].min = min(st[0..i].value)`. `push` computes it from `st[i−1].min` in O(1); `pop` simply discards the top frame — the previous frame already holds the correct minimum, so nothing needs to be recomputed. (A single `min` variable fails because after popping the minimum you no longer know the second smallest.)

---

## Part B — Problems

### P1 · LC 146 LRU Cache — `LRUCache`
**Ctx:** bounded cache of per-UE context blocks keyed by an RNTI/TEID; evict the coldest.
1. Clarify: `get` on a miss → `-1`; does `get` refresh recency? (yes) does `put` of an existing key refresh? (yes) capacity ≥ 1.
2. Brute: vector of `(key,val)` in recency order, linear search/move → ____ per op
3. Optimise: A1. Write invariant I and say which operation each line of code maintains.
5. Trace: capacity 2: `put1 put2 get1 put3 get2 put4 get1 get3 get4`. Edge: update existing key at capacity.
6. Final: ____

### P2 · LC 359 Logger Rate Limiter — `Logger`
**Ctx:** don't flood the OAM log with the same alarm text more than once per 10 s.
1. Clarify: timestamps non-decreasing seconds; window 10 s; `t == last+10` allowed? Does a suppressed attempt extend the window?
2. Brute: keep a list of all printed `(msg,t)`, scan → ____
3. Optimise: A3. Memory growth issue? How would you bound it?
5. Trace: `(1,foo)(2,bar)(3,foo)(8,bar)(10,foo)(11,foo)`
6. Final: ____

### P3 · LC 362 Design Hit Counter — `HitCounter`
**Ctx:** packets (or RACH attempts) seen in the last 5 minutes, O(1) memory under any load.
1. Clarify: window `(t−300, t]`; timestamps non-decreasing; many hits in the same second.
2. Brute: store every hit, filter on read → ____
3. Optimise: A2 — design both the queue version and the 300-bucket version; state Invariant B; decide which is better under a flood.
5. Trace: `hit1 hit2 hit3 get(4) hit300 get(300) get(301)`; bucket-reuse edge: `hit5 ×3, hit305, get305`.
6. Final: ____

### P4 · LC 705 Design HashSet — `MyHashSet`
**Ctx:** duplicate-detection table of received sequence numbers.
1. Clarify: keys `0..10⁶`; `add` idempotent; `remove` of absent key is a no-op.
2. Brute: bitmap of 10⁶+1 bits (valid! trade space for speed) vs vector scan → ____
3. Optimise: A4 with 769 buckets. Note when the bitmap is *better*.
5. Trace: `add0 add769 remove0 contains769` (collision), `remove99`
6. Final: ____

### P5 · LC 706 Design HashMap — `MyHashMap`
**Ctx:** session-id → session state.
1. Clarify: `get` miss → `-1`; `put` overwrites.
2. Brute: array of 10⁶+1 slots (sentinel `-1`) → ____
3. Optimise: chaining; why must `put` scan the chain before appending?
5. Trace: keys `5, 5+769, 5+2·769` in one chain; remove the middle.
6. Final: ____

### P6 · LC 155 Min Stack — `MinStack`
**Ctx:** track the minimum RTT across an undo-able stack of measurements.
1. Clarify: all ops O(1); `pop/top/getMin` only on non-empty.
2. Brute: scan the stack for min → ____
3. Optimise: A5. Show why a single `min` variable breaks.
5. Trace: `push −2, push 0, push −3, getMin, pop, top, getMin`; edge: duplicate minima `push 1, push 1, pop, getMin`.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "An LRU cache can be a `vector` — it's small anyway."
2. "A `get` that hits does not change eviction order."
3. "`std::map` gives O(1) lookups."
4. "A suppressed log attempt should reset the 10-second timer."
5. "For hit counters, a queue of timestamps always beats buckets."
6. "A hash table with `B` buckets is O(1) no matter how many keys you insert."
7. "Min Stack can keep a single `min` variable and recompute on pop."

## Part D — Self-check
* Write invariant I for LRU and name the operations that must maintain it.
* Why does `splice` not invalidate the iterators stored in the map?
* In the bucket hit counter, how do you know a bucket is stale?
* What is the memory cost of the bucketed counter if traffic is 10⁶ hits/s? Of the queue version?
