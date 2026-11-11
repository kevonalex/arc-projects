#pragma once
// SOLUTION - attempt Q15..Q17 first.
#include <cstddef>
#include <cstdint>

namespace es::types {

inline std::uint8_t add_u8_wrap(std::uint8_t a, std::uint8_t b) {
    // a + b is an INT (promotion): 200 + 100 == 300. Converting 300 to uint8_t is
    // well-defined modular arithmetic for UNSIGNED targets: 300 mod 256 == 44.
    return static_cast<std::uint8_t>(a + b);
}

inline bool u8_add_overflows(std::uint8_t a, std::uint8_t b) {
    return (a + b) > 0xFF;  // computed in int, so the true sum is visible
}

inline std::uint16_t read_be16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);  // max 0xFFFF fits int comfortably
}

inline std::uint32_t read_be32(const std::uint8_t* p) {
    // Cast to uint32_t BEFORE shifting: p[0] << 24 would be an int shift into the sign bit.
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

inline std::uint64_t read_be48(const std::uint8_t* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 6; ++i) {
        v = (v << 8) | p[i];  // v is uint64_t, so p[i] converts to uint64_t: no int-width shift anywhere
    }
    return v;
}

inline std::int32_t sign_extend(std::uint32_t value, unsigned bits) {
    // Work in 64-bit signed so that bits == 32 needs no 32-bit shift.
    const std::int64_t mask = (std::int64_t{1} << bits) - 1;
    const std::int64_t sign = std::int64_t{1} << (bits - 1);
    const std::int64_t v = static_cast<std::int64_t>(value) & mask;
    return static_cast<std::int32_t>(v >= sign ? v - (mask + 1) : v);
}

inline bool less_mixed(int a, unsigned b) {
    if (a < 0) return true;                    // a negative number is below any unsigned value
    return static_cast<unsigned>(a) < b;       // now a >= 0: conversion is value-preserving
}

inline bool narrow_u8(int v, std::uint8_t& out) {
    if (v < 0 || v > 255) return false;
    out = static_cast<std::uint8_t>(v);
    return true;
}

}  // namespace es::types
