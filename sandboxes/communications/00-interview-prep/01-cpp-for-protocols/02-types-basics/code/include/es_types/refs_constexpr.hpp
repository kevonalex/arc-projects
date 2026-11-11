#pragma once
// STUBS - worksheet Q20..Q21 (references, pointers, const, constexpr).
#include <array>
#include <cstddef>
#include <cstdint>

namespace es::types {

// ---- references / pointers / const ---------------------------------------------
// Swap two bytes through references (no pointers, no copies of the objects).
inline void swap_u8(std::uint8_t& a, std::uint8_t& b) {
    (void)a; (void)b;  // TODO
}

// Returns pointer to the first byte equal to v in [begin, end), or `end` if absent.
// Both pointers are to const data: a search must not modify what it searches.
inline const std::uint8_t* find_byte(const std::uint8_t* begin, const std::uint8_t* end, std::uint8_t v) {
    (void)begin; (void)v; (void)end;
    return nullptr;  // TODO (note: nullptr is NOT the "not found" answer here)
}

// Number of non-zero bytes in [p, p+n).
inline std::size_t count_nonzero(const std::uint8_t* p, std::size_t n) {
    (void)p; (void)n;
    return 999;  // TODO
}

// Clamp v into [lo, hi] IN PLACE. Precondition lo <= hi.
inline void clamp_inplace(int& v, int lo, int hi) {
    (void)v; (void)lo; (void)hi;  // TODO
}

// p[i] = (start + i) modulo 256 for i in [0,n).
inline void fill_pattern(std::uint8_t* p, std::size_t n, std::uint8_t start) {
    (void)p; (void)n; (void)start;  // TODO
}

// ---- constexpr -------------------------------------------------------------------
// Low `width` bits set. mask(0)==0, mask(8)==0xFF, mask(32)==0xFFFFFFFF.
// Careful: 1u << 32 is undefined behaviour (pack 05).
constexpr std::uint32_t mask(unsigned width) {
    (void)width;
    return 1;  // TODO
}

// Bytes needed to hold `bits` bits: ceil(bits / 8). bytes_for_bits(0)==0, (1)==1, (9)==2.
constexpr std::size_t bytes_for_bits(std::size_t bits) {
    (void)bits;
    return 99;  // TODO
}

// Reverse the bit order in a byte: 0x01 -> 0x80, 0x2C (0010 1100) -> 0x34 (0011 0100).
// (Some links send LSB first, some MSB first.)
constexpr std::uint8_t reverse_bits8(std::uint8_t b) {
    (void)b;
    return 0;  // TODO
}

// All 256 answers computed at COMPILE time.
constexpr std::array<std::uint8_t, 256> make_reverse_table() {
    std::array<std::uint8_t, 256> t{};
    return t;  // TODO: fill with reverse_bits8(i)
}
inline constexpr std::array<std::uint8_t, 256> kReverseTable = make_reverse_table();

}  // namespace es::types
