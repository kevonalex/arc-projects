# Pack 01 — Bit Operations (Etherstack interview worksheet)

> **Who this is for:** you start from zero. Nothing is assumed except that you can count in decimal and read C++.
> **Rule of the pack:** every trick is *derived* bit by bit before it is stated. If you catch yourself memorising, go back to the derivation.
> **Data rule:** every signal, packet and word in this pack is **synthetic**. No live public-safety receive, no real talkgroup traffic.
> **Companion files:** [`answers.md`](answers.md) · [`cheatsheet.md`](cheatsheet.md) · `code/` (stubs in `code/include/`, tests in `code/tests/`, reference in `code/solutions/`).

Section map (the cheatsheet cites these numbers): **§1** Why · **§2** Prerequisites · **§3** Concept build-up (3.1–3.8) · **§4** Worked example · **§5** Questions Q1–Q16 · **§6** Self-assessment.

---

## §1 Why this matters

Land-mobile-radio (LMR) air interfaces are *bit-packed*. A P25 Network ID is 64 bits on the air, and the first 16 data bits of it hold a 12-bit "Network Access Code" (NAC) glued to a 4-bit "Data Unit ID" (DUID). A DMR burst slices a 20-bit link-control word into fields. A gateway that bridges radio frames to IP must pull those fields out, compare them, and push new ones back in — thousands of times per second, on a microcontroller or an RTOS task with no room for mistakes.

| Where you will meet it | What you do with bits |
|---|---|
| **LMR (P25 / DMR)** | Extract NAC/DUID/colour-code/slot fields; insert a rewritten NAC for a gateway that re-labels traffic |
| **Gateway / network** | Pull `version`, `flags`, `fragment offset` out of header words (Pack 03) |
| **Embedded** | Set/clear one flag bit in a peripheral register without disturbing its 31 neighbours; sign-extend a 12-bit ADC sample |
| **Interviews** | "Count the bits", "is it a power of two", "extract bits [a:b]" are the standard warm-ups. Interviewers watch whether you can *derive* the answer when you forget it. |

**What you will be able to do:** write set/clear/toggle, extract-field, insert-field, sign-extend, lowest-set-bit and popcount from scratch, and *prove* each one on paper, including the traps (`len == 32`, signed shifts, operator precedence).

**Failure if you skip this:** a gateway that mis-extracts a 12-bit NAC as 0 silently drops *every* frame — the radio looks "dead", logs show nothing wrong. We will reproduce that failure in §3.3.

---

## §2 Prerequisites

You need five facts. If any is shaky, do the 2-minute drill beside it.

1. **Binary place value.** Bit *i* is worth 2^*i*. `0b0101` = 4 + 1 = 5. *Drill:* write 37 in binary (answer: `100101`).
2. **Hex is just grouped binary.** One hex digit = 4 bits. `0x2935` = `0010 1001 0011 0101`. *Drill:* write `0xB4` in binary (`1011 0100`).
3. **Bit numbering.** In this pack **bit 0 is the least-significant bit (LSB), the right-most**. Many radio specs number MSB-first; always check (see misconception box 3.3-M).
4. **Unsigned vs signed.** `uint32_t` has 32 bits and *wraps* on overflow (defined behaviour). `int32_t` overflow is undefined behaviour. We use **unsigned** for all bit work.
5. **The three bitwise gates, per bit.**

| a | b | a&b | a\|b | a^b |
|---|---|-----|------|-----|
| 0 | 0 | 0 | 0 | 0 |
| 0 | 1 | 0 | 1 | 1 |
| 1 | 0 | 0 | 1 | 1 |
| 1 | 1 | 1 | 1 | 0 |

Read the table as *behaviours*, not as symbols: **AND with 0 forces 0, AND with 1 keeps. OR with 1 forces 1, OR with 0 keeps. XOR with 1 flips, XOR with 0 keeps.** Everything in this pack is built from those six sentences.

Also: `~x` flips every bit. `x << n` moves bits left (zero fill), `x >> n` moves right (zero fill for unsigned). **C++ rule:** shifting by *n ≥ width of the type* is undefined behaviour.

**Operator precedence (highest first, the part that bites):** `( )`, then `+ -`, then `<< >>`, then `< >`, then `==`, then `&`, then `^`, then `|`. So `a & b == c` is `a & (b == c)` — a classic bug.

How to run the code: `cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure` (details in `code/CMakeLists.txt`). Tests named `student.*` run your stubs; `solution.*` run the reference.

---

## §3 Concept build-up

### 3.1 A word is a row of switches (concrete first)

Picture a **8-switch panel** (a byte). The panel currently reads `1010 0101` (= `0xA5`). Switch numbers run right to left: 0 on the right.

```
bit:    7 6 5 4 3 2 1 0
value:  1 0 1 0 0 1 0 1     = 0xA5
```

Questions you will ask of every register and header word:
- *Is switch 2 on?* (test)
- *Turn switch 3 on / off / flip it* (set / clear / toggle)
- *Read switches 4–7 as one number* (extract a field)
- *Overwrite switches 4–7 with a new number without touching the rest* (insert a field)

A **mask** is a second panel whose "on" switches mark *the switches you care about*.

> **Misconceptions box 3.1**
> 1. *"`0xA5` is a number, so bits are arithmetic."* Bits are positions; the number is just one reading of them.
> 2. *"Bit 1 is the first bit."* Programmers say bit 0 is first. A spec may say bit 1 is the *most* significant. Always find the convention before coding.
> 3. *"A mask is a number I add."* A mask is a pattern you AND/OR/XOR with. Adding it breaks as soon as a bit is already set (§3.2).

### 3.2 Set, clear, toggle — derived from the gate behaviours

Goal: change **only** bit *p* of *x*.

**Step 1 — build a panel with only switch *p* on.** `1u << p`. For p=3: `0000 1000`.

**Step 2 — pick the gate whose "force" behaviour does what we want, and whose "keep" behaviour protects the other bits.** The mask has 0 everywhere except at *p*, so a gate with "0 = keep" is needed for the other 7 positions.

| Want | Gate with "0 → keep" | Why it forces the right thing at *p* | Result |
|---|---|---|---|
| **Set** bit *p* | OR (`\|`): 0 keeps, 1 forces 1 | mask has 1 at *p* → forced to 1 | `x \| (1u << p)` |
| **Toggle** | XOR (`^`): 0 keeps, 1 flips | mask 1 at *p* → flips | `x ^ (1u << p)` |
| **Clear** | AND (`&`): **1** keeps, 0 forces 0 | need 0 at *p* and 1 elsewhere → that is `~(1u<<p)` | `x & ~(1u << p)` |

Concrete: `x = 1010 0101`, p = 3.
- set: `1010 0101 | 0000 1000 = 1010 1101`
- toggle: `1010 0101 ^ 0000 1000 = 1010 1101` (bit was 0 → now 1, same here); applying again gives back `1010 0101`.
- clear: `~0000 1000 = 1111 0111`; `1010 0101 & 1111 0111 = 1010 0101` (bit was already 0 → unchanged).

**Test** bit *p*: shift it down to position 0 and keep one bit: `(x >> p) & 1u`.

**Why not `+` / `-`?** Set with `x + (1u<<p)`: if the bit was already on, the 1 carries into bit *p+1* and corrupts a neighbour. `0xA5 + 0x04` (bit 2 already set) = `0xA9`, bits 2 and 3 both changed. OR is *idempotent* (doing it twice = once); `+` is not.

> **Misconceptions box 3.2**
> 1. *"`x & (1u<<p)` clears the bit."* It keeps *only* that bit and zeros everything else. Clearing needs the **inverted** mask.
> 2. *"`~(1<<p)` is fine with signed `1`."* `1<<31` on a signed int is UB before C++20. Write `1u`.
> 3. *"`x & 1<<p` tests the bit."* Precedence: `<<` before `&`, so it parses correctly, but it returns `1<<p` (not 0/1) when set. Convert with `!= 0`.

### 3.3 Extract a field — `(x >> pos) & ((1u << len) - 1)` (deep dive)

#### 3.3.1 Concrete first: the P25 NID word

Synthetic example. P25 frames carry (after the frame sync) a Network ID. Its first 16 data bits are `[ NAC : 12 bits ][ DUID : 4 bits ]`, NAC first (high bits). *(Layout from TIA-102.BAAA-A — VERIFY the exact bit order/offsets against the standard you are given; the NID is then protected by BCH parity, see Pack 05.)*

Take a made-up received word **`0x2935`**:

```
hex   :   2    9    3    5
binary: 0010 1001 0011 0101
bit   : 15....................0
         |<---- NAC 12 ---->|<DUID 4>|
         0010 1001 0011      0101
```

By eye: NAC = `0010 1001 0011` = `0x293` (659 decimal); DUID = `0101` = 5. We want C++ to do that by eye for us.

*(DUID value 5 means "Logical Link Data Unit 1" in the standard — VERIFY the table; our code does not depend on the meaning.)*

#### 3.3.2 Step A — bring the field to the bottom (shift)

The field must sit at bit 0 so that its numeric value falls out. The NAC's lowest bit is bit 4 (`pos = 4`). `x >> 4` slides everything right by 4 places:

```
x       = 0010 1001 0011 0101
x >> 4  = 0000 0010 1001 0011      = 0x0293
```

The DUID bits fell off the right end. But look at the **left**: for 16-bit data sitting in a 32-bit variable there are 16 leading zeros anyway, so here it looks finished. It is not finished in general — the next picture shows why.

Take the DUID instead (`pos = 0, len = 4`): `x >> 0 = 0x2935`. That is **not** 5. The NAC bits are still sitting above the DUID. The shift alone only fixed the *bottom* edge of the field. We still must remove everything **above** its top edge.

#### 3.3.3 Step B — cut off everything above the field (mask)

We need a panel with exactly `len` ones at the bottom: `0000…0001111` for len=4. AND with it uses the gate rule: **AND with 1 keeps, AND with 0 forces 0.**

```
x >> 0  = 0010 1001 0011 0101
mask    = 0000 0000 0000 1111     (len = 4 ones)
AND     = 0000 0000 0000 0101  = 5 ✓
```

#### 3.3.4 Deriving the mask `(1u << len) - 1`, bit by bit

*Concrete:* want 4 ones. Start from `1u << 4`: a lone 1 at bit 4.

```
1u << 4        = 0001 0000     (decimal 16)
minus 1:
                 0001 0000
               -         1
                 ---------
```
Subtract by hand, right to left. Bit 0: `0 − 1` → can't, borrow from the left. The nearest 1 is at bit 4; every zero between is forced to "lend", turning into 1 after borrowing from the one to its left:

```
borrow ripples:  bit4 1→0,  bits 3,2,1 0→1,  bit0 (10)−1 = 1
result         = 0000 1111     (decimal 15) ✓
```

**General proof.** `1u << len` is 2^len. All ones in the low `len` bits is 2^(len−1) + … + 2 + 1 = 2^len − 1 (geometric sum, or: adding 1 to `111…1` carries all the way to 2^len, so `111…1 = 2^len − 1`). ∎

Bit-level reason you will reuse in §3.7: **subtracting 1 flips the lowest 1 to 0 and every 0 below it to 1.** (Same borrow.)

#### 3.3.5 Assemble it

```cpp
uint32_t extract_field(uint32_t x, unsigned pos, unsigned len) {
  return (x >> pos) & ((1u << len) - 1u);
}
```

Read it aloud: "slide so the field's bottom is at bit 0, then keep only `len` bits."

Check on NAC (`pos=4, len=12`): `mask = (1<<12) − 1 = 0xFFF`; `0x2935 >> 4 = 0x293`; `0x293 & 0xFFF = 0x293` ✓.
Check on DUID (`pos=0, len=4`): `0x2935 & 0xF = 5` ✓.

*Why order shift-then-mask?* Mask is always the same simple low-ones pattern; if you mask first you must shift the mask too: `(x & (mask << pos)) >> pos` — equivalent, but two shifts.

#### 3.3.6 The `len == 32` problem

`1u << 32` shifts a 32-bit value by 32. In C++ that is **undefined behaviour**, not "zero" and not "wraps".

What actually happens, which is *worse* than a crash:
- x86 `SHL` masks the count to 5 bits, so `32 & 31 = 0` → `1u << 32 == 1` → mask `= 1 − 1 = 0` → **extract returns 0 for every input**. Wrong, silent, and only for `len == 32`.
- A compiler that can see the constant may fold it to anything.
- UBSan says: `shift exponent 32 is too large for 32-bit type 'unsigned int'` — which is why our tests run under `-fsanitize=undefined`.

Fix: handle the full-width case where the mask is *all ones*:

```cpp
inline uint32_t low_mask(unsigned len) {
  return len >= 32u ? 0xFFFFFFFFu : ((1u << len) - 1u);
}
```
Alternative with no branch: `~0u >> (32 - len)` — but that breaks at `len == 0` (shift by 32). Every formulation has one edge; know *which*.

#### 3.3.7 Failure modes (reproduce each on paper)

| # | Bad code | What happens on `x=0x2935, pos=4, len=12` | Root cause |
|---|---|---|---|
| F1 | `x >> pos & 1u << len - 1` (parentheses missing) | `len - 1` happens first, so mask = `1u << 11 = 0x800`; `0x293 & 0x800 = 0` → NAC reads as 0 | `-` binds tighter than `<<`, which binds tighter than `&` |
| F2 | `(x >> pos) & (1 << len) - 1` with signed `1` and len = 31 | `1<<31` is signed overflow UB | use `1u` |
| F3 | `len == 32` | returns 0 | §3.3.6 |
| F4 | `pos + len > 32` | field silently zero-padded at the top: `extract(0xFFFFFFFF, 28, 8)` = `0xF`, not `0xFF` | field does not exist; add a precondition/assert |
| F5 | forgetting the mask on a **signed** `x` | `-1 >> 4 = -1` (arithmetic shift fills with the sign bit), so you get all ones, not `0x0FFFFFFF` | signed right shift is sign-extending; the mask is what makes it safe. Prefer unsigned `x` |
| F6 | `uint8_t b = 0x80; b << 24` | `b` promotes to **signed int** → `0x80 << 24` overflows int → UB | integral promotion; cast to `uint32_t` first |
| F7 | assuming bit 0 is MSB | NAC read from the wrong end | check the spec's numbering (box below) |

> **Misconceptions box 3.3-M**
> 1. *"The mask goes first, like `x & mask >> pos`."* That ANDs the *unshifted* x with `mask >> pos`, grabbing bits from the wrong place.
> 2. *"The mask `(1u<<len)-1` is a 'magic formula'."* It is "a 1 above the field, minus 1 → ones below" (§3.3.4). If you forget it, rebuild it with len = 3: `1000 − 1 = 0111`.
> 3. *"`len == 32` is rare, ignore it."* Registers that are 32 bits wide, or `extract(x, 0, width_of_T)` generic code, hit it immediately; and it fails *silently*.
> 4. *"Shifting a 32-bit value by 32 gives 0."* UB. Hardware often gives *unchanged*, not 0.
> 5. *"The spec's 'bits 1–12' are my bits 1–12."* Specs frequently number from the MSB or from 1. Translate to `pos` with a diagram first.
> 6. *"Signed vs unsigned doesn't matter for extracting."* It matters for `>>` (sign fill) — the mask hides it, only if you remember the mask.

**Self-check (do not skip):** use only the definition to extract `len=3, pos=2` from `x = 0b1011_0100`. (Shift: `0b0010_1101`; mask `0b111`; result `0b101` = 5.)

### 3.4 Insert a field

Goal: overwrite bits `[pos, pos+len)` of *x* with *value*, leaving all other bits untouched. Two moves — **clear the hole, drop the value in**:

1. Build the hole mask: `field = ((1u << len) - 1u) << pos` (the low-ones mask, slid to the field).
2. Clear the hole: `x & ~field` (AND with 0 forces 0 only where the field is).
3. Put the value in place **and clip it**: `(value << pos) & field`.
4. Combine: they never overlap, so `|` merges them.

```cpp
(x & ~field) | ((value << pos) & field)
```

Concrete: `x = 0xFFFF`, write DUID-like 4 bits `0x2` at `pos=4`.
```
field   = 0x00F0
~field  = 0xFF0F      x & ~field = 0xFF0F
value<<4= 0x0020      &field     = 0x0020
OR      = 0xFF2F
```

Why the clip? If `value = 0x1F` (5 bits) goes into a 4-bit field, bit 4 of `value` lands on the *neighbour* field. Without `& field` the neighbour is corrupted — in a P25 NID that is a **wrong NAC**, again silently.

> **Misconceptions box 3.4**
> 1. *"OR the value in."* OR can only *set* bits. Old 1s inside the field stay set. You must clear first.
> 2. *"Callers always pass in-range values."* The one who doesn't is the bug at 3 a.m. Clip.
> 3. *"`~mask` needs the mask shifted first."* Yes — shift the mask to `pos` *before* inverting.

### 3.5 Two's complement from first principles (needed for §3.6 and §3.7)

**Concrete: an odometer.** A 4-bit counter wraps from `1111` back to `0000`. Count *down* from 0: `0000 → 1111 → 1110 → 1101 …`. So the pattern one step below 0 is `1111`: **we choose to call `1111` "−1"** because adding 1 to it gives 0 (with the carry thrown away). That is the *entire* idea of two's complement: negative numbers are "the pattern that adds to zero".

Define −*x* as the 4-bit pattern *y* with *x + y ≡ 0 (mod 16)*.

**Derive −x.** For any x, `x + ~x`: in each column exactly one of the two bits is 1, so the sum in every column is 1 and **no carries occur**: `x + ~x = 1111` (= 15 = 2⁴−1). Add 1 more: `x + ~x + 1 = 10000`, whose carry-out is discarded → 0. Therefore **−x = ~x + 1**. ∎

Concrete: x = 5 = `0101`. `~x = 1010`, `+1 = 1011` (= 11). Check: 5 + 11 = 16 ≡ 0 ✓. So `1011` *is* −5.

All 16 patterns in 4 bits:

| bits | 0000 | 0001 | 0010 | 0011 | 0100 | 0101 | 0110 | 0111 | 1000 | 1001 | 1010 | 1011 | 1100 | 1101 | 1110 | 1111 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| unsigned | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
| signed | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | −8 | −7 | −6 | −5 | −4 | −3 | −2 | −1 |

A pattern whose top bit is 1 and unsigned value is *v* means *v − 16* (generally *v − 2ⁿ*). The top bit is the "sign". Asymmetry: −8 exists but +8 does not; `−(−8) = −8` (overflow). In `int32_t`, `-INT32_MIN` is UB.

> **Misconceptions box 3.5**
> 1. *"The top bit is just a sign flag; −5 is `1101`."* That is sign-magnitude, a different system (two zeros, hard adders). Real hardware: `1011`.
> 2. *"Two's complement is a trick to memorise."* It is the odometer: wrap-around makes `1111` the number before 0.
> 3. *"Negating an unsigned value is illegal."* It is well-defined: `0u - x` wraps modulo 2ⁿ, and equals `~x + 1`.

### 3.6 Sign extension — `(v ^ m) - m`

**Problem (embedded/radio):** a 12-bit ADC/IQ sample arrives in the low 12 bits of a 32-bit word. Its meaning is two's complement at 12 bits. If you cast it, `0xF85` is read as +3973, not −123. We need a 32-bit pattern with the *same meaning*: replicate the top bit of the 12-bit value into the 20 bits above.

**Concrete first, 4 bits.** Let `m = 1 << (bits−1) = 1000` (just the sign bit). Compute `(v ^ m) − m` for **every** v:

| v (bits) | v unsigned | v ^ 1000 | (v^m) | (v^m) − 8 | true 4-bit signed value | match |
|---|---|---|---|---|---|---|
| 0000 | 0 | 1000 | 8 | 0 | 0 | ✓ |
| 0001 | 1 | 1001 | 9 | 1 | 1 | ✓ |
| 0010 | 2 | 1010 | 10 | 2 | 2 | ✓ |
| 0011 | 3 | 1011 | 11 | 3 | 3 | ✓ |
| 0100 | 4 | 1100 | 12 | 4 | 4 | ✓ |
| 0101 | 5 | 1101 | 13 | 5 | 5 | ✓ |
| 0110 | 6 | 1110 | 14 | 6 | 6 | ✓ |
| 0111 | 7 | 1111 | 15 | 7 | 7 | ✓ |
| 1000 | 8 | 0000 | 0 | −8 | −8 | ✓ |
| 1001 | 9 | 0001 | 1 | −7 | −7 | ✓ |
| 1010 | 10 | 0010 | 2 | −6 | −6 | ✓ |
| 1011 | 11 | 0011 | 3 | −5 | −5 | ✓ |
| 1100 | 12 | 0100 | 4 | −4 | −4 | ✓ |
| 1101 | 13 | 0101 | 5 | −3 | −3 | ✓ |
| 1110 | 14 | 0110 | 6 | −2 | −2 | ✓ |
| 1111 | 15 | 0111 | 7 | −1 | −1 | ✓ |

All 16 rows agree, which is a complete proof for 4 bits.

**General proof (any width n).** Let `m = 2ⁿ⁻¹`.
- If the top bit of *v* is 0, XOR with *m* sets it: `v ^ m = v + m`. Then `(v + m) − m = v` ✓ (and *v* is non-negative, true value = *v*).
- If the top bit is 1, XOR clears it: `v ^ m = v − m`. Then `(v − m) − m = v − 2m = v − 2ⁿ` ✓ (the two's-complement value of a pattern with top bit set).
∎ The XOR is a "flip the sign bit" that re-biases the number so that a plain subtraction does the job.

**Plain-English why it must be true:** XOR moves the range [0, 2ⁿ) to a window centred differently; subtracting *m* slides that window to [−m, m). Both steps are shifts of the number line by exactly *m*.

**Do the arithmetic unsigned.** `(v ^ m) - m` in `uint32_t` wraps correctly; cast to `int32_t` at the end (modular on all mainstream compilers, defined in C++20). First **mask `v` to n bits**, otherwise garbage above bit n−1 survives.

**Failure if ignored:** `static_cast<int32_t>(0xF85)` = +3973; a decoder treating a −123 sample as +3973 produces a full-scale click. Also the common shortcut `(int32_t)(v << (32-n)) >> (32-n)` relies on implementation-defined behaviour (before C++20) and breaks for n = 0.

> **Misconceptions box 3.6**
> 1. *"Sign extension means padding with zeros."* That is *zero* extension. Sign extension copies the top bit.
> 2. *"I can skip the mask on `v`."* Dirty upper bits (e.g. from a packed word) break the result.
> 3. *"`int x = v;` already extends."* Only if `v` is already a signed type of the *same* width as its meaning.
> 4. *"It only works for 4 bits."* The proof never used n = 4.

### 3.7 `x & -x` and `x & (x-1)` — after two's complement

Write any non-zero x as **A 1 0ᵏ**: some prefix **A**, then the *lowest* 1, then *k* zeros (k ≥ 0).
Example: `x = 0110 1000` → the lowest 1 is bit 3, so k = 3 zeros sit below it and A = `0110` (bits 7..4) sits above it.

**x − 1 (borrow, from §3.3.4):** the zeros below the lowest 1 become ones, that 1 becomes 0, A is untouched:
```
x       =  A 1 0ᵏ
x − 1   =  A 0 1ᵏ
x & (x−1) = (A&A) (1&0) (0ᵏ&1ᵏ) = A 0 0ᵏ     → lowest set bit CLEARED
```
Concrete: `0110 1000 & 0110 0111 = 0110 0000`.

**−x = ~x + 1:**
```
x       =  A  1  0ᵏ
~x      = ~A  0  1ᵏ
~x + 1  = ~A  1  0ᵏ      (the +1 carries through the 1ᵏ run, turns them to 0, lands on the 0 → 1)
x & −x  = (A&~A) (1&1) (0ᵏ&0ᵏ) = 0 1 0ᵏ      → ONLY the lowest set bit survives
```
(`A & ~A` is all zeros: a bit and its complement are never both 1.)
Concrete: `0110 1000 & 1001 1000 = 0000 1000`.

**Power of two test.** A power of two has exactly one 1, so clearing it leaves 0: `x != 0 && (x & (x-1)) == 0`. **x = 0 trap:** `0 & (0−1) = 0 & 0xFFFFFFFF = 0` → the test alone says "yes, 0 is a power of two". Add `x != 0`.

**Unsigned note:** write `x & (0u - x)` (not `x & -x`) to avoid the MSVC "unary minus on unsigned" warning; same bits.

> **Misconceptions box 3.7**
> 1. *"`x & (x-1)` gives the lowest bit."* No: it *removes* it. `x & -x` *isolates* it.
> 2. *"These tricks are separate things to memorise."* Both are the same fact: the run `1 0ᵏ` is flipped by ±1.
> 3. *"`x-1` underflows for x=0 and is UB."* For **unsigned** it wraps to all-ones; defined.

### 3.8 Popcount — Kernighan's loop

```cpp
unsigned n = 0;
while (x) { x &= x - 1; ++n; }
```
**Why exactly once per set bit:** by §3.7 each `x &= x−1` removes *exactly one* set bit (the lowest) and changes nothing else. So the popcount drops by exactly 1 per pass, and the loop stops when it reaches 0 — after **popcount(x)** passes. A 32-bit word with 3 bits set takes 3 passes, `0x80000000` takes 1 (the naive shift-and-test loop takes 32).

Trace `x = 0xB4 = 1011 0100` (popcount 4): `1011 0100 → 1011 0000 → 1010 0000 → 1000 0000 → 0` : 4 iterations.

**Failure if you write it the naive way with a signed int:** `while (x) { n += x & 1; x >>= 1; }` with negative `x` loops forever on arithmetic-shift compilers (sign bit refills).

Real code: `std::popcount` (C++20, `<bit>`), `__builtin_popcount` (GCC/Clang), or the `POPCNT` instruction. Kernighan wins when words are sparse.

> **Misconceptions box 3.8**
> 1. *"Kernighan loops 32 times."* It loops *popcount* times.
> 2. *"It counts the bits by looking at each position."* It never looks at positions; it deletes ones.
> 3. *"Popcount is only a puzzle."* Hamming *weight* is how you measure code distance (Pack 05) and parity (`popcount & 1`).

---

## §4 Fully worked example — decode a P25-style word, then an ADC sample

**Part A. Received (synthetic) 16-bit word `0x2935`; gateway policy: accept only NAC `0x293`; if the DUID is 5 relabel NAC to `0xF7E`.**

1. Binary: `0010 1001 0011 0101`.
2. **Extract NAC** (pos 4, len 12): `mask = (1u<<12)−1 = 0000 1111 1111 1111`; `x>>4 = 0000 0010 1001 0011`; AND → `0x293` ✓ accept.
3. **Extract DUID** (pos 0, len 4): `mask = 0xF`; `0x2935 & 0xF = 5`.
4. **Insert new NAC `0xF7E`** at pos 4 len 12: `field = 0xFFF << 4 = 0xFFF0`; `x & ~field = 0x2935 & 0x000F = 0x0005`; `(0xF7E << 4) & 0xFFF0 = 0xF7E0`; OR → `0xF7E5`.
5. Re-extract to check: NAC = `0xF7E`, DUID = 5 still ✓ (neighbouring field untouched).
6. Code: `parse_p25_nid(0x2935)` → `{0x293, 5}`; `build_p25_nid(0xF7E, 5)` → `0xF7E5`.

*(Real frames carry BCH parity after these 16 bits; a rewriting gateway must recompute it — Pack 05. Never mutate protected bits without re-encoding.)*

**Part B. 12-bit sample `0xF85` → signed.**
1. `m = 1 << 11 = 0x800`.
2. `v & 0xFFF = 0xF85`.
3. `v ^ m = 0xF85 ^ 0x800 = 0x785` (= 1925).
4. `0x785 − 0x800 = −123` ✓. Check by definition: top bit 1, so value = 3973 − 4096 = −123.

**Part C. Scan a status register `0x68` for the lowest pending interrupt.**
`0x68 = 0110 1000`; `x & −x = 0000 1000` → bit 3 is the lowest-numbered pending source. Clear it after servicing: `0x68 & 0x67 = 0x60`.

---

## §5 Questions

Each question has a **type**, a **context line**, and an answer in `answers.md` (same number). Attempt on paper first.

**Q1 — Explain.** *Context: embedded peripheral register.* A flag register already has bit 2 set. Explain, using the gate table, why `reg |= (1u << 2)` is safe but `reg += (1u << 2)` is not. Give the numeric result for `reg = 0xA5` for both.

**Q2 — Calculate.** *Context: radio status byte.* Start with `0xA5`. Compute by hand (in binary): (a) set bit 1; (b) clear bit 0; (c) toggle bit 7; (d) test bit 5. Show the mask each time.

**Q3 — Calculate.** *Context: P25 NID, synthetic.* For each word extract NAC (pos 4, len 12) and DUID (pos 0, len 4): (a) `0x2935` (b) `0xF7E0` (c) `0x293C` (d) `0x001F`. Give hex.

**Q4 — Explain.** *Context: why the extract mask works.* Derive `low_mask(5)` bit by bit from `1u << 5`. Then explain in two sentences why AND with it keeps exactly 5 bits.

**Q5 — Predict.** *Context: gateway field parser.* (a) Evaluate `x >> pos & 1u << len - 1` for `x=0x2935, pos=4, len=12`. (b) What does `(x >> 0) & ((1u << len) - 1)` return for `len = 32, x = 0xDEADBEEF` on x86-64 with no sanitizer, and what does UBSan say?

**Q6 — Trace.** *Context: relabelling the NAC.* Trace `insert_field(0xFFFF, 4, 4, 0x2)` writing every intermediate value in hex and binary.

**Q7 — Find the bug.** *Context: embedded driver code review.* Three bugs in one function:
```cpp
uint32_t insert_field(uint32_t x, unsigned pos, unsigned len, uint32_t v) {
  uint32_t mask = (1u << len) - 1;
  return (x & ~mask) | (v << pos);
}
```
List each bug, a failing input for each, and the fix. Then comment on `(int32_t)(v << (32-n)) >> (32-n)` as a sign-extension.

**Q8 — Calculate.** *Context: checksum/CRC groundwork.* In 8 bits compute `−x` for x = `0x14` and x = `0x80` using `~x + 1`; verify `x + (−x) ≡ 0`. What is unusual about `0x80`?

**Q9 — Calculate.** *Context: DSP/IQ samples.* (a) Sign-extend `0x2A` and `0x15` as 6-bit values. (b) Fill the full 3-bit table for `(v^m)−m`. (c) Sign-extend `0xFFF` and `0x800` at 12 bits.

**Q10 — Predict.** *Context: interrupt controller.* For `x = 0x68` give `x & -x` and `x & (x-1)` in binary. Then for `x = 0` and `x = 0x80000000`.

**Q11 — Explain.** *Context: buffer sizing in a network stack.* Why does `x & (x-1) == 0` identify powers of two? Why must `x == 0` be excluded? Why does `x & (x-1) == 0` as written in C++ not do what you want (precedence)?

**Q12 — Trace.** *Context: Hamming weight for FEC.* Trace Kernighan on `0xB4` and on `0x00010001`. How many loop iterations each? How many would shift-and-test need?

**Q13 — Implement.** *Context: gateway library.* Implement every stub in `code/include/bitops.h`. Run `ctest`; `student.*` tests must go green and the sanitizers must stay silent. Then diff against `code/solutions/bitops.h`.

**Q14 — Connect.** *Context: Pack 03 BitReader, Pack 05 FEC.* (a) A `BitReader::read(n)` that crosses a byte boundary: which functions from this pack does it conceptually repeat? (b) Where does `popcount & 1` appear in FEC (Pack 05)? (c) Where does the borrow pattern of §3.3.4 reappear in this pack?

**Q15 — Interview.** *Context: phone screen warm-up.* "Count the set bits of a 32-bit word. Give two approaches, their complexity, and when you'd prefer each. How would you do it in C++20?"

**Q16 — Interview.** *Context: embedded RTOS.* "An ISR and the main loop both do `STATUS |= FLAG_X` / `STATUS &= ~FLAG_Y` on the same memory-mapped register. What can go wrong and what are two fixes?"

---

## §6 Self-assessment

Tick only if you can do it **on paper, with no notes, including the proof**.

- [ ] I can derive set/clear/toggle from the AND/OR/XOR "force/keep" rules.
- [ ] I can extract `(pos, len)` from a word and explain why the mask has `len` ones.
- [ ] I can name three extract-field failure modes and give an input that exposes each.
- [ ] I can explain why `len == 32` is a problem and give two ways to handle it.
- [ ] I can insert a field *and* explain why the value must be clipped.
- [ ] I can derive `−x = ~x + 1` and show the 4-bit table.
- [ ] I can sign-extend with `(v ^ m) − m` and prove it for every 4-bit value.
- [ ] I can derive `x & −x` and `x & (x−1)` from the `A 1 0ᵏ` picture.
- [ ] I can say why Kernighan loops popcount times.
- [ ] `ctest` shows all `solution.*` and (after Q13) all `student.*` tests passing, ASan/UBSan clean.

**Rapid fire (answers at the end of `answers.md`):** (1) `0xFF & ~(1u<<4)`? (2) `(1u<<8)−1`? (3) 6-bit `110000` signed? (4) lowest set bit of `0x50`? (5) popcount of `0xF0F0`?

**VERIFY items for this pack:** P25 NID layout and DUID table (TIA-102.BAAA-A); that your target compiler is C++17/20 and defines out-of-range unsigned→signed conversion as modular (all mainstream do; C++20 mandates it).
