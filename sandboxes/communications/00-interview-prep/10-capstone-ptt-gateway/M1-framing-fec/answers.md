# Capstone M1 — Answers (framing, CRC, FEC)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Toy format, synthetic data only. **VERIFY** = check against the standard/catalogue before quoting.

For the **Story** questions (Q9–Q13) there is no single right answer: a *model answer* is given with the points a strong answer must hit. Rewrite it in your own words with your own numbers.

---

## Q1 — Frame bit budget

**Answer.** Sync 24 + header 46 + body 2268 = **2338 bits**. Body = (160 + 2) bytes × 2 nibbles × 7 bits = 162 × 14 = 2268, i.e. **324 Hamming words** (162 bytes → 324 nibbles → one 7-bit word each). Efficiency `1280 / 2338 = 54.7 %`. Raw rate `2338 / 0.020 = 116.9 kbit/s`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Body = 162 × 8 = 1296 | Forgot the FEC expansion. | Each nibble becomes 7 bits: ×7/4. |
| Header = 24 bits | That's the information. | Two Golay(23,12) words = 46 bits on air. |
| 160/2338 "= 6.8 %" | Divided bytes by bits. | Convert to bits first: 1280/2338. |

**Etherstack angle.** Quote overhead honestly: 45 % of the air bits are not voice. That is the price of robustness and is the lever you tune per channel.

## Q2 — CRC by hand, and init = 0xFFFF

**Answer.** `0xFFFF ^ (0x41 << 8) = 0xFFFF ^ 0x4100 = 0xBEFF`. Step 0: top bit 1 → `(0xBEFF << 1) & 0xFFFF = 0x7DFE`, `^ 0x1021 = 0x6DDF`. Step 1: top bit of `0x6DDF` is 0 → shift only → `0xDBBE`. (Six more steps remain.)
Init `0xFFFF`: with init 0 the register stays 0 while leading zero bytes are processed, so `00 00 41` and `41` produce the same CRC — the CRC cannot see leading zeros. Non-zero init makes them matter.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "XOR the byte into the low byte" | Looks natural. | This (non-reflected, MSB-first) variant XORs into the **high** byte. |
| "XOR 0x1021 on every step" | Simplest loop. | Only when the top bit is 1. |

## Q3 — "CRC corrects 16 errors"

**Answer.** A CRC **detects**, never corrects: it reveals only that the remainder mismatches, with no location information. What it guarantees: **every burst error up to 16 bits long is detected** (a degree-16 polynomial can't divide a burst of length ≤ 16 unless it's all zero), plus all odd-weight errors for this generator. Longer or random corruption is missed with probability ≈ 2^-16.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Detects up to 16 flipped bits anywhere" | 16-bit CRC. | The guarantee is for a *burst* of length ≤ 16; scattered multi-bit patterns have only the HD-based guarantee (HD = 4 for our length; VERIFY). |

## Q4 — Encode Hamming(7,4), nibble 0xC

**Answer.** `0xC = 1100` → bit0=0, bit1=0, bit2=1, bit3=1 → d1=0, d2=0, d3=1, d4=1. `p1 = d1^d2^d4 = 1`; `p2 = d1^d3^d4 = 0`; `p4 = d2^d3^d4 = 0`. Positions 1..7 = `p1 p2 d1 p4 d2 d3 d4 = 1 0 0 0 0 1 1` → bits 0, 5, 6 set → `1 + 32 + 64 = 0x61`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `0xC` with parity appended in the high bits | "Systematic" intuition. | Parity bits are at positions 1, 2, 4 so that the syndrome equals the error position. |

## Q5 — Decode a single error

**Answer.** `0x45 = 0100 0101` → bits 0, 2, 6 set → positions 1, 3, 7 set. Check p1 (positions 1,3,5,7): `1^1^0^1 = 1` (fail). Check p2 (2,3,6,7): `0^1^0^1 = 0` (ok). Check p4 (4,5,6,7): `0^0^0^1 = 1` (fail). Syndrome `p4 p2 p1 = 101 = 5` → flip position 5 (bit 4): `0x45 ^ 0x10 = 0x55` → nibble `0xB`. Correct.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Syndrome 3 (adds the failing checks' counts) | Counted failures. | Syndrome is the *binary number* formed by the checks: p1=1, p4=4. |

## Q6 — Two errors in one Hamming word

**Answer.** `0x56`: checks fail at p1 and p2 → syndrome 3 → flips position 3 (d1) → `0x52` → nibble **`0xA`** (true nibble `0xB`). `corrected = true`, so the decoder believes it succeeded. The **CRC-16** catches it (status `CrcFail`; test `TwoErrorsInOneHammingWordAreCaughtByTheCrc`).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Hamming detects 2 errors" | Heard of SECDED. | Plain Hamming(7,4) has distance 3: 1 correct **or** 2 detect, not both; the decoder always corrects. Extended Hamming (8,4) adds an overall parity bit for double detection. |

## Q7 — Why Golay is perfect

**Answer.** Spheres of radius 3 around each of the `2^12` codewords contain `1 + C(23,1) + C(23,2) + C(23,3) = 1 + 23 + 253 + 1771 = 2048 = 2^11` words, and `2^12 × 2^11 = 2^23`: they cover every 23-bit word exactly once. So every received word is within distance 3 of exactly one codeword; the decoder never fails to return something. With 4 errors the received word lies in **another** codeword's sphere → confident wrong data (test `FourErrorsMiscorrectSilently`). The only safeguard is a layer above.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Perfect = corrects everything" | Word meaning. | Perfect = no gaps between spheres; there is no "uncorrectable" signal at all. |

## Q8 — Probabilities at BER 10^-3

**Answer.** (a) `0.999^24 = 0.9763` ⇒ 2.4 % of frames would have a damaged header with no protection. (b) `≈ C(23,4)·p^4 = 8855 × 10^-12 ≈ 8.9×10^-9` per word (two words ≈ 1.8×10^-8). (c) `≈ C(7,2)·p^2 = 21 × 10^-6 = 2.1×10^-5` per word. (d) 324 words × 2.1×10^-5 = **6.8×10^-3** ⇒ ≈ 0.68 % of frames end in CrcFail (so ≈ 99 % are Ok; the test requires ≥ 90 %). Silent-wrong rate ≈ 6.8×10^-3 × 2^-16 ≈ 10^-7 per frame or lower.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Header fail = 2.4×10^-2 with Golay | Reused (a). | Golay needs ≥ 4 errors in a word to fail, not ≥ 1. |
| Use 7 × 10^-3 per word | Mixed up p. | Two errors needed: `p^2`, multiplied by the 21 pairs. |

## Q9 — Story: why stronger FEC on the header

**Model answer — points a strong story hits:**
1. **Consequence asymmetry.** A wrong header = wrong talkgroup (audio to the wrong people — in public safety, a serious incident), wrong sequence number (jitter buffer/duplicates confusion), wrong type (an end-of-call missed or invented). One damaged payload bit = a click in 1/8000 s of audio.
2. **Cost asymmetry.** Header is 24 information bits; Golay adds 22 bits (< 1 % of the frame). Upgrading the 2268-bit body to Golay-class protection would cost roughly 1000+ extra bits per frame.
3. **Numbers.** BER 10^-3: unprotected header fails 2.4 % of frames; Golay: ~10^-8.
4. **Defence in depth.** The CRC covers header + payload, so even a Golay miscorrection (≥ 4 errors) cannot silently deliver a wrong header.
5. **What I'd change.** Interleave the body, use a stronger code (convolutional/LDPC), and consider repeating the header across frames (late entry).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Headers are more important, period." | True but empty. | Needs the cost/consequence arithmetic and the CRC backstop. |
| "Use Golay everywhere" | Strongest = best. | Rate 0.52 on all data costs capacity/battery; voice tolerates a few errors. |

## Q10 — Story: why CRC-16 over header + payload

**Model answer.** (1) *Target:* residual undetected error ≈ 2^-16 × (fraction of frames with uncorrected errors ≈ 0.7 %) ≈ 10^-7 per frame; at 50 frames/s that is about one undetected wrong frame per 2 days of continuous traffic… and since a wrong voice frame is a click, acceptable. (2) *8-bit:* 2^-8 ≈ 0.4 % undetected is too weak for a header. *32-bit:* +2 bytes/frame (≈ +0.7 % overhead), needed for long data packets, overkill for 160 B. (3) *Covers header:* otherwise a Golay miscorrection yields a valid-looking wrong header. (4) *Why detection and FEC together:* FEC repairs, CRC proves the result; a perfect-code decoder can't tell you it failed. VERIFY: calculate the "per 2 days" figure for your own loss assumptions (50 frames/s × 86 400 × 2 = 8.6×10^6 frames).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "A parity/checksum is enough" | Simple. | Sum-style checksums miss swapped bytes and many multi-bit patterns; CRC has proven burst guarantees. |

## Q11 — Story: Golay + Hamming vs production

**Model answer.** Rate: Golay 12/23 = 0.52, Hamming 4/7 = 0.57 — both costly for their power. Hamming(7,4) corrects 1 in 7 bits; real radio errors come in **bursts** (fades), where one burst puts ≥ 2 errors in a word and defeats it → solution: **interleaving** (spread adjacent bits across many codewords) so a burst becomes scattered single errors. **Soft-decision** decoding (using demodulator confidence, not hard bits) gains roughly 2 dB over hard-decision (VERIFY figure). Production: convolutional + Viterbi, turbo, LDPC/polar (5G NR, M3 and 06/03 pack) with interleaver and rate matching. Public-safety standards use specific mixes of Golay/Hamming/convolutional/trellis coding (VERIFY per standard, do not assert). Why I chose these: they're small enough to implement and test exhaustively and they expose the key lesson (FEC limits and silent miscorrection).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Hamming is fine for fading channels" | Passes the unit test. | Test errors are independent; real fades are bursts. |

## Q12 — Story: sync word and tolerance

**Model answer.** *Sync word:* the receiver needs a pattern whose correlation with every shifted copy of itself and with random data is small, so the peak is unambiguous: exhaustive search shows worst aperiodic sidelobe 3 vs peak 24 (a 24-bit word cannot do much better; VERIFY by search/literature). *Tolerance 3:* lets a frame start be found when the channel flips up to 3 sync bits (at BER 10^-3 sync damage is rare; at worse BER it is common); false alarm per offset `2325 / 2^24 = 1.39×10^-4`; over 2338 offsets ≈ 0.32 candidates per frame-length of noise. Mitigations: choose the **minimum-mismatch** offset (true sync usually has ≈ 0), then Golay + CRC reject false frames cheaply; tolerance 0 would drop every frame with 1 sync bit error. *Trade:* higher tolerance ⇒ more false candidates to process (CPU), lower ⇒ lost frames. Tune from BER sweeps (M3).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use the longest sync possible" | More bits = more certainty. | Overhead per frame and per-bit tolerance matter more; 24 bits already gives 2^-24 exact-match false alarms. |

## Q13 — Story: frame field choices

**Model answer.** *20 ms:* matches a typical speech-codec frame and the RTP packetisation of M2; short enough for low latency, long enough to amortise 70 bits of sync + header (a 5 ms frame would double the overhead share). *160 B:* 20 ms × 8000 samples/s × 1 B μ-law; a stand-in for vocoder bits. *12-bit seq:* 4096 frames × 20 ms = **81.92 s** before wrap: far longer than any plausible reorder or loss window (≪ 1 s), and cheap; use RFC 1982 serial arithmetic for comparison. Also late-entry: seq lets the receiver detect lost frames. *2-bit version:* lets a decoder reject a format it doesn't understand (our parser returns BadHeader when version ≠ 1). Removing it later is expensive: you can't distinguish old and new frames, and a firmware roll-out with mixed versions silently misparses. Also reserve a type code (3) for extension. *2-bit type:* voice / end-of-call (clean teardown without timeout) / data / reserved.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "No version field, we'll never change it" | Saves 2 bits. | Every protocol changes; field-deployed radios live for a decade. |

## Q14 — Order of checks

**Answer.** Checking CRC first needs a payload: with fewer than 2338 bits you read past the end of the vector (**out-of-bounds, ASan failure/crash** — `Truncated` must be first); and without sync/header decode you don't know the header the CRC covers. Statistically, a noise burst would count as `CrcFail`, hiding whether the problem is framing (SyncFail) or channel BER; M4's per-cause counters become meaningless. Correct order: size → sync → header (Golay + version) → body decode → CRC, cheapest checks first and each failure class distinct.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Order doesn't matter, all fail" | Same result: not Ok. | Safety (OOB reads) and diagnostics (per-cause stats) both depend on it. |

## Q15 — Single-bit-flip sweep

**Answer.** For positions **24–2337** (header and body, 2314 bits), exactly **one** correction is reported (Golay fixes a header bit and counts 1 error; Hamming fixes a body bit in exactly one word). For the 24 sync bits (0–23) the frame is Ok with **zero** corrections: the sync tolerance absorbed it before any decoder ran. It's not a "correction" because nothing is repaired: sync is used for alignment, not stored data.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Sync flip counts as 1 correction" | All flips are repaired. | Counters track FEC repairs; sync mismatch is a separate quality metric (could be exported later). |

## Q16 — What a real air interface adds

**Answer (examples; VERIFY per standard).** **Vocoder** (speech at a few kbit/s, not 64); **interleaving** (burst → scattered errors); **scrambling/whitening** (avoid long runs of 0/1 for clock recovery, spectral lines); **link-control / embedded signalling** (late entry, emergency flag, source ID) spread across frames; **encryption + key ID**; frame types for data and control channels; **superframes/slot structure** (TDMA); **stronger FEC** (convolutional/trellis). Testing without live traffic: **synthetic frames** from this builder through the M3 channel simulator (AWGN, CFO, delay) with known seeds; golden vectors; fuzz the parser; no antenna involved.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Record live traffic to get realistic frames" | Realistic. | Illegal or restricted in many places (UK WT Act 2006), unnecessary and non-reproducible. |
