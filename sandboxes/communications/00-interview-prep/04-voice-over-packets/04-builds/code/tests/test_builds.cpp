// Tests for pack 04-audio/04-builds. Shared by the student stub (include/builds.hpp) and the solution (solutions/builds.hpp).
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>

#include "builds.hpp"

using namespace ether;
using Kind = PlayoutResult::Kind;

namespace {
constexpr double kPi = 3.14159265358979323846;
std::vector<int16_t> sine(double f, size_t n, double amp = 10000.0, double fs = 8000.0) {
    std::vector<int16_t> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = static_cast<int16_t>(std::lround(amp * std::sin(2.0 * kPi * f * static_cast<double>(i) / fs)));
    return v;
}
double snr_db(const std::vector<int16_t>& ref, const std::vector<int16_t>& got) {
    double s = 0, e = 0;
    for (size_t i = 0; i < ref.size() && i < got.size(); ++i) { double d = static_cast<double>(ref[i]) - got[i]; s += double(ref[i]) * ref[i]; e += d * d; }
    return e == 0 ? 200.0 : 10.0 * std::log10(s / e);
}
uint32_t be32(const Bytes& b, size_t o) { return (uint32_t(b[o]) << 24) | (uint32_t(b[o + 1]) << 16) | (uint32_t(b[o + 2]) << 8) | b[o + 3]; }
uint32_t le32(const Bytes& b, size_t o) { return uint32_t(b[o]) | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) | (uint32_t(b[o + 3]) << 24); }
uint16_t le16(const Bytes& b, size_t o) { return static_cast<uint16_t>(b[o] | (b[o + 1] << 8)); }
bool tag(const Bytes& b, size_t o, const char* t) { return b.size() >= o + 4 && std::memcmp(&b[o], t, 4) == 0; }

// ---- helpers to hand-build WAV files for the reader tests
void p16(Bytes& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
void p32(Bytes& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back((v >> (8 * i)) & 0xFF); }
void ptag(Bytes& b, const char* t) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(t[i])); }
Bytes fmt_chunk(uint16_t format = 1, uint16_t ch = 1, uint32_t rate = 8000, uint16_t bits = 16) {
    Bytes b; ptag(b, "fmt "); p32(b, 16); p16(b, format); p16(b, ch); p32(b, rate); p32(b, rate * ch * bits / 8); p16(b, ch * bits / 8); p16(b, bits); return b;
}
Bytes data_chunk(const std::vector<int16_t>& s, uint32_t declared = 0xFFFFFFFFu) {
    Bytes b; ptag(b, "data"); p32(b, declared == 0xFFFFFFFFu ? uint32_t(s.size() * 2) : declared);
    for (int16_t v : s) p16(b, static_cast<uint16_t>(v));
    return b;
}
Bytes riff(const std::vector<Bytes>& chunks) {
    Bytes body; ptag(body, "WAVE");
    for (const auto& c : chunks) body.insert(body.end(), c.begin(), c.end());
    Bytes b; ptag(b, "RIFF"); p32(b, uint32_t(body.size())); b.insert(b.end(), body.begin(), body.end());
    return b;
}
}  // namespace

// ============================================================ WAV
TEST(Wav, HeaderLayoutIsCanonical) {
    Bytes w = wav_write_pcm16({1, -2, 0x1234}, 8000);
    ASSERT_EQ(w.size(), 44u + 6u);
    EXPECT_TRUE(tag(w, 0, "RIFF"));
    EXPECT_EQ(le32(w, 4), 36u + 6u);                    // RIFF size = file size - 8
    EXPECT_TRUE(tag(w, 8, "WAVE"));
    EXPECT_TRUE(tag(w, 12, "fmt "));
    EXPECT_EQ(le32(w, 16), 16u);                        // fmt chunk size
    EXPECT_EQ(le16(w, 20), 1u);                         // PCM
    EXPECT_EQ(le16(w, 22), 1u);                         // channels
    EXPECT_EQ(le32(w, 24), 8000u);                      // sample rate
    EXPECT_EQ(le32(w, 28), 16000u);                     // byte rate = rate * block align
    EXPECT_EQ(le16(w, 32), 2u);                         // block align
    EXPECT_EQ(le16(w, 34), 16u);                        // bits per sample
    EXPECT_TRUE(tag(w, 36, "data"));
    EXPECT_EQ(le32(w, 40), 6u);
    EXPECT_EQ(w[44], 0x01); EXPECT_EQ(w[45], 0x00);     // little-endian samples
    EXPECT_EQ(w[46], 0xFE); EXPECT_EQ(w[47], 0xFF);
    EXPECT_EQ(w[48], 0x34); EXPECT_EQ(w[49], 0x12);
}
TEST(Wav, StereoHeader) {
    Bytes w = wav_write_pcm16({1, 2, 3, 4}, 48000, 2);
    ASSERT_EQ(w.size(), 44u + 8u);
    EXPECT_EQ(le16(w, 22), 2u);
    EXPECT_EQ(le32(w, 28), 192000u);                    // 48000 * 4
    EXPECT_EQ(le16(w, 32), 4u);
}
TEST(Wav, RoundTripIncludingExtremesAndStereo) {
    std::vector<int16_t> s = {0, 1, -1, 32767, -32768, 12345, -12345};
    auto r = wav_read_pcm16(wav_write_pcm16(s, 8000));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->sample_rate, 8000u); EXPECT_EQ(r->channels, 1u); EXPECT_EQ(r->samples, s);
    std::vector<int16_t> st = {1, -1, 2, -2, 3, -3};
    auto q = wav_read_pcm16(wav_write_pcm16(st, 44100, 2));
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ(q->sample_rate, 44100u); EXPECT_EQ(q->channels, 2u); EXPECT_EQ(q->samples, st);
}
TEST(Wav, EmptyDataIsValid) {
    auto r = wav_read_pcm16(wav_write_pcm16({}, 16000));
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->samples.empty());
    EXPECT_EQ(r->sample_rate, 16000u);
}
TEST(Wav, ReaderSkipsUnknownChunksWithPadding) {
    Bytes list; ptag(list, "LIST"); p32(list, 5); for (int i = 0; i < 5; ++i) list.push_back('x'); list.push_back(0);   // odd size + pad byte
    auto r = wav_read_pcm16(riff({fmt_chunk(), list, data_chunk({7, 8, 9})}));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->samples, (std::vector<int16_t>{7, 8, 9}));
    Bytes fact; ptag(fact, "fact"); p32(fact, 4); p32(fact, 3);                    // chunk BEFORE fmt is fine too
    auto q = wav_read_pcm16(riff({fact, fmt_chunk(), data_chunk({1})}));
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ(q->samples, (std::vector<int16_t>{1}));
}
TEST(Wav, ReaderRejectsMalformedFiles) {
    EXPECT_FALSE(wav_read_pcm16({}).has_value());
    EXPECT_FALSE(wav_read_pcm16(Bytes(11, 0)).has_value());
    Bytes good = wav_write_pcm16({1, 2, 3, 4}, 8000);
    ASSERT_EQ(good.size(), 44u + 8u);
    Bytes bad_magic = good; bad_magic[0] = 'X';
    EXPECT_FALSE(wav_read_pcm16(bad_magic).has_value());
    Bytes bad_wave = good; bad_wave[8] = 'X';
    EXPECT_FALSE(wav_read_pcm16(bad_wave).has_value());
    EXPECT_FALSE(wav_read_pcm16(riff({fmt_chunk(1, 1, 8000, 8), data_chunk({1, 2})})).has_value());      // 8-bit
    EXPECT_FALSE(wav_read_pcm16(riff({fmt_chunk(3, 1, 8000, 16), data_chunk({1, 2})})).has_value());     // IEEE float format tag
    EXPECT_FALSE(wav_read_pcm16(riff({fmt_chunk(1, 0, 8000, 16), data_chunk({1, 2})})).has_value());     // zero channels
    EXPECT_FALSE(wav_read_pcm16(riff({data_chunk({1, 2}), fmt_chunk()})).has_value());                   // data before fmt
    EXPECT_FALSE(wav_read_pcm16(riff({fmt_chunk()})).has_value());                                       // no data chunk
    EXPECT_FALSE(wav_read_pcm16(riff({fmt_chunk(), data_chunk({1, 2, 3}, 100)})).has_value());           // declared size > file: truncated
    Bytes cut(good.begin(), good.end() - 3);
    EXPECT_FALSE(wav_read_pcm16(cut).has_value());
    EXPECT_FALSE(wav_read_pcm16(riff({fmt_chunk(1, 2, 8000, 16), data_chunk({1, 2, 3}, 6)})).has_value());  // 3 samples is not whole stereo frames
}

// ============================================================ mu-law exhaustive harness
namespace {
// Deliberately broken codecs. Each is the reference with ONE mistake; a good harness must reject every one of them.
uint8_t enc_nobias(int16_t pcm) {
    int s = pcm, sign = 0; if (s < 0) { s = -s; sign = 0x80; } if (s > kMuClip) s = kMuClip;
    int e = 7; for (int m = 0x4000; (s & m) == 0 && e > 0; m >>= 1) --e;
    return static_cast<uint8_t>(~(sign | (e << 4) | ((s >> (e + 3)) & 0x0F)));
}
uint8_t enc_noclip(int16_t pcm) {
    int s = pcm, sign = 0; if (s < 0) { s = -s; sign = 0x80; } s += kMuBias;
    int e = 7; for (int m = 0x4000; (s & m) == 0 && e > 0; m >>= 1) --e;
    return static_cast<uint8_t>(~(sign | (e << 4) | ((s >> (e + 3)) & 0x0F)));
}
uint8_t enc_neg16(int16_t pcm) {                      // -(-32768) overflows int16_t
    int16_t s = pcm; int sign = 0; if (s < 0) { s = static_cast<int16_t>(-s); sign = 0x80; }
    int v = s; if (v > kMuClip) v = kMuClip; v += kMuBias;
    int e = 7; for (int m = 0x4000; (v & m) == 0 && e > 0; m >>= 1) --e;
    return static_cast<uint8_t>(~(sign | (e << 4) | ((v >> (e + 3)) & 0x0F)));
}
uint8_t enc_bias128(int16_t pcm) {                    // 128 instead of 132: segments right, rounding wrong
    int s = pcm, sign = 0; if (s < 0) { s = -s; sign = 0x80; } if (s > kMuClip) s = kMuClip; s += 128;
    int e = 7; for (int m = 0x4000; (s & m) == 0 && e > 0; m >>= 1) --e;
    return static_cast<uint8_t>(~(sign | (e << 4) | ((s >> (e + 3)) & 0x0F)));
}
uint8_t enc_noinvert(int16_t pcm) { return static_cast<uint8_t>(~mulaw_encode(pcm)); }                         // consistent with dec_noinvert
int16_t dec_noinvert(uint8_t u) { return mulaw_decode(static_cast<uint8_t>(~u)); }
uint8_t enc_signflip(int16_t pcm) { return pcm < -20000 ? mulaw_encode(static_cast<int16_t>(-pcm > 32767 ? 32767 : -pcm)) : mulaw_encode(pcm); }
int16_t dec_nomask(uint8_t u) {                       // decoder forgets to subtract the bias again (reconstructs 132 too high at the bottom)
    u = static_cast<uint8_t>(~u);
    int sign = u & 0x80, e = (u >> 4) & 7, m = u & 0x0F;
    int s = ((m << 3) + kMuBias) << e;
    return static_cast<int16_t>(sign ? -s : s);
}
}  // namespace

TEST(MulawExhaustive, ReferenceCodecPasses) {
    MulawReport r = mulaw_verify_exhaustive(mulaw_encode, mulaw_decode);
    EXPECT_EQ(r.inputs_checked, 65536u);
    EXPECT_EQ(r.codes_checked, 255u);                   // 0x7F ("negative zero") is excluded from the round-trip
    EXPECT_EQ(r.vector_failures, 0u);
    EXPECT_EQ(r.roundtrip_failures, 0u);
    EXPECT_EQ(r.error_violations, 0u);
    EXPECT_EQ(r.monotonic_violations, 0u);
    EXPECT_EQ(r.symmetry_violations, 0u);
    EXPECT_TRUE(r.ok());
    EXPECT_LE(r.max_error, 512);                        // half of the largest step (1024)
    EXPECT_GE(r.max_error, 500);                        // ... and the bound is nearly reached: the check is not vacuous
}
TEST(MulawExhaustive, EveryBrokenVariantIsRejected) {
    EXPECT_FALSE(mulaw_verify_exhaustive(enc_nobias, mulaw_decode).ok()) << "no bias";
    EXPECT_FALSE(mulaw_verify_exhaustive(enc_noclip, mulaw_decode).ok()) << "no clipping";
    EXPECT_FALSE(mulaw_verify_exhaustive(enc_neg16, mulaw_decode).ok()) << "int16 negation of -32768";
    EXPECT_FALSE(mulaw_verify_exhaustive(enc_bias128, mulaw_decode).ok()) << "bias 128";
    EXPECT_FALSE(mulaw_verify_exhaustive(enc_noinvert, dec_noinvert).ok()) << "bit inversion forgotten (the known vectors catch it)";
    EXPECT_FALSE(mulaw_verify_exhaustive(enc_signflip, mulaw_decode).ok()) << "sign error for large negative inputs";
    EXPECT_FALSE(mulaw_verify_exhaustive(mulaw_encode, dec_nomask).ok()) << "decoder without bias subtraction";
}
TEST(MulawExhaustive, ReportCountsPointAtTheBug) {
    MulawReport nb = mulaw_verify_exhaustive(enc_nobias, mulaw_decode);
    EXPECT_GT(nb.error_violations + nb.roundtrip_failures, 0u);
    MulawReport ni = mulaw_verify_exhaustive(enc_noinvert, dec_noinvert);
    EXPECT_GT(ni.vector_failures, 0u);                  // the published vectors expose it at once (silence must be 0xFF)
    EXPECT_EQ(ni.inputs_checked, 65536u);               // the harness keeps going and reports everything, it does not stop at the first failure
    MulawReport nc = mulaw_verify_exhaustive(enc_noclip, mulaw_decode);
    EXPECT_GT(nc.monotonic_violations + nc.error_violations, 0u);
}

// ============================================================ RTP
TEST(Rtp, BuildLayout) {
    RtpFields f; f.marker = true; f.pt = 0; f.seq = 0xABCD; f.ts = 0x01020304; f.ssrc = 0xDEADBEEF;
    uint8_t pl[3] = {9, 8, 7};
    Bytes b = rtp_build(f, pl, 3);
    ASSERT_EQ(b.size(), 15u);
    EXPECT_EQ(b[0], 0x80);                              // V=2 P=0 X=0 CC=0
    EXPECT_EQ(b[1], 0x80);                              // M=1 PT=0
    EXPECT_EQ(b[2], 0xAB); EXPECT_EQ(b[3], 0xCD);
    EXPECT_EQ(be32(b, 4), 0x01020304u);
    EXPECT_EQ(be32(b, 8), 0xDEADBEEFu);
    EXPECT_EQ(b[12], 9); EXPECT_EQ(b[14], 7);
    f.marker = false; f.pt = 8;
    EXPECT_EQ(rtp_build(f, pl, 0)[1], 0x08);
}
TEST(Rtp, ParseRoundTripAndRejects) {
    RtpFields f; f.marker = true; f.pt = 96; f.seq = 65535; f.ts = 0xFFFFFFFFu; f.ssrc = 7;
    uint8_t pl[4] = {1, 2, 3, 4};
    ASSERT_EQ(rtp_build(f, pl, 4).size(), 16u);
    auto p = rtp_parse(rtp_build(f, pl, 4));
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(p->f.marker); EXPECT_EQ(p->f.pt, 96); EXPECT_EQ(p->f.seq, 65535); EXPECT_EQ(p->f.ts, 0xFFFFFFFFu); EXPECT_EQ(p->f.ssrc, 7u);
    EXPECT_EQ(p->payload, (Bytes{1, 2, 3, 4}));
    EXPECT_FALSE(rtp_parse(Bytes(11, 0x80)).has_value());                 // shorter than the fixed header
    Bytes v1 = rtp_build(f, pl, 4); v1[0] = 0x40;
    EXPECT_FALSE(rtp_parse(v1).has_value());                              // version 1
    Bytes v0 = rtp_build(f, pl, 4); v0[0] = 0x00;
    EXPECT_FALSE(rtp_parse(v0).has_value());
    auto empty = rtp_parse(rtp_build(f, pl, 0));
    ASSERT_TRUE(empty.has_value());
    EXPECT_TRUE(empty->payload.empty());
}
TEST(Rtp, ParseHandlesCsrcExtensionAndPadding) {
    RtpFields f; f.seq = 1; f.ts = 2; f.ssrc = 3;
    uint8_t pl[3] = {0xAA, 0xBB, 0xCC};
    Bytes base = rtp_build(f, pl, 0);
    ASSERT_EQ(base.size(), 12u);
    // two CSRCs (8 bytes) + extension (profile 0xBEDE, length 2 words) + payload + 2 bytes of padding (last byte = count)
    Bytes b = base; b[0] = 0x80 | 0x20 | 0x10 | 2;
    for (int i = 0; i < 8; ++i) b.push_back(0x11);
    b.push_back(0xBE); b.push_back(0xDE); b.push_back(0); b.push_back(2);
    for (int i = 0; i < 8; ++i) b.push_back(0x22);
    b.insert(b.end(), pl, pl + 3);
    b.push_back(0); b.push_back(2);
    auto p = rtp_parse(b);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->payload, (Bytes{0xAA, 0xBB, 0xCC}));
    Bytes trunc_csrc = base; trunc_csrc[0] = 0x80 | 15;                   // claims 15 CSRCs, has none
    EXPECT_FALSE(rtp_parse(trunc_csrc).has_value());
    Bytes trunc_ext = base; trunc_ext[0] = 0x80 | 0x10;                   // extension bit but no extension header
    EXPECT_FALSE(rtp_parse(trunc_ext).has_value());
    Bytes bad_pad = rtp_build(f, pl, 3); bad_pad[0] |= 0x20; bad_pad.back() = 200;   // pad count larger than the payload
    EXPECT_FALSE(rtp_parse(bad_pad).has_value());
    Bytes zero_pad = rtp_build(f, pl, 3); zero_pad[0] |= 0x20; zero_pad.back() = 0;  // pad count 0 is invalid
    EXPECT_FALSE(rtp_parse(zero_pad).has_value());
}
TEST(Packetiser, SequenceTimestampMarkerAndPayload) {
    PcmuPacketiser pk(0x1234, 1000, 50000);
    auto pcm = sine(440.0, 480);
    auto pkts = pk.push(pcm);
    ASSERT_EQ(pkts.size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        ASSERT_EQ(pkts[i].size(), 12u + 160u);
        auto p = rtp_parse(pkts[i]);
        ASSERT_TRUE(p.has_value());
        EXPECT_EQ(p->f.seq, 1000 + i);
        EXPECT_EQ(p->f.ts, 50000u + 160u * i);
        EXPECT_EQ(p->f.pt, 0);
        EXPECT_EQ(p->f.ssrc, 0x1234u);
        EXPECT_EQ(p->f.marker, i == 0);                  // marker only on the first packet of the talkspurt
        for (size_t k = 0; k < 160; ++k) ASSERT_EQ(p->payload[k], mulaw_encode(pcm[160 * i + k])) << i << "," << k;
    }
    EXPECT_EQ(pk.next_seq(), 1003u);
    EXPECT_EQ(pk.next_ts(), 50480u);
    EXPECT_TRUE(pk.flush().empty());                     // nothing pending
}
TEST(Packetiser, ChunkedInputFlushAndPadding) {
    PcmuPacketiser pk(1, 0, 0);
    auto pcm = sine(300.0, 500);
    std::vector<Bytes> all;
    for (size_t pos = 0; pos < pcm.size();) {            // feed in awkward chunk sizes
        size_t n = std::min<size_t>(pos % 3 == 0 ? 37 : 211, pcm.size() - pos);
        for (auto& b : pk.push(std::vector<int16_t>(pcm.begin() + pos, pcm.begin() + pos + n))) all.push_back(b);
        pos += n;
    }
    ASSERT_EQ(all.size(), 3u);                           // 500 samples = 3 full frames + 20 left over
    auto last = pk.flush();
    ASSERT_EQ(last.size(), 1u);
    auto p = rtp_parse(last[0]);
    ASSERT_TRUE(p.has_value());
    ASSERT_EQ(p->payload.size(), 160u);
    for (size_t k = 0; k < 20; ++k) EXPECT_EQ(p->payload[k], mulaw_encode(pcm[480 + k]));
    for (size_t k = 20; k < 160; ++k) EXPECT_EQ(p->payload[k], 0xFF) << k;          // padded with mu-law silence
    EXPECT_EQ(p->f.seq, 3u); EXPECT_EQ(p->f.ts, 480u);
    EXPECT_TRUE(pk.flush().empty());
}
TEST(Packetiser, WrapAroundOfSequenceAndTimestamp) {
    PcmuPacketiser pk(1, 65535, 0xFFFFFFF0u);
    auto pkts = pk.push(sine(500.0, 320));
    ASSERT_EQ(pkts.size(), 2u);
    auto a = rtp_parse(pkts[0]), b = rtp_parse(pkts[1]);
    ASSERT_TRUE(a && b);
    EXPECT_EQ(a->f.seq, 65535u); EXPECT_EQ(b->f.seq, 0u);
    EXPECT_EQ(a->f.ts, 0xFFFFFFF0u); EXPECT_EQ(b->f.ts, 0xFFFFFFF0u + 160u);       // wraps to 0x90
    EXPECT_EQ(b->f.ts, 0x90u);
}
TEST(Packetiser, SilenceSkipsTimestampNotSequenceAndRearmsMarker) {
    PcmuPacketiser pk(1, 10, 1000);
    auto first = pk.push(sine(500.0, 160));
    ASSERT_EQ(first.size(), 1u);
    pk.skip_silence(8000);                               // 1 s of DTX
    auto second = pk.push(sine(500.0, 160));
    ASSERT_EQ(second.size(), 1u);
    auto a = rtp_parse(first[0]), b = rtp_parse(second[0]);
    ASSERT_TRUE(a && b);
    EXPECT_EQ(b->f.seq, a->f.seq + 1u);                  // contiguous sequence numbers
    EXPECT_EQ(b->f.ts, a->f.ts + 160u + 8000u);          // timestamp jumped by the silence
    EXPECT_TRUE(a->f.marker); EXPECT_TRUE(b->f.marker);
    auto third = pk.push(sine(500.0, 160));
    EXPECT_FALSE(rtp_parse(third[0])->f.marker);
}
TEST(WavToRtp, AcceptsOnlyMono8k) {
    auto ok = wav_to_rtp(wav_write_pcm16(sine(440, 800), 8000), 5, 0, 0);
    ASSERT_TRUE(ok.has_value());
    EXPECT_EQ(ok->size(), 5u);                           // 800 samples = 5 frames of 160
    EXPECT_FALSE(wav_to_rtp(wav_write_pcm16(sine(440, 800), 16000), 5, 0, 0).has_value());
    EXPECT_FALSE(wav_to_rtp(wav_write_pcm16(sine(440, 800), 8000, 2), 5, 0, 0).has_value());
    EXPECT_FALSE(wav_to_rtp(Bytes{1, 2, 3}, 5, 0, 0).has_value());
    auto odd = wav_to_rtp(wav_write_pcm16(sine(440, 850), 8000), 5, 0, 0);          // 850 = 5 frames + 50: flush pads the sixth
    ASSERT_TRUE(odd.has_value());
    EXPECT_EQ(odd->size(), 6u);
}

// ============================================================ network emulator
TEST(NetworkEmulator, PerfectNetworkKeepsOrderAndSpacing) {
    std::vector<Bytes> pk; for (int i = 0; i < 10; ++i) pk.push_back(Bytes{static_cast<uint8_t>(i)});
    NetworkResult r = simulate_network(pk, 1000, 20000, NetworkModel{});
    ASSERT_EQ(r.arrivals.size(), 10u);
    EXPECT_TRUE(r.dropped.empty());
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(r.arrivals[i].index, i);
        EXPECT_EQ(r.arrivals[i].arrival_us, 1000u + 20000u * i);
        EXPECT_EQ(r.arrivals[i].packet, pk[i]);
    }
}
TEST(NetworkEmulator, LossDelayReorderAndDeterminism) {
    std::vector<Bytes> pk(2000, Bytes{1});
    NetworkModel m; m.loss_percent = 10; m.min_delay_us = 5000; m.max_delay_us = 45000; m.seed = 99;
    NetworkResult a = simulate_network(pk, 0, 20000, m), b = simulate_network(pk, 0, 20000, m);
    EXPECT_EQ(a.dropped, b.dropped);                     // deterministic
    ASSERT_EQ(a.arrivals.size(), b.arrivals.size());
    for (size_t i = 0; i < a.arrivals.size(); ++i) { EXPECT_EQ(a.arrivals[i].arrival_us, b.arrivals[i].arrival_us); EXPECT_EQ(a.arrivals[i].index, b.arrivals[i].index); }
    EXPECT_GT(a.dropped.size(), 140u); EXPECT_LT(a.dropped.size(), 260u);          // ~10 % of 2000
    EXPECT_EQ(a.dropped.size() + a.arrivals.size(), 2000u);
    EXPECT_TRUE(std::is_sorted(a.dropped.begin(), a.dropped.end()));
    std::set<size_t> seen(a.dropped.begin(), a.dropped.end());
    size_t reordered = 0;
    for (size_t i = 0; i < a.arrivals.size(); ++i) {
        const Arrival& x = a.arrivals[i];
        EXPECT_TRUE(seen.insert(x.index).second);        // every packet is either dropped or delivered, exactly once
        uint64_t send = 20000ull * x.index;
        EXPECT_GE(x.arrival_us, send + 5000u); EXPECT_LE(x.arrival_us, send + 45000u);
        if (i > 0) { EXPECT_GE(x.arrival_us, a.arrivals[i - 1].arrival_us); if (x.index < a.arrivals[i - 1].index) ++reordered; }
    }
    EXPECT_GT(reordered, 100u);                          // a 40 ms delay spread over 20 ms spacing really reorders packets
    m.seed = 100;
    EXPECT_NE(simulate_network(pk, 0, 20000, m).dropped, a.dropped);
    m.loss_percent = 100;
    EXPECT_TRUE(simulate_network(pk, 0, 20000, m).arrivals.empty());
}

// ============================================================ receiver / whole pipeline
TEST(Pipeline, PerfectNetworkReproducesMulawRoundTripExactly) {
    auto pcm = sine(440.0, 8000);                        // 1 s = 50 frames
    Bytes wav = wav_write_pcm16(pcm, 8000);
    auto pkts = wav_to_rtp(wav, 0xCAFE, 500, 123456);
    ASSERT_TRUE(pkts.has_value());
    ASSERT_EQ(pkts->size(), 50u);
    NetworkResult net = simulate_network(*pkts, 0, 20000, NetworkModel{});
    JbConfig cfg;                                        // depth 3, 20 ms
    ReceiverResult r = run_receiver(net.arrivals, cfg, 60000, 55);
    ASSERT_EQ(r.kinds.size(), 55u);
    ASSERT_EQ(r.pcm.size(), 55u * 160u);
    for (size_t t = 0; t < 50; ++t) {
        ASSERT_EQ(r.kinds[t], Kind::Packet) << "tick " << t;
        EXPECT_EQ(r.slot_ts[t], 123456u + 160u * t);
        for (size_t k = 0; k < 160; ++k) ASSERT_EQ(r.pcm[160 * t + k], mulaw_decode(mulaw_encode(pcm[160 * t + k]))) << t << "," << k;
    }
    for (size_t t = 50; t < 55; ++t) EXPECT_NE(r.kinds[t], Kind::Packet);           // stream over: concealment then silence
    for (size_t i = 50 * 160; i < r.pcm.size(); ++i) ASSERT_EQ(r.pcm[i], 0);
    EXPECT_EQ(r.stats.late, 0u); EXPECT_EQ(r.stats.lost, 0u); EXPECT_EQ(r.bad_packets, 0u);
    std::vector<int16_t> head(r.pcm.begin(), r.pcm.begin() + 8000);
    EXPECT_GT(snr_db(pcm, head), 30.0);                  // mu-law at -10 dBFS: ~37 dB
}
TEST(Pipeline, ShuffledAndDroppedStream) {
    auto pcm = sine(440.0, 8000 * 5);                    // 5 s = 250 frames
    auto pkts = wav_to_rtp(wav_write_pcm16(pcm, 8000), 1, 0, 0);
    ASSERT_TRUE(pkts.has_value());
    ASSERT_EQ(pkts->size(), 250u);
    NetworkModel m; m.loss_percent = 5; m.min_delay_us = 5000; m.max_delay_us = 40000; m.seed = 4242;
    NetworkResult net = simulate_network(*pkts, 0, 20000, m);
    JbConfig cfg; cfg.initial_depth = 4;
    ReceiverResult r = run_receiver(net.arrivals, cfg, 140000, 260);
    std::vector<int16_t> ref_played, got_played;
    uint32_t prev_ts = 0; size_t played = 0, lost = 0;
    std::set<size_t> dropped(net.dropped.begin(), net.dropped.end());
    for (size_t t = 0; t < r.kinds.size(); ++t) {
        if (r.kinds[t] == Kind::Packet) {
            if (played) { EXPECT_GT(r.slot_ts[t], prev_ts) << "tick " << t; }       // strictly increasing: reordering repaired, no duplicates
            prev_ts = r.slot_ts[t]; ++played;
            size_t frame = r.slot_ts[t] / 160;
            ASSERT_LT(frame, 250u);
            EXPECT_EQ(dropped.count(frame), 0u) << "played a packet that never arrived";
            for (size_t k = 0; k < 160; ++k) {
                ref_played.push_back(pcm[160 * frame + k]); got_played.push_back(r.pcm[160 * t + k]);
                ASSERT_EQ(r.pcm[160 * t + k], mulaw_decode(mulaw_encode(pcm[160 * frame + k]))) << "tick " << t << " frame " << frame;
            }
        } else if (r.kinds[t] == Kind::Lost) {
            ++lost;
            for (size_t k = 0; k < 160; ++k) ASSERT_EQ(r.pcm[160 * t + k], 0);       // this pipeline conceals with silence
        }
    }
    EXPECT_GT(net.dropped.size(), 3u);
    EXPECT_GE(lost + 1, net.dropped.size());             // every network loss produced a concealed slot (the last one may fall past the end)
    EXPECT_GE(played + r.stats.late + 3, net.arrivals.size());                      // everything delivered was played or counted as late
    EXPECT_LE(r.stats.late, 10u);                        // depth 80 ms covers delays up to 40 ms
    EXPECT_GT(snr_db(ref_played, got_played), 30.0);
}
TEST(Pipeline, GarbagePacketsAreCountedNotFatal) {
    auto pcm = sine(440.0, 1600);
    auto pkts = wav_to_rtp(wav_write_pcm16(pcm, 8000), 1, 0, 0);
    ASSERT_TRUE(pkts.has_value());
    NetworkResult net = simulate_network(*pkts, 0, 20000, NetworkModel{});
    net.arrivals.push_back({10000, Bytes{1, 2, 3}, 99});
    net.arrivals.push_back({10001, Bytes(40, 0x00), 98});                           // version 0
    std::stable_sort(net.arrivals.begin(), net.arrivals.end(), [](const Arrival& a, const Arrival& b) { return a.arrival_us < b.arrival_us; });
    ReceiverResult r = run_receiver(net.arrivals, JbConfig{}, 60000, 14);
    EXPECT_EQ(r.bad_packets, 2u);
    size_t packets = 0; for (Kind k : r.kinds) if (k == Kind::Packet) ++packets;
    EXPECT_EQ(packets, 10u);
}
