# 03-net / 01 — Layering and Headers, Field by Field

**Time:** ~3 h · **Prereqs:** `02-bits/03-parse-headers`, `02-bits/04-crc-checksum` · **Code:** `code/` (C++17, GoogleTest)
**Etherstack lens:** an LMR↔4G/5G gateway receives voice as IP packets (RTP/UDP), signals with SIP/gRPC (TCP), and talks to the core over SCTP/GTP-U. If you cannot read these headers by eye you cannot debug the gateway in Wireshark or write its parsers.

> **Legal:** use only the synthetic packets in this sheet, published test vectors, or your own loopback captures. Never receive live public-safety traffic (UK Wireless Telegraphy Act 2006).

**How to work:** do Part A *before* reading Parts B–H (concrete first). Attempt every question on paper. Then make the failing tests in `code/` pass. Only then open `answers.md`.

Conventions: all multi-byte header fields are **big-endian ("network order")**. Hex is shown as bytes. `⚠ VERIFY` = a figure I could not confirm from memory with certainty; check it in the cited RFC/standard before quoting it in an interview.

---

## Part A — One real packet, before any theory

A gateway sends one 20 ms G.711 µ-law voice frame to multicast group 239.1.1.1 (a talkgroup). This is the *exact* frame (214 bytes; payload abbreviated):

```text
off  bytes
0    01 00 5e 01 01 01   02 00 00 00 00 01   08 00                          <- Ethernet II (14)
14   45 b8 00 c8  12 34 40 00  40 11 2d 36  0a 00 00 01  ef 01 01 01        <- IPv4 (20)
34   13 8c 13 8c  00 b4 00 00                                                <- UDP (8)
42   80 00 00 01  00 00 00 a0  11 22 33 44                                   <- RTP (12)
54   ff ff ff ... (160 bytes of 0xFF = µ-law digital silence)                <- payload
```

**Q1.** Without reading further, decode every field you can: destination MAC, source MAC, EtherType, IP version, header length in bytes, DSCP, total length, DF flag, TTL, protocol, source and destination IP, UDP ports and length, RTP version/payload type/sequence/timestamp/SSRC. *Context: this is what you'd see in Wireshark when a talkgroup frame leaves the gateway.*

**Q2.** At which byte offset does the voice payload start? What do the four numbers `0xc8`, `0xb4`, `160`, `214` have to do with each other?

Keep your answers. Parts B–H explain **why each field exists**; then re-check Q1.

---

## Part B — Why layers exist (derive it)

### B.1 The problem
Suppose you have **M = 5 applications** (voice, SIP, gRPC, file sync, telemetry) and **N = 4 link technologies** (Ethernet, Wi-Fi, LTE bearer, point-to-point microwave). If every application talks to every link directly you write **M × N = 20** adaptations. Insert one common layer in the middle that every app targets and every link carries, and you write **M + N = 9**. That middle layer is IP — the *narrow waist*. Layering is a *complexity* argument, not a diagram habit.

### B.2 Encapsulation = each layer adds the minimum it needs
Each layer answers exactly one question and puts the answer in a header:

| Layer | Question | Header field answering it |
|-------|----------|---------------------------|
| Link (Ethernet) | Which NIC on *this wire* gets it? | dst/src MAC |
| Network (IP) | Which *host* across many wires? | dst IP, TTL |
| Transport (UDP/TCP/SCTP) | Which *program* on that host? How reliable? | ports, seq/ack |
| Application (RTP/SIP) | What do the bytes *mean* (time, codec)? | RTP PT/ts/seq |

Overhead is the price: for our G.711 frame it is 14 + 20 + 8 + 12 = **54 bytes of header around 160 bytes of voice** — 25 % of the frame is plumbing (derived in `04-audio/01`).

### B.3 What breaks when you ignore layering
* Put the destination *port* in the IP header → routers must understand every transport; new transports (SCTP in the 2000s) would need router upgrades. That is why the **protocol** field exists and routers ignore ports.
* **Layer violations are real:** the UDP/TCP checksum includes IP addresses (pseudo-header, Part F); NAT rewrites ports → must fix checksums; firewalls peek at L4. Layering is a discipline with deliberate exceptions.

> **Common misconceptions**
> 1. *"OSI has 7 layers so the Internet does."* The Internet stack is 4–5 layers; OSI layers 5–6 were never implemented as separate things.
> 2. *"A router reads TCP ports."* A plain router forwards on the IP header only. Firewalls/NAT/ECMP hashing *do* peek at ports — that is a violation, not the rule.
> 3. *"Headers are free."* 54 B of header on a 160 B voice frame costs 25 % of the bandwidth (and 100 % more packets per second than needed if you chose too short a frame).

**Q3.** (Derivation) Four Etherstack gateway software modules (RTP I/O, SIP, gRPC, telemetry) and three transports (UDP, TCP, SCTP): how many integration paths without a common socket-like layer vs with one? *Context: why your C++ code talks to an `Fd`/socket abstraction instead of each protocol's wire format.*

---

## Part C — Ethernet II (L2)

```text
 0                   6                  12     14
 +-------------------+-------------------+------+----------------+-----+
 | dst MAC (6)       | src MAC (6)       | type | payload 46-1500| FCS |
 +-------------------+-------------------+------+----------------+-----+
         optional 802.1Q tag inserted before 'type':  TPID 0x8100 | PCP(3) DEI(1) VID(12)
```

**Why each field**
* **dst first, src second:** the NIC must decide "is this for me?" as early as possible, in hardware, after only 6 bytes. If src came first the filter would wait 6 more bytes. *Receivers drop, they rarely need the sender.*
* **MAC = 48 bits:** big enough for a globally unique burned-in ID with no coordination (24-bit OUI + 24-bit device).
* **Two special bits in the first byte:** bit 0 (**I/G**) = 1 means group/multicast; bit 1 (**U/L**) = 1 means locally administered. `01:00:5e` has I/G=1 → IPv4 multicast. (`02:00:...` in Part A is locally administered unicast.)
* **EtherType (16 bits):** demultiplex key to the layer above: `0x0800` IPv4, `0x86DD` IPv6, `0x0806` ARP, `0x8100` VLAN tag. Original 802.3 used this field as a *length*; values **≤ 1500 are lengths, ≥ 1536 (0x0600) are types** ⚠ VERIFY the 0x0600 boundary. That is why the max payload (MTU) is 1500: it had to stay below 1536 so both interpretations coexist.
* **802.1Q tag (4 bytes):** TPID 0x8100 sits where EtherType would be (so old parsers see "an unknown type"), then **PCP** 3 bits = 8 priority classes (QoS at L2, maps to DSCP in Part 02), **DEI** 1 bit = drop-eligible, **VID** 12 bits = 4096 − reserved 0 and 4095 = **4094** usable VLANs. Frame grows 14 → 18 bytes.
* **Minimum payload 46 B:** a collision on 10BASE-T must be detected while the sender is still transmitting → min frame 64 B including header+FCS (14 + 46 + 4 = 64). Shorter IP packets are **padded**, so the receiver cannot infer the IP length from the frame — **this is why IPv4 has a total-length field**.
* **FCS (CRC-32), 4 bytes:** link-level integrity; usually stripped by the NIC so you do not see it in Wireshark. Link to `02-bits/04` (same polynomial family as many standards).
* **On the wire only:** 7-byte preamble + 1-byte SFD + 12-byte inter-frame gap = **20 bytes** of "invisible" overhead per frame (needed when computing *line-rate* utilisation, not IP bandwidth). ⚠ VERIFY if quoting.

> **Common misconceptions**
> 1. *"MTU 1500 means a 1500-byte frame."* MTU is the **L2 payload** limit; the frame is 1514 (1518 with FCS, 1522 with VLAN).
> 2. *"The MAC address crosses routers."* L2 addresses are rewritten at every hop; IP addresses are end-to-end.

**Q4.** A tiny keep-alive datagram carries 4 bytes of application data: IP total length = 20 (IP) + 8 (UDP) + 4 = 32 bytes. What does the Ethernet payload look like on the wire (how many bytes, what are the extra ones), and how does the receiver know where the IP packet really ends? *Context: tiny keep-alive packets from a radio gateway.*

**Q5.** Compute the Ethernet frame size (with FCS, without preamble/IFG) of Part A, then the size including preamble + IFG. *Context: you're sizing a 1 GbE trunk for N concurrent talkgroups.* (Answer feeds `04-audio/01`: 87.2 kbit/s per voice stream.)

---

## Part D — IPv4 (RFC 791), every field

```text
 0       4       8              16      19                  31
 +-------+-------+--------------+-------+--------------------+
 |Ver(4) |IHL(4) | DSCP(6)ECN(2)|   Total Length (16)        |
 +-------+-------+--------------+-------+--------------------+
 |        Identification (16)   |R|DF|MF| Fragment Offset(13)|
 +------------------------------+-------+--------------------+
 |  TTL (8)      | Protocol (8) |        Header Checksum(16) |
 +---------------+--------------+----------------------------+
 |                  Source Address (32)                       |
 |                  Destination Address (32)                  |
 |                  Options (0..40 B, padded)                 |
 +------------------------------------------------------------+
```

| Field | Bits | Why it must exist (and what fails without it) |
|-------|------|-----------------------------------------------|
| Version | 4 | First nibble selects how to interpret *everything else*. A dual-stack NIC or tunnel decoder uses it to tell v4 from v6. |
| IHL | 4 | Options make the header variable-length; the receiver must know where payload starts. Counted in 32-bit words so 4 bits reach 15×4 = **60 B** max; min 5 (=20 B). Counting bytes would need 6 bits. |
| DSCP / ECN | 6 / 2 | Originally an 8-bit "TOS". Reinterpreted (RFC 2474/3168) as **DSCP** (per-hop queuing class, EF = 46 for voice) + **ECN** (routers signal congestion without dropping). |
| Total length | 16 | Max 65 535 B. Needed because L2 may **pad** (Part C). |
| Identification | 16 | Tag shared by all fragments of one datagram so the receiver can reassemble. Not needed if never fragmented (DF=1). |
| Flags | 3 | bit 0 reserved; **DF** (don't fragment: used for Path-MTU discovery); **MF** (more fragments follow). |
| Fragment offset | 13 | Where this fragment sits in the original payload, **in 8-byte units**. 13 bits can address 8192 units; × 8 = 65 536 B — exactly covers the 16-bit length. Hence every fragment except the last carries a multiple of 8 data bytes. |
| TTL | 8 | Routing loops happen during convergence. Without a hop budget a looping packet would circulate forever and multiply under multicast. Each router decrements; 0 → drop (+ICMP). Used also for **multicast scope** (TTL=1 stays on the link). |
| Protocol | 8 | Demux key to L4: 1 ICMP, 2 IGMP, 6 TCP, 17 UDP, 41 IPv6-in-IPv4, 132 SCTP. |
| Header checksum | 16 | Protects **only the header**; payload is protected by L4/L2. Recomputed at every hop because TTL changes. |
| Src / Dst | 32 | End-to-end identity. |
| Options | ≤40 B | Record route, timestamps; rarely used and often dropped/slow-pathed. |

### D.1 Deriving the Internet checksum (RFC 1071)
Goal: cheap detection of corruption *in software on a 1980s CPU* and cheap incremental update after TTL−1.
1. Treat the header as 16-bit big-endian words. Set the checksum field to 0.
2. Add the words in **one's-complement arithmetic**: any carry out of bit 15 is added back at bit 0 (**end-around carry**).
3. Complement the result; store it.
Receiver adds *all* words including the checksum → result `0xFFFF` (before complement: i.e. the complement is `0`). 
Why one's complement? (a) end-around carry makes the sum **independent of byte order** (compute on LE host, store swapped, still valid); (b) an update after changing one word is local (RFC 1624: `HC' = ~(~HC + ~m + m')`).
Weakness: swapping two 16-bit words, or adding 1 to one and subtracting 1 from another, is invisible. That is why L2 has CRC-32.

Worked example (check against `kIpv4Hdr` in the tests): `4500 0073 0000 4000 4011 [0000] c0a8 0001 c0a8 00c7` → checksum **b861**.

> **Common misconceptions**
> 1. *"IHL is in bytes."* It is in 32-bit words (5 means 20 B).
> 2. *"The IP checksum covers the payload."* Header only.
> 3. *"Fragment offset is in bytes."* 8-byte units, so a naive `offset * 1` is a classic parsing bug.
> 4. *"TTL is time."* It is a hop count in practice.

**Q6.** Part A header: compute the IPv4 header checksum by hand (show the 16-bit additions and the fold). Then verify the receiver-side sum equals `0xFFFF`. *Context: your gateway builds this header; a mismatch means every router silently drops your voice.*

**Q7.** A header has IHL = 7. How many option bytes? What is the largest legal IHL and why that number?

**Q8.** Explain why the fragment offset is in 8-byte units, and derive the maximum value of the offset field times its unit. Then: a 3 000-byte UDP datagram payload+header crossing a 1 500-MTU link with a 20-byte IP header — list each fragment's data length, offset field, and MF. *Context: an SDP/SIP message over UDP that grew too big.*

**Q9.** A multicast talkgroup stream is sent with TTL = 1. Where does it stop? When would you raise TTL, and what is the risk? *Context: LMR talkgroup distribution between sites.*

**Q10.** DSCP 46 (EF) is placed in which byte, with which value? (Compare with `0xb8` in Part A.) *Context: voice priority marking from the gateway.*

---

## Part E — IPv6 (RFC 8200): what was removed and why

```text
 0       4          12                       32
 +-------+----------+------------------------+
 |Ver(4) |TrafficCl(8)| Flow Label (20)      |
 +-------+----------+---------+----+---------+
 | Payload Length (16) | Next Header (8) | Hop Limit (8) |
 +---------------------+-----------------+---------------+
 | Source address (128)  | Destination address (128)      |   total fixed header = 40 B
```

| Removed from v4 | Why |
|-----------------|-----|
| IHL | Header is fixed 40 B; options moved to **extension headers** chained by *Next Header*. Routers skip all but hop-by-hop. |
| ID / flags / offset | Routers **never fragment**. Only the source may (Fragment extension header). So Path-MTU discovery is mandatory; every link must carry **1280 B** (minimum IPv6 MTU). |
| Header checksum | Link CRC + L4 checksum already exist; removing it saves per-hop work (TTL change needs no recompute). Consequence: **UDP checksum becomes mandatory over IPv6** (with narrow tunnel exceptions, RFC 6935/6936 ⚠ VERIFY). |
| TTL → **Hop Limit** | Renamed to say what it is. |
| Total length → **Payload Length** | Counts bytes *after* the 40-byte header (extension headers included). |

New: **Flow Label (20 bits)** lets routers keep packets of one flow on one path (ECMP / queueing) *without parsing L4* — important when L4 is hidden by IPsec/fragments.
Addresses 128 bit: 2¹²⁸ ≈ 3.4×10³⁸. Multicast prefix `ff00::/8`; `ff02::1` = all nodes on link.

> **Common misconceptions**
> 1. *"IPv6 headers are 40 bytes so IPv6 is slower."* Fixed layout is *easier* to parse in hardware.
> 2. *"IPv6 has no fragmentation."* It does — end hosts only.
> 3. *"IPv6 has no checksum so no integrity."* The L4 checksum includes a pseudo-header of the addresses.

**Q11.** Recompute the G.711 RTP voice stream IP-level bandwidth for IPv4 and IPv6 (payload 160 B, 50 pps). *Context: 5G core networks are often IPv6-only.*

**Q12.** Why does IPv6 make UDP checksums mandatory? What does an Etherstack gateway risk if it sends RTP over IPv6 with checksum 0?

**Q13.** Decode the first 4 bytes `6B B1 23 45`: version, traffic class, flow label. (Check `Ipv6.Parse` test.)

---

## Part F — UDP (RFC 768): 8 bytes, 4 fields

`src port (16) | dst port (16) | length (16) | checksum (16)`

* **Ports:** demultiplex to a process. *Source port is optional in spirit* (may be 0 if no reply is expected); RTP conventionally uses an **even** port with RTCP on port+1 ⚠ VERIFY (RFC 3550 §11).
* **Length:** header+data. Redundant with IP total length minus IP header, but UDP must be self-describing for the checksum pseudo-header and for non-IP carriers. Valid range [8, 65 507 for v4 over IP without options].
* **Checksum:** covers a **pseudo-header** + UDP header + data. Pseudo-header = `src IP(4) | dst IP(4) | 0 (1) | protocol 17 (1) | UDP length (2)` (12 B for v4, 40 B for v6). Why include IPs that are not in the UDP header? Because a datagram with intact bytes but **delivered to the wrong host** (a router bug flips a bit in the dst address; the IP checksum is recomputed *by the buggy router*) must be rejected. IPv4: checksum 0 = "none"; a computed 0 is sent as `0xFFFF` (its one's-complement alias).
* **No** sequence numbers, handshake, retransmission, flow/congestion control. That is the point (Part 02): real-time voice prefers a late packet *dropped* to an on-time stream *delayed*.

> **Common misconceptions**
> 1. *"UDP is unreliable so it's bad."* It is a minimal substrate on which RTP builds *just* the reliability voice needs.
> 2. *"UDP checksum 0 is allowed everywhere."* Only IPv4 (and tunnel exceptions).
> 3. *"One `sendto` = one `recvfrom` of the same size."* Datagram boundaries are preserved (unlike TCP) but may be lost/duplicated/reordered.

**Q14.** Compute the UDP checksum for src 192.168.0.1:5004 → 192.168.0.2:5006, payload `41 42 43` (odd length!). Show pseudo-header words and the pad rule. (Test `Udp.ParseAndChecksum` has the reference value — try before looking.) *Context: validating a captured RTP/UDP packet from a lab gateway.*

**Q15.** IP total length 200, IHL 5; what must the UDP length be? What is the largest UDP payload that fits one 1 500-byte-MTU Ethernet frame without fragmentation?

---

## Part G — TCP (RFC 9293), every field

```text
 +--------- src port (16) ---------+--------- dst port (16) ----------+
 |                     sequence number (32)                          |
 |                  acknowledgement number (32)                      |
 | data off(4) | rsvd(3)+NS | CWR ECE URG ACK PSH RST SYN FIN | window(16) |
 |            checksum (16)           |        urgent pointer (16)       |
 |                  options (0..40 B): MSS, WScale, SACK, Timestamps     |
```

| Field | Why |
|-------|-----|
| Seq (32) | Numbers **bytes**, not packets, because TCP is a byte stream and segmentation is free to change. Initial value (ISN) is random so old duplicates from a previous connection are not accepted. SYN and FIN each consume **one** sequence number so they can be acknowledged and retransmitted like data. |
| Ack (32) | **Cumulative:** "next byte I expect". One number acknowledges everything before it → ACK loss is harmless. |
| Data offset (4) | Same reason as IHL: options. Words of 32 bits; 5..15. |
| Flags | **SYN** open, **ACK** ack field valid, **FIN** I'm done sending, **RST** abort, **PSH** deliver now (hint), **URG** urgent ptr valid (obsolete), **ECE/CWR** ECN echo / window reduced. |
| Window (16) | Receiver's free buffer → **flow control**. 16 bits = 64 KiB limit → **Window Scale option** (shift 0..14 → up to ~1 GiB). |
| Checksum | Same algorithm as Part D with pseudo-header; **mandatory** for TCP. |
| Options | **MSS** (largest segment = MTU − 40 for v4), **SACK** (tell sender which holes), **Timestamps** (RTT measure + PAWS: sequence numbers wrap in 2³²/rate seconds — at 10 Gbit/s ≈ 3.4 s), **Window scale**. |

Key numbers: bandwidth–delay product `BDP = rate × RTT`; a window `W` bounds throughput at `W / RTT`.

> **Common misconceptions**
> 1. *"ack = number of the last byte received."* It is the *next expected* byte.
> 2. *"TCP preserves message boundaries."* It does not; one `send(100)` may arrive as two `recv`s.
> 3. *"PSH makes TCP send immediately."* It's mostly ignored on the sender; do not rely on it for framing.

**Q16.** A SIP INVITE of 700 bytes is sent over TCP with ISN(client) = 1000 after the handshake. Give the data segment's seq, the ACK number the server returns, and the sequence of the client's next segment. *Context: SIP over TCP between gateway and an MCPTT server.*

**Q17.** Backhaul to a remote site has RTT = 200 ms. Max TCP throughput with the default 65 535-byte window? What window is needed for 20 Mbit/s? What shift count does that need?

**Q18.** The data offset field reads `8`. How many option bytes? In test `Tcp.SynAndDataOffset` the SYN has offset 6 with an MSS option: explain each of the 4 option bytes (`02 04 05 B4`).

**Q19.** Why do SYN and FIN consume a sequence number but a pure ACK does not?

---

## Part H — SCTP (RFC 4960 / 9260): the transport of the cellular control plane

```text
 Common header (12 B): src port (16) | dst port (16) | verification tag (32) | checksum CRC32c (32)
 Chunk:  type (8) | flags (8) | length (16, includes the 4-byte chunk header) | value ... | padding to 4 B
```

Why it exists: telephony signalling (SS7 over IP, later S1AP/NGAP/Diameter) needed what TCP and UDP each lacked.

| Feature | Reason |
|---------|--------|
| **Message-oriented** | Signalling messages have boundaries; no re-framing as in TCP. |
| **Multi-streaming** | Loss on stream A does not block stream B → avoids TCP **head-of-line blocking** between independent calls (Part 02). |
| **Multi-homing** | A peer has several IPs; failover without dropping the association — carrier availability. |
| **Verification tag** | 32-bit random value each side picks; every packet must carry the *peer's* tag. A blind attacker must guess 2³² values; replaces the weak 3-way handshake security. |
| **4-way handshake with cookie** | INIT → INIT-ACK(cookie) → COOKIE-ECHO → COOKIE-ACK. Server keeps *no state* until the cookie returns → SYN-flood resistance. |
| **CRC32c** | Stronger than Adler-32 (original) and has hardware support (SSE4.2); ⚠ VERIFY RFC 3309→4960 history. |
| **Chunk length excludes padding** | but chunks are padded to 4-byte alignment: a 7-byte chunk occupies 8 bytes. |

> **Common misconceptions**
> 1. *"SCTP replaces TCP on the web."* It's mainly telecom (S1-MME/N2/Diameter); NAT/firewall support is poor, so the web stayed on TCP/QUIC.
> 2. *"SCTP is just TCP with messages."* Different handshake, multi-homing, multi-streaming.

**Q20.** A chunk has `length = 7`. How many bytes does it occupy in the packet? What is the minimum SCTP packet containing one such chunk? (Test `Sctp.CommonAndChunks`.)

**Q21.** (Integration table) For each Etherstack interface give the transport and well-known port, or write VERIFY: RTP voice, RTCP, SIP, SIP-TLS, gRPC, GTP-U (user plane), GTP-C, Diameter, S1AP, NGAP. *Context: configuring a firewall between the gateway and a carrier core.*

---

## Part I — Parsing safely (what interviewers probe)

Rules (all enforced by tests in `code/tests/test_net_headers.cpp`):
1. **Every length field is attacker-controlled.** Check `n >= fixed_header`, then check the field's claim against `n` *before* using it.
2. **IHL and data offset first, then the rest** — a malicious `IHL=15` with a 20-byte buffer is the classic over-read.
3. **No `reinterpret_cast<Ipv4Header*>(buf)`:** alignment (UB on some embedded cores), strict aliasing UB, bit-field order is implementation-defined, padding, endian. Read bytes and shift.
4. **Return `std::optional`/`expected`, never throw on bad input** (hot path, embedded).
5. Do not trust a valid checksum to mean a valid packet (it only says "probably not corrupted"); and do not skip validation of a bad checksum on a fuzzed packet.

**Q22.** Find all bugs:

```cpp
struct Ip { uint8_t ver_ihl, tos; uint16_t len, id, frag; uint8_t ttl, proto; uint16_t csum; uint32_t src, dst; };
void handle(const uint8_t* buf, size_t n) {
    auto* ip = reinterpret_cast<const Ip*>(buf);
    size_t hl = (ip->ver_ihl & 0x0F);
    const uint8_t* l4 = buf + hl;
    if (ip->proto == 17) process_udp(l4, ip->len - hl);
}
```
*Context: the first parser a junior writes in a gateway.*

**Q23.** (Layer violation) NAT rewrites a UDP source port. List every field that must change and why. *Context: gateway behind carrier-grade NAT.*

**Q24.** (Oral, 90 s) "A 20 ms voice frame is produced by the vocoder. Walk me through every header added until it is a bit on the wire, and what each layer's receiver checks." Practise with Part A as your prompt.

---

## Part J — Build tasks (`code/`)

| Step | Function (in `net_headers.hpp`) | Test |
|------|--------------------------------|------|
| 1 | `rd16/rd32/wr16/wr32` | `Endian.ReadWrite` |
| 2 | `internet_checksum` (with `initial` pre-load) | `Checksum.*` |
| 3 | `parse_ethernet` (+802.1Q) | `Ethernet.PlainAndVlan` |
| 4 | `parse_ipv4`, `ipv4_checksum_ok`, `build_ipv4` | `Ipv4.*` |
| 5 | `parse_ipv6` | `Ipv6.Parse` |
| 6 | `parse_udp`, `udp_checksum_v4` | `Udp.ParseAndChecksum` |
| 7 | `parse_tcp` | `Tcp.SynAndDataOffset` |
| 8 | `parse_sctp_common`, `parse_sctp_chunk`, `sctp_padded_len` | `Sctp.CommonAndChunks` |

```bash
cmake -S code -B build && cmake --build build
./build/headers_tests_student    # your code in code/include/  (red until you implement it)
./build/headers_tests_solution   # reference in code/solutions/ (must be green)
ctest --test-dir build --output-on-failure
```
Edit **`code/include/net_headers.hpp`** (the stub). Do not edit `tests/`.
Flags: `-Wall -Wextra -fsanitize=address,undefined`.

**Stretch:** feed a fuzz loop of random bytes into every `parse_*` under ASan/UBSan; the parsers must never crash and must reject all malformed inputs.

---

## ⚠ VERIFY list (this pack)
1. EtherType/length boundary 0x0600 (1536) — IEEE 802.3.
2. Preamble+SFD 8 B and IFG 12 B (=20 B per frame on the wire).
3. RTP/RTCP even/odd port convention — RFC 3550 §11.
4. UDP zero-checksum exceptions over IPv6 — RFC 6935/6936.
5. SCTP checksum history (Adler-32 → CRC32c, RFC 3309 → RFC 4960).
6. Port numbers in Q21 (GTP-U 2152, GTP-C 2123, Diameter 3868, S1AP SCTP 36412, NGAP SCTP 38412, SIP 5060/5061).
7. Wikipedia IPv4 checksum vector `…b861` is reproduced by the solution; confirm against your own Wireshark capture.
