# Pack 02 — Endianness & Sequence-Number Arithmetic (Etherstack interview worksheet)

> **Audience:** starts from zero; C++17/20. **Rule:** concrete first, then derive, then name the trick. If you are memorising, go back to the derivation.
> **Data rule:** all bytes, ports and sequence numbers here are **synthetic**. No live public-safety receive.
> **Companions:** [`answers.md`](answers.md) · [`cheatsheet.md`](cheatsheet.md) · `code/` (stubs `code/include/byteorder.h`, tests `code/tests/`, reference `code/solutions/`).
> **Prereq pack:** Pack 01 (shifts, masks, two's complement) — §2 below lists exactly what you need.

Section map (the cheatsheet cites these): **§1** Why · **§2** Prerequisites · **§3** Concept build-up (3.1–3.8) · **§4** Worked example · **§5** Questions Q1–Q16 · **§6** Self-assessment.

---

## §1 Why this matters

A gateway receives bytes from the network and hands numbers to a radio stack (and vice versa). An embedded ARM/x86 box stores a 32-bit number one way; the RTP/UDP/IP protocols *transmit* it another way. Get it wrong and a UDP port 5004 becomes 35859, a 20 ms audio timestamp becomes garbage, and the jitter buffer thinks packets from the future have arrived.

| Where | What goes wrong without this |
|---|---|
| **Network / gateway** | Ports, lengths, SSRCs, timestamps read byte-swapped; all packets "malformed" or routed to port 35859 |
| **LMR audio over IP** | RTP sequence numbers are 16 bits and **wrap every 65,536 packets** (about 21.8 minutes at 50 packets/s). A naive `>` compare drops audio at every wrap |
| **Embedded** | Peripheral/flash fields may be little-endian while the wire is big-endian; the CPU may be either |
| **Interview** | "Write htons without the library", "detect endianness", "compare sequence numbers with wraparound" are classic. |

**You will be able to:** predict the bytes in memory for any integer, convert between orders with shifts that work on *any* host, and compare wrapping sequence numbers with `static_cast<int16_t>(a - b) > 0` — and say *why* it is correct and *when* it fails.

---

## §2 Prerequisites

- Hex ↔ binary (Pack 01 §2). One byte = two hex digits = 8 bits. `0x12` is one byte.
- `x << n`, `x >> n`, `&`, `|` on **unsigned** types; shifting by ≥ width is UB (Pack 01 §2).
- Two's complement: top bit set means value `v − 2ⁿ` (Pack 01 §3.5). Reminder from the odometer: `1111 1111` is −1 in 8 bits.
- Unsigned arithmetic wraps **modulo 2ⁿ** (defined behaviour). Example: `uint8_t 250 + 10 = 4`.
- **Integral promotion trap:** `uint8_t`/`uint16_t` operands are promoted to `int` before arithmetic, so `uint16_t a=1, b=65535; a - b` is the **int** `−65534`, not an unsigned wrap. Casting back to `uint16_t` or `int16_t` afterwards re-applies the modulo.

---

## §3 Concept build-up

### 3.1 Concrete first: memory is a row of numbered mailboxes

Each *byte* lives at an address. A 32-bit number needs **4 mailboxes**. Take the number **`0x11223344`**. Its bytes, by significance:

```
most significant byte (MSB) = 0x11
                            = 0x22
                            = 0x33
least significant byte (LSB)= 0x44
```

Two machines, same number, different filing:

| address → | +0 | +1 | +2 | +3 | who |
|---|---|---|---|---|---|
| **Big-endian** (BE) | `11` | `22` | `33` | `44` | most-significant byte at the **lowest** address ("big end first") — how you *write* the number |
| **Little-endian** (LE) | `44` | `33` | `22` | `11` | least-significant byte at the lowest address ("little end first") — x86, x86-64, most ARM in default mode |

**Same value, same numeric meaning; different byte order in memory.** Nothing is "wrong" with either.

A 16-bit example, UDP port 5004 = `0x138C`: BE memory `13 8C`; LE memory `8C 13`.

> **Misconceptions box 3.1**
> 1. *"Endianness is about bit order inside a byte."* No. It is about **byte** order in multi-byte values. Within a byte, a CPU never exposes bit order (an 8-bit load gives you the byte).
> 2. *"The number changes value on a different machine."* The *value* is the same; only its memory image differs. A bug appears when bytes cross a boundary (network, file, DMA) without conversion.
> 3. *"Single bytes (`uint8_t`) have endianness."* A one-byte value has only one arrangement.

### 3.2 Why both exist; where the names come from; detecting yours

- **Little-endian** is convenient for hardware doing carry-propagating addition: you meet the low byte first, so the carry can start immediately; and a 32-bit value read as 8-bit or 16-bit at the same address gives the *low part*.
- **Big-endian** matches how humans read digits and hex dumps; and bit/byte-ordered protocols are easier to read left to right.
- **Names:** Danny Cohen's 1980 note *"On Holy Wars and a Plea for Peace"* (IEN 137; later in IEEE Computer, 1981) borrowed *Big-Endians* and *Little-Endians* from Swift's *Gulliver's Travels* (the factions that fight over which end of a boiled egg to crack). *(VERIFY IEN number/date.)*

**Detect at run time** — look at memory, do not trust a cast:

```cpp
uint16_t one = 1;
uint8_t first;
std::memcpy(&first, &one, 1);
bool little = (first == 1);
```
Why `memcpy`? `*(uint8_t*)&one` is *allowed* for `char`-type access, but the habit of `*(uint32_t*)bytes` the other way round violates strict aliasing and alignment (Pack 03 §3.1). C++20 offers `std::endian::native` in `<bit>` — compile-time.

> **Misconceptions box 3.2**
> 1. *"Big-endian is the 'network' one so it is better."* It is a convention; both are fine for CPUs.
> 2. *"I can check by casting a `uint32_t*` to `char*`."* Fine in C++ for `char`/`unsigned char`; but do not cast the other way (aliasing).
> 3. *"x86 is the only little-endian CPU."* Many ARM cores run LE by default but can be BE-capable; never assume — use the portable shift-based code (§3.6).

### 3.3 Network byte order — what it is and why

Internet protocol headers define multi-byte integers to be **transmitted most-significant byte first**, i.e. **big-endian**. This is called **network byte order**. It was fixed by convention in the earliest ARPANET/Internet specs — RFC 791 (IPv4), Appendix B *"Data Transmission Order"*, shows the diagrams read left-to-right, top-to-bottom, with the most significant bit/byte first; and the *Assigned Numbers* series (RFC 1700, since replaced by the IANA registries) states the "big-endian" convention outright. *(VERIFY RFC 791 App. B wording and RFC 1700 sentence before quoting.)*

Why a convention at all? Two machines on a wire must agree *without negotiation*; choosing one order once, for everyone, means each side converts only between **its own** order and the wire order. This is **O(N)** conversions instead of **O(N²)** pairwise conversions.

> **Misconceptions box 3.3**
> 1. *"Network order applies to the whole packet payload."* It applies to the *protocol header fields* defined to be so. The application payload (e.g. your own struct) has whatever order its protocol says.
> 2. *"RTP audio samples are in network byte order."* RTP payload formats define their own (e.g., 16-bit linear audio L16 is big-endian per RFC 3551; μ-law is single bytes). *(VERIFY in RFC 3551 §4.5.)*
> 3. *"Bit order on the wire = byte order."* Bits within a byte are transmitted by the physical layer in its own way; protocol diagrams number bits MSB-first (bit 0 = top bit in RFC diagrams) — which differs from C++ "bit 0 = LSB". Translate carefully (Pack 03).

### 3.4 `htons`, `ntohs`, `htonl`, `ntohl`

*h*ost → *n*etwork *s*hort (16-bit), *l*ong (32-bit). On a BE host: **do nothing**. On an LE host: **swap the bytes**. Both directions are the same swap, so `ntohs` ≡ `htons`.

Concrete: LE host, port `5004 = 0x138C` held in a `uint16_t` as bytes `8C 13`.
`htons(5004)` returns a `uint16_t` whose **numeric value is `0x8C13` = 35859** and whose **memory image is `13 8C`** — exactly the bytes that go on the wire. Write it into the packet with `memcpy`. If you *forget* `htons`, the wire carries `8C 13`, which the receiver decodes (as BE) as 35859 → "port 35859, nobody listening".

Important subtlety: `htons` returns a number whose **memory image** is right, not whose numeric value is meaningful. Never do arithmetic on a value in network order (box 3.4).

> **Misconceptions box 3.4**
> 1. *"`htons` gives the correct number."* It gives a number whose *bytes* are right for the wire; the numeric value looks "wrong" on LE hosts (35859 above).
> 2. *"I should call `htons` twice to be safe."* Double conversion swaps back — a classic bug.
> 3. *"On x86 I don't need it for loopback tests."* Loopback tests pass if both ends make the same mistake; a real peer fails.

### 3.5 Manual byte swap, derived byte by byte

**bswap16.** `v = 0x1234`: high byte `0x12`, low byte `0x34`. Want `0x3412`.
- `v << 8` moves `0x34` up: `0x123400` (in a `uint16_t` the top byte `0x12` falls off after truncation → `0x3400`).
- `v >> 8` moves `0x12` down: `0x0012`.
- OR them: `0x3400 | 0x0012 = 0x3412` ✓.
`static_cast<uint16_t>((v << 8) | (v >> 8))` — the cast is needed because `v << 8` is an `int` (promotion).

**bswap32.** `v = 0xA1B2C3D4`; bytes: `A1 B2 C3 D4`. Want `D4 C3 B2 A1`. Move each byte to its mirrored lane:

| source byte | how to bring it | lands at |
|---|---|---|
| `D4` (lane 0) | `(v & 0x000000FF) << 24` | `0xD4000000` |
| `C3` (lane 1) | `(v & 0x0000FF00) << 8` | `0x00C30000` |
| `B2` (lane 2) | `(v >> 8) & 0x0000FF00` | `0x0000B200` |
| `A1` (lane 3) | `v >> 24` | `0x000000A1` |

OR: `0xD4C3B2A1` ✓. (Compilers often turn this into one `bswap`/`rev` instruction; GCC/Clang have `__builtin_bswap32`; C++23 has `std::byteswap`.)

### 3.6 Host-independent load/store with shifts (the fix for everything)

Insight: `x << 8` and `x >> 8` operate on the **numeric value**, which is the same on every machine. They never look at how the value sits in memory. So a function that *builds a number from bytes by shifting* is correct on any CPU:

```cpp
uint32_t load_be32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
```
Concrete: `p = 12 34 56 78` → `0x12000000 | 0x340000 | 0x5600 | 0x78 = 0x12345678` on *every* host. The little-endian loader is the same with indices reversed (`p[3]` high).

Store: `p[0] = v >> 24; p[1] = (v >> 16) & 0xFF; …`.

**Why cast `p[0]` to `uint32_t` before `<<24`?** `uint8_t` promotes to *signed* `int`; `0xFF << 24` overflows `int` (UB before C++20 / implementation-defined after). Cast first (Pack 01 §3.3.7 F6).

> **Misconceptions box 3.6**
> 1. *"Use `*(uint32_t*)p` for speed."* Misaligned access (crash on some MCUs, slow on others), strict-aliasing UB, *and* wrong order on LE hosts. Use shifts or `memcpy` + `ntohl`.
> 2. *"The shift version is slow."* Compilers recognise the pattern and emit one load + `bswap`.
> 3. *"`memcpy` into a `uint32_t` fixes endianness."* It fixes aliasing/alignment, not byte order; follow with `ntohl`.

### 3.7 Sequence numbers and the clock face

**Problem.** Protocols number packets with a **fixed-width field** (RTP: 16 bits, RFC 3550 §5.1). The counter must eventually wrap from `65535` to `0`. A receiver must still say which of two packets is newer.

**Naive `a > b` fails at the wrap.** Packet `65535` is sent, then packet `0` (the next one). `0 > 65535` is false, so "packet 0 is old" → dropped. Every 21.8 minutes of a 50 pps stream the radio audio would stutter.

**Concrete first: an 8-bit clock.** Use a clock with 256 ticks instead of 65,536 (numbers are small; the idea is identical). Sequence numbers are positions on the clock; "newer" means *a few ticks clockwise*.

```
         0
     250   5        a = 5, b = 250:
   ...   ...        walk clockwise from 250: 251,252,253,254,255,0,1,2,3,4,5
                    that is 11 steps. So 5 is 11 ticks AHEAD of 250: 5 is newer.
```

**Derive the formula.** Clockwise distance from *b* to *a* is `(a − b) mod 256`. 
- a = 5, b = 250: `(5 − 250) mod 256 = −245 mod 256 = 11`. ✓ matches the walk.
- a = 250, b = 5: `(250 − 5) mod 256 = 245` — that's *almost the whole circle* clockwise, i.e. `11` steps **counter-clockwise**. So 250 is *older*.

**Rule:** if the clockwise distance is less than half a turn (`< 128`), *a* is ahead; otherwise *a* is behind. Reading the 8-bit distance as **signed** does exactly this split: values `0..127` stay positive, `128..255` become `−128..−1` (two's complement, Pack 01 §3.5):

| a | b | (a − b) mod 256 | as int8 | newer? |
|---|---|---|---|---|
| 5 | 250 | 11 | +11 | **yes** |
| 250 | 5 | 245 | −11 | no |
| 129 | 2 | 127 | +127 | yes (just inside half) |
| 130 | 2 | 128 | −128 | **no, and neither is `2 > 130`** (exactly half) |
| 7 | 7 | 0 | 0 | no (equal) |

So for 16 bits:

```cpp
bool newer16(uint16_t a, uint16_t b) { return static_cast<int16_t>(a - b) > 0; }
```
`a - b` is computed in `int` (promotion), may be negative; the `static_cast<int16_t>` reduces it modulo 65536 into `[−32768, 32767]`. Example `a=1, b=65535`: `a−b = −65534`; modulo 65536 → `+2` → newer ✓.

**This is RFC 1982 *Serial Number Arithmetic* (R. Elz & R. Bush, 1996, written for DNS zone serials).** Its definition, for `SERIAL_BITS = N`: `s1 < s2` iff `(i1 < i2 and i2 − i1 < 2^(N−1))` or `(i1 > i2 and i1 − i2 > 2^(N−1))`; and the case `|i1 − i2| = 2^(N−1)` is **undefined** (comparison "not defined"). The signed-cast formula implements precisely this and returns "not newer" both ways at exactly half a turn. *(VERIFY the exact inequality statement in RFC 1982 §3.2.)* TCP uses the same arithmetic modulo 2³² for its sequence numbers; RTP receivers use the 16-bit form (RFC 3550, Appendix A.1).

**When it fails — know the assumption.** The method assumes two packets being compared are **less than half a turn apart** (32,768 packets ≈ 10.9 minutes at 50 pps). If a link goes dark longer and the sender keeps counting, "old" and "new" flip. RFC 3550 A.1 therefore adds heuristics (`MAX_DROPOUT`, `MAX_MISORDER`, `MIN_SEQUENTIAL` — VERIFY values 3000/100/2) to decide when to resync.

> **Misconceptions box 3.7**
> 1. *"`a > b` works if I use `uint32_t`."* It only delays the wrap to 2³² (TCP-style counters still need modular compare).
> 2. *"The cast is a hack."* It is modular subtraction + reading the result as signed: the **definition** of "within half a turn clockwise".
> 3. *"`a - b > 0` with `uint16_t` is the same."* Promotion makes `a−b` an `int` that goes *negative* when `a < b` — fails at the wrap. For `uint32_t` it's worse: unsigned minus is never negative, so `a − b > 0` is true whenever `a != b`.
> 4. *"Exactly half a turn is just a rare edge."* It is *undefined by the RFC*, and a stuck sequence in a test can park exactly there.
> 5. *"Sequence numbers start at 0."* RTP starts at a **random** value (RFC 3550 §5.1) to resist plaintext attacks; never assume 0.

### 3.8 Extending 16 bits to a monotonic 64-bit counter

Jitter buffers, loss statistics and recorders want a number that never wraps. Track the **highest extended sequence** `H`. For a new 16-bit `s`:
- `d = (int16_t)(s − (uint16_t)H)` (clock-face distance from the current high point).
- Packet's extended value = `H + d`.
- If `d > 0`, set `H = H + d`; otherwise it is a reordered/late packet — keep `H`.

Concrete: `65534, 65535, 0, 65535, 1`: `H=65534`; 65535 → d=+1 → 65535; 0 → `low16(H)=65535`, `d=+1` → 65536; late 65535 → `low16(H)=0`, `d=−1` → 65535, H stays 65536; 1 → d=+1 → 65537.

This is RFC 3550's *extended highest sequence number received* (§6.4.1 / A.1): `cycles + max_seq`. *(VERIFY section numbers.)*

> **Misconceptions box 3.8**
> 1. *"Count wraps: when `s < last`, `cycles++`."* Breaks on reordering: a late `65535` after `0` would add a spurious cycle.
> 2. *"Use the signed 64-bit difference."* There is no wrap in 64 bits; the issue is *recovering* the missing upper bits from a 16-bit field — done with the signed 16-bit difference.

---

## §4 Fully worked example — one RTP packet through a gateway

Synthetic RTP header bytes on the wire: `80 80 00 01 00 00 00 A0 12 34 56 78` (full parse in Pack 03).

1. **Sequence number** = bytes 2–3 = `00 01`. Read BE: `(0x00 << 8) | 0x01 = 1`. On an LE host `*(uint16_t*)(p+2)` would give `0x0100 = 256` — wrong.
2. **Timestamp** = bytes 4–7 = `00 00 00 A0` → `load_be32` = `0xA0` = 160 = 20 ms at 8 kHz ✓ (RFC 3550 §5.1; 8,000 samples/s × 0.020 s).
3. **SSRC** = bytes 8–11 = `12 34 56 78` → `0x12345678`; the LE-misread would be `0x78563412`.
4. Next packet arrives with sequence `00 02`: `newer16(2, 1)` → `static_cast<int16_t>(1) > 0` ✓.
5. Much later the stream hits `FF FF` then `00 00`: `newer16(0, 65535)`: `a−b = −65535` → mod 65536 = `+1` → newer ✓; `SeqExtender`: 65535 → 65536 (cycle count +1).
6. Sending back: build the 16-bit port 5004 for UDP: `store_be16(p, 5004)` → bytes `13 8C` on any host.

---

## §5 Questions

Each has a type, a context line, and a full answer in `answers.md`. Do them on paper before running tests.

**Q1 — Explain.** *Context: gateway debugging a packet capture.* Draw a memory table (BE and LE) for `uint32_t 0x11223344`. Then say what a hex dump of that variable looks like on each. Which one "reads naturally"?

**Q2 — Calculate.** *Context: UDP port in an RTP stream.* Give the memory bytes (addresses +0, +1) for `uint16_t 5004` on BE and LE. Then give the bytes for `0xCAFEBABE` on both. What 16-bit *integer* does `htons(5004)` return on an LE host (hex and decimal)?

**Q3 — Predict.** *Context: embedded firmware reading a 16-bit register from a byte buffer.* On an LE host: `uint16_t x = 0x1234; uint8_t* p = reinterpret_cast<uint8_t*>(&x);` What are `p[0]`, `p[1]`? Same code on a BE host? What does `ntohs(x)` return on each?

**Q4 — Trace.** *Context: writing your own `htonl` for a stripped-down RTOS.* Trace `bswap32(0xA1B2C3D4)` with the four masks/shifts of §3.5, writing every intermediate hex value.

**Q5 — Explain.** *Context: portable parsing library.* Why is `(p[0] << 8) | p[1]` correct on both BE and LE hosts, while `*(uint16_t*)p` is not? Give three separate reasons.

**Q6 — Find the bug.** *Context: gateway code review.* Find all bugs:
```cpp
uint16_t port = *(uint16_t*)(pkt + 2);           // (a)
uint32_t ssrc = pkt[8]<<24 | pkt[9]<<16 | pkt[10]<<8 | pkt[11];   // (b) pkt is const uint8_t*
uint16_t len  = pkt[4] | pkt[5] << 8;            // (c) field is network byte order
```
State the failing input for each, the symptom, and the fix.

**Q7 — Calculate.** *Context: RTP jitter buffer.* Using the 8-bit clock of §3.7, decide newer(a,b) for: (5,250), (250,5), (200,100), (100,200), (128,0), (127,0), (0,128). Show `(a−b) mod 256` and the signed reading.

**Q8 — Calculate.** *Context: 16-bit RTP sequence compare.* Evaluate `seq_newer16(a,b)` and `seq_diff16(a,b)`: (1,65535), (65535,1), (100,65500), (32767,0), (32768,0), (0,32768), (0,32769).

**Q9 — Explain.** *Context: interview whiteboard.* Derive from the clock face why `static_cast<int16_t>(a - b) > 0` means "a is newer". Why is the threshold half a turn? What if the sender stalls for longer?

**Q10 — Predict.** *Context: radio gateway with a 12-minute link outage.* RTP at 50 pps: sequence 1000 is the last packet before the outage; the sender kept counting, and the first packet after is 1000 + 36000 = 37000. What does `seq_newer16(37000, 1000)` say, and why is that dangerous? What do RFC 3550 A.1 heuristics do instead?

**Q11 — Find the bug.** *Context: transport code review.* Three versions of "is `a` newer than `b`":
```cpp
bool v1(uint16_t a, uint16_t b){ return a > b; }
bool v2(uint16_t a, uint16_t b){ return a - b > 0; }
bool v3(uint32_t a, uint32_t b){ return a - b > 0; }
```
Give one input where each fails. (v2: a=65535,b=1 and a=1,b=65535.)

**Q12 — Implement.** *Context: gateway library.* Implement every stub in `code/include/byteorder.h`; all `student.*` tests green, sanitizers clean.

**Q13 — Trace.** *Context: jitter-buffer statistics.* Run `SeqExtender` over `65534, 65535, 0, 65535, 1, 3, 2` and write the returned value and `highest` after each call.

**Q14 — Connect.** *Context: packet parsing & checksums.* (a) Where in Pack 03 do you need `load_be16/32`? (b) Why does the Internet checksum (Pack 04 §3.8) not care about host byte order but CRC-32 as normally implemented does care about *bit* order (reflection)? (c) Where do you see the "clock face" idea again in this module? (hint: Pack 06 LFSR period, TCP, 802.11 sequence control).

**Q15 — Interview.** *Context: phone screen.* "Write `htons` without calling the library. Make it correct on both BE and LE hosts, with no undefined behaviour." Also: "how do you detect endianness at run time and at compile time (C++17 vs C++20)?"

**Q16 — Interview.** *Context: designing a jitter buffer.* "Design a function to decide whether an incoming RTP packet is a duplicate, late, in-order or from the future; what window sizes do you pick and why?"

---

## §6 Self-assessment

- [ ] I can write the BE/LE memory layout of any 16/32-bit value from memory *by reasoning*, not recall.
- [ ] I can explain network byte order and why a fixed convention beats pairwise conversion.
- [ ] I can write `bswap32` and `load_be32` and explain why shifts are host-independent.
- [ ] I can name the three problems with `*(uint32_t*)buf`.
- [ ] I can derive `static_cast<int16_t>(a-b) > 0` from a clock face and explain integral promotion's role.
- [ ] I can quote the half-turn limitation and what RFC 1982 says about exactly half.
- [ ] I can extend a 16-bit sequence to 64 bits and explain why "cycles++ when s < last" is wrong.
- [ ] All `solution.*` tests pass; after Q12 all `student.*` too, sanitizers silent.

**Rapid fire (key at end of `answers.md`):** (1) LE bytes of `0x0A0B`? (2) 16-bit: newer(3, 65534)? (3) `bswap16(0xBEEF)`? (4) How many packets between a 16-bit wrap at 100 pps? (5) What does `htons(0x0001)` return on LE?

**VERIFY list:** IEN 137 citation details; RFC 791 Appendix B text; RFC 1700 "big-endian" sentence; RFC 1982 §3 definition and "undefined at half" clause; RFC 3550 §5.1 (random start, 16-bit seq, 8 kHz example), §6.4.1 and A.1 (extended seq, MAX_DROPOUT=3000, MAX_MISORDER=100, MIN_SEQUENTIAL=2); RFC 3551 L16 byte order; `std::endian` (C++20) / `std::byteswap` (C++23) availability on your toolchain.
