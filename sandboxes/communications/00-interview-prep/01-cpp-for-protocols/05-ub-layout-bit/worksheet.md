# Pack 05 - Undefined behaviour, memory layout, bit manipulation

**Time:** 2-4 hours. **Code:** `code/` (stubs in `include/es_ubbit/`, reference in `solutions/`, UB demos in `labs/`).
**C++17 note:** `<bit>` (`std::popcount`, `std::bit_cast`, `std::endian`) is C++20; the tests cross-check against it only when available, and all your implementations use C++17-compatible code (`memcpy`).
Build and run labs: `cmake -S . -B build && cmake --build build`, then run `build/lab_ub_check_O0` and `build/lab_ub_check_O2` etc. (labs are built **without** sanitizers on purpose).

---

## 1. Why this matters

Radio protocols pack information tightly: 4-bit opcodes, 12-bit signed fields, 48-bit counters, big-endian words. The C++ you write to pick those apart sits at the boundary where the *language's rules* and the *hardware's behaviour* disagree, and where a compiler optimising at `-O2` may behave differently from your debug build.

Real consequences:

- A bounds check written as `if (a + b < a)` works in the debug build and is *deleted* by the optimiser in the release build shipped to the radio (§3.1).
- A mask helper `(1u << width) - 1` returns 0 for `width == 32` on one CPU and garbage on another (§3.2).
- A header "parsed" by casting `rx_buf + 3` to `Header*` is misaligned, aliasing-violating, padded and endian-wrong at once - and works on the developer's laptop (§3.3-3.4).
- A struct sent raw leaks uninitialised padding bytes on the wire (an information leak) and has different size on the two ends.

This pack explains each rule from the hardware up, so you can predict what the compiler is allowed to do.

> **Safety:** all byte values are synthetic; never connect these exercises to live public-safety traffic.

---

## 2. Prerequisites check

- **P1.** What type is `uint8_t{0x80} << 24`, and what value does it have? *(Pack 02 `../02-types-basics/` §3.2.)*
- **P2.** Why does `-1 < 1u` evaluate to `false`? *(Pack 02 §3.3.)*
- **P3.** What does UBSan with `-fno-sanitize-recover=undefined` do when it sees UB, and why do our tests use it? *(Pack 01 `../01-toolchain/` §3.4.)*
- **P4.** What is a dangling pointer? Why does a use-after-free often "work" in tests? *(Pack 03 `../03-lifetime-move/` Q24.)*
- **P5.** What does `std::memcpy(&dst, src, n)` do, and why is `memcpy(nullptr, p, 0)` still UB? *(Basics: it copies `n` bytes; pointers must be valid even for n = 0.)*

---

## 3. Concept build-up

### 3.1 Undefined behaviour and signed overflow

**Concrete first.** `labs/lab_ub_check.cpp`:

```cpp
static bool overflows(int x) { return x + 1 < x; }     // "obviously" detects wrap-around
int main(int argc, char**) { int x = INT_MAX - 1 + argc; printf("%d\n", overflows(x)); }
```

Run `lab_ub_check_O0` and `lab_ub_check_O2` (no arguments, so `x == INT_MAX`). Typical result: `-O0` prints `1`, `-O2` prints `0`. Same source, opposite answers.

**Derive why the standard says "undefined".** C++ describes behaviour on an *abstract machine*, and for each operation says one of: **defined**; **implementation-defined** (compiler must document: `sizeof(int)`, signedness of `char`); **unspecified** (one of several allowed, e.g. argument evaluation order); **undefined** (no requirements at all).

Signed overflow is undefined because hardware disagrees: two's-complement CPUs wrap (`0x7FFFFFFF + 1 = 0x80000000 = -2147483648`), some DSPs *saturate* (stay at max), some trap. Rather than force a slow emulation on some targets, the standard promises nothing.

**Why compilers *exploit* it.** The optimiser reasons "the programmer never triggers UB", which gives it powerful facts: for signed `x`, `x + 1 > x` is always true, so `x + 1 < x` is always false - the whole function becomes `return false`. Likewise `for (int i = 0; i <= n; ++i)` can be assumed to terminate (no wrap), enabling vectorisation. UB therefore means more than "a wrong number at this line": the compiler may delete checks, reorder code before the UB, or make impossible branches "reachable". *Nothing says the behaviour matches the hardware.*

**What you do instead (patterns):**
- Test **before** the operation, in a form that cannot itself overflow: `a + b > MAX` becomes `a > MAX - b` (for `b > 0`) - see `checked_add`.
- Use **unsigned** types when you *want* wrap-around (defined modulo 2^N).
- `abs(INT_MIN)` is UB too (+2,147,483,648 does not fit); `checked_abs`.
- Flags: `-fwrapv` (GCC/Clang) *defines* signed wrap - a stop-gap, not portable C++; `-fsanitize=undefined` finds it in tests.

**Other UB you meet in protocol code:** shift by >= width (§3.2), strict-aliasing violation (§3.3), misaligned access (§3.4), out-of-bounds/null/dangling access (packs 01/03/04), data race (pack 06), uninitialised reads, modifying a `const` object, reading an inactive union member.

> **Common misconceptions**
> - *"UB means it crashes."* Usually it silently works until the optimiser or hardware changes.
> - *"Signed overflow wraps on my machine, so it's fine."* The compiler, not the machine, is bound by the standard.
> - *"I can check for overflow after the fact."* The overflowing operation *is* the UB.
> - *"UB only affects the faulty line."* It can affect code before and after it.

### 3.2 Shifts: what the hardware does and why the standard stays silent

**Concrete first.** `labs/lab_shift.cpp` computes `1u << n` with `n == 32`. On x86-64 and AArch64 it prints `1` (VERIFY on your CPU); on 32-bit ARM register shifts it prints `0` (VERIFY). UBSan: `shift exponent 32 is too large for 32-bit type 'unsigned int'`.

**The rule.** For `E1 << E2` and `E1 >> E2`: the operand is first promoted (pack 02); if `E2` is negative or `>=` the bit-width of the *promoted* `E1`, the behaviour is undefined. (`uint8_t << 8` is fine: promoted to 32-bit `int`; but `uint8_t{0x80} << 24` produces a value that does not fit signed `int` - pack 02; in C++20 left shift of signed values is defined modulo 2^N.)

**Derive what the hardware does (and why it differs).** A *barrel shifter* shifts by any amount in one step using log2(32) = **5** stages that shift by 1, 2, 4, 8, 16 positions; each stage is switched on by one bit of the count. The cheapest design wires exactly the **low 5 bits** of the count register to those stages and ignores the rest: shifting by 32 = `0b100000` uses low bits `00000` -> shifts by **0**. That is what x86 does (`SHL r32, CL` masks CL to 5 bits). Another design adds comparator logic so counts >= 32 produce 0 (AArch32 register-shifts, which look at the low byte). Both are sensible hardware choices.

| CPU family (VERIFY) | `1u << 32` | `1u << 33` |
|---|---|---|
| x86-64, AArch64 (count masked to 5 bits) | 1 | 2 |
| AArch32 register shift (count taken from low 8 bits, >= 32 -> 0) | 0 | 0 |

If the standard picked "result is 0", x86 compilers would have to insert a compare-and-select for *every* variable shift. If it picked "masked", ARM compilers would pay. So it declares the case undefined and lets each platform be fast; *you* must avoid it.

**Failure if ignored.** `mask = (1u << width) - 1` for `width == 32`: on x86 -> `(1) - 1 = 0` (a field extractor silently returns 0). Fix by special-casing (`width >= 32 ? 0xFFFFFFFFu : ...`) or `~0u >> (32 - width)` for `width >= 1`.

**Rotate.** The textbook `(v << n) | (v >> (32 - n))` shifts by 32 when `n == 0`: UB. Derivation of a safe form: we want the right shift count to be `(32 - n) mod 32`, and `-n & 31` equals that for every `n` (since `-n mod 32 = 32 - n` for `n` in 1..31, and 0 for `n = 0`): `(v << (n & 31)) | (v >> (-n & 31))`. At `n = 0` this is `v | v = v`. Compilers recognise the pattern and emit a single `rol`. C++20 has `std::rotl`.

**Right shift of a negative signed value** is implementation-defined before C++20 (arithmetic on every mainstream compiler), defined as arithmetic from C++20.

> **Common misconceptions**
> - *"Shifting by the width gives 0 (all bits shifted out)."* Not guaranteed; on x86 it can give the original value.
> - *"Hardware behaviour defines C++ behaviour."* The compiler may constant-fold with yet another answer.
> - *"`x << n` for signed `x` is the same as for unsigned."* Pre-C++20, overflow into the sign bit was UB.

### 3.3 Strict aliasing and type punning

**Concrete first.** `labs/lab_alias.cpp`:

```cpp
int set_and_read(int* i, float* f) { *i = 1; *f = 2.0f; return *i; }
int x = 0; set_and_read(&x, reinterpret_cast<float*>(&x));   // -O0: 1073741824   -O2: 1
```

`2.0f` has bits `0x40000000` = 1,073,741,824 as an `int`. At `-O0` the write really lands in `x`. At `-O2` the compiler assumes `i` and `f` cannot point to the same object, so `*f = 2.0f` cannot change `*i` and it returns the cached `1` without re-reading memory.

**Derive.** If *any* pointer could alias *any* other, after every store through a pointer the compiler would have to forget everything it knows and reload from memory - killing register allocation and vectorisation. The **strict aliasing rule** gives it a deal: an object may be accessed only through an lvalue of (a cv/signedness variant of) its own type, a base class, or a *character type* (`char`, `unsigned char`, `std::byte`). Accessing through any other type is UB, so the compiler may assume it never happens.

**Legal ways to reinterpret bytes:**
1. `std::memcpy(&dst, &src, sizeof dst)` - copies bytes between objects; compilers turn it into a register move (no real call).
2. `std::bit_cast<To>(from)` (C++20): same-size, trivially copyable types, constexpr-capable.
3. Inspect an object's bytes through `unsigned char*`/`std::byte*`.
*Not* legal in C++: reading an inactive union member (legal in C), `*reinterpret_cast<T*>(p)` for an unrelated `T`.

**Alignment is a separate rule.** Even where aliasing is OK, `reinterpret_cast<uint32_t*>(buf + 1)` creates a pointer that violates the alignment requirement of `uint32_t` (§3.4). `memcpy` has no alignment requirement.

> **Common misconceptions**
> - *"reinterpret_cast is the way to parse bytes."* It is a way to get UB; `memcpy`/shifts are the way.
> - *"It works on x86 so it's fine."* The *compiler* optimises on the assumption, independent of the CPU.
> - *"`-fno-strict-aliasing` fixes it."* It hides this class (Linux kernel does it) but not misalignment or endianness.
> - *"memcpy is slow."* A fixed-size memcpy compiles to one load.

### 3.4 Alignment, padding and layout, byte by byte

**Concrete first.**

```cpp
struct A { std::uint8_t a; std::uint32_t b; std::uint8_t c; };   // sizeof(A) == 12 on x86-64/ARM
```

| offset | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| content | `a` | pad | pad | pad | `b0` | `b1` | `b2` | `b3` | `c` | pad | pad | pad |

Only 6 of 12 bytes carry data. Reordered `struct B { uint32_t b; uint8_t a; uint8_t c; }` is `b0..b3, a, c, pad, pad` = **8** bytes.

**Derive the rules.**
1. **Why alignment exists.** Memory is fetched in aligned words. A 4-byte load at an address that is not a multiple of 4 straddles two words: x86 does two fetches (slower; atomicity lost), many ARM cores fault (`SIGBUS`/hard fault), and SIMD instructions may *require* alignment. So each type has `alignof(T)` (for scalars normally = `sizeof(T)`), and objects of that type must live at multiples of it.
2. **Field placement.** The compiler places each field at the next offset that is a multiple of its alignment: `offset_k = align_up(end_of_previous, alignof(field_k))`. For `A`: `a@0`, end 1; `b`: `align_up(1,4) = 4`; end 8; `c@8`, end 9.
3. **Tail padding.** `sizeof(S) = align_up(end_of_last, max alignment of fields)`. Why: arrays. Element `i` sits at `i * sizeof(S)`. If `A` were 9 bytes, element 1's `b` would be at 9 + 4 = 13: misaligned. Rounding the size up to the largest alignment keeps *every* field of *every* element aligned.
4. **`align_up` derivation.** For `a = 2^k`, the binary of `a - 1` is `k` ones (`8 - 1 = 0b0111`). `x + (a-1)` pushes any non-multiple of `a` across the next boundary but leaves multiples in place... then clearing the low `k` bits (`& ~(a-1)`) snaps down to the boundary. Examples: `align_up(9, 8)`: `9 + 7 = 16 = 0b10000 & ~0b111 = 16` ✓; `align_up(8, 8)`: `15 = 0b1111 & ~7 = 8` ✓. It needs `a` to be a power of two, because only then `~(a-1)` is a clean mask; `is_pow2(x) = x != 0 && (x & (x-1)) == 0` (`x & (x-1)` clears the lowest set bit - §3.5 - so a power of two becomes 0).
5. **Rule of thumb:** order fields largest-to-smallest to minimise padding.

**Wire format vs memory format.** Never send or parse a struct by `memcpy` of the whole object or by casting a buffer pointer: (a) padding bytes are indeterminate - on the wire an *information leak*; (b) sizes/offsets differ per ABI; (c) host endianness; (d) the cast pointer may be misaligned/aliasing-illegal. `__attribute__((packed))`/`#pragma pack` removes padding, but then members are misaligned and taking a reference/pointer to one is UB-prone; it still has endianness issues. **Serialise field by field** (`write_header`/`read_header`).

**Bit-fields** (`unsigned op : 4;`): allocation order, straddling and padding are implementation-defined; do not use them for wire formats.

> **Common misconceptions**
> - *"sizeof(struct) is the sum of its members."* Padding.
> - *"Padding is zeroed."* It is indeterminate unless you zero the whole object.
> - *"Packed structs are the portable way to map a wire format."* They fix padding, nothing else.
> - *"Unaligned access always crashes."* On x86 it often works - until a vectorised loop uses an aligned instruction.

### 3.5 Bit tricks, endianness, `bit_cast`

**Kernighan popcount.** Why `v & (v - 1)` clears the lowest set bit: subtracting 1 turns the trailing zeros into ones and the lowest `1` into `0`, so the two numbers differ exactly in the lowest set bit and below. Example: `0b101000 - 1 = 0b100111`; `0b101000 & 0b100111 = 0b100000`. A loop doing this until 0 runs once per set bit. Hardware has a `popcnt` instruction; C++20 `std::popcount` uses it when available. **Uses:** Hamming distance `popcount(a ^ b)` (bit errors between two received words), parity (even/odd number of ones; `parity` can fold with xor: `v ^= v >> 16; v ^= v >> 8; ...`), counting active slots in a bitmask.

**Endianness.** The value `0x11223344` is four bytes; which comes first in memory?

| | address +0 | +1 | +2 | +3 |
|---|---|---|---|---|
| big-endian | `11` | `22` | `33` | `44` |
| little-endian | `44` | `33` | `22` | `11` |

x86 and most ARM configurations are little-endian; network/telecom specs are overwhelmingly **big-endian** ("network byte order"; most-significant byte first matches how humans read a hex dump and how a serial stream delivers the high-order bits first - the historical rationale; VERIFY for any specific standard). So bytes `C5 00 12 34` mean `0xC5001234` on the wire, but a naive load on a little-endian host yields `0x341200C5`.

**Portable ways:** (1) build the value by shifts from bytes - correct on any host: `(b0 << 24) | (b1 << 16) | (b2 << 8) | b3` with each byte first widened to `uint32_t` (pack 02); compilers emit a load + `bswap`; (2) `memcpy` into a `uint32_t` then `byteswap` if the host is little-endian (`std::endian::native`, C++20; `std::byteswap` C++23). Detect endianness by looking at the object's first byte (`memcpy` to an `unsigned char`), not with pointer casts.

**`bit_cast` for floats.** IEEE-754 binary32: 1 sign bit, 8 exponent bits (bias 127), 23 fraction bits. `1.0f` = sign 0, exponent 127 = `0111 1111`, fraction 0 -> bits `0 01111111 000...0` = `0x3F800000` ✓. `-2.0f` = sign 1, exponent 128 = `1000 0000` -> `1 10000000 000...0` = `0xC0000000` ✓. `float_bits` is `memcpy`/`bit_cast`; the cast form `*(uint32_t*)&f` is the §3.3 violation.

**Extract/insert fields.** `mask(w) = (1 << w) - 1` (`w` ones); `extract(v, lo, w) = (v >> lo) & mask(w)`; `insert(v, lo, w, f) = (v & ~(mask(w) << lo)) | ((f & mask(w)) << lo)`. Special-case `w == 32` and `w == 0` (§3.2).

> **Common misconceptions**
> - *"Network order = whatever the CPU does."* It is fixed (big-endian); hosts convert.
> - *"`htonl`-style swaps are always needed."* Only when host != network order - portable shift code avoids the question.
> - *"popcount needs a lookup table."* Loops, tables and the instruction all exist; pick for readability, measure if hot.
> - *"Bit-fields are the clean way to describe headers."* Layout is implementation-defined.

---

## 4. Fully worked example: decode a control word from a misaligned buffer

A received buffer (synthetic): `unsigned char buf[8] = {0xAA, 0xAA, 0xAA, 0xC5, 0x00, 0x12, 0x34, 0xBB};` The 4-byte control word starts at offset 3. Toy layout (invented): bits 31..28 opcode, 27..24 flags, 23..16 reserved, 15..0 id.

**Wrong way (see Q15):** `uint32_t w = *reinterpret_cast<const uint32_t*>(buf + 3);` Three violations: `buf + 3` is not 4-aligned (UBSan: `load of misaligned address ... which requires 4 byte alignment`), `unsigned char` data read as `uint32_t` (aliasing), and on a little-endian host the bytes `C5 00 12 34` give `0x341200C5`, not `0xC5001234`.

**Right way, step by step.**
1. `load_be32(buf + 3)`: memcpy 4 bytes -> `0x341200C5` (little-endian host); `byteswap32` -> `0xC5001234`. (Or shifts: `0xC5<<24 | 0x00<<16 | 0x12<<8 | 0x34` = `0xC5001234`.)
2. opcode = `extract_bits(w, 28, 4)`: `0xC5001234 >> 28 = 0xC`; `& 0xF` = `0xC` = 12.
3. flags = `extract_bits(w, 24, 4)`: `w >> 24 = 0xC5`; `& 0xF` = `0x5`.
4. id = `extract_bits(w, 0, 16)` = `0x1234` = 4660.
5. Set bit 0 of flags: `insert_bits(w, 24, 4, 0x5 | 0x1)` -> flags still `0x5` (bit 0 already set) -> unchanged. Setting bit 1: `0x5 | 0x2 = 0x7` -> `0xC7001234`.
6. Parity of the word: popcount: `C`=1100 (2), `5`=0101 (2), `0`,`0` = 0, `1` (1), `2` (1), `3` (2), `4` (1) -> 2+2+0+0+1+1+2+1 = **9** -> odd -> `parity32 = 1`.

Notice what each step avoided: no cast of a byte buffer to a wider type; no shift by >= 32 (`extract_bits(w, 0, 32)` is special-cased); no signed arithmetic.

---

## 5. Questions

### 5.1 Explain why

**Q1.** *Context: a reviewer asks why the code uses `uint32_t` for a rolling counter but signed `int` for a sample delta.* Explain why unsigned overflow is defined but signed overflow is undefined, using what hardware does and what the optimiser gains.

**Q2.** *Context: `mask(width)` returns 0 for `width == 32` on the gateway and 0xFFFFFFFF in the unit tests on another machine.* Explain (using the barrel-shifter derivation) why shifting by the type's width is undefined rather than "0".

**Q3.** *Context: a DSP routine reads samples from a byte buffer by casting to `int16_t*`; it is correct at `-O0` and wrong at `-O2`.* Explain the strict aliasing rule and the optimisation it enables.

**Q4.** *Context: a protocol struct is 8 bytes on the radio and 12 bytes on the gateway.* Explain why padding and alignment exist, why `sizeof` rounds up to the largest member alignment, and why this makes structs unsuitable as wire formats.

### 5.2 Calculate

**Q5.** *Context: sizing a call-state record.* On x86-64: `struct S { uint16_t a; uint8_t b; uint32_t c; uint8_t d; uint64_t e; };` Give the offset of each member, `sizeof(S)`, and `sizeof` after reordering to `e, c, a, b, d`. How many bytes are padding in each?

**Q6.** *Context: carving a DMA buffer pool.* Compute with the mask formula, showing binary: `align_up(37, 16)`, `align_up(0x1235, 0x100)`, `align_up(64, 64)`. Which of `{24, 32, 48, 64}` are valid alignments for the formula and why?

**Q7.** *Context: bit error counting and float telemetry.* Compute: (a) `popcount(0xF0F0F0F0 ^ 0xFF00FF00)`; (b) the bit pattern of `0.5f` (show sign/exponent/fraction); (c) `rotl32(0x80000001, 4)`; (d) `byteswap32(0x0A0B0C0D)`.

### 5.3 Predict

**Q8.** *Context: a "wrap detector" in a timestamp routine.* Predict the output of `lab_ub_check_O0` and `_O2`, say what UBSan would print, and say what the standard guarantees about either output.

**Q9.** *Context: a bitmask helper.* Predict `lab_shift` on x86-64 and on a 32-bit ARM, and what UBSan prints. Which result is "correct"?

**Q10.** *Context: a sensor driver that "just reinterprets" a word.* Predict `lab_alias_O0` vs `_O2` and justify with the bit pattern of `2.0f`. Then say which of these change the answer: (a) replacing the cast with `memcpy`, (b) `-fno-strict-aliasing`, (c) making `set_and_read` take both pointers as `int*`.

### 5.4 Trace by hand

**Q11.** *Context: a rotate helper that "works in tests".* Trace the textbook `(v << n) | (v >> (32 - n))` and the fixed `(v << (n & 31)) | (v >> (-n & 31))` for `v = 0x80000001` and `n = 0`, `n = 1`, `n = 33`. Give each intermediate value.

**Q12.** *Context: debugging a header parse on a little-endian gateway.* Bytes at `p`: `0A 0B 0C 0D`. (a) What does `memcpy` into `uint32_t` give? (b) What does the shift-based big-endian read give? (c) What does `byteswap32` of (a) give? (d) Draw the byte map (offset, field, padding) of `struct Header { uint8_t type; uint16_t length; uint32_t id; }` on x86-64 and give its `sizeof`.

### 5.5 Find the bug

**Q13.** *Context: bounds check before a copy.* Find the bug(s) and give a correct version:
```cpp
bool fits(int used, int add, int cap) { return used + add >= used && used + add <= cap; }
```

**Q14.** *Context: field extraction helper.* Find the bug and its fix:
```cpp
std::uint32_t get_field(std::uint32_t v, unsigned lo, unsigned width) { return (v >> lo) & ((1u << width) - 1); }
```
For which inputs is it wrong, and what does UBSan print?

**Q15.** *Context: a quick-and-dirty receiver.*
```cpp
struct Header { std::uint8_t type; std::uint16_t length; std::uint32_t id; };
void on_rx(const std::uint8_t* rx) {
    const Header* h = reinterpret_cast<const Header*>(rx + 3);
    if (h->length > 100) return;
    handle(h->id);
}
```
List every distinct way this is wrong.

### 5.6 Implement

(`code/include/es_ubbit/`; UBSan runs inside the tests.)

**Q16.** *Context: tick/sample arithmetic that must never be UB.* Implement `checked_add`, `sat_add`, `wrapping_add`, `checked_abs` in `ub_safe.hpp`.

**Q17.** *Context: bit helpers for protocol fields.* Implement `shl`, `shr` (defined for counts >= 32) and `rotl32_manual` in `ub_safe.hpp`.

**Q18.** *Context: planning struct layouts for a shared-memory ring.* Implement `is_pow2`, `align_up`, `is_aligned`, `field_offset`, `struct_size` in `layout.hpp`; verify against the compiler's `offsetof`/`sizeof`.

**Q19.** *Context: wire encoding of a header.* Implement `write_header`/`read_header` (big-endian, 7 bytes) in `layout.hpp`.

**Q20.** *Context: control-word decoding.* Implement `popcount32`, `parity32`, `clz32`, `byteswap16/32`, `is_little_endian`, `extract_bits`, `insert_bits`, `float_bits`, `bits_float`, `load_u32_ne`, `load_be32` in `bits.hpp`.

### 5.7 Connect

**Q21.** *Context: pack 02 (`../02-types-basics/`).* Explain how integer promotion makes `p[0] << 24` an `int` shift and why `std::uint32_t{p[0]} << 24` is the fix; link that to §3.2's rule "shift count < width of the *promoted* type".

**Q22.** *Context: pack 06 (`../06-concurrency-rt/`).* A counter incremented by one thread and read by another with plain `uint32_t` is a data race. Why is that UB (not just "stale values"), and what is the minimal fix?

**Q23.** *Context: pack 04 (`../04-containers-templates/`) and pack 07 (`../07-sm-builds/`).* `RingBuffer<T,N>` uses `index & (N-1)`. Which §3.4/§3.5 facts make that valid, what must `N` satisfy, and how do you enforce it at compile time?

### 5.8 Interview-style

**Q24.** "What is undefined behaviour? Give three examples and explain why a compiler can make a program with UB do *anything*."

**Q25.** "How do you read a big-endian `uint32_t` from a byte buffer portably and without UB? What goes wrong with a cast?"

**Q26.** "Why does `sizeof` of a struct often exceed the sum of its members, and how would you design a wire-format struct?"

**Q27.** "What is strict aliasing? How do you legally reinterpret the bits of a float?"

---

## 6. Self-assessment

- [ ] I can explain why signed overflow is UB and show `x + 1 < x` being optimised away.
- [ ] I can derive the shift-by-width behaviour from a barrel shifter and name the safe rotate form.
- [ ] I can state the strict-aliasing rule and use `memcpy`/`bit_cast` instead of casts.
- [ ] I can lay out a struct byte by byte and compute `sizeof`/offsets by hand.
- [ ] I can derive `align_up` and explain why it needs a power of two.
- [ ] I can explain endianness with a concrete 4-byte example and read BE data portably.
- [ ] I can implement popcount (Kernighan), extract/insert with width 0/32 edges, `float_bits`.
- [ ] All tests pass with UBSan on; I ran the labs at `-O0` and `-O2` and explained the difference.
- [ ] I can answer Q24-Q27 aloud.
