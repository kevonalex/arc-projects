// Tests for 04-audio/02-codecs. Same file builds against the student stubs and the solutions.
#include <gtest/gtest.h>
#include <cmath>
#include <cstdlib>
#include <set>
#include <vector>
#include "codecs.hpp"

using namespace ether;

// find_codec() returns nullptr for an unknown name (e.g. from the student stub). Dereferencing it would be undefined behaviour, so tests go
// through this helper: a missing codec becomes an all-zero entry and the assertions simply fail.
static const CodecInfo& codec_or_dummy(const char* name) {
    static const CodecInfo dummy{};
    const CodecInfo* c = find_codec(name);
    return c ? *c : dummy;
}

static std::vector<int16_t> sine(double amp, size_t n = 8000, double f = 1013.0, double fs = 48000.0) {
    std::vector<int16_t> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = static_cast<int16_t>(std::lround(amp * 32767.0 * std::sin(6.283185307179586 * f * i / fs)));
    return v;
}
static double snr(const std::vector<int16_t>& ref, const std::vector<int16_t>& test) {
    double s = 0, e = 0;
    for (size_t i = 0; i < ref.size(); ++i) { double r = ref[i], d = r - test[i]; s += r * r; e += d * d; }
    return e == 0 ? 200.0 : 10.0 * std::log10(s / e);
}

// ============================================================ mu-law
TEST(Mulaw, KnownCodes) {
    EXPECT_EQ(mulaw_encode(0), 0xFF);                       // silence = all ones after the inversion
    EXPECT_EQ(mulaw_encode(32767), 0x80);                   // loudest positive
    EXPECT_EQ(mulaw_encode(-32768), 0x00);                  // loudest negative (clipped, no int16 overflow)
    EXPECT_EQ(mulaw_encode(-1), 0x7F);                      // "negative zero"
    EXPECT_EQ(mulaw_decode(0xFF), 0);
    EXPECT_EQ(mulaw_decode(0x7F), 0);                       // 0xFF and 0x7F both mean 0: only 255 distinct levels
    EXPECT_EQ(mulaw_decode(0x80), 32124);                   // largest decoded magnitude
    EXPECT_EQ(mulaw_decode(0x00), -32124);
    EXPECT_EQ(mulaw_decode(0xFE), 8);                       // one step above zero = 8 (segment 0 step)
    EXPECT_EQ(mulaw_decode(0x7E), -8);
    EXPECT_EQ(mulaw_decode(0xF0), 120);                     // top of segment 0 : 15 steps of 8
    EXPECT_EQ(mulaw_decode(0xEF), 132);                     // segment 1 starts at 132 (the bias!) and steps by 16
}
TEST(Mulaw, SegmentsAndStepSizeDouble) {
    EXPECT_EQ(mulaw_segment(0xFF), 0u); EXPECT_EQ(mulaw_step(0xFF), 8);
    EXPECT_EQ(mulaw_segment(0xF0), 0u);
    EXPECT_EQ(mulaw_segment(0xEF), 1u); EXPECT_EQ(mulaw_step(0xEF), 16);
    EXPECT_EQ(mulaw_segment(0xDF), 2u); EXPECT_EQ(mulaw_step(0xDF), 32);
    EXPECT_EQ(mulaw_segment(0xCF), 3u); EXPECT_EQ(mulaw_step(0xCF), 64);
    EXPECT_EQ(mulaw_segment(0x80), 7u); EXPECT_EQ(mulaw_step(0x80), 1024);
    EXPECT_EQ(mulaw_segment(0x00), 7u);                     // sign bit does not matter
    int prev = 0;
    for (int c = 0xFF; c >= 0x80; --c) {                    // walking up the positive codes, steps never shrink
        EXPECT_GE(mulaw_step(static_cast<uint8_t>(c)), prev);
        prev = mulaw_step(static_cast<uint8_t>(c));
    }
}
TEST(Mulaw, ExhaustiveCodeRoundTrip) {
    std::set<int> levels;
    for (int c = 0; c < 256; ++c) {
        uint8_t code = static_cast<uint8_t>(c);
        int16_t lin = mulaw_decode(code);
        levels.insert(lin);
        uint8_t back = mulaw_encode(lin);
        if (code == 0x7F) EXPECT_EQ(back, 0xFF);            // the only code that does not survive: -0 -> +0
        else EXPECT_EQ(back, code) << "code " << c << " decoded to " << lin;
    }
    EXPECT_EQ(levels.size(), 255u);                         // 256 codes, one duplicate level (zero)
}
TEST(Mulaw, ExhaustiveInputQuantisationError) {
    int worst = 0; int16_t prev = -32768;
    for (int x = -32768; x <= 32767; ++x) {
        uint8_t c = mulaw_encode(static_cast<int16_t>(x));
        int16_t y = mulaw_decode(c);
        EXPECT_GE(y, prev) << "decoder output must be monotonic at x=" << x;      // companding never reverses order
        prev = y;
        int mag = std::abs(x);
        if (mag <= kMuClip) {
            int err = std::abs(x - y);
            EXPECT_LE(err, mulaw_step(c) / 2) << "x=" << x << " code=" << int(c);  // within half a quantiser step
            worst = std::max(worst, err);
        } else {
            EXPECT_EQ(std::abs(y), 32124) << "clipped input x=" << x;
        }
    }
    EXPECT_EQ(worst, 512);                                  // largest error is half of the top step (1024)
}
TEST(Mulaw, SignSymmetry) {
    for (int x = 1; x <= 32767; ++x)
        EXPECT_EQ(mulaw_encode(static_cast<int16_t>(x)) ^ mulaw_encode(static_cast<int16_t>(-x)), 0x80) << x;
}
TEST(Mulaw, RelativeErrorBounded) {
    // The point of companding: error grows with the signal, so RELATIVE error stays small over a 48 dB range.
    for (int x = 500; x <= 32000; x += 7) {
        double y = mulaw_decode(mulaw_encode(static_cast<int16_t>(x)));
        EXPECT_LT(std::fabs(x - y) / x, 0.04) << x;
    }
}
TEST(Mulaw, CompandingBeatsLinearAtLowLevels) {
    auto lin8 = [](int16_t s) { return static_cast<int16_t>(std::lround(s / 256.0) * 256); };   // 8-bit linear for comparison
    double mu_loud = 0, mu_quiet = 0, lin_loud = 0, lin_quiet = 0;
    for (double amp : {0.5, 0.005}) {
        auto x = sine(amp);
        std::vector<int16_t> a(x.size()), b(x.size());
        for (size_t i = 0; i < x.size(); ++i) { a[i] = mulaw_decode(mulaw_encode(x[i])); b[i] = lin8(x[i]); }
        (amp > 0.1 ? mu_loud : mu_quiet) = snr(x, a);
        (amp > 0.1 ? lin_loud : lin_quiet) = snr(x, b);
    }
    EXPECT_GT(mu_loud, 30.0); EXPECT_GT(mu_quiet, 20.0);                // mu-law keeps ~25-38 dB over a 40 dB range
    EXPECT_GT(lin_loud, 40.0); EXPECT_LT(lin_quiet, 10.0);              // 8-bit linear collapses at -46 dBFS
    EXPECT_GT(mu_quiet - lin_quiet, 15.0);
    EXPECT_LT(mu_loud - mu_quiet, 20.0);                                // roughly flat compared with linear's 40 dB fall
}
TEST(Mulaw, BufferHelpers) {
    std::vector<int16_t> pcm{0, 1000, -1000, 32767, -32768};
    auto enc = mulaw_encode_buffer(pcm);
    ASSERT_EQ(enc.size(), 5u);
    EXPECT_EQ(enc[0], 0xFF);
    auto dec = mulaw_decode_buffer(enc);
    ASSERT_EQ(dec.size(), 5u);
    EXPECT_EQ(dec[0], 0); EXPECT_EQ(dec[3], 32124); EXPECT_EQ(dec[4], -32124);
    EXPECT_NEAR(dec[1], 1000, 32);
    EXPECT_TRUE(mulaw_encode_buffer({}).empty());
}
TEST(Mulaw, ContinuousCurveMatchesStandard) {
    EXPECT_NEAR(mu_compress(0.0), 0.0, 1e-12);
    EXPECT_NEAR(mu_compress(1.0), 1.0, 1e-12);
    EXPECT_NEAR(mu_compress(-1.0), -1.0, 1e-12);
    EXPECT_NEAR(mu_compress(0.01), 0.2284, 1e-3);            // 1% of full scale already uses 23% of the code range
    EXPECT_NEAR(mu_compress(0.1), 0.5912, 1e-3);
    EXPECT_NEAR(mu_expand(mu_compress(0.3)), 0.3, 1e-12);
    EXPECT_NEAR(mu_expand(mu_compress(-0.003)), -0.003, 1e-12);
    double prev = -1;
    for (double x = -1; x <= 1; x += 0.001) { double y = mu_compress(x); EXPECT_GE(y, prev); prev = y; }
    // The 8-segment standard follows the curve: wire magnitude (segment*16+step, 0..127) ~ F(|x|) * 127
    for (int x = 100; x <= 32000; x += 97) {
        uint8_t c = mulaw_encode(static_cast<int16_t>(x));
        int mag = (~c) & 0x7F;
        double ideal = mu_compress(x / 32768.0) * 127.0;
        EXPECT_NEAR(mag, ideal, 6.0) << x;
    }
}

// ============================================================ A-law
TEST(Alaw, KnownCodes) {
    EXPECT_EQ(alaw_encode(0), 0xD5);                         // A-law silence byte
    EXPECT_EQ(alaw_encode(32767), 0xAA);
    EXPECT_EQ(alaw_encode(-32768), 0x2A);
    EXPECT_EQ(alaw_decode(0xD5), 8);                         // there is no exact zero in A-law: smallest levels are +-8
    EXPECT_EQ(alaw_decode(0x55), -8);
    EXPECT_EQ(alaw_decode(0xAA), 32256);                     // largest magnitude
    EXPECT_EQ(alaw_decode(0x2A), -32256);
}
TEST(Alaw, ExhaustiveCodeRoundTrip) {
    std::set<int> levels;
    for (int c = 0; c < 256; ++c) {
        uint8_t code = static_cast<uint8_t>(c);
        int16_t lin = alaw_decode(code);
        levels.insert(lin);
        EXPECT_EQ(alaw_encode(lin), code) << "code " << c << " decoded to " << lin;
    }
    EXPECT_EQ(levels.size(), 256u);                          // all 256 codes are distinct levels
}
TEST(Alaw, ExhaustiveInputQuantisationError) {
    int16_t prev = -32768;
    for (int x = -32768; x <= 32767; ++x) {
        uint8_t c = alaw_encode(static_cast<int16_t>(x));
        int16_t y = alaw_decode(c);
        EXPECT_GE(y, prev) << x; prev = y;
        unsigned seg = ((c ^ 0x55) >> 4) & 7;
        int step = 16 << (seg < 1 ? 0 : seg - 1);            // 16-bit units
        EXPECT_LE(std::abs(x - y), step) << "x=" << x << " code=" << int(c);
    }
}
TEST(Alaw, SignSymmetryOfLevels) {
    for (int c = 0; c < 128; ++c) EXPECT_EQ(alaw_decode(static_cast<uint8_t>(c)), -alaw_decode(static_cast<uint8_t>(c | 0x80))) << c;
}

// ============================================================ PCMU <-> PCMA interworking
TEST(Interworking, Transcode) {
    EXPECT_EQ(mulaw_to_alaw(0xFF), 0xD5);                    // silence maps to silence
    for (int c = 0; c < 256; ++c) {
        uint8_t u = static_cast<uint8_t>(c);
        int a = alaw_decode(mulaw_to_alaw(u)), m = mulaw_decode(u);
        EXPECT_LE(std::abs(a - m), std::max(mulaw_step(u), 512) ) << "mu code " << c;   // within about one quantiser step
        uint8_t a2 = static_cast<uint8_t>(c);
        int m2 = mulaw_decode(alaw_to_mulaw(a2)), a1 = alaw_decode(a2);
        EXPECT_LE(std::abs(m2 - a1), 1024) << "a code " << c;
    }
    // Round trip through the other law is NOT lossless for every code (quantisers differ) - each hop adds error.
    int differ = 0;
    for (int c = 0; c < 256; ++c) if (alaw_to_mulaw(mulaw_to_alaw(static_cast<uint8_t>(c))) != c) ++differ;
    EXPECT_GT(differ, 0);
}

// ============================================================ codec table
TEST(CodecTable, StaticPayloadTypes) {
    ASSERT_NE(find_codec_by_pt(0), nullptr);  EXPECT_STREQ(find_codec_by_pt(0)->name, "PCMU");
    ASSERT_NE(find_codec_by_pt(8), nullptr);  EXPECT_STREQ(find_codec_by_pt(8)->name, "PCMA");
    ASSERT_NE(find_codec_by_pt(9), nullptr);  EXPECT_STREQ(find_codec_by_pt(9)->name, "G722");
    ASSERT_NE(find_codec_by_pt(18), nullptr); EXPECT_STREQ(find_codec_by_pt(18)->name, "G729");
    ASSERT_NE(find_codec_by_pt(13), nullptr); EXPECT_STREQ(find_codec_by_pt(13)->name, "CN");
    ASSERT_NE(find_codec_by_pt(3), nullptr);  EXPECT_STREQ(find_codec_by_pt(3)->name, "GSM");
    EXPECT_EQ(find_codec_by_pt(99), nullptr);
    EXPECT_EQ(find_codec_by_pt(-1), nullptr);                // dynamic payload types cannot be looked up by number
    EXPECT_EQ(find_codec_by_pt(111), nullptr);
}
TEST(CodecTable, FindByNameCaseInsensitive) {
    ASSERT_NE(find_codec("opus"), nullptr);
    EXPECT_EQ(find_codec("OPUS"), find_codec("opus"));
    ASSERT_NE(find_codec("pcmu"), nullptr); EXPECT_EQ(find_codec("pcmu")->static_pt, 0);
    ASSERT_NE(find_codec("AMR-WB"), nullptr); EXPECT_EQ(find_codec("amr-wb")->static_pt, -1);
    EXPECT_EQ(find_codec("speex"), nullptr);
    EXPECT_EQ(find_codec(""), nullptr);
}
TEST(CodecTable, ClockVersusSampleRate) {
    auto* g722 = find_codec("G722"); ASSERT_NE(g722, nullptr);
    EXPECT_EQ(g722->clock_hz, 8000u); EXPECT_EQ(g722->sample_rate_hz, 16000u);       // the famous historical mismatch
    auto* opus = find_codec("opus"); ASSERT_NE(opus, nullptr);
    EXPECT_EQ(opus->clock_hz, 48000u);                                               // always, whatever the audio bandwidth
    EXPECT_EQ(opus->channels, 2u);                                                   // rtpmap says opus/48000/2 even for mono
    auto* amrwb = find_codec("AMR-WB"); ASSERT_NE(amrwb, nullptr);
    EXPECT_EQ(amrwb->clock_hz, 16000u);
    auto* pcmu = find_codec("PCMU"); ASSERT_NE(pcmu, nullptr);
    EXPECT_EQ(pcmu->clock_hz, pcmu->sample_rate_hz);
    EXPECT_EQ(pcmu->bitrate_bps, 64000u);
    EXPECT_EQ(find_codec("G729")->bitrate_bps, 8000u);
    EXPECT_EQ(find_codec("L16")->bitrate_bps, 1411200u);
}
TEST(CodecTable, FramesAndTimestamps) {
    auto* g729 = find_codec("G729"); ASSERT_NE(g729, nullptr);
    EXPECT_EQ(frames_per_packet(*g729, 20), 2u);
    EXPECT_EQ(frames_per_packet(*g729, 10), 1u);
    EXPECT_EQ(frames_per_packet(*g729, 15), 0u);              // not a multiple of the 10 ms frame
    auto* amrwb = find_codec("AMR-WB"); ASSERT_NE(amrwb, nullptr);
    EXPECT_EQ(frames_per_packet(*amrwb, 20), 1u);
    EXPECT_EQ(frames_per_packet(*amrwb, 40), 2u);
    EXPECT_EQ(frames_per_packet(*amrwb, 30), 0u);
    EXPECT_EQ(frames_per_packet(codec_or_dummy("PCMU"), 20), 0u);   // sample based
    EXPECT_EQ(codec_ts_step(codec_or_dummy("PCMU"), 20), 160u);
    EXPECT_EQ(codec_ts_step(codec_or_dummy("PCMU"), 30), 240u);
    EXPECT_EQ(codec_ts_step(codec_or_dummy("G722"), 20), 160u);     // clock 8000, NOT 320
    EXPECT_EQ(codec_ts_step(codec_or_dummy("opus"), 20), 960u);
    EXPECT_EQ(codec_ts_step(codec_or_dummy("opus"), 10), 480u);
    EXPECT_EQ(codec_ts_step(codec_or_dummy("AMR-WB"), 20), 320u);
    EXPECT_EQ(codec_ts_step(codec_or_dummy("L16"), 20), 882u);
}
TEST(CodecTable, NominalPayloadBytes) {
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("PCMU"), 20), 160u);
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("PCMA"), 10), 80u);
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("G722"), 20), 160u);   // 64 kbit/s: same bytes as G.711 although twice the samples
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("G729"), 20), 20u);
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("G729"), 30), 30u);
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("GSM"), 20), 33u);     // 260 bits -> 33 bytes
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("AMR"), 20), 31u);     // 244 bits
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("AMR-WB"), 20), 60u);  // 477 bits
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("opus"), 20), 0u);     // variable
    EXPECT_EQ(codec_payload_bytes_nominal(codec_or_dummy("G729"), 15), 0u);     // not a whole number of frames
}
TEST(CodecTable, OpusFrameSizes) {
    for (uint32_t ok : {25u, 50u, 100u, 200u, 400u, 600u}) EXPECT_TRUE(opus_valid_frame_tenth_ms(ok)) << ok;
    for (uint32_t bad : {0u, 10u, 30u, 150u, 300u, 800u, 1200u}) EXPECT_FALSE(opus_valid_frame_tenth_ms(bad)) << bad;
}

// ============================================================ radio vocoders (frame arithmetic only)
TEST(Vocoders, Imbe) {
    auto v = imbe_full_rate();
    EXPECT_EQ(v.frame_us, 20000u);
    EXPECT_EQ(v.total_bits(), 144u);
    EXPECT_EQ(v.bitrate_bps(), 7200u);
    EXPECT_EQ(v.voice_bps(), 4400u);                         // 88 bits / 20 ms
    EXPECT_EQ(v.fec_bits, 56u);
    EXPECT_EQ(vocoder_payload_bytes(v, 1), 18u);
    EXPECT_EQ(vocoder_payload_bytes(v, 9), 162u);            // 9 frames = 1296 bits
}
TEST(Vocoders, P25LogicalLinkDataUnit) {
    EXPECT_EQ(kLduFrames, 9u);
    EXPECT_EQ(ldu_duration_ms(), 180u);
    EXPECT_EQ(ldu_total_bits(), 1728u);                      // 9600 b/s * 0.18 s
    EXPECT_EQ(ldu_voice_bits(), 1296u);
    EXPECT_EQ(ldu_overhead_bits(), 432u);                    // 25 % of the LDU is not voice
    EXPECT_NEAR(ldu_overhead_bits() * 100.0 / ldu_total_bits(), 25.0, 1e-9);
}
TEST(Vocoders, AmbePlus2HalfRate) {
    auto v = ambe_plus2_half_rate();
    EXPECT_EQ(v.frame_us, 20000u);
    EXPECT_EQ(v.total_bits(), 72u);
    EXPECT_EQ(v.bitrate_bps(), 3600u);
    EXPECT_EQ(v.voice_bps(), 2450u);                         // 49 bits / 20 ms
    EXPECT_EQ(vocoder_payload_bytes(v, 1), 9u);
    EXPECT_EQ(vocoder_payload_bytes(v, 3), 27u);             // 3 frames = 60 ms = one DMR voice burst of 216 bits
}
TEST(Vocoders, TetraAcelp) {
    auto v = tetra_acelp();
    EXPECT_EQ(v.frame_us, 30000u);
    EXPECT_EQ(v.total_bits(), 137u);
    EXPECT_EQ(v.bitrate_bps(), 4567u);                       // 4566.67 rounded
    EXPECT_EQ(vocoder_payload_bytes(v, 1), 18u);             // 137 bits -> 18 bytes (7 padding bits)
    EXPECT_EQ(vocoder_payload_bytes(v, 2), 35u);             // 274 bits -> 35 bytes
}
TEST(Vocoders, FrameRatesAreNotMultiplesOfEachOther) {
    // A gateway between IMBE (20 ms) and TETRA (30 ms) must re-time: LCM = 60 ms = 3 IMBE frames = 2 ACELP frames.
    uint32_t a = imbe_full_rate().frame_us, b = tetra_acelp().frame_us;
    uint32_t l = a; while (l % b) l += a;
    EXPECT_EQ(l, 60000u);
    EXPECT_EQ(l / a, 3u); EXPECT_EQ(l / b, 2u);
}
