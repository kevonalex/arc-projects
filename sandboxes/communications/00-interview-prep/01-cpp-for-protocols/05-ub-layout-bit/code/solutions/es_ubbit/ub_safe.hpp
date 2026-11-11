#pragma once
// SOLUTION - attempt Q16, Q17 first.
#include <cstdint>
#include <limits>
#include <optional>

namespace es::ubbit {

inline std::optional<std::int32_t> checked_add(std::int32_t a, std::int32_t b) {
    constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();
    constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();
    // Rearranged so that the test itself can never overflow:
    //   a + b > kMax   <=>   a > kMax - b      (kMax - b is safe when b > 0)
    //   a + b < kMin   <=>   a < kMin - b      (kMin - b is safe when b < 0)
    if (b > 0 && a > kMax - b) return std::nullopt;
    if (b < 0 && a < kMin - b) return std::nullopt;
    return a + b;
}

inline std::int32_t sat_add(std::int32_t a, std::int32_t b) {
    if (auto r = checked_add(a, b)) return *r;
    return b > 0 ? std::numeric_limits<std::int32_t>::max() : std::numeric_limits<std::int32_t>::min();
}

inline std::int32_t wrapping_add(std::int32_t a, std::int32_t b) {
    // Unsigned arithmetic is DEFINED to wrap modulo 2^32. Converting the result back to int32_t is
    // implementation-defined before C++20 (every real compiler wraps) and defined (modular) from C++20.
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
}

inline std::optional<std::int32_t> checked_abs(std::int32_t a) {
    if (a == std::numeric_limits<std::int32_t>::min()) return std::nullopt;  // -INT_MIN overflows
    return a < 0 ? -a : a;
}

inline std::uint32_t shl(std::uint32_t v, unsigned n) {
    return n >= 32 ? 0u : (v << n);
}
inline std::uint32_t shr(std::uint32_t v, unsigned n) {
    return n >= 32 ? 0u : (v >> n);
}

inline std::uint32_t rotl32_manual(std::uint32_t v, unsigned n) {
    n &= 31u;                       // rotation is modulo the width
    if (n == 0) return v;           // avoids v >> 32 (UB)
    return (v << n) | (v >> (32u - n));
}

}  // namespace es::ubbit
