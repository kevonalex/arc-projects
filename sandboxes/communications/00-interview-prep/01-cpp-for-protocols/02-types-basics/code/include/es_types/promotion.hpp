#pragma once
// STUBS - worksheet Q15..Q17. Tests fail until you implement them.
#include <cstddef>
#include <cstdint>

namespace es::types {

// (a + b) modulo 256. E.g. add_u8_wrap(200, 100) == 44.
inline std::uint8_t add_u8_wrap(std::uint8_t a, std::uint8_t b) {
    (void)a; (void)b;
    return 0;  // TODO
}

// true iff the mathematical sum a + b does not fit in 8 bits.
inline bool u8_add_overflows(std::uint8_t a, std::uint8_t b) {
    (void)a; (void)b;
    return false;  // TODO
}

// Big-endian ("network order") loads. Must be correct for ALL byte values,
// in particular when the first byte is >= 0x80.
inline std::uint16_t read_be16(const std::uint8_t* p) {
    (void)p;
    return 0;  // TODO
}
inline std::uint32_t read_be32(const std::uint8_t* p) {
    (void)p;
    return 0;  // TODO
}
// 6-byte big-endian value (e.g. a 48-bit timestamp/counter field).
inline std::uint64_t read_be48(const std::uint8_t* p) {
    (void)p;
    return 0;  // TODO  (trap: shifting an int by 40 bits)
}

// Interpret the low `bits` bits of `value` as a two's-complement number.
// 1 <= bits <= 32. Bits above `bits` are ignored.
// sign_extend(0xFFF, 12) == -1, sign_extend(0x800, 12) == -2048.
inline std::int32_t sign_extend(std::uint32_t value, unsigned bits) {
    (void)value; (void)bits;
    return 12345;  // TODO
}

// Mathematically correct a < b for a signed a and unsigned b.
// (The built-in `-1 < 1u` is FALSE; yours must return true.)
inline bool less_mixed(int a, unsigned b) {
    (void)a; (void)b;
    return false;  // TODO
}

// If v is in [0,255] store it in out and return true; otherwise return false and leave out untouched.
inline bool narrow_u8(int v, std::uint8_t& out) {
    (void)v; (void)out;
    return false;  // TODO
}

}  // namespace es::types
