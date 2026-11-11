// SOLUTION - 04-audio/01-sampling-pcm. Pure C++17, header-only. Tests: tests/test_audio_sizing.cpp
// Sizing arithmetic every voice-over-IP engineer must do without a calculator, plus PCM helpers for synthetic signals.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ether {

constexpr uint32_t kRtpHdr = 12, kUdpHdr = 8, kIpv4Hdr = 20, kIpv6Hdr = 40;
constexpr uint32_t kEthHdr = 14, kEthFcs = 4, kVlanTag = 4, kEthPreamble = 8, kEthIfg = 12, kEthMinFrame = 64;

// ---- how many samples fit in one packet ---------------------------------------------------------
// samples = fs * t.  fs in Hz, ptime in milliseconds  ->  fs * ptime / 1000   (8000 Hz * 20 ms = 160)
inline uint32_t samples_per_packet(uint32_t fs_hz, uint32_t ptime_ms) {
    return static_cast<uint32_t>(uint64_t{fs_hz} * ptime_ms / 1000);
}
// true if fs*ptime is a whole number of samples (44100 Hz * 20 ms = 882 ok, 44100 * 1 ms = 44.1 is not)
inline bool samples_exact(uint32_t fs_hz, uint32_t ptime_ms) { return (uint64_t{fs_hz} * ptime_ms) % 1000 == 0; }

// ---- payload size ---------------------------------------------------------------------------------
// Raw PCM: samples * channels * bits / 8 (round up to whole bytes)
inline uint32_t pcm_payload_bytes(uint32_t samples, uint32_t channels, uint32_t bits_per_sample) {
    return static_cast<uint32_t>((uint64_t{samples} * channels * bits_per_sample + 7) / 8);
}
// Coded audio: bytes = bitrate * t / 8   ->  bitrate_bps * ptime_ms / 8000, rounded UP (a codec frame is never fractional)
inline uint32_t codec_payload_bytes(uint32_t bitrate_bps, uint32_t ptime_ms) {
    return static_cast<uint32_t>((uint64_t{bitrate_bps} * ptime_ms + 7999) / 8000);
}

// ---- headers and bandwidth --------------------------------------------------------------------------
// IP packet = IP header (20 v4 / 40 v6) + UDP 8 + RTP 12 + payload
inline uint32_t ip_packet_bytes(uint32_t payload, bool ipv6 = false) { return (ipv6 ? kIpv6Hdr : kIpv4Hdr) + kUdpHdr + kRtpHdr + payload; }
// Ethernet frame = 14 header (+4 per VLAN tag) + IP packet + 4 FCS, padded to the 64 B minimum
inline uint32_t eth_frame_bytes(uint32_t ip_bytes, bool vlan = false) {
    return std::max<uint32_t>(kEthMinFrame, kEthHdr + (vlan ? kVlanTag : 0) + ip_bytes + kEthFcs);
}
// What the cable actually carries per frame: + preamble/SFD 8 + inter-frame gap 12
inline uint32_t eth_wire_bytes(uint32_t ip_bytes, bool vlan = false) { return eth_frame_bytes(ip_bytes, vlan) + kEthPreamble + kEthIfg; }
// bytes sent every ptime_ms  ->  kbit/s.   (bytes*8 bits) / (ptime ms) = kbit/s exactly - bits per millisecond IS kilobits per second.
inline double bandwidth_kbps(uint32_t bytes_per_packet, uint32_t ptime_ms) { return ptime_ms ? bytes_per_packet * 8.0 / ptime_ms : 0.0; }
inline uint32_t packets_per_second(uint32_t ptime_ms) { return ptime_ms ? 1000 / ptime_ms : 0; }

// ---- MTU --------------------------------------------------------------------------------------------
inline bool fits_mtu(uint32_t ip_bytes, uint32_t mtu = 1500) { return ip_bytes <= mtu; }
// Largest whole-millisecond ptime whose RAW PCM packet (with RTP/UDP/IP headers) still fits the MTU and holds a whole number of samples.
// 0 if even 1 ms does not fit.
inline uint32_t max_ptime_ms_for_mtu(uint32_t fs_hz, uint32_t channels, uint32_t bits, uint32_t mtu = 1500, bool ipv6 = false) {
    uint32_t best = 0;
    for (uint32_t p = 1; p <= 200; ++p) {
        if (!samples_exact(fs_hz, p)) continue;
        uint32_t ip = ip_packet_bytes(pcm_payload_bytes(samples_per_packet(fs_hz, p), channels, bits), ipv6);
        if (ip <= mtu) best = p;
    }
    return best;
}

// ---- RTP clock --------------------------------------------------------------------------------------
// Timestamp counts SAMPLES of the RTP clock, not milliseconds: increment per packet = clock_hz * ptime / 1000.
inline uint32_t rtp_ts_increment(uint32_t clock_hz, uint32_t ptime_ms) { return samples_per_packet(clock_hz, ptime_ms); }

// ---- sampling theory ---------------------------------------------------------------------------------
inline double nyquist_hz(double fs_hz) { return fs_hz / 2.0; }
// Frequency a pure tone appears at after sampling at fs (aliasing = folding around multiples of fs/2): 5 kHz at 8 kHz -> 3 kHz
inline double alias_hz(double f_hz, double fs_hz) {
    double r = std::fmod(std::fabs(f_hz), fs_hz);
    return r > fs_hz / 2.0 ? fs_hz - r : r;
}
// Ideal SNR of a full-scale sine quantised to N bits: 6.02 N + 1.76 dB
inline double ideal_snr_db(unsigned bits) { return 6.02 * bits + 1.76; }
// Time to clock 'bytes' onto a link of 'link_bps' (microseconds)
inline double serialization_delay_us(uint32_t bytes, double link_bps) { return bytes * 8.0 * 1e6 / link_bps; }

// ---- PCM helpers (synthetic signals only) -------------------------------------------------------------
inline int16_t float_to_s16(double x) {                       // x in [-1,1]; rounds to nearest and CLIPS instead of wrapping
    double v = std::floor(x * 32768.0 + 0.5);
    if (v > 32767.0) v = 32767.0;
    if (v < -32768.0) v = -32768.0;
    return static_cast<int16_t>(v);
}
inline double s16_to_float(int16_t s) { return s / 32768.0; }
inline std::vector<int16_t> sine_s16(uint32_t fs_hz, double f_hz, size_t n, double amplitude = 0.5, double phase = 0.0) {
    std::vector<int16_t> out(n);
    const double two_pi = 6.283185307179586476925286766559;
    for (size_t i = 0; i < n; ++i) out[i] = float_to_s16(amplitude * std::sin(two_pi * f_hz * static_cast<double>(i) / fs_hz + phase));
    return out;
}
// Keep only the top 'bits' bits of a 16-bit sample (round to nearest multiple of 2^(16-bits)), result still scaled as 16-bit.
inline int16_t quantise_bits(int16_t x, unsigned bits) {
    if (bits >= 16) return x;
    if (bits < 1) bits = 1;
    const double step = static_cast<double>(1u << (16 - bits));
    double q = std::floor(x / step + 0.5) * step;
    if (q > 32767.0) q -= step;                               // rounded up past full scale: step back one level
    if (q < -32768.0) q = -32768.0;
    return static_cast<int16_t>(q);
}
// SNR in dB of 'test' against 'ref' (signal power / error power); 200 dB if identical
inline double snr_db(const std::vector<int16_t>& ref, const std::vector<int16_t>& test) {
    double sig = 0, err = 0;
    size_t n = std::min(ref.size(), test.size());
    for (size_t i = 0; i < n; ++i) { double r = ref[i], e = r - test[i]; sig += r * r; err += e * e; }
    if (err == 0.0 || sig == 0.0) return 200.0;
    return 10.0 * std::log10(sig / err);
}

}  // namespace ether
