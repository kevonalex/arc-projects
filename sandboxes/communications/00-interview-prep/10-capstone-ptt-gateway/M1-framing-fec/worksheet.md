# Capstone M1 — Framing, CRC and FEC (the bits on the air)

**Tier:** Capstone, milestone 1 of 4. **Time:** 4–6 h. **Code:** `code/` (stubs `include/framing.hpp`, reference `solutions/framing.hpp`, tests `tests/test_framing.cpp`).
**Safety/legal:** everything here is a **toy over-the-air format** fed with **synthetic data**. It is not P25/DMR/TETRA. Never receive or decode live public-safety traffic (in the UK that also runs into the Wireless Telegraphy Act 2006).

---

## §1 Why this milestone exists

The capstone is a miniature **PTT radio-to-IP gateway**. The whole vision (each milestone builds one box):

```
WAV in -> 20 ms frames -> [M1] radio frame: sync | header (CRC, Golay) | payload (Hamming) | CRC-16
       -> [M3] 4FSK modulator -> AWGN + CFO channel -> sync / demod / decode
       -> [M1] parse + repair -> [M2] mu-law RTP over UDP multicast -> jitter buffer -> WAV out
       -> [M4] gRPC StartCall / EndCall / streaming stats (BER, loss, jitter) + floor control
```

M1 is the **bit-level contract** between a radio and everything behind it. Every later milestone either produces or consumes the 2338-bit frame you build here. It also drills the three answers an Etherstack interviewer actually asks about error control: *what does the CRC guarantee, what does the FEC guarantee, and what happens when they are exceeded?*

By the end you can: implement CRC-16/CCITT, Golay(23,12) and Hamming(7,4) from first principles, assemble and parse a frame with ordered failure statuses, explain **why headers get stronger FEC than payload**, and defend each constant in the format as a design decision.

## §2 Prerequisites

* Binary/hex, XOR, shift. Polynomials over GF(2) (just "XOR instead of subtract"). Pack 05 (DSP) helps for later milestones, not for this one.
* Build: `cmake -S code -B build -DCMAKE_CXX_FLAGS="-Wall -Wextra -fsanitize=address,undefined"` then `cmake --build build && ctest --test-dir build`.
  Student tests (`student.*`) **must fail** until you finish; `solution.*` must pass.

## §3 Concept build-up

### §3.1 The frame, concretely, before any theory (W 3.5)

One 20 ms voice frame is exactly **2338 bits**, MSB first, one bit per vector element:

| Field | Bits | Contents |
|---|---|---|
| Sync | 24 | `0x02B31E` — lets the receiver find the frame start |
| Header | 46 | two Golay(23,12) words = 24 header bits (version 2, type 2, talkgroup 8, seq 12) |
| Body | 2268 | (160 B payload + 2 B CRC-16) = 162 B = 324 nibbles → 324 Hamming(7,4) words × 7 bits |

Arithmetic worth knowing cold: `24 + 46 + 2268 = 2338`; payload information bits `160 × 8 = 1280` → **efficiency 1280 / 2338 = 54.7 %**; raw rate `2338 bits / 20 ms = 116.9 kbit/s`.

Honesty note: the 160 B payload is 20 ms of 8 kHz μ-law (64 kbit/s), a *stand-in* for a vocoder frame. Real narrowband systems compress speech to a few kbit/s with a vocoder (IMBE/AMBE-class; VERIFY) and add FEC on top. The framing logic is identical; the numbers are not.

### §3.2 CRC-16/CCITT-FALSE — detection (W 3.1)

Treat the message as a big polynomial over GF(2), divide by the generator `x^16 + x^12 + x^5 + 1` (`0x1021`), and send the remainder. The receiver recomputes. Parameters here: **init `0xFFFF`, no reflection, no final XOR**; check value `crc("123456789") = 0x29B1`.

Worked example (hand-checkable, matches the test `crc(0x00) = 0xE1F0`). State = `0xFFFF`; XOR the byte into the **high** byte (`0xFFFF ^ 0x0000 = 0xFFFF`); then 8 times: *if the top bit is 1, shift left and XOR `0x1021`; else just shift left* (always keep 16 bits).

| Step | Top bit | Action | State |
|---|---|---|---|
| start | – | – | `0xFFFF` |
| 0 | 1 | shift + xor | `0xEFDF` |
| 1 | 1 | shift + xor | `0xCF9F` |
| 2 | 1 | shift + xor | `0x8F1F` |
| 3 | 1 | shift + xor | `0x0E1F` |
| 4 | 0 | shift | `0x1C3E` |
| 5 | 0 | shift | `0x387C` |
| 6 | 0 | shift | `0x70F8` |
| 7 | 0 | shift | `0xE1F0` |

Why **init `0xFFFF`**: with init 0, a frame that starts with zero bytes has the same CRC regardless of how many leading zeros it has — the CRC is blind to them. A non-zero init makes leading zeros matter.

Guarantees (properties of the polynomial; VERIFY the exact message-length limits in a CRC catalogue): detects **all burst errors ≤ 16 bits**, **all odd-weight errors** (the polynomial has an `x+1` factor), and all 1-, 2- and 3-bit errors for messages of our length (1304 bits); a random corruption slips through with probability about `2^-16 = 1.5×10^-5`.

> **Misconception box.** *"A CRC corrects errors."* No. A CRC only **detects**: it says "something is wrong", not where. Correction is FEC's job. *"A CRC makes data secure."* No: it is linear and public; anyone can recompute it. It protects against noise, not an adversary.

### §3.3 Golay(23,12,7) — strong FEC for short, critical data (W 3.2)

A **block code** maps 12 data bits to a 23-bit codeword. "(23,12,7)": minimum distance **7** between any two codewords, so it corrects **t = 3** errors (`⌊(7−1)/2⌋`) or detects 6.

Encoding (systematic): `codeword = (d << 11) | ((d << 11) mod g)`, generator `g = 0xC75` = `x^11 + x^10 + x^6 + x^5 + x^4 + x^2 + 1`. Concrete: `d = 1` → `0x800 mod 0xC75 = 0x475` → codeword `0xC75` (the test). `d = 0xABC` → `0x55E11E`. For our header word `0x6A59AB` the two 12-bit halves `0x6A5` and `0x9AB` encode to `0x352F1C` and `0x4D5962`.

Decoding (brute force, fine for 4096 codewords): pick the codeword at the smallest Hamming distance from the received 23 bits.

The sphere-packing fact: the number of patterns within 3 bits is `1 + 23 + 253 + 1771 = 2048 = 2^11 = 2^23 / 2^12`. These spheres tile **every** possible 23-bit word — the code is **perfect**. Consequence: the decoder *always* finds a codeword within distance 3. Four or more errors never produce "I give up"; they produce a **confident wrong answer** (test `FourErrorsMiscorrectSilently`).

> **Misconception box.** *"A perfect code is the strongest code."* Perfect means no wasted space, not "can't fail". It means it can't *tell* you it failed. That is exactly why a CRC sits on top.

### §3.4 Hamming(7,4) — cheap FEC for tolerable errors (W 3.3)

Seven bits carry four data bits; three parity bits each check a subset of positions. Layout (1-based positions, position k = bit k−1): `p1 p2 d1 p4 d2 d3 d4`, with parity checks:

* `p1` covers positions 1, 3, 5, 7. `p2` covers 2, 3, 6, 7. `p4` covers 4, 5, 6, 7.

The trick: position number written in binary is **which checks include it** (pos 5 = 101 → checks p1 and p4). So the three parity checks recomputed at the receiver form a **syndrome** whose value *is the position of a single error*.

Worked example: nibble `0xB` = `1011` → bit0..3 = d1=1, d2=1, d3=0, d4=1. `p1 = d1^d2^d4 = 1`, `p2 = d1^d3^d4 = 0`, `p4 = d2^d3^d4 = 0`. Positions 1..7 = `1 0 1 0 1 0 1` → bits 0,2,4,6 → `0x55` (the test). Also `0x6 → 0x33`.

Distance 3 ⇒ corrects **1** error per 7-bit word. It is perfect too (`1 + 7 = 8 = 2^3`). Two errors in a word are **miscorrected**: `0x55` with positions 1 and 2 flipped is `0x56`, whose syndrome is `1 xor 2 = 3`, so the decoder "fixes" position 3 and returns `0xA` instead of `0xB`, claiming success (test `TwoErrorsMiscorrect`).

> **Misconception box.** *"Corrected = fine."* A `corrected` flag only says the decoder changed something; if the error count exceeded its power, the "correction" made things worse. Only an independent check (the CRC) tells you the final answer is right.

### §3.5 Why the CRC covers the header too, and the check order (W 3.4–3.5)

If the CRC covered only the payload, four errors in a Golay header word would be silently "corrected" into a *different valid header* (wrong talkgroup or sequence) and the payload CRC would still pass. So `frame_crc(h, payload)` runs over the 3 header bytes (big-endian 24-bit word) **then** the payload. The parser's order of checks is deliberate and each has its own status:

1. enough bits? else **Truncated**; 2. sync within tolerance? else **SyncFail**; 3. Golay-decode both header words; version must be 1, else **BadHeader**; 4. Hamming-decode the body; 5. CRC over header+payload, else **CrcFail**.

Cheap checks first; the expensive and most informative check (CRC) last, and each failure reports *why*, which M4 turns into stats.

### §3.6 Sync words and sync search (W 3.6)

The receiver sees a stream with unknown frame boundaries. It slides the 24-bit sync word across and picks the offset with the **fewest mismatches**, provided that is ≤ a tolerance (we use 3; first offset wins ties). The sync word is chosen to have **low aperiodic autocorrelation sidelobes**: when the word is slid over itself, every off-peak overlap should look like noise. For `0x02B31E` the worst sidelobe magnitude is 3 (peak 24; test `AutocorrelationSidelobesAreSmall`).

The tolerance is a trade: more tolerance finds frames in noise, but also finds false ones. On random data one offset matches within 3 mismatches with probability `(1 + 24 + 276 + 2024) / 2^24 = 2325 / 16 777 216 = 1.39×10^-4`. Over the 2338 offsets of one frame-length of random bits, that is about **0.32 false candidates**. Hence: take the *best* match, and let the header decode and CRC reject the rest.

### §3.7 Misconceptions summarised

| Claim | Reality |
|---|---|
| FEC and CRC do the same job | FEC repairs up to t errors; CRC detects what's left. You need both. |
| Golay fixes any 3 errors in the header | 3 **per 23-bit word**. 4 in one word miscorrects. |
| More parity is always better | Overhead is bandwidth/battery/latency; protect what costs most to lose. |
| A longer sync is always better | Longer = more overhead; sidelobes and tolerance matter more than raw length. |
| The decoder tells me when it failed | Perfect codes cannot. Only the CRC can. |

## §4 Worked examples

**(a) One flipped body bit.** Hamming syndrome = position of the flip → flip back → nibble restored; `payload_corrections = 1`, CRC passes, status Ok.

**(b) Four adjacent errors in a Golay word** (bits 26–29). Golay returns a *different* valid 12-bit word with `errors ≤ 3`. The reassembled header is wrong, but the CRC (which covers the header) fails → `CrcFail` (or `BadHeader`) and the payload is **not** released. No silent wrong header.

**(c) BER 10^-3 expected counts.** 2338 bits → 2.34 raw errors per frame. Hamming word fails when ≥ 2 of its 7 bits are wrong: `C(7,2)·p^2 ≈ 2.1×10^-5` per word; 324 words → ≈ **6.8×10^-3** per frame (0.68 %). Golay word fails on ≥ 4 of 23: `C(23,4)·p^4 ≈ 8.9×10^-9`. So about 99 % of frames come out Ok, and (with CRC) about `10^-7` or fewer per frame are accepted wrong.

## §5 Questions

Do §5 on paper first. Design questions (marked **Story**) are written answers of 150–250 words: they become your interview stories. Use: *context → decision → alternatives rejected → numbers → what I'd change*.

**Q1 — Calculate.** Compute the frame bit budget (sync/header/body, total), the payload efficiency, and the raw bit rate. How many Hamming words are in a frame and why that many?
*Context: you will quote these numbers in every later milestone.*

**Q2 — Trace.** Run the CRC-16 algorithm by hand for the single byte `0x41` for the first two shift steps only (state after XOR with the byte, then after steps 0 and 1). Then explain why init = `0xFFFF` and not `0x0000`.
*Context: W 3.1.*

**Q3 — Explain.** A colleague says "the CRC corrects up to 16 bit errors". Correct them in three sentences, including what the CRC *can* guarantee for a 16-bit burst.
*Context: W 3.1.*

**Q4 — Calculate.** Hamming(7,4)-encode the nibble `0xC` by hand (show d1..d4, p1, p2, p4, final byte).
*Context: W 3.3.*

**Q5 — Trace.** The receiver gets `0x45`, which is `0x55` with one bit flipped. Compute the three parity checks and the syndrome and say which position is repaired.
*Context: W 3.3.*

**Q6 — Predict.** Positions 1 and 2 of codeword `0x55` are flipped (receive `0x56`). What nibble does the decoder return, what does `corrected` say, and which later layer catches the damage?
*Context: W 3.3, test `TwoErrorsMiscorrect`.*

**Q7 — Explain.** Why is Golay(23,12) "perfect"? Show the sphere-count arithmetic and say what that implies about what the decoder does with 4 errors.
*Context: W 3.2.*

**Q8 — Calculate.** At BER = 10^-3 (independent bits): (a) probability the 24 header bits are all correct if sent **unprotected**; (b) probability one Golay word fails; (c) probability a Hamming word fails; (d) expected CrcFail fraction per frame.
*Context: the numbers behind Q9.*

**Q9 — Story (design).** *Why does the header get stronger FEC (Golay, 3-error) than the payload (Hamming, 1-error)?* Discuss: consequence of each error type (misrouted call vs a click), overhead (22 extra header bits vs thousands in the body), where the CRC fits, and the cost if you simply used Golay for everything.

**Q10 — Story (design).** *Why CRC-16, and why over header + payload?* Cover: what undetected-error probability you accept, why not an 8-bit checksum or a 32-bit CRC, why it must cover the header, and why detection is paired with FEC rather than replacing it.

**Q11 — Story (design).** *Why Golay and Hamming here, and what would you use in a production radio?* Cover rate (12/23 vs 4/7), burst errors and **interleaving**, soft-decision decoding, convolutional/turbo/LDPC alternatives, and what existing public-safety standards use (VERIFY, do not assert specifics).

**Q12 — Story (design).** *Defend the sync word and tolerance of 3.* Include the false-alarm figure `1.39×10^-4` per offset, the 0.32 per frame length, the "best match then CRC" mitigation, and the sidelobe test.

**Q13 — Story (design).** *Defend the frame fields:* 20 ms frame, 160-byte payload, 12-bit sequence number (compute its wrap time), 2-bit version, 2-bit type. What breaks if the version field is removed in year 3 of a product?

**Q14 — Debug.** A teammate checks the CRC *before* the sync test and the `Truncated` check. Describe two concrete failure modes (crash and misleading statistics) and state the correct order with a reason for each step.
*Context: W 3.5.*

**Q15 — Predict.** The test flips each of the 2338 bits in turn. For which positions is `header_corrections + payload_corrections == 1`, and what happens for the 24 sync bits? Why is sync damage not counted as a correction?
*Context: test `EverySingleBitFlipAnywhereIsRepaired`.*

**Q16 — Etherstack.** What does a *real* narrowband radio air interface add that this toy omits (e.g. vocoder, interleaving, scrambling/whitening, link-control words, encryption/key ID, frame types)? Name what each solves. Then state, in one sentence, how you test a decoder without ever listening to live public-safety traffic.
*Context: interview framing; VERIFY any standard-specific claims.*

## §6 Self-assessment

You are done when you can, from memory and without notes:

* [ ] Recompute 2338, 54.7 %, 116.9 kbit/s, 324 Hamming words.
* [ ] Run CRC steps by hand and explain why init is `0xFFFF`.
* [ ] Encode a nibble in Hamming(7,4) and decode a syndrome.
* [ ] Explain "perfect code ⇒ silent miscorrection ⇒ CRC on top".
* [ ] Tell the "header vs payload FEC" story in 90 seconds with numbers.
* [ ] `ctest` shows all `solution.*` green, ASan/UBSan clean, with `student.*` red until you implement.

**VERIFY list (not tested here; confirm before quoting in an interview):**

1. CRC-CCITT's exact Hamming-distance/message-length limits and burst guarantees (CRC catalogue / Koopman tables).
2. Which FEC and frame structures P25, DMR and TETRA actually use (this format is a toy, not any of them).
3. Vocoder families and bit rates for narrowband radio (IMBE/AMBE-class).
4. Interleaving and soft-decision gain figures if you quote any.
5. The sync-word property "worst sidelobe 3" is verified by exhaustive search in this pack, but other word lengths are not.
