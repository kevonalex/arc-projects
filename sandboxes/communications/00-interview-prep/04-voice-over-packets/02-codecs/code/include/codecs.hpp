// STUB - 04-audio/02-codecs. Implement every TODO. Tests: tests/test_codecs.cpp
// Pure C++17, header-only. Frame arithmetic only: no proprietary vocoder is implemented.
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

// ---------------------------------------------------------------- G.711 mu-law (PCMU, PT 0)
// Wire byte (before the final bit inversion):  S EEE MMMM  = sign, segment 0..7, step 0..15. Step size doubles per segment.
// Algorithm: take |x| (clip to 32635), add the bias 0x84, find the position of the highest set bit among bits 14..7 -> exponent,
// mantissa = (x >> (exponent+3)) & 0xF, byte = ~(sign | exponent<<4 | mantissa).
constexpr int kMuBias = 0x84;
constexpr int kMuClip = 32635;
inline uint8_t mulaw_encode(int16_t pcm) { (void)pcm; return 0; }                       // TODO (careful: -(-32768) overflows int16)
// TODO inverse: b = ~byte; sample = (((mantissa << 3) + 0x84) << exponent) - 0x84; apply sign.
inline int16_t mulaw_decode(uint8_t u) { (void)u; return 1; }
inline unsigned mulaw_segment(uint8_t code) { (void)code; return 99; }                  // TODO: exponent of a WIRE byte (remember the inversion)
inline int mulaw_step(uint8_t code) { (void)code; return 0; }                           // TODO: quantiser step in 16-bit units = 1 << (segment + 3)

// ---------------------------------------------------------------- G.711 A-law (PCMA, PT 8)
// 13-bit input (pcm >> 3). Positive: mask 0xD5; negative: mask 0x55 and v = -v - 1. Segment boundaries 0x1F,0x3F,...,0xFFF.
// Segments 0 and 1 share the step; code = seg<<4 | 4 mantissa bits; XOR with the mask. (Reference: ITU-T G.711 / Sun g711.c)
inline uint8_t alaw_encode(int16_t pcm) { (void)pcm; return 0; }                        // TODO
inline int16_t alaw_decode(uint8_t a) { (void)a; return 1; }                            // TODO
inline uint8_t mulaw_to_alaw(uint8_t u) { (void)u; return 0; }                          // TODO: via linear
inline uint8_t alaw_to_mulaw(uint8_t a) { (void)a; return 0; }                          // TODO: via linear
inline std::vector<uint8_t> mulaw_encode_buffer(const std::vector<int16_t>& pcm) { (void)pcm; return {}; }   // TODO
inline std::vector<int16_t> mulaw_decode_buffer(const std::vector<uint8_t>& u) { (void)u; return {}; }      // TODO

// TODO: the continuous curve  F(x) = sign(x) * ln(1 + mu|x|) / ln(1 + mu)   and its inverse. |x| is clamped to 1.
inline double mu_compress(double x, double mu = 255.0) { (void)x; (void)mu; return 0.0; }
inline double mu_expand(double y, double mu = 255.0) { (void)y; (void)mu; return 0.0; }

// ---------------------------------------------------------------- codec table
struct CodecInfo {
    const char* name;
    int static_pt;              // -1 = dynamic
    uint32_t clock_hz;          // RTP timestamp clock
    uint32_t sample_rate_hz;    // real sampling rate (may differ from the clock)
    uint32_t channels;
    uint32_t bitrate_bps;       // nominal, 0 = variable
    uint32_t frame_us;          // frame duration in microseconds, 0 = sample-based or variable
    uint32_t frame_bits;        // bits per frame (nominal mode), 0 if not frame-based
};
// TODO: fill the table from worksheet Part C: PCMU, GSM, PCMA, G722, L16 (PT 10 stereo), CN, G729, AMR (12.2k, 244 bits),
// AMR-WB (23.85k, 477 bits), opus (clock 48000). Order does not matter. Note G722's clock != its sampling rate.
inline const std::vector<CodecInfo>& codec_table() {
    static const std::vector<CodecInfo> t = {};
    return t;
}
inline bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
inline const CodecInfo* find_codec(const std::string& name) { (void)name; return nullptr; }     // TODO: case-insensitive
inline const CodecInfo* find_codec_by_pt(int pt) { (void)pt; return nullptr; }                  // TODO: static PTs only (pt < 0 -> nullptr)
// TODO: whole frames per packet; 0 for sample-based/variable codecs or if ptime is not a multiple of the frame duration
inline uint32_t frames_per_packet(const CodecInfo& c, uint32_t ptime_ms) { (void)c; (void)ptime_ms; return 99; }
// TODO: RTP timestamp step = clock * ptime / 1000
inline uint32_t codec_ts_step(const CodecInfo& c, uint32_t ptime_ms) { (void)c; (void)ptime_ms; return 0; }
// TODO: frame-based: frames * ceil(frame_bits/8); sample-based: ceil(bitrate*ptime/8000); variable bitrate: 0
inline uint32_t codec_payload_bytes_nominal(const CodecInfo& c, uint32_t ptime_ms) { (void)c; (void)ptime_ms; return 99; }
// TODO: Opus frame durations 2.5, 5, 10, 20, 40, 60 ms (argument in tenths of a millisecond)
inline bool opus_valid_frame_tenth_ms(uint32_t tenth_ms) { (void)tenth_ms; return false; }

// ---------------------------------------------------------------- radio vocoder arithmetic (VERIFY each figure in the standard)
struct VocoderInfo {
    const char* name;
    uint32_t frame_us;
    uint32_t voice_bits;
    uint32_t fec_bits;
    uint32_t total_bits() const { return voice_bits + fec_bits; }
    // TODO: bits * 1e6 / frame_us rounded to the nearest bit/s
    uint32_t bitrate_bps() const { return 0; }
    uint32_t voice_bps() const { return 0; }                     // TODO: same for voice_bits only
};
// TODO: IMBE (P25 Phase 1): 20 ms, 88 voice + 56 FEC. AMBE+2 half-rate: 20 ms, 49 voice + 23 FEC. TETRA ACELP: 30 ms, 137 bits, no FEC counted.
inline VocoderInfo imbe_full_rate() { return {"IMBE", 1, 0, 0}; }
inline VocoderInfo ambe_plus2_half_rate() { return {"AMBE+2 half-rate", 1, 0, 0}; }
inline VocoderInfo tetra_acelp() { return {"TETRA ACELP", 1, 0, 0}; }

constexpr uint32_t kP25ChannelBps = 9600;
constexpr uint32_t kLduFrames = 9;
inline uint32_t ldu_duration_ms() { return 0; }          // TODO: 9 frames * 20 ms
inline uint32_t ldu_total_bits() { return 0; }           // TODO: channel rate * duration
inline uint32_t ldu_voice_bits() { return 0; }           // TODO: 9 * IMBE total bits
inline uint32_t ldu_overhead_bits() { return 99; }       // TODO: total - voice (sync, NID, link control, low-speed data, status symbols)
// TODO: ceil(total_bits * frames / 8)
inline uint32_t vocoder_payload_bytes(const VocoderInfo& v, uint32_t frames) { (void)v; (void)frames; return 0; }

}  // namespace ether
