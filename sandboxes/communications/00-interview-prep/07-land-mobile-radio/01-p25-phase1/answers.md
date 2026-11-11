# Pack 01 — Answers (P25 Phase 1)

Format for every question: **Answer → Reasoning (derived) → Wrong answers table (wrong answer | why tempting |
why wrong) → Etherstack/interview angle.** Synthetic frames only — see the safety box in `worksheet.md`.
Anything marked **VERIFY** must be checked against the TIA-102 documents before you quote it.

---

## Q1 — Explain (modulation)

**Answer.** Four levels carry 2 bits per symbol, doubling the bit rate at a given symbol rate (and therefore at
about the same occupied bandwidth). The mapping `01(+3) 00(+1) 10(−1) 11(−3)` is Gray: adjacent levels differ by one
bit, so the most likely symbol error costs one bit error. Constant-envelope FM lets the PA run non-linear
(efficient, cheap, cool), which extends handheld battery life.

**Reasoning.**
1. Spectrum is licensed per 12.5 kHz. Bits/s = symbols/s × bits/symbol. Raising bits/symbol is the way to raise
   rate without raising baud (baud sets bandwidth).
2. Noise moves a received deviation to a *neighbouring* level far more often than to a distant one (Gaussian tail).
   Mapping so neighbours differ by one bit minimises expected bit errors per symbol error: 1 instead of up to 2.
3. FM has no amplitude information, so there is no linearity requirement on the PA.
4. Trade-off: constant-envelope FM isn't as spectrally tight as linear modulation — one reason Phase 2 switches to
   a linear downlink (H-DQPSK) and a constrained-envelope CPM uplink (pack 02).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Four levels give four times the bit rate." | 4 levels, so ×4? | Bits/symbol = log2(4) = 2, so ×2 versus 2-level. |
| "The mapping is plain binary 00,01,10,11 in level order." | Natural counting. | A one-level slip 01→10 would flip 2 bits. Gray ordering avoids it. |
| "Constant envelope improves range." | Efficient PA ⇒ more power? | It improves *efficiency*, not link budget; range comes from power, antenna, sensitivity. |
| "More levels (8, 16) would be strictly better." | More bits per symbol. | SNR needed grows quickly; levels get close together; noise errors rise. It's a trade-off. |

**Interview angle.** Say "Gray mapping" and "non-linear PA" unprompted; mention the DIBIT interface to the PHY.

---

## Q2 — Explain (FEC strength)

**Answer.** An error in NID corrupts the *interpretation* of the entire following frame (1728 bits, 180 ms, or a
control message), while an error in voice damages only the 20 ms in which it occurs, and the ear masks much of that.
So you spend FEC where the damage multiplier is largest. Inside a voice frame, bits differ in *perceptual*
importance; the codec puts critical parameters under Golay(23,12) (corrects 3), moderately important under
Hamming(15,11) (corrects 1), and leaves the 7 least significant bits raw.

**Reasoning with numbers.**
- NID: 16 info bits, 63-bit BCH (47 parity) + 1 parity bit → rate 0.25; corrects up to 11 of 63 bits (17 %).
- Voice: 88 info bits → 144 bits, rate 0.61; Golay corrects 3/23 (13 %); Hamming 1/15 (6.7 %); 7 bits unprotected.
- Cost of protecting the NID heavily: 48 extra bits **per frame**, small. Cost of protecting every voice bit as
  strongly: ~4× the voice bits, which would not fit in the channel.
- Rule: **FEC budget ∝ (probability of error) × (cost of an error) ÷ (bits to protect).** Headers: cost high, bits few.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Voice is less important than data." | Public-safety voice is critical. | Voice *is* critical; the point is the *damage per error*, not importance of the service. |
| "Voice can be retransmitted so needs less FEC." | ARQ in IP. | Radio voice is broadcast and real-time; there is no retransmission. |
| "Voice has no FEC." | Only 88 info bits matter. | It has 56 bits of FEC; only the 7 least significant are unprotected. |
| "Stronger code = better everywhere." | Obviously safer. | Overhead bits displace payload; you need an economic argument. |

**Etherstack angle.** In conformance runs the stack must (a) decode NID with ≤11 errors and (b) *not* misdetect. Know
which one you configure.

---

## Q3 — Calculate (bit budget)

**Answer.**
(a) 48 (sync) + 64 (NID) + 9×144 = 1296 (IMBE) + 240 (LC/ES coded) + 32 (LSD coded) + 48 (24 status symbols × 2 bits)
= **1728**. 1728 bits / 9600 bit/s = **180 ms**; 864 symbols / 4800 = 180 ms.
(b) 1 frame / 20 ms = **50 IMBE frames/s**; 9 per LDU × (1000/180) LDUs/s = 50 ✔.
(c) Speech information: 88 × 50 = 4400 bit/s = **45.8 %** of 9600. Voice on air (144 × 50 = 7200 bit/s) = **75 %**.
The remaining 2400 bit/s is overhead (sync, NID, LC/ES, LSD, status).

**Derivation of 144 and 88:** 4×Golay(23,12) = 92 coded / 48 info; 3×Hamming(15,11) = 45 / 33; 7 raw = 7 / 7.
Coded 92+45+7 = 144; info 48+33+7 = 88.

**Derivation of 240 for LC:** 72 info bits = 12 hex symbols → RS(24,12) = 24 hex = 144 bits → each 6-bit
word gets Hamming(10,6) → 240 bits. For ES: 96 info = 16 hex → RS(24,16) = 144 → 240 (**VERIFY** codes).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "1296+112 = 1408 is the whole LDU." | Forgetting LC and LSD. | The unaccounted 320 bits (240+32+48) are real payload and status. |
| "Status symbols are 24 bits." | 24 symbols. | A symbol is 2 bits ⇒ 48 bits. |
| "50 frames/s × 144 = 9600 so voice fills the channel." | Round numbers. | 50×144 = 7200. 9600 × 0.18 s = 1728 is the *whole* LDU, of which voice is 1296. |
| "180 ms = 9 voice frames × 40 ms." | Mis-remembered frame period. | IMBE frame = 20 ms; 9 × 20 = 180. |

---

## Q4 — Calculate (false sync)

**Answer.**
(a) Words within 4 errors: C(48,0..4) = 1 + 48 + 1128 + 17296 + 194580 = 213 053. P = 213053 / 2⁴⁸ =
213053 / 281 474 976 710 656 = **7.57 × 10⁻¹⁰**.
(b) False syncs/s = 7.57e-10 × 4800 = 3.63e-6 ⇒ one per **≈ 2.75 × 10⁵ s ≈ 76.5 hours**.
(c) 32-bit sync, m = 4: C(32,0..4) = 1+32+496+4960+35960 = 41 449; P = 41449/2³² = 9.65e-6; × 4800 = 0.0463/s ⇒ one
per **≈ 21.6 s**. A 4× reduction in length costs a factor of ~3.5 × 10⁶ in reliability.

**Why it matters:** the false sync is not fatal — the NID (BCH) will reject it — but it burns CPU and, worse, a
false lock can *hide* a real frame arriving during the garbage "frame".

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "P = 4/48." | 4 errors out of 48 bits. | That's an error *rate*; we need the *count of acceptable words* over all 2⁴⁸. |
| "P = 1/2⁴⁸ regardless of m." | Only one correct sync. | A tolerant receiver accepts many words (213 053 for m=4). |
| "Use 9600 windows/s." | Bit rate is 9600. | Symbol timing is known; windows are tested on symbol (dibit) boundaries: 4800/s. If you *do* search bit-wise, double the rate and halve the time. |
| "The sync is unaffected by noise." | It's a fixed pattern. | It's received through the same noisy channel; that's why we allow errors. |

---

## Q5 — Predict (error behaviour)

**Answer.**
(a) 3 errors: decoded correctly (3 ≤ t = 11), `errors = 3`.
(b) 11 errors: decoded correctly; this is the limit (2t+1 = 23 ⇒ spheres of radius 11 around codewords don't overlap).
(c) 12 errors: **undefined by the BCH guarantee**. Any other codeword is at distance ≥ 23 − 12 = 11 from the received
word, so either *no* codeword is within 11 (decoder reports failure) or one is at exactly 11 (decoder
**miscorrects** to the wrong NID — silent).

With the overall parity bit: a distance-11 miscorrection changes the 63-bit word by an *odd* number of flips from
what was received while the 12 real errors were an *even* number; the corrected codeword's parity therefore
disagrees with the received parity bit ⇒ the decoder treats that as a 12th error ⇒ total > 11 ⇒ **reject**.
Hence d = 24 gives "correct 11 / detect 12" (11 + 12 + 1 = 24).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "12 errors always fails cleanly." | A bounded-distance decoder sounds safe. | Beyond t it can decode to another codeword silently (miscorrection). |
| "BCH(63,16) corrects 16 errors." | k = 16. | t = ⌊(d−1)/2⌋ = 11, and d = 23 comes from the construction (47 parity bits). |
| "Parity corrects errors." | Extra bit. | A single parity bit *detects* odd weight; here it lifts d to 24 for detection. |
| "The 64th bit is protected by the BCH." | It's in the NID. | It's outside the 63-bit code; it's the overall-parity bit (**VERIFY** convention). |

---

## Q6 — Trace (NID packing)

**Answer.**
(a) NAC 0x293, LDU2 (0xA): `(0x293 << 4) | 0xA = 0x2930 | 0xA = 0x293A`.
(b) Data word with NAC 0xF7E and DUID 0x7: `0xF7E7` (0xF7E << 4 = 0xF7E0; | 7). The NAC is 0xF7E (wildcard,
**VERIFY**), and DUID 0x7 = TSBK.
(c) Unassigned: 0x1, 0x2, 0x4, 0x6, 0x8, 0x9, 0xB, 0xD, 0xE. After a *successful* BCH decode a parser should treat the
frame as "unknown type": log it, count it, drop it, and **resynchronise** — don't guess a length. (The code is
valid, so this is probably a *newer* standard feature or a sender bug, not noise.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "0xA293" | DUID first. | NAC is the high 12 bits. |
| "0x293A is 0x29 3A so DUID is 0x3A." | Byte boundaries. | NAC/DUID boundary is at bit 4, not byte 1. |
| "`nac<<4 & duid`" | Confusing the operators. | `&` would zero everything (§Q9). |
| "Unknown DUID ⇒ treat as TSBK." | Fail-open. | You'd parse random bits as signalling; fail-closed. |

---

## Q7 — Trace (late entry)

Setup: frames alternate LDU1 [0,180), LDU2 [180,360), period 360 ms. The receiver must see a **frame start**
(sync+NID) and then the **complete** LDU, because LC/ES are only valid after the whole LDU is FEC-decoded.

**(a) ES (in LDU2).** Join in LDU1 at offset u: next frame start = LDU2 at 180; complete at 360 ⇒ wait 360 − u
∈ (180, 360]. Join in LDU2 at t ∈ [180,360): next LDU1 at 360, next LDU2 at 540, complete at 720 ⇒ wait 720 − t ∈
(360, 540]. **Worst ≈ 540 ms** (join just after an LDU2 started).
**(b) LC (in LDU1).** Join in LDU1 at t: next LDU1 starts 360, completes 540 ⇒ wait 540 − t ∈ (360,540]. Join in LDU2:
wait 540 − t ∈ (180, 360]. **Worst ≈ 540 ms.**
**(c) Expected** (uniform join over 360 ms): ES: ½·270 + ½·450 = **360 ms**; LC: ½·450 + ½·270 = **360 ms**.
(Clear voice audio can start earlier: at the next frame start: worst 180, mean 90 ms.)
If a frame is lost (probability p of failing FEC), expected extra wait = 360 ms × p/(1−p).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "360 ms (one superframe)." | Repeats every 360 ms. | You also need to *wait for the frame start* and *finish receiving* it: +180. |
| "Join time 20 ms (one IMBE frame)." | Voice frames are 20 ms. | Frames are decodable only after finding LDU framing; and LC/ES need the whole LDU. |
| "The HDU gives it to late entrants." | HDU has the same fields. | By definition they missed the HDU. |
| "MI is constant for the call." | A "message indicator" sounds fixed. | It advances (VERIFY rule); hence ES must be repeated to give the *current* value. |

---

## Q8 — Find the bug

1. **(1) Unmasked window.** `window` is a sliding 64-bit register; bits above 47 are old data. XOR-popcount counts
   them ⇒ extra phantom errors ⇒ real sync is *rejected* when preceded by non-zero history. Fix: `& kMask48`
   before popcount (and `__builtin_popcountll` isn't portable to MSVC; use `std::bitset`/C++20 `std::popcount`).
2. **(2) `<` vs `<=`.** "Within m errors" includes m. With `m = 0` the function can never match even a perfect
   sync (errs < 0 false). Fix: `<=`.
3. **(3) kBitRate = 4800.** That's the *symbol* rate. Bit rate = 4800 × 2 = 9600. All durations computed from it are
   **doubled** (an LDU becomes 360 ms instead of 180 ms).
4. **(4) `k % 36 == 0`.** Marks symbol 0 (a sync symbol) as status and misses symbol 35. Fix: `(k + 1) % 36 == 0`
   (**VERIFY** origin). Symptom: the NID is read 2 bits off and BCH fails for *every* frame, which looks like "bad
   radio" but is a parser bug.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Only bug (2)." | It's the visible one. | (1) shows up only in streaming; (3) and (4) appear when tests are on aligned data. |
| "(4) is fine: modulo 36 is right." | Period is right. | The *phase* is wrong. |
| "(3) is a naming issue." | Constant just mislabelled. | It changes arithmetic. |

---

## Q9 — Find the bug (parsing)

For `nac = 0x293`, `d = LDU1 (5)`:
- `(nac << 4) & 5` = `0x2930 & 0x5` = **0**. Must be `|`. (Every NID would read NAC 0 / HDU.) Also `nac << 4` is an
  `int` shift — fine here, but validate `nac ≤ 0xFFF`.
- `lc[4] | lc[5]` = `0x12 | 0x34` = **0x36**. Needs `(lc[4] << 8) | lc[5]` = 0x1234.
- `source`: precedence: `<<` binds tighter than `|`, so `lc[8] << 24` is shifted by 24 not placed at bit 0:
  `(lc[6]<<16) | (lc[7]<<8) | (lc[8]<<24)` = `0 | 0xAB00 | 0xCD000000` = **0xCD00AB00**. Correct is
  `(lc[6]<<16) | (lc[7]<<8) | lc[8]` = **0x00ABCD**. Additionally `0xCD << 24` overflows signed `int` ⇒ **undefined
  behaviour** that `-fsanitize=undefined` would flag. Cast to `uint32_t` before shifting.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Bitwise OR and `+` are equivalent here." | Often are. | Not when fields overlap (here, the bug is `&`/missing shift). |
| "The sanitizer would catch all of it." | We built with UBSan. | It catches the signed overflow only; wrong-but-defined arithmetic is invisible. Tests catch the rest. |
| "A 24-bit ID can go in `uint16_t`." | Fits typical IDs. | 24 bits needs `uint32_t`. |

---

## Q10 — Implement (BCH)

**Reference implementation:** `code/solutions/p25p1/bch.hpp`.

Key points:
- `poly_mod`: for i from high to `deg`: if bit i set, XOR `g << (i − deg)`. (That *is* polynomial long division
  over GF(2): subtraction = XOR.)
- Encode: `shifted = data << 47; cw = shifted | poly_mod(shifted, g, 47)`. Parity is the remainder, so
  `cw ≡ 0 (mod g)`.
- Decode: compare with all 65 536 codewords, keep nearest; `ok = (dist ≤ 11)`. Correct because spheres of radius 11
  don't overlap (d_min ≥ 23 — and `MinimumDistanceIsAtLeast23` proves it by exhaustion for *your* generator).
- NID64: append the overall parity bit.

**Brute force is O(2¹⁶) per decode** — ~65 k XOR+popcounts ≈ ~100 µs; at thousands of NIDs/s per channel × many
channels it eats a core. **Alternatives:** (1) *syndrome decoding* with the Berlekamp–Massey + Chien-search
algorithm (the textbook BCH decoder; cost polynomial in t); (2) *restricted candidate matching*: a receiver
configured for one NAC only needs to compare against the **7 valid DUID** codewords for that NAC (7 distances!) and
accept the closest within threshold. (3) Lookup tables / bit-sliced SIMD.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Parity = XOR of data bits." | "Parity bit" vocabulary. | That's a single-bit parity check; BCH parity is a polynomial remainder. |
| "Copy the generator from a table." | Faster. | A mis-transcribed constant silently gives a *different* code; deriving from GF(64) plus the d ≥ 23 test is verifiable. |
| "`ok = (dist < 11)`." | Off-by-one. | t = 11 ⇒ distance 11 is correctable. |
| "Brute force is fine in production." | It works in the test. | Complexity; see above. |

---

## Q11 — Implement (LC / ES)

Reference: `code/solutions/p25p1/ldu.hpp`. Big-endian, **cast before shifting** (`static_cast<uint32_t>(lc[6]) << 16`).
`is_clear(algid) = (algid == 0x80)`. `emergency = so & 0x80` (**VERIFY** bit position).

Common error summary: TGID is 16 bits, source is **24** bits; MI is 9 bytes (72 bits); KID big-endian.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "`is_clear = (algid == 0)`." | Zero = none. | P25's *clear* code point is 0x80, not 0. |
| "MI is 8 bytes (64-bit DES IV)." | DES IV is 64 bits. | The field is 72 bits; DES uses 64 of them (VERIFY). |
| "Little-endian since x86." | Host order. | Air/standard order is big-endian; parsing is host-independent. |

---

## Q12 — Connect (CAI → ISSI)

**Sketch (model answer; details VERIFY [TIA-102.BACA]):**
1. **RF side** terminates the CAI: sync/NID/FEC/status handled; yields clean IMBE frames + LC/ES.
2. **Call control → SIP.** A group call setup between RF subsystems becomes SIP INVITE/200/ACK with SDP; the
   talkgroup identity (and system/unit identities from the LC) go into the SIP/SDP or an ISSI-defined header;
   the call-level service options (emergency, priority, encrypted) are mapped to ISSI parameters.
3. **Voice → RTP.** IMBE frames (not LDUs) are packetised with RTP timestamps (20 ms frame clock) and sequence
   numbers. Several IMBE frames can share one RTP packet (trade latency vs overhead: header 40 B IPv4/UDP/RTP
   per packet is large relative to an 18-byte IMBE frame).
4. **Floor control**: PTT/transmit-request state travels as ISSI messages/RTP control (VERIFY).
5. **Encryption:** end-to-end encrypted voice stays encrypted — the ISSI carries ALGID/KID/MI so the far end can decrypt.
6. **Regenerated, not transported:** frame sync, NAC (re-inserted from the *destination* system's NAC),
   status symbols, FEC (recomputed by the destination RF subsystem). **Not preserved:** air bit errors (corrected
   or dropped).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Send the raw 1728-bit LDU in RTP." | Simplest. | Wastes bandwidth (carries FEC/sync), couples both ends' NAC/RF details. |
| "NAC is carried across the ISSI." | It's in every frame. | NAC is a local RF-system tag. |
| "The ISSI is symmetrical to CAI." | Same standard family. | It's IP/SIP/RTP; different layer entirely. |
| "SIP carries the voice." | SIP = VoIP. | SIP sets up; RTP carries media. |

---

## Q13 — Interview: bits to first voice frame

**Model answer (aim for 90 seconds):**
1. **Symbol recovery:** demodulate to dibits (C4FM at 4800 sym/s → dibit stream).
2. **Frame sync:** slide a 48-bit window, accept ≤ m errors (m chosen from a false-alarm budget: for 4, ≈ one false
   lock per 76 h of noise).
3. **Strip status symbols** (every 36th symbol) ahead of FEC.
4. **NID:** 64 bits → BCH(63,16) decode + parity check → NAC (filter or accept-any) and DUID.
5. **Dispatch on DUID:** HDU / LDU1 / LDU2 / TSBK / TDU / PDU. Unknown ⇒ resync.
6. **FEC per field:** IMBE voice (Golay/Hamming) → 9 frames; LC/ES via RS + Hamming; LSD cyclic code.
7. **Late entry / state:** keep the latest LC/ES (TG, source, ALGID/KID/MI) → decide: play, decrypt, or squelch.
8. **Vocoder:** IMBE → PCM (or forward to ISSI/RTP).
**Where it can go wrong:** wrong sync tolerance; status symbols not stripped; NID bit order; NAC filter too strict
(or "accept any" in a co-channel area); mis-correction; frame length from unknown DUID; endianness; MI not
advancing; encrypted audio played as noise; timing drift.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Skipping status symbols. | They're not in the NID diagram. | They're interleaved in the stream. |
| "Decode IMBE first, then look for sync." | Voice is the goal. | Everything is framed by sync/NID. |
| Listing only RF steps. | PHY bias. | Interviewers want the *state/late-entry* layer too. |

---

## Q14 — Interview: testing without radio, and why not live traffic

**Model answer.**
- **Golden vectors:** write an *encoder* (stub → solution) that produces frames from structs; feed frames into the
  *decoder*; assert struct equality (round-trip) and test corruption (flip k bits, k ≤ t works, k > t fails/detects).
- **Property tests:** linearity of the code, `min distance ≥ 23` by exhaustion, timing arithmetic.
- **Conformance vectors:** TIA-102.BAAB (CAI conformance — **VERIFY**) defines test procedures; vendors derive
  vectors from the spec.
- **Fault injection:** drop frames, duplicate, reorder at the ISSI; late entry at every offset.
- **Sanitizers:** ASan/UBSan (we build with them) catch overflow in parsers.
- **Why never live traffic:** (1) legality — intercepting public-safety communications may be a crime in your
  jurisdiction; (2) *encrypted* traffic is out of bounds by design; (3) privacy/operational security of
  responders; (4) reproducibility — live data can't be re-run; (5) professional trust — a vendor who tests on
  live public-safety systems loses credibility instantly. Synthetic frames are *better engineering*, not just
  safer.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Record real traffic with an SDR; it's realistic." | Realism. | Legal risk, privacy, not reproducible; test generators are more thorough. |
| "Unit tests are enough." | We have them. | Need interface, interoperability and conformance tests too. |
| "Encryption prevents any problem." | Encrypted = safe. | Metadata (talkgroups, IDs, timing) is itself sensitive and legally protected in places. |

---

## Self-check answers

- 4800 baud × 2 = 9600 bit/s; 1728 / 9600 = 0.18 s.
- Sync: probability argument (Q4).
- NAC ≠ security; ALGID ≠ key.
- BCH: shift, divide, append remainder; decode: syndrome / nearest codeword.
- Headers get stronger FEC: damage multiplier (Q2).
- Late entry worst case 540 ms, mean 360 ms (Q7).
- CAI = air; ISSI/CSSI = SIP/RTP over IP.
