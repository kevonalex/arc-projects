// SOLUTION - 04-audio/04-builds. Pure C++17, header-only, deterministic. Tests: tests/test_builds.cpp
//   1. WAV (RIFF/PCM16) writer and a defensive reader
//   2. Exhaustive mu-law verification harness (tests the codec, and tests that the harness catches broken codecs)
//   3. RTP header build/parse + a PCMU packetiser
//   4. A deterministic network emulator (loss, delay => reordering) and a receiver that glues RTP parse -> JitterBuffer -> mu-law decode
// Given components (third/): mulaw.hpp (pack 02) and jitter_buffer.hpp (pack 03). Synthetic audio only.
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <vector>

#include "jitter_buffer.hpp"
#include "mulaw.hpp"

namespace ether {

// ============================================================================================
// 1. WAV
// ============================================================================================
struct WavData {
    uint32_t sample_rate = 0;
    uint16_t channels = 0;
    std::vector<int16_t> samples;       // interleaved if channels > 1
};

namespace detail {
inline void put16(Bytes& b, uint16_t v) { b.push_back(static_cast<uint8_t>(v & 0xFF)); b.push_back(static_cast<uint8_t>(v >> 8)); }
inline void put32(Bytes& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF)); }
inline void puttag(Bytes& b, const char* t) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(t[i])); }
inline uint16_t get16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
inline uint32_t get32(const uint8_t* p) { return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24); }
inline bool tag_is(const uint8_t* p, const char* t) { return std::memcmp(p, t, 4) == 0; }
}  // namespace detail

// 44-byte canonical header: RIFF <36+n> WAVE | fmt  16 <1 ch rate byterate blockalign 16> | data <n> ... samples (little-endian)
inline Bytes wav_write_pcm16(const std::vector<int16_t>& samples, uint32_t sample_rate, uint16_t channels = 1) {
    using namespace detail;
    const uint32_t data_bytes = static_cast<uint32_t>(samples.size() * 2);
    const uint16_t block_align = static_cast<uint16_t>(channels * 2);
    Bytes b;
    b.reserve(44 + data_bytes);
    puttag(b, "RIFF"); put32(b, 36 + data_bytes); puttag(b, "WAVE");
    puttag(b, "fmt "); put32(b, 16); put16(b, 1); put16(b, channels); put32(b, sample_rate);
    put32(b, sample_rate * block_align); put16(b, block_align); put16(b, 16);
    puttag(b, "data"); put32(b, data_bytes);
    for (int16_t s : samples) put16(b, static_cast<uint16_t>(s));
    return b;
}

// Accepts only integer PCM 16-bit. Skips unknown chunks (chunks are padded to an even size). Returns nullopt for anything malformed.
inline std::optional<WavData> wav_read_pcm16(const Bytes& b) {
    using namespace detail;
    if (b.size() < 12 || !tag_is(b.data(), "RIFF") || !tag_is(b.data() + 8, "WAVE")) return std::nullopt;
    size_t pos = 12;
    WavData w;
    bool have_fmt = false;
    while (pos + 8 <= b.size()) {
        const uint8_t* ck = b.data() + pos;
        const uint32_t size = get32(ck + 4);
        pos += 8;
        if (tag_is(ck, "fmt ")) {
            if (size < 16 || size > b.size() - pos) return std::nullopt;
            const uint8_t* f = b.data() + pos;
            const uint16_t format = get16(f), bits = get16(f + 14);
            w.channels = get16(f + 2);
            w.sample_rate = get32(f + 4);
            if (format != 1 || bits != 16 || w.channels == 0) return std::nullopt;    // not integer PCM16
            have_fmt = true;
        } else if (tag_is(ck, "data")) {
            if (!have_fmt) return std::nullopt;
            if (size > b.size() - pos) return std::nullopt;                           // truncated
            if (size % (2u * w.channels) != 0) return std::nullopt;                   // not whole frames
            w.samples.resize(size / 2);
            for (size_t i = 0; i < w.samples.size(); ++i) w.samples[i] = static_cast<int16_t>(get16(b.data() + pos + 2 * i));
            return w;
        }
        if (size > b.size() - pos) return std::nullopt;
        pos += static_cast<size_t>(size) + (size & 1u);                               // skip the chunk body + pad byte
    }
    return std::nullopt;                                                              // no data chunk
}

// ============================================================================================
// 2. Exhaustive mu-law verification
// ============================================================================================
using MulawEncodeFn = uint8_t (*)(int16_t);
using MulawDecodeFn = int16_t (*)(uint8_t);

struct MulawReport {
    size_t inputs_checked = 0;          // PCM inputs examined (65 536)
    size_t codes_checked = 0;           // wire codes examined for the round trip (255: 0x7F is "negative zero")
    size_t vector_failures = 0;         // published/known encode and decode vectors that did not match
    size_t roundtrip_failures = 0;      // codes c with encode(decode(c)) != c
    size_t error_violations = 0;        // inputs whose reconstruction error exceeds half the quantiser step of the chosen segment
    size_t monotonic_violations = 0;    // decode(encode(x)) must never decrease as x increases
    size_t symmetry_violations = 0;     // decode(encode(-x)) must equal -decode(encode(x))
    int max_error = 0;
    bool ok() const {
        return inputs_checked == 65536 && codes_checked == 255 && vector_failures == 0 && roundtrip_failures == 0 &&
               error_violations == 0 && monotonic_violations == 0 && symmetry_violations == 0;
    }
};

inline MulawReport mulaw_verify_exhaustive(MulawEncodeFn enc, MulawDecodeFn dec) {
    MulawReport r;
    // known vectors (hand-derived in pack 02): silence 0xFF, 1000 -> 0xCE -> 988, extremes +-32124
    const struct { int16_t pcm; uint8_t code; } ev[] = {{0, 0xFF}, {1, 0xFF}, {1000, 0xCE}, {-1000, 0x4E}, {32767, 0x80}, {-32768, 0x00}};
    for (const auto& v : ev) if (enc(v.pcm) != v.code) ++r.vector_failures;
    const struct { uint8_t code; int16_t pcm; } dv[] = {{0xFF, 0}, {0xCE, 988}, {0x4E, -988}, {0x80, 32124}, {0x00, -32124}};
    for (const auto& v : dv) if (dec(v.code) != v.pcm) ++r.vector_failures;

    for (int c = 0; c < 256; ++c) {
        if (c == 0x7F) continue;
        ++r.codes_checked;
        if (enc(dec(static_cast<uint8_t>(c))) != c) ++r.roundtrip_failures;
    }

    int prev = -1000000;
    for (int x = -32768; x <= 32767; ++x) {
        ++r.inputs_checked;
        const uint8_t code = enc(static_cast<int16_t>(x));
        const int y = dec(code);
        const int xc = std::max(-kMuClip, std::min(kMuClip, x));          // inputs beyond +-32635 are clipped first
        const int err = std::abs(xc - y);
        const int step = 8 << (((~code) >> 4) & 7);                          // 8, 16, ... 1024
        r.max_error = std::max(r.max_error, err);
        if (2 * err > step) ++r.error_violations;
        if (y < prev) ++r.monotonic_violations;
        prev = y;
    }
    for (int x = 1; x <= 32767; ++x)
        if (dec(enc(static_cast<int16_t>(-x))) != -dec(enc(static_cast<int16_t>(x)))) ++r.symmetry_violations;
    return r;
}

// ============================================================================================
// 3. RTP
// ============================================================================================
struct RtpFields {
    bool marker = false;
    uint8_t pt = 0;                 // 0 = PCMU
    uint16_t seq = 0;
    uint32_t ts = 0;
    uint32_t ssrc = 0;
};
struct RtpParsed {
    RtpFields f;
    Bytes payload;
};

// 12-byte fixed header (V=2, P=0, X=0, CC=0), big-endian fields, then the payload.
inline Bytes rtp_build(const RtpFields& f, const uint8_t* payload, size_t n) {
    Bytes b;
    b.reserve(12 + n);
    b.push_back(0x80);
    b.push_back(static_cast<uint8_t>((f.marker ? 0x80 : 0x00) | (f.pt & 0x7F)));
    b.push_back(static_cast<uint8_t>(f.seq >> 8)); b.push_back(static_cast<uint8_t>(f.seq & 0xFF));
    for (int i = 3; i >= 0; --i) b.push_back(static_cast<uint8_t>((f.ts >> (8 * i)) & 0xFF));
    for (int i = 3; i >= 0; --i) b.push_back(static_cast<uint8_t>((f.ssrc >> (8 * i)) & 0xFF));
    b.insert(b.end(), payload, payload + n);
    return b;
}

// Handles CSRC list, header extension and padding; rejects version != 2 and anything truncated.
inline std::optional<RtpParsed> rtp_parse(const Bytes& b) {
    if (b.size() < 12 || (b[0] >> 6) != 2) return std::nullopt;
    const size_t cc = b[0] & 0x0F;
    const bool pad = (b[0] & 0x20) != 0, ext = (b[0] & 0x10) != 0;
    size_t hdr = 12 + 4 * cc;
    if (hdr > b.size()) return std::nullopt;
    if (ext) {
        if (hdr + 4 > b.size()) return std::nullopt;
        hdr += 4 + 4 * static_cast<size_t>((b[hdr + 2] << 8) | b[hdr + 3]);
        if (hdr > b.size()) return std::nullopt;
    }
    size_t end = b.size();
    if (pad) {
        const size_t padlen = b[b.size() - 1];
        if (padlen == 0 || padlen > end - hdr) return std::nullopt;
        end -= padlen;
    }
    RtpParsed p;
    p.f.marker = (b[1] & 0x80) != 0;
    p.f.pt = b[1] & 0x7F;
    p.f.seq = static_cast<uint16_t>((b[2] << 8) | b[3]);
    p.f.ts = (static_cast<uint32_t>(b[4]) << 24) | (static_cast<uint32_t>(b[5]) << 16) | (static_cast<uint32_t>(b[6]) << 8) | b[7];
    p.f.ssrc = (static_cast<uint32_t>(b[8]) << 24) | (static_cast<uint32_t>(b[9]) << 16) | (static_cast<uint32_t>(b[10]) << 8) | b[11];
    p.payload.assign(b.begin() + static_cast<std::ptrdiff_t>(hdr), b.begin() + static_cast<std::ptrdiff_t>(end));
    return p;
}

// PCM16 -> mu-law -> RTP packets of ptime_samples bytes each (PCMU is 1 byte per sample, clock 8000).
class PcmuPacketiser {
public:
    PcmuPacketiser(uint32_t ssrc, uint16_t seq0, uint32_t ts0, size_t ptime_samples = 160)
        : ssrc_(ssrc), seq_(seq0), ts_(ts0), ptime_(ptime_samples) {}

    // Encode and buffer the samples; emit one packet for every complete frame. The marker bit is set on the first packet of a talkspurt.
    std::vector<Bytes> push(const std::vector<int16_t>& pcm) {
        for (int16_t s : pcm) pending_.push_back(mulaw_encode(s));
        std::vector<Bytes> out;
        while (pending_.size() >= ptime_) {
            out.push_back(emit(pending_.data()));
            pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(ptime_));
        }
        return out;
    }
    // Pad the final partial frame with mu-law silence (0xFF) and send it. Returns 0 or 1 packets.
    std::vector<Bytes> flush() {
        std::vector<Bytes> out;
        if (!pending_.empty()) {
            pending_.resize(ptime_, 0xFF);
            out.push_back(emit(pending_.data()));
            pending_.clear();
        }
        return out;
    }
    // The talker was silent for 'samples' (DTX): the timestamp jumps, the sequence number does NOT, the next packet carries the marker.
    // Call flush() first; any partial frame still pending is discarded.
    void skip_silence(uint32_t samples) { pending_.clear(); ts_ += samples; marker_next_ = true; }

    uint16_t next_seq() const { return seq_; }
    uint32_t next_ts() const { return ts_; }

private:
    Bytes emit(const uint8_t* payload) {
        RtpFields f;
        f.marker = marker_next_; marker_next_ = false;
        f.pt = 0; f.seq = seq_++; f.ts = ts_; f.ssrc = ssrc_;
        ts_ += static_cast<uint32_t>(ptime_);
        return rtp_build(f, payload, ptime_);
    }
    uint32_t ssrc_; uint16_t seq_; uint32_t ts_; size_t ptime_;
    bool marker_next_ = true;
    Bytes pending_;
};

// WAV (must be mono, 8 kHz, PCM16) -> list of RTP packets. nullopt for any other input.
inline std::optional<std::vector<Bytes>> wav_to_rtp(const Bytes& wav, uint32_t ssrc, uint16_t seq0, uint32_t ts0, size_t ptime_samples = 160) {
    auto w = wav_read_pcm16(wav);
    if (!w || w->channels != 1 || w->sample_rate != 8000) return std::nullopt;
    PcmuPacketiser p(ssrc, seq0, ts0, ptime_samples);
    std::vector<Bytes> out = p.push(w->samples);
    for (auto& x : p.flush()) out.push_back(std::move(x));
    return out;
}

// ============================================================================================
// 4. Network emulator + receiver
// ============================================================================================
struct NetworkModel {
    uint32_t loss_percent = 0;          // 0..100
    uint32_t min_delay_us = 0;
    uint32_t max_delay_us = 0;          // one-way delay uniform in [min, max]; a spread > packet spacing causes reordering
    uint32_t seed = 1;
};
struct Arrival {
    uint64_t arrival_us = 0;
    Bytes packet;
    size_t index = 0;                   // position in the sent list
};
struct NetworkResult {
    std::vector<Arrival> arrivals;      // sorted by (arrival_us, index)
    std::vector<size_t> dropped;        // sent indices that never arrive (ascending)
};

// Packet k is sent at first_send_us + k * spacing_us. For each packet in order: draw u (loss if u % 100 < loss_percent); if kept, draw a delay.
// LCG: s = s * 1664525 + 1013904223; u = s >> 8.
inline NetworkResult simulate_network(const std::vector<Bytes>& pkts, uint64_t first_send_us, uint64_t spacing_us, const NetworkModel& m) {
    NetworkResult r;
    uint32_t s = m.seed ? m.seed : 1;
    auto next = [&s]() { s = s * 1664525u + 1013904223u; return s >> 8; };
    const uint32_t spread = m.max_delay_us >= m.min_delay_us ? m.max_delay_us - m.min_delay_us + 1u : 1u;
    for (size_t k = 0; k < pkts.size(); ++k) {
        if (next() % 100 < m.loss_percent) { r.dropped.push_back(k); continue; }
        const uint64_t delay = m.min_delay_us + (next() % spread);
        r.arrivals.push_back({first_send_us + k * spacing_us + delay, pkts[k], k});
    }
    std::stable_sort(r.arrivals.begin(), r.arrivals.end(), [](const Arrival& a, const Arrival& b) {
        return a.arrival_us != b.arrival_us ? a.arrival_us < b.arrival_us : a.index < b.index; });
    return r;
}

struct ReceiverResult {
    std::vector<int16_t> pcm;                         // ticks * ptime_samples samples; Lost / Silence / Buffering slots are zeros
    std::vector<PlayoutResult::Kind> kinds;           // one per tick
    std::vector<uint32_t> slot_ts;                    // RTP timestamp of each slot (valid for Packet and Lost)
    JbStats stats;
    size_t bad_packets = 0;                           // arrivals that were not valid RTP
};

// Playout clock: tick t happens at first_tick_us + t * ptime_us. Before each tick, every arrival with arrival_us <= tick time is delivered to the
// jitter buffer (arrivals must be sorted). Packet slots are mu-law decoded; a payload of the wrong length is truncated / zero-padded.
inline ReceiverResult run_receiver(const std::vector<Arrival>& arrivals, const JbConfig& cfg, uint64_t first_tick_us, size_t ticks) {
    ReceiverResult r;
    JitterBuffer jb(cfg);
    const uint64_t ptime_us = static_cast<uint64_t>(cfg.ptime_samples) * 1000000ull / cfg.clock_hz;
    size_t next_arrival = 0;
    for (size_t t = 0; t < ticks; ++t) {
        const uint64_t now = first_tick_us + t * ptime_us;
        while (next_arrival < arrivals.size() && arrivals[next_arrival].arrival_us <= now) {
            const Arrival& a = arrivals[next_arrival++];
            auto p = rtp_parse(a.packet);
            if (!p) { ++r.bad_packets; continue; }
            RtpPacket pk;
            pk.seq = p->f.seq; pk.ts = p->f.ts; pk.marker = p->f.marker; pk.payload = std::move(p->payload);
            jb.push(pk, a.arrival_us);
        }
        PlayoutResult out = jb.pop();
        r.kinds.push_back(out.kind);
        r.slot_ts.push_back(out.ts);
        const size_t base = r.pcm.size();
        r.pcm.resize(base + cfg.ptime_samples, 0);
        if (out.kind == PlayoutResult::Kind::Packet)
            for (size_t i = 0; i < cfg.ptime_samples && i < out.payload.size(); ++i) r.pcm[base + i] = mulaw_decode(out.payload[i]);
    }
    r.stats = jb.stats();
    return r;
}

}  // namespace ether
