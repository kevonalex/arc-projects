#pragma once
// STUBS - worksheet Q20 (bit manipulation, endianness, type punning without UB).
// Write these by hand FIRST; the tests cross-check against std::popcount, std::countl_zero,
// std::bit_cast (C++20 <bit>) when your standard library has them.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace es::ubbit {

// Number of 1 bits. Hand-written (Kernighan: v &= v - 1 clears the lowest set bit).
inline unsigned popcount32(std::uint32_t v) {
    (void)v;
    return 99;  // TODO
}

// 1 if the number of set bits is odd, else 0 (xor-fold, no loop over 32 bits needed).
inline unsigned parity32(std::uint32_t v) {
    (void)v;
    return 99;  // TODO
}

// Number of leading zero bits; clz32(0) == 32.
inline unsigned clz32(std::uint32_t v) {
    (void)v;
    return 99;  // TODO
}

inline std::uint16_t byteswap16(std::uint16_t v) {
    (void)v;
    return 0;  // TODO  0x1234 -> 0x3412
}
inline std::uint32_t byteswap32(std::uint32_t v) {
    (void)v;
    return 0;  // TODO  0x11223344 -> 0x44332211
}

// true on a little-endian host. Determine it by LOOKING AT THE BYTES of an object (memcpy / unsigned char),
// not by pointer casts.
inline bool is_little_endian() {
    return false;  // TODO
}

// Extract `width` bits of v starting at bit `lo` (bit 0 = least significant). 0 <= width <= 32, lo + width <= 32.
// extract_bits(0xABCD1234, 8, 8) == 0x12. Beware: (1u << 32) is UB.
inline std::uint32_t extract_bits(std::uint32_t v, unsigned lo, unsigned width) {
    (void)v; (void)lo; (void)width;
    return 0xDEADBEEF;  // TODO
}

// Replace those bits of v with the low `width` bits of `field` (extra high bits of `field` are ignored).
inline std::uint32_t insert_bits(std::uint32_t v, unsigned lo, unsigned width, std::uint32_t field) {
    (void)v; (void)lo; (void)width; (void)field;
    return 0xDEADBEEF;  // TODO
}

// Reinterpret a float's bits as uint32_t and back WITHOUT undefined behaviour
// (std::memcpy; in C++20 std::bit_cast). 1.0f <-> 0x3F800000.
inline std::uint32_t float_bits(float f) {
    (void)f;
    return 0;  // TODO
}
inline float bits_float(std::uint32_t u) {
    (void)u;
    return 0.0f;  // TODO
}

// Load a uint32_t in HOST byte order from possibly-unaligned bytes, without UB (memcpy).
inline std::uint32_t load_u32_ne(const unsigned char* p) {
    (void)p;
    return 0;  // TODO
}

// Load a big-endian uint32_t from bytes, correct on any host endianness. Build it from load_u32_ne + byteswap32
// (when the host is little-endian) OR by shifting bytes - but your answer must pass on both kinds of host.
inline std::uint32_t load_be32(const unsigned char* p) {
    (void)p;
    return 0;  // TODO
}

}  // namespace es::ubbit
