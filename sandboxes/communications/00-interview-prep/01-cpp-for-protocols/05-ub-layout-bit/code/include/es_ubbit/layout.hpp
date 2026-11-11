#pragma once
// STUBS - worksheet Q18 (alignment/padding) and Q19 (explicit serialisation).
#include <cstddef>
#include <cstdint>

namespace es::ubbit {

// ---- alignment arithmetic ----------------------------------------------------------
constexpr bool is_pow2(std::size_t x) {
    (void)x;
    return false;  // TODO: exactly one bit set. Hint: x & (x - 1) clears the lowest set bit.
}

// Round x up to the next multiple of `a`. Precondition: a is a power of two.
// align_up(0,8)=0, (1,8)=8, (8,8)=8, (9,8)=16. Use a mask, not division (derive why it works).
constexpr std::size_t align_up(std::size_t x, std::size_t a) {
    (void)a;
    return x;  // TODO
}

// true iff the ADDRESS is a multiple of `a`.
inline bool is_aligned(const void* p, std::size_t a) {
    (void)p; (void)a;
    return false;  // TODO
}

// ---- compute struct layout by hand ---------------------------------------------------
struct FieldSpec {
    std::size_t size;
    std::size_t align;
};

// Offset of field `index` when the fields are laid out in order with natural alignment
// (each field starts at the next multiple of its own alignment).
constexpr std::size_t field_offset(const FieldSpec* f, std::size_t index) {
    (void)f; (void)index;
    return 12345;  // TODO
}

// sizeof the whole struct: end of the last field, rounded up to the LARGEST field alignment
// (so that arrays of the struct keep every element aligned). n == 0 -> 0.
constexpr std::size_t struct_size(const FieldSpec* f, std::size_t n) {
    (void)f; (void)n;
    return 12345;  // TODO
}

// ---- explicit, portable serialisation -----------------------------------------------------
// In memory this struct has padding and the host's endianness; on the wire it must not.
struct Header {
    std::uint8_t type;
    std::uint16_t length;
    std::uint32_t id;
};

// Wire format (7 bytes, big-endian): type | length(2) | id(4)
inline constexpr std::size_t kHeaderWireSize = 7;

// Returns 7 on success, 0 (writing nothing) if cap < 7.
inline std::size_t write_header(const Header& h, std::uint8_t* out, std::size_t cap) {
    (void)h; (void)out; (void)cap;
    return 0;  // TODO
}

// Returns false (leaving `out` untouched) if n < 7.
inline bool read_header(const std::uint8_t* in, std::size_t n, Header& out) {
    (void)in; (void)n; (void)out;
    return false;  // TODO
}

}  // namespace es::ubbit
