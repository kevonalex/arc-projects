# 03-net / 04 — RTP/RTCP (RFC 3550) and SIP/SDP: every field, and why signalling ≠ media

**Time:** ~3–4 h · **Prereqs:** `01-headers-layering`, `02-udp-tcp-qos`, `03-sockets-poll` · **Code:** `code/include/rtp_sip.hpp` (your stub), `code/solutions/rtp_sip.hpp`, `code/tests/test_rtp_sip.cpp`
**Etherstack lens:** the LMR↔4G/5G gateway turns a vocoder frame into RTP, sets the call up with SIP/SDP (or an MCPTT profile of it), and watches RTCP to see if the radio-to-IP leg is healthy. Every field below is something you will read in a Wireshark capture during an interop session.

> **Legal:** use the synthetic packets here, your own loopback captures or published test vectors. Never receive live public-safety traffic (UK Wireless Telegraphy Act 2006).

---

## Part A — A packet first (concrete before abstract)

Here is the start of an RTP packet from the gateway. Decode it before reading Part B.

```text
81 80 1f 40  00 01 38 80  ca fe ba be  01 02 03 04  ff ff ff ff ...(160 B)
```

**Q1.** Decode: version, padding, extension, CSRC count, marker, payload type (which codec?), sequence number, timestamp (what wall-clock time is that if the clock is 8 kHz?), SSRC, CSRC list; header length in bytes; offset of the payload. *Context: the first packet of a talk burst from a mixed talkgroup stream.*

---

## Part B — RTP header, field by field (RFC 3550 §5.1)

```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+---------------+---------------+-------------------------------+
|                           timestamp                           |
+---------------------------------------------------------------+
|           synchronization source (SSRC) identifier            |
+---------------------------------------------------------------+
|            contributing source (CSRC) identifiers  (0..15)    |
+---------------------------------------------------------------+
|  [X=1] profile-defined (16) | length in 32-bit words (16)  ... |
```

The design problem RTP solves: UDP delivers packets late, out of order, duplicated, or not at all — and the *receiver* needs to know **(1) which codec, (2) what order, (3) when each sample was captured, (4) which source**. Each field answers exactly one of those.

| Field | Bits | Why it exists / design reasoning |
|-------|------|----------------------------------|
| **V** | 2 | Version = 2. Also a **demultiplexing guard**: on a shared port, STUN starts with bits `00`, DTLS 20–63, **RTP/RTCP 128–191** as the first byte, so V=2 (`10xxxxxx`) identifies RTP ⚠ VERIFY RFC 7983. |
| **P** | 1 | Padding: payload is followed by padding whose count is in the **last byte** (including itself). Needed by block ciphers (SRTP/older encryption) that need aligned lengths, and by fixed-size lower layers. Receiver must strip it (test `Rtp.CsrcExtensionAndPadding`). |
| **X** | 1 | Header extension follows the CSRC list: `profile(16) length(16, in words)` + data. Used for per-packet metadata (audio level, MID) without changing the payload format; one-byte-header extensions use profile `0xBEDE` (RFC 8285 ⚠ VERIFY). |
| **CC** | 4 | Number of CSRC entries (0–15). 4 bits because conferences rarely have > 15 *simultaneous* talkers. |
| **M** | 1 | Marker: meaning defined by the **profile**. For audio (RFC 3551): **first packet of a talkspurt** after silence suppression. Lets the receiver re-estimate playout delay *during the silence gap*, where adjusting is inaudible. For video: last packet of a frame. |
| **PT** | 7 | Payload type: which codec/format. **0–95 static** (RFC 3551: 0 = PCMU, 8 = PCMA, 9 = G.722, 18 = G.729, 13 = CN); **96–127 dynamic**, bound by SDP (`a=rtpmap`). 7 bits because the marker took one bit of the original byte. Types 72–76 are avoided so `M|PT` cannot collide with RTCP types 200–204 when RTP and RTCP share a port (RFC 5761 ⚠ VERIFY). |
| **Sequence number** | 16 | +1 per **packet** (not per byte or sample). Receiver detects **loss** (gaps), **reordering**, **duplicates**. Initial value is **random** (known-plaintext defence for encryption). 16 bits wraps: 65 536 / 50 pps = **1 310.7 s ≈ 21.8 min**, so use *extended* sequence numbers (cycles × 65 536 + seq) in all comparisons (`SeqTracker`, `seq_newer`). |
| **Timestamp** | 32 | The **sampling instant of the first octet** of the payload, in units of the payload's **clock** (8 000 Hz for G.711, 48 000 for Opus, 90 000 for video). Random initial value. Increments by the **number of sample-clock ticks** the packet covers (160 for 20 ms of G.711). Separate from seq because with **silence suppression** seq stays contiguous while time jumps (Part C). Wraps in 2³²/8000 = **536 871 s ≈ 6.2 days** at 8 kHz, **24.9 h** at 48 kHz. |
| **SSRC** | 32 | Random identifier of the **stream source**, independent of IP:port. Lets many streams share one port, survives NAT port rebinding, and is the key for all RTCP statistics. If two sources collide (probability ≈ N²/2³³ for N sources) both must detect it and one picks a new SSRC (RFC 3550 §8). |
| **CSRC list** | 32×CC | When a **mixer** (conference bridge, gateway mixing talkers) combines streams, the mixer is the new SSRC and lists the original sources here so the receiver can show *who* is talking. A **translator** (transcoder, relay) keeps the original SSRC instead. |

> **Common misconceptions**
> 1. *"RTP gives reliable, ordered delivery."* It gives **information** (seq, timestamp) so the application can reorder/conceal; RTP itself does nothing.
> 2. *"Timestamp is in milliseconds."* It is in **sample-clock ticks**; the clock rate comes from the payload type (so 20 ms = 160 or 960 or 1 800).
> 3. *"Sequence number and timestamp are redundant."* Not under DTX/silence suppression or when a codec produces variable-size frames.
> 4. *"The marker bit is part of the codec."* It's profile-defined; RTP itself says nothing about it.
> 5. *"Dynamic PT 96 always means Opus."* PT numbers are *per-session*; only SDP defines them.

**Q2.** (Timestamp arithmetic) (a) G.711 at 20 ms: ts increment? (b) Opus (RTP clock 48 kHz, always): ts increment for 20 ms? (c) G.722: samples at 16 kHz but RTP clock is **8 000**: 20 ms → ts increment? Explain the historical reason. (d) The previous G.711 packet had seq 100, ts 16 000 (covering ts 16 000–16 159). The talker stays silent for **500 ms**, then speaks. Give seq, ts and M of the next packet. *Context: Etherstack vocoder bridge with voice activity detection.*

**Q3.** Why does the receiver need the timestamp *and* the sequence number? Use the 500 ms silence example from Q2(d) to show what each reveals.

**Q4.** Compute: seq wrap time at 50 pps and at 100 pps; timestamp wrap time at 8 kHz and 48 kHz. Which one will you hit during a long-lived multicast talkgroup stream, and what code defends against it? *(Test `Rtp.SerialArithmetic`, `Rtp.SequenceTrackerWrapAndLoss`.)*

**Q5.** SSRC collision: with 32-bit random SSRCs, estimate the collision probability for (a) 50 sources in a session, (b) 1 000 sources (birthday approximation `p ≈ N²/2^33`). What do you do on detecting a collision? *Context: a gateway that bridges many talkers into one session.*

**Q6.** Marker bit: describe how a jitter buffer uses it. What goes wrong if the gateway sets M on *every* packet? On none?

**Q7.** A gateway mixes 3 talkers into one stream towards the LTE side. Which is better, mixer or translator semantics? Write the RTP header fields (SSRC, CC, CSRC). *Context: talkgroup audio with 3 simultaneous speakers (unusual in PTT but possible in conference mode).*

---

## Part C — Why the timestamp is not enough/too much: silence and DTX (concrete)

Timeline of one talker (20 ms frames, 8 kHz):
```text
t(ms)  seq  ts     M   comment
 0     100  16000  1   talkspurt starts (marker)
20     101  16160  0
40     102  16320  0   <- last packet (covers t=40..60); talker pauses, VAD says silence, no packets
...(500 ms of nothing, t=60..560)...
560    103  20480  1   new talkspurt: ts jumped by 4160 (= 160 of the last packet + 4000 of silence), seq by only 1
```
(The sequence number says "no packet was lost"; the timestamp says "500 ms of time passed". If the receiver only had seq it would play the new burst **immediately** after the last one — the pause would disappear and the rhythm of speech would be destroyed. If it only had timestamps it could not distinguish a lost packet from DTX.) *That is the one-sentence interview answer for "why both?"*

---

## Part D — RTCP: the control channel (RFC 3550 §6)

Why RTCP exists even though media is fire-and-forget: a sender never learns whether anyone receives anything. RTCP gives **feedback** (loss, jitter, RTT), **identification** (CNAME in SDES, stable across SSRC changes), **timing for synchronisation**, and **membership** (BYE). It runs on port **RTP + 1** (odd) unless multiplexed (`a=rtcp-mux`, RFC 5761).

### D.1 Sender Report (SR, PT=200) and Receiver Report (RR, PT=201)
```text
 header: V=2 P RC(5) | PT | length (words-1)      ← length of the whole packet in 32-bit words minus one
 SSRC of this report's sender
 [SR only] NTP timestamp (64: seconds since 1900 . fraction)
 [SR only] RTP timestamp   (same instant, in media clock units)
 [SR only] sender's packet count, sender's octet count
 RC × report blocks (24 B each):
   SSRC_n | fraction lost (8) | cumulative lost (24, SIGNED) | extended highest seq | jitter | LSR | DLSR
```
| Field | Why |
|-------|-----|
| **NTP + RTP timestamp pair** | Gives receivers the mapping `RTP ts ↔ wall clock`. With one pair per stream you can **synchronise audio with video** (lip-sync), or two audio streams from different sources. |
| **Packet / octet count** | Receiver can compute the sender's rate and detect rate changes. |
| **Fraction lost** | Loss in the last interval, in 1/256 units: `(expected − received) × 256 / expected`, clamped to 255 (`loss_fraction_256`). The sender adapts (lower bitrate, add FEC). |
| **Cumulative lost (signed 24 bit)** | `expected − received` since start; can be **negative** (duplicates) → sign-extend. |
| **Extended highest seq** | 16-bit seq + 16-bit cycle count → unwrapped, 32 bits (`SeqTracker::extended_max`). |
| **Interarrival jitter** | See D.2. In timestamp units. |
| **LSR / DLSR** | Last SR timestamp (middle 32 bits of NTP, 1/65 536 s units) and the delay since receiving it; used to compute **RTT without synchronised clocks**: `RTT = A − LSR − DLSR` (A = arrival time of the RR in the same format). |

### D.2 Interarrival jitter — derive it
Let packet *i* have RTP timestamp `S_i` and arrival time `R_i` (both in timestamp units). If the network had constant delay, `R_i − S_i` would be constant. The **transit time** is `T_i = R_i − S_i`; variation is `D(i−1,i) = T_i − T_{i−1} = (R_i − R_{i−1}) − (S_i − S_{i−1})`. Smooth with an exponential filter:
`J ← J + (|D| − J)/16`.
Why 1/16? RFC 3550 A.8 explains it as a good **noise-reduction vs convergence** compromise; 16 is a shift in integer code. The sender's clock offset cancels because only *differences* matter, so **no clock synchronisation** is needed. Unit: timestamp ticks (÷ clock rate × 1000 for ms).

### D.3 RTCP bandwidth rule (why it doesn't swamp the session)
RTCP is limited to **5 % of the session bandwidth** (senders get 25 % of that if they are ≤ 25 % of members), with a minimum report interval of **5 s** (halved before the first report) — `interval ≈ max(Tmin, n × avg_size / rtcp_bw)` (RFC 3550 §6.2 ⚠ VERIFY the exact constants). That is why RTCP feedback is *slow* (seconds) and not suitable for per-packet adaptation; RFC 4585 (AVPF) allows faster feedback for NACK/PLI.

### D.4 Other RTCP packets
SDES (CNAME, NAME…), **BYE** (leaving; also changes SSRC when collision), APP (application-defined — 3GPP MCPTT floor control uses an RTCP-based MBCP profile ⚠ VERIFY TS 24.380).

> **Common misconceptions**
> 1. *"RTCP carries the media's control (play/pause)."* That is RTSP. RTCP is **monitoring + sync + identity**.
> 2. *"RTT needs NTP-synchronised clocks."* LSR/DLSR make it a *round-trip*, so each side's clock offset cancels.
> 3. *"RTCP jitter is milliseconds."* It's in **timestamp units**.
> 4. *"Loss fraction is cumulative."* It's per interval; cumulative lost is separate.

**Q8.** Layout: how many bytes is an SR with one report block? What is the `length` field? An RR with two blocks? *(Test `Rtcp.SenderReportLayout`.)*

**Q9.** Jitter exercise (8 kHz): timestamps `0, 160, 320, 480, 640, 800`; arrival times in ms `0, 21, 38, 65, 79, 103`. Convert to timestamp units, compute transit, D and J after each packet, and give the final jitter in ms. *Context: loaded backhaul. (Test `Rtp.JitterEstimator` checks the first steps.)*

**Q10.** A receiver's first packet had seq 1000; the highest so far is extended seq 1099; it received 95 packets in total. Compute expected, cumulative lost, and (if the previous report interval had expected_prior 50, received_prior 49) the fraction lost for the last interval in 1/256 units and as a percentage.

**Q11.** RR arrives at your sender at `A = 0x00039EB8` (mid-32 NTP). The block says `LSR = 0x00030000`, `DLSR = 0x00008000`. Compute the RTT in ms. What if `LSR = 0`?

**Q12.** Session: G.711 (IP rate 80 kbit/s). Compute the RTCP budget, and the report interval for a two-party call with 120-byte compound RTCP packets. What does Wireshark show in practice (spacing)? ⚠ VERIFY against RFC 3550 §6.3.

**Q13.** Which UDP port does RTCP use if RTP is on 49170? What changes with rtcp-mux, and why does that matter for firewalls and NAT? *Context: opening pinholes on a carrier firewall.*

---

## Part E — Why signalling ≠ media (derive the architecture)

Two different jobs:
| | Signalling (SIP) | Media (RTP) |
|---|------------------|-------------|
| Purpose | find the peer, agree codec/ports, ring, answer, hang up, transfer | carry samples |
| Traffic | a few messages per call, KB | 50 packets/s for the whole call, 80 kbit/s |
| Reliability | **must** arrive (retransmit; TCP/TLS or UDP with timers) | late = lost; no retransmit |
| Path | via **proxies/servers** that route, authenticate, bill, fork | **direct** between endpoints (or via media gateway/SBC) |
| Format | text, extensible (headers) | compact binary |
| Security | TLS, digest auth | SRTP (pack 05) |
| Failure mode | call fails to set up | audio glitches |

Why separate? (1) *Scale:* servers see only signalling (kbit/s), not every call's megabits. (2) *Different QoS needs* (reliable vs low-latency). (3) *Independent failure:* a lost BYE should not stop media instantly; media loss should not tear down a call without a policy (RTCP/ICE/timers). (4) *Flexibility:* media can move (hold, transfer, codec change via re-INVITE) without changing the signalling relationship. (5) *Legacy:* the PSTN split SS7 from voice trunks the same way — SIP's SDP is the IP analogue of a trunk seizure.
Cost: more moving parts, **NAT traversal** pain (addresses/ports inside SDP), two things to secure and two to debug.

---

## Part F — SIP call flow (RFC 3261)

Basic INVITE dialog, UDP, no proxies:

```text
UAC (gateway)                              UAS (MCPTT server / SIP phone)
   | ---- INVITE (SDP offer) -------------> |
   | <--- 100 Trying -----------------------|   provisional, hop-by-hop
   | <--- 180 Ringing ----------------------|   provisional
   | <--- 200 OK (SDP answer) --------------|   final success
   | ---- ACK ----------------------------> |   3rd message of the INVITE "handshake"
   |===== RTP A→B  and  B→A ================|   media starts (may begin early on 183)
   | ---- BYE ----------------------------> |
   | <--- 200 OK ---------------------------|
```

**Why an ACK after 200 OK?** An INVITE may take a long time (the callee must answer), so the UAS keeps **retransmitting the 200 OK** until it hears the ACK — otherwise it can't know the caller received the answer and that the media path is ready. It is the same logic as the third TCP handshake message (pack 02, Part C). The ACK for a 200 is a *new transaction*; ACK for a non-2xx is part of the INVITE transaction.

**Identification** (what Wireshark flows are keyed on):
* **Dialog** = `Call-ID` + `From` tag + `To` tag (the To tag is added by the UAS in its first response).
* **Transaction** = top `Via` **branch** (starts with the magic cookie `z9hG4bK`) + method; **CSeq** orders requests within a dialog.
* **Via** records the path so responses retrace it; **Contact** says where to send future in-dialog requests directly; **Record-Route** forces proxies to stay in the path.

Status classes: **1xx** provisional, **2xx** success, **3xx** redirect, **4xx** client error (486 Busy Here, 488 Not Acceptable Here), **5xx** server error, **6xx** global failure.
Timers over UDP: **T1 = 500 ms** RTT estimate, retransmit intervals double (T2 = 4 s cap), transaction timeout 64 × T1 = **32 s** ⚠ VERIFY RFC 3261 §17 / Table 4.
SIP over UDP vs TCP: use TCP/TLS when messages approach the MTU (pack 02, Q13).

**Message anatomy:** start line, headers (`Name: value`, case-insensitive names, **compact forms** `v f t i m l c`), blank line (`CRLF CRLF`), optional body (here SDP) whose length is `Content-Length`. Tests: `Sip.*`.

> **Common misconceptions**
> 1. *"SIP carries the audio."* SIP carries *messages*; the audio is RTP on negotiated ports.
> 2. *"The 200 OK ends the call setup."* Not until the **ACK** arrives.
> 3. *"The `Call-ID` alone identifies the dialog."* Forking can create several dialogs with one Call-ID (different To tags).
> 4. *"BYE is lost = call still on."* Without session timers/RTCP-timeout policy, the media can run on after the signalling state is gone — design for it.

**Q14.** Give five reasons signalling and media are separate and one concrete failure scenario where the separation causes a bug (hint: BYE lost, or SDP contains a private IP behind NAT).

**Q15.** Draw the INVITE dialog for a PTT call setup and mark where each of: SDP offer, SDP answer, first RTP packet, Via/Call-ID/tags, ACK retransmission logic appear. What exactly is retransmitted if the 200 OK is lost? If the INVITE is lost?

**Q16.** Write the minimal valid SIP 180 response to the INVITE in the tests (`build_sip_response`): which headers must be copied and which must be added? When must you **not** add a To tag?

**Q17.** Explain the three identifiers (dialog, transaction, branch) and what breaks if a proxy forgets to add its own Via.

---

## Part G — SDP and offer/answer (RFC 4566 / RFC 3264)

SDP is a **description format**, not a protocol: lines `type=value`, types in a fixed order:
```text
v=0                                              version (always 0)
o=alice 2890844526 2890844526 IN IP4 10.0.0.1    origin: user, session id, session VERSION (bump on re-offer), net, addr
s=-                                              session name (unused)
c=IN IP4 10.0.0.1                                where to send media (session or media level; media level wins)
t=0 0                                            time (0 0 = unbounded)
m=audio 49170 RTP/AVP 96 0 8 101                 media: type, PORT, transport, payload-type LIST in PREFERENCE order
a=rtpmap:96 opus/48000/2                         bind dynamic PT to encoding/clock/channels (static PTs need no rtpmap)
a=rtpmap:101 telephone-event/8000
a=ptime:20                                       packetisation time (ms)
a=sendrecv                                       direction: sendrecv | sendonly | recvonly | inactive
```
**Offer/answer model:** one party sends an *offer* listing what it can do **in preference order**; the other replies with an *answer* that (1) has the **same number of `m=` lines** in the same order; (2) lists a **subset** of the offered formats it supports (usually its choice first); (3) gives its **own** IP and port; (4) may set port 0 to reject a stream; (5) must respond in the offered direction semantics (`sendonly` offer → `recvonly` answer — hold). After the exchange both sides send/receive only formats in both lists. Code: `parse_sdp`, `negotiate_codec`, `build_sdp_answer` (match **name and clock**, case-insensitive: `opus/16000` ≠ `opus/48000`).

Why the **RTP clock** matters in the match: the timestamp unit is part of the format; two sides with different clocks for "the same codec" would compute different playout times (famous G.722 clock = 8000 erratum).
Where SDP rides: SIP body (`Content-Type: application/sdp`); can be in INVITE+200 (offer/answer), or 200+ACK (late offer). Early media: an answer in 183 Session Progress.
Re-INVITE: same dialog, new SDP with `o=` version +1 → hold (`a=sendonly`/`inactive`), codec change, address change.

> **Common misconceptions**
> 1. *"The answer picks anything it likes."* It must stay within the offered formats.
> 2. *"SDP negotiates the transport network."* It states *addresses/ports as seen by that endpoint* — behind NAT those are wrong without STUN/ICE/ALG.
> 3. *"`m=` port is the local bind port."* It is the port the *receiver* of this SDP should send to.
> 4. *"ptime is binding."* It's a preference; the stream's real packet duration is observable only in the RTP timestamps.

**Q18.** Offer = `m=audio 4000 RTP/AVP 96 0 8` (`96 = opus/48000/2`). Your gateway supports **PCMA and G.729**. Write the answer's `m=` and `a=` lines. What if it supports only G.729 and the offer has no 18? What SIP response (or SDP) do you send? *(Test `Sdp.NegotiationFollowsOfferersOrderAndMatchesClock`.)*

**Q19.** Write the offer for a gateway that wants **PCMU, 20 ms, plus DTMF (RFC 4733 `telephone-event`)**, then the hold re-INVITE and the answer to it.

**Q20.** The offer lists `a=rtpmap:97 AMR-WB/16000` and your gateway supports AMR-WB at **16000** and Opus. What does RTP's timestamp increment per 20 ms look like? (AMR-WB: 320 samples / 20 ms at 16 kHz, RTP clock 16 000.)

---

## Part H — Review and integration

**Q21.** Bugs in this packetiser (find ≥ 7):

```cpp
struct Rtp { uint8_t v:2, p:1, x:1, cc:4, m:1, pt:7; uint16_t seq; uint32_t ts, ssrc; };
void send_frame(int fd, const uint8_t* pcm, size_t n) {
    Rtp r{}; r.v = 2; r.pt = 0; r.seq += n; r.ts = now_ms(); r.ssrc = 1; r.m = 0;
    char buf[2000]; memcpy(buf, &r, sizeof r); memcpy(buf + 12, pcm, n);
    send(fd, buf, 12 + n, 0);
}
```
*Context: a first-draft RTP sender on the gateway.*

**Q22.** (MCPTT/PTT mapping) When a user presses PTT on a gateway-connected radio, which actions go over SIP (INVITE/re-INVITE/BYE), which over RTP, and which over RTCP/floor-control (MBCP)? ⚠ VERIFY the 3GPP TS 24.379/24.380 details — say what you are sure of and flag the rest.

**Q23.** (Oral, 3 min) Explain RTP, RTCP, SIP and SDP to a colleague, using one call from PTT press to release, and name the *three* numbers you check first in Wireshark when voice is choppy (loss, jitter, sequence errors → *Telephony → RTP → Stream Analysis*).

---

## Part I — Build tasks (`code/`)

| Step | Functions in `rtp_sip.hpp` | Tests |
|------|----------------------------|-------|
| 1 | `parse_rtp`, `build_rtp` | `Rtp.ParseMinimal`, `Rtp.MarkerAnd…`, `Rtp.CsrcExtensionAndPadding`, `Rtp.RejectsMalformed` |
| 2 | `seq_newer`, `ts_diff`, `SeqTracker` | `Rtp.SerialArithmetic`, `Rtp.SequenceTracker*` |
| 3 | `JitterEstimator`, `loss_fraction_256` | `Rtp.Jitter*`, `Rtp.LossFraction` |
| 4 | `build_rtcp`, `parse_rtcp`, `ntp_middle32`, `rtt_ms` | `Rtcp.*` |
| 5 | `parse_sdp`, `negotiate_codec`, `build_sdp_answer` | `Sdp.*` |
| 6 | `parse_sip`, `build_sip_response`, `call_next` | `Sip.*` |

```bash
cmake -S code -B build && cmake --build build
./build/rtpsip_tests_student     # red until you finish code/include/rtp_sip.hpp
./build/rtpsip_tests_solution    # reference
```
Parsing rules (pack 01 Part I) apply: every length (CC, extension words, padding, RTCP length, `Content-Length`) is **attacker-controlled** — check it against the buffer before using it.

**Stretch:** fuzz `parse_rtp`/`parse_rtcp`/`parse_sip` under ASan/UBSan; send your RTP over loopback (pack 03 sockets) and open the capture in Wireshark → *RTP Streams → Analyze* and compare its jitter to `JitterEstimator`.

---

## ⚠ VERIFY list (this pack)
1. RTP/STUN/DTLS demux byte ranges (RFC 7983).
2. RTP header-extension profile `0xBEDE` (RFC 8285).
3. Reserved PT range 72–76 and rtcp-mux rationale (RFC 3550 §12, RFC 5761).
4. RTCP bandwidth fractions (5 %, 25 %) and 5 s minimum interval (RFC 3550 §6.2–6.3).
5. SIP timers T1 = 500 ms, T2 = 4 s, Timer B/F = 64·T1 (RFC 3261 Table 4).
6. 3GPP MCPTT floor control mapping on RTCP/MBCP (TS 24.380) and SIP usage (TS 24.379).
7. G.722 RTP clock 8 000 (RFC 3551 erratum), AMR-WB clock 16 000 (RFC 4867).
8. SSRC collision approximation `N²/2³³`.
