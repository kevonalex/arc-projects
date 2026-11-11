# Pack 05 - Answers

Format: **Reasoning -> Answer -> Common wrong answers**. Reference code: `code/solutions/es_ubbit/`.

---

## Prerequisites

**P1.** `uint8_t{0x80}` promotes to `int`; `<< 24` gives `0x80000000`, which does not fit in a signed 32-bit `int`: before C++14 UB, C++14/17 defined via the unsigned representation then implementation-defined conversion (`-2147483648`), C++20 modular. Either way a *negative* `int`, which sign-extends if later widened (pack 02 Q11).
**P2.** The usual arithmetic conversions turn `-1` into `4294967295u`, so the test is `4294967295 < 1`.
**P3.** It prints a diagnostic and aborts at the first UB so the test fails instead of continuing with a silently-wrong value.
**P4.** A pointer to an object whose lifetime ended; freed memory usually still holds old bytes until reused, so tests pass and the field fails.
**P5.** Copies `n` bytes. `memcpy`'s pointer arguments must be valid even when `n == 0` (a null pointer is UB), so guard empty cases.

---

## 5.1 Explain why

**Q1.** *Reasoning:* hardware: two's-complement adders wrap, but other representations (sign-magnitude, saturating DSPs, trapping ISAs) exist/existed. The standard picked: **unsigned** arithmetic is defined as modulo 2^N (useful for counters, hashes, sequence numbers); **signed** overflow undefined, so the optimiser may assume it never happens: `x + 1 > x` is always true; loops with `int` counters can be assumed to terminate and be vectorised; pointer arithmetic can be strength-reduced. *Answer:* defined-wrap for things you *want* to wrap, UB for things that are bugs - and the UB buys optimisation. *Wrong:* "signed overflow is UB because CPUs trap" - tempting, but most CPUs wrap; the point is portability + optimisation. "Unsigned is always safer" - unsigned wrap is silent and causes underflow bugs (pack 02).

**Q2.** *Reasoning:* a 32-bit barrel shifter has 5 stages (shift by 1,2,4,8,16); the cheapest wiring uses the **low 5 bits** of the count. `32 = 0b100000` -> low 5 bits `00000` -> shift 0. Other CPUs add logic so counts >= 32 give 0. For `x << n` with `n` in a register the compiler normally emits a single shift instruction; if the standard defined "result is 0", x86 would need an extra compare+select on *every* variable shift; if it defined "masked", ARM32 pays. Undefined = each platform stays fast and you avoid the case. So `mask(32)` on x86 computes `(1u << 32) - 1 = 1 - 1 = 0`, while a different compiler/optimisation level may produce something else. *Wrong:* "all bits shifted out so the result is 0" - that is ARM32's hardware, not the language; "x86 gives garbage" - it gives a well-defined hardware answer (the unshifted value), but C++ doesn't promise it.

**Q3.** *Reasoning:* see §3.3: if `int16_t*` and `uint8_t`-array storage could alias freely, every store would force the optimiser to reload other values. The rule lets it assume stores through one type don't affect objects of unrelated types, so it keeps values in registers, reorders loads/stores, vectorises. `-O0` does none of that, so the bug is invisible. Casting a byte buffer to `int16_t*` violates the rule (and may misalign). *Fix:* `memcpy` each sample (or read bytes and assemble). *Wrong:* "the optimiser is buggy" - tempting after a `-O2`-only failure; it is following the rules.

**Q4.** *Reasoning:* alignment exists because memory is fetched in aligned words: a misaligned access spans two fetches (slow, non-atomic) or faults on some ARM cores. Hence each field is placed at a multiple of its alignment (padding fills the gaps), and `sizeof` rounds up to the largest member alignment so that in an *array* element `i` at `i*sizeof` keeps all its members aligned. Radio 8 bytes vs gateway 12: different ABIs/compilers choose different alignment (and `long`, pointers, etc. differ), different member order/padding; so layout is not a contract. *Answer:* serialise field by field with explicit sizes and endianness. *Wrong:* "padding is wasted space the compiler adds randomly" - it is deterministic from alignment rules; "`#pragma pack` solves it" - it fixes size, not endianness/misalignment hazards.

---

## 5.2 Calculate

**Q5.** Offsets: `a@0` (2 bytes, end 2); `b@2` (end 3); `c`: `align_up(3,4) = 4` (pad 1 byte at offset 3), end 8; `d@8`, end 9; `e`: `align_up(9,8) = 16` (pad 7 bytes: 9..15), end 24; `sizeof = align_up(24, 8) = 24`. Padding = 1 + 7 = **8 bytes**; data = 16. Reordered `e,c,a,b,d`: `e@0..7`, `c@8..11`, `a@12..13`, `b@14`, `d@15`, end 16, `sizeof = 16`, **0 padding**. *Wrong:* 16 for the original (summing members); forgetting tail padding.

**Q6.** `align_up(37,16)`: `37 + 15 = 52 = 0b110100`; `~15` clears low 4 bits: `0b110000 = 48`. `align_up(0x1235, 0x100)`: `0x1235 + 0xFF = 0x1334`; `& ~0xFF` = `0x1300`. `align_up(64,64)`: `64 + 63 = 127 = 0b1111111`; `& ~63 = 64` (already aligned stays). Valid alignments for the mask formula: **32 and 64** (powers of two). `24` and `48`: `a - 1` is `0b10111`/`0b101111`, not a contiguous low-bit mask, so `& ~(a-1)` clears the wrong bits (e.g. `align_up(25, 24)`: `25 + 23 = 48`, `& ~23` = `0b110000 & 0b...101000` = 32, not 48). *Wrong:* "it works for any a" - only powers of two.

**Q7.** (a) `0xF0F0F0F0 ^ 0xFF00FF00 = 0x0FF00FF0` -> four `F` nibbles -> **16**. (b) `0.5f = 2^-1`: sign 0; exponent = -1 + 127 = 126 = `0111 1110`; fraction 0: `0 01111110 000...` = `0011 1111 0000 ...` = **`0x3F000000`**. (c) `0x80000001 << 4 = 0x00000010` (upper bits shifted out), `>> 28 = 0x8` -> `0x10 | 0x8 = ` **`0x18`**. (d) `byteswap32(0x0A0B0C0D)` = **`0x0D0C0B0A`**. *Wrong:* (c) `0x80000018` - tempting if you forget that rotation moves the top bit around to the bottom *and* removes it from the top (the top nibble `8` becomes the bottom nibble: `0x80000001` rotated by 4 = `0x00000018`).

---

## 5.3 Predict

**Q8.** `-O0`: prints `1` (the add executes and wraps to INT_MIN, `INT_MIN < INT_MAX`). `-O2`: prints `0` (folded: `x + 1 < x` is "always false" for signed). UBSan build: `runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'` and abort. The standard guarantees **nothing** for either output. *Wrong:* "-O2 is the buggy one" - tempting; both are legal.

**Q9.** x86-64 (and AArch64): `1u << 32` -> count masked to 5 bits -> `1 << 0` = **1** (VERIFY on your CPU). 32-bit ARM register shift: **0**. UBSan: `shift exponent 32 is too large for 32-bit type 'unsigned int'`. Neither is "correct": the program has UB; a compiler may also fold it to anything. *Wrong:* "0 is correct since all bits shifted out".

**Q10.** `-O0`: **1073741824** (`0x40000000`, bits of `2.0f`: sign 0, exponent 128 = `1000 0000`, fraction 0 -> `0 10000000 000... = 0100 0000 0000 ... = 0x40000000`). `-O2`: **1**. Which changes help: (a) replacing the cast by `float tmp = 2.0f; std::memcpy(&x, &tmp, sizeof x);` makes the program *well-defined*: `x` becomes 1073741824 at every optimisation level (there is no longer a pointer pair the compiler may assume disjoint). (b) `-fno-strict-aliasing`: the compiler no longer assumes unrelated types don't overlap, so `-O2` also prints 1073741824 (the program is still non-conforming C++, just compiled leniently). (c) both parameters `int*`: the compiler must assume they may alias (same type), so it re-reads `*i` after `*f = ...`; the function then returns the last value written through the second pointer (the bits of `2` if you write `*f = 2`). Concept: the printed answer depends on what the compiler is *allowed to assume* about the two pointers.

---

## 5.4 Trace by hand

**Q11.** `v = 0x80000001`.

| n | textbook `(v<<n)\|(v>>(32-n))` | fixed `(v<<(n&31))\|(v>>(-n&31))` |
|---|---|---|
| 0 | `v<<0 = 0x80000001`; `v>>32` **UB** (x86: `v>>0`; ARM32: 0) -> "works" on both by luck, compiler free to do anything (UBSan aborts) | `n&31 = 0`, `-0&31 = 0`: `v \| v = 0x80000001` |
| 1 | `v<<1 = 0x00000002`; `v>>31 = 1` -> `0x00000003` | `n&31 = 1`, `-1&31 = 31`: same -> `0x3` |
| 33 | `v<<33` UB; `v>>(-1)` UB (negative count in `unsigned` wraps to a huge shift) | `n&31 = 1`; `-33 mod 32`: `-33 = -64 + 31` -> `31`; same as n=1 -> `0x3` |

*Why `-n & 31 = (32-n) mod 32`:* `-n` is `2^32 - n` for unsigned; `2^32` is a multiple of 32, so `& 31` yields `(-n) mod 32 = (32 - n) mod 32`. *Wrong:* "n=0 is harmless because the result is right anyway" - right on this CPU, UB regardless.

**Q12.** (a) LE host: `memcpy` -> `0x0D0C0B0A`. (b) shifts: `0x0A<<24 | 0x0B<<16 | 0x0C<<8 | 0x0D = 0x0A0B0C0D` (any host). (c) `byteswap32(0x0D0C0B0A) = 0x0A0B0C0D`. (d) x86-64 `Header`: `type@0`, pad@1, `length@2-3`, `id@4-7`, `sizeof = 8` (align 4); wire format is 7 bytes - they cannot be `memcpy`ed into each other.

---

## 5.5 Find the bug

**Q13.** `used + add >= used` is an after-the-fact overflow test built on signed wrap (UB). The optimiser may simplify it to `add >= 0`, which doesn't detect overflow (e.g. `used = INT_MAX, add = 1`), and the following `used + add <= cap` then overflows for real. Also negative inputs aren't rejected. Correct: `return used >= 0 && add >= 0 && used <= cap && add <= cap - used;` (every subexpression is overflow-free because `cap - used >= 0`). *Wrong:* "use `long`" - moves the problem; "cast to unsigned" - turns negative `add` into huge, accepted or rejected for the wrong reasons unless you also validate sign.

**Q14.** `(1u << width) - 1` is UB for `width >= 32`; for `width == 32` x86 yields `(1 << 0) - 1 = 0` (mask 0, extracts nothing); also `v >> lo` is UB for `lo >= 32`. UBSan: `shift exponent 32 is too large for 32-bit type 'unsigned int'`. Fix: `if (width == 0) return 0; const uint32_t m = width >= 32 ? 0xFFFFFFFFu : ((1u << width) - 1u);` and precondition `lo < 32`. *Wrong:* "width 32 is unusual so ignore" - field extractors are routinely called with the full word.

**Q15.** (1) `rx + 3` is not aligned for `uint16_t`/`uint32_t` fields: UB (UBSan `member access within misaligned address`), hard fault on some ARM. (2) Strict aliasing: the bytes were `uint8_t` objects; reading them through `Header` is UB. (3) Layout: the struct has padding and `sizeof` 8 on x86-64; the wire has 7 bytes with different offsets, so `id` is read from the wrong bytes. (4) Endianness: multi-byte fields read in host order (little-endian host vs big-endian wire). (5) No length check: reading 7-8 bytes from `rx + 3` without knowing the buffer holds them (buffer over-read). Fix: `read_header(rx + 3, remaining, h)` (explicit field-by-field, bounds-checked).

---

## 5.6 Implement

**Q16.** `checked_add`: `if (b > 0 && a > MAX - b) fail; if (b < 0 && a < MIN - b) fail; return a + b;` - the subtractions are overflow-free because of the sign guards. `sat_add`: on failure return `b > 0 ? MAX : MIN`. `wrapping_add`: add in `uint32_t`, cast back. `checked_abs`: reject `INT32_MIN`. Wrong: `if (a + b < a)` (the UB form); `abs()` from `<cstdlib>` (UB at INT_MIN); `return a + b` after `long long` widening is fine for 32-bit inputs *if* you then range-check the 64-bit result - a valid alternative.

**Q17.** `shl`: `n >= 32 ? 0 : v << n`; same for `shr`. `rotl32_manual`: `n &= 31; if (n == 0) return v; return (v << n) | (v >> (32 - n));` (or the `-n & 31` form). Wrong: `(v << n) | (v >> (32 - n))` unguarded; `n % 32` fine, `n / 32` mistaken.

**Q18.** `is_pow2`: `x != 0 && (x & (x-1)) == 0`. `align_up`: `(x + a - 1) & ~(a - 1)`. `is_aligned`: `uintptr_t % a == 0`. `field_offset`: accumulate `align_up(off, f[i].align) + f[i].size` for fields before the index, then `align_up(off, f[index].align)`. `struct_size`: same loop, then `align_up(off, max_align)`. Wrong: `x / a * a` (rounds *down*); `(x + a) & ~(a-1)` (adds a full block to already-aligned x); forgetting tail padding.

**Q19.** `write_header`: `if (cap < 7) return 0;` then bytes `type`, `length>>8`, `length`, `id>>24`, `id>>16`, `id>>8`, `id` - widening to `uint32_t` before shifting is not needed here because `h.id` is already `uint32_t`; for `length` (`uint16_t` -> promoted `int`) `>> 8` is fine. `read_header`: check `n >= 7`; build `id` with `uint32_t` casts per byte. Wrong: `memcpy(out, &h, sizeof h)` (8 bytes with padding, host endianness).

**Q20.** See `bits.hpp` solution. Key edge cases: `extract_bits` with `width == 0` and `width == 32`; `insert_bits` masks `field` and `lo+width <= 32`; `popcount32` via `v &= v - 1`; `parity32` by xor-folding 16/8/4/2/1; `clz32(0) == 32`; `is_little_endian` via `memcpy` to `unsigned char`; `float_bits` via `memcpy`; `load_be32` = `load_u32_ne` + `byteswap32` iff little-endian. Wrong: `*(uint32_t*)p`; `(1u << width) - 1` unguarded; `union { float f; uint32_t u; }` punning (UB in C++, defined in C).

---

## 5.7 Connect

**Q21.** `p[0]` is `uint8_t` -> promoted to `int` (32-bit); `p[0] << 24` is an `int` shift: legal count (< 32) but the result may not fit (`0xFF << 24`), giving a negative `int` which sign-extends if widened to 64 bits (pack 02 Q11), and in pre-C++20 the signed-overflow rules apply. `std::uint32_t{p[0]} << 24` makes the operand unsigned *before* shifting: defined modulo 2^32. For a 40-bit shift on `int`: count 40 >= 32 = UB. The rule: shift count must be < the width of the *promoted* left operand, so widen first.

**Q22.** Two threads accessing the same non-atomic object, at least one writing, with no happens-before relation = data race = UB for the whole program. Practical effects: the compiler may keep the variable in a register (the reader spins forever), tear a 32-bit store on an 8/16-bit CPU, or reorder. Minimal fix: `std::atomic<std::uint32_t>` with `fetch_add(1, std::memory_order_relaxed)` / `load(relaxed)` for a pure counter (pack 06 §3.3), or a mutex. *Wrong:* "`volatile` fixes it" - `volatile` is not atomic and gives no ordering between threads.

**Q23.** If `N` is a power of two, `N - 1` is a contiguous low-bit mask (`8-1 = 0b0111`), so `i & (N-1) == i % N` (§3.4 item 4: the same mask idea as `align_up`) - a single AND instead of a division. Enforce: `static_assert(N != 0 && (N & (N - 1)) == 0, "N must be a power of two")` (`is_pow2`). With free-running unsigned indices that wrap at 2^32 (or 2^64), the wrap point is itself a multiple of N, so `index & (N-1)` stays consistent across overflow (derived in pack 07).

---

## 5.8 Interview-style

**Q24.** Model answer: UB = the standard imposes no requirements on the program's behaviour for the *whole execution* once an operation is UB. Examples: signed overflow, shift >= width, out-of-bounds/null/dangling dereference, strict-aliasing violation, data race, reading uninitialised memory (indeterminate), modifying a const object. Compilers can do anything because their optimisations are *proofs under the assumption that UB never occurs*: a branch that would need UB to be taken is "unreachable" and removed; a check that is only false under UB is "always true"; code before the UB may be reordered with it. Mention tools: UBSan, ASan, `-fwrapv`, compile warnings. *Weak:* "it crashes".

**Q25.** Shift-based assembly with each byte widened first: `(uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | p[3]` - correct on any endianness/alignment; compilers emit load+bswap. Or `memcpy` + conditional byteswap (`std::endian`). A cast `*(uint32_t*)p` is wrong: misaligned, strict-aliasing violation, host endianness, and unchecked length.

**Q26.** Padding for alignment (hardware fetches, array element alignment) -> `sizeof` > sum. Wire-format design: don't use struct memory layout; define the wire format as a table of byte offsets/widths/endianness in the spec; write `serialize`/`parse` functions field by field (with bounds checks); order in-memory fields largest-first if you care about size; `static_assert(sizeof(...))` only for *internal* structs where layout matters (shared memory between processes built by the same compiler) - and still prefer explicit offsets.

**Q27.** Strict aliasing: an object may be accessed only through its own type (plus cv/sign variants, base types, and `char`/`unsigned char`/`std::byte`), so the optimiser can assume different-typed pointers don't overlap. Legal bit reinterpretation: `std::bit_cast<std::uint32_t>(f)` (C++20), or `std::memcpy(&u, &f, sizeof u)` (pre-C++20; optimised to a register move). Not: `*reinterpret_cast<uint32_t*>(&f)`, nor union punning in C++.
