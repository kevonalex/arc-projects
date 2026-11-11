# 05 · Arrays & Hashing — Cheatsheet

**Core idea:** ask "what exact value am I looking for?" → hash lookup; "what do all items in a class share?" → canonical key; "result depends on everything except one" → prefix × suffix; "O(1) space" → the array is the table.

| LC | Move | Key | Time / Space |
|----|------|-----|--------------|
| 1 | one pass: lookup `T-a[i]` **then** insert `a[i]` | value → index | O(n) / O(n) |
| 49 | group by canonical key | 26-count string (with separators) or sorted string | O(nL) / O(nL) |
| 238 | `out[i]=prefix; prefix*=a[i]` then right→left with `suffix` | – | O(n) / O(1) |
| 128 | set; start at `x` iff `x-1∉S`; extend while `x+1∈S` | value | O(n) / O(n) |
| 41 | cyclic place `v` at index `v-1`; first `i` with `a[i]≠i+1` | index | O(n) / O(1) |

## Templates
```cpp
// Two Sum (lookup before insert)
unordered_map<int,int> seen;
for i: { auto it = seen.find(T - a[i]); if (it != end) return {it->second, i}; seen[a[i]] = i; }

// Cyclic sort for 41
for i: while (1 <= a[i] && a[i] <= n && a[a[i]-1] != a[i]) swap(a[i], a[a[i]-1]);
```

## Complexity reminders
* `unordered_map`: expected O(1), worst O(n). `map`: O(log n).
* Sorting alternative: O(n log n), no extra hashing risk.
* Answer for "first missing positive" ∈ `[1, n+1]` (pigeonhole).

## Gotchas
* Two Sum: lookup before insert; use 64-bit for `T - a[i]`.
* Group anagrams: separator in the count key.
* 238: zeros; don't divide.
* 128: iterate the *set*, skip non-starts; watch `INT_MIN-1` / `INT_MAX+1`.
* 41: `while` + "already at home" guard; duplicates; values > n ignored.
* Mutating the input — say so and ask if allowed.

## Script
"Clarify duplicates/overflow/mutation → brute force O(n²) → which lookup removes the inner loop? → state the invariant of the table → code → trace normal + duplicate/zero edge → O(n) expected."
