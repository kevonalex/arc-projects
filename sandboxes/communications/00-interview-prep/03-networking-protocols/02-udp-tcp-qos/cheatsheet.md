# Cheatsheet — 03-net/02 UDP vs TCP, QoS
*Each line cites the worksheet part that derived it. ⚠ = VERIFY.*

## Voice transport (A, B)
- Mouth-to-ear target ≈ 150 ms ⚠ (G.114). Late = lost → retransmit is useless after the playout point.
- TCP: ordered + reliable → HOL blocking: one loss stalls everything behind it (A: frames 4–9 delivered at 290 ms).
- UDP: hole = 1 frame; conceal it (PLC). Signalling → TCP; media → UDP/RTP.

## Handshake (C)
- 3-way = announce+confirm for each ISN, middle two merged. 3rd packet kills delayed-duplicate SYNs.
- SYN/FIN consume 1 seq. First data byte = ISN+1. Cost: 1 RTT.
- TIME_WAIT 2×MSL (Linux 60 s ⚠) → port exhaustion (~470 conn/s at 28 232 ports) → reuse connections.
- SYN cookies = no state until 3rd packet.

## Windows & congestion (D)
- Throughput ≤ W/RTT; BDP = rate × RTT. 65 535 B at 200 ms RTT = 2.6 Mbit/s. Scale shift s: 65 535·2^s ≥ BDP.
- Slow start ×2/RTT (IW10 ⚠); AIMD sawtooth W/2..W; ssthresh = max(cwnd/2, 2 MSS).
- Mathis: Rate ≈ (MSS/RTT)·1.22/√p ⚠.
- RTO (RFC 6298): RTTVAR = ¾RTTVAR + ¼|SRTT−R|; SRTT = ⅞SRTT + ⅛R; RTO = SRTT + 4·RTTVAR; first sample SRTT=R, RTTVAR=R/2.

## Nagle (E)
- Rule: send now if full MSS or nothing unacked; else wait for ACK. Delayed ACK + Nagle = stall.
- Always `TCP_NODELAY` for RPC/SIP/floor control; send each message with one `send`/`writev`.

## MTU/fragmentation (F)
- Fragment data = ⌊(MTU−hdr)/8⌋·8; offset in 8-byte units; MF on all but last. 3000 B @1500 → 1480/1480/40, offsets 0/185/370.
- UDP max payload @1500 = 1472. TCP MSS = MTU−40 (v4), −60 (v6).
- Avoid fragmentation: loss amplification, reassembly state, L4 header only in fragment 0. IPv6: source-only + PMTUD, min 1280.
- GTP-U tunnel costs 36 B (v4) / 56 B (v6) ⚠.

## Multicast/IGMP (G)
- 224.0.0.0/4; 239/8 admin scope; 232/8 SSM ⚠. MAC = 01:00:5e + low 23 bits → 32 groups per MAC.
- IGMPv2: report 0x16, leave 0x17, query 0x11; 8 B `type|resp|csum|group`; snooping on switches; TTL scopes.
- Unicast N copies vs multicast 1 copy; UDP semantics (no reliability).

## DSCP (H)
- TOS = DSCP6|ECN2. EF 46 = 0xB8; AF41 34 = 0x88; CS5 0xA0; CS6 0xC0; AFxy = 8x+2y.
- Per-hop treatment, may be bleached; priority queue + policer. IPv6: `IPV6_TCLASS`. PCP 5 ≈ voice ⚠.
- Cellular legs use bearer QoS (QCI/5QI), not DSCP ⚠.
