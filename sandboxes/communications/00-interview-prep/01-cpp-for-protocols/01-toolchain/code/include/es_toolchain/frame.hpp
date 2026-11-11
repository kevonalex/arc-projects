#pragma once
// STUB - worksheet Q16 / Q17.
//
// Wire format of a toy debug frame:
//   byte 0      : 0x7E  (start marker)
//   byte 1      : N     (payload length, 0..255)
//   bytes 2..N+1: payload
//   byte N+2    : xor_checksum(payload)
// Total size = N + 3.
#include <cstddef>
#include <cstdint>
#include <es_toolchain/checksum.hpp>

namespace es::toolchain {

inline constexpr std::uint8_t kFrameStart = 0x7E;
inline constexpr std::size_t kFrameOverhead = 3;
inline constexpr std::size_t kMaxPayload = 255;
inline constexpr std::size_t kDecodeError = static_cast<std::size_t>(-1);

// Writes a frame into out[0..out_cap). Returns bytes written, or 0 if the
// payload is > kMaxPayload or the frame would not fit. MUST NOT write past
// out_cap (ASan will tell you if you do).
inline std::size_t frame_encode(const std::uint8_t* payload, std::size_t n,
                                std::uint8_t* out, std::size_t out_cap) {
    (void)payload; (void)n; (void)out; (void)out_cap;
    return 0;  // TODO
}

// Validates a frame and copies the payload to out. Returns payload length or
// kDecodeError if: too short, wrong start byte, length field disagrees with
// in_len, bad checksum, or out_cap too small.
inline std::size_t frame_decode(const std::uint8_t* in, std::size_t in_len,
                                std::uint8_t* out, std::size_t out_cap) {
    (void)in; (void)in_len; (void)out; (void)out_cap;
    return 0;  // TODO
}

}  // namespace es::toolchain
