# 04 · Design — Answers & Write-ups

Reference code: `code/solutions/design_solutions.cpp`; tests `code/tests/test_design.cpp` (13 tests, ASan+UBSan clean).

---

## P1 · 146 LRU Cache

1. **Clarify.** `get(k)`: value or `-1`, a hit makes `k` most-recently-used (MRU). `put(k,v)`: insert or update (update also refreshes), evicting the LRU key if size would exceed capacity. Capacity ≥ 1 (guard 0 defensively). Single-threaded.
2. **Brute.** Vector of pairs kept in recency order: lookup O(n), move-to-front O(n) ⇒ O(n) per op. (A hash map alone can't find the LRU victim without an O(n) scan.)
3. **Optimise.** `unordered_map<int, list::iterator>` + `list<pair<int,int>>`, front = MRU. Invariant I (worksheet A1). Each op:
   * `get` hit: `splice(begin, list, it)` — O(1), iterator remains valid.
   * `put` update: set value, splice to front.
   * `put` new at capacity: erase `index[order.back().key]`, `pop_back`, then `emplace_front`, `index[k] = begin()`.
   Order of the eviction steps matters: read the victim's key **before** `pop_back`.
4. **Code.** see solution.
5. **Trace, capacity 2** (list shown MRU→LRU):
```
put1        [1]
put2        [2,1]
get1 ->1    [1,2]         (hit refreshes)
put3        full -> evict back (2): [3,1]
get2 -> -1  miss
put4        full -> evict back (1): [4,3]
get1 -> -1
get3 -> 3   [3,4]
get4 -> 4   [4,3]
```
   Edge: `put1 put2 put(1,10) put3`: update makes `1` MRU `[1,2]`, so `put3` evicts `2`, and `get1 = 10` ✓ (a bug that forgets to refresh on update evicts `1`). Edge capacity 1: every put of a new key evicts the previous.
6. **Final.** O(1) expected per op, O(capacity) space.

## P2 · 359 Logger Rate Limiter

1. **Clarify.** Timestamps non-decreasing (seconds). A message may print at `t` iff it has never printed or `t ≥ lastPrinted + 10`. Printing at exactly `+10` is allowed (example: `foo@1` prints, `foo@10` blocked, `foo@11` prints). A **suppressed** attempt changes nothing.
2. **Brute.** Log of all printed `(msg, t)`; scan it per call — O(#printed).
3. **Optimise.** `unordered_map<string,int> next_allowed`. `if (it != end && t < it->second) return false; next_allowed[m] = t + 10; return true;`. Invariant: `next_allowed[m] = lastPrinted(m) + 10`.
   Memory is O(#distinct messages) forever; bound it by a periodic sweep deleting entries with `next ≤ now`, or by a ring of 10 buckets of sets keyed by `t mod 10` (each bucket holds messages printed at that second).
4. **Code.** see solution.
5. **Trace:**
```
(1,foo)  none        -> print, next[foo]=11   true
(2,bar)  none        -> print, next[bar]=12   true
(3,foo)  3 < 11      -> suppress              false
(8,bar)  8 < 12      -> suppress              false
(10,foo) 10 < 11     -> suppress              false
(11,foo) 11 >= 11    -> print, next[foo]=21   true
```
   Edge: `(0,x)` true, `(9,x)` false, `(10,x)` **true** — if the 9 had extended the window, the 10 would wrongly be blocked.
6. **Final.** O(1) expected per call, O(#distinct keys) space (unbounded without sweep).

## P3 · 362 Design Hit Counter

1. **Clarify.** `getHits(t)` counts hits with `t' ∈ (t−300, t]`, i.e. `t − t' < 300`. Calls come in non-decreasing time. Many hits per second possible.
2. **Brute.** Store every hit in a vector; count by scanning → O(#hits) per query and unbounded memory.
3. **Optimise.**
   * *Queue version:* `deque<int>` of timestamps; on `getHits(t)` pop front while `t − front ≥ 300`; answer = size. Amortised O(1) but memory = hits in window (1 M/s ⇒ 300 M entries).
   * *Bucket version (coded):* arrays `stamp[300]`, `count[300]`. `hit(t)`: `i = t % 300`; if `stamp[i] != t` the bucket held an expired second — reset `stamp[i]=t, count[i]=0`; then `++count[i]`. `getHits(t)`: sum `count[i]` over buckets with `t − stamp[i] < 300`. Invariant B (worksheet A2). Correctness: two live seconds `t₁ ≠ t₂ ∈ (now−300, now]` differ by `< 300`, so `t₁ mod 300 ≠ t₂ mod 300`; a bucket's stored stamp tells us which second it represents, so stale data is never counted.
   Cost: `hit` O(1); `getHits` O(300) = O(1) *independent of traffic*; memory 2·300 ints.
4. **Code.** `stamp` initial `-1` (never matches a real time).
5. **Trace:** `hit1,hit2,hit3`: buckets 1,2,3 = (1,1),(2,1),(3,1). `getHits(4)`: all `4−stamp<300` → 3 ✓. `hit(300)`: `300%300 = 0` → bucket0=(300,1). `getHits(300)`: stamps 1,2,3,300 → ages 299,298,297,0 all <300 → **4** ✓. `getHits(301)`: ages 300,299,298,1 → the t=1 hit has age 300 (not <300) → **3** ✓. Edge reuse: `hit5 ×3` bucket5=(5,3); `hit305`: `305%300=5`, stamp 5≠305 → reset → (305,1); `getHits(305)=1` ✓; `getHits(1000)` → all ages ≥ 300 → 0.
6. **Final.** `hit` O(1), `getHits` O(W) with W=300 constant; space O(W).

## P4 · 705 Design HashSet

1. **Clarify.** Keys `0..10⁶`; ops `add/remove/contains`. Idempotent add; remove-missing is a no-op.
2. **Brute.** `vector<bool>(10⁶+1)`: O(1) everything, 125 KB — *this is a legitimately good answer for a dense bounded key space*; mention it. A vector scan is O(n).
3. **Optimise.** Chaining: 769 buckets (prime), `bucket = key % 769`. `add` checks membership first; `remove` unlinks. Expected cost `1 + α`, with `α = n/769`; for general key ranges add rehashing when `α > 1` (amortised O(1)).
4. **Code.** `vector<list<int>>`, `std::find` / `list::remove`.
5. **Trace:** `add 0`, `add 769` (both → bucket 0, chain `[0,769]`); `remove 0` → chain `[769]`; `contains 0` false, `contains 769` true ✓. `remove 99` no-op ✓.
6. **Final.** O(1 + n/B) expected per op; O(B + n) space.

## P5 · 706 Design HashMap

1. **Clarify.** `put` overwrites; `get` miss → `-1`; `remove` of absent key no-op.
2. **Brute.** `int[10⁶+1]` with sentinel `-1`: O(1)/4 MB.
3. **Optimise.** Same table with `(key,value)` nodes. `put` **scans the chain first** — appending blindly creates duplicate keys and `get` would see a stale value.
4. **Code.** see solution.
5. **Trace:** `put5→50`, `put774→60`, `put1543→70` all in bucket 5 (chain `[5,774,1543]`). `remove 774` → `[5,1543]`. `get 5 = 50`, `get 774 = −1`, `get 1543 = 70` ✓.
6. **Final.** O(1 + n/B) expected; O(B + n) space.

## P6 · 155 Min Stack

1. **Clarify.** All ops O(1).
2. **Brute.** `getMin` scans O(n).
3. **Optimise.** Frame = `(value, minSoFar)`; `push` computes `min(val, top.minSoFar)`; `pop` discards the frame (A5 invariant). Alternative: a second stack of minima that pushes only when `val ≤ currentMin` (the `≤` handles duplicates).
4. **Code.** `vector<pair<int,int>>`.
5. **Trace:**
```
push -2  frames [(-2,-2)]
push  0  [(-2,-2),(0,-2)]
push -3  [(-2,-2),(0,-2),(-3,-3)]   getMin -> -3
pop      [(-2,-2),(0,-2)]
top -> 0   getMin -> -2  ✓
```
   Edge duplicates: `push1 push1 pop` → `[(1,1)]`, `getMin = 1`. A single-`min` variable that resets to "unknown" on pop of the min fails; a second-stack version using `<` instead of `≤` would pop the shared minimum too early.
6. **Final.** O(1) per op, O(n) space.

---

## Part C answers
1. **False for performance:** O(n) per op; it is "fine" only for tiny capacities (and good to say so).
2. **False.** A hit refreshes recency.
3. **False.** `std::map` is O(log n); `std::unordered_map` is O(1) expected.
4. **False** under the 359 spec — only printed messages update the window.
5. **False.** The queue's memory scales with traffic; buckets cap at W.
6. **False.** Expected O(1 + n/B); without rehash the chain length grows linearly.
7. **False.** After popping the min you need the previous min: store it per frame.

## Part D answers
* I: same key set in `index` and `order`; `index[k]` → node of `k`; list sorted MRU→LRU. Maintained by get-hit (splice), put-update (set+splice), put-new (insert in both), eviction (erase in both).
* `std::list::splice` relinks nodes without copying or reallocating them; iterators/pointers to the node stay valid.
* `stamp[i] != t` on write (reset) and `now − stamp[i] ≥ W` on read.
* Bucketed: 2·300 ints ≈ 2.4 KB regardless. Queue: ~3·10⁸ timestamps ≈ 1.2 GB.
