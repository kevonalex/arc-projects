#pragma once
// SOLUTION - attempt Q18..Q19 first.
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace es::types {

inline std::byte high_nibble(std::byte b) {
    return b >> 4;  // std::byte supports shifts by an integer; result is a byte
}

inline std::byte nibble_swap(std::byte b) {
    return (b << 4) | (b >> 4);  // (b<<4) is truncated to 8 bits by std::byte's operator<<
}

inline std::string to_hex(const std::byte* p, std::size_t n) {
    static const char kDigits[] = "0123456789ABCDEF";
    std::string s;
    s.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
        const unsigned v = std::to_integer<unsigned>(p[i]);
        s.push_back(kDigits[v >> 4]);
        s.push_back(kDigits[v & 0xF]);
    }
    return s;
}

template <class E>
constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

enum class Protocol : std::uint8_t { P25 = 0, DMR = 1, TETRA = 2 };

inline const char* name(Protocol p) {
    switch (p) {
        case Protocol::P25: return "P25";
        case Protocol::DMR: return "DMR";
        case Protocol::TETRA: return "TETRA";
    }
    return "UNKNOWN";  // reached for out-of-range values; no `default:` so -Wswitch still helps
}

inline bool protocol_from_u8(std::uint8_t v, Protocol& out) {
    if (v > to_underlying(Protocol::TETRA)) return false;
    out = static_cast<Protocol>(v);
    return true;
}

enum class Flags : std::uint8_t { None = 0, Encrypted = 1, Emergency = 2, Data = 4 };

constexpr Flags operator|(Flags a, Flags b) {
    return static_cast<Flags>(static_cast<std::uint8_t>(to_underlying(a) | to_underlying(b)));
}
constexpr Flags operator&(Flags a, Flags b) {
    return static_cast<Flags>(static_cast<std::uint8_t>(to_underlying(a) & to_underlying(b)));
}
constexpr bool has(Flags f, Flags bits) {
    return (f & bits) == bits;
}

}  // namespace es::types
