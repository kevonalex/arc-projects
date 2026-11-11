# Pack 01 — Answers (bit operations)

Format per question: **Answer & reasoning** → **Common wrong answers** (*why tempting* / *why wrong*). Section numbers (`§`) refer to [`worksheet.md`](worksheet.md). All data synthetic.

---

## Q1 — OR vs `+` on an already-set bit (Explain)

**Answer.** Per bit, OR's rule is "1 forces 1, 0 keeps" (§2 gate table). Bit 2 is already 1, OR with 1 → 1, nothing else changes: `0xA5 | 0x04 = 1010 0101 | 0000 0100 = 1010 0101 = 0xA5`. Addition has no such rule; it adds numbers. `1 + 1 = 10` in binary, so the 1 stays in column 2 as 0 and **carries into column 3**: `0xA5 + 0x04 = 0xA9 = 1010 1001` — bit 2 became 0 (the flag was *cleared*) and bit 3 became 1 (an unrelated flag was *set*). OR is idempotent (`x|m|m == x|m`); `+` is not.

**Common wrong answers.**
- *"Both give 0xA5 / both are fine."* — Tempting because when the bit is **clear**, `+` and `|` give the same result and many tests start from zero. Wrong because the whole point of flag code is that the bit may already be set.
- *"`+` is wrong only because it might overflow."* — Tempting (arithmetic mindset). Wrong: the corruption happens long before overflow, in the neighbouring bit.

---

## Q2 — Set / clear / toggle / test on `0xA5` (Calculate)

`0xA5 = 1010 0101`.
- (a) set bit 1: mask `0000 0010`; OR → `1010 0111` = **0xA7**.
- (b) clear bit 0: mask `~0000 0001 = 1111 1110`; AND → `1010 0100` = **0xA4**.
- (c) toggle bit 7: mask `1000 0000`; XOR → `0010 0101` = **0x25**.
- (d) test bit 5: `0xA5 >> 5 = 0000 0101`, `& 1 = 1` → **true** (bit 5 of `1010 0101`, counting 7..0, is 1).

**Common wrong answers.**
- (b) *`0xA5 & 0x01 = 0x01`.* — Tempting: "AND with the mask to clear". Wrong: that *isolates* bit 0 and kills the other seven bits; clearing needs the **inverted** mask (§3.2, box 2.1).
- (d) *"Bit 5 is 0 because the 5th digit from the left is 0."* — Tempting: counting from the left, 1-based. Wrong: bit numbers run from the right, starting at 0 (§3.1).
- (c) *`0xA5 ^ 0x07`* — Tempting: confusing "bit 7" with "the value 7". Wrong: the mask is `1u << 7 = 0x80`.

---

## Q3 — NAC/DUID extraction (Calculate)

NAC = `(w >> 4) & 0xFFF`, DUID = `w & 0xF`.
- (a) `0x2935`: NAC **0x293**, DUID **5**.
- (b) `0xF7E0`: `>>4 = 0x0F7E`; NAC **0xF7E**, DUID **0**.
- (c) `0x293C`: NAC **0x293**, DUID `0xC` = **12**.
- (d) `0x001F`: `>>4 = 0x001`; NAC **0x001**, DUID **0xF**.

**Common wrong answers.**
- *DUID of (a) = 0x2935 (forgot the mask).* — Tempting: "shift by 0, nothing to do". Wrong: shifting only fixes the bottom edge; the NAC bits above remain (§3.3.2).
- *NAC of (a) = 0x29.* — Tempting: shifting by 8 because "a byte". Wrong: the field starts at bit 4, so shift by 4; hex digit boundaries are 4 bits, so NAC is the *three* hex digits `293`.
- *NAC of (d) = 0x1F.* — Tempting: forgetting that the DUID's 4 bits must be dropped. Wrong: `0x1F >> 4 = 1`.

---

## Q4 — `low_mask(5)` derivation (Explain)

`1u << 5 = 0010 0000` (32). Subtract 1 column by column from the right: bit 0: `0 − 1` needs a borrow; the nearest 1 is at bit 5, so bits 4,3,2,1 each have to lend and become 1; bit 5 gives up its 1 and becomes 0; bit 0 becomes `10 − 1 = 1`. Result `0001 1111` = 31 = 2⁵ − 1.

Why AND keeps 5 bits: AND with 1 keeps the bit, AND with 0 forces 0 (§2). The mask has 1 in exactly bits 0–4, so those five bits of the input pass through, and every higher bit is forced to 0.

**Common wrong answers.**
- *`1u << 5` is the mask (32 = `100000`).* — Tempting: "a mask is a shifted 1". Wrong: that has a *single* 1; AND with it keeps one bit, not five.
- *`(1u << 5) + 1`.* — Tempting: off-by-one in the other direction. Wrong: `100001`; ones must be *below* the 1, which subtraction produces.

---

## Q5 — Precedence and `len == 32` (Predict)

**(a)** `x >> pos & 1u << len - 1`: `-` first → `len - 1 = 11`; then `x >> pos = 0x293` and `1u << 11 = 0x800`; then `0x293 & 0x800`. `0x293 = 0010 1001 0011` has bit 11 = 0 → **0**. NAC silently becomes 0.

**(b)** On x86-64 the hardware `SHL` uses only the low 5 bits of the count, so `1u << 32` behaves like `1u << 0 = 1`; mask `= 1 − 1 = 0`; result **0** (not `0xDEADBEEF`). Strictly it is undefined behaviour, so an optimising compiler may do anything. UBSan reports: `runtime error: shift exponent 32 is too large for 32-bit type 'unsigned int'`.

**Common wrong answers.**
- *(a) 0x293.* — Tempting: reading the expression as the "intended" version. Wrong: C++ obeys precedence, not intent (§3.3.7 F1).
- *(b) `0xDEADBEEF` ("shifting by 32 gives 0, 0−1 wraps to all-ones").* — Tempting: "0 − 1 = 0xFFFFFFFF is exactly the right mask!" Wrong: shift-by-width is *not* defined to give 0; real hardware gives 1 here. Only the sanitizer, the standard, or a test reveals it.
- *(b) "Compile error."* — The compiler may *warn* for constants but this is a runtime value.

---

## Q6 — Trace `insert_field(0xFFFF, 4, 4, 0x2)` (Trace)

1. `low_mask(4) = 0xF = 0000 1111`.
2. `field = 0xF << 4 = 0xF0 = 1111 0000`.
3. `~field = …1111 0000 1111` (as 32-bit: `0xFFFFFF0F`).
4. `x & ~field = 0xFFFF & 0xFFFFFF0F = 0xFF0F = 1111 1111 0000 1111`.
5. `value << pos = 0x2 << 4 = 0x20`; `& field = 0x20 & 0xF0 = 0x20`.
6. `0xFF0F | 0x20 = 0xFF2F = 1111 1111 0010 1111`.

**Common wrong answers.**
- *`0xFFFF | 0x20 = 0xFFFF`.* — Tempting: "insert = OR". Wrong: OR can't clear the old 1s in the field (§3.4 box 1).
- *`0xFFF2` (value dropped in at bit 0).* — Tempting: forgetting that the value must be shifted to `pos`. Wrong: the field is bits 7..4, so `0x2` must become `0x20` first.
- *`0x002F` (`0xFFFF & 0x2F`).* — Tempting: "AND to keep the right part". Wrong: that destroys the high byte, which must stay untouched.

---

## Q7 — Find the bugs in `insert_field` (Find the bug)

```cpp
uint32_t insert_field(uint32_t x, unsigned pos, unsigned len, uint32_t v) {
  uint32_t mask = (1u << len) - 1;      // BUG 1 (position), BUG 3 (len==32)
  return (x & ~mask) | (v << pos);      // BUG 2 (no clip)
}
```
1. **Mask not shifted to `pos`.** It clears bits `[0, len)` instead of `[pos, pos+len)`. Input `x=0xFFFF, pos=4, len=4, v=2`: `~mask = …FFF0`, `x & ~mask = 0xFFF0`, `| 0x20 = 0xFFF0` → the field is still `F`; expected `0xFF2F`. Fix: `field = mask << pos`.
2. **Value not clipped.** `x=0, pos=0, len=4, v=0x1F` returns `0x1F`: bit 4 leaks into the next field. Expected `0xF`. Fix: `(v << pos) & field`.
3. **`len == 32` is UB** (`1u << 32`). Fix with `low_mask`. (Also `pos >= 32` is UB.)

Fixed:
```cpp
uint32_t field = low_mask(len) << pos;
return (x & ~field) | ((v << pos) & field);
```

**On `(int32_t)(v << (32-n)) >> (32-n)`:** it works on mainstream compilers but leans on (i) unsigned→signed out-of-range conversion (implementation-defined before C++20), (ii) right shift of a negative signed value (implementation-defined, arithmetic in practice; defined in C++20), and (iii) `n == 0` → shift by 32 is UB. If `v` were signed, `<<` into the sign bit is UB before C++20. `(v ^ m) - m` stays in unsigned arithmetic until a single final conversion.

**Common wrong answers.**
- *"Only the mask shift is wrong."* — Tempting because it is the first thing you see. Wrong: after fixing it, `v=0xABC` still overwrites neighbours.
- *"Add `assert(v < (1u<<len))`."* — Tempting as defensive coding. Reasonable for debug, but in a gateway fed by the network the safe behaviour is to clip, not abort.

---

## Q8 — Two's complement negation in 8 bits (Calculate)

- `x = 0x14 = 0001 0100`; `~x = 1110 1011 = 0xEB`; `+1 = 0xEC = 1110 1100`. Check: `0x14 + 0xEC = 0x100` → carry dropped → `0x00` ✓. `0xEC` as signed = 236 − 256 = −20 ✓.
- `x = 0x80 = 1000 0000`; `~x = 0111 1111`; `+1 = 1000 0000 = 0x80`. Check: `0x80 + 0x80 = 0x100 ≡ 0` ✓.

Unusual: `0x80` (−128) is **its own negative**. There is no +128 in 8-bit two's complement (range −128..127). In C++ `-INT32_MIN` is UB; with unsigned `0u - x` it wraps harmlessly.

**Common wrong answers.**
- *`-0x14 = 0x94` (set the top bit).* — Tempting: sign-magnitude thinking. Wrong: `0x94 + 0x14 = 0xA8 ≠ 0` mod 256 (§3.5 box 1).
- *`-0x14 = 0xEB` (just flip).* — Tempting: forgetting `+1`. Wrong: `0x14 + 0xEB = 0xFF`, not 0.
- *`-0x80 = +128 = 0x80` so "it's fine".* — The bit pattern is the same but the *value* is −128, not +128; code that assumes `-x > 0` for negative x breaks.

---

## Q9 — Sign extension numbers (Calculate)

**(a)** 6 bits, `m = 0x20`.
- `0x2A = 10 1010`: `v ^ m = 0x0A = 10`; `10 − 32 = −22`. (Check: 42 − 64 = −22 ✓.)
- `0x15 = 01 0101`: `v ^ m = 0x35 = 53`; `53 − 32 = 21` ✓ (top bit 0 → positive).

**(b)** 3 bits, `m = 4`:

| v | 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
|---|---|---|---|---|---|---|---|---|
| v^100 | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |
| −4 | 0 | 1 | 2 | 3 | −4 | −3 | −2 | −1 |

**(c)** 12 bits, `m = 0x800`: `0xFFF ^ 0x800 = 0x7FF = 2047; 2047 − 2048 = −1`. `0x800 ^ 0x800 = 0; 0 − 2048 = −2048`.

**Common wrong answers.**
- *(a) `0x2A` → +42.* — Tempting: reading the number as plain unsigned. Wrong: in a 6-bit field the top bit (bit 5) is the sign (§3.6).
- *(a) `0x2A` → −10.* — Tempting: sign-magnitude (`−` and magnitude `0x0A`). Wrong: that is not two's complement.
- *(c) `0x800` → +2048.* — Tempting: unsigned. Wrong: it is the most negative 12-bit value.

---

## Q10 — `x & -x` and `x & (x-1)` (Predict)

- `x = 0x68 = 0110 1000`: `−x = 1001 1000` → `x & −x = 0000 1000` (**0x08**); `x − 1 = 0110 0111` → `x & (x−1) = 0110 0000` (**0x60**).
- `x = 0`: `−0 = 0`, `0 & 0 = 0`; `0 − 1 = 0xFFFFFFFF`, `0 & 0xFFFFFFFF = 0`. Both **0**, no problem (no lowest set bit exists).
- `x = 0x80000000`: `−x = 0x80000000` (wraps to itself) → `x & −x = 0x80000000`; `x − 1 = 0x7FFFFFFF` → `x & (x−1) = 0`.

**Common wrong answers.**
- *`x & (x−1) = 0x08` (confusing the two tricks).* — Tempting: both involve `x` and a neighbour. Wrong: `x−1` *clears* the lowest 1 and sets the zeros below it, so the AND removes the bit (§3.7 box 1).
- *"`x - 1` for `x = 0` is UB."* — Tempting from signed-overflow lore. Wrong for unsigned: wraps to all-ones.

---

## Q11 — Power-of-two test (Explain)

A power of two is `0…010…0`: exactly one 1. `x & (x−1)` removes the lowest set bit (§3.7). If there was only one set bit, removing it leaves 0. If there were two or more, at least one remains → non-zero. For **x = 0** there is no set bit; `0 & 0xFFFFFFFF = 0`, so the bare test claims 0 is a power of two → add `x != 0`.

Precedence: `==` binds tighter than `&`, so `x & (x-1) == 0` parses as `x & ((x-1) == 0)`. `(x−1) == 0` is true only for x = 1, so the expression is `x & 1` when x = 1 (true) and `x & 0 = 0` otherwise: it reports **only 1** as a power of two. Correct: `x != 0 && (x & (x - 1)) == 0`.

**Common wrong answers.**
- *"The test works for all x including 0."* — Tempting: 2⁰… "zero powers"? Wrong: 0 is not 2ⁿ for any n; buffer-size code that accepts 0 divides by zero later.
- *"The precedence version is the same, just unparenthesised."* — Tempting from algebra habits. Wrong: C++ precedence puts `==` above `&` (§2).

---

## Q12 — Kernighan traces (Trace)

- `0xB4 = 1011 0100`: `x&(x−1)`: `1011 0100 & 1011 0011 = 1011 0000` (n=1) → `1011 0000 & 1010 1111 = 1010 0000` (n=2) → `1010 0000 & 1001 1111 = 1000 0000` (n=3) → `1000 0000 & 0111 1111 = 0` (n=4). **4 iterations**, result 4.
- `0x00010001`: `& 0x00010000` → `0x00010000` (n=1); `& 0x0000FFFF = 0` (n=2). **2 iterations**, result 2.
- Shift-and-test (`while(x){n+=x&1;x>>=1;}`): 8 iterations for `0xB4` (highest set bit is bit 7), 17 for `0x00010001`; a fixed `for(i<32)` loop is always 32.

**Common wrong answers.**
- *"Kernighan loops 32 times — it's a bit-by-bit algorithm."* — Tempting: all bit counters "look at each bit". Wrong: it *deletes* ones (§3.8 box 2).
- *"`0xB4` takes 8 iterations."* — Confusing it with shift-and-test (8 positions until highest bit).

---

## Q13 — Implement the stubs (Implement)

Reference: `code/solutions/bitops.h`. Check yourself: (1) all `student.*` green; (2) no ASan/UBSan output (a `len==32` shift would trigger UBSan); (3) you can explain each function's *why* from §3 without looking.

**Common wrong answers (what the tests are designed to catch).**
- `low_mask` as `(1u << len) - 1` → fails `LowMask.Len32DoesNotInvokeUB` (UBSan) — §3.3.6.
- `insert_field` missing the clip → fails `OversizedValueDoesNotLeak`.
- `sign_extend` without masking `v` → fails `IgnoresGarbageAboveWidth`.
- `is_power_of_two` without `x != 0` → fails `PowerOfTwo.Examples`.
- `popcount` via `while (x) { n += x & 1; x >>= 1; }` passes (it is correct for unsigned) but misses the point of the question; interviewers ask for complexity.

---

## Q14 — Connections (Connect)

**(a)** A reader holds a bit position `p`. The current byte is `data[p >> 3]` and, MSB-first, the bit within it is `7 − (p & 7)`. Reading *n* bits that straddle bytes is "shift, then mask" repeated per chunk — conceptually `extract_field` with `pos` measured from the right edge of each byte (Pack 03 §3.2). The writer is `insert_field` in the opposite direction.

**(b)** Even parity = XOR of a chosen subset of bits = `popcount(x & subset) & 1`. Hamming(7,4) parity bits (Pack 05 §3.2) and the P25/DMR parity checks are exactly this. Hamming *distance* between words is `popcount(a ^ b)`.

**(c)** The borrow ripple (`1 0ᵏ − 1 = 0 1ᵏ`) explains `low_mask` (§3.3.4), `x & (x−1)` and the carry in `~x + 1` (§3.7). One fact, three uses. Also CRC (Pack 04) uses shift-and-conditional-XOR on the top bit — the same gates.

**Common wrong answers.**
- *"No connection, they are unrelated utilities."* — Tempting because each is a one-liner. Wrong: Packs 03–06 are built almost entirely from these primitives.

---

## Q15 — Interview: count set bits (Interview)

Strong answer outline:
1. **Shift-and-test:** loop over bit positions, `n += (x >> i) & 1`; O(W) with W = 32 (or up to highest set bit). Simple, always safe on unsigned.
2. **Kernighan:** `while (x) { x &= x-1; ++n; }`; O(popcount). Best for sparse words; worst case 32 iterations.
3. **Lookup table:** 256-entry byte table, 4 lookups: O(W/8), cache-friendly; good when popcount is dense.
4. **SWAR / parallel reduction** (sum pairs, nibbles, bytes): O(log W) fixed ops, branch-free.
5. **Hardware:** `POPCNT` / ARM `CNT`; C++20 `std::popcount(x)` from `<bit>`; GCC/Clang `__builtin_popcount`. In production use these; in the interview show Kernighan and *explain why* (§3.8).

Must-mention pitfalls: signed shifts (infinite loop on negative), `__builtin_popcount` takes `unsigned int` (use `popcountll` for 64-bit).

**Common wrong answers.**
- *"Kernighan is O(1)."* — Tempting: loop counts are small. Wrong: O(popcount), worst-case 32.
- *"`std::popcount` is available in C++17."* — It is C++20 (`<bit>`). Under C++17 use the builtin.

---

## Q16 — Interview: read-modify-write on a shared register (Interview)

`STATUS |= FLAG_X` compiles to **read → OR → write**. If the ISR fires between the read and the write and does `STATUS &= ~FLAG_Y`, the main loop then writes back its *stale* copy, **undoing the ISR's change** (lost update). It may work for months, then fail once per million events.

Fixes:
1. **Critical section:** disable interrupts around the RMW (short, bounded).
2. **Hardware set/clear registers** (write-1-to-set / write-1-to-clear aliases; Cortex-M bit-banding where present): the write itself is the operation, no read needed. *(VERIFY the reference manual for your MCU.)*
3. **Single owner:** only the ISR writes `STATUS`; main loop posts requests via a separate variable (`std::atomic<uint32_t>` in RAM with `fetch_or`/`fetch_and`).
Also: `volatile` stops the compiler caching the register but does **not** make RMW atomic.

**Common wrong answers.**
- *"Mark it `volatile`."* — Tempting: `volatile` is what the textbook says for registers. Wrong: it prevents optimisation, not interleaving.
- *"`|=` is a single instruction so it's atomic."* — Tempting on x86 (`or [mem], imm` exists). Wrong on many MCUs (load-store architectures need 3 instructions), and even on x86 it is not atomic across cores without `lock`.

---

## §6 Self-assessment key

Rapid fire: (1) `0xFF & ~0x10 = 0xEF`. (2) `(1u<<8) − 1 = 255 = 0xFF`. (3) `110000` = 48 unsigned → 48 − 64 = **−16**. (4) `0x50 = 0101 0000` → `x & −x = 0x10`. (5) `0xF0F0` has 4 + 4 = **8** set bits.

If you missed ≥ 2, re-read §3.2/§3.7 and redo Q2/Q10 before moving on.

**VERIFY (this pack):** P25 NID bit layout and DUID values (TIA-102.BAAA-A); `std::popcount` availability on your toolchain; MCU-specific set/clear registers.
