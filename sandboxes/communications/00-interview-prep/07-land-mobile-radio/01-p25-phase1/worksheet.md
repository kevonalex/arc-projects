# Pack 01 — P25 Phase 1 (TIA-102 CAI): FDMA, C4FM, NID, LDU anatomy

> **SAFETY / LEGAL — read first.** Every frame in this pack is **synthetic**: bits you generate yourself in
> a unit test. You must **never** point software from this pack at a live public-safety channel, an SDR
> tuned to a real P25 system, or a recording of one. Receiving or decoding live public-safety traffic can
> be unlawful in many jurisdictions (e.g. interception/wiretap law, and encrypted traffic is not yours to
> decode). We learn the *format*, not anybody's *conversations*. Reading open-source decoders (OP25,
> dsd-fme, SDRTrunk) as a *study of structure* is optional; **do not copy their (copyrighted) code**.

Standards cited: TIA-102.BAAA (Common Air Interface, "CAI"), TIA-102.BABA (IMBE vocoder), TIA-102.AABC
(trunking control channel messages), TIA-102.BACA (ISSI), TIA-102.BAHA (DFSI). Document numbers/revisions
are from memory — mark **VERIFY** wherever you see it and check the current TIA catalogue before quoting
in an interview. I deliberately do **not** invent section numbers: "§ VERIFY" means "go and look".

---

## 1. Why this matters (Etherstack context)

Etherstack's **core business is LMR (Land Mobile Radio) protocol stacks**: software that implements the
radio protocols (P25, DMR, TETRA, dPMR, NXDN, analogue/MPT1327 families — **VERIFY** the current product
list on etherstack.com) so that a radio vendor, a network infrastructure vendor, or a gateway vendor does
not have to write them from scratch. This pack does not claim inside knowledge of their code. It teaches
what *any* engineer working on such a stack must be able to do from a blank page:

1. Take a **bit stream** and find where frames start (sync).
2. Decide **what kind** of frame it is (NID/DUID), **who** it is for (NAC) — and survive bit errors doing so.
3. Split the payload into **voice frames, control words and signalling**, applying the right **FEC**.
4. Hand clean messages up to a **state machine** (call control) and out to **interfaces** (ISSI/CSSI/DFSI).

P25 Phase 1 is the "reference" LMR digital standard in North America and a large installed base worldwide.
If you can explain *why* each field of a P25 frame exists, you can learn DMR/TETRA fast (packs 03–04).

**What an interviewer is really testing:** can you derive a design choice instead of reciting it?
("Why 48 bits of sync?" "Why is the NID protected by a stronger code than the voice?")

## 2. Prerequisites

Nothing is assumed beyond:

- Binary/hex, bit shifts, masks in C++ (`>>`, `<<`, `&`, `^`).
- C++17: `std::array`, `std::vector`, `std::optional`, `constexpr`, GoogleTest `EXPECT_EQ`.
- A Linux/macOS/WSL shell with `cmake`, `g++` or `clang++` (AddressSanitizer needs this; MinGW has no ASan).

Everything else (symbols, FEC, polynomial arithmetic over GF(2)) is built in §3. Build commands:

```bash
cd code
cmake -S . -B build              # tests built against STUBS  -> they FAIL (that's the point)
cmake --build build -j && ctest --test-dir build --output-on-failure
cmake -S . -B build_sol -DLMR_USE_SOLUTIONS=ON   # reference solutions; everything green
```

Flags used: `-Wall -Wextra -fsanitize=address,undefined`.

## 3. Concept build-up

### 3.1 From a radio channel to symbols (concrete first)

A P25 Phase 1 channel is **12.5 kHz wide, one conversation at a time** (FDMA — Frequency Division Multiple
Access: you separate users by *frequency*, not by time) [TIA-102.BAAA].

The transmitter sends **4800 symbols per second** (*baud*). Each symbol is one of **four frequency
deviations** from the carrier. Four levels carry log2(4) = **2 bits per symbol**:

| dibit | symbol | deviation (VERIFY) |
|-------|--------|--------------------|
| `01`  | +3     | +1.8 kHz           |
| `00`  | +1     | +0.6 kHz           |
| `10`  | −1     | −0.6 kHz           |
| `11`  | −3     | −1.8 kHz           |

**4800 sym/s × 2 bit/sym = 9600 bit/s** on the air. This modulation is **C4FM** (Continuous 4-level FM).

*Derive the design choices:*

- **Why four levels, not two?** Bandwidth is the scarce resource (a licensed 12.5 kHz slot). Two levels
  at 4800 baud give 4800 bit/s; to get 9600 bit/s with two levels you would need 9600 baud, which needs
  roughly twice the spectrum. Packing 2 bits per symbol buys speed *without* more bandwidth.
- **Why this bit mapping?** Look at the table top to bottom: `01 → 00 → 10 → 11`. Neighbouring levels
  differ in exactly **one** bit (a *Gray* code). The most likely noise error is "slid to the adjacent level",
  and with Gray mapping that costs **1 bit error, not 2**.
- **Why FM-based (constant envelope)?** FM has constant amplitude, so the transmitter's power amplifier can
  run in an efficient *non-linear* class (C/E). Handheld battery life matters in public safety. (The
  trade-off: C4FM can't squeeze quite as much spectral efficiency as linear schemes; Phase 2 later changes
  this — pack 02.) There is also a linear-compatible receiver form, CQPSK, used in simulcast — see §3.10.

> **Misconception box — "9600 bit/s of voice".** 9600 is the *gross* air rate. It includes sync, NID,
> status symbols, FEC and signalling. Speech itself is **4400 bit/s** (§3.6). Never say "P25 gives you
> 9.6 k of voice".

> **Misconception box — "baud = bit/s".** Baud counts symbols. 4800 baud × 2 bits = 9600 bit/s. Mixing
> them up halves every timing calculation you do.

### 3.2 Frame sync: finding the start of a frame

A receiver sees a never-ending stream of dibits. It needs a **marker** it can search for. The marker is a
fixed **48-bit frame sync** = 24 symbols: `0x5575F5FF77FF` (**VERIFY** [TIA-102.BAAA]).

Look at it in dibits: `0x55 = 01 01 01 01`, `0x75 = 01 11 01 01`, `0xF5 = 11 11 01 01`, `0xFF = 11 11 11 11`,
`0x77 = 01 11 01 11`, `0xFF = 11 11 11 11`. Only `01` (+3) and `11` (−3) appear: the sync uses **only the
extreme deviations**. That makes it easy for a slicer to detect (maximum signal swing, maximum contrast
from random data) and gives it a sharp auto-correlation peak.

**How long should a sync be? Derive it.** Suppose the receiver slides a 48-bit window one symbol at a time
over *noise* (random bits). It declares sync if the window is within *m* bit errors of the pattern (it
*must* tolerate some errors, or one noisy bit would make it miss the whole frame).

Number of 48-bit words within distance *m* of the pattern = Σₖ₌₀..ₘ C(48,k).

| m | words within m | false-sync probability per window |
|---|----------------|-----------------------------------|
| 0 | 1              | 1 / 2⁴⁸ = 3.55e-15 |
| 1 | 49             | 1.74e-13 |
| 4 | 1+48+1128+17296+194580 = 213 053 | **7.57e-10** |

At 4800 windows/s, *m* = 4: 7.57e-10 × 4800 = 3.63e-6 false syncs/s → one every ≈ **275 000 s ≈ 76 h**.
(You will compute this in Q4.) A *false sync* is harmless if the next stage (NID with BCH) rejects it —
the NID is a second, independent check. A 24-bit sync at *m* = 4 would be false-triggering every few
seconds. Length is a **probability budget**.

> **Misconception box — "sync = 48 bits so it needs all 48 right."** Real receivers allow several bit
> errors; that is why you must quantify the false-alarm rate.

### 3.3 The Network ID (NID): who and what

Immediately after sync comes a **64-bit NID**. Its job: say **what kind of frame follows** (DUID) and **which
system/ network area it belongs to** (NAC).

- **NAC — Network Access Code, 12 bits** (0x000–0xFFF). Think of it as the digital equivalent of CTCSS/CDCSS
  "squelch tones" on analogue radios: a receiver configured for NAC 0x293 ignores frames from another
  system sharing the channel (co-channel interference). **It is not encryption and gives no security.**
  Special values: `0xF7E` = "receive any NAC" and `0xF7F` = "any NAC / repeater-ing" (**VERIFY** [TIA-102.BAAC]).
- **DUID — Data Unit ID, 4 bits** (VERIFY values [TIA-102.BAAA]):

| DUID | Frame |
|------|-------|
| 0x0 | HDU — Header Data Unit (start of voice call) |
| 0x3 | TDU — simple Terminator (end of call) |
| 0x5 | LDU1 — Logical Link Data Unit 1 (voice + **Link Control**) |
| 0x7 | TSBK — Trunking Signalling Block (control channel) |
| 0xA | LDU2 — Logical Link Data Unit 2 (voice + **Encryption Sync**) |
| 0xC | PDU — Packet Data Unit |
| 0xF | TDULC — Terminator with Link Control |

12 + 4 = **16 information bits**. They are protected into 64 bits: 16 info + 47 BCH parity = 63, + 1 overall
parity bit = **64**. The code is **BCH(63,16)**.

*Why such a heavy code (47 parity bits for 16 data bits — a rate of 0.25)?* Because **one corrupted DUID
destroys an entire frame** (1728 bits = 180 ms of voice, or a trunking message). The receiver would parse
LDU2 bits as if they were LDU1 and produce garbage audio/keys. There are only 16 bits to protect, so
protecting them massively is cheap in absolute terms (it costs 64 − 16 = 48 bits per *frame*).

**What is a BCH code? Build it.** A *cyclic* code of length 63 treats a 63-bit word as a polynomial over
GF(2) (coefficients 0/1, addition = XOR). Pick a **generator polynomial g(x)**; valid codewords are exactly the
multiples of g(x). Encode *systematically*: put the data in the high bits, divide by g, append the remainder
(the parity). Receiver check: remainder (syndrome) of the received word divided by g is 0 ⇒ looks valid.

BCH design rule: if g(x) has as roots *consecutive* powers α¹…α²ᵗ of a primitive element α in GF(64),
the code corrects **t** errors (min distance ≥ 2t+1). Here t = 11 ⇒ roots α¹…α²². Each root drags in its
*conjugates* (αʳ, α²ʳ, α⁴ʳ… mod 63 — the "cyclotomic coset"), giving cosets of sizes
6+6+6+6+3+6+6+6+2 = **47** ⇒ deg g = 47 ⇒ k = 63 − 47 = **16**. The numbers fall out exactly — that is
*why* it is called BCH(63,16) with min distance 23 and t = 11. `support/p25p1/gf64.hpp` computes g(x) this
way. (**VERIFY**: that the standard picks the same field polynomial x⁶+x+1 and hence the identical g(x).)

*Why add one more parity bit?* With d = 23 you correct 11 errors but a 12-error pattern could be
*mis-corrected* silently. One extra overall-parity bit makes the minimum distance 24 (all weights even),
which lets the decoder correct 11 **and detect 12**. (**VERIFY** the parity convention [TIA-102.BAAA].)

> **Misconception box — "NAC is a password."** NAC just filters co-channel systems. Anyone can transmit
> any NAC. Security comes from encryption (§3.7) and authentication in the infrastructure.

> **Misconception box — "BCH(63,16) holds 16 *bytes*."** Sixteen **bits**: 12 NAC + 4 DUID.

### 3.4 Status symbols: the interleaved 2-bit side channel

Every **35 data symbols** the transmitter inserts one extra **status symbol** (2 bits). So the symbol at
index k (0-based, counted from the start of the frame sync) is a status symbol iff **(k+1) mod 36 = 0**
(**VERIFY** the exact counting origin). On a conventional channel these carry busy/idle information so
subscriber radios can tell whether an inbound slot is free; on a trunked inbound they help access control.
Consequence for parsers: **status symbols interrupt other fields**. The NID's 32 symbols straddle the first
one (§4).

> **Misconception box — "status symbols are part of the NID/FEC".** They are interleaved *after* FEC
> and must be stripped *before* FEC decoding.

### 3.5 The frame types and a call's timeline

A voice call on a conventional channel looks like:

```
HDU | LDU1 LDU2 | LDU1 LDU2 | LDU1 LDU2 | ... | TDU (or TDULC)
```

- **HDU** carries a *header word*: message indicator (MI), manufacturer ID, algorithm ID (ALGID), key ID
  (KID), talkgroup ID (TGID) — enough for a receiver to prepare decryption **before** speech starts.
  Total on air ≈ 792 bits ≈ 82.5 ms (**VERIFY** the exact accounting of trailing bits).
- **LDU1 + LDU2 = one 360 ms superframe**: 2 × 9 IMBE frames = 18 × 20 ms = 360 ms of speech.
- **TDU/TDULC** ends the call.
- **TSBK** frames appear on a *trunking control channel*, up to 3 per frame outbound (§3.8).

*Why alternate LDU1/LDU2?* Each LDU has a limited spare payload (240 coded bits after the 9 voice frames).
Link control (who/which group/emergency) and encryption sync (MI/ALGID/KID) both need to be *repeated*
for late entry (§3.7) but both don't fit in one LDU, so each LDU carries one of them and you get **both
every 360 ms**.

### 3.6 Anatomy of an LDU — build the bit budget

One LDU on air is **1728 bits = 864 symbols = 180 ms** (864 / 4800 = 0.18 s). Derive the contents:

| Field | Bits | How we know |
|-------|------|-------------|
| Frame sync | 48 | §3.2 |
| NID | 64 | §3.3 |
| 9 × IMBE voice frame | 9 × 144 = 1296 | 20 ms per frame × 9 = 180 ms ✔ |
| LDU1 link control **or** LDU2 encryption sync (after FEC) | 240 | below |
| Low-speed data (LSD) | 32 | 2 × (8 info → 16 coded) |
| Status symbols | 24 × 2 = 48 | 864 / 36 = 24 |
| **Total** | **1728** ✔ | |

**IMBE voice frame = 144 bits.** IMBE is the Phase 1 vocoder (Improved Multi-Band Excitation) [TIA-102.BABA].
Raw speech parameters: **88 bits per 20 ms = 4400 bit/s**. FEC: the frame is split into 7 vectors
û₀…û₆ and 7 raw bits; the more perceptually critical vectors get stronger codes:
4 × Golay(23,12) = 92 bits (carries 48 info), 3 × Hamming(15,11) = 45 bits (33 info), 7 unprotected =
7 → 92 + 45 + 7 = **144**; info 48 + 33 + 7 = **88** ✔. 144 × 50 frames/s = **7200 bit/s** of "voice" on air
(7200 of 9600 — the rest is overhead).

*Why FEC the voice at all, and why unequally?* Errors in a voice frame are *local* (one 20 ms glitch,
masked by the listener), and retransmission is impossible in a broadcast radio stream. So the codec is
designed around the **ear**: protect only the bits whose corruption sounds worst (pitch, gain, low-order
spectral amplitudes), leave the 7 least-significant bits bare. **Contrast** with the NID: an error there
isn't a 20 ms glitch, it's loss of the whole frame. That is **why headers get stronger FEC than voice**
(this is the §5 Q2 interview answer).

**Link control and encryption sync FEC chain:** LC 72 bits → Reed-Solomon(24,12,13) over 6-bit symbols
→ 144 bits → Hamming(10,6,3) per hex-word → **240 bits**. ES 96 bits → RS(24,16,9) → 144 → Hamming(10,6) →
240. (**VERIFY** codes [TIA-102.BAAA].) Reed-Solomon works on *symbols* (here 6-bit), so a burst that wipes out a
few adjacent bits corrupts only 1–2 symbols — Reed-Solomon is the classic "burst eraser". The inner Hamming
code cleans single-bit errors cheaply before RS sees them.

### 3.7 What LDU1 and LDU2 carry — and why they repeat

**LDU1 — Link Control (LC), 72 bits.** Fields (group voice channel user format — **VERIFY**
[TIA-102.BAAA/AABF]): LCF/LCO (what kind of LC), MFID (manufacturer, 0x00 = standard), service options
(emergency, encrypted, priority), TGID (16-bit talkgroup), source unit ID (24-bit).

**LDU2 — Encryption Sync (ES), 96 bits:** **MI** (72-bit Message Indicator, an IV that changes during the call),
**ALGID** (8 bits: `0x80` = unencrypted; `0x81` DES-OFB, `0x84` AES-256, `0xAA` ADP/RC4 — **VERIFY**), **KID**
(16-bit Key ID: which key from the radio's keyset).

> **Why repeat LC and ES in every superframe? — LATE ENTRY.** A radio that is switched on, or moves into
> coverage, or loses sync mid-call, never saw the HDU. If who-is-talking and which-key-to-use were sent only
> once, that radio could *never* join (clear call: it wouldn't know the talkgroup; encrypted call: it would
> not know ALGID/KID/MI and could not decrypt). Repeating them costs bandwidth but gives
> **worst-case join time ≈ 540 ms** (Q7). MI also *advances* during the call (an LFSR step per LDU2 — **VERIFY**),
> so a late-entrant gets the *current* IV, not the starting one — a cipher in OFB mode needs the right IV or
> output is noise.

> **Misconception box — "ALGID is the key."** ALGID names the *algorithm*, KID names *which key*. The key
> itself never goes over the air in the clear; OTAR (over-the-air rekeying, pack 05) delivers keys encrypted
> under a key-encryption key.

### 3.8 Trunking signalling: TSBK in one paragraph

On a trunked system a **control channel** broadcasts short messages. A **TSBK** is a 96-bit block: last-block
flag (1), protect flag (1), opcode (6), MFID (8), arguments (64), **CRC-16** (16) = 96. It is coded with a
**rate-½ trellis code** → 196 bits (98 dibits; 96 bits + flush → 49 dibits in, 98 out). Example messages
(**VERIFY** names/opcodes [TIA-102.AABC]): unit registration, group affiliation, group voice channel grant,
queued/deny responses, identifier updates (frequency plan), RFSS/network status broadcasts.
*Why trellis + CRC?* The trellis (a convolutional-style code decoded with Viterbi) corrects scattered errors;
the CRC then tells you whether the decode succeeded, because a trellis decoder always emits *something*.
**FEC corrects; CRC *checks*.** You need both.

### 3.9 Interfaces: CAI, ISSI, CSSI (and DFSI)

| Interface | Connects | Transport | Standard |
|-----------|----------|-----------|----------|
| **CAI** — Common Air Interface | radio ↔ infrastructure over RF | the frames above | TIA-102.BAAA |
| **ISSI** — Inter-RF-Subsystem Interface | RF subsystem ↔ RF subsystem (system-to-system roaming, group calls across sites/systems) | **SIP** (call control, with SDP) + **RTP** (voice) over IP | TIA-102.BACA (VERIFY) |
| **CSSI** — Console Subsystem Interface | dispatch console ↔ RF subsystem | SIP + RTP (ISSI-derived) | TIA-102.BACE? **VERIFY** |
| **DFSI** — Digital Fixed Station Interface | fixed station (repeater) ↔ controller | serial / IP, carries voice and control (VERIFY) | TIA-102.BAHA |

*Why SIP/RTP?* Because by the time the ISSI was designed, IP networks and the SIP/RTP pair were mature,
standardised, multi-vendor, and routable. Using them lets a vendor reuse off-the-shelf IP/VoIP tooling.
The design splits *signalling* (SIP: who calls whom, which group) from *media* (RTP: timestamps and sequence
numbers, which also solve jitter). Note the important consequence: **the air interface and the network
interface are different protocols**; the stack *translates* (e.g., unwraps IMBE frames from an LDU, re-wraps
them in RTP).

> **Misconception box — "ISSI is the radio protocol."** ISSI is **wire-line IP** between infrastructure
> pieces. Radios never speak ISSI.

### 3.10 A stack's view (how pieces map to code)

```
bits ─▶ [sync search] ─▶ [NID: BCH decode] ─▶ [DUID dispatch]
                                    │
        HDU / LDU1 / LDU2 / TSBK ◀──┘ ─▶ [strip status] ─▶ [FEC: Golay/Hamming/RS/trellis]
                                    ─▶ [message structs: LC, ES, TSBK] ─▶ [call state machine]
                                    ─▶ [IMBE frames → vocoder or → RTP (ISSI/CSSI)]
```

Simulcast systems send the same signal from several towers on one frequency; the linear **CQPSK/LSM**
modulation is used there because C4FM's non-linear detection suffers when delayed copies overlap (pack 05).

## 4. Fully worked example — building one synthetic LDU1 skeleton

**Step 1 – NID information bits.** NAC = `0x293`, DUID = LDU1 = `0x5`.
`data16 = (0x293 << 4) | 0x5 = 0x2930 | 0x5 = 0x2935 = 0010 1001 0011 0101`.

**Step 2 – the parity idea, on a toy code you can do by hand.** Hamming(7,4), g(x) = x³ + x + 1 (`1011`).
Data `1101` = x³+x²+1. Shift by 3: x⁶+x⁵+x³. Reduce with x³ ≡ x+1:
x³ = x+1; x⁵ = x²·x³ = x²(x+1) = x³+x² = x²+x+1; x⁶ = (x³)² = x²+1.
Sum: (x²+1)+(x²+x+1)+(x+1) = 1 ⇒ remainder `001`. Codeword = `1101 001`.
Flip bit 4 (x⁴): received `1111001`. Its remainder = x⁴ mod g = x⁴ = x·x³ = x²+x = `110`. Look up: the
syndrome of x⁴ is `110` ⇒ the error is at position 4 ⇒ flip it back. **BCH(63,16) is the same machine** with
deg g = 47: encode = shift data by 47 and take the remainder (your `bch63_16_encode`), and a *valid*
received word has remainder 0.

**Step 3 – where does everything land in the symbol stream?** Count symbols from 0:

| Symbol index | Content |
|--------------|---------|
| 0 – 23 | frame sync (24 symbols = 48 bits) |
| 24 – 34 | first 11 NID symbols (22 bits) |
| **35** | **status symbol** ((35+1) mod 36 = 0) |
| 36 – 56 | remaining 21 NID symbols (42 bits) → 22 + 42 = 64 ✔ |
| 57 – … | IMBE voice frame 1 (72 symbols) … |

So the NID ends at symbol **56**, and the first IMBE frame starts at 57. A parser that forgets the
status symbol mis-reads the NID by 2 bits and the BCH syndrome is garbage.

**Step 4 – check the budget:** 48 + 64 + 1296 + 240 + 32 + 48 = 1728 bits = 864 symbols = 180 ms ✔.

**Step 5 – a 3-second group call:** HDU (≈ 82.5 ms) + 16 LDUs (8 superframes = 2880 ms) + TDU ≈ 2.97 s +
terminator. Your IMBE frame count = 16 × 9 = 144 frames = 144 × 20 ms = 2.88 s ✔.

## 5. Questions

Every question has an **Etherstack context** line. Answers (full reasoning, wrong answers, why tempting,
why wrong) are in `answers.md`. Code questions map to `code/` — implement the stubs in `code/include/p25p1/`
and run `ctest`.

---

### Q1 — Explain (modulation)
Why does P25 Phase 1 use four deviation levels with a Gray-like mapping, and what does the constant envelope
buy a handheld radio?
*Etherstack context:* a stack's PHY-facing API exchanges **dibits**, not bits; the dibit→symbol mapping is
where bit order bugs start.

### Q2 — Explain (FEC strength)
Why is the 16-bit NID protected by 47 parity bits while a 20 ms voice frame gets only 56 FEC bits for 88
speech bits — and, within the voice frame, why are some bits left unprotected?
*Etherstack context:* conformance testing of a stack includes sending corrupted NIDs and voice; you must
know which corruption the stack *must* survive.

### Q3 — Calculate (bit budget) — **code: `budget.hpp`**
(a) Derive the 1728-bit LDU budget in the table of §3.6. (b) How many IMBE frames/s? (c) What fraction of the
9600 bit/s channel is speech payload (88-bit information) and what fraction is "voice on air" (144-bit)?
*Etherstack context:* throughput and latency budgets for RTP packetisation at the ISSI/CSSI edge come from
exactly these numbers.

### Q4 — Calculate (false-sync) — **code: `sync.hpp`**
With a 48-bit sync, tolerating *m* = 4 bit errors, testing one window per symbol (4800/s): (a) false-sync
probability per window, (b) mean time between false syncs, (c) the time if the sync were 32 bits.
*Etherstack context:* the stack's sync tolerance is a tunable; it trades missed frames against CPU burned in
false NID decodes.

### Q5 — Predict (error behaviour)
A receiver gets a valid NID with (a) 3, (b) 11, (c) 12 bit errors in the 63-bit BCH word (and the 64th
parity bit is correct). What does a nearest-codeword decoder return in each case? What changes if the
decoder also checks the overall parity bit?
*Etherstack context:* "wrong-NID" vs "dropped-NID" is a product decision (false frames vs missed frames).

### Q6 — Trace (NID packing) — **code: `nid.hpp`**
Trace by hand: NAC `0x293`, DUID LDU2. (a) data16? (b) which NID byte does a receiver extract if the first 12
bits are `0xF7E` and the DUID bits are `0x7`? (c) Which DUIDs are *not* assigned and what should a parser do
when it sees one after a successful BCH decode?
*Etherstack context:* clean handling of unknown code points is how a stack stays forward-compatible.

### Q7 — Trace (late entry timeline)
A radio powers up at a random instant during a long *encrypted* group call. Frames are LDU1, LDU2
alternating, 180 ms each. (a) Worst-case time before it has the ES (MI/ALGID/KID)? (b) Worst-case time before
it has the LC (TGID/source)? (c) Expected times if joining is uniform in time.
*Etherstack context:* "time to audio" for late-entry handsets is a classic acceptance-test number.

### Q8 — Find the bug — **code: `sync.hpp`**
```cpp
// Intended: true if the last 48 received bits are within 'm' bit errors of the sync word.
bool sync_matches(uint64_t window, int m) {
  int errs = __builtin_popcountll(window ^ kFrameSync48);   // (1)
  return errs < m;                                          // (2)
}
// Intended: bits per symbol
constexpr int kBitRate = 4800;                              // (3)
// Intended: status symbol positions inside a frame (0-based symbol index k)
bool is_status(int k) { return k % 36 == 0; }               // (4)
```
List every bug, say what symptom each produces, and fix.
*Etherstack context:* off-by-one and unmasked-window bugs are the bread and butter of protocol-stack
code review.

### Q9 — Find the bug (parsing)
```cpp
uint16_t pack_nid(uint16_t nac, Duid d) { return (nac << 4) & static_cast<uint8_t>(d); }
GroupVoiceLc parse(const uint8_t lc[9]) {
  GroupVoiceLc g;  g.tgid = lc[4] | lc[5];  g.source = (lc[6] << 16) | (lc[7] << 8) | lc[8] << 24;
  return g;
}
```
What does each line do for `nac=0x293, d=LDU1` and `lc[4..8] = 12 34 00 AB CD`? Fix.
*Etherstack context:* ID truncation (a 24-bit unit ID in a 32-bit field) causes mis-routed calls in the field.

### Q10 — Implement (BCH encode/decode) — **code: `bch.hpp`**
Implement `poly_mod`, `bch63_16_encode`, `bch63_16_decode` (brute force) and `nid_encode64`. Then explain why
the brute-force decoder is acceptable for an exercise but not for a base-station that must decode thousands
of NIDs per second, and name two production alternatives.
*Etherstack context:* a real stack uses syndrome decoding / lookup tables; knowing the reference model lets you
test the optimised one against it ("golden model" testing).

### Q11 — Implement (LC / ES parsing) — **code: `ldu.hpp`**
Implement `parse_group_voice_lc`, `parse_encryption_sync`, `is_clear`, `service_options_emergency`.
*Etherstack context:* these structs are what the call-control layer and the ISSI gateway consume.

### Q12 — Connect (CAI → ISSI)
A group call is heard on the air as HDU, LDU1, LDU2…. Sketch how a gateway would turn it into SIP + RTP for
the ISSI: what goes in SIP, what goes in RTP, which air-interface fields become which network fields, and
what is lost or must be regenerated (hint: status symbols, NAC, FEC).
*Etherstack context:* this is exactly the boundary Etherstack-style stacks sell: air-interface side ↔
network-interface side.

### Q13 — Interview
"Walk me through everything that has to happen between a raw bit stream and the first decoded voice frame of a
P25 Phase 1 call. Where can it go wrong?"
*Etherstack context:* expect this as a whiteboard question.

### Q14 — Interview
"How would you test a P25 stack end-to-end without any radio — and why is it important that you can't
accidentally test it on real traffic?"
*Etherstack context:* a vendor must prove conformance with synthetic vectors; legal/compliance staff will
ask this too.

## 6. Self-assessment

Tick honestly; 80% ticks ⇒ you are ready for the interview on this pack.

- [ ] I can compute 4800 baud → 9600 bit/s → 1728-bit LDU → 180 ms without a calculator.
- [ ] I can explain why sync has 48 bits using a probability argument.
- [ ] I can say what NAC and DUID are, and that NAC is *not* security.
- [ ] I can explain BCH encoding as "shift, divide, append remainder", and the decode idea as "syndrome".
- [ ] I can say why headers are stronger-FEC than voice, in one sentence.
- [ ] I can say why LC and ES repeat (late entry) and compute the worst-case join time.
- [ ] I can list CAI / ISSI / CSSI and state which uses SIP/RTP.
- [ ] All tests are green with `-DLMR_USE_SOLUTIONS=OFF` **after my implementations**.
- [ ] I can state three things in this pack I would VERIFY before quoting.

| Score | Meaning |
|-------|---------|
| 0–4 ticks | Re-read §3, redo §4 by hand |
| 5–7 | Do Q2, Q5, Q7 again without notes |
| 8–9 | Move to pack 02 |

### VERIFY list for this pack
1. Frame sync value `0x5575F5FF77FF` and dibit→deviation mapping (TIA-102.BAAA).
2. NID parity-bit convention; that the generator equals the one derived from x⁶+x+1.
3. NAC special values `0xF7E`/`0xF7F`.
4. DUID code points; ALGID code points (`0x81`, `0x84`, `0xAA`).
5. HDU total bits (792?) and trailing-bit accounting; TDU/TDULC lengths.
6. LC group-voice layout, service-options bit positions, LCF values.
7. Which RS/Hamming/Golay codes protect LC / ES / HDU (RS(24,12,13), RS(24,16,9), RS(36,20,17), Hamming(10,6,3), Golay(18,6,8)).
8. Status-symbol counting origin; MI update rule per LDU2.
9. CSSI/ISSI/DFSI document numbers.
