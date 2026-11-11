# Pack 05 — Block Forward Error Correction: Parity, Hamming, Golay, BCH, Reed–Solomon (Etherstack interview worksheet)

> **Audience:** from zero; C++17/20. **Rule:** concrete bits first, then derive, then name it. You should be able to rebuild Hamming(7,4) on a napkin without remembering any table.
> **Data rule:** all words, bursts and "received" bits are **synthetic**. No live public-safety RX, no real radio traffic.
> **Companions:** [`answers.md`](answers.md) · [`cheatsheet.md`](cheatsheet.md) · `code/` (stubs `code/include/blockfec.h`, tests, reference in `code/solutions/`).
> **Cross-links:** Pack 01 (popcount, parity) · **Pack 04** (a CRC is a cyclic code; Golay/BCH encoding *is* CRC-style division) · **Pack 06** (convolutional codes, interleaving for bursts).

Sections (cheatsheet cites these): **§1** Why · **§2** Prerequisites · **§3** Concept build-up (3.1–3.8) · **§4** Worked example · **§5** Questions Q1–Q16 · **§6** Self-assessment.

---

## §1 Why this matters

A radio channel flips bits. **Forward error correction (FEC)** adds redundancy so the receiver can repair damage *without asking for a resend* — essential in broadcast, push-to-talk and real-time voice where there is no time (or no return channel) for retransmission (ARQ).

| Where | What it's for |
|---|---|
| **LMR (P25, DMR)** | Control words (network ID, link control, headers) get strong block codes; voice gets weaker codes plus tolerance of residual errors. Specific codes from memory (**VERIFY each against the standard**): P25 FDMA Network ID = BCH(63,16,23) + 1 parity bit; P25 voice frames use Golay(23,12,7) and Hamming(15,11,3)-type codes; P25 headers/link control use Golay(18,6,8)/(24,12,8) and Hamming(10,6,3); Reed–Solomon over GF(64) protects header/link-control words; DMR uses Golay(20,8,7), quadratic-residue (16,7,6), Hamming(15,11,3)/(13,9,3)/(16,11,4) inside BPTC, and RS(12,9) (TIA-102.BAAA-A; ETSI TS 102 361-1). |
| **Gateway** | A gateway that rewrites a protected field (e.g. NAC, Pack 01 §4) must **re-encode** the parity, not just flip bits; and one that relays frames must decide whether to forward corrected or raw bits. |
| **Embedded** | ECC memory (SECDED Hamming), NAND flash BCH, QR codes and CDs (Reed–Solomon). |
| **Interview** | "Implement Hamming(7,4)", "why is the syndrome the error position?", "what is minimum distance and how many errors can you fix?" |

**You will be able to:** compute parity; derive Hamming(7,4) so the syndrome *equals* the error position; use minimum distance to say how many errors a code detects/corrects (`t = ⌊(d−1)/2⌋`) and draw the spheres picture; explain why extended Hamming gives SECDED; explain why Golay(23,12,7) is "perfect"; and say what BCH and Reed–Solomon add.

**Failure if ignored:** a decoder that "corrects" a double error in Hamming(7,4) *confidently produces wrong data* (we reproduce it in §3.3). Knowing the limits of your code is as important as knowing how to decode.

---

## §2 Prerequisites

- Pack 01: XOR, popcount (number of 1 bits = *weight*), mask/shift.
- Pack 04 §3.2: GF(2) arithmetic — XOR is add *and* subtract; polynomial long division by hand.
- Combinations: C(n,k) = n!/(k!(n−k)!). We use C(23,1)=23, C(23,2)=253, C(23,3)=1771.
- Position convention in the code and tables: **position k (1-based) ↔ bit k−1** of the integer; position 1 = least significant bit.

---

## §3 Concept build-up

### 3.1 Concrete first: the repetition code

Send each bit three times: `0 → 000`, `1 → 111`. The receiver takes a **majority vote**.

| Sent | Received | Majority | Verdict |
|---|---|---|---|
| 000 | 000 | 0 | fine |
| 000 | 010 | 0 | one error **corrected** |
| 000 | 110 | 1 | two errors → decoded **wrongly**, confidently |
| 111 | 011 | 1 | one error corrected |

Cost: **rate** = 1/3 (three channel bits per data bit). Benefit: corrects any 1 error per 3. The *same idea* — add redundancy so valid words are "far apart" — scales up to Hamming, Golay, BCH, RS. The rest of this pack answers: *how far apart are the valid words, and how cheaply can we arrange that?*

### 3.2 Parity: the cheapest check

**Even parity bit** = XOR of all data bits = 1 if the data has an odd number of 1s. Appended, the whole word always has an *even* number of 1s.
Example: data `0110 1001` has four 1s → parity 0 → sent `0110 1001 0`. Data `0110 1000` has three 1s → parity 1.
In code: `parity = popcount(x) & 1` (Pack 01 §3.8), or the XOR fold `x ^= x>>16; x ^= x>>8; x ^= x>>4; x ^= x>>2; x ^= x>>1; x&1`. Why the fold works: each step XORs the upper half onto the lower half; XOR is associative and commutative, so after 5 steps bit 0 holds the XOR of all 32 bits.

**What it catches:** flipping one bit changes the count of 1s by ±1 → parity flips → **detected**. Flipping *two* bits changes the count by −2/0/+2 → parity unchanged → **missed**. **Odd** numbers of errors are detected; **even** numbers are not. It cannot say *where* the error is, so it cannot correct.

> **Misconceptions box 3.2**
> 1. *"Parity detects errors."* Only odd-weight error patterns — in a bursty radio channel, double errors are common.
> 2. *"A parity bit corrects a single error."* It locates nothing: with 9 bits, which one flipped?
> 3. *"Even vs odd parity matters for strength."* Same strength; it's a convention (the spec says which).

### 3.3 Hamming distance, minimum distance, and what it buys

**Hamming distance** `d(a,b)` = number of bit positions where `a` and `b` differ = `popcount(a XOR b)`. (E.g., `d(1011, 0010) = 2`.)

A **code** is the set of *valid* words (codewords). Its **minimum distance** `d` is the smallest distance between two distinct codewords.

**Linear codes (Hamming, Golay, BCH, CRC…):** the XOR of two codewords is a codeword, and `d(a,b) = weight(a XOR b)`. So **minimum distance = minimum weight of a nonzero codeword** — you only need to inspect one list, not all pairs.

**The spheres picture.** Draw each codeword as a point, with a *sphere* of radius `t` = all words within `t` bit flips of it.

```
  repetition code d = 3 (cube corners 000 and 111 on a 3-bit cube)

    000 ----- 001 ----- 011 ----- 111
     |  radius 1 ball     |  radius 1 ball
   {000,001,010,100}    {111,110,101,011}      <- the two balls do not overlap, and they
                                                  cover every one of the 8 words (perfect)
```
- If a received word lies in exactly one ball, decode to that ball's centre.
- Balls of radius `t` do **not overlap** iff `d ≥ 2t + 1`. So the **number of errors you can always correct is `t = ⌊(d − 1)/2⌋`**.
- A word that lands *between* balls (distance > t from every codeword) is **detected as uncorrectable**. You can **detect up to `d − 1`** errors (any pattern of ≤ d−1 flips can't turn one codeword into another).
- **Correct t and detect more:** correcting up to `t` while *detecting* up to `s ≥ t` needs `d ≥ t + s + 1`.

| d | correct `t=⌊(d−1)/2⌋` | detect only `d−1` | Example |
|---|---|---|---|
| 2 | 0 | 1 | single parity bit |
| 3 | 1 | 2 | Hamming(7,4), repetition×3 |
| 4 | 1 (+detect 2) | 3 | extended Hamming(8,4) = SECDED |
| 5 | 2 | 4 | BCH(15,7,5) |
| 7 | 3 | 6 | Golay(23,12,7) |
| 8 | 3 (+detect 4) | 7 | extended Golay(24,12,8) |
| 23 | 11 | 22 | BCH(63,16,23) (P25 NID) |

**Failure — beyond the limit, the decoder lies.** With `d = 3` and **two** errors the received word is inside *another* codeword's ball, and decoding picks that wrong codeword with no warning (we reproduce this with Hamming(7,4) in §3.4).

**Sphere-packing bound.** A radius-`t` ball in n-bit space holds `V(n,t) = Σ C(n,i)` (i=0..t) words. The balls around the `2^k` codewords can't overlap: `2^k · V(n,t) ≤ 2^n`. If equality holds the code is **perfect**: every word is within `t` of exactly one codeword. Hamming(7,4): `16 · (1+7) = 128 = 2^7` ✓ perfect. Golay(23,12,7): `4096 · (1+23+253+1771) = 4096 · 2048 = 2^23` ✓ perfect.

> **Misconceptions box 3.3**
> 1. *"d = 3 corrects 1 error **and** detects 2."* It does either, not both at once: a word at distance 2 from codeword A may be at distance 1 from codeword B (decoder fixes it wrongly). Detecting 2 *and* correcting 1 needs `d ≥ 4`.
> 2. *"More redundancy always means more correction."* Only if the extra bits increase `d` (a bad code can add bits and not help).
> 3. *"Perfect codes can detect failures."* A perfect code has *no* gaps: every received word decodes to something; beyond t errors it decodes to the wrong codeword (§3.6).
> 4. *"Min distance = min pairwise distance, so check all pairs."* For linear codes just check the minimum nonzero weight.

### 3.4 Hamming(7,4) — derived so that **syndrome = position**

**Goal.** 4 data bits → 7-bit codeword, correcting any single-bit error, and with a decoder so simple that the failing checks *spell out the error position*.

**Step 1 — how many check bits?** We must be able to say "no error" or "error at position 1…7": 8 outcomes. Each parity check contributes 1 bit of outcome, so `r` checks give `2^r` outcomes. Need `2^r ≥ n + 1` with `n = k + r`: for k=4, r=3 works (8 ≥ 7+1). Generalising, `n = 2^r − 1`: (3,1), (7,4), (15,11), (31,26)…

**Step 2 — name the positions in binary.**

| position | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| binary | 001 | 010 | 011 | 100 | 101 | 110 | 111 |

**Step 3 — define the three checks.** Check *i* (i = 1, 2, 4) covers **every position whose binary number has bit i set**:
- **Check 1** (bit 0 set): positions **1, 3, 5, 7**
- **Check 2** (bit 1 set): positions **2, 3, 6, 7**
- **Check 4** (bit 2 set): positions **4, 5, 6, 7**
Each check demands **even parity** over its group.

**Step 4 — why does the syndrome equal the position?** Suppose the bit at position *p* flips. It belongs to check *i* **iff bit i of p is set** (by construction). So exactly those checks fail, and reading (check-4 result, check-2 result, check-1 result) as a binary number gives **the binary representation of p**. *That is the syndrome.* No lookup table needed: `syndrome = position of the error` (0 = no error). ∎

**Step 5 — where do the check bits go?** Positions 1, 2, 4: the powers of two. Each of those lies in **exactly one** group (e.g. 4 = `100` only has bit 2 set), so each check bit can be set *independently* to make its own group even, without solving simultaneous equations. The data bits go in positions 3, 5, 6, 7 (our code: nibble bit0→3, bit1→5, bit2→6, bit3→7).

**Encoding formulas:** `p1 = c3 ^ c5 ^ c7`, `p2 = c3 ^ c6 ^ c7`, `p4 = c5 ^ c6 ^ c7` (so each group's total is even).

**Concrete example.** Data nibble `1011` (= 11): `d1=1 (pos3), d2=1 (pos5), d3=0 (pos6), d4=1 (pos7)`.
`p1 = 1^1^1 = 1`, `p2 = 1^0^1 = 0`, `p4 = 1^0^1 = 0`.
Codeword, positions 1..7: `1 0 1 0 1 0 1` → integer (bit k−1) `0b1010101 = 0x55`.

**Decode a damaged word.** Flip position 6 → positions 1..7 = `1 0 1 0 1 1 1`.
- check 1 over {1,3,5,7} = 1^1^1^1 = **0** (ok)
- check 2 over {2,3,6,7} = 0^1^1^1 = **1** (fail)
- check 4 over {4,5,6,7} = 0^1^1^1 = **1** (fail)
- syndrome = (check4, check2, check1) = `110` = **6** → flip position 6 → `0x55` → data `1011` ✓.

**Properties.** Codeword weights: 1 of weight 0, 7 of weight 3, 7 of weight 4, 1 of weight 7 → minimum weight 3 ⇒ **d = 3**, `t = 1`, perfect (§3.3).

**Failure — two errors.** Flip positions 1 and 2 of `0x55`: `0x56`. Syndrome = `001 XOR 010 = 011 = 3`: the decoder confidently flips position 3 and returns `1010`, **not** `1011` — a *miscorrection*. Hamming(7,4) cannot tell one error from two. (Fix: §3.5.)

> **Misconceptions box 3.4**
> 1. *"The check bits can go anywhere."* They can (any systematic arrangement is an equivalent code), but **only the power-of-two layout makes the syndrome equal the position**; otherwise you need a lookup table.
> 2. *"The syndrome is a function of the data."* It depends only on the **error pattern**, never on the data (the codeword part cancels — same as CRC, Pack 04 §3.2).
> 3. *"Hamming corrects burst errors."* One bit per codeword only; use interleaving (Pack 06) to turn bursts into single errors per codeword.
> 4. *"Syndrome 0 means the word is correct."* It means it is *a* codeword — possibly the wrong one after ≥ 3 errors.

### 3.5 Extended Hamming (8,4,4): SECDED

Append **one overall even-parity bit** over all 7 bits → 8-bit word, `d = 4`. Now two checks tell the story:

| syndrome `s` (of the 7 bits) | overall parity `P` (of all 8) | Meaning | Action |
|---|---|---|---|
| 0 | 0 | no error | accept |
| ≠ 0 | 1 | **one** error at position `s` | correct it |
| 0 | 1 | one error, in the **overall parity bit** | correct data unchanged |
| ≠ 0 | 0 | **two** errors (even count, but checks fail) | **detect, do not correct** |

Why: one flip changes the overall parity (P=1); two flips leave it unchanged (P=0) while the Hamming checks still fail (`s ≠ 0`, since two different positions XOR to non-zero). This is **SECDED** — *single error correct, double error detect* — as used in ECC memory. `d = 4`: correct 1 **and** detect 2 (needs `d ≥ 1 + 2 + 1`).

> **Misconceptions box 3.5**
> 1. *"The extra bit just adds redundancy."* It specifically separates odd from even error counts.
> 2. *"SECDED fixes two errors."* It *detects* them and refuses to guess.

### 3.6 Golay (23,12,7) and (24,12,8)

The **binary Golay code** maps 12 data bits to 23 bits and has `d = 7` ⇒ corrects **3** errors. It is **cyclic**: its codewords are the multiples of the generator polynomial
`g(x) = x¹¹ + x¹⁰ + x⁶ + x⁵ + x⁴ + x² + 1 = 0xC75` (an alternative generator is `0xAE3`), a divisor of `x²³ + 1`.

**Encoding is CRC-style division (Pack 04):** `codeword = (data << 11) | ((data << 11) mod g)`. Example `data = 1`: `x¹¹ mod g = g − x¹¹ = 0x475`, so the codeword is `0x800 | 0x475 = 0xC75` = `g` itself (the data-1 codeword *is* the generator). `data = 0xABC` → `0x55E11E` (checked independently).

**Weight distribution** (counted independently): weight 0: 1, 7: 253, 8: 506, 11: 1288, 12: 1288, 15: 506, 16: 253, 23: 1 (total 4096 = 2¹²). Smallest nonzero weight **7** ⇒ `d = 7`.

**Why "perfect" (derive).** Ball volume at radius 3: `1 + C(23,1) + C(23,2) + C(23,3) = 1 + 23 + 253 + 1771 = 2048 = 2¹¹`. And `2¹²` codewords × `2¹¹` = `2²³` = *all* 23-bit words. The balls tile the whole space exactly: **every** received word is within 3 bits of exactly one codeword. Consequence: 1, 2 or 3 errors → corrected perfectly; **4 or more errors → silently decoded to a wrong codeword** (there is no "uncorrectable" outcome). Our `golay23_decode` therefore reports `errors` = distance to the nearest codeword (0..3), and the reference solution's `errors = -1` branch is **unreachable** (defensive only) — a perfect code has no "uncorrectable" words.

**Decoding.** Simplest correct method: compare the received word with all 4096 codewords and keep the nearest (**minimum-distance decoding**). Real implementations use a 2048-entry syndrome table (syndrome = remainder of the received word divided by g; each of the 2048 syndromes corresponds to exactly one error pattern of weight ≤ 3 — the perfect-code property again).

**Extended Golay (24,12,8):** add an overall parity bit: `d = 8` ⇒ **correct 3 and detect 4**. **Shortened codes:** to protect fewer data bits (e.g. 6), fix the missing data bits to 0 and don't transmit them: Golay(18,6,8) in P25 headers is a shortened extended Golay (**VERIFY**). Shortening keeps `d` (≥), reduces `n` and `k`.

> **Misconceptions box 3.6**
> 1. *"Golay can correct 3 errors and detect the rest."* Perfect ⇒ it cannot detect >3 errors in the (23,12) form; the (24,12,8) form can detect 4.
> 2. *"Golay is a CRC."* Same mathematics (division by `g`) but *used for correction* with a syndrome→error lookup; a CRC is used for detection.
> 3. *"Shortening weakens the code."* It does not reduce `d`; it reduces rate.

### 3.7 BCH — the "design your own t" family (intuition)

**Hamming** is a BCH code with `t = 1`. **BCH codes** are cyclic codes designed so that the generator polynomial has **consecutive powers of a field element α as roots** (`α, α², …, α²ᵗ`); the **BCH bound** says that guarantees `d ≥ 2t + 1`. Block lengths are `n = 2^m − 1`; the price is about `m·t` check bits.

Reading codes: BCH(n, k, d). P25's NID is **BCH(63, 16, 23)**: `n = 63 = 2⁶ − 1`, `k = 16` (NAC 12 + DUID 4, Pack 01 §3.3), `d = 23` ⇒ `t = ⌊22/2⌋ = 11` errors correctable, using `63 − 16 = 47` check bits, plus 1 extra overall parity bit → 64 bits on air (**VERIFY**, TIA-102.BAAA-A). Heavy protection for 16 bits: if the NID is wrong, the receiver can't even decide whether the frame is for it.

You do **not** need the algebra for interviews; you do need: *what t is*, *how to compute it from d*, and *that encoding is CRC-like division and decoding is syndrome → error locator*.

### 3.8 Reed–Solomon — correcting symbols, not bits (intuition)

**Key idea:** *two points define a line; k points define a polynomial of degree k − 1.* Treat k data **symbols** as the coefficients (or sample values) of a polynomial, and send its value at **n > k** points. Any k received values determine the polynomial, hence the data.

**Toy over the integers mod 5 (a field, GF(5)).** Message `(a0, a1) = (3, 1)` → `p(x) = 3 + x`. Send `p(0), p(1), p(2), p(3) = 3, 4, 0, 1` (n = 4, k = 2).
Lose two symbols — say you only get `p(1) = 4` and `p(3) = 1`. Slope `a1 = (1 − 4)/(3 − 1) = −3/2 (mod 5)`: `−3 ≡ 2`, `2⁻¹ ≡ 3 (mod 5)`, so `a1 = 2·3 = 6 ≡ 1`; intercept `a0 = 4 − 1·1 = 3` ✓. Any 2 of the 4 symbols rebuild the message.

**Distance.** Two different degree-≤1 polynomials agree in at most 1 point, so codewords differ in at least `n − k + 1 = 3` symbols: `d = n − k + 1` (the **Singleton bound** — RS meets it; "maximum distance separable"). With `n − k = 2` check symbols: correct **1 symbol error**, or recover **2 erasures** (known-bad positions). General rule: `2·(errors) + (erasures) ≤ n − k`.

**Why RS for bursts:** it corrects *symbols*; a burst of up to 8 consecutive bit errors inside an 8-bit symbol is **one** symbol error. Real RS uses symbols of `m` bits and arithmetic in GF(2ᵐ) (P25: 6-bit symbols, GF(64): RS(24,12,13), RS(24,16,9), RS(36,20,17) — **VERIFY**); CDs and QR codes also use RS.

> **Misconceptions box 3.8**
> 1. *"RS corrects bits."* It corrects whole symbols; one flipped bit and eight flipped bits in the same symbol cost the same.
> 2. *"Erasures and errors are the same."* An erasure is a *known-bad* position (e.g. lost packet): costs 1 check symbol; an unknown error costs 2.
> 3. *"The toy is just interpolation, not coding."* It is exactly RS (evaluation form); real implementations just use GF(2ᵐ) arithmetic and fast decoders.

---

## §4 Fully worked example — a protected control word, end to end (synthetic)

**1. Hamming(7,4).** Message nibble `1011`. Encode → `0x55` (§3.4). Channel flips position 6 → `0x75`? Check: `0x55 ^ 0x20 = 0x75` (position 6 is bit 5 = `0x20`). Decode:
- bits of `0x75 = 111 0101`: positions 1..7 = `1 0 1 0 1 1 1`.
- syndrome `110` = 6 → flip bit 5 → `0x55` → nibble `1011` ✓.

**2. Same word, two errors** (positions 1 and 2 → `0x56`): syndrome 3 → flips position 3 → nibble `1010` ✗ (miscorrection); with the extended code `0x55 → 0x55` (parity bit 0) and `0x56` has `P = 0` but `s = 3 ≠ 0` → **Uncorrectable** — the system can now **drop the frame or request a repeat** instead of passing garbage.

**3. Golay.** 12-bit control word `0xABC` → `0x55E11E`. Flip 3 bits → decoder returns `0xABC`, `errors = 3`. Flip 4 bits → returns *some other* 12-bit word with `errors ≤ 3` — wrong, confidently. A system that needs to know must add an outer check (CRC or the extended code's parity).

**4. Gateway rule.** In Pack 01 §4 we rewrote the NAC `0x293 → 0xF7E` in the 16 data bits of an NID. The 47 BCH check bits (+1 parity) now disagree with the data — the rewritten frame is *invalid* until the gateway recomputes them with the NID encoder.

**5. RS toy.** `(3,1) → 3,4,0,1`; drop symbols 0 and 2 → recover from `(1,4)`, `(3,1)` → `(3,1)` ✓ (§3.8).

---

## §5 Questions

Each has a **type**, a **context line**, and an answer in `answers.md`.

**Q1 — Explain / Calculate.** *Context: embedded serial link.* (a) Compute even parity for bytes `0x69`, `0x68`, `0xFF`. (b) Why does one parity bit detect every single-bit error but miss every double-bit error? (c) Why can it not correct?

**Q2 — Calculate.** *Context: designing a toy control-word code.* Codewords `{00000, 01011, 10101, 11110}`. Compute all pairwise distances, `d`, `t`, and how many errors it detects. Show this is a linear code and that `d` equals the minimum weight.

**Q3 — Calculate.** *Context: reading a standard.* For d = 3, 4, 5, 7, 8, 23 compute `t = ⌊(d−1)/2⌋` and the number of errors detected (alone). For which can you correct *and* detect extra errors, and how many?

**Q4 — Predict.** *Context: repetition-3 code in a toy link.* Sent `000` or `111`. Which received words decode correctly under majority vote? How many of the 8 words decode wrongly if **two** bits flip? Draw the spheres.

**Q5 — Calculate.** *Context: encoding a P25-style 4-bit field with Hamming(7,4).* Encode nibbles `0110` and `1001` with the position layout of §3.4; give codewords as bit strings (positions 1..7) and hex (bit k−1). Verify with `0110 ^ 1001 = 1111` using linearity (`0x7F`).

**Q6 — Trace.** *Context: decoding.* Received `0x24`. Compute the three checks, the syndrome, the corrected word, the data nibble.

**Q7 — Predict.** *Context: bursty channel.* Codeword `0x55` is hit by two adjacent flips (positions 1 and 2). What does Hamming(7,4) output? What would extended Hamming(8,4) say? Why does this matter for a radio link with bursts?

**Q8 — Explain.** *Context: derive it on a whiteboard.* Why 3 check bits for 4 data bits? Why positions 1, 2, 4? Why is the syndrome equal to the position? How many check bits for 11 data bits? For 26?

**Q9 — Calculate.** *Context: ECC memory.* Classify these received 8-bit extended-Hamming words (encoding of nibble `1011` is `0x55`): `0x55`, `0xD5`, `0x45`, `0x56`.

**Q10 — Calculate.** *Context: Golay in P25/DMR headers.* (a) Show Golay(23,12,7) is perfect. (b) What happens with 4 errors? (c) What does extending to (24,12,8) buy? (d) What is `t` and what can be detected?

**Q11 — Explain.** *Context: whiteboard sphere picture.* Draw spheres for d = 5 (t = 2) and d = 4. Show why d = 4 allows correct-1 + detect-2 but d = 3 doesn't. Why does correcting `t` and detecting `s` need `d ≥ t + s + 1`?

**Q12 — Find the bug.** *Context: code review.*
```cpp
Hamming74Result decode(uint8_t cw) {
  unsigned s1 = parity(cw & 0x55), s2 = parity(cw & 0x66), s4 = parity(cw & 0x78);
  unsigned syn = s1 + s2 + s4;                 // (a)
  cw ^= (1u << syn);                            // (b)
  return { uint8_t(cw & 0x0F), (uint8_t)syn, true };   // (c)
}
```
Find at least four bugs and fix them.

**Q13 — Implement.** *Context: gateway FEC library.* Implement every stub in `code/include/blockfec.h` (parity, distance, Hamming(7,4)/(8,4), Golay(23,12), RS-GF(5)). All `student.*` tests green; sanitizers silent.

**Q14 — Connect.** *Context: whole module.* (a) How is Golay/BCH encoding related to Pack 04 CRC? (b) Why do real systems **interleave** before the channel and what does it do to a burst (Pack 06)? (c) After a gateway edits the NAC (Pack 01), which code must it recompute and why?

**Q15 — Interview.** *Context: phone screen.* "Implement Hamming(7,4) encode and decode. Why does the decoder work without a lookup table? What are its limits?"

**Q16 — Interview.** *Context: system design.* "You must protect a 16-bit control word on a link with up to 3 random bit errors per word, 32 bits budget. Which code(s)? How do you know the decoder isn't fooling you? What if the budget were only 24 bits? FEC vs ARQ?" (Hint: BCH(31,16,7) has `d=7`, `t=3`, `n=31`. Golay(24,12,8) protects only 12 bits per 24, so 16 bits would need two words = 48 bits. Think about an outer CRC to catch miscorrection.)

---

## §6 Self-assessment

- [ ] I can compute parity and explain why it catches odd errors only.
- [ ] I can compute Hamming distance and min distance of a small code, and use `t=⌊(d−1)/2⌋`.
- [ ] I can draw the spheres picture and explain the sphere-packing/perfect-code condition.
- [ ] I can derive Hamming(7,4) from scratch: check-bit count, positions, groups, syndrome = position.
- [ ] I can encode and decode a Hamming(7,4) word by hand and show a double-error miscorrection.
- [ ] I can explain SECDED and classify `(s, P)` cases.
- [ ] I can state Golay(23,12,7)'s `g`, `d`, `t`, and why it is perfect.
- [ ] I can describe BCH and Reed–Solomon at the level of: what `t` means, bits vs symbols, `d = n−k+1`.
- [ ] `ctest`: all `solution.*` and (after Q13) `student.*` tests pass; sanitizers silent.

**Rapid fire (key in `answers.md`):** (1) `t` for d=9? (2) How many check bits for k=11 Hamming? (3) Hamming(7,4) rate? (4) syndrome for a flip at position 5? (5) RS(255,223) corrects how many symbol errors?

**VERIFY list:** which codes P25 (TIA-102.BAAA-A, BAAB) and DMR (ETSI TS 102 361-1, -2) use for each field: BCH(63,16,23) NID, Golay(23,12,7)/(24,12,8)/(18,6,8)/(20,8), Hamming(10,6,3)/(15,11,3)/(13,9,3)/(16,11,4), QR(16,7,6), RS over GF(64) (24,12,13)/(24,16,9)/(36,20,17), DMR RS(12,9); Golay generators `0xC75`/`0xAE3`; BCH(31,16,7) parameters; the claim that P25 NID has a 64th overall parity bit.
