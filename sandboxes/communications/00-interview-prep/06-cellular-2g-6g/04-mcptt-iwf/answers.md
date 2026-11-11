# Pack 06/04 — Answers (MCPTT and the LMR IWF)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic data only. **VERIFY** = check 3GPP before quoting.

---

## Q1 — Draw: MCX architecture (model)

**Answer.** MCX client (UE) —LTE/5G radio→ eNB/gNB → EPC/5GC (P-GW/UPF) → IMS core (SIP: TS 24.379) → **MC server(s)** (MCPTT AS: call control, floor control) with **common services** (identity, group, configuration, key management: TS 23.280) → **IWF** → LMR system. Media: RTP (AMR-WB/EVS); floor: MBCP (TS 24.380) on the media path; voice bearer QCI 65, signalling QCI 69 (VERIFY); optional eMBMS for group broadcast; ProSe for off-network.

**Rubric:** client, bearer(s), IMS/SIP, MC server, common services, IWF, LMR side, one protocol label per link, QCI.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Floor control is SIP" | SIP does call setup. | Floor control is MBCP (TS 24.380), carried with media. |
| "MCPTT is a codec" | 'Push-to-talk'. | It is a service and architecture; codecs are AMR-WB/EVS. |

## Q2 — Explain: releases

**Answer.** Rel-13: **MCPTT** (group/private calls, emergency, floor control, off-network). Rel-14: **MCData** (short data, file, IP data) and **MCVideo**. Rel-15: **interworking with LMR** (TS 23.283) and enhancements. VERIFY exact release for each spec and the later MCX evolution (5G).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "MCPTT is Rel-8" | LTE start. | Public-safety features arrived with Rel-12 (ProSe)/Rel-13. |

## Q3 — Explain: floor control vs QoS

**Answer.** QoS (QCI/ARP) allocates and prioritises **bandwidth and delay** per bearer; floor control arbitrates **who may speak** in a half-duplex group (grant, queue, deny, preempt, revoke). With perfect QoS, two users pressing PTT would still talk over each other.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "High priority bearer = gets the floor" | Priority sounds like authority. | Priority is an input to floor decisions, not the decision. |

## Q4 — Calculate: first bytes

| Byte | Binary | Version | PT | E | S | PN | Verdict |
|---|---|---|---|---|---|---|---|
| 0x30 | 001 1 0 0 0 0 | 1 | 1 | 0 | 0 | 0 | OK (mandatory 8-byte header) |
| 0x32 | 001 1 0 0 1 0 | 1 | 1 | 0 | 1 | 0 | OK, 4 optional bytes |
| 0x34 | 001 1 0 1 0 0 | 1 | 1 | 1 | 0 | 0 | OK, optional + extension chain |
| 0x36 | 001 1 0 1 1 0 | 1 | 1 | 1 | 1 | 0 | OK |
| 0x20 | 001 0 0 0 0 0 | 1 | **0** | 0 | 0 | 0 | **NotGtpu** (GTP') |
| 0x50 | 010 1 0 0 0 0 | **2** | 1 | 0 | 0 | 0 | **BadVersion** |

**Reasoning.** version = bits 7–5, PT = bit 4, bit 3 spare, E bit 2, S bit 1, PN bit 0 (VERIFY TS 29.281).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "0x32 is version 3" | Reading the low nibble. | Version is the top 3 bits. |

## Q5 — Calculate: lengths

| Packet | length field | header_len |
|---|---|---|
| plain, 100-byte payload | 100 | 8 |
| with seq | 104 | 12 |
| with one 4-byte extension (E set) | 4 + 4 + payload | 16 |
| with two chained | 4 + 8 + payload | 20 |

**Reasoning.** `length` counts everything after the first 8 bytes; `header_len` is the payload offset.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "length = 108 for plain" | Add header. | Mandatory 8 bytes are *not* in length. |
| "E alone: no optional 4 bytes" | Seems logical. | Any of E/S/PN ⇒ the 4 optional bytes exist. |

## Q6 — Explain: parser defences

1. **Bounds**: check remaining bytes before each read (test: 3-byte packet → TooShort).
2. **Version/PT**: reject unknown (BadVersion, NotGtpu tests).
3. **Length consistency**: `8 + length ≤ len` (trunc test → LengthMismatch); optional bytes ≤ length (flags_no_room test).
4. **Extension validity**: length ≥ 1 and not overrunning (zero_ext, overrun tests → BadExtension) — also guarantees loop termination.
5. **Status not UB**: always initialise the header, return an enum; run under ASan/UBSan and fuzz.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The network is trusted internally" | Core is private. | Misconfigured/compromised peers; lateral movement. |
| "Use exceptions" | Idiomatic C++. | Fine, but hot paths prefer error codes; and *never* leave state half-built. |

## Q7 — Write: build bytes

**Answer.** `30 FF 00 02 A1 B2 C3 D4 45 00`; with a sequence number (shown for TEID 1 and seq 0x0102, as in the test): `32 FF 00 06 00 00 00 01 01 02 00 00 45 00` (length 6 = 4 optional + 2 payload).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "length 02 with seq" | Payload size only. | Optional bytes count in length. |
| TEID little-endian | x86 habit. | Network byte order. |

## Q8 — Calculate: nrcalc

| Call | Result |
|---|---|
| (0, 20) | SCS 15, slot 1.0 ms, symbol 66.667 µs, **106 PRB, 19.08 MHz** |
| (1, 20) | SCS 30, slot 0.5 ms, 33.333 µs, **51 PRB, 18.36 MHz** |
| (3, 100) | SCS 120, 0.125 ms, 8.333 µs, **66 PRB, 95.04 MHz** |
| (2, 20) | SCS 60 … **PRBs −1, occupied −1** (not in table) |
| (7, 20) | row with **μ = −1** (invalid) |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "occupied = bandwidth" | Same 'MHz'. | Occupied = PRB·12·SCS (guard bands excluded). |

## Q9 — Design: sentinels vs exceptions

**Answer.** Lookup failure is *expected*, not exceptional; sentinel/`optional` keeps call sites explicit, avoids exceptions across embedded/real-time boundaries (no unwinding in an ISR/hot path) and allows `-fno-exceptions` builds (embedded pack 08/04). Throw when an **invariant** is broken and the caller cannot continue (programming error), or at a boundary where error codes would be silently ignored. Document sentinel semantics and test them.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Return 0 for unknown PRBs" | Simple. | 0 is a plausible value; silently wrong calculations. |

## Q10 — Design: floor mapping

| LMR event | MCPTT message |
|---|---|
| PttPressed | FloorRequest |
| PttReleased | FloorRelease |
| EmergencyPttPressed | FloorRequestEmergency |
| ChannelBusy | None |

| MCPTT floor event | LMR action |
|---|---|
| Granted | TxPermit |
| Denied | BusyTone |
| Taken | BusyTone |
| Revoked | TxTerminate |
| Idle | ClearChannel |

**Reasoning.** ChannelBusy is *state of the LMR channel*, not a user request; forwarding it as a request would loop the arbitration. Denied/Taken give the same feedback (can't talk now); Revoked demands an immediate action because the transmitter is **on the air** (interference, double talker).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Revoked → BusyTone" | Same as denied. | The radio is transmitting: must terminate TX. |
| "ChannelBusy → FloorRequest" | Busy ≈ contention. | It is an input to *local* arbitration. |

## Q11 — Design: one-to-one map

**Answer.** Many-to-one (LMR 100 and 101 → same URI): two LMR groups hear each other — cross-talk, a **confidentiality** breach. One-to-many (LMR 100 → two URIs): one speaker's audio goes to two MCPTT groups; reverse mapping ambiguous (which LMR group should MCPTT audio go to?). `add` refuses both; a conflict is a **configuration error** to be surfaced, not silently overwritten.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Overwrite on duplicate" | Last wins is simple. | Silent reconfiguration of live traffic. |
| "Many-to-one is useful for patching" | Patching groups exists. | That's an explicit *patch* with its own rules, not a mapping. |

## Q12 — Design: strict URI parsing

| Input | Why dangerous if accepted |
|---|---|
| `sip:tg@x` (no digits) | tgid 0 → routes to "no group"/default |
| `sip:tg12a@x` | atoi-style parse yields 12 → wrong group |
| `sip:tg70000@x` | truncation to 16 bits → 4464 → **another group's traffic** |
| `tel:+441234` | wrong scheme → treated as TG |
| `sip:tg12` (no `@`) | domain missing → unauthenticated/unknown realm |

**Reasoning.** Identity strings are an **injection surface**; accepting loosely turns typos and attacks into misrouted voice. Strict parse + one test per rejection.

## Q13 — Explain: four interworking domains

**Answer.** Floor, identity, voice, security. Hardest in practice: **security** (key management across two worlds; transcoding forces decrypt/re-encrypt at the gateway — it becomes a trusted element) and **floor timing** (latency, race conditions); voice is mostly engineering (tandem-coding quality, 20–60 ms extra delay). Latency budget: keep gateway processing ≪ 100 ms so that PTT-to-first-audio stays within the system's few-hundred-ms target (VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Voice transcoding is the whole problem" | It's audible. | Wrong floor/identity = dead air or leaks. |

## Q14 — Exercise rubric (srsRAN reading)

A good table answers: bit order (MSB-first? packed with shifts or bitfield struct?), endianness handling, length validation, error propagation (bool/`expected`/exception/log), optional field model, test coverage (unit tests/fuzz). Typical contrasts: production code is **templated, buffer-abstraction-based (byte_buffer), avoids allocation**, may log-and-drop; yours is a plain-pointer parser returning status. Improvement for yours: zero-copy views, fuzz target; for theirs (as you read it): maybe stricter reject paths — only claim what you actually saw. **Never claim to remember code you didn't read** (VERIFY by reading).

## Q15 — Interview: DMR talkgroup → MCPTT group (model answer)

"I'd put an IWF behind IMS: (1) **identity**: a configured one-to-one map DMR talkgroup ↔ MC group URI, validated at load; (2) **floor**: the DMR side PTT/emergency events become MBCP requests; MBCP grants/denials/revokes drive talk-permit, busy tone or TX-terminate on the DMR side; (3) **voice**: AMBE+2 ↔ AMR-WB/EVS transcode (tandem loss accepted, latency budgeted) with a jitter buffer on the IP side; (4) **security**: no cleartext leakage — keys remain in each domain's KMS; gateway is a trust point. Tests: floor race, late entry, emergency preemption, packet loss. Metrics: PTT-to-audio latency, floor-denied rate, loss, jitter."

## Q16 — Etherstack: placement and rules

**Answer.** Behind the P-GW/UPF (SGi/N6) on the IP side and as an IMS/MC peer; on the LMR side via ISSI/CSSI-style or radio-site interfaces (VERIFY per technology). It must **never** be fed with, or be used to capture, live public-safety traffic outside an authorised deployment; development and tests use synthetic streams; no decryption of encrypted traffic.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Test with a live scanner feed" | Real data. | Unlawful in many places; the repo's rule is synthetic only. |
