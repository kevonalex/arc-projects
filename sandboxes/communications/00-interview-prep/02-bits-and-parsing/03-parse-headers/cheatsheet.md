# Pack 03 — Cheatsheet (every line points to its derivation in `worksheet.md`)

Re-derive, don't memorise: open the cited section when a line isn't obvious.

| Fact | Where derived |
|---|---|
| RFC diagrams: bit 0 = leftmost = MSB; each row = one 32-bit word | W §2 |
| `size_t` underflow: `len - 8` with `len < 8` is huge | W §2, §3.3 |
| Never cast a buffer to a struct: byte order, alignment, aliasing, padding/bit-fields, bounds, variable length | W §3.1 |
| `#pragma pack` / `memcpy` fix only some of those | W §3.1 (box) |
| Bit `p` lives in byte `p>>3`, C++ bit `7-(p&7)`: `bit(p) = (d[p>>3] >> (7-(p&7))) & 1` | W §3.2 |
| `BitReader::read(n)`: `v=(v<<1)\|bit` n times; fail without consuming if `n>32` or short | W §3.2 |
| `BitWriter::write(v,n)`: bits from `n-1` down to 0; new byte at each boundary; clipped to width | W §3.2 |
| Five rules: check len first; validate invariants; never trust lengths; beware underflow; layered bounds | W §3.3 |
| Return `bool`/`optional`, parse into a local, assign on success | W §3.3 |
| Test parsers with random garbage under ASan | W §3.3 |
| RTP byte 0: `V:2 P:1 X:1 CC:4`; byte 1: `M:1 PT:7`; then seq16, ts32, ssrc32 | W §3.4 |
| RTP `header_len = 12 + 4·CC (+ 4 + 4·ext_words if X)`; payload `= len − header_len − pad` | W §3.4 |
| Padding: last octet = count including itself; must be ≥1 and ≤ bytes after header | W §3.4 |
| Marker = talkspurt (PTT) start in audio; timestamp = sampling ticks (160 per 20 ms @ 8 kHz); seq counts packets | W §3.4, Q11 |
| SSRC identifies a stream (not an IP); CSRC lists mixed sources | W §3.4 |
| IPv4 word 0: `Ver:4 IHL:4 DSCP:6 ECN:2 TotalLen:16`; IHL counts 32-bit words (min 5 = 20 B) | W §3.5 |
| IPv4 flags: reserved, DF, MF; fragment offset 13 bits in 8-byte units (`185 → 1480 B`) | W §3.5 |
| IPv4 validate: `ihl≥5`, `len≥ihl·4`, `ihl·4 ≤ total_length ≤ len`; bytes past total_length = padding | W §3.5, §4 |
| IPv4 checksum covers header only; protocol 17 = UDP | W §3.5 |
| UDP: `src16 dst16 len16 csum16`; `8 ≤ length ≤ len`; checksum 0 = none (IPv4), mandatory in IPv6 | W §3.6 |
| Layer bounds: IP hands UDP `total_length − hl`; UDP hands RTP `length − 8` | W §3.3, §4 |
| Builders mirror readers; test `parse(build(h)) == h` | W §3.7 |
| Chain parser rejects fragments (only first has UDP header) | W §3.5 (box), §4 |

**VERIFY:** RFC 3550 §5.1/§8, RFC 3551 PT table & marker, RFC 8285, RFC 791 §3.1, RFC 768, RFC 8200 (see W §3.4–§3.6, §6).
