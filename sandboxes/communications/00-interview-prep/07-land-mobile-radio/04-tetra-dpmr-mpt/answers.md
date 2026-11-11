# Pack 04 — Answers (TETRA, dPMR, MPT 1327)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → angle.**
Synthetic data only. **VERIFY** = check ETSI EN 300 392 / TS 102 490 / TS 102 658 / MPT 1327 before quoting.

---

## Q1 — Explain: 6.25 kHz per call

**Answer.** Regulators pushed LMR from 25 → 12.5 → 6.25 kHz-equivalent per call. Two ways: **slice time** (TDMA:
DMR 12.5 kHz/2, P25 Phase 2 12.5 kHz/2, TETRA 25 kHz/4 — all = 6.25 kHz per call) or **slice frequency** (FDMA: dPMR uses a
6.25 kHz channel for one call). All need a vocoder of ≈ 3.6 kbit/s on air so that a call fits the share of the pipe.

**Reasoning.** Hz per call = channel ÷ calls: 12.5/2, 12.5/2, 25/4, 6.25/1 = 6.25 kHz. Bits per call: DMR 9600/2 = 4800;
dPMR 4800; TETRA net 7200/slot; P25p2 (12 000 − overhead)/2.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "TDMA creates more spectrum." | Capacity grows. | Same spectrum, shared in time; capacity grows because the per-call rate shrank. |
| "dPMR must be TDMA to get 6.25 kHz." | Others are. | FDMA with a narrower channel works; that is the point. |
| "TETRA is 25 kHz per call." | Carrier width. | 4 slots ⇒ 6.25 kHz per call. |

---

## Q2 — Explain: 30 ms codec, control frame

**Answer.** A multiframe is 1.02 s and has **17** traffic frames. If each traffic slot carries **two** codec frames, the
speech per slot must be 1020 ms ÷ 17 = **60 ms**, so the codec frame is 60 ÷ 2 = **30 ms**. That is why ACELP frames are 30 ms: the
codec framing is *derived from the slot schedule*. The 18th (control) frame costs 1/18 = 5.6 % of capacity but gives a guaranteed
signalling opportunity and monitoring window without stealing speech; the speech schedule is simply built around it
(17 × 60 ms = 1020 ms ✔).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "30 ms because humans speak in 30 ms units." | Speech-processing folklore. | The frame size is the result of the slot/multiframe arithmetic (and codec design). |
| "Frame 18 is wasted." | It carries no traffic. | It guarantees signalling and monitoring; otherwise you'd steal speech bursts. |
| "Slots recur every 60 ms." | 60 ms of speech per slot. | They recur every 56.667 ms, except across frame 18; the *average* spacing is 60 ms. |

---

## Q3 — Calculate

| Quantity | Derivation | Value |
|---|---|---|
| gross | 18 000 × 2 | **36 000 bit/s** |
| slot | 510 / 36 000 | **14.167 ms** |
| frame | 4 × 14.167 | **56.667 ms** |
| multiframe | 18 × 56.667 | **1020 ms** |
| hyperframe | 60 × 1.02 | **61.2 s** |
| normal-burst overhead | 510 − 432 | **78 bits** (VERIFY breakdown) |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "slot = 510/18 000 = 28.3 ms." | 18 000 is in the spec. | That's the symbol rate; slots are measured in bits at the bit rate (or 255 symbols at 18 ksym/s = 14.17 ms). |
| "Multiframe = 17 frames." | 17 traffic frames. | 18 frames including control; 1.02 s. |
| "Hyperframe in ms." | Units drift. | 61.2 **seconds**. |

---

## Q4 — Calculate

- TCH/7.2: 432 × 17 / 1.02 = 7344 / 1.02 = **7200 bit/s** per timeslot (net, unprotected).
- Per carrier ×4 = **28 800 bit/s**; net/gross = 28 800 / 36 000 = **0.8** (the remaining 20 % = burst overhead 78/510 = 15.3 % + the control frame 1/18 = 5.6 %: 1 − (432/510)(17/18) = 1 − 0.847 × 0.944 = 0.80 ✔).
- ACELP: 137 / 0.030 = **4566.7 bit/s**; channel coding: 274 bits → 432 per slot ⇒ code rate 0.634.
- Gross spectral efficiency: 36 000 / 25 000 = **1.44 bit/s/Hz** (P25 Phase 2: 0.96; DMR/P25p1: 0.768). Net: 28 800/25 000 = 1.15.
- Why is TETRA's gross efficiency higher? 18 ksym/s with roll-off 0.35 occupies ≈ 24.3 kHz (VERIFY), nearly filling 25 kHz; P25/DMR use 4800–6000 sym/s within a 12.5 kHz mask and need adjacent-channel margins.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Net = 36 000 × 4/4." | Four slots share it. | Net is 80 % of gross after burst overhead and the control frame. |
| "TETRA is 7.2 kbit/s total." | Quoted figure. | 7.2 per timeslot; 28.8 per carrier. |
| "17 vs 18 doesn't matter." | 5 % difference. | It is exactly the figure that makes 7200. |

---

## Q5 — Calculate

(a) dPMR: 2400 × 2 = **4800 bit/s**; 4800 / 6250 = **0.768 bit/s/Hz** (equal to P25 Phase 1 and DMR gross).
(b) ±1 ppm at 450 MHz = ±450 Hz. Channel width 6.25 kHz ⇒ 450/6250 = **7.2 %** of the channel (at 12.5 kHz 3.6 %, at 25 kHz 1.8 %).
Narrower channels demand proportionally better oscillators (and tighter filters) — a real cost of FDMA at 6.25 kHz; also why
dPMR radios ship with ≤ 1 ppm TCXOs (VERIFY spec limits).
(c) 64 / 1200 = **53.33 ms**; information 48 bits per 53.33 ms = **900 bit/s** (75 % of 1200).
(d) 1200 / 2 = **600 Hz** (tones 1200 and 1800).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "dPMR is slower, 2400 bit/s." | 2400 symbols/s. | 2 bits/symbol. |
| "ppm is a fixed Hz amount." | A tolerance sounds constant. | Hz = ppm × carrier; the *relative* damage depends on channel width. |
| "MPT info rate = 1200 bit/s." | Bit rate. | 16 of every 64 bits are check/parity. |

---

## Q6 — Predict: wrong colour code

(a) **TETRA:** colour code isn't a field; it initialises the scrambler. With the wrong code the receiver descrambles to **pseudo-random bits**:
all CRCs fail, nothing decodes. It looks exactly like heavy noise. (The BSCH synchronisation burst uses a fixed/default scrambling so a
mobile can always read it and *learn* the colour code — VERIFY.)
(b) **DMR:** the colour code is an **explicit 4-bit field** in slot type/EMB. The burst decodes cleanly; the CC field is simply ≠ ours ⇒
the radio drops it (a logged "CC mismatch").
(c) **Debugging:** DMR mismatch is *visible and diagnosable*; TETRA's looks like noise, so you check frequency, timing, then colour code
(MCC/MNC/CC) — a good reason for stacks to log "CRC failures on all bursts" and offer an auto-scan of CC candidates (64 values).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "TETRA has a CC field like DMR." | Same name. | It's a scrambling seed. |
| "Wrong CC = weak signal." | Both give no audio. | Different signatures; check logs. |

---

## Q7 — Trace

n → (timeslot, frame, multiframe):
- 0 → (1, 1, 1)
- 67 → ts = 67 mod 4 = 3 → **4**; frame_abs = 16 → **17**; mf 1 → (4, 17, 1)
- 68 → (1, **18**, 1) — control frame
- 71 → (4, **18**, 1) — control frame
- 72 → (1, 1, **2**)
- 4320 = 72 × 60 → (1, 1, **1**) — hyperframe wrap
Control-frame slots: 68–71 (and every slot in frame 18 of every multiframe).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "67 → timeslot 3." | 67 mod 4 = 3. | Numbering is 1-based: 3 → timeslot 4. |
| "4320 → multiframe 61." | Continue counting. | Multiframe numbers wrap at 60 (hyperframe). |

---

## Q8 — Trace: PTT in a TETRA group call

| Hop | Layer | PDU (VERIFY) | Role |
|---|---|---|---|
| 1 | **CMCE** (call control) | `U-TX DEMAND` | "I want the floor" |
| 2 | **MLE** | (adds PDU discriminator; selects the CMCE entity) | routing within layer 3 |
| 3 | **LLC** | `BL-DATA` / `BL-ADATA` (+ `BL-ACK` back if acknowledged) | link reliability |
| 4 | **MAC** | `MAC-ACCESS` (uplink random access in a slot granted by the AACH) or via stolen traffic bits (STCH) | when/how to send |
| 5 | **PHY** | burst (SCH/HU or STCH) | modulation/coding |
| ← | MAC → LLC → MLE → **CMCE** | `MAC-RESOURCE` carrying `D-TX GRANTED` (to the requester) / `D-TX GRANTED` or `D-TX CEASED` to others | floor decision |
MM is **not** involved (registration and group attach happened earlier).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "PTT is an MM message." | Mobility/identity. | Floor control belongs to call control (CMCE). |
| "MAC sets who talks." | MAC arbitrates slots. | MAC arbitrates *radio access*; the floor is a CMCE concept. |
| "Order: MAC first." | Radio first. | Uplink goes down the stack: CMCE → MLE → LLC → MAC → PHY. |

---

## Q9 — Find the bug

1. `510.0 / 18000 * 1000` = 28.33 ms — used the **symbol** rate with bits. Fix: `510.0 / 36000 * 1000` = 14.167 ms (or 255 symbols / 18 000).
2. `432 * 18 / 1.02` = 7623.5 bit/s — **17** traffic frames, not 18. Fix → 7200.
3. `n % 4` returns 0–3; the standard numbers timeslots **1–4** (VERIFY). Fix: `n % 4 + 1` (and be consistent for frame/multiframe).
4. `frame == 17` — with 1-based frame numbers the control frame is **18**.
5. `return baud;` — tone spacing is **baud/2** (600 Hz for 1200 baud).
Typical consequence of (3)+(4) together: off-by-one in *which* slots are skipped for traffic.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "(3) and (4) are consistent: 0-based both." | They might be. | (3) is 0-based and (4) tests the 1-based control frame; mixed conventions. Pick one and document it. |
| "(1) is right; 18 000 is the clock." | Symbol clock. | Dividing bits by symbol rate gives wrong units. |

---

## Q10 — Implement (MPT codeword); why bursts ≤ 15 are caught

Reference: `solutions/lmr_legacy/legacy.hpp`.
- `crc15`: `v = info48 << 15`; for i = 62 … 15: if bit i set, `v ^= g << (i−15)`; return `v & 0x7FFF`.
- `encode`: `body = (info48 << 15) | crc15`; `parity` = popcount(body) & 1; `cw = (body << 1) | parity`.
- `check`: total weight even **and** `cw >> 1` divisible by g.
**Burst proof.** An error burst of length ≤ 15 is `e(x) = x^k·b(x)` with `b(0) = 1` and `deg b ≤ 14`. The received word is divisible by g iff g
divides e. g has constant term 1 so `gcd(g, x^k) = 1`; thus g | e ⇒ g | b, impossible since `deg b < deg g = 15` and b ≠ 0. So **every**
such burst changes the remainder. (Two scattered flips aren't guaranteed — depends on the order of g — that's why parity and the
protocol's own timeouts exist.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "A CRC detects all errors." | Marketing. | Only certain classes (all bursts ≤ r, odd-weight errors if (x+1) divides g, …). |
| "The parity bit is redundant given a CRC." | CRC is stronger. | It cheaply rules out odd-weight patterns, and brings the total weight parity check. |
| "Shift by 16." | We use 16 trailing bits. | The CRC shift is 15 (the degree); the extra bit is the parity. |

---

## Q11 — Implement

References: `solutions/tetra/*.hpp`.
- `time_of_slot`: `n % 4 + 1`, `(n/4) % 18 + 1`, `(n/72) % 60 + 1`.
- `normalize_pdu_name`: upper-case (cast to `unsigned char` before `std::toupper`: negative `char` is UB), `' '`/`'_'` → `'-'`.
- `layer_of_pdu`: ordered prefix tests; unknown ⇒ `Unknown` (never guess).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `std::toupper(c)` with a `char`. | Compiles. | Undefined for negative values; UBSan may flag it. |
| Substring `find` instead of prefix. | More lenient. | "U-SETUP" would match "D-SETUP-…" in other contexts; prefix is precise. |

---

## Q12 — Connect: TETRA ↔ P25 Phase 1 gateway

| Item | Mapping |
|---|---|
| Identities | TETRA SSI 24-bit (within a network) + MCC 10 + MNC 14 ⇒ ITSI 48 bits; P25 unit ID 24-bit (+WACN 20 + SysID 12) and **TGID 16-bit**. Use mapping tables, not casts. |
| Floor | `U-TX DEMAND` / `D-TX GRANTED` / `D-TX CEASED` ↔ P25 PTT, HDU start, TDU/TDULC end. Gateway tracks the floor state (pack 06). |
| Speech | ACELP 137 bits/30 ms (240 samples at 8 kHz) ↔ IMBE 144 bits/20 ms (160 samples): lcm(30, 20) = **60 ms** (480 samples) ⇒ the gateway needs a PCM re-framing buffer; added delay ≈ a frame or two plus codec look-ahead (VERIFY numbers). Tandem coding degrades quality. |
| Emergency | TETRA call priority (15 = emergency, pre-emptive — VERIFY) ↔ P25 emergency flag in service options. |
| Encryption | TEA1–4 vs P25 DES/AES: no mapping; encrypted calls don't cross without keys at the gateway. |
| SDS | TETRA SDS (types 1–4, up to ~2047 bits — VERIFY) ↔ P25 short data/text — truncation/format differences. |
| Registration | MM location update + group attach ↔ P25 registration + group affiliation. |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Frames are 20 ms on both sides." | Voice = 20 ms. | TETRA is 30 ms. |
| "Just forward the speech bits." | Both are voice codecs. | Different codecs. |

---

## Q13 — Interview: the TETRA stack

**Model answer.** "PHY: π/4-DQPSK bursts, 4 slots, 36 kbit/s gross. MAC: slot access, fragmentation, logical channels (`MAC-RESOURCE`) — hides
the TDMA. LLC: acknowledged/unacknowledged links (`BL-ACK`) — hides radio unreliability. MLE: which cell/network, roaming, attach (`D-NWRK-BROADCAST`)
— hides mobility from the upper layers. Above that CMCE (calls, floor control `U-TX DEMAND`/`D-TX GRANTED`, SDS) and MM (registration,
authentication, group attach `U-LOCATION UPDATE DEMAND`). Plus SNDCP for IP packet data. Each layer hides one kind of change."

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Layers are just a historical OSI copy." | They look like OSI. | Each hides a distinct volatility; that's the design value. |

---

## Q14 — Interview: dPMR (FDMA 6.25 kHz) vs DMR (TDMA 12.5 kHz)

**Model answer.** Same gross spectral efficiency (0.768), same bits and Hz per call. **dPMR:** simpler radios (no slot sync, guard or ramp), continuous
transmission, low latency, easy direct mode; but one repeater/transmitter per call, tighter oscillator and filter requirements (7.2 % of channel for ±1 ppm
at 450 MHz), and a smaller ecosystem. **DMR:** one repeater serves two calls (cheaper infrastructure), 50 % TX duty (battery), big ecosystem, Tier III trunking; but
needs slot timing, guard/ramp, more complex RF/DSP. *Choice:* greenfield single-site, licence-exempt or small fleets → dPMR; networks needing repeaters/trunking and
vendor choice → DMR. (Always say "it depends on spectrum rules, ecosystem and infrastructure cost" and show the arithmetic.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "TDMA is always better." | Newer. | It buys repeater economics at the cost of timing complexity. |
| "dPMR has more capacity." | 6.25 kHz sounds narrow. | Same Hz per call. |

---

## Q15 — Interview: MPT 1327 today

**Model answer.** Large installed analogue trunked fleets (utilities, transport, industrial, some public safety) with long equipment lifetimes and no budget/
spectrum for migration. A stack vendor needs: bit-exact codeword handling (CRC-15 + parity, SYNC/SYNCT), FFSK modem or interface to it, control-channel
timing, call-setup/registration state machines, and **interworking** (IWF) to digital systems for gradual migration. Also, understanding MPT 1327 shows the
common LMR shape: sync word → codeword with CRC → state machine, as in P25/DMR.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Nobody uses it." | Old. | Still deployed; migration is gradual. |
| "FFSK is the MPT protocol." | It's the modem. | MPT 1327 is the signalling above it. |

---

## Self-check answers
36 kbit/s, 510 bits, 14.167/56.667/1020 ms, 61.2 s ✔; 7200/28 800 ✔; 17 × 60 = 1020 ✔; layers MAC/LLC/MLE/CMCE/MM ✔; dPMR 4800 bit/s, 6.25 kHz ✔;
600 Hz tone spacing ✔; burst-≤15 property ✔.
