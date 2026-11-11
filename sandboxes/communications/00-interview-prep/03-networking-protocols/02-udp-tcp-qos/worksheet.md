# 03-net / 02 — UDP vs TCP for Voice, Windows, Nagle, MTU, Multicast, DSCP

**Time:** ~3 h · **Prereqs:** `01-headers-layering` · **Code:** `code/` (`net_qos.hpp`)
**Etherstack lens:** push-to-talk audio between an LMR site and an MCPTT/4G core crosses links with loss and jitter. Choosing UDP for media, TCP for signalling, multicast for talkgroup fan-out, and EF marking for priority is the first design conversation in the interview.

> **Legal:** all traffic here is synthetic or on your own loopback/lab network. Never receive live public-safety traffic (UK Wireless Telegraphy Act 2006).

---

## Part A — A timeline before the theory (concrete first)

A gateway sends one 20 ms voice frame every 20 ms (frame *i* leaves at *t = 20i* ms). One-way network delay is 30 ms. **Frame 3 is lost** the first time it is sent. The receiver plays audio with a **60 ms playout deadline measured from send time** (a frame arriving > 60 ms after it was sent is useless).

**UDP:** nothing is retransmitted. **TCP:** the sender notices after a retransmission timeout of **RTO = 200 ms** (generous; real stacks are ≥ 200 ms, RFC 6298 recommends 1 s ⚠ VERIFY) and resends; the retransmission arrives 30 ms later. TCP delivers bytes to the application **in order only**.

Fill in the table (all times ms since frame 0 was sent). *Do this before reading on.*

| Frame | Sent | Arrives (network) | UDP: app gets it at | TCP: app gets it at | TCP delay | Playable at 60 ms? (UDP / TCP) |
|-------|------|-------------------|---------------------|---------------------|-----------|--------------------------------|
| 2 | 40 | | | | | |
| 3 (lost) | 60 | | — (lost) | | | |
| 4 | 80 | | | | | |
| 5 | 100 | | | | | |
| 9 | 180 | | | | | |

**Q1.** Complete the table. How many of the 10 frames (0–9) are unusable under UDP? Under TCP? *Context: a 1 % loss on a backhaul link — what does one loss do to a talk burst?*

**Q2.** With **fast retransmit** (3 duplicate ACKs, ~RTT 60 ms) the loss is detected after the 3rd dup-ACK instead of an RTO. At what time does frame 3 reach the application? What is the extra delay for frame 4? Is it playable at 60 ms? *Context: why even "good" TCP is too slow for a 150 ms mouth-to-ear budget.*

The code `simulate_udp` / `simulate_tcp` / `unusable_count` reproduces this table (tests `Hol.*`).

---

## Part B — UDP vs TCP for voice (derive the choice)

### B.1 The deadline argument
Human conversation tolerates ≈ **150 ms one-way** mouth-to-ear delay before it feels broken (ITU-T G.114 ⚠ VERIFY: 150 ms recommended, up to 400 ms tolerable in planning). Of that budget the codec framing (20 ms), packetisation, jitter buffer (typically 40–60 ms), and the network share the rest. A frame that arrives later than the playout point is **worthless**: speech is real-time, so *late = lost*.

### B.2 What TCP guarantees, and what that costs voice
TCP guarantees **a complete, ordered byte stream**. To do so it must:
1. **Retransmit** lost data → at least one extra RTT (plus detection time).
2. **Deliver in order** → **head-of-line (HOL) blocking**: later data already in the kernel buffer cannot be read until the hole is filled (Part A: frames 4–9 all land at 290 ms).
3. **Back off** under loss (congestion control halves the window) → rate drops precisely when the radio link is already weak.
4. Hide packet boundaries (byte stream) → the app must re-frame 160-byte voice frames.

For file transfer all of that is *what you want*. For speech it is the opposite: a 20 ms hole is **cheaply concealed** (PLC, `04-audio/03`), while a 200 ms stall is a dropout *plus* a burst of stale frames.

### B.3 What UDP gives instead
No ordering, no retransmit, no back-off: **the app owns the policy**. RTP adds sequence numbers/timestamps so the receiver reorders and conceals; RTCP reports loss so the sender can adapt (`04-rtp-sip`).

### B.4 When TCP/TLS carriage of voice is acceptable
Restrictive firewalls (UDP blocked) — accept the HOL penalty as a last resort. Never as the default for PTT media. Signalling (SIP, gRPC `StartCall`) is the opposite case: small, must arrive, tolerates 1 RTT.

> **Common misconceptions**
> 1. *"UDP is faster."* Not per packet — both ride IP. UDP is *more predictable*: no stall behind a lost packet.
> 2. *"TCP loses nothing, so audio is better."* Nothing is lost, everything is late — which for speech is the same as lost.
> 3. *"Retransmitting voice always hurts."* Selective retransmit **within the jitter-buffer window** (RTCP NACK, RFC 4585 ⚠ VERIFY) or FEC can help; TCP's blind in-order policy is the problem.

**Q3.** List three concrete reasons voice media uses UDP and three reasons SIP/gRPC signalling use TCP. *Context: gateway design review.*

**Q4.** Your jitter buffer holds 60 ms. A path has p = 2 % random loss and RTT = 60 ms. Compute the expected fraction of frames a *UDP+RTP* receiver must conceal and compare with the fraction of frames a *TCP* receiver sees delayed beyond the playout point if each loss causes a 200 ms stall during which ~10 frames pile up. *(Order-of-magnitude reasoning; the point is "1 loss ≈ 1 frame vs ≈ 10 frames".)*

---

## Part C — Why the TCP handshake is three-way (derive it)

Goal: both sides must (1) agree to talk, and (2) each choose an **initial sequence number (ISN)** and know the **other has received it**. That is 2 independent "announce + confirm" steps = 4 messages; the middle two merge (SYN+ACK) → **3 messages**.

```text
client                 server
  | --- SYN, seq=x ----> |      "I will number my bytes from x"
  | <-- SYN+ACK, seq=y,  |
  |         ack=x+1 ---- |      "I will number from y; I got your x"
  | --- ACK, ack=y+1 --> |      "I got your y"          -> both ESTABLISHED
```

**Why not two messages?** Suppose a SYN from an *old, dead* connection wanders the network and arrives late (**delayed duplicate**). With a 2-way handshake the server would open a connection for a client that no longer wants one, and send data. With 3-way, the server's SYN+ACK reaches the real client, who sees an ack for a sequence it never sent and replies **RST**; the half-open connection dies. The third message is the client's *proof of liveness*. (This is the original argument, Tomlinson 1975 ⚠ VERIFY attribution.)

**Why random ISN?** Segments from a previous incarnation of the same 4-tuple must not be accepted as new data; and predictable ISNs allow blind spoofing.

**Cost:** 1 RTT before the first data byte. A 200 ms RTT backhaul pays 200 ms before the first SIP INVITE can be processed (TLS adds more) — one reason call setup latency matters. Mitigations: TCP Fast Open, connection reuse (gRPC keeps channels open), QUIC (0-RTT).

**Teardown:** FIN in each direction (a half-close is legal). The side that sent the last ACK sits in **TIME_WAIT for 2×MSL** (MSL = 60 s in the RFC → 120 s; Linux uses 60 s total ⚠ VERIFY) so that (a) a lost final ACK can be re-sent and (b) old duplicates expire before the 4-tuple is reused.

**SYN flood:** the server allocates state at SYN; attackers send SYNs and never ACK. **SYN cookies** encode state in the ISN `y` so nothing is stored until the final ACK. (SCTP's cookie design in Part 01-H is the same idea.)

> **Common misconceptions**
> 1. *"The third packet is redundant."* It defeats delayed duplicates and proves the client received `y`.
> 2. *"A closed socket frees the port immediately."* TIME_WAIT keeps the 4-tuple busy.
> 3. *"TCP is connection-oriented so the network keeps state."* Only the two endpoints do; routers see independent packets.

**Q5.** Draw the sequence-number exchange for ISN(client)=5000, ISN(server)=9000, listing seq/ack/flags in each of the three packets. What is the first data byte's sequence number from each side?

**Q6.** A gateway opens a new TCP connection per call-control request to one core node (same dst IP:port). Ephemeral range 32768–60999 (28 232 ports) and TIME_WAIT = 60 s. What is the sustained connection rate at which it runs out of source ports? What is the architectural fix? ⚠ VERIFY the Linux numbers.

**Q7.** Trace the state machine (code `tcp_next_state`) for a **simultaneous open** where both ends send SYN at once. How many packets are exchanged?

---

## Part D — Windows and congestion (derive the formulas)

### D.1 Two different limits
* **Flow control:** protect the *receiver's buffer*: the advertised window `rwnd` (Part 01-G).
* **Congestion control:** protect the *network*: the sender's own `cwnd`. Bytes in flight ≤ `min(rwnd, cwnd)`.

### D.2 Throughput ceiling
A sender may have at most `W` bytes unacknowledged. After sending a window it must wait one RTT for the first ACK, so **throughput ≤ W / RTT**. To keep the pipe full you need `W ≥ rate × RTT` = **bandwidth–delay product (BDP)**.
Example: 20 Mbit/s × 200 ms = 4 Mbit = **500 000 B** — more than 65 535, hence **window scaling**.

### D.3 Slow start and AIMD
* **Slow start:** start at IW (initial window, 10 segments in modern stacks, RFC 6928 ⚠ VERIFY); each ACK adds 1 MSS → `cwnd` **doubles every RTT** (exponential) until loss or `ssthresh`.
* **Congestion avoidance (AIMD):** +1 MSS per RTT (additive), halve on loss (multiplicative). Why *multiplicative decrease*? Flows that halve when congested converge to a fair share (Chiu & Jain analysis); additive-decrease does not.
* The window traces a **sawtooth** between W/2 and W. Average ≈ ¾ W.
* **Mathis formula (steady state, random loss p):** `throughput ≈ (MSS / RTT) × 1.22 / √p`. Derivation sketch: the sawtooth between W/2 and W loses one packet per cycle; packets per cycle ≈ (3/8)W²; so `p = 1 / ((3/8)W²)` → `W = √(8/(3p))`; throughput = ¾W·MSS/RTT = `√(3/2)·MSS/(RTT·√p)`, √(3/2) ≈ 1.2247 ⚠ VERIFY the constant for your loss model (it depends on delayed ACKs).
* TCP can be **bad on radio links** because it reads *any* loss as congestion. That is why cellular/satellite backhaul uses PEPs, large buffers, or BBR-style algorithms.

### D.4 RTO (RFC 6298)
Estimator: first sample `R`: `SRTT = R`, `RTTVAR = R/2`. Then `RTTVAR = ¾·RTTVAR + ¼·|SRTT − R|`, `SRTT = ⅞·SRTT + ⅛·R`, `RTO = SRTT + 4·RTTVAR` (min 1 s per RFC, 200 ms common). Why `4·RTTVAR`? Cover jitter at roughly 4 mean deviations so spurious retransmits are rare.

> **Common misconceptions**
> 1. *"Bigger window = faster."* Only until BDP; beyond that it adds queueing delay (bufferbloat).
> 2. *"Packet loss means link failure."* On Internet-style paths loss is the *congestion signal*; TCP backs off even if the loss was radio noise.
> 3. *"Voice over UDP can ignore congestion."* RFC 8085 expects UDP applications to adapt (RTCP feedback, codec rate switching).

**Q8.** Backhaul: 5 Mbit/s, RTT 600 ms (satellite). (a) BDP in bytes? (b) Which window-scale shift is needed? (c) With IW = 10 MSS (MSS 1460), how many RTTs of slow start (and how long) to reach one BDP? *Context: a remote LMR site with a satellite link back to the core.*

**Q9.** MSS 1400, RTT 60 ms, p = 0.5 % random loss on an LTE backhaul. Estimate TCP throughput with the Mathis formula. Compare with a 4 Mbit/s requirement for a file sync from the gateway. *(Check `Congestion.MathisFormula`.)*

**Q10.** RTT samples 100, 120, 90 ms. Compute SRTT, RTTVAR, RTO after each (min RTO = 0 so you see the raw value). *(Test `Rto.Rfc6298`.)*

---

## Part E — Nagle's algorithm and delayed ACK

**Motivation (concrete):** a user types one character at a time over telnet. Each keystroke → 1 byte of data + 40 bytes of TCP/IP header = **4000 % overhead** ("tinygram" problem). Nagle's rule (RFC 896):
> *If there is unacknowledged data in flight, hold small writes and coalesce them until the ACK arrives (or a full MSS has accumulated). Otherwise send immediately.*
Code: `nagle_should_send(pending, unacked, mss)`.

**Delayed ACK:** receivers wait up to ~40–200 ms hoping to piggyback the ACK on reply data. If the sender is waiting for that ACK (Nagle) and the receiver is waiting for more data (delayed ACK) you get the classic **write-write-read stall**: tens to hundreds of ms of added latency per message.

Run `nagle_simulate(10, 160, 20, 100, nagle_on)` (tests `Nagle.*`): with Nagle **off** you send 10 segments with 0 ms added delay; **on**, 3 segments and up to 100 ms of added latency to a voice/PTT frame — i.e. a full RTT of buffering.

**Fix:** `setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, 1)` on all latency-sensitive TCP (SIP, gRPC control, floor control over TCP). Prefer writing a message with a single `send` (or `writev`) rather than header then body.

> **Common misconceptions**
> 1. *"Nagle is always bad."* It's good for bulk/interactive-text; it is wrong for request/response RPC.
> 2. *"NODELAY removes ACK delay."* It disables *sender* coalescing only. Receiver delayed-ACK is a separate knob (`TCP_QUICKACK` on Linux).
> 3. *"Nagle affects UDP."* No: UDP has no such mechanism.

**Q11.** A gRPC unary `EndCall` is written as: 5-byte gRPC prefix, then 40-byte body (two `send` calls). RTT 80 ms, receiver delayed-ACK 40 ms. Explain the possible stall and give two fixes. *Context: call teardown latency on a satellite link.*

**Q12.** For 10 PTT voice frames of 160 B every 20 ms over TCP with Nagle ON at RTT 100 ms, state how many segments and the worst added latency (use the test as the oracle and explain each number by hand).

---

## Part F — MTU and fragmentation (derive)

**MTU** = largest L3 packet a link carries. Ethernet 1500. A link with smaller MTU forces:
* **IPv4 fragmentation** (if DF=0): router splits the IP payload into pieces whose data size is a multiple of 8 (offset field is in 8-byte units), copying the IP header (ID same, MF=1 except last).
* or, if DF=1, drops the packet and sends **ICMP "Fragmentation needed"** — the basis of **Path MTU Discovery (PMTUD)**. IPv6 *only* does the latter.

Example: 3 000-byte IP payload over MTU 1500 → data per fragment = ⌊(1500 − 20)/8⌋ × 8 = **1480** → fragments (1480, off 0, MF), (1480, off 185, MF), (40, off 370) — total wire = 3000 + 3×20 = **3060 B** (code: `fragment_ipv4`).

**Why avoid fragmentation, especially for voice:**
1. Lose **any** fragment → whole datagram lost (loss rate amplified ≈ k×).
2. Receiver must buffer fragments (memory, reassembly timeouts, DoS surface).
3. Only fragment 0 has the UDP/TCP header → stateless firewalls/NAT/ECMP mishandle the rest.
4. Reassembly adds latency → jitter.
5. PMTUD black holes: ICMP filtered → connections hang on big packets.

**Design rule:** size voice packets so `IP + UDP + RTP + payload ≤ path MTU` (budget tunnel headers too: GTP-U adds outer IP 20 + UDP 8 + GTP 8 = 36 B (IPv4) → inner MTU 1464 on a 1500 path ⚠ VERIFY). For TCP set **MSS = MTU − 40** (v4) / −60 (v6) and clamp MSS on tunnels.

> **Common misconceptions**
> 1. *"Fragmentation just works."* It works until a fragment is lost or filtered.
> 2. *"MTU 1500 means 1500 bytes of UDP payload."* 1472 (v4): minus 20 IP + 8 UDP.
> 3. *"IPv6 routers fragment too."* Never; the source fragments, or PMTUD tells it to send smaller.

**Q13.** (a) SIP INVITE with SDP of 1 800 bytes over UDP on a 1 500-MTU path: list fragments. (b) RFC 3261 says to switch to TCP when a request is within 200 B of the MTU or > 1300 B if MTU is unknown ⚠ VERIFY — why that rule? *Context: SIP between gateway and carrier.*

**Q14.** 48 kHz stereo 16-bit PCM for 20 ms is 3 840 B (see `04-audio/01`). With RTP (12) + UDP (8) it is 3 860 IP payload B on a 1500-MTU link. Give the fragment list and the effective loss multiplier if each fragment is lost independently at 1 %. *Context: why uncompressed wideband audio over IP is a mistake.*

**Q15.** Compute the maximum RTP payload for IPv6 over a GTP-U tunnel with outer IPv6 on a 1500-MTU path (outer IPv6 40 + UDP 8 + GTP-U 8 = 56; inner: IPv6 40 + UDP 8 + RTP 12). ⚠ VERIFY GTP-U header size (8 mandatory bytes).

---

## Part G — Multicast and IGMP

**Why multicast (concrete):** a talkgroup stream at 80 kbit/s IP rate must reach **40 sites**. Unicast: the source link carries 40 × 80 kbit/s = **3.2 Mbit/s** and the source sends 40 copies. Multicast: **80 kbit/s** and one copy; the network replicates where paths diverge. That is why LMR talkgroup distribution maps naturally onto multicast groups.

**Addressing:** IPv4 multicast = **224.0.0.0/4** (first nibble `1110`). Useful ranges: 224.0.0.0/24 link-local control (not forwarded; e.g. 224.0.0.1 all-hosts, 224.0.0.22 IGMPv3), 232.0.0.0/8 source-specific, **239.0.0.0/8 administratively scoped** (private use, like RFC 1918) ⚠ VERIFY.

**L2 mapping (derive the ambiguity):** Ethernet multicast MAC prefix `01:00:5e` leaves **23 free bits**. A group has **28 significant bits** (32 − 4). Therefore **2^(28−23) = 32 groups share one MAC** (code: `ipv4_mcast_mac`, `mcast_groups_per_mac`). Hosts must filter again at IP. Example: 239.1.1.1 and 239.129.1.1 map to the same MAC.

**IGMP (IPv4, RFC 2236/3376):** how routers/switches learn who listens.
1. Host joins group G → sends **Membership Report** (type `0x16` in v2) to G.
2. Router (querier) periodically sends **General Query** to 224.0.0.1; hosts reply (randomised delay to suppress duplicates).
3. Host leaves → **Leave** (type `0x17`) to 224.0.0.2; router sends a group-specific query before pruning (v2 leave latency ~ seconds).
4. **IGMP snooping:** switches listen to IGMP to forward multicast only to ports with members; without it multicast floods like broadcast.
5. Cross-router forwarding uses PIM (sparse mode); **SSM** (IGMPv3) lets receivers pick the source.
Message format (8 bytes): `type | max-resp (1/10 s) | checksum | group`.

IPv6 uses **MLD** (ICMPv6) instead of IGMP; `ff02::1` all-nodes. Cellular networks use **eMBMS/5G MBS** for broadcast, not IP multicast (`06-cellular`).

> **Common misconceptions**
> 1. *"Multicast goes across the internet."* Mostly not; ISPs don't route it. It's an intra-domain tool (the gateway fabric).
> 2. *"One group = one MAC."* 32:1.
> 3. *"Joining a group = receiving the stream."* The network also needs a path (PIM/snooping) and your socket must bind to the port and join on the right interface.
> 4. *"Multicast is reliable."* It's UDP: loss and reorder like any other, no retransmit.

**Q16.** Compute the 8-byte IGMPv2 membership report for 239.1.1.1 including checksum (type 0x16, max-resp 0). Verify the receiver sum is 0. *(Test `Multicast.Igmpv2Report`.)*

**Q17.** Do 239.1.1.1 and 239.129.1.1 collide at L2? List which bits are dropped. Design a talkgroup→group allocation that never collides inside one deployment. *Context: the capstone gateway maps talkgroups to groups.*

**Q18.** 40 sites, 300 active talkgroups, each 80 kbit/s IP, average 5 receiving sites per group. Compare aggregate core-link load for unicast replication vs multicast in the *worst case where all are active*. State your assumption.

---

## Part H — DSCP and EF (QoS marking)

The IPv4 TOS byte / IPv6 Traffic Class = `DSCP (6 bits) | ECN (2 bits)`. DSCP selects a **per-hop behaviour (PHB)** — a queueing/dropping treatment at each router, not an end-to-end guarantee.

| PHB | DSCP | Binary | Use |
|-----|------|--------|-----|
| BE | 0 | 000000 | default |
| CSn | 8n | nnn000 | legacy IP-precedence compatible |
| AFxy | 8x+2y | xxx yy 0 | 4 classes × 3 drop precedences (AF41 = 34) |
| **EF** | **46** | **101110** | low loss/latency/jitter: voice media |
| (VOICE-ADMIT) | 44 | 101100 | admitted voice, RFC 5865 ⚠ VERIFY |

TOS byte for EF = `101110 00` = **0xB8**. 802.1Q PCP 5 conventionally carries voice at L2 ⚠ VERIFY mapping per vendor.

**Why EF is built the way it is:** voice wants a *strict-priority queue* with a **policer** (so it cannot starve others) — low queueing delay because the queue is almost always empty; the policer enforces `rate ≤ allocation`.

**Reality checks:** DSCP is just a label — operators may **re-mark or bleach** it at trust boundaries; QoS only helps at *congested* links; ECN bits share the byte, so never overwrite the full TOS with `0xB8` blindly if ECN is in use. Setting it: `setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof tos)`.

> **Common misconceptions**
> 1. *"EF gives me guaranteed bandwidth end to end."* It's a per-hop treatment; the carrier must honour it.
> 2. *"DSCP 46 = 0x2E on the wire."* The wire byte is 0xB8 (shifted left by 2).
> 3. *"Marking everything EF makes it faster."* It breaks the priority queue and gets policed to nothing.

**Q19.** Compute TOS bytes for EF, AF41, CS5, CS6 and BE (ECN = 0), and the byte with ECN = CE (3) for EF. *(Test `Dscp.TosAndNames`.)*

**Q20.** Your gateway marks RTP with DSCP 46 but Wireshark at the far end shows DSCP 0. Give three possible causes and how you'd locate each. *Context: carrier handover.*

---

## Part I — Synthesis questions

**Q21.** MCPTT floor-control (3GPP TS 24.380) runs over UDP with its own retransmission timers instead of TCP ⚠ VERIFY. Give the engineering argument. *Context: a floor-grant message must arrive in tens of ms; RTP media is on a separate port.*

**Q22.** (Oral, 2 min) "Why does RTP run over UDP, and why not over QUIC?" Cover: HOL, retransmit semantics, handshake, congestion control, middleboxes.

**Q23.** Design: a gateway needs (a) PTT media, (b) call control RPC, (c) bulk logging to a server, (d) a heartbeat. Pick transport, TOS, and socket options for each; justify.

---

## Part J — Build tasks (`code/`)

| Step | Functions in `net_qos.hpp` | Tests |
|------|----------------------------|-------|
| 1 | `simulate_udp`, `simulate_tcp`, `worst_delay_ms`, `unusable_count` | `Hol.*` |
| 2 | `tcp_next_state` | `Handshake.*` |
| 3 | `bdp_bytes`, `max_tcp_throughput_bps`, `window_scale_for` | `Window.*` |
| 4 | `slow_start_cwnd`, `reno_ssthresh_after_loss`, `mathis_throughput_bps`, `RtoEstimator` | `Congestion.*`, `Rto.*` |
| 5 | `nagle_should_send`, `nagle_simulate` | `Nagle.*` |
| 6 | `fragment_ipv4`, `max_udp_payload`, `total_wire_bytes_with_fragments` | `Frag.*` |
| 7 | `is_ipv4_multicast`, `ipv4_mcast_mac`, `mcast_groups_per_mac`, `checksum16`, `build_igmpv2` | `Multicast.*` |
| 8 | `dscp_to_tos`, `tos_to_dscp`, `dscp_name` | `Dscp.*` |

```bash
cmake -S code -B build && cmake --build build
./build/qos_tests_student     # edit code/include/net_qos.hpp until green
./build/qos_tests_solution    # reference in code/solutions/
```

---

## ⚠ VERIFY list (this pack)
1. G.114 numbers (150 ms / 400 ms).
2. RTO minimum: RFC 6298 says 1 s; Linux 200 ms.
3. TIME_WAIT: 2×MSL; Linux 60 s; ephemeral range 32768–60999.
4. IW10 (RFC 6928); Mathis constant (1.22 vs √(3/2)).
5. RTCP NACK (RFC 4585); RFC 8085 (UDP usage guidelines).
6. SIP→TCP rule (RFC 3261 §18.1.1: 1300 B / MTU−200 B).
7. GTP-U overhead (8 B mandatory header); 802.1p ↔ DSCP mappings are vendor policy.
8. RFC 5865 VOICE-ADMIT = 44; multicast ranges 232/8 SSM, 239/8 admin-scoped.
9. MCPTT floor control carried over UDP (TS 24.380).
