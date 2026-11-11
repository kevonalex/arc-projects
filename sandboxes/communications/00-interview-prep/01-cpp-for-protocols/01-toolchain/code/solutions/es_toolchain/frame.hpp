#pragma once
// SOLUTION - do not read before attempting Q16 / Q17.
#include <cstddef>
#include <cstdint>
#include <es_toolchain/checksum.hpp>

namespace es::toolchain {

inline constexpr std::uint8_t kFrameStart = 0x7E;
inline constexpr std::size_t kFrameOverhead = 3;
inline constexpr std::size_t kMaxPayload = 255;
inline constexpr std::size_t kDecodeError = static_cast<std::size_t>(-1);

inline std::size_t frame_encode(const std::uint8_t* payload, std::size_t n,
                                std::uint8_t* out, std::size_t out_cap) {
    if (n > kMaxPayload) return 0;
    const std::size_t total = n + kFrameOverhead;  // cannot overflow: n <= 255
    if (out_cap < total) return 0;                 // '<' not '<=': exact fit is legal
    out[0] = kFrameStart;
    out[1] = static_cast<std::uint8_t>(n);
    for (std::size_t i = 0; i < n; ++i) out[2 + i] = payload[i];
    out[2 + n] = xor_checksum(payload, n);
    return total;
}

inline std::size_t frame_decode(const std::uint8_t* in, std::size_t in_len,
                                std::uint8_t* out, std::size_t out_cap) {
    if (in_len < kFrameOverhead) return kDecodeError;  // check size BEFORE reading in[1]
    if (in[0] != kFrameStart) return kDecodeError;
    const std::size_t n = in[1];
    if (in_len != n + kFrameOverhead) return kDecodeError;
    if (out_cap < n) return kDecodeError;
    if (xor_checksum(in + 2, n) != in[2 + n]) return kDecodeError;
    for (std::size_t i = 0; i < n; ++i) out[i] = in[2 + i];
    return n;
}

}  // namespace es::toolchain
