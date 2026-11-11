# Pack 05 — Answers (Block FEC)

Format: **Answer → full reasoning → common wrong answers (why tempting / why wrong)**. Section refs (§) point into [`worksheet.md`](worksheet.md). All data is synthetic. Numeric examples were cross-checked independently with a small script (see disclosure in the final report); the C++ was not compiled in the authoring environment.

---

## Q1 — Parity

**(a)** `0x69 = 0110 1001`: four 1s → parity **0**. `0x68 = 0110 1000`: three 1s → parity **1**. `0xFF`: eight 1s → parity **0**.

**(b)** Parity bit = XOR of all bits, so the whole word (data + parity) always has an even count of 1s. Flip one bit → the count changes by exactly ±1 → odd → detected. Flip two bits → the count changes by −2, 0 or +2 → still even → **missed**. Generally: odd numbers of flips are detected; even numbers are invisible (§3.2).

**(c)** The check only says "an odd number of bits are wrong". With 9 bits there are 9 equally likely suspects; nothing identifies which, so it cannot correct. To locate you need *several* independent checks whose pattern of failure encodes the position — Hamming (§3.4).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Parity detects all errors up to 2 bits." | One bit is caught, so two "should" be. | Two flips cancel in the XOR; `d = 2` means detect `d−1 = 1`, not 2. |
| "Parity 1 for `0x69` since there are 4 ones, and 4 is 'a lot'." | Confuses count with parity. | Even parity = XOR; even count → 0. |
| "Parity corrects by flipping the last bit." | Correction feels like a flip of the check bit. | The error may be in any bit, including data. |

---

## Q2 — The toy code `{00000, 01011, 10101, 11110}`

Pairwise distances (`popcount(a ^ b)`):
- 00000 vs 01011: 3 · vs 10101: 3 · vs 11110: 4
- 01011 vs 10101: `11110` → 4 · vs 11110: `10101` → 3
- 10101 vs 11110: `01011` → 3

**d = 3**, so `t = ⌊(3−1)/2⌋ = 1` error corrected, **2** errors detected (alone).
**Linear:** `01011 ^ 10101 = 11110`, `01011 ^ 11110 = 10101`, `10101 ^ 11110 = 01011` — closed under XOR (with 00000 as the identity) ⇒ a linear code. The minimum nonzero weight is `min(3, 3, 4) = 3 = d` (§3.3: for a linear code `d(a,b) = weight(a ^ b)`, and `a ^ b` is itself a codeword).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "d = 4 (the biggest distance)." | Eyeballed a long pair. | `d` is the **minimum**; one pair at 3 caps it. |
| "d = 2 (only 5 bits, 2 data bits)." | Mixes up rate with distance. | Count; (5,2,3) is a legitimate code. |
| "It corrects 2 errors since d=3 detects 2." | Confuses detect with correct. | `t = ⌊(d−1)/2⌋ = 1`. |

---

## Q3 — The `d → t` table

| d | t = ⌊(d−1)/2⌋ | detect-only (d−1) | correct + detect extra (t'+s+1 ≤ d) |
|---|---|---|---|
| 3 | 1 | 2 | correct 1 **or** detect 2 (not both) |
| 4 | 1 | 3 | correct 1 **and** detect 2 |
| 5 | 2 | 4 | correct 2, or correct 1 + detect 3 |
| 7 | 3 | 6 | correct 3, or 2 + detect 4, or 1 + detect 5 |
| 8 | 3 | 7 | correct 3 **and** detect 4 |
| 23 | 11 | 22 | correct 11 and detect 11 |

Reason: balls of radius `t` around codewords are disjoint iff `d ≥ 2t + 1` (the triangle inequality); a received word that is more than `t` from *every* codeword is a detected failure; an `s`-error word must not land within `t` of a different codeword, so `d > s + t` (§3.3, Q11).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "t = d/2." | Half is the obvious fraction. | At d = 4 a word at distance 2 is a **tie** between two codewords — not correctable. Use the floor of `(d−1)/2`. |
| "d = 3 corrects 1 and detects 2." | The table of "t / detect" appears side by side. | Both are *capabilities*, not simultaneous: a 2-error word is within 1 of another codeword (Hamming(7,4) miscorrects it). |

---

## Q4 — Repetition-3

Majority vote: `000, 001, 010, 100 → 0`; `011, 101, 110, 111 → 1`. Four words decode to 0, four to 1.
Sent `000`: received `000` (0 errors) and `001, 010, 100` (1 error) decode correctly (0 errors ✓, 1 error ✓). Two flips give `011, 101, 110` → decode to **1**: wrong. Three flips `111` → wrong. So all **3** of the 2-error patterns decode wrongly (and the 3-error one).
Spheres: ball(000) = {000,001,010,100}, ball(111) = {111,110,101,011}; disjoint and cover all 8 words — this is a **perfect** code (§3.3).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Majority vote fixes any 2 errors in 3 bits." | Two of three bits still agree… with the wrong value. | After 2 flips the majority *is* the wrong bit. |
| "Only `111` decodes wrongly." | Confuses with "all wrong". | Three of the four words that decode to 1 are reachable by two flips of `000`. |

---

## Q5 — Encoding `0110` and `1001`

Layout recap: nibble bit0→pos 3, bit1→pos 5, bit2→pos 6, bit3→pos 7; `p1=c3^c5^c7`, `p2=c3^c6^c7`, `p4=c5^c6^c7`.

**`0110`** (d4=0, d3=1, d2=1, d1=0): c3=0, c5=1, c6=1, c7=0. `p1 = 0^1^0 = 1`, `p2 = 0^1^0 = 1`, `p4 = 1^1^0 = 0`. Positions 1..7 = `1 1 0 0 1 1 0` → bits `1+2+16+32 = 51 = 0x33`.

**`1001`** (d4=1, d3=0, d2=0, d1=1): c3=1, c5=0, c6=0, c7=1. `p1 = 1^0^1 = 0`, `p2 = 1^0^1 = 0`, `p4 = 0^0^1 = 1`. Positions 1..7 = `0 0 1 1 0 0 1` → `4 + 8 + 64 = 76 = 0x4C`.

**Linearity check:** `0110 ^ 1001 = 1111`; `0x33 ^ 0x4C = 0x7F` and `encode(1111)`: all data 1 → every parity `1^1^1 = 1` → all seven bits 1 = `0x7F` ✓. Parity is XOR, so the encoder is linear: `encode(a ^ b) = encode(a) ^ encode(b)`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Writing data bits in positions 1..4 and parity after (`0110 110`). | "Systematic" layout feels natural. | Valid code, but then the syndrome ≠ position; decoder needs a lookup. We use the power-of-two layout (§3.4). |
| Reading the hex of the position string as if position 1 is the MSB. | Reading left to right. | Position k is bit k−1: position 1 = LSB. For palindromic strings (like `1010101`) no difference, for `1100110` it matters (0x33 vs 0x66). |

---

## Q6 — Decoding `0x24`

`0x24 = 0010 0100`: bits 2 and 5 → positions 3 and 6. Positions 1..7: `0 0 1 0 0 1 0`.
- check 1 over {1,3,5,7}: `0^1^0^0 = 1`
- check 2 over {2,3,6,7}: `0^1^1^0 = 0`
- check 4 over {4,5,6,7}: `0^0^1^0 = 1`
Syndrome = (c4,c2,c1) = `101` = **5**. Flip position 5 (bit 4, `0x10`): `0x24 ^ 0x10 = 0x34` (positions 3,5,6 set; a valid codeword — `encode(7) = 0x34`). Data: d1=pos3=1, d2=pos5=1, d3=pos6=1, d4=pos7=0 → nibble `0111` = **7**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Syndrome = 3 + 6 = 9." | Adding the positions of the 1-bits. | The syndrome is the **XOR** of the positions of all 1-bits: `011 ^ 110 = 101 = 5` (a nice shortcut that agrees with the three checks). A sum like 9 is outside 1..7. |
| Flipping bit 5 (0x20) instead of position 5. | Off-by-one, position ↔ bit. | Position k is bit k−1. |

---

## Q7 — Two adjacent flips

`0x55 ^ 0x03 = 0x56`: syndrome `001 ^ 010 = 011 = 3` → Hamming(7,4) flips position 3 → `0x52`, data `1010 = 0xA` ≠ `1011`. **Miscorrection, no warning.**
Extended (8,4): `0x55` has weight 4 → parity bit 0 → word `0x55`. Two flips → `0x56`; `s = 3 ≠ 0` and overall parity **even** → **Uncorrectable** (detected).
Why it matters for bursts: radio errors cluster; adjacent flips are the *common* case, and Hamming(7,4) alone turns them into silently wrong data. Mitigations: SECDED to detect and discard, interleaving to spread the burst (Pack 06 §3.4), and an outer CRC (Pack 04).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The decoder will say no error." | Two errors cancel in parity. | They don't cancel in the *syndrome*: two different positions XOR to a nonzero value — but pointing to a *third*, innocent position. |
| "Hamming(7,4) detects 2 errors (d=3 → d−1 = 2)." | Detect-only capability. | Detect-only if you **don't** correct; once correcting, the 2-error word is inside another ball. |

---

## Q8 — Deriving Hamming(7,4)

*Why 3 check bits?* The decoder must name "no error" + 7 positions = 8 outcomes; `r` checks give `2^r`; with `n = k + r`: `2^r ≥ k + r + 1`. `r = 2` gives 4 < 7; `r = 3` gives `8 ≥ 8` ✓.
*Why positions 1, 2, 4?* Check *i* covers all positions with bit *i* set in their binary index. Position `2^i` is in exactly one group, so each check bit appears in only one equation and can be set independently — no simultaneous equations (§3.4 step 5).
*Why syndrome = position?* Error at position `p` flips exactly those checks whose bit is set in `p`; the pattern of failing checks *is* the binary number `p` (§3.4 step 4).
*k = 11:* smallest `r` with `2^r ≥ 11 + r + 1`: r=3 → 8 ≥ 15 ✗; r=4 → 16 ≥ 16 ✓ → **4 check bits** (Hamming(15,11)). *k = 26:* r=4 → 16 ≥ 31 ✗; r=5 → 32 ≥ 32 ✓ → **5** (Hamming(31,26)).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Just add one check bit per data bit pair, so 2." | Parity bits are cheap, so use few. | 2 checks give 4 syndromes < 8 needed (no error + 7 positions). The count comes from needing `n + 1` distinguishable syndromes. |
| "k=11 needs 3 check bits, as k=4 did." | Pattern-matching on the previous case. | `2³ = 8 < 11 + 3 + 1`; the condition `2^r ≥ k + r + 1` needs r = 4 (n = 15 = 2⁴ − 1). |

---

## Q9 — Classifying extended-Hamming words

| Word | low 7 bits | syndrome s | overall parity P (of 8 bits) | Verdict | Data |
|---|---|---|---|---|---|
| `0x55` | `0x55` | 0 | weight 4 → 0 | **Ok** | `0xB` |
| `0xD5` | `0x55` | 0 | weight 5 → 1 | **Corrected** (the parity bit itself was flipped) | `0xB` |
| `0x45` | `0x45` | 5 (position 5 cleared) | weight 3 → 1 | **Corrected** (position 5) | `0xB` |
| `0x56` | `0x56` | 3 | weight 4 → 0 | **Uncorrectable** (two errors) | (don't trust) |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `0xD5` "uncorrectable because s = 0 but word ≠ valid". | A mismatch looks fatal. | `s = 0, P = 1` is a *single* error in the overall parity bit: the data is fine. |
| `0x56` "corrected at position 3". | That's what plain Hamming would do. | P is even ⇒ even error count ⇒ do not correct (§3.5). |

---

## Q10 — Golay(23,12,7)

**(a)** Ball volume `1 + 23 + 253 + 1771 = 2048 = 2¹¹`; times `2¹²` codewords = `2²³` = every 23-bit word ⇒ **perfect** (§3.6). The generator is `0xC75`; codeword weights start at 7 ⇒ `d = 7`.
**(b)** Four errors: the received word is at distance 4 from the sent codeword, but because the balls tile the space it is within 3 of **another** codeword, which the decoder returns (reporting `errors ≤ 3`). Confidently wrong; the (23,12) form cannot detect it.
**(c)** Extended Golay `(24,12,8)`: add an overall parity bit; `d = 8`; correct 3 and **detect** 4 (`3 + 4 + 1 = 8`); a 4-error word is now equidistant (distance 4) from several codewords, so there's a tie to report rather than a silent miscorrection.
**(d)** `t = 3`; detect-only up to 6.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Golay corrects 3 and detects up to 6 simultaneously." | `d − 1 = 6`. | Correcting 3 uses up distance: `3 + s + 1 ≤ 7 ⇒ s ≤ 3`. And in the *perfect* (23,12) code, no word is "unclaimed", so nothing is detected. |
| "4 errors → decoder returns error." | Expect a failure flag. | No gaps in a perfect code. |

---

## Q11 — Spheres for d = 4 and d = 5

- **d = 5 (t = 2):** codewords 5 apart; radius-2 balls (2 + 2 = 4 < 5) are disjoint: any word within 2 of a codeword is unambiguous. A word at distance 3 from one codeword is at distance 2 from nothing → either detected or (for a wide gap) near a different codeword.
- **d = 4:** radius-1 balls are disjoint (1 + 1 = 2 < 4) with a **gap** between them: a word at distance 2 from one codeword may be at distance 2 from another (a *tie*): the decoder knows it is not in any radius-1 ball ⇒ **detects**. So d=4 corrects 1 *and* detects 2.
- **d = 3:** radius-1 balls are disjoint but *touch* (1 + 1 + 1 = 3): a 2-error word lies within 1 of another codeword ⇒ the decoder "corrects" it wrongly.
- **Rule `d ≥ t + s + 1`:** an `s`-error word from codeword A must stay farther than `t` from every other codeword B: `d(A,B) − s > t`. 

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "d = 4 corrects 2 errors." | 4/2 = 2. | A 2-error word at the midpoint is a tie; only 1 error is *always* correctable. |

---

## Q12 — Find the bug

Masks `0x55 / 0x66 / 0x78` are **correct** (groups {1,3,5,7}, {2,3,6,7}, {4,5,6,7} → bits {0,2,4,6}, {1,2,5,6}, {3,4,5,6}). The bugs:

1. **(a)** `syn = s1 + s2 + s4` *adds* the check results; it must **weight them by position**: `syn = s1 | (s2 << 1) | (s4 << 2)`. (With addition, errors at positions 1, 2, 4 all give `syn = 1`.)
2. **(b)** `1u << syn` — position `syn` is **bit `syn − 1`**: off by one.
3. **(b')** It is applied unconditionally. When `syn == 0` (no error), `1u << (0−1)` is undefined (or, with the buggy shift, flips bit 0), *corrupting a good word*. Guard with `if (syn != 0)`.
4. **(c)** `cw & 0x0F` — the data are at positions 3,5,6,7 = bits 2,4,5,6, not bits 0–3. Extract: `((cw>>2)&1) | (((cw>>4)&1)<<1) | (((cw>>5)&1)<<2) | (((cw>>6)&1)<<3)`.
5. **(c')** `corrected = true` always; should be `syn != 0`.
6. Input hygiene: `cw` should be masked to 7 bits before decoding (`cw &= 0x7F`) if the caller might pass 8 bits.

```cpp
Hamming74Result decode(uint8_t cw) {
  cw &= 0x7F;
  unsigned s = parity(cw & 0x55) | (parity(cw & 0x66) << 1) | (parity(cw & 0x78) << 2);
  bool fixed = false;
  if (s != 0) { cw ^= uint8_t(1u << (s - 1)); fixed = true; }
  uint8_t data = ((cw >> 2) & 1) | (((cw >> 4) & 1) << 1) | (((cw >> 5) & 1) << 2) | (((cw >> 6) & 1) << 3);
  return { data, uint8_t(s), fixed };
}
```

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The masks are wrong." | Hex masks look arbitrary. | They are the groups of §3.4 written as bits; derive, don't guess. |
| "Only the shift is off." | It's the visible one. | At least four independent bugs. |

---

## Q13 — Implement

Check `code/solutions/blockfec.h` for the reference. Order of attack, each guarded by a test: `parity32` (XOR fold) → `hamming_distance` (`popcount(a^b)` / Kernighan) → `min_distance` (all pairs) → `hamming74_encode/decode` → `hamming84_*` (decode order: syndrome first, then overall parity; `s != 0 && P == 0` ⇒ Uncorrectable) → `golay23_encode` (long division by `0xC75`, same loop as Pack 04) → `golay23_decode` (nearest of 4096) → `rs5_encode/recover` (line through two points mod 5; modular inverses `{1,3,2,4}`).
Common pitfalls: position vs bit index (off by one); decoding on the unmasked byte; forgetting `s == 0 && P == 1`; negative modulo in `rs5_recover` (use `((x % 5) + 5) % 5`).
Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build` — expect `solution.*` green first, then `student.*` as you fill the stubs.

---

## Q14 — Connections

**(a) Golay/BCH ↔ CRC (Pack 04):** all three are *cyclic codes*: codeword = `(data << r) | ((data << r) mod g(x))` in GF(2). A CRC uses a long `g` for **detection** (the syndrome is compared to zero); Golay/BCH choose `g` with large `d` and use a syndrome → error-pattern table to **correct**. The CRC register *is* the LFSR from Pack 06 §3.5.
**(b) Interleaving (Pack 06):** block codes correct *few errors per codeword*. A burst of `B` consecutive bit errors would exceed `t`. Interleaving reorders bits across `rows` codewords so that a burst hits each codeword once or twice — within `t`. Cost: delay (must buffer the whole block).
**(c) After the NAC rewrite (Pack 01):** the NID's BCH check bits (+ parity) are computed over the NAC and DUID; changing the NAC without recomputing makes the frame invalid — a receiver would either reject, or **"correct" the edit away**: old NAC `0x293` vs new `0xF7E` differ in `0x293 ^ 0xF7E = 0xDED` → 9 bits, and 9 ≤ t = 11, so the stale check bits make the decoder restore the *old* NAC. So recompute the BCH encoding.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Golay and CRC are unrelated: different polynomial families." | Different jobs. | Same algebra; same shift-and-XOR division loop. |
| "Interleaving adds redundancy." | It's applied alongside FEC. | It adds no bits; it only changes the order. |

---

## Q15 — Interview: Hamming(7,4)

Say: "Positions 1..7; check bits at powers of two; check *i* covers positions whose index has bit *i* set. Encode: place data in 3,5,6,7, compute the three parities to make each group even. Decode: recompute the three checks; read them as a 3-bit number — that's the position of the flipped bit (0 = none); flip it. It works because a flip at `p` fails exactly the checks whose bit is set in `p`. Limits: `d = 3`, one error per 7 bits; with two errors it miscorrects silently; bursts need interleaving; fix with an overall parity bit (SECDED) to detect two." Then write the 10-line code of Q12's fix.
Follow-ups: rate 4/7; extension to Hamming(15,11); alternatives (Golay, BCH, RS).

---

## Q16 — Interview: 16-bit control word, 3 errors, 32-bit budget

**Feasibility (sphere-packing).** With `k = 16`, `t = 3`: need `2¹⁶ · V(n,3) ≤ 2ⁿ`. For `n = 24`: `V = 1 + 24 + 276 + 2024 = 2325`, `65536 · 2325 ≈ 1.5·10⁸ > 2²⁴ ≈ 1.7·10⁷` ⇒ **impossible at 24 bits**. For `n = 31`: `V = 1 + 31 + 465 + 4495 = 4992`, `65536 · 4992 ≈ 3.3·10⁸ ≤ 2³¹ ≈ 2.1·10⁹` ✓ — and **BCH(31,16,7)** exists (d = 7 ⇒ t = 3). 31 ≤ 32 ✓; the spare bit can be an overall parity (d=8, correct 3 detect 4).
**Not being fooled:** BCH(31,16,7) is not perfect, so many (not all) ≥ 4-error words are flagged uncorrectable; to catch the rest add an outer CRC (Pack 04) over the whole message.
**FEC vs ARQ:** FEC costs constant overhead but needs no return channel and has bounded latency (broadcast voice/PTT); ARQ costs nothing when the channel is clean but needs an ACK path and unbounded delay; hybrid (HARQ) = light FEC + retransmit.
(Verify BCH(31,16,7) parameters, **VERIFY**.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use Golay(24,12,8) in 24 bits." | Golay is famous for t = 3. | It protects 12 bits, not 16; 16 bits need two words (48 bits) or a different code. |
| "Hamming(31,26) — cheapest overhead." | Few check bits. | `t = 1`; won't survive 3 errors. |

---

## Rapid fire
1. d=9 → `t = ⌊8/2⌋ = 4`.
2. 4 check bits (Hamming(15,11)).
3. 4/7 ≈ 0.571.
4. Flip at position 5 → checks 1 and 4 fail → `101` = 5.
5. RS(255,223): `n − k = 32` check symbols → corrects up to **16 symbol errors** (or 32 erasures).
