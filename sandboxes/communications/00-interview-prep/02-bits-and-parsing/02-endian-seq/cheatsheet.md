# Pack 02 — Cheatsheet (every line points to its derivation in `worksheet.md`)

Re-derive, don't memorise: open the cited section if a line is not obvious.

| Fact | Where derived |
|---|---|
| Endianness = **byte** order of multi-byte values in memory, not bit order | W §3.1 |
| `0x11223344`: BE memory `11 22 33 44`, LE memory `44 33 22 11` | W §3.1 |
| UDP port 5004 = `0x138C`: BE `13 8C`, LE `8C 13` | W §3.1, §3.4 |
| LE: low byte first (carry-friendly hardware); BE: reads like hex dumps | W §3.2 |
| Names come from Cohen, IEN 137 (1980), after Gulliver's Travels — VERIFY | W §3.2 |
| Detect host order with `memcpy` of `uint16_t 1` (or C++20 `std::endian`) | W §3.2 |
| Network byte order = big-endian (RFC 791 App. B, Assigned Numbers) — VERIFY | W §3.3 |
| One wire convention → each host converts once (N conversions, not N²) | W §3.3 |
| `htons/htonl`: no-op on BE, swap on LE; `ntoh*` is the same swap | W §3.4 |
| `htons` result has the right *memory image*, not a meaningful number; don't convert twice | W §3.4 |
| `bswap16 = (v<<8)\|(v>>8)`; `bswap32` = four masked shifts, one per byte lane | W §3.5 |
| Shifts act on values, so shift-built loads/stores work on any host | W §3.6 |
| `load_be32 = p[0]<<24 \| p[1]<<16 \| p[2]<<8 \| p[3]` — cast `p[i]` to `uint32_t` first | W §3.6 |
| Never `*(uint32_t*)buf`: wrong order, misaligned, aliasing UB | W §3.6 |
| `uint8_t/uint16_t` promote to `int` before arithmetic | W §2, §3.7 |
| Sequence compare: clockwise distance `(a-b) mod 2ⁿ` read as signed; `> 0` ⇒ newer | W §3.7 |
| `static_cast<int16_t>(a - b) > 0` (16-bit), `static_cast<int32_t>(a - b) > 0` (32-bit) | W §3.7 |
| Exactly half a turn (32768 for 16-bit) is undefined in RFC 1982 → "not newer" both ways | W §3.7 |
| Valid only if compared packets are < half a turn apart (≈ 10.9 min at 50 pps, 16-bit) | W §3.7 |
| `a > b`, `uint16_t a-b > 0`, `uint32_t a-b > 0` are all wrong | W §3.7, Q11 |
| RTP seq starts random; 8 kHz × 20 ms = 160 ticks per packet (RFC 3550 §5.1) | W §3.7, §4 |
| Extended seq: `d = diff16(s, low16(H))`; value `H+d`; move `H` only if `d>0` | W §3.8 |
| `cycles++ when s < last` breaks on reordering | W §3.8 |
| Jitter buffer: classify by `d` (in-order / late / dup / restart); RFC 3550 A.1 heuristics | W Q16 |

**VERIFY:** IEN 137, RFC 791 App. B, RFC 1700 wording, RFC 1982 §3, RFC 3550 §5.1/§6.4.1/A.1, RFC 3551 L16 order (all in W §3.2, §3.3, §3.7, §3.8).
