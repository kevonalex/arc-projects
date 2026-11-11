# Cheatsheet — 03-net/01 Headers & Layering
*Every line cites the worksheet section that derived it. ⚠ = VERIFY before quoting.*

## Sizes (bytes)
| Header | Size | § |
|--------|------|---|
| Ethernet II | 14 (+4 VLAN, +4 FCS) | C |
| IPv4 | 20–60 (IHL×4) | D |
| IPv6 | 40 fixed | E |
| UDP | 8 | F |
| TCP | 20–60 (offset×4) | G |
| SCTP common / chunk hdr | 12 / 4 | H |
| RTP (no CSRC) | 12 | A |
| Eth wire extras: preamble+SFD 8, IFG 12 ⚠ | 20 | C |

RTP/UDP/IPv4/Eth voice frame overhead = 12 + 8 + 20 + 14 (+4 FCS) = 54 (58). IPv6 → +20. (B.2, Q5, Q11)

## Layering (B)
- M×N → M+N: IP is the narrow waist. Each layer answers one question (MAC→host→program→meaning).
- Violations are real: pseudo-header checksum, NAT, firewall port peeking.

## Ethernet (C)
- dst before src (early NIC filter); first byte bit0 = multicast (I/G), bit1 = local (U/L).
- EtherType ≥ 0x0600 ⚠; 0x0800 v4, 0x86DD v6, 0x0806 ARP, 0x8100 VLAN.
- 802.1Q: PCP3 DEI1 VID12 (4094 usable). Min payload 46 → padding → IP needs total length.
- IPv4 multicast MAC = `01:00:5e` + low 23 bits of group.

## IPv4 (D)
- IHL in words (5..15, max 60 B); frag offset in 8-byte units (13 bits → 64 KiB); flags R/DF/MF.
- TOS byte = DSCP6 + ECN2. EF=46 → 0xB8. TTL = hop budget (also multicast scope). Proto: 1 ICMP, 2 IGMP, 6 TCP, 17 UDP, 41, 132 SCTP.
- Checksum: header only; sum 16-bit words, end-around carry, complement; valid → re-sum = 0xFFFF. Byte-order independent. Incremental: `HC' = ~(~HC + ~m + m')`.
- Known vector: `4500 0073 0000 4000 4011 b861 c0a8 0001 c0a8 00c7`.

## IPv6 (E)
- Fixed 40 B; no checksum, no IHL, no router fragmentation; min MTU 1280; Hop Limit; Flow Label 20 bits; options = extension headers via Next Header.
- Consequence: UDP checksum mandatory in v6.

## UDP (F)
- 4 × 16 bits. length ≥ 8. Pseudo-header (v4: src, dst, 0, 17, len = 12 B). v4 checksum 0 = none; computed 0 → 0xFFFF.
- Max payload at MTU 1500: 1472 (v4). Odd length → pad zero low byte for sum only.

## TCP (G)
- seq counts bytes; SYN & FIN use 1 seq; ack = next expected byte (cumulative).
- Flags (MSB→LSB): CWR ECE URG ACK PSH RST SYN FIN. Window 16 bits (+scale ≤14).
- Throughput ≤ W/RTT; BDP = rate × RTT. MSS = MTU − 40 (1460).

## SCTP (H)
- Message-oriented, multi-stream (no cross-stream HOL), multi-homed, 4-way cookie handshake, verification tag, CRC32c.
- Chunk length excludes padding; round to 4. Used by S1AP (36412) ⚠, NGAP (38412) ⚠, Diameter (3868) ⚠.

## Parsing rules (I)
1. Check n ≥ fixed header, then check every length/IHL/offset claim vs n.
2. Never `reinterpret_cast` packets; shift bytes.
3. Return `optional`; do not throw on bad input.
4. Non-first fragments have no L4 header.
