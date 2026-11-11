# 02 · Parsing — Answers & Write-ups

Reference code: `code/solutions/parsing_solutions.cpp`; tests `code/tests/test_parsing.cpp` (12 tests, ASan+UBSan clean).

---

## P1 · 8 String to Integer

1. **Clarify.** Skip leading `' '` only; optional single `+`/`-`; read digits until non-digit; no digits → 0; clamp to 32-bit range.
2. **Brute.** Trim, accumulate in `long long`, clamp at the end. O(n) but a 100-digit input overflows `long long` too (UB) — you must clamp *during* accumulation.
3. **Optimise.** Three phases, single left-to-right pointer (A3). Saturate with `acc > (INT_MAX - d) / 10` (A4). Proof of equivalence: integers `acc, d` with `0 ≤ d ≤ 9`: `10·acc + d ≤ M ⟺ 10·acc ≤ M − d ⟺ acc ≤ ⌊(M−d)/10⌋`. For negatives we accumulate the **magnitude** (≤ `INT_MAX`); if the true magnitude is 2147483648 we return `INT_MIN`, which is the correct clamp *and* the correct value, so the asymmetry costs nothing.
4. **Code.** `my_atoi` in solutions.
5. **Trace.**
   * `"   -42"`: skip 3 spaces; sign `-` → sign=-1; digit 4 → acc=4; digit 2 → 42; end → `-42` ✓.
   * `"2147483648"`: acc after `214748364`; next `d=8`: `(2147483647−8)/10 = 214748363`; `214748364 > 214748363` → return `INT_MAX` ✓.
   * `"-"`: sign consumed, no digits → `sign*0 = 0` ✓. `""` → 0 ✓. `"+-12"`: sign `+`, then `-` is not a digit → 0 ✓.
6. **Final.** O(n) time (each char once), O(1) space.

## P2 · 468 Validate IP Address

1. **Clarify.** IPv4: exactly 4 decimal fields, each 1–3 digits, value ≤ 255, no leading zero unless the field is `"0"`. IPv6: exactly 8 hex fields of 1–4 digits (case-insensitive), no `::`. Anything else → `"Neither"`.
2. **Brute.** A regex per family: works, but is the classic bug farm (anchors, `{1,3}` allowing `256`, leading zeros) and hides worst-case behaviour; avoid in a parser you must reason about.
3. **Optimise.** Decide family by delimiter (`.` ⇒ try v4; `:` ⇒ try v6; neither ⇒ Neither; both present ⇒ validation fails naturally, since a `:` is not a digit and a `.` is not hex). Split **keeping empty tokens** so `"1.1.1."` → `{"1","1","1",""}` ⇒ 4 parts but last is empty ⇒ rejected (and `"1.1.1.1."` → 5 parts). Validate each token with explicit character checks — no `stoi` (accepts `+`, spaces, trailing junk).
4. **Code.** `split_keep_empty`, `valid_v4`, `valid_v6`.
5. **Trace.**
   * `"172.16.254.1"` → `{172,16,254,1}`: lengths 3,2,3,1 ok; no leading zero; values ≤255 → IPv4 ✓.
   * `"192.168.01.1"` → token `"01"`: size>1 and starts with `'0'` → Neither ✓.
   * `"1.1.1.1."` → 5 tokens → Neither ✓.
   * `"2001:0db8:85a3::8A2E:037j:7334"` → 7 tokens (an empty one for `::`) → Neither ✓ (also `j` not hex).
6. **Final.** O(n) time, O(n) space for tokens (could be O(1) with an index scan).

## P3 · 271 Encode and Decode Strings

1. **Clarify.** Payload is arbitrary; empty strings and empty list must round-trip; the decoder gets only the encoded string.
2. **Brute.** Join with `,` — ambiguous if a payload contains `,` or if `{"" , ""}` vs `{","}` collide. Escaping (`\,`) works but each payload byte may expand and the decoder needs a state machine.
3. **Optimise.** `<len>#<payload>` (A5). *Invariant of decode:* at the top of each iteration `i` points at the start of a record. Digits cannot contain `#`, so the first `#` ends the length field unambiguously; then exactly `len` bytes belong to the payload no matter what they are; advance `i` by `len`. Correct by induction on the number of records; injective because the decoder is a left inverse.
4. **Code.** `std::to_string(size) + '#' + payload`.
5. **Trace.** `{"ab",""}` → `"2#ab"` + `"0#"` = `"2#ab0#"`. Decode: read `2`, `#`, take `ab`; i=4; read `0`, `#`, take `""`; i=6 = size → done → `{"ab",""}` ✓. Edge `{"3#abc"}` → `"5#3#abc"`: read `5`, take 5 bytes `3#abc` ✓ — the inner `#` is harmless. Edge `{}` → `""` → `{}` ✓ (note `{""}` → `"0#"` is distinct).
6. **Final.** O(total length) time, O(total length) space for the output.

*Production note:* real stacks use fixed-width binary lengths (u16/u32 big-endian) plus a **maximum length** check to bound allocation from untrusted data. Decimal here keeps it readable; add the cap in any real parser.

## P4 · 443 String Compression

1. **Clarify.** Compress `chars` in place: for each maximal run write the char, then the decimal count *only if > 1*; counts ≥ 10 become several chars (`"12"` → `'1','2'`). Return the new length; extra space O(1).
2. **Brute.** Build a `std::string` result, copy back — O(n) time but O(n) extra space (violates the constraint).
3. **Optimise.** `read` scans runs; `write` emits. **Why `write ≤ read` always:** a run of length `r` is replaced by `1` char if `r = 1`, else `1 + digits(r)` chars. `r=1: 1 ≤ 1`; `r=2: 2 ≤ 2`; `r=9: 2 ≤ 9`; `r=10: 3 ≤ 10`; in general `1 + ⌊log₁₀ r⌋ + 1 ≤ r` for `r ≥ 2`. So each run's output fits within the input space it just consumed; the write pointer never reaches unread data.
4. **Code.** two nested loops: outer per run, inner to measure the run.
5. **Trace `a a b b c c c`:**
```
read=0 c='a' run=2 -> write 'a','2'       write=2
read=2 c='b' run=2 -> write 'b','2'       write=4
read=4 c='c' run=3 -> write 'c','3'       write=6
return 6, chars[0..6) = "a2b2c3"  ✓
```
   Edge: `a` + `b`×12 → `a` (run 1: no count) then `b`,`1`,`2` → `"ab12"` length 4 ✓. Edge: 100 × `z` → `"z100"` ✓. Edge: empty → 0.
6. **Final.** O(n) time, O(1) extra space.

## P5 · 65 Valid Number — the DFA

1. **Clarify.** Grammar (from LeetCode): decimal (`sign? (digits '.'? | digits '.' digits | '.' digits)`) or integer, optionally followed by `(e|E) sign? digits`. So `"4."` ✓, `".5"` ✓, `"."` ✗, `"e9"` ✗, `"1e"` ✗, `"1e+"` ✗, `"1 "` ✗ (no whitespace).
2. **Brute.** Nested `if`s/regex; easy to miss `"+.8"` or `"4e+"`; hard to prove.
3. **Optimise — the automaton.**

| State | Meaning | digit | sign | `.` | `e/E` | other |
|-------|---------|-------|------|-----|-------|-------|
| S0 | start | S2 | S1 | S4 | ✗ | ✗ |
| S1 | after leading sign | S2 | ✗ | S4 | ✗ | ✗ |
| S2 ✔ | integer digits | S2 | ✗ | S3 | S6 | ✗ |
| S3 ✔ | `digits.` | S5 | ✗ | ✗ | S6 | ✗ |
| S4 | `.` with no digits yet | S5 | ✗ | ✗ | ✗ | ✗ |
| S5 ✔ | fraction digits | S5 | ✗ | ✗ | S6 | ✗ |
| S6 | after `e` | S8 | S7 | ✗ | ✗ | ✗ |
| S7 | exponent sign | S8 | ✗ | ✗ | ✗ | ✗ |
| S8 ✔ | exponent digits | S8 | ✗ | ✗ | ✗ | ✗ |

   Accepting = {S2, S3, S5, S8}: exactly the states reachable after the last *required* element (a digit) of the mantissa or exponent. Non-accepting S4 enforces "`.` alone is invalid"; S6/S7 enforce "an exponent needs digits". Verification: each ✗ cell is a grammar violation (second dot, second sign, `e` without prior digits, dot after `e`).
4. **Code.** `kTable[9][5]`, `-1` = dead.
5. **Trace.**
   * `"-.9"`: S0 –`-`→ S1 –`.`→ S4 –`9`→ S5 ✔ → valid ✓.
   * `"53.5e93"`: S0 –5→ S2 –3→ S2 –`.`→ S3 –5→ S5 –e→ S6 –9→ S8 –3→ S8 ✔ ✓.
   * `"99e2.5"`: …S2 –e→ S6 –2→ S8 –`.`→ ✗ dead → invalid ✓.
   * `"."`: S0 –`.`→ S4 (not accepting) → invalid ✓.
6. **Final.** O(n) time, O(1) space.

---

## Part C answers

1. **False.** `atoi` has UB on overflow, returns 0 for garbage (indistinguishable from `"0"`); `stoi` accepts leading whitespace/`+` and ignores trailing junk unless you check `pos`. Validate with your own grammar.
2. **Depends on the splitter.** Many splitters (`getline`, `strtok`) drop empty tokens. Validators need `"1.1.1."` → 4 parts with an empty last one.
3. **False.** Text with `#`, `,`, newlines or NULs breaks delimiter framing as well.
4. **False.** Not dead ≠ accepted. `"1e"` ends in S6 (alive, not accepting).
5. **False.** `atoi` semantics *allow* trailing junk (`"4193 with words"` → 4193); address validation must consume the **entire** input.
6. **False** for a correct scheme — see the `write ≤ read` proof; an incorrect scheme (e.g. expanding runs) *can*.

## Part D answers

* Empty tokens are the evidence of malformed input (`..`, trailing `.`); dropping them would accept `"1..1.1"`/`"1.1.1.1."` as valid.
* Accepting states are those where input so far is a complete valid number: S2, S3, S5, S8.
* See A4.
* `{"a,b"}` vs `{"a","b"}` both encode to `a,b`.
