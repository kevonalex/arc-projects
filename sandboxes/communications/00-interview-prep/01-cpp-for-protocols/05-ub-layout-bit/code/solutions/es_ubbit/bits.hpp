#pragma once
// SOLUTION - attempt Q20 first.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace es::ubbit {

inline unsigned popcount32(std::uint32_t v) {
    unsigned c = 0;
    while (v != 0) {
        v &= v - 1;  // clears the lowest set bit; loop runs once per set bit
        ++c;
    }
    return c;
}

inline unsigned parity32(std::uint32_t v) {
    v ^= v >> 16;  // fold: parity of 32 bits == parity of the xor of the two halves
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return v & 1u;
}

inline unsigned clz32(std::uint32_t v) {
    if (v == 0) return 32;
    unsigned n = 0;
    while ((v & 0x80000000u) == 0) {
        v <<= 1;
        ++n;
    }
    return n;
}

inline std::uint16_t byteswap16(std::uint16_t v) {
    return static_cast<std::uint16_t>((v << 8) | (v >> 8));  // promoted to int, narrowed back
}

inline std::uint32_t byteswap32(std::uint32_t v) {
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) |
           ((v & 0xFF000000u) >> 24);
}

inline bool is_little_endian() {
    const std::uint32_t one = 1;
    unsigned char first;
    std::memcpy(&first, &one, 1);  // inspecting object bytes via memcpy / unsigned char is always allowed
    return first == 1;
}

inline std::uint32_t extract_bits(std::uint32_t v, unsigned lo, unsigned width) {
    if (width == 0) return 0;
    const std::uint32_t mask = (width >= 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    return (v >> lo) & mask;  // lo < 32 by precondition
}

inline std::uint32_t insert_bits(std::uint32_t v, unsigned lo, unsigned width, std::uint32_t field) {
    if (width == 0) return v;
    const std::uint32_t mask = (width >= 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    return (v & ~(mask << lo)) | ((field & mask) << lo);
}

inline std::uint32_t float_bits(float f) {
    static_assert(sizeof(float) == sizeof(std::uint32_t), "float must be 32 bits (IEEE 754 binary32 assumed)");
    std::uint32_t u;
    std::memcpy(&u, &f, sizeof u);  // C++20: return std::bit_cast<std::uint32_t>(f);
    return u;
}

inline float bits_float(std::uint32_t u) {
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}

inline std::uint32_t load_u32_ne(const unsigned char* p) {
    std::uint32_t v;
    std::memcpy(&v, p, sizeof v);  // fine for any alignment; compilers emit a single load where legal
    return v;
}

inline std::uint32_t load_be32(const unsigned char* p) {
    const std::uint32_t v = load_u32_ne(p);
    return is_little_endian() ? byteswap32(v) : v;
}

}  // namespace es::ubbit
