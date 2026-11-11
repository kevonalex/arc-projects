# Pack 04 — TETRA (EN 300 392), dPMR (6.25 kHz FDMA) and MPT 1327 (1200 bit/s FFSK)

> **SAFETY / LEGAL.** Everything here uses **synthetic** frames and arithmetic you produce in unit tests.
> **Never** receive, record or decode live public-safety / commercial radio traffic with anything from this pack
> (or an SDR). It may be unlawful, and encrypted traffic is off-limits by design. Studying the structure of
> open-source decoders (OP25, dsd-fme, SDRTrunk) is optional; **do not copy their copyrighted code**.

Standards (numbers from memory — **VERIFY**; I never invent clause numbers, "clause VERIFY" = look it up):
ETSI **EN 300 392** (TETRA Voice + Data; part 2 = air interface, part 3 = interworking/ISI, part 7 = security),
ETSI EN 300 395 (TETRA speech codec), ETSI TS 102 490 / TS 102 658 (dPMR), **MPT 1327** (signalling) and
**MPT 1343** (radio interface), UK origin.

---

## 1. Why this matters (Etherstack context)

Etherstack's **core business is LMR protocol stacks** (P25, DMR, TETRA, dPMR, analogue/MPT-family… **VERIFY** the current
product list). Customers (radio vendors, infrastructure vendors, gateway/dispatch vendors) migrate between
generations and technologies, so a stack team must be fluent in *all* of them: TETRA is the dominant public-safety
standard in much of Europe/Middle East/Asia-Pacific; dPMR is the FDMA narrowband sibling of DMR; MPT 1327 is the
legacy analogue trunking signalling still running in many fleets. A single interview question might ask you to
**compare** them from first principles — this pack gives you the common frame: *bandwidth per call, bit
budget, layer model*.

## 2. Prerequisites

- Pack 01 §3.1 (symbols, dibits) and §3.3 (cyclic codes: shift, divide, append remainder).
- Pack 02 §3.4 (differential QPSK, π/4 family) and §3.2 (TDMA arithmetic).
- Pack 03 (bursts, colour code idea).
- C++17, GoogleTest, Linux/macOS/WSL with `-Wall -Wextra -fsanitize=address,undefined`.

```bash
cd code
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure  # stubs: FAIL
cmake -S . -B build_sol -DLMR_USE_SOLUTIONS=ON && cmake --build build_sol -j && ctest --test-dir build_sol
```

## 3. Concept build-up

### 3.1 The common yardstick: *bandwidth per call*

Spectrum is licensed per channel, so compare technologies by **Hz per simultaneous call**:

| System | Channel | Calls per channel | Hz per call | Multiple access |
|---|---|---|---|---|
| Analogue FM (narrow) | 12.5 kHz | 1 | 12.5 kHz | FDMA |
| P25 Phase 1 | 12.5 kHz | 1 | 12.5 kHz | FDMA |
| P25 Phase 2 | 12.5 kHz | 2 | 6.25 kHz | TDMA |
| DMR | 12.5 kHz | 2 | 6.25 kHz | TDMA |
| **dPMR** | **6.25 kHz** | 1 | 6.25 kHz | **FDMA** |
| **TETRA** | **25 kHz** | **4** | 6.25 kHz | TDMA |

The industry converged on **6.25 kHz per call** (regulators' "narrowbanding" goals): DMR/P25p2/TETRA reach it
by slicing time; dPMR by slicing frequency. (VERIFY: exact regulatory targets by region.)

### 3.2 TETRA's time hierarchy

TETRA (Terrestrial Trunked Radio): **25 kHz carrier**, **π/4-DQPSK at 18 000 symbols/s** ⇒ 2 bits/symbol ⇒
**36 000 bit/s gross**.

| Unit | Contents | Size / time |
|---|---|---|
| **Slot** | 255 symbols = **510 bits** | 510 / 36000 = **14.167 ms** |
| **Frame** | 4 slots (timeslots 1–4) | 2040 bits = **56.667 ms** |
| **Multiframe** | 18 frames; **frame 18 = control frame** | **1.02 s** |
| **Hyperframe** | 60 multiframes | **61.2 s** |

Numbering is 1-based (timeslot 1–4, frame 1–18, multiframe 1–60 — VERIFY, EN 300 392-2 clause on time structure).

*Why four slots?* 25 kHz / 4 = 6.25 kHz per call, matching §3.1. *Why a control frame (frame 18)?* A trunked
network must always be able to reach every mobile with signalling and let mobiles monitor neighbouring cells; reserving
one frame in 18 (5.6 %) gives a **guaranteed** signalling opportunity without stealing a speech burst.
(VERIFY detail of how mobiles use it.)

### 3.3 Rates — derive the net rate, don't recite it

Per traffic slot the normal burst carries **2 × 216 = 432** user/coded bits; the other 78 bits of the 510 are
training sequence, tail, phase adjustment and broadcast/signalling bits (VERIFY breakdown).

- Traffic frames per multiframe: **17** (frame 18 is control).
- **Net rate of one timeslot, unprotected data (TCH/7.2):** 432 bits × 17 frames ÷ 1.02 s = **7200 bit/s** ✔ (the "7.2 kbit/s per timeslot" figure).
- Four timeslots: **28 800 bit/s** per carrier = 80 % of gross.

**Speech.** The TETRA speech codec is ACELP: **137 bits per 30 ms** frame = 4.567 kbit/s. A slot carries **two** speech frames
(60 ms of speech; 274 bits, channel-coded to 432). Each traffic slot recurs every 56.667 ms with 60 ms of speech
in it; across a multiframe 17 slots × 60 ms = **1020 ms = exactly one multiframe**: the speech fills the schedule exactly
(Q2) — that is *why* the codec frame is 30 ms and not 20 ms.

*Spectral efficiency:* 36 000 / 25 000 = 1.44 bit/s/Hz gross — higher than P25 Phase 2 (0.96): π/4-DQPSK at 18 ksym/s with
roll-off 0.35 occupies ≈ 18 000 × 1.35 = 24.3 kHz (VERIFY roll-off), nearly filling the 25 kHz carrier, whereas P25/DMR
keep a 12.5 kHz mask with 4800–6000 sym/s.

### 3.4 Modulation: π/4-DQPSK

Information is carried in phase *changes* (see pack 02 §3.4): bit pairs `00 → +π/4`, `01 → +3π/4`, `10 → −π/4`, `11 → −3π/4`
(VERIFY bit-pair order). Differential detection makes the receiver robust to unknown carrier phase; the π/4 family never
makes a 180° flip so the envelope doesn't pass through zero. **Cost:** it's a *linear* modulation, so TETRA handset/base PAs
must be linearised (less power-efficient than FM-based C4FM/4FSK). TETRA accepted that for spectral efficiency.

### 3.5 Logical structure (names to know — VERIFY each against EN 300 392-2)

- **Normal burst** (traffic and signalling): 2 × 216-bit blocks + training sequence + tails + phase adjustment; downlink adds a
  **broadcast block** carrying the **AACH** (access assignment channel: who may use the next uplink slot).
- Logical channels: **BSCH** (synchronisation), **BNCH** (network broadcast), **AACH**, **SCH/F**, **SCH/HD**, **SCH/HU**
  (signalling, full/half slot), **STCH** (stealing channel: signalling stolen from traffic), **TCH** (traffic).
- The **MCCH** (main control channel) lives in timeslot 1 of the main carrier.
- **Colour code:** TETRA doesn't put a colour code in a field. An *extended colour code* (derived from MCC 10 bits + MNC 14 bits +
  CC 6 bits = 30 bits — VERIFY) **initialises the scrambler**; a receiver with the wrong code descrambles garbage and every CRC
  fails. Implicit tagging, same purpose as DMR's colour code / P25's NAC.

### 3.6 The protocol layers (and *why* each exists)

```
CMCE (calls, floor, SDS)     MM (who you are)      [SNDCP: packet data]
                 \               /
              MLE  (which cell / network)             <- layer 3
              LLC  (reliable link: ARQ, BL-ACK)       <- layer 2
              MAC  (slots, fragmentation, access)     <- layer 2
              PHY  (bursts, modulation, coding)       <- layer 1
```

| Layer | Question it answers | Example PDUs (VERIFY) |
|---|---|---|
| **MAC** | When may I send, how do I split a message across slots, which logical channel? | `MAC-RESOURCE`, `MAC-ACCESS`, `MAC-DATA`, `MAC-FRAG`, `MAC-END` |
| **LLC** | Did the message arrive? (ack/retransmit) | `BL-ADATA`, `BL-ACK`, `BL-DATA`, `AL-…` |
| **MLE** | Which cell am I on, should I roam, am I attached to this network? Hides mobility from layers above. | `D-NWRK-BROADCAST`, `D-NEW-CELL`, `U-PREPARE` |
| **CMCE** | What *call* services: setup, release, **floor control**, short data (SDS) | `D-SETUP`, `U-SETUP`, `D-CONNECT`, `U-TX DEMAND`, `D-TX GRANTED`, `D-TX CEASED`, `D-SDS-DATA` |
| **MM** | Who am I, am I allowed (registration, authentication, group attach) | `U-LOCATION UPDATE DEMAND`, `D-LOCATION UPDATE ACCEPT`, `U-ATTACH/DETACH GROUP IDENTITY` |

*Why layered like this?* Each layer hides a different kind of change: radio conditions (MAC/LLC), network topology (MLE),
services (CMCE), subscriber state (MM). You can add a call type without touching the radio layers.

**Identities:** ITSI = MCC (10 bits) + MNC (14 bits) + SSI (24 bits) = **48 bits** (VERIFY); within a network only the 24-bit SSI is
used on air. Compare P25's 56-bit global ID (WACN 20 + SysID 12 + unit 24) from pack 01.

**Security note (VERIFY):** TETRA air-interface encryption uses TEA1–TEA4; in 2023 independent researchers published weaknesses in
TEA1 and other issues ("TETRA:BURST"). In an interview: say *encryption algorithm choice and key management matter more than
the protocol framing*.

### 3.7 dPMR: FDMA at 6.25 kHz

- **6.25 kHz** channel, **4FSK at 2400 symbols/s** ⇒ 2 bits/symbol ⇒ **4800 bit/s**, one call per channel (FDMA).
- Spectral efficiency 4800 / 6250 = **0.768 bit/s/Hz** — *identical* to P25 Phase 1 (9600/12500) and to DMR's gross.
- Per call: dPMR 4800 bit/s, DMR (9600 / 2) 4800 bit/s — **the same bits per call in the same Hz per call**. dPMR is the
  frequency-division twin of a single DMR slot. Vocoder is AMBE+2 class (≈ 3600 bit/s on air, 75 % of 4800 — VERIFY).
- Modes (VERIFY): Mode 1 peer-to-peer, Mode 2 repeater/conventional, Mode 3 trunked/multi-site. Standards TS 102 490 / TS 102 658.

*Why FDMA?* **No slot timing, no guard/ramp overhead, simple radios**, continuous transmission, low latency. *Cost:* a repeater needs
one transmitter and filter per call; channel filters and oscillators must be tight. **Frequency stability at 6.25 kHz:** at
450 MHz, 6.25 kHz = 13.9 ppm *total channel width*, so an oscillator error of ±1 ppm already shifts the carrier by ±450 Hz = 7 % of the channel (Q5).

### 3.8 MPT 1327: signalling on an analogue channel

MPT 1327 (with MPT 1343 for the radio interface) is a UK-originated **analogue trunking** signalling standard still in use.

- **1200 bit/s FFSK** (fast frequency-shift keying) as an *audio-band subcarrier* inside a normal analogue FM channel: `1200 Hz` and
  `1800 Hz` tones (VERIFY mapping of tone to bit value).
- *Why those tones?* Baud = 1200. Tone spacing = 600 Hz = baud/2 ⇒ modulation index h = 0.5 (**MSK**: the tones differ by exactly
  *half a cycle per bit*: 1200 Hz = 1.0 cycle/bit, 1800 Hz = 1.5 cycles/bit). That is the *minimum* spacing at which the two tones are
  orthogonal over one bit — maximum bits per Hz without interference.
- *Why in the audio band?* Existing analogue FM radios and voice paths can carry it; no new RF hardware — a retrofit-friendly design.
- **Codeword = 64 bits** = **48 information** + **15 check bits** + **1 parity**; check polynomial
  g(x) = x¹⁵ + x¹⁴ + x¹³ + x¹¹ + x⁴ + x² + 1 = `0xE815` (VERIFY; the real standard also complements the check bits — this pack doesn't).
  64 bits at 1200 bit/s = **53.33 ms**; information throughput 48/53.33 ms = **900 bit/s** (75 %).
- *Why 15 check bits?* A CRC of degree *r* with a non-zero constant term **detects every error burst of length ≤ r**: any error
  pattern `x^k·b(x)` with deg b < 15 can't be divisible by g (see Q10 and the burst test). FM fades cause burst errors —
  detection of 15-bit bursts is exactly what's needed.
- Synchronisation: a preamble (alternating 1010…), then **SYNC `0xC4D7`** on a control channel; **SYNCT `0x3B28`** (its complement) on a
  traffic channel (VERIFY). Because SYNCT = ~SYNC the two can't be mistaken for each other (16-bit distance).
- Addressing: a 20-bit identity = 7-bit prefix (fleet) + 13-bit ident (VERIFY).

> **Misconception box — "TETRA is just DMR with more slots."** TETRA is a *different system*: 25 kHz carrier, linear π/4-DQPSK at 18 ksym/s,
> a full network stack (MM/MLE/CMCE), integrated trunking, ISI, DMO, security. DMR Tier II is conventional; Tier III trunking is a separate layer.

> **Misconception box — "FFSK is a digital protocol."** It's an *analogue audio-band* modem carrying digital codewords. The "protocol" is MPT 1327.

## 4. Fully worked example

**(a) TETRA time arithmetic.** Where is absolute slot 100 000?
timeslot = 100000 mod 4 = 0 → **TS 1**. frame_abs = ⌊100000/4⌋ = 25000; frame = 25000 mod 18 = 16 → **frame 17** (1-based). multiframe_abs =
⌊100000/72⌋ = 1388; multiframe = 1388 mod 60 = 8 → **multiframe 9**. Time = 100000 × 14.1667 ms ≈ 1416.7 s = 23.6 min.

**(b) Net rate.** 432 × 17 / 1.02 = 7344 / 1.02 = 7200 bit/s ✔; × 4 = 28 800; ÷ 36 000 = 0.8.

**(c) Speech fit.** 17 speech slots × 60 ms = 1020 ms = multiframe ✔. If the codec were 20 ms frames (3 per 60 ms) it would also fit
— but then the 137-bit/30 ms codec wouldn't match; TETRA chose codec framing to fit its slot.

**(d) MPT 1327 codeword by hand (toy).** For intuition use 4 info bits and g = x³ + x + 1 (pack 01 §4): data `1101` → shift 3 → remainder `001` →
`1101001`; even-parity bit: weight 4 → parity 0 → `11010010`. The real encoder is the same shape with 48 info bits, degree 15, 1 parity bit.

**(e) FFSK.** Bit period 1/1200 s = 833 µs: 1200 Hz tone completes 1.0 cycle, 1800 Hz completes 1.5 cycles.

## 5. Questions

All questions carry an **Etherstack context** line. Answers: `answers.md`. Code: `code/include/…`.

### Q1 — Explain
Why do DMR, P25 Phase 2, TETRA and dPMR all end up at 6.25 kHz per call, and what are the two ways (time slicing vs frequency slicing) of getting there?
*Etherstack context:* a customer comparing technologies asks you for a one-slide answer.

### Q2 — Explain
Why does TETRA's speech codec use 30 ms frames and *why* is the "control frame" (frame 18) not a waste? Show the arithmetic that makes speech fill the multiframe exactly.
*Etherstack context:* the scheduling constants in a TETRA stack's TCH path.

### Q3 — Calculate — **code: `tetra/timing.hpp`**
Derive: gross rate, slot/frame/multiframe/hyperframe durations, overhead in a normal burst.
*Etherstack context:* the base-station timing engine.

### Q4 — Calculate — **code: `tetra/timing.hpp`**
Derive TCH/7.2, per-carrier net rate, net/gross, ACELP rate, spectral efficiency (gross), and compare with P25 Phase 2.
*Etherstack context:* sizing IP backhaul for a TETRA site.

### Q5 — Calculate — **code: `lmr_legacy/legacy.hpp`**
(a) dPMR bit rate and spectral efficiency; (b) frequency tolerance in Hz for ±1 ppm at 450 MHz vs channel width; (c) MPT 1327 codeword duration and information throughput; (d) FFSK tone spacing.
*Etherstack context:* RF hardware assumptions behind "FDMA is simple".

### Q6 — Predict
(a) A TETRA mobile uses the wrong colour code. What does it see? (b) A DMR radio uses the wrong colour code. What does it see? (c) How do the failure modes differ for debugging?
*Etherstack context:* field issue "radio receives nothing" — which checks first?

### Q7 — Trace — **code: `tetra/timing.hpp`**
Trace absolute slots 0, 67, 68, 71, 72, 4320 into (timeslot, frame, multiframe). Which are control-frame slots?
*Etherstack context:* debugging frame-number-dependent features (e.g. encryption sync uses the hyperframe/multiframe/frame numbers — VERIFY).

### Q8 — Trace — **code: `tetra/layers.hpp`**
A handset presses PTT in a group call on TETRA. Trace the message from the CMCE down to the air, naming the layer and the PDU at each hop.
*Etherstack context:* floor control is the heart of an LMR stack (packs 05–06).

### Q9 — Find the bug — **code: `tetra/timing.hpp`, `legacy.hpp`**
```cpp
double slot_ms()  { return 510.0 / 18000 * 1000; }                  // (1)
double tch72()    { return 432 * 18 / 1.02; }                       // (2)
int timeslot(uint64_t n) { return n % 4; }                          // (3)
bool is_control(int frame) { return frame == 17; }                  // (4) frame numbers are 1-based
double tone_spacing(double baud) { return baud; }                   // (5)
```
Find all five bugs, state each symptom, fix them.
*Etherstack context:* first-draft timing code review.

### Q10 — Implement — **code: `legacy.hpp`**
Implement the MPT codeword (CRC-15 + parity) and prove (by test) that every burst of ≤ 15 bits in the body is detected. Why does it work?
*Etherstack context:* analogue-trunking interworking requires bit-exact codewords.

### Q11 — Implement — **code: `tetra/timing.hpp`, `tetra/layers.hpp`**
Make `test_tetra_timing` and `test_tetra_layers` pass.
*Etherstack context:* layer-aware logging/tracing is a daily tool.

### Q12 — Connect
A gateway joins a TETRA group call to a P25 Phase 1 talkgroup. Map identities, floor control messages, speech frames (30 ms vs 20 ms; transcoding buffer), emergency, encryption, SDS.
*Etherstack context:* IWF — pack 05 develops this further.

### Q13 — Interview
"Describe the TETRA protocol stack from the radio up, with one PDU per layer, and say what each layer hides."
*Etherstack context:* classic whiteboard question.

### Q14 — Interview
"dPMR is FDMA at 6.25 kHz; DMR is TDMA at 12.5 kHz. Same spectral efficiency. Which would you choose and why?"
*Etherstack context:* customer-facing technology advice.

### Q15 — Interview
"Why would a customer still run MPT 1327 and what must an LMR stack vendor provide for it?"
*Etherstack context:* legacy fleets are a real revenue stream.

## 6. Self-assessment

- [ ] I can derive 36 kbit/s, 510 bits, 14.167 ms, 56.667 ms, 1.02 s, 61.2 s.
- [ ] I can derive 7.2 kbit/s per slot and 28.8 kbit/s per carrier.
- [ ] I can explain why speech (17 × 60 ms) fills the multiframe exactly.
- [ ] I can name the layers MAC, LLC, MLE, CMCE, MM with example PDUs and say what each hides.
- [ ] I can explain TETRA's implicit colour code vs DMR's explicit one.
- [ ] I can compare dPMR with DMR per call (4800 bit/s, 6.25 kHz).
- [ ] I can derive FFSK tone spacing and the burst-detection property of the MPT CRC.
- [ ] Tests pass.

| Score | Meaning |
|---|---|
| 0–3 | Re-read §3.1–3.3 |
| 4–6 | Redo Q2, Q4, Q10 |
| 7–8 | Move to pack 05 |

### VERIFY list for this pack
1. TETRA constants: 510-bit slot, 4 slots, 18 frames, 60 multiframes, 1-based numbering, normal-burst overhead (78 bits) and breakdown, roll-off 0.35.
2. TETRA speech codec numbers (137 bits/30 ms; 274 → 432 coding) [EN 300 395, EN 300 392-2].
3. Logical channel names and the MCCH location; AACH; frame 18 usage.
4. Colour-code mechanism (30-bit extended colour code initialising the scrambler; field widths).
5. PDU names and which layer owns each; ITSI structure (10+14+24 bits); TEA1–TEA4 and the 2023 findings.
6. dPMR modes, vocoder rate, 80 ms/frame structure (not used here), standard numbers (TS 102 490/658).
7. MPT 1327: tone-to-bit mapping, 1200/1800 Hz, polynomial 0xE815, complemented check bits, SYNC/SYNCT values, 7+13-bit identity, codeword/frame structure [MPT 1327, MPT 1343].
