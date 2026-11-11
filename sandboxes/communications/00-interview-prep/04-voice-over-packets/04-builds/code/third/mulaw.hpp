// REFERENCE COPY of the G.711 mu-law codec from 04-audio/02-codecs (solutions/codecs.hpp). Given, not edited here:
// this pack TESTS it exhaustively and builds on it.
#pragma once
#include <cstdint>

namespace ether {

constexpr int kMuBias = 0x84;      // 132
constexpr int kMuClip = 32635;     // largest magnitude that does not overflow after the bias

inline uint8_t mulaw_encode(int16_t pcm) {
    int s = pcm;
    int sign = 0;
    if (s < 0) { s = -s; sign = 0x80; }               // -(-32768) = 32768 fits in int, then it is clipped
    if (s > kMuClip) s = kMuClip;
    s += kMuBias;
    int exponent = 7;
    for (int mask = 0x4000; (s & mask) == 0 && exponent > 0; mask >>= 1) --exponent;
    int mantissa = (s >> (exponent + 3)) & 0x0F;
    return static_cast<uint8_t>(~(sign | (exponent << 4) | mantissa));
}
inline int16_t mulaw_decode(uint8_t u) {
    u = static_cast<uint8_t>(~u);
    int sign = u & 0x80, exponent = (u >> 4) & 7, mantissa = u & 0x0F;
    int s = (((mantissa << 3) + kMuBias) << exponent) - kMuBias;
    return static_cast<int16_t>(sign ? -s : s);
}
// segment (exponent) of a wire byte, and the quantiser step size in 16-bit linear units: 8, 16, 32 ... 1024
inline unsigned mulaw_segment(uint8_t code) { return ((~code) >> 4) & 7; }
inline int mulaw_step(uint8_t code) { return 1 << (mulaw_segment(code) + 3); }

}  // namespace ether
