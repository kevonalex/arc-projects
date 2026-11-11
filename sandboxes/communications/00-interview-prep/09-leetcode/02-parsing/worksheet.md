# 02 · Parsing — Pattern Worksheet

**Pattern:** read input left to right, **one position at a time**, keeping a small explicit *state* that says what is legal next. Parsers are deterministic finite automata (DFAs) with a few counters.
**Why it matters at a radio/protocol shop:** every inbound byte stream (SIP/SDP text, config files, TLV headers, framed sockets) is attacker- or noise-controlled. A parser must be **total** (any bytes → defined result, never out-of-bounds) and **canonical** (one meaning per input).
**Coded:** 8, 468, 271, 443, 65 — `code/headers/parsing.hpp`. Answers: `answers.md`. Recap: `cheatsheet.md`.

> Context lines are illustrative radio/protocol framings, not claims about real interview questions.

## The method — every problem

1 **Clarify** (alphabet, whitespace, empty input, limits, what is *invalid*) → 2 **Brute force + complexity** → 3 **Optimise** (name the state machine / pointer invariant) → 4 **Code** (C++17/20, `size_t` indices, no reads past `size()`) → 5 **Hand-trace** one normal + one edge input *state by state* → 6 **Final complexity**.

Build/test: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure` (ASan+UBSan on).

---

## Part A — Why the technique works

### A1. A DFA = (states, alphabet classes, transition table, accepting set)
Parse a *language* `L` of valid strings. A DFA decides `L` in one pass if `L` is **regular**. The state encodes exactly the information about the prefix that matters for the future ("have I seen a sign? digits? a dot?"). **Invariant:** after reading prefix `p`, `state = δ*(start, p)`, and `p·s ∈ L ⟺ δ*(state, s) ∈ Accept`. Correctness of the code is therefore *correctness of the table* — you can verify it by inspecting each (state, class) cell, not by testing every string.

Complexity: one table lookup per character → **O(n) time, O(1) space** (table size is a constant of the grammar).

### A2. Collapse the alphabet into classes
Don't branch on 256 byte values. Map each byte to a **class** (digit, sign, dot, `e/E`, other). Table size = `#states × #classes`. "Other" always leads to the **dead state** (reject). This is how you keep the parser total.

### A3. Order of checks = order of the grammar
`atoi` is `ws* sign? digit*`; each stage can only be entered from the previous. Write code in that order, with one pointer that only moves right; there is no backtracking, so each byte is examined O(1) times.

### A4. Saturate *before* you overflow
To append digit `d` to `acc` (non-negative `int`) without UB, check `acc*10 + d ≤ INT_MAX` **without computing it**: `acc ≤ (INT_MAX − d) / 10` (integer division; for integers `acc·10 + d ≤ M ⟺ acc ≤ ⌊(M−d)/10⌋`). Signed overflow is UB; the check happens in range.

### A5. Framing: length-prefix is a *prefix-free* code
A delimiter-only encoding (`a,b,c`) breaks when payload contains the delimiter; escaping makes it ambiguous-prone. `"<len>#<payload>"` is **self-delimiting**: the decoder reads the digits up to the first `#` (digits can't contain `#`, so the first `#` is unambiguous), then takes exactly `len` bytes — *whatever they are*. Payload content can never influence framing. Decoding is the inverse by induction on the number of strings.

### A6. In-place two-pointer rewriting
`read` scans, `write ≤ read` always. **Invariant:** `chars[0..write)` is the compressed form of `chars[0..read)`. The write can never overwrite unread data if each output run is no longer than the input run it replaces (checked in the answer).

---

## Part B — Problems

### P1 · LC 8 String to Integer (atoi) — `my_atoi`
**Ctx:** parse a decimal `Content-Length`-style field out of a text header, saturating instead of overflowing.
1. Clarify: leading spaces only `' '`? sign once? stop at first non-digit? `"+-12"`? result clamped to `[INT_MIN, INT_MAX]`?
2. Brute: strip → strtol-like with `long long` and clamp → ____ (what breaks for 20-digit strings?)
3. Optimise: single pass, three phases (A3), pre-overflow check (A4).
5. Trace: `"   -42"`, `"2147483648"`, `"-"`, `""`.
6. Final: ____
*Trap:* checking overflow *after* multiplying; treating `"+-1"` as −1; reading `s[i]` when `i == n`.

### P2 · LC 468 Validate IP Address — `valid_ip_address`
**Ctx:** validate an endpoint address string from a provisioning file before opening a bearer socket.
1. Clarify: IPv4 = 4 decimal fields 0–255, no leading zeros; IPv6 = 8 fields of 1–4 hex digits, no `::` compression here; classify "IPv4"/"IPv6"/"Neither".
2. Brute: regex → ____ (and why that's a trap)
3. Optimise: split **keeping empty tokens** (`"1.1.1."` → 5 parts); validate each token; decide family by which delimiter appears.
5. Trace: `"172.16.254.1"`, `"192.168.01.1"`, `"1.1.1.1."`, `"2001:0db8:85a3::8A2E:037j:7334"`
6. Final: ____
*Trap:* `"0.0.0.0"` valid but `"00.0.0.0"` not; `"1e1.1.1.1"`; `std::stoi` accepting `"+1"`/`" 1"`.

### P3 · LC 271 Encode and Decode Strings — `encode_strings` / `decode_strings`
**Ctx:** frame arbitrary text records over a TCP byte stream.
1. Clarify: payload may contain **any** char including `#`, digits, empty string, NUL; list may be empty.
2. Brute: join with a delimiter (why wrong?) ; escape scheme (why fragile?)
3. Optimise: A5. State the decoder invariant and why the first `#` is unambiguous.
5. Trace: `{"ab",""}` → `"2#ab0#"`; `{"3#abc"}`.
6. Final: ____

### P4 · LC 443 String Compression — `compress`
**Ctx:** run-length compress a long idle/pad pattern in place in a fixed buffer.
1. Clarify: runs of length 1 are *not* followed by a count; counts ≥ 10 are split into digit characters; return new length; O(1) extra space.
2. Brute: build a new string, copy back → ____
3. Optimise: read/write pointers (A6). Prove `write ≤ read`.
5. Trace: `a a b b c c c`; `a` + twelve `b`.
6. Final: ____

### P5 · LC 65 Valid Number — `is_number`
**Ctx:** validate a numeric config field such as `868.1e6` or `-.5` without calling `strtod` (whose accepted grammar is wider: hex, inf, nan, whitespace).
1. Clarify: write the grammar: `[sign] (digits [. [digits]] | . digits) [(e|E) [sign] digits]`. Is `"4."` valid? `".5"`? `"."`? `"e9"`? `"1e"`?
2. Brute: regex / nested ifs → ____
3. Optimise: draw the **9-state DFA** yourself (start, sign, int digits, `digits.`, `.` alone, fraction digits, `e`, exp sign, exp digits). Mark accepting states. Fill the table in `answers.md` *after* you've tried.
5. Trace: `"-.9"`, `"53.5e93"`, `"99e2.5"`, `"."`
6. Final: ____

---

## Part C — Misconceptions (T/F)

1. "`std::stoi` / `atoi` is a safe way to validate a number."
2. "Splitting `"1.2.3."` on `'.'` gives 3 parts." (depends on the splitter — which do you need?)
3. "Length-prefix framing is only needed for binary data."
4. "If the DFA never reaches the dead state, the string is valid."
5. "A parser that ignores trailing garbage is fine for `atoi` but also for IP validation."
6. "In-place compression can overwrite data it hasn't read yet."

## Part D — Self-check
* Why must the IP splitter keep empty tokens?
* Write the transition table for 65 from memory; what are the accepting states and why those?
* Prove the `INT_MAX` pre-check is equivalent to `acc*10 + d ≤ INT_MAX`.
* Show a payload for which a delimiter-only framing is ambiguous.
