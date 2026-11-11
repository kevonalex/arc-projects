# Pack 03 — DMR (ETSI TS 102 361): 2-slot TDMA, 4FSK, the 264-bit burst

> **SAFETY / LEGAL.** All frames in this pack are **synthetic** — bits you create in unit tests. **Never**
> point anything from this pack at a live public-safety or commercial radio system, an SDR, or a recording of
> real traffic; it can be illegal (interception law) and encrypted/private calls are off-limits by design.
> Studying the *structure* of open-source decoders (OP25, dsd-fme, SDRTrunk) is optional; **do not copy their
> copyrighted code**.

Standards: ETSI TS 102 361-1 (DMR air-interface protocol), -2 (voice and generic services), -3 (data),
-4 (Tier III trunking). Annex/clause numbers are from memory: where a number or value is not certain it is
tagged **VERIFY** (and I never invent clause numbers — "clause VERIFY" means "go and look").

---

## 1. Why this matters (Etherstack context)

DMR (Digital Mobile Radio) is the world's most widely deployed digital LMR standard outside North America's
public-safety P25 base, built on **two-slot TDMA in 12.5 kHz**. Etherstack's **core business is LMR protocol
stacks** (P25, DMR, TETRA, dPMR… **VERIFY** the current product list): a DMR stack must parse the 264-bit burst
at line rate, run the TDMA slot clock, classify sync words, decode slot types and link control, assemble voice
superframes, and expose the result to call-control and to gateways (e.g., DMR↔P25 or DMR↔broadband PTT).

If you can *derive* the burst layout (why sync in the middle? why 3 voice frames? why 6 bursts per superframe?)
you can learn any TDMA LMR protocol.

## 2. Prerequisites

- Pack 01 §3.1–3.2 (symbols, dibits, sync probability) and §3.3 (cyclic codes as polynomial division).
- Pack 02 §3.2 (TDMA basics; slot arithmetic).
- C++17, GoogleTest, Linux/macOS/WSL with `-Wall -Wextra -fsanitize=address,undefined`.

```bash
cd code
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure  # stubs: FAIL
cmake -S . -B build_sol -DLMR_USE_SOLUTIONS=ON && cmake --build build_sol -j && ctest --test-dir build_sol
```

## 3. Concept build-up

### 3.1 Channel, modulation, rates

- **12.5 kHz** channel. **4FSK** (4-level frequency shift keying) at **4800 symbols/s** ⇒ 2 bits/symbol ⇒
  **9600 bit/s gross**.
- Dibit→symbol mapping (VERIFY, TS 102 361-1): `01 → +3`, `00 → +1`, `10 → −1`, `11 → −3` (a Gray map again: neighbours
  differ by one bit; the same idea as P25 C4FM in pack 01 §3.1).
- **2-slot TDMA:** two logical channels ("timeslot 1" and "timeslot 2") alternate every **30 ms**. One 12.5 kHz
  carrier ⇒ **two simultaneous calls** (same goal as P25 Phase 2 — pack 02 — but reached with *unchanged* 4800 baud).

> **Misconception box — "DMR is just P25 Phase 2."** Both are 2-slot TDMA, 12.5 kHz, AMBE+2-class vocoders, but they
> differ in modulation (4FSK 4800 sym/s vs 6000 sym/s DQPSK/CPM), burst layout, signalling and standards body
> (ETSI vs TIA). They do **not** interoperate over the air.

### 3.2 Time: slot, burst and gap

Slot = 30 ms = 9600 × 0.030 = **288 bits**. The **burst** is **264 bits** (27.5 ms). The remaining
**24 bits (2.5 ms)** are *not* part of the burst: on a base-station downlink they carry the **CACH** (common
announcement channel: access-type/busy-idle info and short link control — VERIFY), on an uplink they act as a
**guard** so a late radio doesn't collide with the next slot.

> **Misconception box — "a DMR burst lasts 30 ms."** A *slot* is 30 ms; the *burst* in it is 27.5 ms.
> Using 30 ms × 9600 = 288 for the burst size is the classic off-by-24 bug.

*Why a guard at all?* Radios have different distances from the base; propagation (3.3 µs/km) and timing error
shift bursts. 24 bits = 2.5 ms of slack absorbs that and PA ramp-up/ramp-down (see pack 02 §3.5).

### 3.3 The burst: 108 + 48 + 108 = 264

```
| 108 bits payload | 48 bits MIDDLE | 108 bits payload |
```

**Data/control burst:** `[98 BPTC bits][10 slot-type bits][48 SYNC or EMB field][10 slot-type bits][98 BPTC bits]`
= 98 + 10 + 48 + 10 + 98 = **264** ✔. (Two halves of 98 = 196 BPTC bits; slot type 20 bits split around the middle.)

**Voice burst:** `[108 voice][48 SYNC or EMB+embedded][108 voice]` ⇒ 216 voice bits = **3 × 72-bit AMBE+2 frames**.

*Why put the sync in the **middle**?* (Derive.) A receiver estimates the channel (timing, fading, frequency offset)
from the known sync pattern and then applies that estimate to the rest of the burst. A sync at the *start* is
up to 264 bits (27.5 ms) from the last data bit; on a moving vehicle the channel changes in that time. With the
sync at the *centre* the farthest data bit is only 108 bits (54 symbols = 11.25 ms) from the edge of the sync,
so the estimate is, on average, twice as fresh. GSM uses the same "midamble" idea. Bonus: voice frame 2 *straddles* the middle, so a
mid-burst fade hurts only part of one frame.

**Sync words** (48 bits each; VERIFY every digit [TS 102 361-1]):

| Source | Voice | Data |
|---|---|---|
| Base station | `0x755FD7DF75F7` | `0xDFF57D75DF5D` |
| Mobile station | `0x7F7D5DD57DFD` | `0xD5D7F77FD757` |

The receiver doesn't just *find* a burst; the sync tells it **who sent it (BS/MS)** and **what it carries
(voice/data)** before any FEC. BS-voice and BS-data syncs differ in exactly **24 of 48 bits** (half), which
makes the classification very robust (tolerating several errors still leaves them unambiguous).

### 3.4 Voice: why 3 frames per burst, why 6 bursts per superframe

AMBE+2 frames are **72 bits per 20 ms** (3600 bit/s: 49 speech + 23 FEC bits; VERIFY split). With 2-slot TDMA, a
logical channel gets a burst every **2 × 30 = 60 ms**. To carry speech *continuously* the burst must hold
60 ms of speech = 60/20 = **3 frames** ✔ (3 × 72 = 216 ✔ matches the 216 voice bits). This is a *derived* design:
**frames per burst = slot repeat period ÷ vocoder frame period**. (Contrast P25 Phase 2, where a 4V burst
holds 4 frames, 80 ms, but the pattern includes non-voice slots — pack 02 §3.5.)

**Voice superframe = 6 bursts A–F = 6 × 60 ms = 360 ms** = 18 frames. Why 6? It's the smallest repeat unit that
fits the signalling payload: a **72-bit link control + 5-bit CRC = 77 bits**, FEC-expanded to **128 bits**, is
too big for one burst's 32-bit embedded field, so it is cut into **4 fragments of 32 bits** (bursts B, C, D, E);
A carries the sync, and F carries other embedded signalling (null/reverse channel; VERIFY). 1 + 4 + 1 = 6.

Burst **A** has the 48-bit **voice sync** in the middle. Bursts **B–F** have, instead, `EMB (8) + embedded (32) + EMB (8)`.
**EMB** = 16 bits = colour code (4) + PI (1, privacy indicator) + LCSS (2, "link control start/stop": single,
first, last, continuation fragment) + 9 parity bits from a quadratic-residue **QR(16,7,6)** code (4+1+2 = 7
info bits). (VERIFY.)

> **Late entry:** the embedded LC is repeated every superframe (360 ms). A radio that joins mid-call can recover
> talkgroup/source from it. With "must see the first fragment, then the rest", worst-case acquisition = **540 ms**
> (Q7), exactly like P25's LDU1/LDU2 repetition (pack 01 §3.7). Why repeat? Same reason: a radio that missed the
> *voice LC header* at the start must still be able to join.

### 3.5 Slot type and colour code (the 20-bit field)

Slot type = **4-bit colour code (CC)** + **4-bit data type**, 8 info bits → **20 bits** via a Golay code, split
10 + 10 around the middle (data bursts only; voice bursts use EMB).

**CC (0–15)** is DMR's "NAC": it separates co-channel systems. **It is not security** — there are only 16 values and it's sent in
clear. A receiver ignores bursts whose CC doesn't match.

Data types (VERIFY): 0 PI header, 1 voice LC header, 2 terminator with LC, 3 CSBK, 4 MBC header, 5 MBC
continuation, 6 data header, 7 rate-½ data, 8 rate-¾ data, 9 idle, 10 rate-1 data.

**Why a Golay code for 8 bits? Build it.** The binary Golay code (23,12) is **perfect**: the number of error
patterns of weight ≤ 3 in 23 bits is 1 + 23 + 253 + 1771 = **2048 = 2¹¹**, and there are exactly 2¹¹ = 2048 syndromes.
So every received word lies within distance 3 of exactly one codeword; the decoder is a 2048-entry table
indexed by the syndrome. DMR needs only 8 info bits, so it **shortens** the code (drops leading zero info bits) and
adds an overall parity bit: (23,12) → (19,8) → (20,8). Our teaching construction (the exact ETSI generator
matrix is **VERIFY** TS 102 361-1 Annex B): `word20 = (word19 << 1) | parity`. The result has minimum distance 8:
it **corrects 3 errors and detects 4**.

*A sobering number:* with ≤3-error decoding, **256 × 1351 = 345 856 of the 2²⁰ words (≈ 33 %) are accepted** from
pure noise. That is why one more filter (CC match: 1/16 ⇒ ≈ 2 %) and later CRCs are needed.

### 3.6 Data and link control: BPTC(196,96)

Full LC (72 bits) + 24 bits of Reed-Solomon(12,9) parity = **96 bits**; CSBK = 80 bits + 16-bit CRC = **96 bits** (VERIFY).
Both go through **BPTC(196,96)** (Block Product Turbo Code):

- Matrix: **9 rows × 11 columns = 99** info positions (96 data bits + 3 reserved R bits).
- Every **row** is a Hamming(15,11,3) word (+4 parity columns ⇒ 15 columns).
- Every **column** is a Hamming(13,9,3) word (+4 parity rows ⇒ 13 rows).
- 13 × 15 = **195** matrix bits + 1 leading reserved bit = **196** ✔; 196 = 98 + 98 = the two payload halves of a data burst.

*Why a product code?* Rows and columns give two independent chances to correct an error; the product of
two d = 3 codes has d = 3 × 3 = **9** (VERIFY decoder capability), and decoding is just "run the small Hamming decoders
on rows, then columns, repeat" — cheap, hardware friendly.

**Interleaving.** Matrix index *a* is transmitted at position **(a × 181) mod 196** (VERIFY direction [Annex B]). 181 is prime and
gcd(181,196) = 1 ⇒ it is a *permutation*. Neighbouring transmitted bits come from matrix cells 13 apart
(181⁻¹ ≡ 13 mod 196, since 181 × 13 = 2353 = 12 × 196 + 1). A short fade (burst error) therefore damages
*scattered* matrix cells, which a product code handles well, instead of a contiguous block that could kill a
row or column.

### 3.7 Why headers get stronger FEC than voice (again)

An LC/CSBK/slot-type error mis-routes or kills a *whole call or message*; a voice-bit error damages 20 ms that the
listener's ear masks. So control info gets heavy FEC: BPTC rate = 96/196 = 0.49, slot type 8/20 = 0.4; voice
AMBE+2 speech rate = 49/72 = 0.68 with unequal protection. Same economics as pack 01 §3.6.

### 3.8 Link control content and addressing

LC (72 bits, VERIFY layout): PF(1) R(1) FLCO(6) FID(8) + 56 bits: service options(8) + destination ID(24) + source ID(24).
FLCO values (VERIFY): 0 = group voice channel user (destination is a talkgroup), 3 = unit-to-unit voice channel
user (private call). **IDs are 24-bit** (≈ 16.7 M), as in P25 unit IDs (pack 01).

## 4. Fully worked example

**(a) Slot type for CC = 1, data type = 1 (voice LC header).**
info byte = `0x11`; as a 12-bit Golay info word `0x011`. Shift by 11: `0x8800`. Divide by g = `0xC75`:
`0x8800 ⊕ (g≪4 = 0xC750) = 0x4F50`; `⊕ (g≪3 = 0x63A8) = 0x2CF8`; `⊕ (g≪2 = 0x31D4) = 0x1D2C`;
`⊕ (g≪1 = 0x18EA) = 0x05C6` (11 bits ⇒ done). Parity = `0x5C6`. Codeword23 = `0x8800 | 0x05C6 = 0x8DC6`.
Drop 4 leading zero info bits ⇒ word19 = `0x8DC6` (19 bits). `0x8DC6` has 8 ones ⇒ parity 0 ⇒
**word20 = 0x8DC6 ≪ 1 = 0x11B8C**. The info byte sits at bits 19..12: `0x11B8C >> 12 = 0x11` ✔.
Flip bits 3 and 15: `0x11B8C ⊕ 0x8008 = 0x19B84` (try it in a test) — decoder: syndrome ≠ 0 ⇒ table ⇒ weight-2
pattern ⇒ corrected, 2 errors reported.

**(b) Burst arithmetic.** slot = 9600 × 0.030 = 288 b; burst 264 b = 27.5 ms; gap 24 b = 2.5 ms; voice bits 216 =
3 × 72; overhead per slot = (264 − 216) + 24 = 72 b = 25 % of the slot.

**(c) Where do voice frames sit?** Burst bits `[0,108)` first half, `[108,156)` middle, `[156,264)` second half:
frame 0 = bits 0–71; frame 1 = bits 72–107 **and** 156–191; frame 2 = bits 192–263.

**(d) Superframe timeline (one timeslot):** bursts at t = 0 (A, sync), 60 (B, LC frag 1), 120 (C, frag 2), 180 (D, frag 3),
240 (E, frag 4), 300 (F), 360 (A of next superframe) ms.

**(e) BPTC interleave start:** a = 0,1,2,3,4,5 → 0, 181, 166, 151, 136, 121 (each step subtracts 15 mod 196).

## 5. Questions

All questions have an **Etherstack context**; answers in `answers.md`; code in `code/include/dmr/`.

### Q1 — Explain
Why is the 48-bit SYNC/EMB field in the **middle** of the burst rather than at the start? What goes wrong with a
mid-burst fade in each case?
*Etherstack context:* the stack's equaliser/timing recovery depends on where the training sequence sits.

### Q2 — Explain
Derive "3 voice frames per burst" and "6 bursts per superframe" from first principles.
*Etherstack context:* these are the constants in the voice-path state machine of any DMR stack.

### Q3 — Calculate — **code: `burst.hpp`**
Compute slot bits, burst bits, gap bits, burst duration, voice bits per burst and overhead share.
*Etherstack context:* RTP/ISSI-style packetisation of DMR voice depends on these numbers.

### Q4 — Calculate
(a) Number of error patterns of weight ≤ 3 in 23 bits; why the Golay(23,12) code is perfect. (b) Fraction of random
20-bit words that a ≤3-error slot-type decoder accepts. (c) Compare DMR's voice share of gross rate with
P25 Phase 2's (pack 02).
*Etherstack context:* false-accept rates drive how many extra checks (CC, CRC, sync context) a stack needs.

### Q5 — Predict — **code: `slot_type.hpp`**
A slot type word arrives with 0, 1, 3, 4 and 5 bit errors. What does the decoder report for each? What happens at
5 errors for a fraction of the cases, and what protects the stack?
*Etherstack context:* "never act on a mis-decoded data type" is a stack invariant.

### Q6 — Predict
During a call, burst A's sync is hit by 8 bit errors and not recognised. Does the radio lose audio? What
mechanism keeps the receiver in step?
*Etherstack context:* flywheel/timing prediction is how stacks survive fades.

### Q7 — Trace — **code: `burst.hpp`, `superframe.hpp`**
(a) Which burst-bit positions make up AMBE+2 frame 1 (the straddling one)? (b) A radio joins at a random time:
what are the best, worst and mean times to collect a full embedded LC under the "first fragment first" rule?
*Etherstack context:* late-entry latency is an acceptance-test metric.

### Q8 — Trace — **code: `bptc.hpp`**
Compute interleave positions for matrix indices 0..5, prove 181 is invertible mod 196 and find the inverse, and
show that bits 1..14 apart in transmit order come from matrix cells ≥ 13 apart.
*Etherstack context:* a BPTC de-interleave table is built once at start-up in a real stack.

### Q9 — Find the bug — **code: `burst.hpp`, `slot_type.hpp`, `superframe.hpp`**
```cpp
std::copy(bits.begin()+72, bits.begin()+144, f1.begin());                 // (1) voice frame 1
uint64_t m = 0; for (int i = 0; i < 48; ++i) m |= uint64_t(mid[i]) << i;  // (2) pack mid
constexpr int kSuperframeMs = 5 * 60;                                     // (3)
uint8_t st = (dt << 4) | cc;                                              // (4) slot type byte
constexpr double kBurstMs = 30.0;                                         // (5)
```
Find all five bugs and fix them.
*Etherstack context:* a typical review of a first-draft DMR parser.

### Q10 — Implement — **code: `burst.hpp`**
Make `test_burst` pass.
*Etherstack context:* the burst splitter is the first stage of every DMR receive path.

### Q11 — Implement — **code: `slot_type.hpp`**
Make the slot-type tests pass (shortened + extended Golay). Then explain how you would test your decoder with *no*
radio, and what the `NoiseAcceptanceDensity` test tells you.
*Etherstack context:* golden-model tests for FEC.

### Q12 — Implement — **code: `superframe.hpp`, `bptc.hpp`**
Make `test_superframe` and `test_bptc` pass.
*Etherstack context:* superframe indexing and de-interleaving are inner-loop code.

### Q13 — Connect
A gateway joins a DMR talkgroup to a P25 talkgroup. Map: identities (24-bit IDs, talkgroup widths), colour code vs
NAC, timeslot, vocoder, late-entry/link-control repetition, call hang. What can't be mapped 1:1?
*Etherstack context:* DMR↔P25 interworking is a classic gateway product.

### Q14 — Interview
"Walk me through a DMR voice burst, bit by bit, and tell me what the receiver does with each part."
*Etherstack context:* whiteboard question.

### Q15 — Interview
"Is the DMR colour code a security feature? What happens if two neighbouring systems use the same CC? Compare to
P25's NAC and to TETRA's colour code."
*Etherstack context:* RF planning vs security — a common customer confusion.

## 6. Self-assessment

- [ ] I can derive 288 / 264 / 24 / 216 and 27.5 ms.
- [ ] I can explain the midamble argument.
- [ ] I can derive 3 frames per burst and 6 bursts per superframe.
- [ ] I can state the four sync words' roles (BS/MS × voice/data) and that voice/data differ by 24 bits.
- [ ] I can explain why Golay(23,12) is perfect and how slot type is shortened/extended.
- [ ] I can derive BPTC dimensions 9×11 → 13×15 → 196 and the interleaver.
- [ ] I can compute late-entry best/worst/mean (180 / 540 / 360 ms).
- [ ] All tests pass with my code.

| Score | Meaning |
|---|---|
| 0–3 | Re-read §3.2–3.4 |
| 4–6 | Redo Q2, Q4, Q7 |
| 7–8 | On to pack 04 |

### VERIFY list for this pack
1. Dibit→symbol map and 4FSK deviations; BS/MS sync digits (all four).
2. CACH contents; guard vs CACH in the 24-bit gap.
3. AMBE+2 49+23 split per 72-bit frame; whether DMR and P25 Phase 2 frame layouts are identical.
4. EMB structure (CC 4, PI 1, LCSS 2, QR(16,7,6)), embedded LC fragmenting (77→128 bits), what burst F carries.
5. Exact Golay(20,8) generator matrix in the standard (this pack uses a teaching shortening+parity construction); its stated min distance.
6. BPTC geometry, interleave direction ((a·181) mod 196), reserved bits, decoder strength.
7. LC layout, FLCO values, data type table, RS(12,9) usage, CRC for CSBK.
