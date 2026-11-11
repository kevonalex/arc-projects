# 03-net / 01 — Answers (open only after attempting the worksheet and the tests)

**Q1. Decode of the Part A frame**

| Bytes | Meaning |
|-------|---------|
| `01 00 5e 01 01 01` | dst MAC: IPv4 multicast MAC for 239.1.1.1 (`01:00:5e` + low 23 bits of the group = `01:01:01`; I/G bit set) |
| `02 00 00 00 00 01` | src MAC: locally-administered (U/L bit set) unicast |
| `08 00` | EtherType IPv4 |
| `45` | version 4, IHL 5 → 20 bytes |
| `b8` | DSCP = 0xb8 >> 2 = 46 (EF), ECN = 0 |
| `00 c8` | total length 200 |
| `12 34` | identification |
| `40 00` | flags DF=1, MF=0, offset 0 |
| `40` | TTL 64 |
| `11` | protocol 17 = UDP |
| `2d 36` | header checksum |
| `0a 00 00 01` / `ef 01 01 01` | 10.0.0.1 → 239.1.1.1 |
| `13 8c` ×2 | UDP ports 5004 → 5004 |
| `00 b4` | UDP length 180 |
| `00 00` | UDP checksum absent (legal in IPv4) |
| `80` | RTP V=2, P=0, X=0, CC=0 |
| `00` | M=0, PT=0 (PCMU) |
| `00 01` | seq 1 |
| `00 00 00 a0` | timestamp 160 |
| `11 22 33 44` | SSRC |

**Q2.** Payload starts at offset 14 + 20 + 8 + 12 = **54**. `0xc8` = 200 = IP total length = 20 + 8 + 12 + 160. `0xb4` = 180 = UDP length = 8 + 12 + 160. `160` = payload (20 ms × 8000 Hz × 1 B). `214` = 14 + 200 = frame without FCS (218 with FCS).

**Q3.** Without a common layer each of 4 modules would need to know each of 3 transports' details: 4 × 3 = **12**. With one socket abstraction: 4 + 3 = **7** (each module speaks the abstraction; each transport implements it). Also the reason `Fd`/socket RAII is the single I/O seam in the gateway (pack 03).

**Q4.** The Ethernet payload on the wire is **46 bytes**: the 32-byte IP packet + **14 padding bytes** (usually zero, not part of IP). Receiver reads IPv4 *total length* (32) and ignores the rest. That is exactly why IPv4 needs a total-length field. (Frame = 14 + 46 + 4 FCS = 64 B minimum.)

**Q5.** With FCS: 14 + 20 + 8 + 12 + 160 + 4 = **218 B**. At 50 pps: 218 × 8 × 50 = **87.2 kbit/s**. Add preamble+SFD (8) + IFG (12) = 20 B → **238 B** → 95.2 kbit/s of *line-rate* utilisation. Compare IP-level: 200 B → 80 kbit/s.

**Q6.** Words (checksum zero): `45b8 + 00c8 + 1234 + 4000 + 4011 + 0000 + 0a00 + 0001 + ef01 + 0101`. Sum = `0x1d2c8`. Fold the carry: `0xd2c8 + 1 = 0xd2c9`. Complement: `~0xd2c9 = 0x2d36`. Receiver check: `0xd2c9 + 0x2d36 = 0xffff` ✓ (complement 0).
(Code check: test `Ipv4.WorksheetPartAPacket`.)

**Q7.** IHL 7 → 28 bytes → **8 option bytes**. Max IHL = 15 (4-bit field) → 60 bytes → 40 bytes of options.

**Q8.** Offset in 8-byte units because 13 bits can only count 8192 positions, but the datagram can be 65 535 B; 8192 × 8 = 65 536 covers it. (Max offset value 8191 → byte offset 65 528.) Data per fragment must be a multiple of 8: (1500 − 20) = 1480 = 8 × 185 ✓.
3000 bytes of IP payload → fragments: **(1480, off 0, MF 1), (1480, off 185, MF 1), (40, off 370, MF 0)**. Packet sizes 1500, 1500, 60. Overhead: two extra IP headers = 40 B. Losing any one fragment loses the whole datagram — why you avoid fragmentation for voice/SIP-over-UDP.

**Q9.** A router that receives TTL=1 decrements to 0 and discards, so the stream stays on the local link/subnet. Raise TTL (or use TTL thresholds/admin scopes `239.0.0.0/8`) to cross routers between sites. Risk: talkgroup audio leaking to unintended sites/loops and load; also IGMP/PIM must carry the group.

**Q10.** IPv4 byte 1 (frame offset 15): DSCP 46 = `101110`b; shifted left 2 with ECN `00` → `10111000` = **0xb8**.

**Q11.** IPv4: 160 + 12 + 8 + 20 = 200 B × 50 × 8 = **80 kbit/s**. IPv6: 160 + 12 + 8 + 40 = 220 B × 400 = **88 kbit/s** (+10 %).

**Q12.** IPv6 has no header checksum, so the UDP checksum is the *only* guard for the addresses/ports/payload. Receivers discard IPv6 UDP datagrams with checksum 0 (outside narrow tunnel exceptions) → the gateway's RTP would be silently dropped by compliant stacks.

**Q13.** `6B B1 23 45` = `0110 1011 1011 0001 0010 0011 0100 0101`: version **6**; traffic class = low nibble of byte 0 (`B`) + high nibble of byte 1 (`B`) = **0xBB** = DSCP 46 (EF) + ECN 3 (CE); flow label = `1 23 45` = **0x12345**.

**Q14.** Pseudo-header words: `c0a8 0001 c0a8 0002 0011 000b`; UDP header: `138c 138e 000b 0000` (checksum zeroed); data `41 42 43` is odd → padded with a zero *low* byte: `4142 4300`. Sum = `0x22cd6`; fold: `0x2cd6 + 2 = 0x2cd8`; complement → **0xd327** (matches the test). Non-zero so sent as is.

**Q15.** UDP length = 200 − 20 = **180**. Largest UDP payload without fragmentation at MTU 1500: 1500 − 20 − 8 = **1472** (1460 if RTP's 12-byte header counts as payload-of-UDP overhead).

**Q16.** After the handshake the client's next seq is ISN + 1 = **1001** (SYN consumed 1). The INVITE segment: seq 1001, 700 bytes. Server ACK = 1001 + 700 = **1701**. Client's next segment seq = **1701**.

**Q17.** Max = window / RTT = 65 535 × 8 / 0.2 = **2.62 Mbit/s**. For 20 Mbit/s: W = 20e6 × 0.2 / 8 = **500 000 B** (the BDP). 65 535 × 2^s ≥ 500 000 → 2^s ≥ 7.63 → **shift 3** (window 524 280 B).

**Q18.** Offset 8 → 32 bytes → **12 option bytes**. `02 04 05 B4`: kind 2 = MSS, length 4, value 0x05B4 = **1460** = 1500 − 20 − 20.

**Q19.** SYN and FIN are events that must be delivered reliably and ordered with data; giving each a sequence number lets the cumulative ACK and retransmission machinery cover them without special cases. A pure ACK carries no new information; if it consumed a sequence number it would require its own ACK → infinite ACK ping-pong.

**Q20.** Occupies **8 bytes** (7 + 1 pad). The padding is not counted in `length`. Minimum packet with this chunk: 12 (common) + 8 = **20 B**.

**Q21.** (⚠ VERIFY all)

| Interface | Transport | Port |
|-----------|-----------|------|
| RTP voice | UDP | dynamic (SDP), even by convention |
| RTCP | UDP | RTP port + 1 (or rtcp-mux on same) |
| SIP | UDP/TCP | 5060 |
| SIP-TLS | TCP | 5061 |
| gRPC | TCP (HTTP/2) | deployment-specific (50051 is the examples' convention) |
| GTP-U | UDP | 2152 |
| GTP-C | UDP | 2123 |
| Diameter | TCP or SCTP | 3868 |
| S1AP | SCTP | 36412 |
| NGAP | SCTP | 38412 |

**Q22.** Bugs: (1) `reinterpret_cast` — alignment/aliasing UB, field order assumes struct padding; (2) no `n >= 20` check; (3) version not checked (could be IPv6); (4) `hl` is **words**, must be `× 4`; and `IHL < 5` unchecked; (5) `len`, `id`, `frag` are network byte order — no `ntohs`; (6) `ip->len` unchecked vs `n`, and `len - hl` can underflow (unsigned wrap → huge length → over-read); (7) `l4` pointer not bounds-checked against `n`; (8) fragmentation ignored: non-first fragments have no UDP header, so `process_udp` reads garbage; (9) checksum never verified; (10) `ip->len` may be < `hl`.

**Q23.** NAT changes: IP source address (and header checksum, TTL−1); UDP source port; UDP checksum (pseudo-header contains the IP, plus the port) — unless it was 0 in IPv4. At the application layer, SDP `c=`/`m=`, SIP `Via`/`Contact` contain addresses/ports that NAT cannot see → needs an ALG, or STUN/ICE/symmetric RTP. Lesson: IPs leaking into upper layers make NAT a layering violation.

**Q24.** Outline: vocoder frame (e.g. 160 B G.711) → **RTP** adds 12 B (version, PT, seq, timestamp, SSRC) → **UDP** adds ports, length, checksum (8 B) → **IP** adds src/dst, TTL, DSCP=EF, protocol 17, header checksum (20 B) → **Ethernet** resolves next-hop MAC via ARP, adds dst/src/EtherType (14 B) + FCS (4 B) → PHY adds preamble/IFG. Receiver: NIC filters on dst MAC & FCS → IP checks version/IHL/length/checksum, TTL, protocol → UDP checks length/checksum, finds socket by port → RTP checks version, SSRC, seq (reorder) → jitter buffer → decoder. Total 54 B overhead (80 kbit/s IP vs 64 kbit/s codec).
