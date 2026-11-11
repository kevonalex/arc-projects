#pragma once
// STUBS - worksheet Q18..Q19 (std::byte, enum class).
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace es::types {

// ---- std::byte --------------------------------------------------------------
// Upper nibble moved down: high_nibble(0xAB) == 0x0A.
inline std::byte high_nibble(std::byte b) {
    (void)b;
    return std::byte{0xFF};  // TODO
}
// nibble_swap(0xAB) == 0xBA.
inline std::byte nibble_swap(std::byte b) {
    (void)b;
    return std::byte{0xFF};  // TODO
}
// Upper-case hex, two digits per byte, no separators: {0x00,0xAB,0x7E} -> "00AB7E".
inline std::string to_hex(const std::byte* p, std::size_t n) {
    (void)p; (void)n;
    return "?";  // TODO
}

// ---- enum class -----------------------------------------------------------------
// Provided (C++23 has std::to_underlying; we are on 17/20).
template <class E>
constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

enum class Protocol : std::uint8_t { P25 = 0, DMR = 1, TETRA = 2 };

// "P25", "DMR", "TETRA"; anything else -> "UNKNOWN".
inline const char* name(Protocol p) {
    (void)p;
    return "?";  // TODO
}

// A value read off the wire is just a byte: validate before converting.
// Returns true and sets out iff v is 0, 1 or 2.
inline bool protocol_from_u8(std::uint8_t v, Protocol& out) {
    (void)v; (void)out;
    return false;  // TODO
}

// A bit-flag enum. enum class has NO implicit operators, so you write them.
enum class Flags : std::uint8_t { None = 0, Encrypted = 1, Emergency = 2, Data = 4 };

constexpr Flags operator|(Flags a, Flags b) {
    (void)a; (void)b;
    return Flags::None;  // TODO
}
constexpr Flags operator&(Flags a, Flags b) {
    (void)a; (void)b;
    return Flags::Data;  // TODO
}
// true iff every bit of `bits` is set in `f`.
constexpr bool has(Flags f, Flags bits) {
    (void)f; (void)bits;
    return false;  // TODO
}

}  // namespace es::types
