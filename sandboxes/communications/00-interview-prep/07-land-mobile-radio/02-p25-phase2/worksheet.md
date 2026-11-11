# Pack 02 — P25 Phase 2: two-slot TDMA, H-DQPSK / H-CPM, AMBE+2

> **SAFETY / LEGAL.** Everything here uses **synthetic** frames and arithmetic you generate yourself in unit
> tests. **Never** receive, record or decode live public-safety traffic with anything from this pack (or any
> SDR tool): it may be illegal, and encrypted traffic is off-limits by design. Reading open-source decoders
> (OP25, dsd-fme, SDRTrunk) to study *structure* is optional; **do not copy their copyrighted code**.

Standards: TIA-102.BBAB (Phase 2 physical layer), TIA-102.BBAC (Phase 2 MAC), TIA-102.BABA (IMBE, Phase 1),
AMBE+2 vocoder (proprietary, DVSI; referenced by TIA-102.BABG? — **VERIFY**). Document numbers are from
memory: **VERIFY** before quoting. I do not invent section numbers; "§ VERIFY" = look it up.

---

## 1. Why this matters (Etherstack context)

Etherstack's **core business is LMR protocol stacks** (P25, DMR, TETRA, dPMR… **VERIFY** current list). P25
customers moving from Phase 1 to Phase 2 (more calls per licensed channel) need a stack that handles **both**:
FDMA control and voice channels (Phase 1) *and* TDMA voice channels (Phase 2), with a different vocoder
(IMBE vs AMBE+2) and strict **time-slot timing**. Time is the new difficulty: Phase 1 is a stream you
parse; Phase 2 is a **schedule** you must keep.

Interview value: "Does TDMA double capacity?" is a classic trap question — the honest answer needs arithmetic
(§3.3) — and "what's different about implementing Phase 2?" tests whether you understand slot timing.

## 2. Prerequisites

- Pack 01 §3.1 (symbols, dibits, gross vs speech rate) and §3.6 (bit budgets).
- Basic trigonometry: angles in radians, π, wrap-around of angles.
- C++17, GoogleTest. Build as in pack 01 (`-Wall -Wextra -fsanitize=address,undefined`; use Linux/macOS/WSL).

```bash
cd code
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure   # stubs: FAIL
cmake -S . -B build_sol -DLMR_USE_SOLUTIONS=ON && cmake --build build_sol -j && ctest --test-dir build_sol
```

## 3. Concept build-up

### 3.1 Recap — what Phase 1 left on the table

Phase 1: 12.5 kHz, 4800 sym/s, 9600 bit/s gross, IMBE needs 7200 bit/s on air per call (pack 01 §3.6).
**One call per channel**, 75 % of the channel is voice. Licences are per channel and spectrum is scarce, so
regulators/agencies wanted **two calls in the same 12.5 kHz**.

### 3.2 TDMA, concretely

*TDMA = Time Division Multiple Access:* users share one frequency by **taking turns**.

Imagine two radios, A and B, one frequency. The time axis is chopped into **30 ms slots** (VERIFY):

```
time →   |slot0|slot1|slot2|slot3|slot4|slot5| ...
          A     B     A     B     A     B
```

A transmits in even slots, B in odd. Phase 2 sends **6000 symbols/s**, 2 bits per symbol ⇒ **12 000 bit/s**
gross (12 kbit/s). One slot = 6000 × 0.030 = **180 symbols = 360 bits**.

**12 slots = 360 ms = a superframe** (VERIFY). Logical channel 0 (even slots) and 1 (odd slots) each own **6
slots per superframe**.

### 3.3 "Why does TDMA double capacity?" — derive it honestly

Tempting answer: "two slots = twice the calls." Check the numbers:

| | Phase 1 | Phase 2 |
|---|---|---|
| Symbol rate | 4800 | 6000 (+25 %) |
| Gross rate | 9600 bit/s | 12 000 bit/s |
| Vocoder, on-air bits per call | IMBE: 144 bit/20 ms = **7200 bit/s** | AMBE+2: 72 bit/20 ms = **3600 bit/s** |
| Calls per 12.5 kHz | 1 | 2 |
| Voice share of gross | 7200 / 9600 = 75 % | 2 × 3600 / 12 000 = 60 % |
| Overhead | 2400 bit/s | 4800 bit/s |
| Spectral efficiency (gross) | 0.768 bit/s/Hz | 0.96 bit/s/Hz (+25 %) |

What this shows:

1. **Time-slicing alone does not create capacity.** Putting two **IMBE** calls (2 × 7200 = 14 400 bit/s) into
   a 12 000 bit/s carrier is impossible. Capacity doubles because the **vocoder halves** (7200 → 3600 bit/s):
   AMBE+2 is a *half-rate* codec at similar perceived quality (VERIFY subjective claims) — *and* because
   modulation was changed to push the gross rate up 25 % within the same 12.5 kHz mask.
2. **TDMA is the mechanism to *share* the faster pipe** between two half-rate calls without two transmitters,
   two combiners, or two licences.
3. **Overhead share rises** (40 % vs 25 %): each slot needs guard time, ramp-up/ramp-down of the PA, sync,
   signalling. TDMA pays a tax for sharing.

Side benefits: a **subscriber transmits only 50 % of the time** (duty cycle 1/2) — less heating, longer
battery for the same peak power; a repeater site needs one carrier for two calls (cheaper RF plant).

> **Misconception box — "TDMA doubles the bit rate."** The *gross* rate went up 25 %, not 100 %. What
> doubled is *calls*, thanks to a half-rate vocoder.

> **Misconception box — "Phase 2 speech is twice as good/bad."** Speech information: 2 × 2450 = 4900 bit/s per
> carrier vs 4400 (IMBE). Not double; quality per call is a vocoder-design question.

### 3.4 Modulation: H-DQPSK downlink, H-CPM uplink

Both carry **2 bits per symbol at 6000 sym/s** (so both = 12 kbit/s).

**Differential QPSK (DQPSK)** — information in the **change of phase** between consecutive symbols, not the
absolute phase. Teaching table (exact Phase 2 table: **VERIFY** TIA-102.BBAB):

| dibit | phase step |
|-------|-----------|
| 00 | +π/4  |
| 01 | +3π/4 |
| 10 | −π/4  |
| 11 | −3π/4 |

Every step is an **odd multiple of π/4**, so absolute phase alternates between two constellations (0, π/2, π,
3π/2) and (π/4, 3π/4, …). That's *π/4-DQPSK*: a transition never passes through the origin of the
IQ plane (no 180° flip), which keeps the envelope variation modest.

*Why differential?* A receiver doesn't know the carrier's absolute phase (oscillators, path length). With
absolute QPSK you need a *reference* (pilot or tracking loop); with differential you compare each symbol with
**the previous one**, so any **constant** phase offset cancels (see Q5). The price: noise corrupts two
consecutive comparisons per symbol (≈ 2–3 dB worse than coherent).

*Why different modulations up and down?*

- **Downlink (infrastructure → radio): H-DQPSK**, a *linear* modulation: the base station can afford a linear
  PA and wants *spectral efficiency*; compatible in spirit with Phase 1's linear simulcast mode (CQPSK/LSM).
- **Uplink (radio → infrastructure): H-CPM**, a constant-envelope (continuous-phase) modulation: **handheld**
  PAs run non-linear, efficient classes, and CPM's constant envelope allows it without spectral regrowth.
- The "H" stands for *harmonised* (VERIFY the exact meaning), i.e. chosen to coexist with Phase 1 receivers/
  networks.

### 3.5 Burst types and the superframe

In a 360 ms superframe one logical channel owns six slots. A voice **4V burst** carries **4 AMBE+2 frames**
(4 × 72 = 288 bits of the 360 bits; the other 72 bits are sync/ID/signalling overhead — **VERIFY** the
breakdown). A **2V burst** carries 2 frames. A **SACCH** slot carries slow signalling (no voice).

Teaching layout (real order **VERIFY** TIA-102.BBAC), per channel: `4V 4V 4V 4V 2V SACCH` ⇒ 4+4+4+4+2 = **18 voice
frames** = 18 × 20 ms = **360 ms** ✔ — the voice must exactly fill the superframe.

Logical channels (VERIFY names): **VCH** voice; **SACCH** slow associated control (always there, low rate);
**FACCH** fast associated control — it *steals* a voice burst when urgent signalling must go out *now*
(e.g. a transmitter-ending message, an encryption change); the stolen voice is lost (concealed). *Why steal?*
Waiting for the next SACCH slot would take up to 360 ms; urgent messages can't wait, and a lost 80 ms of
voice is cheaper than a late "end of call".

> **Misconception box — "a slot is 30 ms of one voice frame."** A 4V slot carries **80 ms** of speech in 30 ms of
> air time. The radio must buffer ≥ 80 ms before it can send the first burst: TDMA adds latency.

### 3.6 Encryption sync and late entry (same idea as Phase 1)

Encryption sync (ESS: algorithm ID, key ID, message indicator) is spread over the voice bursts so that a
radio that joins mid-call can pick it up within a superframe (VERIFY ESS-A/ESS-B split) — the same *repeat for
late entry* principle as Phase 1's LDU2 (pack 01 §3.7).

### 3.7 Coexistence with Phase 1; vocoders

A system can use a Phase 1 (FDMA) control channel and mixed FDMA/TDMA voice channels (VERIFY dynamic
switching rules). A stack therefore needs *per-channel mode*, a vocoder abstraction (IMBE ↔ AMBE+2) and a
**transcoder** at gateways (pack 05, Q-IWF). IMBE and AMBE+2 are **licensed proprietary** vocoders (VERIFY
licensing model); stacks integrate licensed implementations rather than ship their own.

### 3.8 Time as a data structure

A software TDMA stack keeps an **absolute symbol counter** and derives everything:

```
slot_abs         = symbol / 180
superframe       = slot_abs / 12
slot_in_sf       = slot_abs % 12
logical channel  = slot_in_sf % 2
```

Pitfall: use a 64-bit counter. A `uint32_t` symbol counter overflows after 2³² / 6000 ≈ 8.3 days, and 2³² is
*not* a multiple of 180, so the slot phase **jumps by 76 symbols** at wrap (Q8).

## 4. Fully worked example

**(a) Slot arithmetic.** Where is symbol 10 000?
slot_abs = ⌊10000 / 180⌋ = 55 (55 × 180 = 9900); superframe = ⌊55/12⌋ = 4 (48 slots); slot_in_sf = 55 − 48 = 7;
channel = 7 mod 2 = 1; symbol-in-slot = 10000 − 9900 = 100. Time = 10000 / 6000 = 1.667 s.

**(b) Capacity.** Gross 12 000; voice 2 × 3600 = 7200; overhead = 4800 bit/s. A third call would need
10 800 bit/s ⇒ total 15 600 > 12 000: impossible. Max overhead for two calls = 12 000 − 7200 = 4800.

**(c) DQPSK by hand.** Start phase 0. Dibits 00, 01, 10, 11:
- 00: 0 + π/4 = **π/4**
- 01: π/4 + 3π/4 = π → **π** (π is in (−π, π])
- 10: π − π/4 = **3π/4**
- 11: 3π/4 − 3π/4 = **0**
Check the π/4 property: after 1 step π/4 (odd multiple of π/4), after 2 steps π (multiple of π/2), 3 steps 3π/4, 4
steps 0 ✔. Now let the channel add θ = +0.5 rad to every sample: phases become π/4+0.5, π+0.5, 3π/4+0.5, 0.5.
Differences: (π+0.5)−(π/4+0.5) = 3π/4 → 01 ✔ … the 0.5 cancels in every difference. (You need one *reference* symbol
first — that's why Phase 2 bursts begin with sync.)

## 5. Questions

Each has an **Etherstack context** line. Answers: `answers.md`. Code: `code/include/p25p2/*.hpp`.

### Q1 — Explain
"TDMA doubles capacity." Explain precisely why that is only half true, using numbers.
*Etherstack context:* a customer asks "why do I need new vocoder licences for Phase 2?" — this is the answer.

### Q2 — Explain
Why does Phase 2 use a linear H-DQPSK downlink but a constant-envelope H-CPM uplink, even though both are
6000 sym/s and 12 kbit/s?
*Etherstack context:* a stack's PHY abstraction must distinguish uplink/downlink burst formats.

### Q3 — Calculate — **code: `timing.hpp`**
Derive: gross bit rate, symbols/slot, bits/slot, superframe duration, slots per logical channel, voice frames per
channel per superframe, overhead in a 4V slot.
*Etherstack context:* these constants end up in every scheduler and every conformance test.

### Q4 — Calculate — **code: `timing.hpp`**
(a) Spectral efficiency Phase 1 vs Phase 2. (b) Transmit duty cycle. (c) Maximum overhead that still allows two
AMBE+2 calls; can two IMBE calls fit? (d) Voice share of gross rate in each phase.
*Etherstack context:* the maths behind marketing claims ("2× capacity") that sales engineers must defend.

### Q5 — Predict — **code: `dqpsk.hpp`**
(a) The channel rotates all phases by 0.7 rad. What does a differential decoder output? An absolute-phase QPSK
decoder? (b) What if instead of a constant offset there is a *frequency* offset of 500 Hz at 6000 sym/s — what
happens to each step? At what offset does decoding start to fail (ignoring noise)?
*Etherstack context:* oscillator drift in cheap radios is a real interop problem.

### Q6 — Trace — **code: `timing.hpp`**
Locate symbols 5399, 5400, and 2 160 000 (superframe, slot, channel, symbol-in-slot, time).
*Etherstack context:* debugging a slot-slip bug means being able to do this by hand.

### Q7 — Trace
Using the teaching pattern `4V 4V 4V 4V 2V SACCH`, list for logical channel 0 the **start time** (ms) of each of its
six slots in a superframe and which speech interval (ms) each burst carries. How much buffering delay does
TDMA add at the transmitter, minimum and worst case?
*Etherstack context:* latency budgets for PTT-to-speech through a gateway.

### Q8 — Find the bug — **code: `timing.hpp`**
```cpp
constexpr int kSymbolRate = 4800;                          // (1)
int symbols_per_slot() { return kSymbolRate * 30; }        // (2)
int bits_per_slot()    { return symbols_per_slot(); }      // (3)
int channel_of_slot(int slot) { return slot / 2; }         // (4)
uint32_t symbol_counter;  void on_symbol() { ++symbol_counter; }   // (5)
```
Find and fix all five; state the symptom of each.
*Etherstack context:* typical review findings in timing code of a base-station stack.

### Q9 — Implement — **code: `timing.hpp`**
Make `test_timing` pass.
*Etherstack context:* exactly the constants/arithmetic a TDMA scheduler is built on.

### Q10 — Implement — **code: `dqpsk.hpp`**
Make `test_dqpsk` pass (encode, decode, blind decode).
*Etherstack context:* knowing the differential principle explains why Phase 2 receivers can be simple.

### Q11 — Connect
A site runs a Phase 1 FDMA control channel and Phase 2 TDMA voice channels. A Phase 1 radio and a Phase 2 radio
are both affiliated to talkgroup 100 and a gateway forwards the call over the ISSI. Sketch the stack's
responsibilities: which vocoder each leg uses, where transcoding occurs, what per-channel state is needed.
*Etherstack context:* dual-mode stacks and gateways are exactly the product surface.

### Q12 — Interview
"Does TDMA double the capacity of a channel?"
*Etherstack context:* sales-engineering style question.

### Q13 — Interview
"You're asked to port a Phase 1 stack to Phase 2. What's fundamentally different, and what would you test first?"
*Etherstack context:* a realistic project discussion.

## 6. Self-assessment

- [ ] I can derive 12 kbit/s, 180 symbols, 360 bits, 12 slots, 360 ms without notes.
- [ ] I can explain why vocoder halving, not time-slicing, creates the doubling.
- [ ] I can explain differential modulation and why a constant phase offset cancels.
- [ ] I can say why uplink and downlink use different modulations.
- [ ] I can do slot arithmetic from a symbol index.
- [ ] I can list the five bugs of Q8.
- [ ] All tests pass with my implementations.

| Score | Meaning |
|---|---|
| 0–3 | Redo §3.2–3.3 by hand |
| 4–6 | Redo Q4, Q5, Q7 |
| 7 | Go to pack 03 |

### VERIFY list for this pack
1. Slot duration (30 ms) and superframe composition (12 slots / 360 ms) [TIA-102.BBAB/BBAC].
2. Burst overhead breakdown (72 bits in 4V), exact superframe ordering, ISCH/SACCH placement.
3. Dibit→phase mapping for H-DQPSK; meaning of "H"; H-CPM parameters.
4. AMBE+2 half-rate: 72 bits per 20 ms (49 + 23) as used in Phase 2.
5. FACCH/SACCH/ESS naming and ESS placement.
6. Dynamic Phase 1 / Phase 2 coexistence rules; vocoder document reference and licensing model.
