# Pack 03 — Parsing Headers Safely: BitReader/BitWriter, RTP, IPv4, UDP (Etherstack interview worksheet)

> **Audience:** from zero; C++17/20. **Rule:** concrete bytes first, derive the layout, then write code. If you catch yourself memorising field offsets, re-derive them from the diagrams.
> **Data rule:** every byte below is **synthetic** (private addresses, made-up SSRCs). No live public-safety RX, no real traffic captures.
> **Companions:** [`answers.md`](answers.md) · [`cheatsheet.md`](cheatsheet.md) · `code/` (stubs in `code/include/pktheaders.h`, tests, reference in `code/solutions/`).
> **Prereqs:** Pack 01 (shift/mask/extract) and Pack 02 (big-endian loads).

Sections (cheatsheet cites these): **§1** Why · **§2** Prerequisites · **§3** Concept build-up (3.1–3.7) · **§4** Worked example · **§5** Questions Q1–Q16 · **§6** Self-assessment.

---

## §1 Why this matters

An Etherstack-style **LMR gateway** takes voice that arrived as RTP/UDP/IPv4 from a network (a console, a repeater site, another gateway) and hands it to a radio stack — and the reverse. Everything arrives as raw bytes **from a network you do not control**. Any field can be wrong, hostile, or truncated.

| Where | Why parsing discipline matters |
|---|---|
| **Gateway / network** | A bad length field read as trusted is a buffer over-read: crash, or memory leak to a peer. A parser is *the* attack surface of a gateway. |
| **LMR audio** | RTP `seq`/`timestamp`/`marker` drive jitter buffering and push-to-talk (PTT) start detection. Mis-parse them and speech stutters or clips the first syllable. |
| **Embedded** | No MMU to catch an over-read; unaligned loads fault on some MCUs; code size and speed matter, so no heavy libraries. |
| **Interview** | "Parse this header", "what do you validate?", "why not cast the buffer to a struct?" |

**You will be able to:** read bit-packed fields MSB-first with a `BitReader`; parse RTP, IPv4, UDP field by field and say *why each field exists*; and write a parser that cannot read past its buffer.

**Failure if ignored:** the Heartbleed class of bug (a length field trusted without bounds-checking against the real buffer; TLS heartbeat, 2014 — VERIFY) is what happens when step "check `len`" is skipped.

---

## §2 Prerequisites

- **Pack 01:** `(x >> pos) & mask`; shifting by ≥ width is UB; use unsigned.
- **Pack 02:** network order is big-endian; load with shifts, never `*(uint16_t*)p`; integral promotion.
- **Reading RFC header diagrams.** A diagram like
```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
```
is read **left to right, top to bottom**; the tick numbers count bits **from the left, the most-significant bit first**. Each row is one 32-bit word (4 bytes). So "bit 0" in an RFC is the *top* bit of the first byte. In C++ we have bit 0 = LSB. A diagram field spanning ticks 4–7 of the first byte is the **low nibble** of that byte (`b & 0x0F`), not "bits 4..7" in C++ numbering.
- `size_t` is unsigned; `len - 8` when `len < 8` wraps to a huge number (§3.3).

---

## §3 Concept build-up

### 3.1 Why not `reinterpret_cast` a buffer to a struct?

**Concrete first.** Here is a synthetic RTP header on the wire (12 bytes):

```
80 80 00 01 00 00 00 A0 12 34 56 78
```
Tempting code:
```cpp
struct Rtp { uint8_t vpxcc; uint8_t mpt; uint16_t seq; uint32_t ts; uint32_t ssrc; };
const Rtp* h = reinterpret_cast<const Rtp*>(buf);
use(h->seq, h->ts);
```
What goes wrong (each is independent; any one is enough to reject it):

| # | Problem | Concrete consequence here |
|---|---|---|
| 1 | **Byte order** | On a little-endian CPU `h->seq` reads memory `00 01` as `0x0100 = 256`, not 1; `h->ts` = `0xA0000000`, not 160 |
| 2 | **Alignment** | `buf` at an odd address (Ethernet header is 14 bytes, so the IP header starts at offset 14 — not 4-byte aligned unless the driver offsets by 2). Unaligned 16/32-bit loads fault on some MCUs and are slow on others |
| 3 | **Strict aliasing** | Reading `uint8_t`/`char` storage as `uint16_t`/`uint32_t` is undefined behaviour; the optimiser may reorder or drop it |
| 4 | **Padding / bit-field layout** | The compiler may insert padding; bit-field order (MSB-first vs LSB-first) is **implementation-defined** — `uint8_t version:2, padding:1,...` is not portable. `#pragma pack` fixes padding only |
| 5 | **No bounds check** | The struct overlay trusts that 12 bytes exist. A 7-byte datagram makes `h->ssrc` an over-read |
| 6 | **Variable length** | CSRC list and extension have lengths decided *by fields inside the header*; a fixed struct cannot express "12 + 4·CC bytes" |

**The fix is simple and boring:** read the bytes into **local variables** with explicit big-endian loads and bit extraction; copy into your own struct that the compiler may lay out any way it likes.

> **Misconceptions box 3.1**
> 1. *"`#pragma pack(1)` makes the cast safe."* It removes padding; byte order, aliasing, bounds and bit-fields remain.
> 2. *"The cast is faster."* Compilers turn shift-loads into a single load (+ `bswap`) on supporting hardware; and unaligned-access faults cost far more than they save.
> 3. *"`memcpy` into the struct is the same as casting."* `memcpy` fixes 2 and 3, not 1, 4, 5, 6.
> 4. *"It works in my tests on x86."* x86 forgives misalignment; the first big-endian or Cortex-M0 port breaks it.

### 3.2 MSB-first `BitReader` / `BitWriter`

**Why MSB-first?** Protocol diagrams list fields from the left, most-significant bit first. Reading fields in that order means "take the next *n* bits from the left."

**Concrete first.** Bytes `29 35` = `0010 1001 | 0011 0101`. Read 12 bits then 4 bits:

```
bit index p :  0 1 2 3 | 4 5 6 7 | 8 9 10 11 | 12 13 14 15
bits        :  0 0 1 0 | 1 0 0 1 | 0 0 1  1  | 0  1  0  1
first 12    :  0010 1001 0011  = 0x293
next  4     :  0101            = 5
```

**Derive "where is bit p?"** Counting from the left, bit index *p* lives in byte `p / 8 = p >> 3`. Within that byte, index `p mod 8 = p & 7` is counted from the left, but C++ bits are counted from the right, so the C++ bit number is `7 − (p & 7)`. Hence

```
bit(p) = (data[p >> 3] >> (7 - (p & 7))) & 1
```
Check p = 4 in `0x29 = 0010 1001`: `7 − 4 = 3`; `0x29 >> 3 = 0000 0101`; `& 1 = 1` ✓ (the 5th bit from the left is 1).

**Reader algorithm.** For `read(n)`: if `n > 32` or fewer than `n` bits remain → return `false` and **consume nothing**. Otherwise `v = 0; repeat n times: v = (v << 1) | bit(p); ++p`. Each new bit enters at the bottom and earlier bits shift up — so the *first* bit read ends up most significant, as it should. (This is the same shift/mask idea as Pack 01 `extract_field`, just walking the stream.)

**Writer algorithm.** For `write(value, n)`: for `i = n−1` down to 0: take bit `(value >> i) & 1`; if the position is at a byte boundary, push a new zero byte; OR the bit into `7 − (pos & 7)` of the last byte. Higher bits of `value` than `n` are never touched (clipped for free — compare Pack 01 §3.4).

**Failure modes.**
| Mistake | Effect |
|---|---|
| LSB-first (`bit = (byte >> (p&7)) & 1`) | Every field comes out bit-reversed within each byte: `0x29` reads as `1001 0100` |
| Not checking `n` vs remaining | Over-read; ASan error |
| `n == 32` with `(1u << n) - 1` mask | UB (Pack 01 §3.3.6); the loop version avoids it |
| Failed read that still advances | Caller cannot recover or retry; sticky-error is cleaner, but be consistent |
| Reading a 33-bit field | Cannot fit in `uint32_t`; reject |

> **Misconceptions box 3.2**
> 1. *"MSB-first means big-endian."* Different axes: byte order (Pack 02) vs bit order inside the stream. Network protocols are MSB-first **and** big-endian; some radio air interfaces are MSB-first per *bit on the air* with their own packing rules.
> 2. *"I can read a 12-bit field as `*(uint16_t*)p >> 4`."* Wrong order (LE), wrong alignment, and the field may straddle three bytes.
> 3. *"A slow bit-at-a-time loop is unacceptable."* For headers (tens of bits) it's invisible; optimise only if profiling says so. The version with mask+shift per byte is a later step.

### 3.3 The parsing discipline (five rules)

1. **Check `len` before you read.** `if (len < 12) return false;` — *then* read.
2. **Validate invariants** you can check: `version == 2` (RTP), `version == 4` and `ihl >= 5` (IPv4).
3. **Never trust a length-like field.** Recompute what it implies and compare with what you have: `header_len = 12 + 4·CC` must be `<= len`; UDP `length` must satisfy `8 <= length <= len`; IPv4 `total_length` must satisfy `ihl*4 <= total_length <= len`.
4. **Beware unsigned underflow.** `size_t payload = udp_length - 8;` with `udp_length = 3` becomes 18446744073709551611 → a huge `memcpy`. Check *before* subtracting.
5. **Layered bounds.** Each layer passes the next layer *only the bytes it claims*: IPv4 hands UDP `total_length − header_len` bytes; UDP hands RTP `length − 8`. Trailing Ethernet padding (frames are padded to a 46-byte minimum payload) is **not** payload.

Return a boolean (or `std::optional`) rather than throwing — gateway parse failures are *expected* traffic, not exceptional events, and embedded builds often disable exceptions.

**Test with garbage.** Feed 2,000 random buffers of length 0–63 to every parser under AddressSanitizer — any over-read is an immediate report (our `Chain.DoesNotReadPastBufferOnRandomGarbage` test).

> **Misconceptions box 3.3**
> 1. *"The sender is our own gateway, so lengths are right."* Bugs, corruption (UDP checksum may be off), and attackers exist. Treat every packet as hostile.
> 2. *"If `length` is too big I'll just clamp it."* Clamping silently accepts malformed packets; rejecting is usually correct (document the choice).
> 3. *"`assert` is validation."* Asserts vanish in release builds.

### 3.4 RTP fixed header — field by field (RFC 3550 §5.1)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            contributing source (CSRC) identifiers (0..15)      |
```
Sample: `80 80 00 01 00 00 00 A0 12 34 56 78`.

| Field | Bits | Sample | **Why it exists** (the problem it solves) |
|---|---|---|---|
| **V** version | 2 | `10` = 2 | Lets receivers reject packets from other formats; 2 is the current version. Two bits leave room for a rare future change. *Byte 0 `0x80`: V=`10`.* |
| **P** padding | 1 | 0 | Payload padded to a block-size multiple (e.g., block ciphers); the **last octet** is the count of padding octets *including itself*. |
| **X** extension | 1 | 0 | One header extension follows the CSRC list: 16-bit profile id, 16-bit length (in 32-bit words, excluding those 4 bytes), then data. (One-byte/two-byte forms: RFC 8285 — VERIFY.) |
| **CC** CSRC count | 4 | 0 | How many 32-bit CSRC ids follow (0–15, 4 bits). |
| **M** marker | 1 | 1 | Profile-defined. For audio, **first packet of a talkspurt** — in LMR, *the start of a PTT transmission*. Use it to reset the jitter buffer/playout delay at talkspurt starts (RFC 3551 §4.1). |
| **PT** payload type | 7 | 0 (PCMU) | What the payload *is*: e.g., 0 = PCMU (G.711 µ-law), 8 = PCMA (A-law) (RFC 3551 Table 4); 96–127 dynamic, mapped by signalling. |
| **seq** | 16 | 1 | +1 per packet: **loss detection and reorder**; random start (RFC 3550 §5.1); wraps (Pack 02 §3.7). |
| **timestamp** | 32 | 160 | Sampling-instant clock for **playout timing and jitter** (not wall-clock). For 8 kHz audio, 20 ms = **160 ticks**. Silence suppression ⇒ timestamps jump while seq stays +1; so seq ≠ timestamp. |
| **SSRC** | 32 | `0x12345678` | Identifies the **stream's source**, independent of network address (two radios behind one gateway IP). Chosen randomly; collisions are detected and resolved (RFC 3550 §8). |
| **CSRC** list | 0–15 × 32 | none | When a **mixer/bridge** combines streams (a gateway mixing talkgroup audio), lists the original sources. |

**Derive `header_len`:** `12 + 4·CC`, plus (if X) `4 + 4·ext_words`. Then the **payload** is `len − header_len − pad`, where `pad` = last byte if P is set (must be ≥ 1 and ≤ `len − header_len`).

Byte 0 by hand: `0x80 = 1000 0000` → V = top 2 bits `10` = 2; P = next bit 0; X = 0; CC = low nibble 0. Byte 1: `0x80 = 1000 0000` → M = top bit 1; PT = low 7 bits `000 0000` = 0.

> **Misconceptions box 3.4**
> 1. *"Marker means 'last packet'."* In RTP audio it's the *first* of a talkspurt (profile-defined; video uses it for the *last* packet of a frame).
> 2. *"timestamp increments by 1 per packet."* It advances by the number of *sampling ticks* in the packet (160 per 20 ms at 8 kHz).
> 3. *"SSRC is the sender's IP."* It's an independent random identity; one IP may send many SSRCs.
> 4. *"Padding count is in the header."* It's the **last byte of the packet**.

### 3.5 IPv4 header — field by field (RFC 791 §3.1)

Synthetic-but-classic example (this exact header is used in the Wikipedia checksum example — VERIFY there): 

`45 00 00 73  00 00 40 00  40 11 b8 61  c0 a8 00 01  c0 a8 00 c7`

| Word | Field | Bits | Value | **Why it exists** |
|---|---|---|---|---|
| 0 | **Version** | 4 | `4` (high nibble of `0x45`) | Distinguish IPv4 vs IPv6 on the same link |
| 0 | **IHL** | 4 | `5` (low nibble) | Header length in **32-bit words**. Words, not bytes: 4 bits can count 0–15 → 15 × 4 = 60 bytes max, minimum 5 → 20 bytes. Tells you where the payload starts when **options** exist |
| 0 | **DSCP** | 6 | `0` | Differentiated services/QoS class (RFC 2474) — e.g., EF (46) for voice |
| 0 | **ECN** | 2 | `0` | Congestion notification (RFC 3168) |
| 0 | **Total length** | 16 | `0x0073` = 115 | Whole datagram in bytes. Lets the receiver **ignore link-layer padding**; 16 bits ⇒ max 65,535 |
| 1 | **Identification** | 16 | `0` | Groups fragments of one original datagram |
| 1 | **Flags** | 3 | `010` | bit 0 reserved(0); **DF** (don't fragment) = 1; **MF** (more fragments) = 0 |
| 1 | **Fragment offset** | 13 | 0 | Where this fragment's data starts, in **8-byte units** |
| 2 | **TTL** | 8 | `0x40` = 64 | Hop limit; decremented per router; prevents loops |
| 2 | **Protocol** | 8 | `0x11` = 17 | Next layer (17 = UDP, 6 = TCP, 1 = ICMP) — a **demultiplexer** |
| 2 | **Header checksum** | 16 | `0xB861` | Detects header corruption; covers **header only** (Pack 04 §3.8) |
| 3 | **Source** | 32 | `C0A80001` = 192.168.0.1 | |
| 4 | **Destination** | 32 | `C0A800C7` = 192.168.0.199 (`0xC7` = 199) | |

**Why the fragment offset is in 8-byte units (derive):** we must address any byte in a datagram up to 65,535 bytes but have only 13 bits (0–8191). 8191 × 8 = 65,528 ≈ 65,535; so counting in 8-byte blocks covers the range. Consequence: every fragment except the last carries a multiple of 8 data bytes. Example: offset field `0x00B9` = 185 → 185 × 8 = **1480** bytes — the usual second-fragment start for a 1500-byte MTU (1500 − 20 header = 1480).

Flags+offset 16-bit word `0x4000` = `0100 0000 0000 0000`: top bit reserved 0, **DF = 1**, MF = 0, offset 0. `0x20B9` = `0010 0000 1011 1001`: reserved 0, DF 0, **MF 1**, offset `0 0000 1011 1001` = 185.

**Validate:** `len ≥ 20`; version 4; `ihl ≥ 5`; `len ≥ ihl·4`; `ihl·4 ≤ total_length ≤ len`. Trailing bytes after `total_length` are link padding, not data.

> **Misconceptions box 3.5**
> 1. *"IHL is in bytes."* Words (×4). `IHL = 5` means 20 bytes.
> 2. *"Total length = what the socket gave me."* Receive length may exceed it (padding) or be less (truncated capture).
> 3. *"The fragment offset is a byte offset."* ×8.
> 4. *"The IP checksum covers the payload."* Header only; UDP/TCP carry their own.
> 5. *"Fragments can be parsed like whole datagrams."* Only the first fragment has the UDP header; later ones are raw bytes — our chain parser rejects fragments.

### 3.6 UDP header (RFC 768)

`9C 40 | 13 8C | 00 14 | 00 00` → src port 40000, dst port 5004, length 20, checksum 0.

| Field | Why it exists |
|---|---|
| **Source port** | Return address for replies / flow identification |
| **Destination port** | Demultiplexes to the application (5004 is a commonly *suggested* RTP port; RTP port assignment is by signalling) |
| **Length** | Header + payload in bytes, **minimum 8**. Redundant with IP total length — which is why it must be **cross-checked**, not trusted |
| **Checksum** | Optional in IPv4 (`0` = not computed), **mandatory in IPv6** (RFC 8200). Covers a pseudo-header (src/dst IP, protocol, length) + UDP header + data (Pack 04 §3.8) |

Payload length = `length − 8` **only after** checking `length ≥ 8`.

> **Misconceptions box 3.6**
> 1. *"Checksum 0 means checksum failed."* In IPv4 it means "none".
> 2. *"UDP length is the buffer length."* It's what the *sender* claims.

### 3.7 Building packets (the mirror image)

`BitWriter` mirrors `BitReader`: write fields in diagram order. RTP: `write(V,2) write(P,1) write(X,1) write(CC,4) write(M,1) write(PT,7) write(seq,16) write(ts,32) write(ssrc,32)` → 12 bytes. **Round-trip property:** `build(parse(bytes)) == bytes` for valid inputs, and `parse(build(h)) == h`. Property tests like this catch field-width typos better than examples do.

---

## §4 Fully worked example — a 40-byte IPv4/UDP/RTP packet (synthetic)

```
offset 0   45 00 00 28 00 00 40 00 40 11 00 00 0A 00 00 01 0A 00 00 02     IPv4 (20 B)
offset 20  9C 40 13 8C 00 14 00 00                                          UDP  (8 B)
offset 28  80 80 00 01 00 00 00 A0 12 34 56 78                              RTP  (12 B)
```
(10.0.0.1 → 10.0.0.2, private addresses, RFC 1918. IP checksum left `0000` here; Pack 04 computes real ones.)

**Layer 1, IPv4.** `len = 40`. `0x45` → version 4, IHL 5 → `header_len = 20`. Total length `0x0028 = 40`: check `20 ≤ 40 ≤ 40` ✓. Flags/offset `0x4000` → DF, offset 0 → not a fragment ✓. Protocol `0x11 = 17` → UDP ✓. IP payload = `40 − 20 = 20` bytes at offset 20.
**Layer 2, UDP** (given 20 bytes). Ports 40000 → 5004. Length `0x0014 = 20`; check `8 ≤ 20 ≤ 20` ✓. UDP payload = `20 − 8 = 12` bytes at offset 28.
**Layer 3, RTP** (given 12 bytes). `len ≥ 12` ✓. Byte 0 `0x80` → V=2 ✓, P=0, X=0, CC=0 → `header_len = 12`. Byte 1 `0x80` → M=1, PT=0 (PCMU). seq `00 01` = 1. Timestamp `00 00 00 A0` = 160. SSRC `0x12345678`. Payload = `12 − 12 = 0` bytes (a header-only packet, fine for a test).

**What if we hand it garbage?**
1. UDP length changed to `0x0030` (48): layer 2 check `48 ≤ 20` fails → reject. (Never read the 28 phantom bytes.)
2. RTP byte 0 = `0x8F` (CC = 15): `header_len = 12 + 60 = 72 > 12` → reject.
3. Frame padded to 46 bytes with zeros: IPv4 `total_length` still 40 → UDP and RTP see exactly their bytes; padding ignored.
4. Byte 0 = `0x40` in RTP (version 1) → reject.

---

## §5 Questions

Each has a **type**, a **context line**, and an answer in `answers.md`. Do them on paper first.

**Q1 — Explain.** *Context: gateway code review.* List at least five distinct reasons not to `reinterpret_cast` a received buffer to a header struct, each with a concrete failure on the §3.1 bytes.

**Q2 — Calculate.** *Context: RTP stream triage.* Decode by hand (binary, then fields) the first two bytes of: (a) `80 08`, (b) `93 E0`, (c) `A0 60`. Give V, P, X, CC, M, PT.

**Q3 — Trace.** *Context: IPv4 first word.* Run a `BitReader` over bytes `45 B9`: `read(4), read(4), read(6), read(2)`. Show bit index and value for each step.

**Q4 — Trace.** *Context: building an RTP header.* Run a `BitWriter`: `write(2,2) write(0,1) write(1,1) write(3,4)`. What byte results? (Compare with Q2(b).) Then `write(0x1FF, 4)`: what happens to the extra bits?

**Q5 — Calculate.** *Context: RTP packet of length 200 bytes.* CC = 3, X = 1 with extension length field = 2 words, P = 1 with last byte 4. Give `header_len`, padding, `payload_len`. Is that a plausible G.711 20 ms payload?

**Q6 — Calculate.** *Context: IPv4 decode.* Decode every field of `45 00 00 73 00 00 40 00 40 11 b8 61 c0 a8 00 01 c0 a8 00 c7` (dotted addresses too). Why is this packet not parseable as-is with `len = 20`?

**Q7 — Calculate.** *Context: fragmentation.* A fragment has flags+offset word `0x20B9`. Which flags are set, what is the offset in bytes, and what is the *next* fragment's offset field if this fragment carries 1480 bytes of data?

**Q8 — Find the bug.** *Context: legacy parser in a gateway.*
```cpp
struct RtpHdr { uint8_t v:2, p:1, x:1, cc:4; uint8_t m:1, pt:7; uint16_t seq; uint32_t ts, ssrc; };
void on_packet(const uint8_t* buf, size_t len) {
  auto* h = reinterpret_cast<const RtpHdr*>(buf);
  if (h->v != 2) return;
  play(buf + 12, len - 12, h->seq, h->ts);
}
```
List all bugs (≥ 6) with a failing input each.

**Q9 — Find the bug.** *Context: UDP receive path.*
```cpp
bool parse_udp(const uint8_t* d, size_t len, UdpHeader& h) {
  h.src_port = (d[0] << 8) | d[1];
  h.dst_port = (d[2] << 8) | d[3];
  h.length   = (d[4] << 8) | d[5];
  size_t payload = h.length - 8;
  deliver(d + 8, payload);
  return true;
}
```
Find every flaw; give inputs; write the fixed version.

**Q10 — Predict.** *Context: parser behaviour on edge cases.* For each input say accept/reject and why: (a) RTP, 12 bytes, byte0 `0x8F`; (b) IPv4 header OK, `total_length = len + 1`; (c) UDP length field = 8, buffer 8 bytes; (d) IPv4, IHL = 4; (e) RTP `0xA0…` 12 bytes, last byte `0x01`.

**Q11 — Explain.** *Context: why the format is what it is.* For each RTP field (seq, timestamp, SSRC, marker, CC/CSRC) name the real-world problem it solves for an LMR voice gateway. Why does `timestamp` jump while `seq` doesn't when the radio pauses (silence suppression)?

**Q12 — Explain.** *Context: reading RFC 791.* Why is IHL in 32-bit words? Why is the fragment offset in 8-byte units? Why is the IP checksum header-only?

**Q13 — Implement.** *Context: gateway library.* Implement every TODO in `code/include/pktheaders.h` (BitReader, BitWriter, RTP, IPv4, UDP, chain). All `student.*` tests green; ASan/UBSan silent.

**Q14 — Connect.** *Context: whole module.* (a) Where does Pack 01's `extract_field` reappear here? (b) Where does Pack 04's Internet checksum plug into IPv4/UDP? (c) Which pack's code would you call after parsing a P25 NID to decide whether it needs correcting?

**Q15 — Interview.** *Context: phone screen.* "Given a `const uint8_t*` and a length, parse an IPv4 header. What do you validate, in what order, and what do you return?"

**Q16 — Interview.** *Context: senior design question.* "How do you fuzz/test a binary parser? How do you handle a protocol whose fields are big-endian bit-fields in C++ without UB?"

---

## §6 Self-assessment

- [ ] I can list six reasons the struct-cast is wrong and give a failing case for each.
- [ ] I can derive `bit(p) = (data[p>>3] >> (7-(p&7))) & 1` and trace a 12+4-bit read.
- [ ] I can read an RFC header diagram and convert tick ranges to C++ shifts/masks.
- [ ] I can name the reason each RTP, IPv4 and UDP field exists.
- [ ] I can state the five parsing rules and show how unsigned underflow bites (`length − 8`).
- [ ] I can compute `header_len`/`payload_len` with CSRC, extension and padding.
- [ ] My parser passes all `solution.*` tests and, after Q13, all `student.*` tests, under ASan/UBSan.

**Rapid fire (key in `answers.md`):** (1) `0x45` → version, IHL? (2) `0x2000` flags/offset → ? (3) RTP `0x90` → X? (4) 8 kHz, 30 ms → timestamp step? (5) UDP length 8 means?

**VERIFY list:** RFC 3550 §5.1 (header, padding, CSRC, extension, SSRC §8), RFC 3551 (PT table, marker semantics §4.1), RFC 8285 (extension forms), RFC 791 §3.1 (IHL, flags, offset), RFC 768, RFC 2474/3168, RFC 8200 (UDP checksum mandatory in IPv6), RFC 1918; the Wikipedia IPv4 checksum example bytes; Heartbleed description.
