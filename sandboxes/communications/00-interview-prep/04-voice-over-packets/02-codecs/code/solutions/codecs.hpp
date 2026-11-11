// SOLUTION - 04-audio/02-codecs. Pure C++17, header-only. Tests: tests/test_codecs.cpp
// G.711 mu-law / A-law (reference algorithms), a codec table, and the arithmetic of radio vocoders.
// Vocoder numbers are textbook figures from public descriptions - flagged VERIFY in the worksheet. No vocoder algorithm
// is implemented (IMBE/AMBE+2 are patented, DVSI-licensed); only frame arithmetic.
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace ether {

// ============================================================================================
// G.711 mu-law (PCMU, RTP PT 0)
//   Idea: the ear judges loudness logarithmically, so the quantiser step should grow with the signal. mu-law compresses
//   |x| with ln(1 + mu|x|)/ln(1 + mu), mu = 255, then quantises UNIFORMLY to 7 bits (+ sign). The standard approximates the
//   curve with 8 segments x 16 steps; the step DOUBLES each segment. Byte layout (before the final inversion):
//        S EEE MMMM     S = sign, E = segment (exponent), M = step within the segment (mantissa)
//   All bits are inverted on the wire (so silence is 0xFF, and long runs of zero samples are 1-bits - old line-coding reasons).
//   A bias of 0x84 (132) is added before finding the segment so that segment boundaries fall at powers of two.
// ============================================================================================
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

// ============================================================================================
// G.711 A-law (PCMA, RTP PT 8) - same idea, European variant: 13-bit input, even bits inverted (XOR 0x55), no bias.
// ============================================================================================
inline uint8_t alaw_encode(int16_t pcm) {
    static const int seg_end[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
    int v = pcm >> 3;                                  // 16 -> 13 bit (arithmetic shift: floor)
    int mask;
    if (v >= 0) mask = 0xD5;                           // sign bit set for positive, plus the 0x55 alternate-bit inversion
    else { mask = 0x55; v = -v - 1; }
    int seg = 0;
    while (seg < 8 && v > seg_end[seg]) ++seg;
    if (seg >= 8) return static_cast<uint8_t>(0x7F ^ mask);
    int a = seg << 4;
    a |= (seg < 2) ? ((v >> 1) & 0x0F) : ((v >> seg) & 0x0F);
    return static_cast<uint8_t>(a ^ mask);
}
inline int16_t alaw_decode(uint8_t a) {
    a ^= 0x55;
    int t = (a & 0x0F) << 4;
    int seg = (a & 0x70) >> 4;
    switch (seg) {
        case 0: t += 8; break;
        case 1: t += 0x108; break;
        default: t += 0x108; t <<= (seg - 1); break;
    }
    return static_cast<int16_t>((a & 0x80) ? t : -t);
}

// Gateway interworking PCMU <-> PCMA (decode to linear, re-encode: lossy at the segment edges)
inline uint8_t mulaw_to_alaw(uint8_t u) { return alaw_encode(mulaw_decode(u)); }
inline uint8_t alaw_to_mulaw(uint8_t a) { return mulaw_encode(alaw_decode(a)); }

inline std::vector<uint8_t> mulaw_encode_buffer(const std::vector<int16_t>& pcm) {
    std::vector<uint8_t> out(pcm.size());
    for (size_t i = 0; i < pcm.size(); ++i) out[i] = mulaw_encode(pcm[i]);
    return out;
}
inline std::vector<int16_t> mulaw_decode_buffer(const std::vector<uint8_t>& u) {
    std::vector<int16_t> out(u.size());
    for (size_t i = 0; i < u.size(); ++i) out[i] = mulaw_decode(u[i]);
    return out;
}

// The continuous curve the standard approximates (x in [-1,1]):  F(x) = sign(x) ln(1 + mu|x|) / ln(1 + mu)
inline double mu_compress(double x, double mu = 255.0) {
    double a = std::fabs(x) > 1.0 ? 1.0 : std::fabs(x);
    double y = std::log1p(mu * a) / std::log1p(mu);
    return x < 0 ? -y : y;
}
inline double mu_expand(double y, double mu = 255.0) {
    double a = std::fabs(y) > 1.0 ? 1.0 : std::fabs(y);
    double x = (std::pow(1.0 + mu, a) - 1.0) / mu;
    return y < 0 ? -x : x;
}

// ============================================================================================
// Codec table (RTP static payload types from RFC 3551; dynamic ones are negotiated in SDP: static_pt = -1)
// ============================================================================================
struct CodecInfo {
    const char* name;
    int static_pt;              // -1 = dynamic
    uint32_t clock_hz;          // RTP timestamp clock (what 'a=rtpmap:' says)
    uint32_t sample_rate_hz;    // actual audio sampling rate (may differ from the clock!)
    uint32_t channels;
    uint32_t bitrate_bps;       // nominal; 0 = variable
    uint32_t frame_us;          // codec frame duration in microseconds; 0 = sample-based (G.711, L16) or variable (Opus)
    uint32_t frame_bits;        // bits per frame at the nominal mode; 0 if not frame-based
};

inline const std::vector<CodecInfo>& codec_table() {
    static const std::vector<CodecInfo> t = {
        {"PCMU",   0, 8000, 8000, 1, 64000, 0, 0},
        {"GSM",    3, 8000, 8000, 1, 13000, 20000, 260},
        {"PCMA",   8, 8000, 8000, 1, 64000, 0, 0},
        {"G722",   9, 8000, 16000, 1, 64000, 0, 0},                  // sampled at 16 kHz, RTP clock 8000 (historical)
        {"L16",   10, 44100, 44100, 2, 1411200, 0, 0},               // PT 10 = stereo, PT 11 = mono
        {"CN",    13, 8000, 8000, 1, 0, 0, 0},                       // comfort noise
        {"G729",  18, 8000, 8000, 1, 8000, 10000, 80},
        {"AMR",   -1, 8000, 8000, 1, 12200, 20000, 244},             // top mode 12.2 kbit/s
        {"AMR-WB",-1, 16000, 16000, 1, 23850, 20000, 477},           // top mode 23.85 kbit/s
        {"opus",  -1, 48000, 48000, 2, 0, 0, 0},                     // RTP clock ALWAYS 48000, 'channels' in rtpmap is always 2
    };
    return t;
}
inline bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
inline const CodecInfo* find_codec(const std::string& name) {
    for (const auto& c : codec_table()) if (iequals(c.name, name)) return &c;
    return nullptr;
}
inline const CodecInfo* find_codec_by_pt(int pt) {
    if (pt < 0) return nullptr;
    for (const auto& c : codec_table()) if (c.static_pt == pt) return &c;
    return nullptr;
}
// Whole codec frames in a packet of ptime_ms (0 for sample-based/variable codecs, or if ptime is not a multiple of the frame)
inline uint32_t frames_per_packet(const CodecInfo& c, uint32_t ptime_ms) {
    if (c.frame_us == 0) return 0;
    uint64_t us = uint64_t{ptime_ms} * 1000;
    return us % c.frame_us == 0 ? static_cast<uint32_t>(us / c.frame_us) : 0;
}
// RTP timestamp step per packet = clock * ptime / 1000
inline uint32_t codec_ts_step(const CodecInfo& c, uint32_t ptime_ms) { return static_cast<uint32_t>(uint64_t{c.clock_hz} * ptime_ms / 1000); }
// Payload bytes per packet (nominal mode, ignoring payload-format headers such as AMR's CMR/ToC):
//   frame-based: frames * ceil(frame_bits/8); sample-based: ceil(bitrate * ptime / 8000); variable (Opus): 0
inline uint32_t codec_payload_bytes_nominal(const CodecInfo& c, uint32_t ptime_ms) {
    if (c.frame_us != 0) return frames_per_packet(c, ptime_ms) * ((c.frame_bits + 7) / 8);
    if (c.bitrate_bps == 0) return 0;
    return static_cast<uint32_t>((uint64_t{c.bitrate_bps} * ptime_ms + 7999) / 8000);
}
// Opus frame durations (RFC 6716): 2.5, 5, 10, 20, 40, 60 ms (in tenths of ms here: 25, 50, 100, 200, 400, 600)
inline bool opus_valid_frame_tenth_ms(uint32_t tenth_ms) {
    return tenth_ms == 25 || tenth_ms == 50 || tenth_ms == 100 || tenth_ms == 200 || tenth_ms == 400 || tenth_ms == 600;
}

// ============================================================================================
// Radio vocoder arithmetic (frame sizes only). VERIFY every figure against the public standard documents.
//   P25 Phase 1 IMBE : 20 ms frame = 144 bits = 88 voice + 56 FEC            -> 7.2 kb/s (4.4 voice + 2.8 FEC)
//   P25 voice LDU    : 9 IMBE frames per 180 ms Logical Link Data Unit; channel rate 9.6 kb/s -> 1728 bits per LDU
//   AMBE+2 half-rate : 20 ms frame = 72 bits = 49 voice + 23 FEC             -> 3.6 kb/s (DMR: 3 frames = 60 ms per voice burst)
//   TETRA ACELP      : 30 ms frame = 137 bits                                -> 4.567 kb/s (2 frames = 274 bits per 60 ms slot-pair)
// ============================================================================================
struct VocoderInfo {
    const char* name;
    uint32_t frame_us;
    uint32_t voice_bits;
    uint32_t fec_bits;
    uint32_t total_bits() const { return voice_bits + fec_bits; }
    // rounded to the nearest bit/s
    uint32_t bitrate_bps() const { return static_cast<uint32_t>((uint64_t{total_bits()} * 1000000 + frame_us / 2) / frame_us); }
    uint32_t voice_bps() const { return static_cast<uint32_t>((uint64_t{voice_bits} * 1000000 + frame_us / 2) / frame_us); }
};
inline VocoderInfo imbe_full_rate() { return {"IMBE", 20000, 88, 56}; }
inline VocoderInfo ambe_plus2_half_rate() { return {"AMBE+2 half-rate", 20000, 49, 23}; }
inline VocoderInfo tetra_acelp() { return {"TETRA ACELP", 30000, 137, 0}; }       // 137 bits: channel coding is added later in the TETRA stack

constexpr uint32_t kP25ChannelBps = 9600;
constexpr uint32_t kLduFrames = 9;
inline uint32_t ldu_duration_ms() { return kLduFrames * (imbe_full_rate().frame_us / 1000); }                              // 180
inline uint32_t ldu_total_bits() { return kP25ChannelBps * ldu_duration_ms() / 1000; }                                     // 1728
inline uint32_t ldu_voice_bits() { return kLduFrames * imbe_full_rate().total_bits(); }                                    // 1296 (voice + FEC)
inline uint32_t ldu_overhead_bits() { return ldu_total_bits() - ldu_voice_bits(); }                                        // 432: sync, NID, link control / low speed data, status symbols
// bytes needed to carry 'frames' vocoder frames when packed back to back and padded to whole bytes
inline uint32_t vocoder_payload_bytes(const VocoderInfo& v, uint32_t frames) { return (v.total_bits() * frames + 7) / 8; }

}  // namespace ether
