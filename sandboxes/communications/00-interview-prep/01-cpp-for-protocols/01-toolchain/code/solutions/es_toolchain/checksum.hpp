#pragma once
// SOLUTION - do not read before attempting Q15.
#include <cstddef>
#include <cstdint>

namespace es::toolchain {

inline std::uint8_t xor_checksum(const std::uint8_t* data, std::size_t n) {
    std::uint8_t acc = 0;
    for (std::size_t i = 0; i < n; ++i) {
        // acc ^ data[i] is computed as int (promotion, pack 02); the cast
        // narrows back. Value always fits because both inputs are <= 0xFF.
        acc = static_cast<std::uint8_t>(acc ^ data[i]);
    }
    return acc;
}

}  // namespace es::toolchain
