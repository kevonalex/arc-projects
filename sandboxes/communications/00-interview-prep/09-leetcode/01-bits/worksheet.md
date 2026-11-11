# 01 · Bits — Pattern Worksheet

**Pattern:** treat an integer as a vector of 32 independent bits; use `& | ^ ~ << >>` to touch all bits at once.
**Why it matters at a radio/protocol shop:** every header, CRC, bitmap, HARQ mask and flag word you will ever parse is a packed integer. If bit tricks feel natural, framing/parsing code feels natural.
**Coded:** all 12 problems have stubs + tests (`code/`). Answers: `answers.md`. One-page recap: `cheatsheet.md`.

> Etherstack-style context lines below are *illustrative* (radio / protocol-stack flavoured). They are not claims about any specific interview question.

---

## The method — use it on EVERY problem

| # | Step | You must write down |
|---|------|---------------------|
| 1 | **Clarify** | input ranges, signed/unsigned, width (32 vs 64), empty/min/max, what "valid" means |
| 2 | **Brute force + complexity** | the obvious loop, its time and space |
| 3 | **Optimise** | the invariant/property that removes work; one-sentence proof |
| 4 | **Code** | C++17/20, fixed-width unsigned types, no UB |
| 5 | **Hand-trace** | one normal input *and* one edge input, bit by bit, in binary |
| 6 | **Final complexity** | time + space, and what dominates |

Build/run: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure`
(flags `-Wall -Wextra -fsanitize=address,undefined` are already in the CMake file).

---

## Part A — Why the technique works

### A1. An integer is a polynomial in 2; bitwise ops are per-column

`n = Σ bᵢ·2ⁱ`, `bᵢ ∈ {0,1}`. `&`, `|`, `^`, `~` act on each column independently — no carries between columns. Only `+`, `-`, `*` and shifts mix columns. **Every bit trick is "do something to one column, for all 32 columns in parallel".**

Per-column truth table (memorise):

| a | b | a&b | a\|b | a^b |
|---|---|-----|------|-----|
| 0 | 0 | 0 | 0 | 0 |
| 0 | 1 | 0 | 1 | 1 |
| 1 | 0 | 0 | 1 | 1 |
| 1 | 1 | 1 | 1 | 0 |

### A2. XOR is addition in GF(2) — this is *why* it cancels pairs

Treat one column as arithmetic mod 2: `a ^ b = (a + b) mod 2`. Then, for whole words:

1. **Identity:** `x ^ 0 = x`
2. **Self-inverse:** `x ^ x = 0` (since `1+1 = 0 mod 2`)
3. **Commutative:** `a ^ b = b ^ a`
4. **Associative:** `(a ^ b) ^ c = a ^ (b ^ c)`

(1)–(4) make the 32-bit words an **abelian group in which every element is its own inverse**. Consequences you will use:

* Order of XOR does not matter → you may *reorder* a stream so equal values sit next to each other; each adjacent pair becomes `x ^ x = 0` and disappears; only unpaired values survive. **This is the whole proof for 136 and 268** (you will write it out below).
* `a ^ b = c  ⇒  a = b ^ c` (XOR both sides by `b`) → the XOR "swap" and "recover the missing one" tricks.

> **Your turn A2:** prove that `x ^ x ^ y ^ x ^ y ^ z` reduces to `x ^ z` using *only* rules 1–4, one rule per line.

### A3. `n & (n-1)` clears the lowest set bit

Write `n = P · 1 · 0ᵏ` (any prefix `P`, lowest set bit, `k` trailing zeros). Subtracting 1 borrows through the zeros: `n-1 = P · 0 · 1ᵏ`. AND of the two: prefix survives (`P & P = P`), the borrowed column gives `1 & 0 = 0`, the trailing columns give `0 & 1 = 0`. Result: `P · 0 · 0ᵏ` — exactly `n` minus its lowest set bit. Each application removes exactly one `1`, so a loop runs `popcount(n)` times. `n & -n` isolates that lowest set bit (same picture with `-n = ~n + 1`).

### A4. Two's complement and why we use **unsigned**

On a 32-bit machine, arithmetic on `uint32_t` is arithmetic **mod 2³²**. Negative `int` values are represented as `2³² − |x|`; `-x == ~x + 1`. Consequences:

* Adding two's-complement numbers with an *unsigned* adder gives the right bit pattern for signed results too (that is why hardware has one adder). So 371 can be done on `uint32_t` and cast back.
* In C++, **signed overflow is UB** (`INT_MAX + 1`), and so is shifting bits *past* the sign bit (`2 << 31`) or a shift count `>= width`. Shifting a `1` *into* the sign bit was UB in C/C++11, implementation-defined in C++14/17 and defined in C++20 — so to stay portable do bit work on **unsigned** types, where wrap-around is always well-defined. UBSan flags the UB cases. `>>` on a negative signed value is an arithmetic (sign-extending) shift in practice (implementation-defined before C++20) — never use it as a "logical" shift.
* Always use `std::uint32_t`/`std::uint64_t` for masks and shifts; shift counts must be `< width`.

### A5. Counting by columns

If a quantity is a *sum over pairs/elements of a per-bit function*, swap the sums: loop over the 32 columns outside, elements inside. In one column with `k` ones among `n` elements:

* number of differing pairs is `k·(n−k)` → **477**
* `count mod 3` of that column over a "thrice + one" multiset equals that bit of the single → **137**

### A6. Common-prefix and Gray structure

* AND of a contiguous range `[L, R]` keeps only the bits above the first bit position where `L` and `R` differ (everything below flips somewhere inside the range) → **201**.
* Reflected Gray code `g(i) = i ^ (i >> 1)`: stepping `i → i+1` flips a run of low bits (`0111 → 1000`); the XOR with the shifted copy turns that run into a *single* flipped bit → **89**.

---

## Part B — Problems

Legend: **Ctx** = protocol/radio-flavoured context line. For each, fill steps 1–6 *before* looking at `answers.md`. Stub names are in `code/headers/bits.hpp`.

### P1 · LC 191 Number of 1 Bits — `hamming_weight`
**Ctx:** count asserted flags in a 32-bit PDCP/MAC control word.
1. Clarify: input type? (`uint32_t`) value `0`? all ones?
2. Brute: test 32 bits → ____ time ____ space
3. Optimise: use A3. How many loop iterations in the worst case? ____
5. Trace: `0b1011`, and `0x80000000`
6. Final: ____
*Trap:* writing `n >> 1` on a signed int, or looping `while (n)` with `n -= 1`.

### P2 · LC 190 Reverse Bits — `reverse_bits`
**Ctx:** convert MSB-first on-air bit order to LSB-first register order.
1. Clarify: width fixed at 32 even if the high bits are zero?
2. Brute: build a string of bits, reverse, parse → ____
3. Optimise: shift-in/shift-out loop; then name the O(log w) "swap halves" divide-and-conquer version.
5. Trace: `0b0001` (→ `0x80000000`), `0xFFFFFFFF`
6. Final: ____
*Trap:* forgetting that leading zeros must be reversed too.

### P3 · LC 338 Counting Bits — `count_bits`
**Ctx:** precompute a popcount table (weight of each syndrome value 0..n).
1. Clarify: `n = 0`? output length?
2. Brute: popcount each `i` → ____
3. Optimise: find a recurrence relating `bits(i)` to a *smaller* index. Give two (shift-based and `i & (i-1)` based) and prove one.
5. Trace: `n = 5`
6. Final: ____

### P4 · LC 136 Single Number — `single_number`
**Ctx:** every frame id is logged twice (tx + ack) except one lost ack.
1. Clarify: exactly one unpaired? negative values? empty?
2. Brute: hash-map counts → ____ ; sort → ____
3. Optimise: **prove with A2 why XOR of everything leaves only the single**. Write the regrouping argument.
5. Trace: `[4,1,2,1,2]` as binary columns
6. Final: ____
*Misconception to refute:* "XOR needs the array sorted so pairs are adjacent."

### P5 · LC 137 Single Number II — `single_number_ii`
**Ctx:** triple-modular redundancy: every id arrives 3×, one only once.
1. Clarify: why does plain XOR fail here? (what is `x^x^x`?)
2. Brute: hash-map → ____
3. Optimise (a): per-column count mod 3 (32 columns). (b) the `ones/twos` two-bit counter. Draw the state machine `00→01→10→00`.
5. Trace: `[2,2,3,2]`
6. Final: ____

### P6 · LC 268 Missing Number — `missing_number`
**Ctx:** HARQ process ids `0..n` — which one never reported in?
1. Clarify: n values drawn from `[0, n]`, distinct, so exactly one is missing.
2. Brute: sort, scan → ____ ; hash-set → ____ ; Gauss sum → ____
3. Optimise: **XOR argument.** Build the multiset `{0..n} ∪ nums`. Which values appear twice? once? Use A2. Also say why Gauss sum can overflow `int` and XOR cannot.
5. Trace: `[3,0,1]` (n=3), and `[0]` (answer is n itself)
6. Final: ____

### P7 · LC 461 Hamming Distance — `hamming_distance`
**Ctx:** number of bit errors between transmitted and received symbol.
1. Clarify: signed inputs? 2. Brute: compare 31 bits. 3. Optimise: `x ^ y` marks differing columns (A1), then P1. 5. Trace: `1, 4`. 6. Final: ____

### P8 · LC 477 Total Hamming Distance — `total_hamming_distance`
**Ctx:** total pairwise distance of a code book — an input to minimum-distance reasoning.
1. Clarify: n up to 10⁴, values up to 10⁹; answer fits `int`? (it fits; intermediate `k·(n−k)` is safest as 64-bit).
2. Brute: all pairs × popcount → ____
3. Optimise: A5. Derive `Σ_bits k·(n−k)`.
5. Trace: `[4,14,2]`
6. Final: ____

### P9 · LC 371 Sum of Two Integers — `get_sum`
**Ctx:** the half-adder/carry-save step inside a hardware CRC accumulator or an FPGA adder you must model without `+`.
1. Clarify: negative values? overflow? may not use `+ -`.
2. Brute: increment `a` `b` times — ____ (and breaks for negatives).
3. Optimise: per column, `sum_bit = a^b`, `carry_bit = a&b` which lands in the *next* column → `(a&b)<<1`. Iterate until carry is 0. **Show the algorithm bit by bit on `13 + 7`** (table with columns a, b, a^b, (a&b)<<1 per round).
4. Code: use `uint32_t` for A4.
5. Trace: `13 + 7`; `-1 + 1`; `INT_MIN + INT_MAX`
6. Final: ____ (hint: max number of rounds = carry-chain length, ≤ 32)
*Trap:* `(a & b) << 1` on a negative `int` is UB.

### P10 · LC 201 Bitwise AND of Numbers Range — `range_bitwise_and`
**Ctx:** `[L, R]` is a range of addresses; find the CIDR-like common prefix that every address shares.
1. Clarify: `0 ≤ L ≤ R ≤ 2³¹−1`. 2. Brute: AND all → ____ . 3. Optimise: A6; shift both right until equal, count shifts, shift back. Prove why low bits are 0. 5. Trace: `5..7`, `26..30`. 6. Final: ____

### P11 · LC 89 Gray Code — `gray_code`
**Ctx:** constellation labelling — adjacent symbols differ by 1 bit so a nearest-neighbour symbol error costs 1 bit error.
1. Clarify: list of `2ⁿ` integers, start `0`, neighbours (including last→first) differ by one bit. 2. Brute: backtracking over unused numbers → ____ . 3. Optimise: (a) mirror construction (b) `i ^ (i>>1)`. Prove (b) flips one bit per step (A6). 5. Trace: `n = 3`. 6. Final: ____

### P12 · LC 393 UTF-8 Validation — `valid_utf8`
**Ctx:** variable-length header field — the first byte says how many continuation bytes follow (same shape as many length-extension encodings).
1. Clarify: only the low 8 bits of each int matter; 1–4 byte chars. 2. Brute: n/a (single pass is natural) — list the states. 3. Optimise: single pass with `remaining` counter and four masks. Write the mask table `0xxxxxxx, 110xxxxx, 1110xxxx, 11110xxx, 10xxxxxx`. 5. Trace: `[197,130,1]`, `[235,140,4]`. 6. Final: ____

---

## Part C — Misconceptions (decide true/false, then check `answers.md`)

1. "`x >> 1` always divides a negative number by two."
2. "`1 << 31` is always fine for `int`, and so is `1 << 32`."
3. "XOR-based Single Number needs the input sorted."
4. "`a & b == 0` parses as `(a & b) == 0`." (Operator precedence! Check.)
5. "For 137 I can XOR everything, like 136."
6. "`~x` for `uint8_t x` gives a `uint8_t`." (integer promotion!)
7. "Reversing bits of `1` gives `1` when the width is 32."

## Part D — Self-check (no notes)

* State the four XOR laws and name the algebraic object they form.
* Derive `n & (n-1)` from the picture `P 1 0ᵏ`.
* Why does `(a ^ b)` + `((a & b) << 1)` equal `a + b`? Prove for one column plus carry-in.
* Which of these 12 problems decompose by column? (Answer: 137, 338-style, 477, 191, 190.)
