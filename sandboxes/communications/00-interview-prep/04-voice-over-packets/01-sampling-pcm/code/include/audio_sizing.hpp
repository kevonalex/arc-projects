// STUB - 04-audio/01-sampling-pcm. Implement every TODO. Tests: tests/test_audio_sizing.cpp
// Pure C++17, header-only. Synthetic signals only.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ether {

constexpr uint32_t kRtpHdr = 12, kUdpHdr = 8, kIpv4Hdr = 20, kIpv6Hdr = 40;
constexpr uint32_t kEthHdr = 14, kEthFcs = 4, kVlanTag = 4, kEthPreamble = 8, kEthIfg = 12, kEthMinFrame = 64;

// TODO: samples in one packet = fs * t. fs in Hz, ptime in milliseconds (8000 Hz * 20 ms = 160). Use 64-bit intermediates.
inline uint32_t samples_per_packet(uint32_t fs_hz, uint32_t ptime_ms) { (void)fs_hz; (void)ptime_ms; return 0; }
// TODO: true if fs*ptime is a whole number of samples (44100 Hz * 20 ms = 882: yes; 44100 * 1 ms = 44.1: no)
inline bool samples_exact(uint32_t fs_hz, uint32_t ptime_ms) { (void)fs_hz; (void)ptime_ms; return false; }

// TODO: raw PCM payload bytes = samples * channels * bits / 8, rounded up to whole bytes
inline uint32_t pcm_payload_bytes(uint32_t samples, uint32_t channels, uint32_t bits_per_sample) { (void)samples; (void)channels; (void)bits_per_sample; return 0; }
// TODO: coded payload bytes = bitrate * t / 8 = bitrate_bps * ptime_ms / 8000, rounded UP
inline uint32_t codec_payload_bytes(uint32_t bitrate_bps, uint32_t ptime_ms) { (void)bitrate_bps; (void)ptime_ms; return 0; }

// TODO: IP packet = IP header (20 v4 / 40 v6) + UDP 8 + RTP 12 + payload
inline uint32_t ip_packet_bytes(uint32_t payload, bool ipv6 = false) { (void)payload; (void)ipv6; return 0; }
// TODO: Ethernet frame = 14 (+4 per VLAN tag) + IP packet + 4 FCS, never below the 64-byte minimum
inline uint32_t eth_frame_bytes(uint32_t ip_bytes, bool vlan = false) { (void)ip_bytes; (void)vlan; return 0; }
// TODO: on-the-wire bytes per frame = frame + preamble/SFD 8 + inter-frame gap 12
inline uint32_t eth_wire_bytes(uint32_t ip_bytes, bool vlan = false) { (void)ip_bytes; (void)vlan; return 0; }
// TODO: kbit/s of 'bytes_per_packet' sent every 'ptime_ms'. Hint: bits per millisecond IS kilobits per second.
inline double bandwidth_kbps(uint32_t bytes_per_packet, uint32_t ptime_ms) { (void)bytes_per_packet; (void)ptime_ms; return 0.0; }
// TODO: 1000 / ptime (0 if ptime is 0)
inline uint32_t packets_per_second(uint32_t ptime_ms) { (void)ptime_ms; return 0; }

inline bool fits_mtu(uint32_t ip_bytes, uint32_t mtu = 1500) { return ip_bytes <= mtu; }
// TODO: largest whole-millisecond ptime (1..200) whose RAW PCM packet (+RTP/UDP/IP headers) fits the MTU and holds a whole number of
// samples. 0 if even 1 ms does not fit.
inline uint32_t max_ptime_ms_for_mtu(uint32_t fs_hz, uint32_t channels, uint32_t bits, uint32_t mtu = 1500, bool ipv6 = false) {
    (void)fs_hz; (void)channels; (void)bits; (void)mtu; (void)ipv6; return 99999;
}

// TODO: RTP timestamp increment per packet = clock * ptime / 1000 (counts SAMPLES, not milliseconds)
inline uint32_t rtp_ts_increment(uint32_t clock_hz, uint32_t ptime_ms) { (void)clock_hz; (void)ptime_ms; return 0; }

// TODO: fs / 2
inline double nyquist_hz(double fs_hz) { (void)fs_hz; return 0.0; }
// TODO: the frequency a pure tone appears at after sampling at fs: fold f around multiples of fs/2. (5 kHz at 8 kHz -> 3 kHz)
inline double alias_hz(double f_hz, double fs_hz) { (void)f_hz; (void)fs_hz; return -1.0; }
// TODO: ideal SNR of a full-scale sine quantised to N bits: 6.02 N + 1.76 dB
inline double ideal_snr_db(unsigned bits) { (void)bits; return 0.0; }
// TODO: microseconds to clock 'bytes' onto a link of link_bps
inline double serialization_delay_us(uint32_t bytes, double link_bps) { (void)bytes; (void)link_bps; return 0.0; }

// TODO: x in [-1,1] -> int16: round(x*32768), CLIP to [-32768, 32767] (never wrap)
inline int16_t float_to_s16(double x) { (void)x; return 0; }
inline double s16_to_float(int16_t s) { return s / 32768.0; }
// TODO: n samples of amplitude*sin(2*pi*f*i/fs + phase) via float_to_s16
inline std::vector<int16_t> sine_s16(uint32_t fs_hz, double f_hz, size_t n, double amplitude = 0.5, double phase = 0.0) {
    (void)fs_hz; (void)f_hz; (void)amplitude; (void)phase; return std::vector<int16_t>(n, 0);
}
// TODO: keep only the top 'bits' bits: round to the nearest multiple of 2^(16-bits); stay within int16 range
inline int16_t quantise_bits(int16_t x, unsigned bits) { (void)x; (void)bits; return 0; }
// TODO: 10*log10(sum(ref^2) / sum((ref-test)^2)); 200 dB if the error (or the signal) is zero
inline double snr_db(const std::vector<int16_t>& ref, const std::vector<int16_t>& test) { (void)ref; (void)test; return -999.0; }

}  // namespace ether
