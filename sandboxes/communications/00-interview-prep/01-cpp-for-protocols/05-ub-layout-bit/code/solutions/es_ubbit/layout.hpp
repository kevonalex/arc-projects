#pragma once
// SOLUTION - attempt Q18, Q19 first.
#include <cstddef>
#include <cstdint>

namespace es::ubbit {

constexpr bool is_pow2(std::size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

// For a power of two a, a - 1 is a mask of the low log2(a) bits (8 -> 0b0111).
// Adding a-1 pushes any non-multiple past the next boundary; clearing the low bits snaps down to it.
constexpr std::size_t align_up(std::size_t x, std::size_t a) {
    return (x + a - 1) & ~(a - 1);
}

inline bool is_aligned(const void* p, std::size_t a) {
    return reinterpret_cast<std::uintptr_t>(p) % a == 0;  // converting a pointer to an integer is fine
}

struct FieldSpec {
    std::size_t size;
    std::size_t align;
};

constexpr std::size_t field_offset(const FieldSpec* f, std::size_t index) {
    std::size_t off = 0;
    for (std::size_t i = 0; i < index; ++i) {
        off = align_up(off, f[i].align) + f[i].size;  // place field i, move to its end
    }
    return align_up(off, f[index].align);              // padding before field `index`
}

constexpr std::size_t struct_size(const FieldSpec* f, std::size_t n) {
    std::size_t off = 0;
    std::size_t max_align = 1;
    for (std::size_t i = 0; i < n; ++i) {
        off = align_up(off, f[i].align) + f[i].size;
        if (f[i].align > max_align) max_align = f[i].align;
    }
    return align_up(off, max_align);  // tail padding
}

struct Header {
    std::uint8_t type;
    std::uint16_t length;
    std::uint32_t id;
};

inline constexpr std::size_t kHeaderWireSize = 7;

inline std::size_t write_header(const Header& h, std::uint8_t* out, std::size_t cap) {
    if (cap < kHeaderWireSize) return 0;
    out[0] = h.type;
    out[1] = static_cast<std::uint8_t>(h.length >> 8);
    out[2] = static_cast<std::uint8_t>(h.length);
    out[3] = static_cast<std::uint8_t>(h.id >> 24);
    out[4] = static_cast<std::uint8_t>(h.id >> 16);
    out[5] = static_cast<std::uint8_t>(h.id >> 8);
    out[6] = static_cast<std::uint8_t>(h.id);
    return kHeaderWireSize;
}

inline bool read_header(const std::uint8_t* in, std::size_t n, Header& out) {
    if (n < kHeaderWireSize) return false;
    out.type = in[0];
    out.length = static_cast<std::uint16_t>((in[1] << 8) | in[2]);
    out.id = (static_cast<std::uint32_t>(in[3]) << 24) | (static_cast<std::uint32_t>(in[4]) << 16) |
             (static_cast<std::uint32_t>(in[5]) << 8) | static_cast<std::uint32_t>(in[6]);
    return true;
}

}  // namespace es::ubbit
