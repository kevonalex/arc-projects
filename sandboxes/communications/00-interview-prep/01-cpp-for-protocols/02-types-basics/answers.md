# Pack 02 - Answers

Format: **Reasoning -> Answer -> Common wrong answers** (why tempting / why wrong). Reference code is in `code/solutions/es_types/`.

---

## Prerequisites

**P1.** `0xAB = 1010 1011`; `1010 0110 = 0xA6`. (Group in nibbles: A=1010, B=1011.)
**P2.** `1111 1111` (invert `0000 0001` -> `1111 1110`, add 1). *Wrong:* `1000 0001` ("sign-magnitude") - tempting because it looks like "minus one"; wrong because that is a different (obsolete) representation and addition would not work with the same circuit.
**P3.** `EXPECT_EQ` records a failure and continues; `ASSERT_EQ` records and returns from the function.
**P4.** UBSan inserts checks for undefined behaviour; no-recover makes the first UB abort the process so the test fails instead of just printing.
**P5.** A pointer is a variable holding the address of an object; `nullptr` is the "points to nothing" value.

---

## 5.1 Explain why

**Q1.** *Reasoning:* CPUs add whole registers; the cheapest model is "load small value into a word-sized register, do the arithmetic there". C codified this as integral promotion: types narrower than `int` convert to `int` (if all values fit) before any arithmetic. So `uint8_t{200} + uint8_t{100}` is `int` 300; only when you *store* into a `uint8_t` is it reduced modulo 256 (=44). To get wrap-around: `static_cast<uint8_t>(a + b)` (or store into a `uint8_t`); to detect overflow: compare the `int` sum with 255 *before* narrowing.
*Wrong answers:*
- "It is `uint8_t` because both operands are" - tempting by symmetry; wrong because the promotion happens first.
- "It is `unsigned` because both are unsigned" - tempting; wrong, promotion picks *signed* `int` when it can represent all values, which has consequences (`uint16_t*uint16_t` overflow is signed UB).
- "Add `unsigned` before it" - doesn't guarantee 8-bit wrap, only changes the intermediate type.

**Q2.** *Reasoning:* `enum class` gives scoped names (no `DMR` clash with another enum), no implicit int conversion (cannot pass the wrong id or do arithmetic by accident) and a fixed underlying type (`: uint8_t` -> 1 byte on the wire/in a struct). But `static_cast<Protocol>(x)` accepts any `x`: the wire is untrusted, so check `x <= TETRA` first, otherwise later code (`switch` with no matching case, array indexing by the enum) runs on garbage. *Answer:* type safety + scoping + size control; validation is a separate step the type system cannot do.
*Wrong:* "enum class values are checked at cast time" - tempting because it feels like a safer type; wrong, `static_cast` never checks. "Use `default:` to handle unknown" - handles the symptom at one switch and disables missing-case warnings.

**Q3.** *Reasoning:* `const uint8_t*` documents and enforces "this function only reads". It lets callers pass const buffers (e.g. a `static const` test vector in flash) without casts and stops the author modifying by accident. Output parameter: reference if the argument is mandatory (`void parse(..., Header& out)`); pointer if optional (`nullptr` allowed) or if you iterate. Many teams prefer *returning* a struct or `std::optional` (pack 04) instead of out-params.
*Wrong:* "const is just a hint" - tempting because casting it away compiles; wrong, it is part of the type; writing through it is a compile error, and modifying a truly const object by cast is UB. "Reference is faster than pointer" - same machine code.

---

## 5.2 Calculate

**Q4.** (a) `int`, 260. (b) `uint8_t`, 4 (260 mod 256). (c) `~b`: `b` (10 = `0000 1010`) promotes to `int` 10; `~` flips all 32 bits: `0xFFFFFFF5` = -11, type `int`. (d) `static_cast<uint8_t>(0xFFFFFFF5)` = `0xF5` = 245. (e) Both operands promote to `int`; true product 65535^2 = 4,294,836,225 > 2,147,483,647, so it is **signed overflow = UB** (UBSan: `signed integer overflow`); fix with `uint32_t{a} * b` (or `static_cast<std::uint32_t>(a) * b`).
*Wrong:* (c) = `0xF5` as `uint8_t` - tempting since "it's a byte op"; wrong, it is an `int` and prints as -11 or compares unequal to 245. (e) = 1 ("wraps mod 2^16") - tempting because of "unsigned wraps"; wrong, signed `int` intermediate.

**Q5.** (a) 37 = `0010 0101`; invert `1101 1010`; +1 -> `1101 1011` = `0xDB` (check: 256 - 37 = 219 = 0xDB ✓). (b) `0xE5 = 1110 0101`: invert `0001 1010` = 26, +1 = 27, so **-27** (check: 229 - 256 = -27 ✓). (c) `static_cast<int8_t>(0x80)` = -128 (modular conversion; implementation-defined before C++20); `int8_t` has 127 positive values (1..127), 128 negative (-128..-1), plus zero.
*Wrong:* (a) `1010 0101` (just sets the sign bit - sign-magnitude); (b) -101 (reading bits 6..0 as magnitude); both tempting because they feel "obvious"; wrong because two's complement is *defined* by `x + (-x) = 0 mod 2^n`.

**Q6.** `0x7FF`: bit 11 clear -> 2047. `0x800`: bit 11 set -> 2048 - 4096 = -2048. `0xA5C = 10*256 + 5*16 + 12 = 2652`; bit 11 set (0xA = 1010, top bit 1) -> 2652 - 4096 = **-1444**.
*Wrong:* `-(0xA5C)` = -2652 - tempting ("it's negative so negate it"); wrong, two's complement is "subtract 2^12", not negate the value.

---

## 5.3 Predict

**Q7.** x86 GCC (`char` signed): `-1 neg`. ARM GCC default (`char` unsigned): `255 pos`. The conversion `static_cast<char>(0xFF)` is implementation-defined for signed char (modular in C++20), and the promotion to `int` sign-extends only if `char` is signed. Fix: use `std::uint8_t`/`std::byte` for raw bytes. *Wrong:* "-1 everywhere" - tempting because it is the PC behaviour you saw; wrong, ABI-dependent.

**Q8.** `A` does not print; `B` prints. `~x`: `x = 0xF0` promoted to `int` `0x000000F0`; `~` -> `0xFFFFFF0F` (= -241) which `!=` `0x0F`. After `static_cast<uint8_t>` -> `0x0F`. *Wrong:* "A prints" - tempting because `~0xF0 == 0x0F` in 8-bit thinking; the CPU thinks in `int`.

**Q9.** `len + delta`: `delta` (int) converted to `unsigned` = 4294967292; `3 + 4294967292` = 4294967295 (mod 2^32) -> prints `4294967295`; `< 0` is *always false* for unsigned -> prints `ok`. Compilers warn: `-Wextra`'s "comparison of unsigned expression < 0 is always false". The intent ("negative -> short") is lost. Fix: compare before subtracting (`if (len < static_cast<unsigned>(-delta))`) or use a signed type deliberately and validate ranges. *Wrong:* prints `-1`/`short` - tempting from human arithmetic; wrong because `%u` and unsigned conversion apply.

---

## 5.4 Trace by hand

**Q10.** byte0 = `0x40 = 0100 0000`: emergency = bit 7 = **0**; protocol = `(0x40 >> 5) & 3` = `2 & 3` = 2 -> **TETRA**. TG = `0x01<<16 | 0x00<<8 | 0x00 = 0x010000` = **65536**. tail = `(0x80 << 8) | 0x1A = 0x801A`; offset raw = `0x801A >> 4 = 0x801` = 2049; bit 11 set -> 2049 - 4096 = **-2047**; seq = `0x801A & 0xF = 0xA` = **10**.
*Wrong:* offset = 2049 (forgot sign-extension); TG = 1 (shifted by 8 instead of 16).

**Q11.** `p[0]` is `uint8_t` -> `int` 255. `255 << 24` = `0xFF000000`: does not fit in `int` (max 0x7FFFFFFF). Before C++14 UB; C++14/17: defined when the value is representable in the *unsigned* counterpart (it is), result converted to `int` = -16777216 (impl-defined conversion); C++20: modular. The ORs: `-16777216 | 0 | 0 | 1` = `0xFF000001` as `int` = -16777215. Converting that `int` to `uint64_t` **sign-extends**: `0xFFFFFFFFFF000001`. Expected `0x00000000FF000001`. Fix: `std::uint64_t v = (std::uint64_t{p[0]} << 24) | (std::uint64_t{p[1]} << 16) | (std::uint64_t{p[2]} << 8) | p[3];` *Wrong:* "result is `0xFF000001`" - tempting; wrong, the widening is where the damage occurs. Also tests with small IDs pass - which is why it "works in tests".

---

## 5.5 Find the bug

**Q12.** `i` is `uint8_t`; `i < 256` is always true (max 255) and `++i` wraps 255 -> 0: infinite loop, and `table[i]` fine but the loop never ends. GCC: `-Wtype-limits` ("comparison is always true due to limited range of data type"; enabled by `-Wextra`). Fix: `for (unsigned i = 0; i < 256; ++i)`. *Wrong:* "off by one - use `<= 255`" - same infinite loop (255 + 1 wraps).

**Q13.** `total - hdr` with `total=5, hdr=8` is `size_t` 5 - 8 = 18,446,744,073,709,551,613; `> 0` true -> `copy_payload(buf, 18446744073709551613)` -> buffer overrun. Fix: `if (total > hdr) copy_payload(buf, total - hdr);`. *Wrong:* "make `hdr` an `int`" - converts to `size_t` again in the subtraction; same bug.

**Q14.** Dangling reference (lifetime bug): returning a reference to local `zero`. UBSan doesn't cover it; ASan with `detect_stack_use_after_return=1` (default off on older GCC/Clang runtime; on by default only in newer Clang) can report `stack-use-after-return`; the compiler warns `-Wreturn-local-addr`/`reference to local variable returned`. Fixes: return by value (`std::uint8_t`), or return a pointer (`nullptr` for "none"), or `std::optional` (pack 04), or a `static const std::uint8_t kZero = 0;` (safe but awkward). *Wrong:* "make `zero` `static`" - technically avoids the lifetime bug but mutable global state is a smell; "make the reference non-const" - no effect.

---

## 5.6 Implement

**Q15.** `static_cast<uint8_t>(a + b)`; overflow: `(a + b) > 0xFF`. Common wrong: `a + b < a` (the unsigned-wrap test) - false for `uint8_t` because the sum is a non-wrapped `int`, so it never detects overflow. Also: `uint8_t sum = a + b; return sum < a;` works but needs the narrowing - fine but harder to reason about.

**Q16.** `read_be16`: `(p[0] << 8) | p[1]`. `read_be32`: cast each byte to `uint32_t` before shifting (shift by 24 on `int` risks the sign-bit issue). `read_be48`: loop `v = (v << 8) | p[i]` with `uint64_t v`. Wrong: `p[0] << 40` on an `int` (shift >= width = UB; UBSan: `shift exponent 40 is too large for 32-bit type`); `static_cast<uint64_t>(p[0] << 24 | ...)` (cast too late; sign-extends).

**Q17.** `sign_extend`: mask to `bits`, test the sign bit, subtract `2^bits`; use 64-bit math so `bits == 32` has no `1u << 32`. Alternative idiom: `((v & mask) ^ sign) - sign` using 64-bit signed. `less_mixed`: `if (a < 0) return true; return unsigned(a) < b;`. `narrow_u8`: range check *before* assigning (leave `out` untouched on failure). Wrong: `(int)b > a` in `less_mixed` - breaks for `b > INT_MAX`; `static_cast<uint8_t>(v)` without a check in `narrow_u8` (silent truncation of 256 -> 0).

**Q18.** `high_nibble`: `b >> 4` (byte shift operator). `nibble_swap`: `(b << 4) | (b >> 4)` (`std::byte`'s `<<` truncates to 8 bits so no mask needed; with `uint8_t` you would need a cast). `to_hex`: table `"0123456789ABCDEF"` indexed by `v >> 4` and `v & 0xF`; wrong: `std::to_string(int)` (decimal, no leading zero), `std::hex` without `setw(2)`/`setfill('0')` (drops leading zeros, e.g. `0F` -> `F`).

**Q19.** `name`: `switch` with no `default`, `return "UNKNOWN"` after it. `protocol_from_u8`: `v <= 2`, then `static_cast`; leave `out` untouched on failure. `Flags`: operators via `to_underlying` and `static_cast<uint8_t>` (the `|` yields `int`); `has(f, bits)`: `(f & bits) == bits`. Wrong: `has` as `(f & bits) != Flags::None` - returns true if *any* bit matches (test `has(f, Emergency|Data)` catches it).

**Q20.** `swap_u8`: temp variable. `find_byte`: loop `it != end`, return `end` if not found. `count_nonzero`: loop. `clamp_inplace`: `if/else if`. `fill_pattern`: `static_cast<uint8_t>(start + i)` (`i` is `size_t`, narrowing mod 256 is well-defined for unsigned). Wrong: returning `nullptr` for not found (breaks the `[begin,end)` convention and callers comparing with `end`); `for (it <= end)` dereferencing `end`.

**Q21.** `mask`: `width >= 32 ? 0xFFFFFFFFu : (1u << width) - 1u`. `bytes_for_bits`: `(bits + 7) / 8` (adding `divisor - 1` before dividing implements ceiling for non-negative integers: `ceil(n/d) = (n + d - 1) / d`; bits near `SIZE_MAX` would overflow - note as a limitation). `reverse_bits8`: loop shifting out the low bit of the input into the low bit of the output, shifting the output up each time. Table: loop 0..255 in a `constexpr` function (C++17 allows loops and non-const `std::array::operator[]` in constexpr). Wrong: `(1 << width) - 1` (signed shift: bit 31 problem and UB at 32); `bits / 8 + 1` (gives 2 for 8 bits, wrong).

---

## 5.7 Connect

**Q22.** UBSan (`-fsanitize=undefined`: `signed integer overflow`) catches the multiplication at run time. `-Wconversion` (not in `-Wall -Wextra`) flags `uint8_t d = a + b;` narrowing at compile time; `-Wextra` includes `-Wtype-limits` (Q12) and `-Wsign-compare` (some mixed compares). Both: compile-time warnings find *suspicious code* even if never executed; runtime sanitizers find *actual executed UB*, including data-dependent cases no warning can see. *Wrong:* "ASan reports it" - ASan is memory errors only.

**Q23.** `std::span<const uint8_t>` bundles pointer+length, so a call can't pass a mismatched length (a `(p, n)` pair swapped or wrong `n`), and loops become `for (auto b : s)`. It removes "length bookkeeping errors"; it does *not* prevent dangling (span of a destroyed vector), nor out-of-bounds `s[i]` unless you check (`at` doesn't exist on span in C++20; hardened libs may check). Pack 05 §3.2 explains why `1u << 32` is UB, what the hardware does, and why `mask()` needs a branch. *Wrong:* "span is bounds-checked" - not by default.

---

## 5.8 Interview-style

**Q24.** `int`. Reason: integral promotion - narrower than `int` operands convert to `int` since `int` can represent all `uint8_t` values; machine arithmetic is word-sized. `uint32_t + int`: no promotion needed (both rank >= int); usual arithmetic conversions: `int` becomes `uint32_t` (same rank, unsigned wins), so `uint32_t{1} + -2` = 4294967295. Interviewers want: promotion, then conversions, plus a concrete trap (`~`, `-1 < 1u`). *Wrong:* "unsigned wins because it is bigger" - it's about rank and signedness, not "bigger".

**Q25.** `const`: immutable through this name, may be initialised at run time. `constexpr`: usable in constant expressions; implies const for variables; functions may run at compile or run time. `consteval` (C++20): *must* run at compile time. `const T* p`: pointee const; `T* const p`: pointer const. A constexpr function is evaluated at run time when called with non-constant args or in a non-constant-evaluated context (e.g. `int w = read(); mask(w);`). *Wrong:* "constexpr always means compile time" (see §3.7).

**Q26.** `std::byte` is a distinct type: no accidental arithmetic or printing as a character, clear intent ("raw storage"), bitwise ops allowed, explicit conversion `std::to_integer`. `char` has implementation-defined signedness; `unsigned char` works but is also used for small numbers and strings, blurring intent. All three may alias other objects. *Wrong:* "std::byte is faster" - no.

**Q27.** Solution: `v & mask`, test the sign bit (`1 << (bits-1)`), subtract `1 << bits` if set. Edge cases: `bits == 1` (values -1/0), `bits == 32` (avoid `1u << 32`), bits above `bits` set in input (mask them), `bits == 0` (undefined/precondition: `1 << -1`), result type range (`INT32_MIN`). Mention testing with 0x7FF/0x800/0xFFF for 12 bits. *Wrong:* `(int)(v << (32 - bits)) >> (32 - bits)` - the common shift trick; it relies on arithmetic right shift of negative values (implementation-defined before C++20) and on left-shifting into the sign bit (UB-ish before C++14), and fails at `bits == 0`; acceptable only with the caveats stated.
