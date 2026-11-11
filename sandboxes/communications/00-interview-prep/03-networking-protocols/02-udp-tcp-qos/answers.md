# 03-net / 02 — Answers

**Q1. Timeline table** (one-way 30 ms, RTO 200 ms; frame *i* sent at 20i)

| Frame | Sent | Arrives | UDP app gets | TCP app gets | TCP delay | Playable ≤ 60 ms (UDP / TCP) |
|-------|------|---------|--------------|--------------|-----------|------------------------------|
| 2 | 40 | 70 | 70 | 70 | 30 | yes / yes |
| 3 (lost) | 60 | 290 (retransmit: 60 + 200 + 30) | lost | 290 | 230 | no / no |
| 4 | 80 | 110 | 110 | **290** (blocked behind 3) | 210 | yes / no |
| 5 | 100 | 130 | 130 | 290 | 190 | yes / no |
| 9 | 180 | 210 | 210 | 290 | 110 | yes / no |

Unusable: **UDP 1** frame (3); **TCP 7** frames (3–9; frames 0–2 arrive before the stall). Under UDP one 20 ms hole is concealed. Under TCP one loss turns into a 140–230 ms stall followed by a burst of stale frames (HOL blocking).

**Q2.** Frames 4, 5, 6 are sent at 80, 100, 120, arrive at 110, 130, 150; each triggers a dup-ACK that reaches the sender at 140, 160, 180. Third dup-ACK at **180** → retransmit leaves at 180, arrives at **210**. Frame 3 reaches the app at 210 (delay 150). Frame 4 is delivered at 210 → delay 130 ms → **not playable** at 60 ms. Frames 4–9 all unblock at 210 (frame 9 only arrives at 210). Still 7 unusable: even fast retransmit costs ≥ RTT + 3 packet times.

**Q3.** *UDP for media:* (1) late = lost, so retransmission is worthless after the deadline; (2) no HOL blocking — a hole costs one frame; (3) the application owns loss policy (PLC, FEC, NACK within the jitter window) and rate adaptation instead of TCP's blind halving; (4) packet boundaries preserved and multicast possible.
*TCP for signalling:* (1) INVITE/BYE/StartCall must arrive exactly once, in order; (2) messages can exceed an MTU without app-level fragmentation; (3) congestion control + firewall/NAT friendly, TLS available; (4) one RTT of latency is acceptable and the connection gives liveness detection.

**Q4.** UDP: p = 2 % → 1 frame in 50 concealed, each a single 20 ms gap. TCP: each loss stalls ~200 ms = ~10 frames late (all unusable under a 60 ms deadline) → ≈ 2 % × 10 ≈ **20 %** of frames beyond the playout point (a stall every second lasting 0.2 s). Ratio ≈ 10×. TCP additionally halves its window per loss, throttling the stream further.

**Q5.**
1. C→S: `SYN`, seq = 5000, ack = –
2. S→C: `SYN+ACK`, seq = 9000, ack = 5001
3. C→S: `ACK`, seq = 5001, ack = 9001
First data byte: client **5001**, server **9001** (SYN consumed one number each).

**Q6.** Port budget / TIME_WAIT: 28 232 / 60 s ≈ **470 connections/s** to the same dst IP:port (the 4-tuple space is exhausted). Fixes: **persistent connections / multiplexing** (HTTP/2 or gRPC channel reuse), pooling, let the server close first (it then carries the TIME_WAIT), spread across more dst IPs/ports, Linux `tcp_tw_reuse` for outgoing connections (⚠ VERIFY). Architecture lesson: one long-lived channel per peer.

**Q7.** Both: Closed →(ActiveOpen)→ SynSent [send SYN]; each receives the other's SYN → SynReceived [send SYN+ACK]; each receives the SYN+ACK (which carries an ACK) → Established. **4 packets** (2 SYN + 2 SYN+ACK) instead of 3. Code path: `SynSent+RecvSyn→SynReceived`, `SynReceived+RecvAck→Established`.

**Q8.** (a) BDP = 5e6 × 0.6 / 8 = **375 000 B**. (b) 65 535 × 2^s ≥ 375 000 → 2^s ≥ 5.72 → **s = 3** (524 280 B). (c) IW = 14 600 B; need 375 000 / 14 600 ≈ 25.7 → 2^n ≥ 25.7 → **n = 5 RTTs** (4 RTTs give only 233 600 B) → 5 × 0.6 = **3 s**, plus 0.6 s for the handshake. Take-away: on satellite, short TCP flows never fill the pipe → PEP/long-lived connections.

**Q9.** (1400 × 8 / 0.06) = 186 667 bit/s; × 1.22 / √0.005 (= 0.07071) = × 17.25 → **≈ 3.22 Mbit/s** (test value 1 424 960 uses p = 1 %). Less than the 4 Mbit/s requirement: a bulk sync would be loss-limited, not capacity-limited — consider FEC, UDP-based transport, or reduce p.

**Q10.** After 100: SRTT 100, RTTVAR 50, RTO **300**. After 120: RTTVAR = 0.75·50 + 0.25·|100−120| = 42.5; SRTT = 0.875·100 + 0.125·120 = 102.5; RTO = 102.5 + 170 = **272.5**. After 90: RTTVAR = 0.75·42.5 + 0.25·|102.5−90| = 35.0; SRTT = 0.875·102.5 + 0.125·90 = 100.9375; RTO = 100.9375 + 140 = **240.94 ms**. (Note RTTVAR uses the *old* SRTT.)

**Q11.** First `send(5)` goes out immediately (nothing in flight). The second `send(40)` is a small segment while 5 bytes are unacknowledged → **Nagle holds it**. The receiver has only a partial gRPC message and no reply data, so its **delayed ACK** waits up to ~40 ms. The body leaves only when that ACK arrives (+ ½ RTT) → tens of ms extra, more over satellite. Fixes: (1) build the 45-byte message in one buffer / `writev` and send once; (2) `TCP_NODELAY` on the sender; (3) optional `TCP_QUICKACK` at the receiver. (gRPC implementations do (1)+(2).)

**Q12.** **3 segments, 100 ms.** t = 0: segment 1 (nothing in flight), ACK due at 100. Frames at 20, 40, 60, 80 are held (small, data unacked). At 100 the ACK arrives → segment 2 carries frames 1–4 (640 B), the oldest waited 80 ms; frame at 100 starts a new hold; frames to 180 accumulate; the ACK for segment 2 returns at 200 → segment 3 with frames 5–9 (800 B), oldest waited **100 ms** = one RTT. (Test `Nagle.VoiceFramesGetBatchedAndDelayed`.)

**Q13.** (a) Assuming the 1 800 B is the UDP datagram incl. header → 1 808 IP-payload bytes: **(1480, off 0, MF), (328, off 185, last)**; two fragments. (b) Reasons for the SIP rule: fragments are fragile (one loss kills the message, UDP has no retransmit beyond SIP's own timers), PMTU is unknown, and **proxies add Via/Record-Route headers** so the request grows in transit — hence a 200 B safety margin; TCP gives segmentation and congestion control.

**Q14.** 3 860 IP payload B at MTU 1500 → **(1480, 0), (1480, 185), (900, 370)** → 3 fragments, 3 920 wire-IP bytes per frame (1.57 Mbit/s). Loss of the frame = 1 − 0.99³ ≈ **2.97 %** (≈ 3× the single-packet loss). Also reassembly adds jitter. Fix: compress (Opus/G.711) to < 200 B/frame.

**Q15.** Outer overhead 56 B → inner packet ≤ 1500 − 56 = **1444 B**; inner IPv6 40 + UDP 8 + RTP 12 = 60 → RTP payload ≤ **1384 B**. Voice frames are far below this, but SDP/SIP-over-UDP or large payloads (e.g. 48 kHz PCM) are not.

**Q16.** Words: `1600 0000 ef01 0101` → sum `0x10602` → fold `0x0603` → complement **0xF9FC**. Message: `16 00 f9 fc ef 01 01 01`. Receiver: `1600 + f9fc + ef01 + 0101 = 0x1fffe` → fold `0xffff` → complement 0 ✓.

**Q17.** 239.1.1.1 = `EF 01 01 01`; 239.129.1.1 = `EF 81 01 01`. The dropped bits are the 5 bits above the low 23 (the low nibble of the first octet, which is identical here, and the top bit of the second octet: `0x81 & 0x7F = 0x01`) → **same MAC 01:00:5e:01:01:01**: yes, they collide. An IGMP-snooping switch maps by MAC, so members of either group receive both; the IP stack discards the unwanted one but link bandwidth is wasted. Allocation: `group(N) = 239.1.(N>>8).(N & 0xFF)` for talkgroup N < 65 536 → unique MACs `01:00:5e:01:hi:lo`; never use 2nd octets ≥ 128 within the same first octet, and avoid anything mapping to `01:00:5e:00:00:xx` (224.0.0.x and aliases are always flooded).

**Q18.** Assumption: all 300 groups active, 5 receiving sites each, one shared core link between the source gateway and the distribution router, replication at the edge. Unicast: 300 × 5 × 80 kbit/s = **120 Mbit/s**. Multicast: one copy per group on the shared link: 300 × 80 kbit/s = **24 Mbit/s** (5× less); per-site downlinks carry each group once either way. The source gateway also sends 1 packet instead of 5 per frame.

**Q19.** EF 46 → **0xB8**; AF41 34 → **0x88**; CS5 40 → **0xA0**; CS6 48 → **0xC0**; BE → **0x00**; EF + CE (ECN 3) → **0xBB**.

**Q20.** (1) The app/stack never set it: wrong option (IPv6 needs `IPV6_TCLASS`), `setsockopt` error ignored, or the OS overwrote it → check `getsockopt`/capture on the *sending* host. (2) **Bleached/re-marked** by a switch, firewall, NAT, VPN or carrier at a trust boundary (and tunnel entry may not copy inner DSCP to the outer header) → capture on both sides of each hop to see where it changes; ask the carrier about its trust/re-marking policy. (3) The cellular leg uses **bearer QoS (QCI/5QI)** not DSCP, so the marking is mapped/dropped at the core ⚠ VERIFY (QCI 1 for conversational voice). (4) Capture point is after decapsulation or on a different VLAN/PCP mapping.

**Q21.** Floor requests/grants are tiny, latency-critical request/response messages; a dedicated UDP protocol can use its own short retransmission timers and bounded retries (tens to hundreds of ms) instead of TCP's RTO floor (200 ms – 1 s) and HOL; no per-client connection state at scale; works with multicast; same network treatment as media. TCP's Nagle/delayed-ACK interactions and handshake would add latency.

**Q22.** Outline: RTP needs *late = lost* semantics: TCP/QUIC streams retransmit and deliver in order (HOL), QUIC removes HOL between streams but each stream is still reliable, and its congestion controller may stall the audio; QUIC datagrams (RFC 9221) are unreliable and could carry RTP, as in RTP-over-QUIC drafts — the honest answer is "possible, but you give up multicast, accept a handshake/TLS 1.3 cost, UDP middlebox/QoS inspection issues, and the ecosystem (SBCs, RTCP tooling, hardware) is RTP/UDP." ⚠ VERIFY the RTP-over-QUIC status.

**Q23.**

| Flow | Transport | TOS/DSCP | Socket options |
|------|-----------|----------|----------------|
| PTT media | UDP (RTP), multicast within the fabric | EF (46) → 0xB8 | `IP_TOS`, `SO_REUSEADDR`, big `SO_RCVBUF`, nonblocking |
| Call control RPC | TCP (gRPC/HTTP-2) | CS3/AF31 or CS5 (policy) ⚠ | `TCP_NODELAY`, keepalive, reuse the channel |
| Bulk logging | TCP | CS1/BE (scavenger) | large buffers; Nagle on is fine; rate-limit |
| Heartbeat | UDP (or app-level on gRPC stream) | CS6/CS3 or BE | small, timer-driven, no retransmit; detect loss by N missed beats |
