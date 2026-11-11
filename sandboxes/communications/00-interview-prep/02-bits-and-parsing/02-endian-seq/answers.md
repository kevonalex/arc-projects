# Pack 02 — Answers (endianness & sequence arithmetic)

Each answer: **reasoning** → **common wrong answers** (*why tempting* / *why wrong*). `§` = section in [`worksheet.md`](worksheet.md). All data synthetic.

---

## Q1 — Memory layout of `0x11223344` (Explain)

| address | +0 | +1 | +2 | +3 |
|---|---|---|---|---|
| BE | 11 | 22 | 33 | 44 |
| LE | 44 | 33 | 22 | 11 |

A hex dump prints bytes in address order. BE dump: `11 22 33 44` — reads like the number. LE dump: `44 33 22 11` — you must reverse it mentally. So BE "reads naturally" in dumps; LE is natural for hardware adders (low byte first). The *value* is `0x11223344` on both (§3.1).

**Common wrong answers.**
- *"LE stores the bits reversed: `44 33 22 11` with each byte bit-reversed."* — Tempting: "little-endian = backwards". Wrong: bytes are reordered, bits inside each byte are untouched.
- *"The value is different on LE."* — Tempting from seeing `44` first. Wrong: loading the variable gives the same number; only the memory image differs.

---

## Q2 — Bytes for 5004 and `0xCAFEBABE` (Calculate)

`5004 = 0x138C` (1·4096 + 3·256 + 8·16 + 12 = 5004 ✓).
- BE: `+0 = 13`, `+1 = 8C`. LE: `+0 = 8C`, `+1 = 13`.
- `0xCAFEBABE`: BE `CA FE BA BE`; LE `BE BA FE CA`.
- `htons(5004)` on LE swaps bytes: returns `0x8C13` = 8·4096 + 12·256 + 1·16 + 3 = 32768 + 3072 + 19 = **35859**. Its *memory image* on LE is `13 8C` — the wire bytes (§3.4).

**Common wrong answers.**
- *"`htons(5004) == 5004`."* — Tempting: "it converts, so the value stays". Wrong: it converts the *representation*; on LE the integer changes.
- *`0x138C` as decimal 5004 → "bytes 50 04".* — Tempting: reading decimal digits as bytes. Wrong: bytes are base-256 digits, not decimal.

---

## Q3 — Pointer to a `uint16_t` (Predict)

LE host: memory is `34 12` → `p[0] = 0x34`, `p[1] = 0x12`. BE host: `p[0] = 0x12`, `p[1] = 0x34`.
`ntohs(x)` interprets the *memory image* of `x` as big-endian: on LE the image `34 12` is read as `0x3412`, so it returns **`0x3412`**; on BE the image `12 34` is already network order, so it returns **`0x1234`**.

(Accessing an object through `uint8_t*` is permitted; the reverse direction is the aliasing problem.)

**Common wrong answers.**
- *"`ntohs(0x1234) = 0x1234` everywhere because it's just a converter."* — Tempting: confusing "network→host" with "no-op". Wrong: on LE it swaps.
- *"`p[0] = 0x12` on all machines."* — Tempting: the number is written 12 34. Wrong: that's BE.

---

## Q4 — Trace `bswap32(0xA1B2C3D4)` (Trace)

- `v & 0x000000FF = 0xD4`; `<< 24` → `0xD4000000`.
- `v & 0x0000FF00 = 0xC300`; `<< 8` → `0x00C30000`.
- `v >> 8 = 0x00A1B2C3`; `& 0x0000FF00 = 0x0000B200`.
- `v >> 24 = 0x000000A1`.
- OR: `0xD4000000 | 0x00C30000 = 0xD4C30000`; `| 0x0000B200 = 0xD4C3B200`; `| 0xA1 = 0xD4C3B2A1` ✓.

**Common wrong answers.**
- *Using `(v >> 8) & 0xFF` for the `B2` lane.* — Tempting: symmetry with the other lanes. Wrong: `v >> 8 = 0x00A1B2C3`, and `& 0xFF` keeps `0xC3`, the wrong byte, in the wrong lane. The `B2` byte sits in bits 15..8 after the shift, so the mask must be `0x0000FF00`, giving `0x0000B200`.
- *Forgetting masks.* — `(v << 24) | (v << 8) | (v >> 8) | (v >> 24)` leaves stray bytes (e.g. `0xC3D40000 | …`), giving garbage.

---

## Q5 — Why shifts are portable (Explain)

1. **Value vs memory:** `p[0] << 8 | p[1]` builds a number from two *byte values* and numeric shifts — operations defined on values, identical on every machine. `*(uint16_t*)p` reads the **memory image** with the host's own byte order.
2. **Alignment:** `p` may be odd; on some MCUs a 16-bit load at an odd address faults (or is slow). Byte loads are always legal.
3. **Strict aliasing:** accessing `uint8_t` storage as `uint16_t` is UB; optimisers can reorder/eliminate such accesses.
(Also: signedness—`uint16_t` result is explicit; no sign surprises.)

**Common wrong answers.**
- *"Because the compiler swaps bytes for me on LE."* — Tempting: "portable means automatic". Wrong: nothing is swapped implicitly; the shifts *express* the wire order.
- *"It's only about speed."* — Tempting: shift looks like extra work. Wrong: correctness and UB are the issue; modern compilers emit one load + `bswap` anyway.

---

## Q6 — Find the bugs (Find the bug)

**(a)** `*(uint16_t*)(pkt + 2)`: (i) wrong byte order on LE (bytes `13 8C` read as `0x8C13`); (ii) possible misalignment (pkt+2 may be unaligned if `pkt` is odd); (iii) strict-aliasing UB; (iv) **no `ntohs`**. Fix: `load_be16(pkt + 2)`.

**(b)** `pkt[8]<<24 | …`: `pkt[8]` is `uint8_t` → promoted to `int`; `0x80 << 24` overflows `int` (UB until C++20; implementation-defined result after). Input SSRC `0xFF…`: sign bit gets set, and widening to `uint32_t` happens to look right on most compilers, but UBSan flags it and the compiler may exploit UB. Fix: `uint32_t(pkt[8]) << 24`, i.e. `load_be32`.

**(c)** `pkt[4] | pkt[5] << 8`: that is **little-endian** loading of a **big-endian** field. Input `01 00` (= 256 in BE) yields 1. Symptom: lengths wrong by byte swap; short packets accepted/rejected wrongly. Fix: `(pkt[4] << 8) | pkt[5]` → `load_be16`.

**Common wrong answers.**
- *"(c) is fine; it's just another way to write it."* — Tempting: both forms look symmetrical. Wrong: indices matter; `pkt[4]` is the *high* byte on the wire.
- *"(b) is fine on x86."* — Tempting: it works in practice. Wrong: signed-overflow UB; behaviour changes with optimisation level and under UBSan.

---

## Q7 — 8-bit clock comparisons (Calculate)

| (a,b) | (a−b) mod 256 | signed | newer(a,b) |
|---|---|---|---|
| (5,250) | 11 | +11 | **yes** |
| (250,5) | 245 | −11 | no |
| (200,100) | 100 | +100 | **yes** |
| (100,200) | 156 | −100 | no |
| (128,0) | 128 | −128 | no (exactly half) |
| (127,0) | 127 | +127 | **yes** |
| (0,128) | 128 | −128 | no (exactly half) |

**Common wrong answers.**
- *(128,0) → yes because 128 > 0.* — Tempting: plain integer comparison. Wrong: on the clock, 128 ticks is *exactly opposite*; RFC 1982 leaves it undefined; the signed formula says no both ways.
- *(100,200) → yes because it's "100 behind 200 so a is newer".* — Tempting: confusion about which is `a`. Wrong: `a−b = −100` → `a` is *behind* `b`.

---

## Q8 — 16-bit comparisons (Calculate)

| (a,b) | `a−b` (int) | mod 65536 → int16 | newer | diff |
|---|---|---|---|---|
| (1,65535) | −65534 | +2 | yes | 2 |
| (65535,1) | 65534 | −2 | no | −2 |
| (100,65500) | −65400 | +136 | yes | 136 |
| (32767,0) | 32767 | +32767 | yes | 32767 |
| (32768,0) | 32768 | −32768 | no | −32768 |
| (0,32768) | −32768 | −32768 | no | −32768 |
| (0,32769) | −32769 | +32767 | yes | 32767 |

Note (32768,0) and (0,32768) are both "not newer": exactly half a turn.

**Common wrong answers.**
- *(1,65535): "no, 1 < 65535".* — Tempting: the naive compare. Wrong: that is exactly the wrap bug.
- *(0,32769) "no".* — Tempting: 0 < 32769. Wrong: 32767 ticks clockwise from 32769 to 0 (wrapping) is less than half a turn → 0 is newer.

---

## Q9 — Deriving the compare (Explain)

Put the 65,536 sequence numbers on a clock. The clockwise distance from `b` to `a` is `(a − b) mod 65536`, in `[0, 65535]`. Interpret "newer" as "a modest number of steps clockwise" and "older" as "a modest number of steps anticlockwise". The ambiguous middle is half a turn, 32768 ticks. So: clockwise distance in `1..32767` → newer; `32769..65535` → older (it's `65536 − d` steps *anticlockwise*); `0` → equal; `32768` → ambiguous.

Reading the 16-bit result as two's complement turns `0..32767` into itself and `32768..65535` into `−32768..−1`. So `> 0` ⇔ clockwise distance in `1..32767`. The cast `static_cast<int16_t>(a - b)` does the "mod 65536" step too, because converting an out-of-range value to a 16-bit type is modular (guaranteed in C++20; every mainstream compiler before that).

Half a turn is the threshold because with *any* two packets there must be a single unambiguous reading; the farthest apart two points can be on a circle is half a turn. If the sender stalls (or the receiver is offline) for more than 32,767 packets (≈ 10.9 min at 50 pps), a genuinely newer packet appears *older*: you lose sync and must resync (RFC 3550 A.1 heuristics).

**Common wrong answers.**
- *"Because 16 bits is signed"* — Tempting: `int16_t` appears in the code. Wrong: the sequence field is unsigned; the signed type is just a *reading* of the distance.
- *"Half is arbitrary."* — Wrong: it's the unique symmetric split; any other split makes some pairs have two valid answers.

---

## Q10 — 12-minute outage (Predict)

`37000 − 1000 = 36000` ticks clockwise (≥ 32768). `static_cast<int16_t>(36000)` = `36000 − 65536 = −29536` → `seq_newer16(37000, 1000)` is **false**. The receiver concludes packet 37000 is *older* than 1000 and discards the entire stream after the outage — silent audio even though the link is back. RFC 3550 A.1 doesn't trust a single comparison: a packet whose sequence jumps by more than `MAX_DROPOUT` (3000, VERIFY) is treated as a *probable restart*; the receiver holds that value and, if the *next* packet is sequential (`seq == bad_seq + 1`), re-initialises sequence state. A jump backwards less than `MAX_MISORDER` (100, VERIFY) is a reordered packet.

**Common wrong answers.**
- *"It returns true since 37000 > 1000."* — Tempting: plain integers. Wrong: the formula measures clock distance, which exceeds half a turn.
- *"Use a bigger window."* — Tempting: just change the type. Wrong: the field is only 16 bits; extending requires extra state (§3.8) and heuristics.

---

## Q11 — Three broken compares (Find the bug)

- `v1: a > b`: fails at the wrap: `(a=0, b=65535)` returns false though 0 is the next packet.
- `v2: a - b > 0` with `uint16_t`: both operands promote to `int`, so `a−b` is a genuine signed integer: `(a=1, b=65535)` → `−65534 > 0` false (**should be true**); `(a=65535, b=1)` → `65534 > 0` true (**should be false**). It "works" without wrap, then inverts exactly at the wrap.
- `v3: a - b > 0` with `uint32_t`: unsigned subtraction never goes negative → true for every `a != b`. `(a=0, b=1)` → `0xFFFFFFFF > 0` true — everything is "newer".

Fix: `static_cast<int16_t>(a - b) > 0` / `static_cast<int32_t>(a - b) > 0`.

**Common wrong answers.**
- *"v2 is correct, `uint16_t` wraps."* — Tempting: unsigned types wrap! Wrong: promotion to `int` happens before subtraction (§2).
- *"v3 only fails at the wrap."* — Tempting: same as v2. Wrong: it fails for all `a < b`.

---

## Q12 — Implement (Implement)

Reference: `code/solutions/byteorder.h`. Tests that catch typical mistakes:
- `LoadStore.LoadHighByteDoesNotSignExtend` — forgot `uint32_t(p[0])` cast.
- `Hton.MemoryImageIsBigEndian*` — implemented `htons` as identity or swapped on BE.
- `Seq16.HalfTurnBoundary`, `Seq16.MatchesRfc1982Definition` — compare via `a > b` or off-by-one at half a turn.
- `SeqExtender.ReorderedPacketsGet…` — incrementing cycles whenever `s < last`.

**Common wrong answers:** see Q5, Q6, Q11; and *"detect endianness with `reinterpret_cast<uint8_t*>`-based unions"* — union type-punning is UB in C++ (OK in C); use `memcpy` or `std::endian`.

---

## Q13 — Trace `SeqExtender` (Trace)

| input | `low16(H)` | `d` | returned | `H` after |
|---|---|---|---|---|
| 65534 | — (first) | — | 65534 | 65534 |
| 65535 | 65534 | +1 | 65535 | 65535 |
| 0 | 65535 | +1 | 65536 | 65536 |
| 65535 | 0 | −1 | 65535 | 65536 |
| 1 | 0 | +1 | 65537 | 65537 |
| 3 | 1 | +2 | 65539 | 65539 |
| 2 | 3 | −1 | 65538 | 65539 |

**Common wrong answers.**
- *Late `65535` returns 131071.* — Tempting: "`s < last` → new cycle". Wrong: the clock distance says it's 1 *behind* H, not 65535 ahead.
- *`highest` drops to 65535 on the late packet.* — Wrong: the high-water mark never moves backwards.

---

## Q14 — Connections (Connect)

**(a)** Every multi-byte header field in Pack 03 (RTP seq/timestamp/SSRC, IPv4 total length/id/addresses, UDP ports/length) is big-endian → `load_be16/32`; `BitReader` handles sub-byte fields (version/IHL/flags).

**(b)** The one's-complement sum is computed mod 65535 (Pack 04 §3.8); `256 · x mod 65535` is a byte-swap of a 16-bit word, and sum commutes with multiplication by 256 → swapping every input word just swaps the sum: byte order doesn't matter. CRC-32 as standardised processes bits in a specific order (LSB-first "reflected") over a byte stream; a different bit order produces a different remainder, so implementations must match.

**(c)** TCP sequence numbers (mod 2³², RFC 793/RFC 9293), 802.11 12-bit sequence control, DNS zone serials (RFC 1982). Pack 06's LFSR state lives on a circle of length 2ⁿ−1: after 127 steps a 7-bit LFSR returns to its start, like the clock.

**Common wrong answers.**
- *"Byte order affects the Internet checksum."* — Tempting: it sums 16-bit words. Wrong: it is order-independent as above (RFC 1071 §2(B)).

---

## Q15 — Interview: `htons` and endianness detection (Interview)

Portable `htons` with no UB:
```cpp
uint16_t my_htons(uint16_t v) {
  uint8_t b[2] = { uint8_t(v >> 8), uint8_t(v & 0xFF) };   // wire bytes
  uint16_t out; std::memcpy(&out, b, 2); return out;       // image that memory-copies to the wire
}
```
This is correct on any host *because* it builds the big-endian bytes from the numeric value, then reinterprets them in host order. Or branch on endianness and swap.

Detect at run time: `memcpy` a `uint16_t 1` into a byte; compare. Compile time: C++20 `std::endian::native == std::endian::little` (`<bit>`); C++17: `__BYTE_ORDER__` (GCC/Clang) or platform macros — no standard way. Portable code avoids needing to know: use `load_be*`/`store_be*`.

**Common wrong answers.**
- *"`return (v << 8) | (v >> 8);` always."* — Tempting: it's the LE answer. Wrong on a BE host (would corrupt) — correct only if you check endianness.
- *"Use a union."* — Wrong in C++ (UB), fine in C.

---

## Q16 — Interview: jitter-buffer classification (Interview)

Keep `H` (highest extended seq) and a bitmap/window of received sequences. For a new 16-bit `s`: `d = seq_diff16(s, low16(H))`.
- `d > 0` and small (`< MAX_DROPOUT`, e.g. 3000): **in order or small gap** → advance `H`, record gap as loss.
- `d ≤ 0` and `−d` within the reorder window (`MAX_MISORDER` ≈ 100, or your buffer depth in packets): **late/reordered** — insert if not already in the bitmap, else **duplicate**.
- `|d|` large (> thresholds): **source restart or outage** — don't trust; hold the candidate and resync after `MIN_SEQUENTIAL` (2) consecutive packets (RFC 3550 A.1).
Window sizes: reorder window = jitter-buffer depth in packets (e.g. 60 ms ÷ 20 ms = 3 + margin); dropout bound well below half a turn (32768).

**Common wrong answers.**
- *"Treat `s < last` as late."* — Wrong at wrap.
- *"Accept any packet as new if far away."* — Allows an attacker or a stale sender to hijack sequence state; hold and verify.

---

## §6 Self-assessment key

(1) LE bytes of `0x0A0B`: `0B 0A`. (2) `newer(3, 65534)`: `3−65534 = −65531 ≡ +5` → **yes**. (3) `bswap16(0xBEEF) = 0xEFBE`. (4) A 16-bit sequence wraps every 65,536 packets: at 100 pps that's ≈ 655 s ≈ 10.9 min. (5) `htons(0x0001)` on LE = `0x0100` = 256.

**VERIFY (this pack):** see the list at the end of the worksheet (RFC 791 App. B, RFC 1700, RFC 1982 §3, RFC 3550 §5.1/§6.4.1/A.1, RFC 3551, IEN 137).
