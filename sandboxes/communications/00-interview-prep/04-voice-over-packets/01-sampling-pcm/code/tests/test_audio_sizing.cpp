// Tests for 04-audio/01-sampling-pcm. Same file builds against the student stubs and the solutions.
#include <gtest/gtest.h>
#include <cmath>
#include "audio_sizing.hpp"

using namespace ether;

TEST(Sizing, SamplesPerPacket) {
    EXPECT_EQ(samples_per_packet(8000, 20), 160u);
    EXPECT_EQ(samples_per_packet(8000, 10), 80u);
    EXPECT_EQ(samples_per_packet(16000, 20), 320u);
    EXPECT_EQ(samples_per_packet(48000, 20), 960u);
    EXPECT_EQ(samples_per_packet(44100, 20), 882u);
    EXPECT_EQ(samples_per_packet(48000, 2500), 120000u);          // no 32-bit overflow in the intermediate product
    EXPECT_EQ(samples_per_packet(0, 20), 0u);
    EXPECT_EQ(samples_per_packet(8000, 0), 0u);
}
TEST(Sizing, SamplesExact) {
    EXPECT_TRUE(samples_exact(8000, 20));
    EXPECT_TRUE(samples_exact(44100, 20));
    EXPECT_FALSE(samples_exact(44100, 1));                         // 44.1 samples
    EXPECT_TRUE(samples_exact(48000, 1));
    EXPECT_FALSE(samples_exact(11025, 10));                        // 110.25
}
TEST(Sizing, PayloadBytesFromPcm) {
    EXPECT_EQ(pcm_payload_bytes(160, 1, 8), 160u);                 // G.711: 8 bits per sample
    EXPECT_EQ(pcm_payload_bytes(160, 1, 16), 320u);                // L16 mono
    EXPECT_EQ(pcm_payload_bytes(960, 2, 16), 3840u);               // THE TRAP: 48 kHz stereo 16-bit 20 ms
    EXPECT_EQ(pcm_payload_bytes(5, 1, 12), 8u);                    // 60 bits -> 8 bytes (rounded up)
}
TEST(Sizing, PayloadBytesFromBitrate) {
    EXPECT_EQ(codec_payload_bytes(64000, 20), 160u);               // G.711
    EXPECT_EQ(codec_payload_bytes(8000, 20), 20u);                 // G.729: two 10-byte frames
    EXPECT_EQ(codec_payload_bytes(8000, 10), 10u);
    EXPECT_EQ(codec_payload_bytes(7200, 20), 18u);                 // IMBE 7.2 kb/s -> 144 bits = 18 bytes
    EXPECT_EQ(codec_payload_bytes(4567, 30), 18u);                 // 137.01 bits -> 18 bytes (rounded UP; TETRA ACELP frame is 137 bits)
    EXPECT_EQ(codec_payload_bytes(23850, 20), 60u);                // AMR-WB 23.85 kb/s: 477 bits -> 60 bytes
    EXPECT_EQ(codec_payload_bytes(3600, 20), 9u);                  // AMBE+2 half rate 72 bits
}
TEST(Sizing, G711WorkedExample) {
    uint32_t n = samples_per_packet(8000, 20);
    uint32_t payload = pcm_payload_bytes(n, 1, 8);
    EXPECT_EQ(n, 160u); EXPECT_EQ(payload, 160u);
    uint32_t ip = ip_packet_bytes(payload);
    EXPECT_EQ(ip, 200u);                                           // 20 + 8 + 12 + 160
    EXPECT_EQ(packets_per_second(20), 50u);
    EXPECT_DOUBLE_EQ(bandwidth_kbps(ip, 20), 80.0);                // IP-level
    EXPECT_EQ(eth_frame_bytes(ip), 218u);                          // + 14 header + 4 FCS
    EXPECT_DOUBLE_EQ(bandwidth_kbps(eth_frame_bytes(ip), 20), 87.2);
    EXPECT_EQ(eth_wire_bytes(ip), 238u);
    EXPECT_DOUBLE_EQ(bandwidth_kbps(eth_wire_bytes(ip), 20), 95.2);
    EXPECT_EQ(rtp_ts_increment(8000, 20), 160u);                   // timestamp advances by the sample count
    EXPECT_DOUBLE_EQ(bandwidth_kbps(payload, 20), 64.0);           // payload alone = the codec bit rate
}
TEST(Sizing, HeaderVariants) {
    EXPECT_EQ(ip_packet_bytes(160, true), 220u);                   // IPv6 header is 40
    EXPECT_EQ(eth_frame_bytes(200, true), 222u);                   // 802.1Q tag +4
    EXPECT_EQ(eth_frame_bytes(10), 64u);                           // padded to the minimum frame
    EXPECT_EQ(eth_wire_bytes(10), 84u);
    EXPECT_EQ(packets_per_second(0), 0u);
    EXPECT_EQ(packets_per_second(30), 33u);
    EXPECT_DOUBLE_EQ(bandwidth_kbps(100, 0), 0.0);
}
TEST(Sizing, SmallerPacketsCostMoreOverhead) {
    // Same 64 kbit/s of audio, different ptime: header overhead is 40 B per packet.
    EXPECT_DOUBLE_EQ(bandwidth_kbps(ip_packet_bytes(codec_payload_bytes(64000, 10)), 10), 96.0);
    EXPECT_DOUBLE_EQ(bandwidth_kbps(ip_packet_bytes(codec_payload_bytes(64000, 20)), 20), 80.0);
    EXPECT_DOUBLE_EQ(bandwidth_kbps(ip_packet_bytes(codec_payload_bytes(64000, 40)), 40), 72.0);
    // G.729: 8 kbit/s payload at 20 ms -> 20 B payload, 60 B packet = 24 kbit/s (3x the codec rate!)
    EXPECT_DOUBLE_EQ(bandwidth_kbps(ip_packet_bytes(codec_payload_bytes(8000, 20)), 20), 24.0);
}
TEST(Mtu, The48kStereoTrap) {
    uint32_t ip = ip_packet_bytes(pcm_payload_bytes(samples_per_packet(48000, 20), 2, 16));
    EXPECT_EQ(ip, 3880u);
    EXPECT_FALSE(fits_mtu(ip));
    EXPECT_TRUE(fits_mtu(200));
    EXPECT_TRUE(fits_mtu(1500)); EXPECT_FALSE(fits_mtu(1501));
    EXPECT_EQ(max_ptime_ms_for_mtu(48000, 2, 16), 7u);             // 7 ms * 192 B/ms = 1344 + 40 <= 1500; 8 ms = 1536 + 40 > 1500
    EXPECT_EQ(max_ptime_ms_for_mtu(48000, 1, 16), 15u);            // 96 B/ms: 15 ms = 1440 + 40 = 1480
    EXPECT_EQ(max_ptime_ms_for_mtu(8000, 1, 8), 182u);             // narrowband G.711: 8 B/ms; 182 ms = 1456 + 40 <= 1500 (183 ms = 1464 + 40 = 1504)
    EXPECT_EQ(max_ptime_ms_for_mtu(48000, 2, 16, 1500, true), 7u); // IPv6: 1344 + 60 still fits; 8 ms does not
    EXPECT_EQ(max_ptime_ms_for_mtu(48000, 2, 16, 100), 0u);        // not even 1 ms (192 B) fits in 100
    EXPECT_EQ(max_ptime_ms_for_mtu(44100, 2, 16), 0u);             // 44.1 kHz: only multiples of 20 ms are whole samples; 20 ms = 3528 B > MTU
}
TEST(Link, SerializationDelay) {
    EXPECT_NEAR(serialization_delay_us(200, 1e6), 1600.0, 1e-6);   // 200 B on 1 Mbit/s = 1.6 ms
    EXPECT_NEAR(serialization_delay_us(1500, 64000), 187500.0, 1e-3);   // MTU packet on a 64 kbit/s link = 187.5 ms (why voice needs small packets/LFI there)
    EXPECT_NEAR(serialization_delay_us(238, 1e9), 1.904, 1e-9);
}
TEST(Sampling, NyquistAndAliasing) {
    EXPECT_DOUBLE_EQ(nyquist_hz(8000), 4000.0);
    EXPECT_DOUBLE_EQ(nyquist_hz(48000), 24000.0);
    EXPECT_NEAR(alias_hz(1000, 8000), 1000.0, 1e-9);               // below Nyquist: unchanged
    EXPECT_NEAR(alias_hz(4000, 8000), 4000.0, 1e-9);               // exactly Nyquist
    EXPECT_NEAR(alias_hz(5000, 8000), 3000.0, 1e-9);               // folds down
    EXPECT_NEAR(alias_hz(7000, 8000), 1000.0, 1e-9);
    EXPECT_NEAR(alias_hz(8000, 8000), 0.0, 1e-9);                  // a tone at fs looks like DC
    EXPECT_NEAR(alias_hz(9000, 8000), 1000.0, 1e-9);
    EXPECT_NEAR(alias_hz(12500, 8000), 3500.0, 1e-9);              // 12.5k mod 8k = 4.5k -> folds to 3.5k
    EXPECT_NEAR(alias_hz(-1000, 8000), 1000.0, 1e-9);
}
TEST(Sampling, AliasedToneIsTheSameSamples) {
    // A 7 kHz tone sampled at 8 kHz produces the same samples as a 1 kHz tone of opposite sign (sin(2*pi*7k*n/8k) = -sin(2*pi*1k*n/8k)).
    auto hi = sine_s16(8000, 7000, 64, 0.5);
    auto lo = sine_s16(8000, 1000, 64, 0.5);
    for (size_t i = 0; i < hi.size(); ++i) EXPECT_NEAR(hi[i], -lo[i], 1.0) << i;
}
TEST(Pcm, FloatToS16ClipsAndRounds) {
    EXPECT_EQ(float_to_s16(0.0), 0);
    EXPECT_EQ(float_to_s16(1.0), 32767);                           // +1.0 is just beyond full scale: clip, don't wrap to -32768
    EXPECT_EQ(float_to_s16(-1.0), -32768);
    EXPECT_EQ(float_to_s16(2.5), 32767);
    EXPECT_EQ(float_to_s16(-9.0), -32768);
    EXPECT_EQ(float_to_s16(0.5), 16384);
    EXPECT_EQ(float_to_s16(-0.5), -16384);
    EXPECT_EQ(float_to_s16(1.0 / 65536.0), 1);                     // 0.5 LSB rounds to nearest (up)
    EXPECT_NEAR(s16_to_float(-32768), -1.0, 1e-12);
    EXPECT_NEAR(s16_to_float(16384), 0.5, 1e-12);
}
TEST(Pcm, SineShape) {
    auto s = sine_s16(8000, 1000, 16, 0.5);                        // 8 samples per cycle
    ASSERT_EQ(s.size(), 16u);
    EXPECT_EQ(s[0], 0);
    EXPECT_EQ(s[2], 16384);                                        // quarter cycle = peak = 0.5 * 32768
    EXPECT_EQ(s[4], 0);   // sin(pi) ~ 1e-16 rounds to 0
    EXPECT_EQ(s[6], -16384);
    EXPECT_EQ(s[8], s[0]); EXPECT_EQ(s[10], s[2]);                 // periodic
    auto c = sine_s16(8000, 1000, 4, 1.0, 1.5707963267948966);     // phase pi/2 -> cosine, clipped at +full scale
    EXPECT_EQ(c[0], 32767);
    EXPECT_TRUE(sine_s16(8000, 1000, 0).empty());
}
TEST(Pcm, QuantiseBits) {
    EXPECT_EQ(quantise_bits(12345, 16), 12345);
    EXPECT_EQ(quantise_bits(0, 8), 0);
    EXPECT_EQ(quantise_bits(255, 8), 256);                         // 255/256 rounds to 1 step
    EXPECT_EQ(quantise_bits(127, 8), 0);                           // below half a step
    EXPECT_EQ(quantise_bits(128, 8), 256);                         // exactly half a step rounds up
    EXPECT_EQ(quantise_bits(-129, 8), -256);
    EXPECT_EQ(quantise_bits(32767, 8), 32512);                     // must not overflow int16
    EXPECT_EQ(quantise_bits(-32768, 8), -32768);
    for (int x = -32768; x <= 32767; x += 997) {
        int q = quantise_bits(static_cast<int16_t>(x), 12);
        EXPECT_LE(std::abs(q - x), 8) << x;                        // step 16 -> error <= 8
        EXPECT_EQ(q % 16, 0) << x;
    }
}
TEST(Pcm, EmpiricalSnrFollowsSixDbPerBit) {
    // Derivation check: each extra bit halves the step -> error power /4 -> +6.02 dB.
    auto sig = sine_s16(48000, 1013.0, 48000, 0.999);
    for (unsigned bits : {6u, 8u, 10u, 12u, 14u}) {
        std::vector<int16_t> q(sig.size());
        for (size_t i = 0; i < sig.size(); ++i) q[i] = quantise_bits(sig[i], bits);
        double got = snr_db(sig, q);
        EXPECT_NEAR(got, ideal_snr_db(bits), 2.0) << bits << " bits: " << got;
    }
    EXPECT_NEAR(ideal_snr_db(8), 49.92, 1e-9);
    EXPECT_NEAR(ideal_snr_db(16), 98.08, 1e-9);
    EXPECT_NEAR(ideal_snr_db(12), 74.0, 1e-9);
    EXPECT_DOUBLE_EQ(snr_db(sig, sig), 200.0);
    EXPECT_DOUBLE_EQ(snr_db({0, 0}, {1, 1}), 200.0);               // silent reference: defined as 200
}
