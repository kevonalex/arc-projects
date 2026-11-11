# Pack 02 - Types and basics: integers, promotion, enum class, references, pointers, const, constexpr

**Time:** 2-4 hours. **Code:** `code/` (stubs in `include/es_types/`, reference in `solutions/`).
**Build:** see `../README.md` (`cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`).
Answers: `answers.md`. Recall: `cheatsheet.md`.

---

## 1. Why this matters

A protocol stack is a machine for moving *bit fields* in and out of bytes. A DMR header, a P25 link-control word or a TETRA PDU is specified as "3 bits here, 24-bit ID there, 12-bit signed value over there". C++ does not have a "24-bit unsigned" type; it has `int` (whose size depends on the CPU), implicit conversions that silently widen and narrow, and rules about what `a + b` even *means* for small types.

Typical field failures that trace back to this pack:

- A talkgroup ID with the top bit set is read as a negative number and the call is routed to the wrong group.
- `~mask` on a `uint8_t` is compared to a byte constant and the comparison is *never* true.
- A `char` that is signed on the x86 gateway and unsigned on the ARM radio makes the same frame parse differently on each.
- A "length minus header" computed in `size_t` underflows to 18 quintillion and a copy runs off the end of the buffer.

If you can say *why* the compiler did what it did, you stop guessing. That is what interviewers probe.

> **Safety:** all byte values here are synthetic. Never connect these exercises to live public-safety channels.

---

## 2. Prerequisites check

- **P1.** Convert `0xAB` to binary, and `1010 0110` to hex. *(Basics; if shaky, do binary/hex drills first.)*
- **P2.** What is `-1` as an 8-bit two's-complement pattern? *(Pack 02 §3.3 teaches this from scratch; if you have never seen it, read §3.3 now.)*
- **P3.** Build and run pack 01's tests; what is the difference between `EXPECT_EQ` and `ASSERT_EQ`? *(Pack 01 `../01-toolchain/` §3.3.)*
- **P4.** What does `-fsanitize=undefined` do and why do our tests use `-fno-sanitize-recover=undefined`? *(Pack 01 §3.4.)*
- **P5.** What is a pointer, in one sentence? What is `nullptr`? *(Basics; §3.6 builds the precise model.)*

---

## 3. Concept build-up

### 3.1 Fixed-width integers

**Concrete first.** On the radio's 16-bit-`int` microcontroller `sizeof(int) == 2`; on the Linux gateway `sizeof(int) == 4`. The standard guarantees only `sizeof(char) == 1`, `short >= 16 bits`, `int >= 16 bits`, `long >= 32 bits`, `long long >= 64 bits` (VERIFY the exact minimums in the standard if you quote them).

**Derive why `<cstdint>` exists.** A protocol standard says "16-bit field". If you store it in `int`, then on one platform you have 16 bits and on another 32; shifts, wrap-around and overflow all change meaning. The standard lets you say exactly what you mean: `std::uint8_t`, `std::uint16_t`, `std::uint32_t`, `std::uint64_t` (and signed `int8_t`... `int64_t`): *exactly* N bits, no padding, two's complement for the signed ones. They are optional (a machine with no 8-bit type will not provide `uint8_t`), but every target we care about has them. Use `std::size_t` for sizes and indices (it is the type of `sizeof`), `std::ptrdiff_t` for pointer differences.

**Failure if ignored.** `unsigned long` is 32 bits on 32-bit Linux/ARM and Windows, 64 bits on 64-bit Linux. A struct written to disk or a socket with `unsigned long` fields has a different layout on each (pack 05).

> **Common misconceptions**
> - *"`int` is 32 bits."* Only on some platforms.
> - *"`uint8_t` is a special byte type."* It is almost always a `typedef` for `unsigned char` - which is why it prints as a character with `cout` and why it behaves like a character type (§3.4).
> - *"Use the smallest type to save memory."* Small types are fine in arrays/structs; for *arithmetic* locals, small types buy you nothing and invite the promotion traps of §3.2.

### 3.2 Integer promotion and the usual arithmetic conversions

**Concrete first.**

```cpp
std::uint8_t a = 200, b = 100;
auto c = a + b;          // c == 300, and decltype(c) is int
std::uint8_t d = a + b;  // d == 44   (300 mod 256) - narrowing happens on assignment
```

`a + b` did **not** wrap at 256. It produced 300 in a wider type.

**Derive the rule.** C (and so C++) was designed on machines whose arithmetic unit and registers worked on a "natural word" - `int`. Loading an 8-bit value into a register and adding it as a full word is the cheapest operation; special 8-bit arithmetic was a luxury. So the language says:

1. **Integral promotion.** Any operand of a type narrower than `int` (`bool`, `char`, `signed/unsigned char`, `short`, `unsigned short`, and enums of those sizes) is first converted to `int` if `int` can represent *all* its values, else to `unsigned int`. For `uint8_t` and `uint16_t` (16-bit `int` aside) the answer is always `int`.
2. **Usual arithmetic conversions.** Then, for a binary operator, both operands are converted to a common type, roughly: if one is `long long`/`unsigned long long`, go there; else if one is `unsigned int` and the other `int`, **the signed one becomes unsigned**; else both are `int`.

Consequences, each with a number:

| Expression (all operands `uint8_t`/`uint16_t` unless noted) | Type | Value |
|---|---|---|
| `uint8_t{200} + uint8_t{100}` | `int` | 300 |
| `~uint8_t{0x0F}` | `int` | -16 = `0xFFFFFFF0` (NOT `0xF0`) |
| `uint8_t{0x80} << 1` | `int` | `0x100` (the top bit did not "fall off") |
| `uint16_t{65535} * uint16_t{65535}` | `int` | true product 4,294,836,225 > `INT_MAX` 2,147,483,647 -> **signed overflow, UB** |
| `int{-1} < unsigned{1}` | compares as `unsigned` | -1 becomes 4,294,967,295, so the result is `false` |

**Failure if ignored.**

```cpp
std::uint8_t mask = 0x0F;
if (~mask == 0xF0) { /* never runs: ~mask is 0xFFFFFFF0 */ }
```

The fix is to narrow deliberately: `static_cast<std::uint8_t>(~mask) == 0xF0`.

**Why not "fix" the language?** Compatibility with 50 years of code and efficient code generation on word-oriented CPUs. You live with it; the habit is: *when a result must be N bits, cast back to N bits explicitly, at the end.*

> **Common misconceptions**
> - *"`uint8_t + uint8_t` is `uint8_t`."* It is `int` (Q1).
> - *"Unsigned arithmetic always wraps."* Only types of `unsigned int` rank or above wrap in the arithmetic; small unsigned types are promoted to *signed* `int` first, so `uint16_t * uint16_t` can overflow *signed* int.
> - *"Casting the result fixes the problem."* `static_cast<uint16_t>(a * b)` casts *after* the signed overflow already happened; cast an *operand* (`uint32_t{a} * b`).
> - *"`a += b` on `uint8_t` avoids promotion."* It still computes in `int`, then narrows (maybe with a warning under `-Wconversion`).

### 3.3 Signed vs unsigned, two's complement

**Concrete first.** The 8-bit pattern `1111 1111` is `255` as `uint8_t` and `-1` as `int8_t`. Same bits, different interpretation.

**Derive two's complement.** We want addition hardware to be the *same circuit* for signed and unsigned. In an n-bit register arithmetic is modulo 2^n: `255 + 1 = 0` (8 bits). So define the negative of x as the value y such that `x + y = 0 (mod 2^n)`: y = 2^n - x. For n = 8, x = 37: y = 256 - 37 = 219 = `1101 1011`. Quick mnemonic that follows from the algebra: *invert all bits and add one* (since `x + ~x = 111...1 = 2^n - 1`, so `-x = ~x + 1`). Range: the top bit acts as `-2^(n-1)`; for 8 bits that is -128..127. There is exactly one more negative number than positive ones.

**Which operations are defined?**
- **Unsigned overflow wraps** (modulo 2^n) - defined, usable (hash/sequence numbers).
- **Signed overflow is undefined behaviour** (UB). Why: lets the compiler assume `x + 1 > x` for signed `x`, enabling optimisations (loop analysis, pointer arithmetic). The *hardware* wraps; the *language* does not promise it (pack 05 shows a compiler deleting a "wrap check").
- **Conversion to unsigned** is always defined: the value modulo 2^n. `static_cast<uint8_t>(-1) == 255`.
- **Conversion to signed of an out-of-range value** was implementation-defined before C++20 (every real compiler wraps) and is defined as modular wrap since C++20.

**Sign extension of a protocol field.** A 12-bit signed field holding `0xFFF` means -1, not 4095. To widen: if bit 11 is set, subtract 2^12. Check: `0xFFF`: bit 11 set -> 4095 - 4096 = -1 ✓. `0x800`: 2048 - 4096 = -2048 ✓. `0x7FF`: bit 11 clear -> 2047 ✓.

**The mixed-comparison trap**, numerically: `-1 < 1u`. Usual arithmetic conversions turn `-1` into `4294967295u`, so the test is `4294967295 < 1` = false. The same trap: `for (std::size_t i = v.size() - 1; i >= 0; --i)` never terminates because `i >= 0` is always true for unsigned; and `v.size() - 1` on an empty vector is 18,446,744,073,709,551,615.

> **Common misconceptions**
> - *"Unsigned is safer because it cannot be negative."* It cannot be negative, so `a - b` with `a < b` becomes huge, silently.
> - *"Signed overflow wraps like unsigned."* Not guaranteed (pack 05 `../05-ub-layout-bit/` §3.1 shows the consequences).
> - *"`int8_t x = 0x80;` is -128, guaranteed."* The conversion is modular in C++20 and implementation-defined earlier - write intent with `static_cast` or use the unsigned type plus `sign_extend`.

### 3.4 `char`, `unsigned char`, `std::byte`

**Concrete first.**

```cpp
char c = 0xFF;   // 0xFF does not fit if char is signed
int x = c;       // x == -1 on x86 GCC (signed char), x == 255 on ARM GCC (unsigned char)
```

**Derive.** Plain `char` has *three* distinct types sharing a representation (`char`, `signed char`, `unsigned char`), and whether plain `char` is signed is implementation-defined (historic reason: the PDP-11 sign-extended loads cheaply, ARM prefers unsigned). So *never* use plain `char` for raw bytes or arithmetic. Use `std::uint8_t` (arithmetic-friendly) or `std::byte`.

`std::byte` (C++17, `<cstddef>`) is an `enum class : unsigned char`. Why a distinct type: raw memory is *not a number*; you should not add or print it by accident. It supports only bitwise ops (`& | ^ ~ << >>`) and `std::to_integer<T>(b)` for explicit conversion; `std::byte{0xAB}` needs the braces (no implicit int conversion). Both `unsigned char` and `std::byte` are allowed to alias any object (pack 05, strict aliasing); `uint8_t` usually is `unsigned char` so it inherits that, but `std::byte` makes the *intent* "raw storage" explicit.

> **Common misconceptions**
> - *"`char` is a byte."* Same size, but signedness varies and arithmetic on it is promoted like any small type.
> - *"`std::byte` is slower or special."* Same representation; zero cost.
> - *"`uint8_t` always prints as a number."* It prints as a character with iostreams; cast to `int`/`unsigned` to print.

### 3.5 `enum class`

**Concrete first.**

```cpp
enum Color { Red, Green };         // old style
enum Mode  { Idle, Active };       // error: ... but if both define `Idle`, redefinition!
int x = Red + Green;               // compiles (!), nonsense

enum class Protocol : std::uint8_t { P25 = 0, DMR = 1, TETRA = 2 };
// int y = Protocol::DMR;                       // ERROR: no implicit conversion
// bool same = (Protocol::DMR == SomeOther::X); // ERROR: different types
```

**Derive the three benefits.** (1) **Scoped names**: `Protocol::DMR` cannot clash with `Mode::DMR`. (2) **No implicit conversion** to/from integers: a call `set_protocol(2)` does not compile, so mixing up a `Protocol` and a `Slot` is a compile error, not a field bug. (3) **Specified underlying type** (`: std::uint8_t`): `sizeof(Protocol) == 1`, so it can be a wire/struct field with a known size, and you can forward-declare it.

**Wire values are untrusted.** `static_cast<Protocol>(77)` compiles and, with a *fixed* underlying type, is well-defined (the object holds 77) - but then your `switch` has no case for it. Always validate the integer first (`protocol_from_u8`), then cast. Without a fixed underlying type, a value outside the enum's range is UB.

**Bit-flag enums.** `enum class` has no operators. Write `operator|`, `operator&` that go through `to_underlying`. (C++23 has `std::to_underlying`; we provide it.) Test for *all* bits: `(f & bits) == bits`; testing `(f & bits) != None` means "any bit".

**`switch` over an `enum class` without `default:`** lets `-Wswitch` warn when you add an enumerator and forget a case. That is a safety net you lose if you add `default:`.

> **Common misconceptions**
> - *"enum class values can't be compared/ordered."* Same-type `==`, `<` work.
> - *"Casting an int to an enum validates it."* It does not.
> - *"`default:` makes a switch safer."* It silences the compiler's missing-case warning.

### 3.6 References, pointers, const

**Concrete first.**

```cpp
void swap_u8(std::uint8_t& a, std::uint8_t& b) { auto t = a; a = b; b = t; }
std::uint8_t x = 1, y = 2;
swap_u8(x, y);                    // x == 2, y == 1: the function changed the caller's objects
```

**Mental model.** An object lives at an address. A **pointer** is a variable holding an address (may be `nullptr`, can be re-pointed, supports `p + k` which moves by `k * sizeof(*p)` bytes). A **reference** is another *name* for an existing object: it must be bound at creation, can never be null (in valid code), and cannot be re-bound. Use a reference when "must exist, don't re-seat"; a pointer when "optional or iterating".

**Pointer arithmetic and ranges.** For `const uint8_t* b` and `e = b + n`, the range is *half-open* `[b, e)`. `e` may be formed (one-past-the-end) and compared but not dereferenced. Why half-open: an empty range is `b == e` (no special value needed) and `e - b` is the length. `find_byte` therefore returns `e` for "not found", never `nullptr` (Q20).

**const reading rule: read right to left.**

| Declaration | Meaning |
|---|---|
| `const uint8_t* p` / `uint8_t const* p` | pointer to const byte: can re-point `p`, cannot write `*p` |
| `uint8_t* const p` | const pointer: cannot re-point, can write `*p` |
| `const uint8_t* const p` | neither |
| `const uint8_t& r` | reference to const: read-only alias; may bind to temporaries |

**Why const-correctness is worth the noise.** `find_byte(const uint8_t*, const uint8_t*, ...)` *promises* not to modify the capture buffer; the compiler enforces it, callers can pass const buffers, and a reviewer reads intent from the signature. Failure if ignored: a function taking `uint8_t*` cannot be called on a `const` buffer without a cast, and a cast away of const is exactly where bugs hide.

**Pass small trivially copyable things (<= 2 words) by value, bigger by `const&`.** For byte buffers pass pointer+length (or `std::span`, pack 04).

**Dangling.** A reference or pointer to a local variable outlives the variable the moment the function returns:

```cpp
const std::uint8_t& first_or_zero(const std::uint8_t* p, std::size_t n) {
    std::uint8_t zero = 0;
    if (n == 0) return zero;     // returns a reference to a dead local -> UB
    return p[0];
}
```

> **Common misconceptions**
> - *"A reference is a pointer that's automatically dereferenced."* An implementation detail; semantically it is an alias.
> - *"`const` means the memory is read-only."* It means *this name* cannot be used to modify; another name might.
> - *"A pointer one past the end is invalid."* Forming it is valid; dereferencing is not.
> - *"`const int* p` and `int* const p` are the same."* Opposite meanings.

### 3.7 `constexpr`

**Concrete first.**

```cpp
constexpr unsigned kSymbolRate = 4800;                         // DMR: symbols per second (VERIFY ETSI TS 102 361-1)
constexpr unsigned kSlotMs = 30;                               // DMR timeslot length (VERIFY)
constexpr unsigned kSlotSymbols = kSymbolRate * kSlotMs / 1000; // 144, computed by the COMPILER
static_assert(kSlotSymbols == 144, "30 ms * 4800 sym/s = 144 symbols (288 bits at 2 bits/symbol; VERIFY)");
```

**Derive.** `const` says "this object won't change after init". `constexpr` says "*can* be evaluated at compile time" - it implies const and requires a constant initialiser. A `constexpr` *function* may be used in constant expressions (array bounds, `static_assert`, template arguments) **and** may also be called at run time with run-time arguments.

**Why embedded cares.** A 256-entry lookup table built by `make_reverse_table()` at compile time lives in read-only flash; nothing runs at boot to fill RAM, no static-initialisation-order problems, and mistakes become compile errors (`static_assert`). Since C++17, `inline constexpr` variables in headers are safe to include everywhere (one entity, ODR-compliant).

**Failure if ignored.** The classic constexpr-function bug is hidden UB: `(1u << width) - 1` with `width == 32` is undefined; inside a constant expression the compiler *refuses to compile* (good), at run time it may produce garbage. Test the edge (`mask(32)`).

> **Common misconceptions**
> - *"constexpr = fast."* It moves work to compile time only if the call is in a constant-evaluated context.
> - *"constexpr functions run only at compile time."* That is `consteval` (C++20).
> - *"`const` globals are constant expressions."* Only if initialised by a constant expression of integral/enum type (and even then, prefer `constexpr`).

---

## 4. Fully worked example: parse a toy header

Toy fixed header (6 bytes; **invented for teaching, not a real standard**):

```
byte 0     : bit 7 = emergency | bits 6..5 = protocol (0=P25, 1=DMR, 2=TETRA) | bits 4..0 reserved (0)
bytes 1..3 : talkgroup, 24-bit big-endian unsigned
bytes 4..5 : upper 12 bits = signed offset | lower 4 bits = sequence number
```

Input bytes: `A0 00 12 34 FF F3`.

**Step 1 - byte 0.** `0xA0 = 1010 0000`.
- emergency = bit 7 = 1.
- protocol = (byte0 >> 5) & 0b11. `0xA0 >> 5`: types: `0xA0` is `uint8_t` -> promoted `int` 160; 160 >> 5 = 5 (`101`); `& 3` = `01` = 1 -> **DMR**. (Validate 1 <= 2 via `protocol_from_u8`.)

**Step 2 - talkgroup.** `p[1]=0x00, p[2]=0x12, p[3]=0x34`. Each `p[i]` promotes to `int`. Combine in `uint32_t`: `(uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | p[3] = 0x001234 = 4660`. (Casting *first* matters if a byte can be >= 0x80 and the shift is 24 or more - see Q11.)

**Step 3 - the 16-bit tail.** `read_be16(p+4)`: `(0xFF << 8) | 0xF3 = 0xFFF3` (as `uint16_t`, 65523).
- offset raw = `0xFFF3 >> 4`: `uint16_t` promoted to `int` 65523; `>> 4` = 4095 = `0xFFF`.
- sign-extend 12 bits: bit 11 set -> 4095 - 4096 = **-1**.
- sequence = `0xFFF3 & 0xF` = 3.

**Step 4 - result.** `{emergency=true, protocol=DMR, talkgroup=4660, offset=-1, seq=3}`.

Code:

```cpp
const std::uint8_t* p = bytes;
bool emergency = (p[0] & 0x80) != 0;
Protocol proto;
if (!protocol_from_u8(static_cast<std::uint8_t>((p[0] >> 5) & 0x3), proto)) return false;
std::uint32_t tg = (std::uint32_t{p[1]} << 16) | (std::uint32_t{p[2]} << 8) | p[3];
std::uint16_t tail = read_be16(p + 4);
std::int32_t offset = sign_extend(tail >> 4, 12);   // tail >> 4 is int; converts to uint32_t, value 0xFFF
unsigned seq = tail & 0xF;
```

Notice every narrowing is explicit, every shift is in a type wide enough, and the enum is validated before use. Now do Q10 for another header.

---

## 5. Questions

### 5.1 Explain why

**Q1.** *Context: a junior engineer asks why their 8-bit sequence counter "doesn't wrap" in `if (a + b > 255)`.* Explain why `uint8_t + uint8_t` is `int`, deriving it from how the CPU does arithmetic, and say what you must do when you want 8-bit wrap-around.

**Q2.** *Context: a gateway receives a protocol-ID byte from a radio and casts it to `enum class Protocol`.* Why is `enum class` better than `enum` for protocol identifiers, and why must the wire value still be validated before the cast?

**Q3.** *Context: reviewing an API: `parse(uint8_t* data, size_t n)` which never writes to `data`.* Why should it be `const uint8_t*`, and when would you use a reference vs a pointer parameter for a function that fills in a result?

### 5.2 Calculate

**Q4.** *Context: fixing "weird" checksum arithmetic.* For `uint8_t a = 250, b = 10;` give the type and value of (a) `a + b`, (b) `static_cast<uint8_t>(a + b)`, (c) `~b`, (d) `static_cast<uint8_t>(~b)`, (e) `uint16_t{65535} * uint16_t{65535}` (say what is wrong with it).

**Q5.** *Context: a signed RSSI-offset byte.* (a) Write -37 as an 8-bit two's-complement pattern in binary and hex. (b) What is the signed value of the byte `0xE5`? (c) What is `static_cast<int>(static_cast<int8_t>(0x80))` and how many *positive* values does `int8_t` have vs negative?

**Q6.** *Context: decoding a 12-bit signed measurement field.* Sign-extend the 12-bit values `0x7FF`, `0x800`, `0xA5C` and give the decimal results. Show the arithmetic for `0xA5C`.

### 5.3 Predict

**Q7.** *Context: porting from the x86 gateway to an ARM radio.* Predict the output on each platform:

```cpp
char c = static_cast<char>(0xFF);
int x = c;
std::printf("%d %s\n", x, (c < 0) ? "neg" : "pos");
```

**Q8.** *Context: masking a status byte.* Predict whether the branch runs, and the value of `y`:

```cpp
std::uint8_t x = 0xF0;
if (~x == 0x0F) std::puts("A");
std::uint8_t y = static_cast<std::uint8_t>(~x);
if (y == 0x0F) std::puts("B");
```

**Q9.** *Context: a length check in a frame parser.* Predict the output and explain the integer conversions:

```cpp
unsigned len = 3; int delta = -4;
std::printf("%u\n", len + delta);
if (len + delta < 0) std::puts("short"); else std::puts("ok");
```

### 5.4 Trace by hand

**Q10.** *Context: debugging with a captured toy header.* Using §4's format, decode `40 01 00 00 80 1A`: emergency, protocol, talkgroup (decimal), signed offset, sequence. Show each shift/mask result in hex.

**Q11.** *Context: an engineer wrote a 32-bit loader that "works" in tests but breaks on high IDs.* Trace this for bytes `{0xFF, 0x00, 0x00, 0x01}`:

```cpp
std::uint64_t v = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
```

State the type of each sub-expression, the `int` value after the ORs, and the final `uint64_t` value. Then give the corrected one-line form.

### 5.5 Find the bug

**Q12.** *Context: a table-init loop in firmware.* Find the bug(s) (compiler warns; what does it say?):

```cpp
for (std::uint8_t i = 0; i < 256; ++i) table[i] = compute(i);
```

**Q13.** *Context: payload-length computation.* Find the bug and say what happens when `total == 5`, `hdr == 8`:

```cpp
std::size_t total = frame_len, hdr = 8;
if (total - hdr > 0) copy_payload(buf, total - hdr);
```

**Q14.** *Context: helper that returns the first byte or a default.* See §3.6's `first_or_zero`. Identify the bug class, say what ASan/UBSan might report, and give two fixes.

### 5.6 Implement

(Edit `code/include/es_types/*.hpp`. Run `ctest` often.)

**Q15.** *Context: a sequence counter on a radio link.* Implement `add_u8_wrap` and `u8_add_overflows` in `promotion.hpp`.

**Q16.** *Context: reading multi-byte fields from a receive buffer.* Implement `read_be16`, `read_be32`, `read_be48`. All three must work for every byte value; `read_be48` must not sign-extend and must not shift an `int` by >= 32.

**Q17.** *Context: signed measurements packed in odd bit widths.* Implement `sign_extend`, `less_mixed`, `narrow_u8` in `promotion.hpp`. `sign_extend(0xFFFFFFFF, 32)` must work too.

**Q18.** *Context: raw frame dump for logging.* Implement `high_nibble`, `nibble_swap`, `to_hex` in `bytes_enums.hpp` with `std::byte`.

**Q19.** *Context: protocol and flag fields from the wire.* Implement `name`, `protocol_from_u8` and the three `Flags` operators/`has`.

**Q20.** *Context: scanning a capture buffer for sync bytes.* Implement `swap_u8`, `find_byte`, `count_nonzero`, `clamp_inplace`, `fill_pattern` in `refs_constexpr.hpp`.

**Q21.** *Context: lookup tables in flash.* Implement `mask`, `bytes_for_bits`, `reverse_bits8`, `make_reverse_table`. `mask(32)` must not trigger UBSan.

### 5.7 Connect

**Q22.** *Context: pack 01 (`../01-toolchain/`) taught sanitizers.* Which sanitizer reports the `uint16_t * uint16_t` overflow of Q4(e), and which compile warning flags would have caught the narrowing in `uint8_t d = a + b;`? Why does your test build use both?

**Q23.** *Context: pack 04 (`../04-containers-templates/`) introduces `std::span`.* Re-express the `(const uint8_t* p, size_t n)` convention from `count_nonzero` with a span. What bug class does it remove, and what does it *not* remove? Also: where does pack 05 (`../05-ub-layout-bit/`) pick up the shift-by-width problem you saw in `mask(32)`?

### 5.8 Interview-style

**Q24.** "In C++, what is the type of `a + b` when both are `uint8_t`? Why? What about `uint32_t + int`?"

**Q25.** "What's the difference between `const`, `constexpr` and `consteval`, and between `const T*` and `T* const`? When is a `constexpr` function evaluated at run time?"

**Q26.** "Why would you use `std::byte` rather than `unsigned char` or `char` for raw bytes?"

**Q27.** "Write `int32_t sign_extend(uint32_t v, unsigned bits)` on the whiteboard and list its edge cases."

---

## 6. Self-assessment

- [ ] I can explain integral promotion and the usual arithmetic conversions with a number (300, -16, 4,294,836,225).
- [ ] I can predict when `~x`, `x << n`, and `a * b` on small unsigned types give surprising results.
- [ ] I can derive two's complement and sign-extend an N-bit field by hand.
- [ ] I can explain the `-1 < 1u` trap and the `size_t` underflow trap.
- [ ] I know why plain `char` is not for bytes and what `std::byte` is for.
- [ ] I can justify `enum class` and validate wire values before casting.
- [ ] I can read any const/pointer declaration right-to-left and explain half-open ranges.
- [ ] I can write and test `constexpr` helpers including edge widths.
- [ ] All tests pass with sanitizers on.
- [ ] I can answer Q24-Q27 aloud.
