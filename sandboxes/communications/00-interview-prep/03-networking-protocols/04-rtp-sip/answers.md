# 03-net / 04 — Answers

**Q1.** `81` = `10 0 0 0001` → V = 2, P = 0, X = 0, CC = 1. `80` = M 1, PT 0 → **PCMU (G.711 µ-law)**, marker set = first packet of a talkspurt. `1f 40` = seq **8000**. `00 01 38 80` = timestamp **80 000** = 10.0 s of media time at 8 kHz *relative to the stream's random origin* (not wall-clock). `ca fe ba be` = SSRC. `01 02 03 04` = one CSRC (CC = 1). Header = 12 + 4 = **16 bytes**; payload at offset **16** (160 B of `ff`: µ-law silence).

**Q2.** (a) **160** (20 ms × 8 000). (b) **960** (20 ms × 48 000, regardless of the actual Opus sample rate). (c) **160**: G.722 samples at 16 kHz but the RTP spec registered its clock as 8 000 — a historical error (the 64 kbit/s ADPCM stream carries 8 000 octets/s) kept for compatibility (RFC 3551 §4.5.2 ⚠ VERIFY). (d) The previous packet covers ts 16 000–16 159, so media time after it is 16 160; plus 500 ms = 4 000 ticks → **ts = 20 160**, **seq = 101** (no packet lost), **M = 1** (new talkspurt).

**Q3.** The ts jump (20 160 − 16 000 = 4 160 ticks = 26 frames of 160) says "520 ms of media time passed since the last packet began"; the seq says "next packet in order, nothing lost". Together: *silence*. With only seq the receiver would butt the new burst up against the old one (no pause). With only the timestamp, 26 frames missing could equally be 26 *lost* packets; the contiguous seq proves they weren't sent.

**Q4.** Seq: 65 536 / 50 = **1 310.7 s ≈ 21.8 min**; at 100 pps **655 s ≈ 10.9 min**. Timestamp: 2³² / 8 000 = 536 871 s ≈ **6.2 days**; /48 000 = 89 478 s ≈ **24.9 h**. The sequence number wraps within minutes → defended by `seq_newer` (signed 16-bit difference = serial arithmetic) and `SeqTracker` (extended 32-bit sequence with a cycle counter); timestamps by `ts_diff` (int32 difference, valid for differences < 2³¹ ticks).

**Q5.** `p ≈ N² / 2³³`: (a) 2 500 / 8.59e9 ≈ **2.9 × 10⁻⁷**; (b) 10⁶ / 8.59e9 ≈ **1.2 × 10⁻⁴**. On detecting that another source uses *your* SSRC (different transport address/CNAME): send **RTCP BYE** for the old SSRC, pick a new random SSRC, reset sequence/timestamp state as appropriate, remember the conflicting address so loops are avoided (RFC 3550 §8.2). A gateway that mixes should also avoid reusing any received SSRC as its own.

**Q6.** On `M = 1` the jitter buffer knows a talkspurt starts: it can **re-base** its playout point — silently stretching or shrinking the (inaudible) gap between bursts to follow the current jitter estimate, and reset PLC/decoder state. `M` on every packet: the receiver would re-base at every packet → constant playout-point changes, unstable delay, audible glitches (or the receiver learns to ignore it). `M` never set: still decodable — the receiver infers talkspurts from timestamp gaps — but adaptation is only possible at the cost of audible adjustments.

**Q7.** **Mixer.** The output waveform is a *sum*; it is a new source, so SSRC = the gateway's own mixer SSRC, **CC = 3**, **CSRC = [ssrc_A, ssrc_B, ssrc_C]** (the radio-side source ids you track). A *translator* preserves a single input's SSRC and is only appropriate for 1-to-1 transforms (transcode, relay); it cannot represent a combined stream. The CSRC list lets the LTE-side client display who is speaking.

**Q8.** SR + 1 block: 4 (header) + 4 (SSRC) + 20 (sender info) + 24 (block) = **52 B**; length field = 52/4 − 1 = **12**. RR + 2 blocks: 4 + 4 + 48 = **56 B**; length = **13**.

**Q9.** Units = ms × 8: arrivals 0, 168, 304, 520, 632, 824. Transit `T = R − S`: 0, 8, −16, 40, −8, 24. `D = |T_i − T_{i−1}|`: 8, 24, 56, 48, 32. `J`: 0.5 → 1.969 → 5.346 → 8.012 → **9.511 units** → 9.511 / 8 ≈ **1.19 ms**.

**Q10.** Expected = 1 099 − 1 000 + 1 = **100**; cumulative lost = 100 − 95 = **5**. Interval: expected 50, received 49 → lost 1 → 1 × 256 / 50 = 5.12 → **5/256 ≈ 1.95 %** (true 2 %; truncation). Cumulative lost can go negative with duplicates (signed field).

**Q11.** `A − LSR − DLSR = 0x39EB8 − 0x30000 − 0x8000 = 0x1EB8 = 7 864` units of 1/65 536 s → 0.12 s = **≈ 120 ms** (119.995 ms). If `LSR = 0`, the reporter has not yet received an SR from you → no RTT can be computed (`rtt_ms` returns −1).

**Q12.** Session bandwidth = 80 kbit/s → RTCP = 5 % = **4 kbit/s**. Two members, 120-byte (960-bit) compound packets: `n × size / bw = 2 × 960 / 4000 = 0.48 s` — below the **5 s minimum**, so the interval is **≈ 5 s** (randomised to 0.5–1.5 ×, and the first report is sent after ≈ 2.5 s). Wireshark therefore shows an SR/RR roughly every 5 s per direction. ⚠ VERIFY constants in RFC 3550 §6.2–6.3.

**Q13.** RTCP on **49 171** (RTP port + 1). With **rtcp-mux** (`a=rtcp-mux`) both share 49 170 and the receiver demultiplexes by the second byte (RTCP PT 192–223 vs RTP PT/M). Effect: one firewall pinhole and one NAT binding instead of two; fewer ports; but both ends must support it, and the SDP must say so (or use `a=rtcp:` to name a different RTCP port explicitly).

**Q14.** Reasons: (1) **scale** — proxies see KB, not Mbit/s; (2) **QoS** — reliable vs low-latency; (3) **independent failure/lifetime**; (4) **flexibility** — move/hold/transcode media without redoing signalling; (5) **security/policy** — authenticate and bill at the signalling plane, encrypt media separately; (6) PSTN heritage (SS7 vs trunks). Failure examples: **SDP contains a private IP** behind NAT → signalling succeeds, call "connected", **no audio / one-way audio**; or **lost BYE** → one side keeps media running and holds resources → mitigate with session timers (RFC 4028 ⚠), RTCP-timeout/ICMP policies.

**Q15.**
```text
UAC                                UAS
 |-- INVITE [SDP offer, Via branch, Call-ID, From tag] -->|     INVITE lost: UAC retransmits (Timer A: 500 ms, 1 s, 2 s, … up to 32 s)
 |<-- 100 Trying ----------------------------------------|
 |<-- 180 Ringing [To tag added] -------------------------|
 |<-- 200 OK [SDP answer, To tag] ------------------------|     200 lost: UAS retransmits 200 until ACK
 |-- ACK ------------------------------------------------>|     UAC re-ACKs every retransmitted 200
 |<======== RTP both ways (first RTP after SDP is known) =>|
```
The UAS **must** keep resending the 200 until the ACK arrives (the INVITE is a three-way handshake). Over TCP the transport retransmits, but the transaction timers still guard against a dead peer.

**Q16.** Copy **Via** (all, in order), **From** (unchanged), **To**, **Call-ID**, **CSeq**. Add: **To tag** (`;tag=…`) for a dialog-creating response, `Content-Length: 0` (or the body length), and for 1xx/2xx to INVITE normally a **Contact** (and often `Server`). Do **not** add a tag if the request's To already has one (in-dialog request such as BYE/re-INVITE — test `ResponseDoesNotOverwriteExistingToTag`). RFC 3261 §8.2.6.2 also says a **100 Trying** carries no new tag ⚠ VERIFY.

**Q17.** *Dialog* = `Call-ID` + local tag + remote tag: the long-lived peer relationship (re-INVITEs, BYE belong to it). *Transaction* = one request and its responses, matched by the top **Via branch** (+ sent-by + method); the branch (starting `z9hG4bK`, the RFC 3261 "magic cookie") must be unique per transaction. `CSeq` orders requests inside a dialog. If a proxy forgets its own Via, responses skip it: it can't run its server transaction state, billing or loop detection, and the upstream may see a response to a request it didn't send to that hop.

**Q18.** Offer formats `96 0 8`; PCMA (8) is the only intersection (opus unsupported; G.729 not offered; 0 PCMU unsupported). Answer:
```text
m=audio <your port> RTP/AVP 8
a=ptime:20
a=sendrecv
```
(PT 8 is static: no rtpmap needed.) Only G.729 supported and 18 not offered → no common format → reject: **488 Not Acceptable Here** (usually with a `Warning: 305 Incompatible media format`), or for one stream accept the dialog and answer `m=audio 0 RTP/AVP 96` to reject the media — in single-media calls 488 is conventional. (An SBC with a transcoder would instead pick PCMA and transcode.)

**Q19.** Offer:
```text
v=0
o=gw 1 1 IN IP4 10.0.0.2
s=-
c=IN IP4 10.0.0.2
t=0 0
m=audio 5004 RTP/AVP 0 101
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-15
a=ptime:20
a=sendrecv
```
Hold re-INVITE: same dialog, `o=gw 1 2 …` (version +1), `a=sendonly`. Answer: `o=… <their id> <their version> …`, same codec, **`a=recvonly`**, and its own address and port unchanged.

**Q20.** AMR-WB's RTP clock is 16 000 Hz: 20 ms = **320 ticks** per packet (no payload-size relation). Opus would be 960. Since AMR-WB is first in the offer and supported, the answer picks 97; both ends then expect ts += 320.

**Q21.** Bugs: (1) **bit-fields**: order and layout are implementation-defined — build bytes with shifts; (2) `memcpy` of a struct → no **network byte order** for seq/ts/ssrc (`htons/htonl`); (3) `r.seq += n`: increments by *bytes* (should be +1 per packet) and `r` is a *fresh local each call* so seq restarts at 0 — state must live in a per-stream object with a **random initial seq**; (4) `ts = now_ms()` wrong unit — should be **sample ticks**, +160 per 20 ms G.711 packet, random start; (5) `ssrc = 1` constant — should be random per stream (collisions); (6) marker never set at talkspurt start; (7) `buf[2000]` overflow if `n` > 1988 — validate against the MTU budget (≈ 1 460 B); (8) `send` result unchecked (partial/failed send); (9) `sizeof r` assumed 12; (10) no per-stream pacing/DSCP.

**Q22.** *Sure:* SIP/SDP sets up the session (INVITE + 200 + ACK, BYE to end; re-INVITE to modify); RTP carries the voice once the talker has the floor; RTCP carries receiver reports/SR. *Flag as ⚠ VERIFY:* in 3GPP MCPTT (TS 24.379/24.380) the **floor control** messages (Floor Request/Granted/Deny/Release/Taken/Idle) are sent as **RTCP APP-based MBCP** over UDP on a dedicated `m=application` media stream negotiated in SDP, while the PTT button maps to a Floor Request; the call itself (group/private, pre-arranged or chat) is established with SIP.

**Q23.** Outline: user presses PTT → gateway INVITE (offer: G.711 + telephone-event + floor-control stream) → 100/180/200 (answer) → ACK → **floor grant** → RTP frames every 20 ms (M on first, seq +1, ts +160, SSRC fixed) → RTCP SR/RR every ~5 s (loss, jitter, RTT) → PTT release → floor release → BYE/200. In Wireshark when choppy: *Telephony → RTP → RTP Streams → Analyze*: (1) **lost packets %** and **sequence errors**; (2) **jitter** mean/max (compare with your jitter buffer depth); (3) **delta** max / skew (are 20 ms packets bursting?). Then check DSCP, CPU/scheduling and receive-buffer drops (pack 03).
