# Pack 03 — Answers (parsing headers safely)

Each answer: **reasoning** → **common wrong answers** (*why tempting* / *why wrong*). `§` = [`worksheet.md`](worksheet.md) section. All data synthetic.

---

## Q1 — Why not `reinterpret_cast` (Explain)

On bytes `80 80 00 01 00 00 00 A0 12 34 56 78`:
1. **Endianness:** LE host reads `seq` as `0x0100 = 256`, `ts` as `0xA0000000`.
2. **Alignment:** if `buf` is odd-aligned (e.g. IP header at offset 14 of an Ethernet frame), a 32-bit load at `buf+4` is unaligned: bus fault on Cortex-M0 / some DSPs, slow on others.
3. **Strict aliasing:** accessing `uint8_t` storage as `uint16_t`/`uint32_t` is UB; the optimiser may reorder/eliminate the loads.
4. **Padding & bit-field layout:** compiler padding and implementation-defined bit-field allocation (GCC little-endian packs the *first* declared bit-field into the *low* bits, the reverse of the RFC diagram), so `version:2` may read bits 1..0 of byte 0 instead of bits 7..6.
5. **No bounds check:** a 7-byte datagram makes `ssrc` an over-read.
6. **Variable length:** CSRC list (`4·CC`) and extension are not expressible as a fixed struct; the payload offset is *computed*.
7. (Bonus) `sizeof` not guaranteed 12 across ABIs; no validation hook for `version`.

**Common wrong answers.**
- *"Just add `#pragma pack(1)`."* — Tempting: it removes the padding surprise, the most visible symptom. Wrong: leaves 1, 2 (even more unaligned), 3, 4, 5, 6.
- *"Use `memcpy` into the struct."* — Tempting: fixes aliasing/alignment, the "UB" reasons. Wrong: still byte order, bit-fields, bounds and variable length.
- *"It works on x86 so it's fine."* — Tempting: tests pass. Wrong: x86 forgives alignment; byte order is *still* wrong on x86 (it's LE).

---

## Q2 — Decode RTP first two bytes (Calculate)

- (a) `80 08`: `0x80 = 1000 0000` → V = `10` = 2, P = 0, X = 0, CC = 0000 = 0. `0x08 = 0000 1000` → M = 0, PT = `000 1000` = **8** (PCMA).
- (b) `93 E0`: `0x93 = 1001 0011` → V = 2, P = 0, **X = 1**, **CC = 3**. `0xE0 = 1110 0000` → **M = 1**, PT = `110 0000` = 0x60 = **96** (dynamic).
- (c) `A0 60`: `0xA0 = 1010 0000` → V = 2, **P = 1**, X = 0, CC = 0. `0x60 = 0110 0000` → M = 0, PT = 96.

**Common wrong answers.**
- *(a) PT = 0x08 and M = 1 because "0x80 is the marker".* — Tempting: confusing byte 0 and byte 1. Wrong: M is the **top bit of byte 1**; byte 0 holds V/P/X/CC.
- *(b) CC = 9 (reading the high nibble).* — Tempting: hex digits read left to right, "first digit = first field". Wrong: the high nibble `9 = 1001` packs V, P, X; CC is the **low** nibble, 3.
- *(c) Version = 5 (`0xA0 >> 5`).* — Tempting: shifting by the wrong amount. Wrong: version is the top **2** bits: `0xA0 >> 6 = 2`.

---

## Q3 — BitReader trace on `45 B9` (Trace)

Bits: `0100 0101 1011 1001`.
| call | start bit `p` | bits | value |
|---|---|---|---|
| `read(4)` | 0 | `0100` | 4 (version) |
| `read(4)` | 4 | `0101` | 5 (IHL) |
| `read(6)` | 8 | `101110` | 46 = 0x2E (DSCP, EF) |
| `read(2)` | 14 | `01` | 1 (ECN) |
After: `p = 16`, `bits_remaining = 0`.

**Common wrong answers.**
- *`read(6)` = 0x39 (`0xB9 & 0x3F`).* — Tempting: taking the *low* 6 bits (C++ habit). Wrong: MSB-first reads the **top** 6 bits first.
- *`read(2)` = 2.* — Reading bits 14–15 of `1011 1001` wrong end: the last two bits are `01`.

---

## Q4 — BitWriter trace (Trace)

`write(2,2)` → `10`; `write(0,1)` → `0`; `write(1,1)` → `1`; `write(3,4)` → `0011`. Concatenated: `1001 0011` = **0x93** (matches Q2(b)'s byte 0: V=2, P=0, X=1, CC=3).
`write(0x1FF, 4)`: only the low 4 bits `1111` are written; bits above the width are ignored (clipped), not spilled into the next field.

**Common wrong answers.**
- *`0x39` (fields placed LSB-first).* — Tempting: building the byte "from the bottom". Wrong: protocol bytes list V first at the *top* bits.
- *`write(0x1FF,4)` writes 9 bits.* — Wrong: width is the contract; the extra bit would corrupt the next field (Pack 01 §3.4).

---

## Q5 — RTP lengths (Calculate)

`header_len = 12 + 4·3 (CSRC) = 24`; extension: `4 + 4·2 = 12` → `header_len = 36`. Padding `pad = 4` (last byte) → `payload_len = 200 − 36 − 4 = 160`. Plausible: G.711 PCMU/PCMA is 1 byte/sample, 8,000 samples/s × 0.020 s = **160** bytes ✓.

**Common wrong answers.**
- *`payload = 200 − 12 = 188`.* — Tempting: the "12-byte header" mantra. Wrong: ignores CSRC, extension, padding.
- *Extension is 2 bytes (`ext_words` taken as bytes).* — Tempting: length in familiar units. Wrong: the length counts **32-bit words**, excluding the 4-byte extension header: `4 + 4·2 = 12`.

---

## Q6 — Decode the IPv4 header (Calculate)

| Field | Bytes | Value |
|---|---|---|
| Version/IHL | `45` | 4 / 5 (20 bytes) |
| DSCP/ECN | `00` | 0 / 0 |
| Total length | `00 73` | 115 |
| ID | `00 00` | 0 |
| Flags/offset | `40 00` | `010`: DF=1, MF=0; offset 0 |
| TTL | `40` | 64 |
| Protocol | `11` | 17 (UDP) |
| Checksum | `b8 61` | 0xB861 |
| Src | `c0 a8 00 01` | 192.168.0.1 |
| Dst | `c0 a8 00 c7` | 192.168.0.199 |

With `len = 20` the parser sees `total_length = 115 > len` → the datagram is **truncated** (a capture of only the header) → reject. You need ≥ 115 bytes (our `wiki_ipv4_packet()` test pads to 115).

**Common wrong answers.**
- *Dst = 192.168.0.199 written as `0xC7 = 119`.* — Hex slip: `0xC7 = 12·16 + 7 = 199`.
- *"It parses fine; the header is all I need."* — Tempting for header-only tools (you *could* build a mode that accepts truncation, deliberately). Wrong for the default: you'd be trusting a length you can't bound.
- *Flags = `0x40`.* — The flags are the top 3 bits of the 16-bit word: `0x4000 >> 13 = 2` = `010`.

---

## Q7 — Fragment word `0x20B9` (Calculate)

`0x20B9 = 0010 0000 1011 1001`: top three bits `001` → reserved 0, DF 0, **MF 1**. Offset = low 13 bits = `0 0000 1011 1001` = 0xB9 = **185** → 185 × 8 = **1480** bytes. If this fragment carries 1480 data bytes the next starts at 1480 + 1480 = 2960 bytes = **370** in offset units (185 + 185).

**Common wrong answers.**
- *Offset = 0x20B9 = 8377.* — Forgetting the 3 flag bits sit above the 13-bit field.
- *Next offset = 185 + 1480 = 1665.* — Mixing units: the field counts 8-byte blocks, so add `1480/8 = 185`.

---

## Q8 — Legacy parser bugs (Find the bug)

1. **Bit-field order** is implementation-defined; on GCC/LE, `v:2` occupies the *low* bits of byte 0 → for `0x80` reads `0`, so valid packets are rejected (or version 0 accepted).
2. **No length check** before `h->v`: `len == 0` over-reads.
3. **`len - 12` underflow** for `len < 12` → huge length passed to `play`.
4. **`h->seq`, `h->ts` not converted** from network order.
5. **Alignment** of the cast (odd `buf`) and **6. strict aliasing**.
7. **CC ignored:** payload starts at `12 + 4·cc`, not 12; **8. X ignored**; **9. P ignored** — padding is played as audio.
10. `sizeof(RtpHdr)` / bit-field packing not guaranteed 12.
Failing inputs: (2) `len=0`; (3) `len=5`; (4) seq `00 01`; (7) `0x82…` with 8 CSRC bytes: first 8 payload bytes are CSRC ids.

**Common wrong answers.** *"Only the endian bug."* — Tempting: the classic. Wrong: the length-underflow is the *exploitable* one.

---

## Q9 — `parse_udp` flaws (Find the bug)

1. No `len >= 8` check: for `len < 6`, `d[4]`/`d[5]` are over-reads.
2. `h.length - 8`: `h.length` is `uint16_t` → promoted to `int` → `3 - 8 = −5` → converted to `size_t` = 2⁶⁴ − 5 → enormous payload length.
3. No `h.length <= len` check: `deliver(d + 8, payload)` reads past the buffer when the sender lies (e.g. length 1000, buffer 40).
4. Returns `true` for any input; no validation at all.
Fixed:
```cpp
bool parse_udp(const uint8_t* d, size_t len, UdpHeader& h) {
  if (len < 8) return false;
  uint16_t length = static_cast<uint16_t>((d[4] << 8) | d[5]);
  if (length < 8 || length > len) return false;
  h = { uint16_t((d[0]<<8)|d[1]), uint16_t((d[2]<<8)|d[3]), length, uint16_t((d[6]<<8)|d[7]) };
  return true;                     // payload = d+8, length-8 bytes: now safe
}
```

**Common wrong answers.** *"`(d[0] << 8)` overflows."* — Tempting after Pack 02's warning. Wrong here: `d[0]` is promoted to `int` and `0xFF << 8 = 0xFF00` fits easily; the dangerous promoted shift is `<< 24`.

---

## Q10 — Edge cases (Predict)

- (a) RTP `0x8F…`, 12 bytes: CC = 15 → `header_len = 72 > 12` → **reject**.
- (b) IPv4 `total_length = len + 1` → **reject** (truncated).
- (c) UDP `length = 8`, buffer 8 → **accept**; payload 0 (legal, empty datagram).
- (d) IHL = 4 → **reject** (`< 5` is below the 20-byte minimum).
- (e) `0xA0…`, 12 bytes, last byte `0x01`: P=1, pad = 1; `header_len + pad = 13 > 12` → **reject**.

**Common wrong answers.**
- *(c) reject because "no payload".* — Tempting: empty feels invalid. Wrong: UDP allows zero-length payloads.
- *(e) accept: pad = 1 so payload_len = 11.* — Tempting: arithmetic `12 − 1`. Wrong: padding bytes must come *after* the header; here the "padding byte" would be inside the header.

---

## Q11 — Why RTP fields exist (Explain)

- **seq:** UDP doesn't order or guarantee delivery; seq gives **loss counting and reorder** (Pack 02 §3.7).
- **timestamp:** the **playout clock**: a jitter buffer re-times audio from network arrival jitter to a steady schedule; increments by sample ticks (160 per 20 ms at 8 kHz).
- **SSRC:** identifies a **stream**, not an address: a gateway behind one IP carrying several radios' audio keeps them apart; supports mixer/translator logic.
- **marker:** talkspurt start = **PTT start** in LMR; the jitter buffer can safely re-adapt playout delay at a talkspurt boundary without glitching mid-word.
- **CC/CSRC:** when a bridge mixes multiple sources (talkgroup audio), CSRCs list the contributors (who is speaking).
**Why timestamp jumps but seq doesn't:** with silence suppression/PTT gaps, no packets are sent during silence; the next packet's timestamp advances by the *elapsed sampling time* (e.g. +4000 ticks = 0.5 s) while seq increases by 1 (one packet sent). Using seq to infer time would shrink silence; using timestamp to detect loss would report false loss.

**Common wrong answers.**
- *"Timestamp is wall-clock time."* — Tempting: the name. Wrong: it's a sampling-clock counter with a random start offset (RFC 3550 §5.1).
- *"seq gaps after silence mean packet loss."* — Wrong: loss shows as a seq gap; silence shows as a timestamp jump with contiguous seq.

---

## Q12 — Why those units (Explain)

- **IHL in words:** the field is 4 bits (0–15). Counting bytes would max out at 15 bytes (< 20-byte minimum!). Counting 4-byte words gives a 60-byte maximum header, and 5 words = 20 bytes minimum.
- **Fragment offset ×8:** 13 bits (0–8191) must reach any offset in a ≤ 65,535-byte datagram; 8191 × 8 ≈ 65,528, so each unit must be 8 bytes. Fragments (but the last) thus carry multiples of 8 bytes.
- **Header-only checksum:** routers decrement TTL and recompute it at **every hop** — checksumming only 20–60 bytes is cheap (an important constraint in 1981 hardware). Payload integrity is the transport protocol's job (UDP/TCP checksum) and the link layer's (CRC, Pack 04).

**Common wrong answers.** *"IHL is in bytes, max 15."* — Tempting: bytes are the usual unit. Wrong: then IHL=5 would mean 5 bytes < 20 minimum.

---

## Q13 — Implement (Implement)

Reference: `code/solutions/pktheaders.h`. Frequent failures the tests catch:
- `BitReader.FailedReadConsumesNothing` — advanced `pos_` before checking.
- `BitReader.FirstBitIsMostSignificant` — LSB-first bit extraction.
- `Rtp.ExtensionLengthLyingIsRejected` — trusted the extension length.
- `Rtp.BadPaddingCountRejected` — padding 0 or too large.
- `Chain.IgnoresTrailingEthernetPadding` — used buffer length rather than IP `total_length`.
- `Chain.DoesNotReadPastBufferOnRandomGarbage` — ASan fires on any over-read.

**Common wrong answers.** *"Parsing in one pass with `memcpy(&hdr, buf, sizeof hdr)`."* — Fails Q1 items 1, 4, 6.

---

## Q14 — Connections (Connect)

**(a)** `BitReader::read` is Pack 01's `extract_field` applied along a byte stream (shift to align, mask to width, repeated per bit/byte); `BitWriter` is `insert_field`.
**(b)** The IPv4 header checksum (Pack 04 §3.8) is computed over the header with the checksum field zeroed; UDP's covers a pseudo-header + UDP header + data. A gateway that rewrites an IP address/TTL must update (or recompute) these (RFC 1624 incremental update — VERIFY).
**(c)** Pack 05: after extracting NAC/DUID (Pack 01) from a P25 NID, the BCH parity (Pack 05 §3.5) must be checked/corrected *before* trusting the fields.

**Common wrong answers.** *"Checksums are only for the receiver."* — A rewriting gateway also *produces* them.

---

## Q15 — Interview: parse an IPv4 header (Interview)

Order: (1) `data != nullptr && len >= 20`; (2) read version/IHL, reject `version != 4`, `ihl < 5`; (3) `hl = ihl*4`, require `len >= hl`; (4) read `total_length`, require `hl <= total_length <= len`; (5) read the rest; optionally verify the checksum; (6) treat bytes beyond `total_length` as link padding; (7) mark/handle fragments (`MF` or `offset != 0`); (8) return `bool`/`std::optional` and keep the parse **side-effect free** until fully validated (parse into a local, assign on success). Mention endianness (explicit BE loads), no struct casting, and test with truncated/oversized lengths.

**Common wrong answers.** *"Check `total_length == len`."* — Tempting: strict. Wrong in practice: Ethernet padding makes `len > total_length` legitimate.

---

## Q16 — Interview: fuzzing and bit-fields (Interview)

**Testing:** (1) boundary tests for every length (0, 1, k−1, k, k+1); (2) property tests `parse(build(h)) == h`; (3) mutation/fuzzing (libFuzzer or AFL++) with ASan+UBSan on the parser entry points; (4) a corpus of real-world-shaped *synthetic* packets plus malformed ones; (5) differential testing against an independent dissector; (6) mutate length fields specifically (`0`, `7`, `8`, `len`, `len+1`, `0xFFFF`).
**Bit-fields:** don't use C++ bit-fields for wire formats (layout implementation-defined). Use shifts/masks (or a `BitReader`), explicit big-endian loads, a local struct that you fill field by field. `std::bit_cast` helps for fixed-width numeric reinterpretation but does not solve byte order or bit-field order.

**Common wrong answers.** *"Static analysis is enough."* — Tempting for time. Wrong: parsers fail on inputs nobody imagined; fuzzing finds those.

---

## §6 Self-assessment key

(1) `0x45` → version 4, IHL 5 (20 bytes). (2) `0x2000` = `001` + offset 0 → **MF set**, offset 0 (first fragment). (3) `0x90 = 1001 0000` → X = 1. (4) 8,000 × 0.030 = **240** ticks. (5) UDP length 8 = header only, empty payload.

**VERIFY (this pack):** see list at end of worksheet (RFC 3550/3551/8285/791/768/2474/3168/8200/1918/1624, Wikipedia IPv4 example, Heartbleed).
