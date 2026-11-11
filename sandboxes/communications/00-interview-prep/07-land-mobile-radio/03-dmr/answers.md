# Pack 03 — Answers (DMR)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → angle.**
Synthetic data only. **VERIFY** = check ETSI TS 102 361-1/-2/-3/-4 before quoting.

---

## Q1 — Explain: why the sync is in the middle

**Answer.** The receiver estimates timing, frequency offset and channel gain/phase from the known sync pattern
and applies that estimate to the data. The farther a data bit is from the training pattern, the staler the
estimate. A *midamble* halves the worst-case distance: 108 bits (54 symbols, 11.25 ms) to the nearest edge of the
sync, versus up to 264 bits (27.5 ms) with a preamble at the start.

**Reasoning.** On a moving vehicle at 800 MHz and 100 km/h, Doppler ≈ 74 Hz; coherence time ~ a few milliseconds to
tens of ms. A fade (deep null) during the burst changes the channel: with a preamble sync, *the entire second half* is
decoded with a stale estimate; with a midamble, both halves are within ~11 ms of fresh training. Secondary benefit:
voice frame 1 straddles the middle, so a fade there damages portions of one frame only.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "To make the burst symmetric for aesthetics." | Looks tidy. | It's an equalisation-error argument (distance from training). |
| "Sync at the start would be found earlier, which is better." | Early detection. | TDMA already knows burst timing; detection isn't the problem — channel tracking is. |
| "The middle is where the CACH goes." | CACH is the 24-bit gap. | CACH sits *between* bursts, not in them. |

---

## Q2 — Explain: 3 frames per burst, 6 bursts per superframe

**Answer.** *Frames per burst* = slot repeat period ÷ vocoder frame period = (2 × 30 ms) ÷ 20 ms = **3**; continuity of
speech demands exactly 60 ms of speech per 60 ms of air-time cycle. 3 × 72 = 216 ✔ equals 108 + 108 voice bits.
*Bursts per superframe*: the embedded LC (72 + 5 CRC = 77 bits, FEC-expanded to 128) needs 128 / 32 = **4 fragments**
(B–E); A carries the voice sync; F carries other embedded signalling ⇒ 1 + 4 + 1 = **6** bursts = 360 ms.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "3 frames because 264 / 72 = 3.67 ≈ 3." | Numerology. | 264 includes the 48-bit middle; voice bits are 216. |
| "6 bursts because there are 6 letters A–F." | Circular. | Letters follow from the signalling payload arithmetic. |
| "4 frames like P25 Phase 2." | Same TDMA family. | P25p2's slot is 30 ms, but the pattern includes non-voice slots; DMR sends voice every burst. |
| "Superframe = 6 × 30 ms = 180 ms." | Slot = 30 ms. | One logical channel gets every *second* slot: 60 ms. |

---

## Q3 — Calculate

| Quantity | Derivation | Value |
|---|---|---|
| slot | 9600 × 0.030 | **288 bits** |
| burst | 108 + 48 + 108 | **264 bits** |
| gap | 288 − 264 | **24 bits** (2.5 ms) |
| burst time | 264 / 9600 | **27.5 ms** |
| voice bits | 2 × 108 | **216 = 3 × 72** |
| overhead / slot | (264 − 216) + 24 | **72 bits = 25 %** |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Burst = 288 bits." | 30 ms × 9600. | That's the slot; 24 bits are CACH/guard. |
| "Burst = 30 ms." | Slot name. | 27.5 ms. |
| "Voice share = 264/288 = 92 %." | Burst fraction. | 48 mid bits aren't voice: 216/288 = 75 %. |

---

## Q4 — Calculate

(a) 1 + 23 + C(23,2)=253 + C(23,3)=1771 = **2048 = 2¹¹** = number of syndromes (2²³⁻¹²). Every syndrome ↔ exactly
one pattern of weight ≤ 3 ⇒ spheres of radius 3 tile the whole space ⇒ **perfect**.
(b) Slot-type (20,8) with d = 8: each codeword's radius-3 sphere in 20 bits has 1 + 20 + 190 + 1140 = 1351 words; 256
codewords × 1351 = 345 856; ÷ 2²⁰ (1 048 576) = **0.3298 ≈ 33 %** (the unit test confirms it). With the CC match
(1/16) ⇒ ≈ 2 %.
(c) DMR: 2 × 3600/9600 = **75 %** voice (overhead 25 %); P25 Phase 2: 7200/12 000 = **60 %** (overhead 40 %). DMR spends
less on overhead but has a lower gross rate; **both carry two calls per 12.5 kHz**. (VERIFY: AMBE+2 frame composition.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Perfect means error-free." | Everyday meaning. | Perfect = no wasted syndromes; it still can't correct > 3. |
| "Accept rate ≈ 1/2⁸." | 8 info bits. | Accepting radius-3 spheres inflates it to 33 %. |
| "DMR has more capacity than P25p2 because less overhead." | Lower overhead share. | Calls/Hz equal (2 per 12.5 kHz); gross rate 9.6 vs 12 kbit/s. |

---

## Q5 — Predict (slot type errors)

| Errors | Decoder result |
|---|---|
| 0 | ok, errors = 0 |
| 1 | ok, corrected, errors = 1 |
| 3 | ok, corrected, errors = 3 |
| 4 | **not ok** (detected: d_min = 8 ⇒ correct 3, detect 4) |
| 5 | pattern-dependent: usually detected, but some 5-error patterns lie within radius 3 of a *different* codeword (distance 8 apart) ⇒ **silent miscorrection** to a wrong CC/data type |

Protection: (i) CC must match ours (1/16); (ii) the data type must be plausible *in context* (e.g. a voice terminator
can't arrive without a call); (iii) the payload's own CRC/FEC (LC RS(12,9), CSBK CRC) fails for a wrong type; (iv)
timing flywheel disagreement. Invariant: **never act on slot type alone.**

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "More than 3 errors is always detected." | d=8 sounds strong. | Detect ≤ 4 only; at 5 miscorrection is possible. |
| "A parity bit would catch it." | Overall parity exists. | It's *included* in the 3-error budget; it can't catch an even/odd mix at 5. |
| "Colour-code check is enough." | CC ties to system. | CC is 4 bits; random hit 1/16. |

---

## Q6 — Predict (sync lost on burst A)

**Answer.** Audio continues. Voice-burst *content* is located by timing, not by the match result: positions of
the three frames are fixed relative to the slot clock. The receiver's **flywheel** (slot/superframe counter
predicted from earlier syncs) keeps it aligned; each B–F burst's EMB (colour code with QR(16,7,6) parity) gives
continuing evidence the burst is ours. Sync-loss is declared only after several consecutive failures (an
implementation parameter; VERIFY standard guidance). The cost: one missed re-sync is harmless; many misses ⇒ drift.
Late-entry receivers *cannot* flywheel (no history) — they depend on seeing syncs.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The whole superframe is lost." | Sync = framing. | Timing is predicted; only the confidence updates. |
| "A voice frame is lost." | Sync is mid-burst. | The sync bits aren't voice; frame 1 straddles but its bits are separate from the sync bits. |
| "Receiver immediately drops the call." | Fail-safe. | That would drop calls on every fade; hysteresis is needed. |

---

## Q7 — Trace

(a) Voice frame 1 = burst bits **72–107** (36 bits, end of first half) followed by **156–191** (36 bits, start of the
second half); the 48 middle bits (108–155) are *skipped*. Frames 0 and 2: 0–71 and 192–263.
(b) Measure between burst centres (the embedded fields are mid-burst). Receiver must see fragment 1 (burst B) and
then C, D, E. Let join time t be uniform over the 6-burst cycle (in bursts, 1 burst = 60 ms). Time to the next B centre
is uniform on (0, 6] bursts; then 3 more bursts to E. So wait = U(0,6] + 3 bursts:
**best ≈ 3 bursts = 180 ms**, **worst = 9 bursts = 540 ms**, **mean = 6 bursts = 360 ms**. Code:
`late_entry_worst_case_ms() = cycle_bursts*60 + (fragments−1)*60 = 360 + 180 = 540`.
(A smarter reassembler that accepts fragments out of order using LCSS can finish sooner; not assumed.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Frame 1 = bits 72–143." | Contiguous. | Bits 108–155 are the middle (sync/EMB), not voice. |
| "Worst case 360 ms (one superframe)." | Repeats every 360. | Need the *first* fragment and then three more. |
| "Mean = worst/2." | Symmetry. | Mean = 360 here because the wait is 3 + U(0,6]. |

---

## Q8 — Trace (BPTC interleave)

- a = 0..5 → (a·181) mod 196 = 0, 181, 362−196 = 166, 543−392 = 151, 724−588 = 136, 905−784 = 121.
- Inverse: Euclid: 196 = 1·181 + 15; 181 = 12·15 + 1 ⇒ gcd = 1. Back-substitute: 1 = 181 − 12·15 = 181 − 12(196 − 181) =
  13·181 − 12·196 ⇒ **181⁻¹ = 13**. Check 181·13 = 2353 = 12·196 + 1 ✔.
- Spread: transmit positions p and p+d ↔ matrix indices differing by 13d (mod 196). d = 1..14 gives circular distance
  min(13d, 196−13d) = 13, 26, 39, 52, 65, 78, 91, 92, 79, 66, 53, 40, 27, 14 — all ≥ 13 ✔. At d = 15, 13·15 = 195 ≡ −1:
  bits 15 apart are *adjacent* in the matrix — the interleaver protects against bursts shorter than ~15 bits.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Interleaving adds redundancy." | Part of FEC chain. | It adds none; it only reorders. |
| "Any multiplier works." | Multiplication mod N. | Must be coprime to N; else not a permutation. |
| "181 is the inverse of 181." | Symmetry. | 181² mod 196 = 32761 mod 196 = 169 ≠ 1. |

---

## Q9 — Find the bug

1. `bits[72..144)` — frame 1 is **72–107 + 156–191**; contiguous copy swallows 36 bits of sync/EMB.
2. `m |= mid[i] << i` packs LSB-first: bit 0 (first transmitted) lands in the LSB; every sync compare is against a
   *bit-reversed* word and fails. Need `m = (m << 1) | mid[i]` (first bit = MSB).
3. `5 * 60` — six bursts: **6 × 60 = 360 ms**.
4. `(dt << 4) | cc` — swapped; colour code is the **high** nibble: `(cc << 4) | dt`. (Colour-code mismatch on every burst.)
5. `kBurstMs = 30.0` — **27.5 ms**; 30 ms is the slot. (Timing budgets off by 2.5 ms: PA ramp/guard.)
Also (for UBSan fans): `uint64_t(mid[i]) << i` is correct; `mid[i] << i` without the cast would overflow `int` for i ≥ 31.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Missing (1). | Looks like a slice. | It's the subtlest: only frame 1 straddles. |
| "(2) is fine, it's just endianness." | Host order. | Bit order within the word defines the compare. |

---

## Q10 / Q11 / Q12 — Implement

References: `solutions/dmr/*.hpp`.
- **burst:** `std::copy` ranges; frame 1 = two copies; `pack_bits48` MSB-first; `classify_sync` picks nearest, `None` on tie
  or over tolerance.
- **slot type:** `cw23 = golay23_encode(info8)`; `word19 = cw23 & 0x7FFFF`; parity = popcount(word19) & 1; decode =
  `golay23_decode(word19)`; reject `data > 0xFF`; add 1 error for a parity mismatch; ok iff ≤ 3. *Testing without radio:*
  exhaustive tests — all 256 info values; all error patterns of weight 1–3 (corrected) and 4 (detected); count of
  accepted words over all 2²⁰ inputs equals 256 × 1351 — a *theory-derived* number, so an off-by-one in the decoder
  shows up as a different count. `NoiseAcceptanceDensity` tells you ≈ 33 % of noise passes this stage alone.
- **superframe:** `static_cast<VoiceBurst>(n % 6)`; the late-entry formula; capacity = 4 × 32.
- **bptc:** dims 9×11 → 13×15 = 195 + 1 = 196; interleave `(a*181)%196`; inverse multiplier 13; `gcd_int` Euclid; circular
  distance `min(|a−b|, N−|a−b|)`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Parity bit first." | Position choice. | Systematic placement: info bits high, then 11 parity, then overall parity. |
| "Accept if errors ≤ 4." | We detect 4. | Detect ≠ correct; ok requires ≤ 3. |
| "Store the Golay table in a file." | Speed. | Build from the polynomial once (static init), verify perfectness in tests. |

---

## Q13 — Connect (DMR ↔ P25 gateway)

| Item | Mapping |
|---|---|
| Identities | DMR source/dest IDs are 24-bit; DMR talkgroups are 24-bit; P25 unit IDs are 24-bit but **TGIDs are 16-bit** (plus WACN/SysID scope). ⇒ a **mapping table** with collision checks, not a cast. |
| Colour code vs NAC | Local RF tags (4 vs 12 bits) — **never mapped**; each side generates its own. |
| Timeslot | DMR slot 1/2 (a logical channel) has no P25 Phase 1 analogue (Phase 2 has a slot). Gateway picks per-call. |
| Vocoder | DMR AMBE+2 vs P25 Phase 1 IMBE ⇒ transcode (lossy tandem). To P25 Phase 2 (AMBE+2) a re-framing might suffice (VERIFY layout equality). |
| Late entry | Each side repeats LC on its own cycle (DMR embedded LC every 360 ms; P25 LDU1/LDU2). The gateway must be **stateful**: store call parameters and *regenerate* repeats. |
| Call end | DMR terminator-with-LC ↔ P25 TDU/TDULC. |
| Emergency/priority | Map service-options bits; semantics differ (VERIFY). |
| Encryption | DMR privacy (basic/enhanced, mostly vendor-specific — VERIFY) ↔ P25 AES-256/DES-OFB: **no mapping**; E2E-encrypted calls don't bridge without keys at the gateway. |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Truncate DMR TG to 16 bits." | Fits. | Collisions: TG 0x010001 and 0x000001 would alias. |
| "Copy the CC into the NAC." | Both ‘network codes’. | Different RF systems; CC is 4 bits and local. |
| "Gateways are stateless relays." | Simple. | Late entry/LC repetition requires state. |

---

## Q14 — Interview: walk through a voice burst

**Model answer.** "A voice burst is 264 bits in a 288-bit slot (the last 24 are CACH/guard). It's 108 bits of voice,
48 bits in the middle, 108 bits of voice. The 216 voice bits are three 72-bit AMBE+2 frames — frame 1 straddles the
middle. In burst A the middle is a 48-bit voice sync (BS or MS flavour) that gives timing and says 'voice from base'.
In B–F the middle is EMB(8)+32 embedded+EMB(8): the EMB carries colour code, privacy indicator, LCSS and QR parity;
the 32 bits are fragments of the embedded link control — four of them (B–E) rebuild the 77-bit LC for late entry.
Six bursts = 360 ms = one superframe. Receiver: detect sync, track the slot clock, check CC via EMB, decode the three
frames to the vocoder, rebuild the LC from fragments, and flywheel through fades."

Common traps: calling the burst 30 ms; forgetting frame 1 straddles; confusing slot type (data bursts) with EMB (voice).

---

## Q15 — Interview: colour code

**Answer.** **Not security.** 4 bits (16 values) in clear: it separates co-channel/adjacent systems so a receiver
ignores foreign bursts (like P25's 12-bit NAC; like TETRA's colour code — VERIFY details of TETRA CC in pack 04). Two
neighbouring systems with the *same* CC on the same frequency: radios decode each other's bursts (cross-talk,
mixed audio, control confusion) — an **RF planning** error, which is more likely with only 16 values than with
P25's 4096 NACs. Security is encryption/authentication, which the CC doesn't provide at all.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "CC authenticates radios." | Like a password. | Anyone can transmit any CC. |
| "CC lets you hide a system." | Looks like a filter. | The filter is only for *friendly* receivers. |
| "More CC values are unnecessary." | 16 is enough. | Dense networks can exhaust CCs; reuse planning is real work. |

---

## Self-check answers
288/264/24/216/27.5 ms ✔; midamble argument ✔; 3 frames = 60/20, 6 bursts = 1+4+1 ✔; BS/MS × voice/data; Golay(23,12) perfect (2048) ✔;
BPTC 9×11→13×15→196 ✔; late entry 180/540/360 ms ✔.
