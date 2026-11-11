# 02 · Parsing — Cheatsheet

**Core idea:** one forward pass, an explicit state, constant-size table; be **total** (never read `s[n]`) and **canonical**.

## Skeleton
```cpp
size_t i = 0, n = s.size();
while (i < n && s[i] == ' ') ++i;                 // phase 1
if (i < n && (s[i]=='+'||s[i]=='-')) { ... ++i; } // phase 2
while (i < n && is_digit(s[i])) { ... ++i; }      // phase 3
// decide: consume everything? (validators) or stop here? (atoi)
```

## Safe numeric append
`if (acc > (INT_MAX - d) / 10) saturate; acc = acc*10 + d;`

## DFA recipe
1. Write the grammar. 2. List "what have I seen" → states. 3. Alphabet → classes. 4. Fill `table[state][class]`. 5. Mark accepting = "just finished a required element". 6. `dead = -1`, `other → dead`.

## Valid Number (65) table (S2,S3,S5,S8 accepting)
`S0:d→2 s→1 .→4 | S1:d→2 .→4 | S2:d→2 .→3 e→6 | S3:d→5 e→6 | S4:d→5 | S5:d→5 e→6 | S6:d→8 s→7 | S7:d→8 | S8:d→8`

## Recipes
| LC | Move | Time / Space |
|----|------|--------------|
| 8 | ws → sign → digits, pre-overflow check | O(n) / O(1) |
| 468 | split **keeping empties**; v4: 4×(1–3 digits, ≤255, no leading 0); v6: 8×(1–4 hex) | O(n) / O(n) |
| 271 | `len#payload`; decode: read digits to `#`, take `len` bytes | O(total) / O(total) |
| 443 | read=run scan, write ≤ read; `to_string(run)` if run>1 | O(n) / O(1) |
| 65 | 9-state DFA | O(n) / O(1) |

## Gotchas
* `stoi`/`atoi`/`strtod` accept more than your grammar.
* Splitters that drop empty tokens hide errors.
* Always bound lengths from untrusted length fields before allocating.
* Leading zeros: IPv4 forbids, `atoi` allows.
* Use `size_t` for indices but be careful with `i - 1` when `i == 0`.
* `"+"`, `"-"`, `"."`, `"e"` alone are all invalid numbers.

## Script
"Clarify grammar & invalid cases → regex/`stoi` brute force is unsafe/unprovable → I'll write the automaton → code table → trace a valid string and one that dies → O(n)/O(1)."
