#pragma once
// SOLUTION - attempt Q20..Q21 first.
#include <array>
#include <cstddef>
#include <cstdint>

namespace es::types {

inline void swap_u8(std::uint8_t& a, std::uint8_t& b) {
    const std::uint8_t t = a;  // if a and b alias the same object this is still correct
    a = b;
    b = t;
}

inline const std::uint8_t* find_byte(const std::uint8_t* begin, const std::uint8_t* end, std::uint8_t v) {
    for (const std::uint8_t* it = begin; it != end; ++it) {
        if (*it == v) return it;
    }
    return end;  // "not found" is `end`, the same convention as the standard library
}

inline std::size_t count_nonzero(const std::uint8_t* p, std::size_t n) {
    std::size_t c = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (p[i] != 0) ++c;
    }
    return c;
}

inline void clamp_inplace(int& v, int lo, int hi) {
    if (v < lo) v = lo;
    else if (v > hi) v = hi;
}

inline void fill_pattern(std::uint8_t* p, std::size_t n, std::uint8_t start) {
    for (std::size_t i = 0; i < n; ++i) {
        p[i] = static_cast<std::uint8_t>(start + i);  // start + i is size_t; narrowing = mod 256
    }
}

constexpr std::uint32_t mask(unsigned width) {
    return width >= 32 ? 0xFFFFFFFFu : ((1u << width) - 1u);  // avoid 1u << 32 (UB)
}

constexpr std::size_t bytes_for_bits(std::size_t bits) {
    return (bits + 7) / 8;
}

constexpr std::uint8_t reverse_bits8(std::uint8_t b) {
    unsigned in = b;
    unsigned out = 0;
    for (int i = 0; i < 8; ++i) {
        out = (out << 1) | (in & 1u);  // take the lowest input bit, push it in at the bottom of out
        in >>= 1;
    }
    return static_cast<std::uint8_t>(out);
}

constexpr std::array<std::uint8_t, 256> make_reverse_table() {
    std::array<std::uint8_t, 256> t{};
    for (unsigned i = 0; i < 256; ++i) {
        t[i] = reverse_bits8(static_cast<std::uint8_t>(i));
    }
    return t;
}
inline constexpr std::array<std::uint8_t, 256> kReverseTable = make_reverse_table();

}  // namespace es::types
