# Pack 06/04 — MCPTT/MCX and the LMR interworking function (IWF): GTP-U parser, NR numerology calculator, floor/identity mapping

> **SAFETY / LEGAL.** Synthetic packets and identities only. **Never** capture, decode or replay live cellular or public-safety radio traffic.
> Parse only bytes you build in tests or capture on **your own isolated test network**. UK: Wireless Telegraphy Act 2006 / Investigatory Powers Act 2016 — VERIFY for your jurisdiction.
> Reading **srsRAN** (§3.6) is allowed; its licence (AGPL-3.0 — **VERIFY**) restricts copying code — **do not copy source into your repo**; read it, close it, write your own from the spec.

Standards (from memory — **VERIFY**; I never invent clause numbers): 3GPP **TS 22.179** (MCPTT requirements), **TS 23.179** (MCPTT architecture), **TS 23.280** (common MC architecture), **TS 24.379** (MCPTT call control),
**TS 24.380** (MCPTT media/floor control — MBCP), **TS 26.179** (MCPTT codecs), **TS 33.180** (MC security), **TS 23.282** (MCData), **TS 23.281** (MCVideo),
**TS 23.283** (MC interworking with LMR — Rel-15, **VERIFY number and scope**), **TS 29.281** (GTP-U).

---

## 1. Why this matters (Etherstack context)

This is the pack closest to Etherstack's gateway work: **LMR (P25/DMR/TETRA) on one side, broadband mission-critical (MCPTT/MCX over LTE/5G) on the other, and an IWF between them**.
Four deliverables train four skills: (1) *draw the architecture from memory* (the interview whiteboard); (2) **GTP-U parser** — defensive binary parsing (the user-plane glue under every LTE/5G gateway);
(3) **NR numerology calculator** — turning formulae into a tool (pack 03 as code); (4) **read a real stack's header packer** (srsRAN) without copying it.

## 2. Prerequisites

Pack 06/02 §3.5 (GTP-U overview, QCI), pack 06/03 §3.1–3.3, pack 07/05 (floor control, IWF), a hex editor in your head (`0x34 = 0011 0100`).

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
./build/nrcalc 1 20      # demo built from the solution: prints a numerology row (mu=1, 20 MHz)
```

## 3. Concept build-up

### 3.1 GTP-U header — read the bytes, then write the parser

A GTP-U packet is `[ header | payload (T-PDU) ]` over UDP port 2152. The **first byte** is a bitfield:

| Bits | 7-5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|
| Field | version (=1) | PT (1 = GTP, 0 = GTP') | spare | **E** ext | **S** seq | **PN** N-PDU |

So `0x30 = 001 1 0 0 0 0` → version 1, PT 1, no optionals (the normal G-PDU case). `0x32` → S set; `0x34` → E set. Next: **message type** (0xFF = G-PDU: user data), **length** (16-bit, big-endian: bytes **after the first 8**),
then a 32-bit **TEID** (tunnel endpoint ID; names the bearer/session at the receiver). Total mandatory header = **8 bytes**.

**Optional 4 bytes** appear if **any** of E/S/PN is set (even if only E!): sequence number (16 bits), N-PDU number (8), next-extension-header type (8) — **regardless of which flags are set, all four bytes are present**; individual fields are only *meaningful* if their flag is set.
Then, if the next-ext type ≠ 0, a **chain of extension headers**: each is `[length in 4-byte units][content…][next type]`; a next type of 0 ends the chain. 5G uses `0x85` — the **PDU Session Container** (QFI etc.; VERIFY).
`header_len` = 8 (+4 optional) + Σ(ext length × 4) = offset of the payload.

Tests: `0x30…` → `header_len 8`; `0x32…` (S) → 12; `0x34…` with one extension → 16; two chained extensions → 20.

**Rules for a safe parser** (this is the interview point):
1. **Never read past `len`.** Check available bytes before each field.
2. **Never trust the length field.** `8 + length` must not exceed the buffer (`LengthMismatch`); optional bytes must fit inside `length` (flags set but length < 4 → `LengthMismatch`).
3. **Extension length 0 is illegal** (infinite loop!) and an extension that overruns the packet is `BadExtension`.
4. **Reject, don't guess**: version ≠ 1 → `BadVersion`; PT = 0 → `NotGtpu`; fewer than 8 bytes → `TooShort`.
5. **Return a status, not an exception or `assert`**: network input is hostile by default (fuzz it with ASan/UBSan; this pack's tests do exactly that).

> **Misconception box — "length covers the whole packet".** It excludes the first 8 mandatory bytes: a header-only G-PDU has length 0, and the optional/extension bytes **are** counted.
> **Misconception box 2 — "the E flag alone means there's a 4-byte-aligned extension *instead of* the optional bytes".** No: E, S, PN each *also* switch on the same 4 optional bytes.

### 3.2 Building a GTP-U packet

`build_gtpu(teid, payload, n, with_seq, seq)`: flags `0x30` (or `0x32` with S), type `0xFF`, `length = n (+4 if with_seq)`, TEID big-endian, then (if seq) `seq(2) | 0 | 0`, then the payload.
Hand-check: payload `45 00`, TEID `A1B2C3D4`, no seq → `30 FF 00 02 A1 B2 C3 D4 45 00`. With seq `0x0102` and TEID 1 → `32 FF 00 06 00 00 00 01 01 02 00 00 45 00`.
**Round trip test** (`BuildThenParseRoundTrip`): `parse(build(x)) == x` — the cheapest property test in the whole pack, and the first one you should write for any codec.

### 3.3 The NR numerology calculator (pack 03 as a tool)

`numerology_row(μ, bw_MHz)` returns `{μ, SCS, slots/subframe, slot ms, useful symbol µs, PRBs, occupied MHz}`; PRBs come from the given `nr_prb_count` table (**VERIFY** against TS 38.101).
`occupied = PRBs × 12 × SCS`: (μ=0, 20 MHz) → 106 PRB, **19.08 MHz**; (μ=1, 20 MHz) → 51 PRB, **18.36 MHz**; (μ=3, 100 MHz) → 66 PRB, **95.04 MHz**; no table entry (μ=2, 20 MHz) → PRBs −1, occupied −1; μ=7 → row with μ −1.
`nrcalc` prints `mu=1 SCS=30 kHz slots/subframe=2 slot=0.5000 ms symbol=33.333 us PRBs=51 occupied=18.36 MHz`. *Design lesson:* **sentinel values (−1) for unknown, not exceptions or silent zeros**, and a separate `format_row` so the logic is testable without capturing stdout.

### 3.4 MCPTT/MCX in one page — the architecture you must draw from memory

**Releases (VERIFY):** **Rel-13** — **MCPTT** (group/private calls, emergency, floor control, off-network ProSe); **Rel-14** — **MCData** (SDS, file transfer, IP data) and **MCVideo**; **Rel-15** — **interworking with LMR** (TS 23.283), more MCX; later releases add interworking with legacy and 5G support (VERIFY).

```
 UE (MCX client) --LTE/5G--> EPC/5GC (P-GW/UPF) --> IMS core (SIP) --> MC server (MCPTT AS: call control + floor participant/controlling)
                                                         |                 | MC common services: identity management, group management,
 media: RTP (AMR-WB / EVS), floor: MBCP (TS 24.380)     HSS/UDM            | configuration management, key management (TS 23.280)
                                                                           +--> IWF --> LMR system (P25 ISSI/CSSI, DMR, TETRA)  [TS 23.283]
```

* **Signalling** over **SIP/IMS** (TS 24.379); **media** over **RTP**; **floor control** via **MBCP** (Media Burst Control Protocol) *carried alongside the media* (TS 24.380) — not SIP.
* **Why app-layer floor control when LTE already has QoS?** QoS (QCI 65/69, ARP preemption) decides **who gets bandwidth**; floor control decides **who is allowed to talk** in a half-duplex group call (one talker, queue, priority, emergency override, revoke). They are different problems.
* **Bearers:** MCPTT voice rides a **dedicated bearer, QCI 65** (75 ms budget), signalling QCI 69 (60 ms) — see pack 02 Q13 (VERIFY table). **eMBMS/MBMS** can broadcast one stream to a whole group (efficient for large groups), with unicast fallback.
* **Off-network (ProSe/sidelink)** MCPTT works with no network at all: direct device-to-device, the broadband analogue of LMR simplex.
* **Latency targets** (TS 22.179 defines an "MCPTT access time" and an end-to-end access time — on the order of a few hundred ms; **VERIFY the actual values**) shape the whole design: the IWF cannot spend 500 ms transcoding before sending audio.

### 3.5 The IWF: four things to map

An LMR↔MCPTT gateway translates **identity**, **floor**, **voice**, **security**:

| Domain | LMR side | MCPTT side | Test in this pack |
|---|---|---|---|
| **Floor** | PTT press/release, emergency, channel busy | FloorRequest / Release / Request-Emergency (MBCP messages) | `lmr_to_mcptt` |
| **Floor result → radio behaviour** | grant tone/TX permit, busy tone, TX terminate, channel idle | Granted / Denied / Taken (someone else) / Revoked / Idle | `mcptt_to_lmr` |
| **Identity** | talkgroup id (16-bit), unit id | MC group identity (URI), MCPTT ID (user) | `make_group_uri`, `TalkgroupMap` |
| **Voice** | IMBE/AMBE+2/ACELP vocoder frames | AMR-WB / EVS, RTP | *transcoding:* tandem-coding loss + latency (pack 07/05 §3.8) |
| **Security** | LMR keys (OTAR/KMF) | MC key management (TS 33.180) | *no crypto in this repo* |

**Floor mapping:** `PttPressed → FloorRequest`; `PttReleased → FloorRelease`; `EmergencyPttPressed → FloorRequestEmergency` (priority + override); `ChannelBusy → None` (it is **information from the LMR side's own arbitration**, not a request).
**Reverse:** `Granted → TxPermit`; `Denied → BusyTone`; `Taken → BusyTone` (someone else holds the floor: you may not talk — same user experience as denied); `Revoked → TxTerminate` (you *were* talking and lost it: **cut the transmitter now**); `Idle → ClearChannel`.
*Why separate Denied and Revoked?* Different **state**: denied = never transmitted; revoked = on the air right now: the radio must stop transmitting to avoid interfering with the new talker.

**Identity:** model URI `sip:tg<ID>@<domain>` (`make_group_uri(1234, "mcptt.example.org") = sip:tg1234@mcptt.example.org`; real MC group identities are specified in TS 23.179/23.280 — **VERIFY**).
`parse_group_uri` is **strict**: prefix `sip:tg`, digits only, `@` present, value ≤ 65535, otherwise `nullopt` (never "best effort"). `TalkgroupMap` must be **one-to-one**: `add` refuses a duplicate LMR id **or** a duplicate URI — a many-to-one map would merge two LMR groups into one MCPTT group (cross-talk), one-to-many would send one talker's audio to two different groups.

> **Misconception box — "interworking = transcoding the voice".** Voice is the *visible* part; most bugs live in **floor and identity mapping** (who is talking, who may talk, which group). A gateway that transcodes perfectly but maps floors wrongly gives double talkers or dead air.

### 3.6 The srsRAN reading exercise (describe, don't copy)

**Goal:** see how a production C++ stack packs/unpacks a protocol header, then compare it with your 80-line parser.
1. Obtain the **upstream** source yourself (`github.com/srsran/srsRAN_Project` or `github.com/srsran/srsRAN_4G` — **VERIFY** current layout and licence). Do **not** paste any of it into this repository.
2. Find **one** header pack/unpack routine (e.g. the GTP-U header handling, or PDCP/RLC header packers — locate by searching for `gtpu`, `pack_header`, `unpack`).
3. Fill in a table **in your own words**: *bit-field order; byte order; how lengths are validated; what happens on malformed input (status? exception? log?); how optional fields are modelled; what is tested.*
4. Write 5 differences between its approach and yours, and **one improvement each way**.
5. Re-implement a *different* header (e.g. the PDCP data PDU header for 12-bit SN, TS 36.323/38.323 — VERIFY layout) from the spec, test-first, in `scratch/` (not committed).
Deliverable: a half-page "what I learned" paragraph you can say aloud in an interview. (Rubric in `answers.md` Q14.)

## 4. Worked examples

**(a) Hand-parse:** `34 FF 00 0A 00 00 01 00 | 00 00 00 85 | 01 00 09 00 | DE AD`. Flags 0x34 → E; length 10 = 4 optional + 4 extension + 2 payload ✓; TEID 0x00000100; optional: seq 0, N-PDU 0, next-type 0x85; extension: length 1 (4 bytes), content `00 09`, next 0 → end; header_len = 8 + 4 + 4 = **16**.
**(b) Truncated:** `30 FF 00 10 … ` claims 16 bytes after the first 8 but only 3 present → `LengthMismatch`, no read beyond the buffer.
**(c) Floor race:** radio A presses PTT → `FloorRequest`; meanwhile MCPTT user B was granted: A's IWF receives `Taken` → `BusyTone`. A never transmits a single frame.

## 5. Questions

1. **Q1 (Draw)** — From memory: UE/MCX client → LTE/5G → IMS → MC server → IWF → LMR system. Label SIP/RTP/MBCP, the common services, QCI 65/69. (W 3.4)
2. **Q2 (Explain)** — MCPTT (Rel-13), MCData/MCVideo (Rel-14), LMR interworking (Rel-15): what did each release add? What would you VERIFY? (W 3.4)
3. **Q3 (Explain)** — Why does MCPTT need its own floor control when the bearer already has QoS? (W 3.4)
4. **Q4 (Calculate)** — Decode the first byte for `0x30`, `0x32`, `0x34`, `0x36`, `0x20`, `0x50`: version/PT/E/S/PN and verdict. (W 3.1)
5. **Q5 (Calculate)** — `header_len` and `length` for: plain G-PDU (100-byte payload); with seq; with one 4-byte extension; with two. (W 3.1)
6. **Q6 (Explain)** — List the five defences a binary parser needs; give one test per defence. (W 3.1)
7. **Q7 (Write)** — Build the bytes for TEID `0xA1B2C3D4`, payload `45 00`, with and without sequence 0x0102. (W 3.2)
8. **Q8 (Calculate)** — `nrcalc` rows for (μ=0, 20 MHz), (μ=1, 20 MHz), (μ=3, 100 MHz), (μ=2, 20 MHz), μ=7. (W 3.3)
9. **Q9 (Design)** — Why return −1 sentinels / `nullopt` instead of throwing, in a gateway? When would you throw instead? (W 3.3, 3.5)
10. **Q10 (Design)** — Complete the LMR → MCPTT and MCPTT → LMR floor tables. Why does `ChannelBusy` produce nothing? Why do Denied and Taken share a tone but Revoked doesn't? (W 3.5)
11. **Q11 (Design)** — Why must the talkgroup map be one-to-one? Show the failure of many-to-one and one-to-many. (W 3.5)
12. **Q12 (Design)** — Why must `parse_group_uri` be strict? List five malformed inputs and their consequence if accepted. (W 3.5)
13. **Q13 (Explain)** — The four domains of interworking (floor, identity, voice, security) — which is hardest and why? Latency budget for PTT-to-first-audio. (W 3.5)
14. **Q14 (Exercise)** — The srsRAN reading table: fill it in for one routine and state your comparison. (W 3.6)
15. **Q15 (Interview)** — "How would you bridge a DMR talkgroup into an MCPTT group?" 3-minute whiteboard answer. (W 3.4–3.5)
16. **Q16 (Etherstack)** — Where would an LMR gateway sit relative to the P-GW/UPF, and what must it never do with traffic from a live system? (W 3.4; safety box)

## 6. Self-assessment

- [ ] Draw the MCX architecture in 90 seconds with SIP/RTP/MBCP and QCI 65/69.
- [ ] Decode any GTP-U first byte; compute `header_len` with extensions; list five parser defences.
- [ ] State what Rel-13, Rel-14, Rel-15 added **and** that TS 23.283 is verified-by-lookup.
- [ ] Explain why the floor table has asymmetric mappings and why talkgroup maps are bijections.
- [ ] Describe how you read (not copied) an upstream header packer.

**VERIFY list:** TS 23.283 number, scope and release (Rel-15); MCData/MCVideo spec numbers (TS 23.282/23.281); MBCP in TS 24.380; QCI 65/69 values; GTP-U ext type 0x85 (PDU session container) and flag bit positions (TS 29.281);
real MC group-identity URI format; MCPTT access-time targets (TS 22.179); srsRAN licence and file locations; PDCP SN layouts (TS 38.323); PRB table (TS 38.101).
