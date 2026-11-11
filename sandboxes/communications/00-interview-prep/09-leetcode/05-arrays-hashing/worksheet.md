# 05 · Arrays & Hashing — Pattern Worksheet

**Pattern:** replace a nested search with a **lookup** (hash table) or a **precomputed prefix/suffix**; or, when memory is tight, use the **array itself as the hash table**.
**Why it matters at a radio/protocol shop:** matching acks to sends, grouping frames by a canonical signature, finding the first free handle/RNTI, spotting gaps in a sequence-number stream.
**Coded:** 1, 49, 238, 128, 41 — `code/headers/arrays_hashing.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem
1 **Clarify** (duplicates? negatives? value range vs `n`? may I mutate input? exactly one answer?) → 2 **Brute force + complexity** → 3 **Optimise** (what do I look up, what is the key, what is the invariant?) → 4 **Code** → 5 **Hand-trace** a normal and an edge input → 6 **Final complexity** (expected vs worst case for hashing).

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

---

## Part A — Why the techniques work

### A1. "Complement lookup" — turning a pair search into a membership test
Want `i < j` with `aᵢ + aⱼ = T`. For fixed `j`, the partner must equal `T − aⱼ`: a single value we can *ask for*. Scan left→right with a map `value → index` of everything seen so far.
**Invariant:** before processing `j`, the map holds every `a_i` with `i < j`. **Completeness:** if a solution `(i, j)` exists with `i < j`, then when we reach `j` the map contains `aᵢ = T − aⱼ` ⇒ found. **Soundness:** we look up *before* inserting `aⱼ`, so an element is never paired with itself. Expected O(1) per lookup ⇒ **O(n) expected time**, O(n) space. (Worst case O(n²) with adversarial hashes; mention it.)
Overflow: `T − aⱼ` may exceed `int` — compute in `long long` and range-check.

### A2. Canonical forms — hashing equivalence classes
Two items are "equivalent" iff they map to the same **canonical representative**; then `hash(canonical)` is a perfect grouping key (49: anagrams ⇔ equal letter-count vectors ⇔ equal sorted strings). The key must be a *function of the class only* and *injective on classes*. Count-vector key: O(L + Σ) per word versus sorted-string O(L log L). A serialised count key needs a **separator** (`"1,11"` vs `"11,1"` would collide without one).

### A3. Prefix/suffix decomposition (238)
`out[i] = (∏_{j<i} aⱼ) · (∏_{j>i} aⱼ) = P[i] · S[i]`. Compute `P` in one forward pass and fold `S` in with one backward pass using a running variable — **no division** (zeros make division wrong), O(n) time, O(1) extra space beyond the output. The same decomposition solves "all-but-one" sums, max, gcd (any associative op).

### A4. Union by "run starts" (128)
Put all numbers in a hash set. An element `x` is the **start of a run** iff `x−1 ∉ S`. From each start, extend while `x+1 ∈ S`. **Why O(n):** every element belongs to exactly one maximal run; the inner `while` executes once per element of *that run only when we are at its start*; so total inner iterations ≤ n. (Without the start test every element would re-walk its whole suffix: O(n²).)

### A5. The array as its own hash table (41)
Pigeonhole: among `n` slots, the first missing positive is in `[1, n+1]`. Values `≤ 0` or `> n` can't matter. Define the **home** of value `v ∈ [1,n]` as index `v−1`. Cyclic-sort: while `nums[i]` is in range and not already at home (and its home doesn't already contain it — duplicates!), swap it home. **Termination/cost:** each swap puts one value permanently at its home, so ≤ n swaps in total ⇒ O(n). Afterwards the first index `i` with `nums[i] ≠ i+1` gives the answer `i+1`; if none, `n+1`.

---

## Part B — Problems

### P1 · LC 1 Two Sum — `two_sum`
**Ctx:** find two PDU sizes that exactly fill a transport block of size `T`.
1. Clarify: exactly one solution? same element twice not allowed; duplicates (`[3,3]`, T=6); negatives; return indices.
2. Brute: all pairs → ____
3. Optimise: A1 (invariant + soundness + completeness). What about the sorted + two-pointers variant — when is it better?
5. Trace: `[2,7,11,15], 9` and `[3,3], 6`; edge target−x overflow with `INT_MAX/INT_MIN`.
6. Final: ____

### P2 · LC 49 Group Anagrams — `group_anagrams`
**Ctx:** bucket received frames that carry the same multiset of symbols (order scrambled by interleaving).
1. Clarify: lowercase only? order of groups free? duplicates stay together; empty string is its own group.
2. Brute: compare every pair with sorted-equality → ____
3. Optimise: A2. Choose between sorted key and count key; justify.
5. Trace: `["eat","tea","tan","ate","nat","bat"]`; edge `[""]`.
6. Final: ____ in terms of `n` strings of max length `L`.

### P3 · LC 238 Product of Array Except Self — `product_except_self`
**Ctx:** leave-one-out gain products across `n` carriers without division (zeros are legal).
1. Clarify: no division allowed; 32-bit products fit; zeros present; O(1) extra space excluding output?
2. Brute: nested loops → ____
3. Optimise: A3. Why does the division trick fail with one zero? with two zeros?
5. Trace: `[1,2,3,4]` and `[-1,1,0,-3,3]`.
6. Final: ____

### P4 · LC 128 Longest Consecutive Sequence — `longest_consecutive`
**Ctx:** longest unbroken run of sequence numbers in an unordered batch of received packets.
1. Clarify: duplicates; unsorted; must be O(n) (so no sort); negatives; values near `INT_MAX`.
2. Brute: sort + scan → ____ ; or for each x walk x+1… → ____
3. Optimise: A4. Prove the O(n) bound.
5. Trace: `[100,4,200,1,3,2]`; edge `[1,1,1]`, empty.
6. Final: ____

### P5 · LC 41 First Missing Positive — `first_missing_positive`
**Ctx:** smallest free positive handle in a table of in-use handles, O(1) extra memory.
1. Clarify: unsorted; duplicates, negatives, huge values; may I mutate? (yes)
2. Brute: hash-set + count up → ____ ; sort → ____
3. Optimise: A5. Why `while` not `if`? Why the "target slot already holds it" check?
5. Trace: `[3,4,-1,1]` (show array after each swap), `[1,1]`.
6. Final: ____

---

## Part C — Misconceptions (T/F)
1. "`unordered_map` lookups are always O(1)."
2. "For Two Sum I should insert first, then look up."
3. "Group Anagrams must sort every string."
4. "Product-except-self is easiest with total product ÷ `nums[i]`."
5. "Longest Consecutive Sequence is O(n²) because of the nested loop."
6. "First Missing Positive needs a hash set for O(n)."
7. "Swapping inside a `for` loop with `if` is enough for cyclic sort."

## Part D — Self-check
* What is the key of the hash table in each problem?
* What invariant does the map hold in Two Sum after each step?
* Give an input where "insert-before-lookup" breaks Two Sum.
* Prove cyclic sort does ≤ n swaps.
