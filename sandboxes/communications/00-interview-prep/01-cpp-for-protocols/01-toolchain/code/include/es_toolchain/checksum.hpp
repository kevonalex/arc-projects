#pragma once
// STUB - worksheet Q15. Tests fail until you implement this.
#include <cstddef>
#include <cstdint>

namespace es::toolchain {

// XOR of all bytes in [data, data+n). Empty input -> 0.
// Etherstack-style use: a trivial integrity byte on an internal debug frame.
inline std::uint8_t xor_checksum(const std::uint8_t* data, std::size_t n) {
    (void)data;
    (void)n;
    return 0xFF;  // TODO: wrong on purpose
}

}  // namespace es::toolchain
