# Pack 02 — Answers (P25 Phase 2)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → angle.**
Synthetic data only. **VERIFY** = check against TIA-102.BBAB / BBAC before quoting.

---

## Q1 — Explain: "TDMA doubles capacity"

**Answer.** *Calls per channel* double (1 → 2). The *bit rate* does not (+25 %: 9600 → 12 000). The doubling
exists only because the per-call on-air rate halves: IMBE 7200 bit/s → AMBE+2 3600 bit/s. TDMA is the mechanism
that lets two half-rate calls share one carrier.

**Reasoning.**
- Phase 1 call = 7200 bit/s on air. Two of them = 14 400 > 12 000 available in Phase 2 ⇒ impossible.
- Phase 2 call = 3600 bit/s. Two = 7200 ≤ 12 000 − overhead (4800) ✓.
- Speech information: IMBE 4400 bit/s per call; AMBE+2 2450 bit/s × 2 = 4900 bit/s per carrier ⇒ almost the same
  information throughput; the gain is that **it's spread over two users**.
- Spectral efficiency: 0.768 → 0.96 bit/s/Hz (+25 %) from the higher symbol rate and linear/CPM modulation.
- Overhead rises (25 % → 40 %): guard/ramp/sync/signalling per slot.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Two slots ⇒ twice the bit rate." | Two of something. | Slots share the bits; they don't add any. The rate rose 25 %, from symbol rate. |
| "TDMA uses twice the bandwidth." | More capacity must cost something. | Same 12.5 kHz; it costs vocoder rate and overhead. |
| "AMBE+2 is just better quality." | Marketing. | It's *lower rate*; quality is comparable (VERIFY), not better. |
| "You could keep IMBE and TDMA." | Why change codec? | 2 × 7200 > 12 000. |

**Interview angle.** Say "half-rate vocoder" in the first sentence, then give the numbers.

---

## Q2 — Explain: H-DQPSK down, H-CPM up

**Answer.** Handhelds (uplink) are battery-/cost-constrained and use non-linear, efficient PAs; a **constant-
envelope** modulation (CPM) tolerates that without spectral splatter. The base station (downlink) can afford linear
amplification and wants spectral efficiency and compatibility with linear simulcast reception, so it uses a
linear (envelope-varying) **DQPSK**. Rates are equal (6000 sym/s × 2 bit) because both must fit the same 12.5 kHz
mask and slot structure.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "DQPSK has a higher bit rate than CPM." | Different schemes ⇒ different speed. | Both 6000 sym/s × 2 bits. |
| "The uplink needs more robustness so it uses CPM's FEC." | Uplinks are weaker. | The choice is about PA linearity, not coding. |
| "H-DQPSK is coherent QPSK." | QPSK familiarity. | It is *differential*: information in phase change. |
| "H = high speed." | Letter H. | "Harmonised" (VERIFY). |

---

## Q3 — Calculate

| Quantity | Derivation | Value |
|---|---|---|
| gross rate | 6000 sym/s × 2 | **12 000 bit/s** |
| symbols/slot | 6000 × 0.030 | **180** |
| bits/slot | 180 × 2 | **360** |
| superframe | 12 slots × 30 ms | **360 ms** |
| slots/channel/superframe | 12 / 2 | **6** |
| voice frames/channel/superframe | 4+4+4+4+2 (+0 SACCH) = 18; check 360 ms / 20 ms | **18** |
| 4V overhead | 360 − 4 × 72 | **72 bits** (VERIFY meaning) |

Consistency check: 12 × 180 symbols = 2160 symbols = 360 ms × 6000/s ✔.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "bits per slot = 180" | Confusing symbols and bits. | 2 bits per symbol. |
| "superframe = 6 slots = 180 ms" | Counting only one channel's slots. | The carrier has 12 slots; one *channel* has 6. |
| "6000 × 30 = 180 000 symbols" | Forgetting ms → s. | 30 ms = 0.030 s. |
| "18 frames = 6 slots × 3" | Even split. | Bursts differ: 4V, 2V, SACCH. |

---

## Q4 — Calculate

(a) Phase 1: 9600 / 12 500 = **0.768**; Phase 2: 12 000 / 12 500 = **0.96** bit/s/Hz (×1.25).
(b) Duty cycle = 1/2 = **50 %**.
(c) Overhead budget for two AMBE+2 calls: 12 000 − 2 × 3600 = **4800 bit/s**; two IMBE calls: (12 000 − 4800)/7200 =
**1**, they don't fit (would need 14 400 alone).
(d) Voice share: Phase 1 7200/9600 = **75 %**; Phase 2 7200/12 000 = **60 %**. TDMA has *more* overhead share.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Spectral efficiency doubled." | 2 calls. | Gross efficiency +25 %. Per-call it halved. |
| "Duty cycle 100 % in TDMA." | The radio is always on. | The *PA* transmits only in its slot; receive may run in the other. |
| "Calls = gross/per-call = 12 000/3600 = 3." | Ignoring overhead. | Overhead (sync, guard, signalling) is not available to voice: (12 000 − 4800)/3600 = 2. |

---

## Q5 — Predict

(a) **Differential decode:** output unchanged (0.7 rad cancels in each difference). **Absolute QPSK:** every symbol
is rotated 0.7 rad ≈ 40°; the decision boundary lies at 45°, so noise-free symbols are barely inside, and with a
little noise (or an offset > π/4 = 0.785 rad) the decoder is wrong for the *whole* burst — absolute schemes need a
phase reference/tracking loop.
(b) Frequency offset f = 500 Hz ⇒ extra phase advance per symbol δ = 2π f / R_s = 2π × 500/6000 = π/6 ≈ 0.52 rad
(30°), added to *every* step. Steps are π/4 apart from the nearest decision boundary: a step now sits at π/4 + π/6
= 5π/12 (75°) vs boundary at π/2 (90°) — still correct, but noise margin fell from 45° to 15°. Failure (ignoring
noise) when |δ| > π/4 ⇒ f > R_s/8 = **750 Hz**. Constant phase offset is free, frequency offset is not.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Differential decode is immune to all carrier errors." | It handled phase offset. | A frequency offset is a *growing* phase offset: it changes every step by δ. |
| "A 0.7 rad offset breaks differential decoding." | Any offset is bad. | Differences cancel constants exactly. |
| "Absolute QPSK would be fine below π/2." | Quadrant 90°. | Decision regions are ±π/4 around each point. |

---

## Q6 — Trace

- Symbol 5399: slot_abs = ⌊5399/180⌋ = 29 (29×180 = 5220); symbol-in-slot = 179. Superframe ⌊29/12⌋ = 2; slot_in_sf = 29 − 24 = **5**;
  channel 5 mod 2 = **1**. Time = 5399/6000 = 0.8998 s.
- Symbol 5400: slot_abs = 30; symbol-in-slot 0; superframe 2; slot_in_sf **6**; channel **0** — the *last symbol of one
  slot is channel 1; the next symbol belongs to channel 0*. Time 0.9 s exactly.
- Symbol 2 160 000: slot_abs = 12 000; superframe = 1000; slot_in_sf 0; channel 0; symbol 0. Time = 360 s = 1000 × 360 ms ✔.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "channel = slot_abs / 2" | Divide to get half. | It alternates: use modulo. |
| "superframe = symbol / 360" | 360 ms or bits. | 2160 symbols per superframe. |

---

## Q7 — Trace

Channel 0 owns even slots: starts at 0, 60, 120, 180, 240, 300 ms.

| Slot | Start | Burst | Speech interval | Cumulative frames |
|---|---|---|---|---|
| 0 | 0 ms | 4V | 0–80 ms | 4 |
| 2 | 60 | 4V | 80–160 | 8 |
| 4 | 120 | 4V | 160–240 | 12 |
| 6 | 180 | 4V | 240–320 | 16 |
| 8 | 240 | 2V | 320–360 | 18 |
| 10 | 300 | SACCH | — | 18 |

The *speech* in a 4V burst spans 80 ms but the burst occurs every 60 ms: five voice bursts per 360 ms average
72 ms of speech spacing — fine because SACCH slots carry no voice. **Latency:** to send a 4V burst the encoder must have
collected 4 × 20 ms = **80 ms** of speech, then wait for the slot (0–60 ms): ≥ 80 ms, up to ≈ 140 ms of
transmit-side buffering (plus propagation, plus receive buffering/playout). Phase 1 can emit each 20 ms IMBE frame
promptly (it still groups into 180 ms LDUs for LC/ES).
(Pattern is a teaching layout — VERIFY real order.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Slot starts: 0, 30, 60…" | 30 ms slots. | Those are all slots; a channel gets *every second*. |
| "Latency 20 ms (one frame)." | Frame is 20 ms. | The 4V burst needs 4 frames queued. |

---

## Q8 — Find the bug

1. `kSymbolRate = 4800` — Phase 1 constant; Phase 2 is **6000**. *Symptom:* slots 20 % too long; every schedule slips.
2. `kSymbolRate * 30` — units: 30 **ms** not seconds. Should be `kSymbolRate * 30 / 1000` = 180. *Symptom:* 180 000 symbols/slot.
3. `bits_per_slot()` returns the symbol count — ×2 missing. *Symptom:* half the bits; payload splits off by 2×.
4. `slot / 2` — should be `slot % 2`; `/` gives 0,0,1,1,2,2…, not alternation. *Symptom:* "channel" index grows unboundedly.
5. `uint32_t` symbol counter wraps after 2³²/6000 ≈ 715 828 s ≈ **8.3 days**; 2³² mod 180 = 76 ⇒ slot phase jumps 76
   symbols. Use `uint64_t`. *Symptom:* base station misaligned after ~8 days — notoriously hard to reproduce.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Finding only (4). | The visible logic error. | (1)–(3) are unit mistakes, (5) is a long-run bug. |
| "(5) uint32 is fine: 8 days is long." | Rare. | Base stations run for years. |

---

## Q9 / Q10 — Implement

Reference: `code/solutions/p25p2/timing.hpp`, `dqpsk.hpp`.
Design points:
- `position_of_symbol`: use `uint64_t`; derive in order slot_abs → superframe → slot_in_sf → channel.
- `calls_that_fit`: clamp negative to 0; integer floor.
- `differential_encode`: `phase = wrap(phase + step)`.
- `phase_step_to_dibit`: wrap first; boundaries at 0 and ±π/2 (midway between ±π/4 and ±3π/4), thresholds
  at 0, π/2, −π/2, π. Because each step is an odd multiple of π/4, the margin to every boundary is π/4.
- `differential_decode_blind`: n phases → n−1 dibits; the first phase is the reference.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Forgetting `wrap_phase` before slicing. | Differences are small. | Differences like (−π+0.1) − (π−0.1) ≈ −2π+0.2 → wrong without wrapping. |
| Using `float`. | Less memory. | Accumulated error in long bursts; `double` is safer; (also equality tests). |
| Returning n dibits from blind decode. | Symmetric. | The first phase has no predecessor. |

---

## Q11 — Connect

Model answer:
- **Per channel state:** mode (FDMA P25p1 / TDMA P25p2), slot clock (symbol counter, 64-bit), logical channel
  ownership, burst scheduler (4V/2V/SACCH/FACCH insertion).
- **Per leg vocoder:** Phase 1 radio ↔ IMBE (7200 bit/s on air, 20 ms frames in LDUs); Phase 2 radio ↔ AMBE+2 (3600
  bit/s, 4V/2V bursts). Both frames are 20 ms of speech, so transcoding is **frame-synchronous** (one-in, one-out) with
  no resampling — only parameter re-quantisation (decode to PCM/parameters and re-encode ⇒ *tandem coding* quality
  loss; VERIFY alternatives such as parametric transcoding).
- **Where to transcode:** at the gateway/infrastructure (RFSS or console server), not in radios. The ISSI carries
  one agreed encoding per call (negotiated in SIP/SDP — VERIFY), so a mixed call forces a transcoder at the border.
- **Control:** Phase 1 control channel messages (TSBK) *grant* channels; the grant specifies the voice channel and
  mode; the stack must switch each leg's framing accordingly.
- **Encryption:** an encrypted call sent end-to-end must not be transcoded (cipher sits on the vocoder bits) —
  mixed-mode encrypted calls need key sharing at the transcoder or are unsupported.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Radios transcode." | They hold the vocoder. | Radios only speak their own mode. |
| "Just copy the IMBE bits into the AMBE slot." | Both are voice frames. | Different codecs & bit allocations. |
| "Transcoding is lossless." | Digital. | Lossy re-quantisation; tandem coding degrades audio. |

---

## Q12 — Interview: does TDMA double capacity?

**Model answer (45 s):** "Calls per 12.5 kHz double — from one to two — but the bit rate doesn't. Phase 2 raises the
gross rate only 25 % (4800→6000 sym/s). The doubling comes from a half-rate vocoder: IMBE needs 7200 bit/s on air per
call, AMBE+2 3600, so two calls = 7200 of 12 000 with 4800 overhead. TDMA is the time-sharing mechanism. The cost is
overhead share (40 vs 25 %), strict slot timing, and some extra latency — but battery duty cycle halves."

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Yes, two slots = two channels." | Simple. | Misses vocoder and overhead. |
| "No, it doesn't." | Pedantic. | Calls *do* double; explain the mechanism. |

---

## Q13 — Interview: porting Phase 1 → Phase 2

**Model answer.** *Fundamental differences:* (1) **time**: a schedule (slot clock, uplink timing alignment, guard,
PA ramp) rather than a stream; (2) **burst formats** and **logical channels** (VCH/SACCH/FACCH, stolen bursts);
(3) **vocoder**: AMBE+2 frame packing in 4V/2V; (4) **modulation**: DQPSK down, CPM up (DSP, not just parsing);
(5) **dual mode**: both phases in one product, channel-by-channel; (6) **latency**: buffering of 4 frames.
*Test first:* slot arithmetic and the 64-bit counter; round-trip burst encode/decode on synthetic data; FACCH
stealing at every slot; late entry joined at every offset; long-run (days) timing with accelerated clock; interop
with Phase 1 control channel grants. Always synthetic data.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Mostly the same, change the constants." | Same family. | Timing/scheduling and modulation are structurally new. |
| "Test with live traffic recordings." | Realism. | Illegal/inadvisable; synthetic vectors are better for coverage. |

---

## Self-check

- 12 000 bit/s, 180 symbols, 360 bits, 12 slots, 360 ms ✔; vocoder halving explains capacity ✔; constant phase offset
  cancels, frequency offset (> R_s/8) does not ✔.
