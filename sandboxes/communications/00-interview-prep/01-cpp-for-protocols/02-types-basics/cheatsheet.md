# Pack 02 cheat sheet - every line points back to the worksheet

**Fixed-width types**
- `int` size varies; use `std::uint8_t/16/32/64_t` for protocol fields, `std::size_t` for sizes - §3.1
- `unsigned long` is 32 or 64 bits depending on platform -> never on the wire - §3.1

**Promotion and conversions**
- operands narrower than `int` -> `int` BEFORE arithmetic - §3.2
- `uint8_t{200}+uint8_t{100}` is `int` 300; assign to `uint8_t` -> 44 - §3.2
- `~uint8_t{0x0F}` is `int` -16 (`0xFFFFFFF0`), not `0xF0`; cast back: `static_cast<uint8_t>(~x)` - §3.2
- `uint16_t*uint16_t` can overflow SIGNED int (65535^2 > INT_MAX) -> UB; widen an operand first - §3.2
- mixed signed/unsigned same rank -> signed becomes unsigned: `-1 < 1u` is false - §3.2, §3.3
- cast the operand before the operation, not the result after - §3.2 (Q11)

**Signed/unsigned**
- two's complement: `-x = ~x + 1`; n-bit range `-2^(n-1)..2^(n-1)-1` - §3.3
- unsigned overflow wraps (defined); signed overflow is UB - §3.3
- sign-extend N-bit field: mask, if sign bit set subtract `2^N` (12-bit `0xFFF` = -1) - §3.3, §4
- `size_t a - b` with `a<b` underflows to huge; compare first (`a > b`) - §3.3 (Q13)
- `size_t i >= 0` is always true -> infinite loop - §3.3

**Bytes**
- plain `char` signedness is implementation-defined (x86 signed, ARM often unsigned) - §3.4
- use `uint8_t` or `std::byte` for raw bytes; `std::to_integer<T>` to read - §3.4
- `uint8_t` prints as a character via iostream; cast to int to print - §3.1, §3.4

**enum class**
- scoped, no implicit int conversion, fixed underlying type (`: uint8_t`) - §3.5
- validate wire value BEFORE `static_cast<Enum>` - §3.5 (Q2)
- bit flags: write `|`, `&` via `to_underlying`; `has` = `(f & bits) == bits` - §3.5
- switch without `default:` keeps `-Wswitch` useful - §3.5

**References, pointers, const**
- reference = alias, bound once, no null; pointer = address, nullable, re-seatable - §3.6
- ranges are half-open `[begin,end)`; "not found" = `end`; one-past-end formable, not dereferenceable - §3.6
- read const right-to-left: `const T*` (pointee const) vs `T* const` (pointer const) - §3.6
- function that only reads takes `const`; returned refs to locals dangle - §3.6 (Q14)

**constexpr**
- `constexpr` = may be evaluated at compile time; function can also run at run time - §3.7
- `static_assert` + `constexpr` tables -> flash, no boot code - §3.7
- `inline constexpr` for header variables (C++17) - §3.7
- `mask(32)`: `1u << 32` is UB -> branch for full width - §3.7, Q21
- DMR example: 4800 sym/s x 30 ms = 144 symbols per slot (VERIFY) - §3.7
