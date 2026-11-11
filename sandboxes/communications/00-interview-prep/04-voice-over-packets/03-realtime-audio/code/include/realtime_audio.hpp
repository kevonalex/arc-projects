// STUB - 04-audio/03-realtime-audio. Implement every TODO. Tests: tests/test_realtime_audio.cpp
// Pure C++17, header-only, deterministic: no threads and no wall clock - time enters only as arguments.
// Read worksheet Part A (jitter buffer design) before touching JitterBuffer.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace ether {

using Bytes = std::vector<uint8_t>;

// ============================================================================================
// Jitter buffer
// ============================================================================================
struct RtpPacket {
    uint16_t seq = 0;
    uint32_t ts = 0;
    bool marker = false;        // first packet of a talkspurt
    bool sid = false;           // comfort-noise packet: the sender is about to go silent
    Bytes payload;
};

struct JbConfig {
    uint32_t clock_hz = 8000;
    uint32_t ptime_samples = 160;
    uint32_t min_depth = 2;           // packets
    uint32_t max_depth = 20;
    uint32_t initial_depth = 3;
    uint32_t max_conceal = 10;        // consecutive empty-buffer slots concealed before switching to silence
    uint32_t jitter_multiplier = 4;   // adaptive target depth = ceil(multiplier * jitter / ptime), clamped to [min,max]
};

struct PlayoutResult {
    enum class Kind { Buffering, Packet, Lost, Silence } kind = Kind::Buffering;
    uint32_t ts = 0;                  // RTP timestamp of the slot (extrapolate last_ts + ptime for Lost)
    Bytes payload;
    bool sid = false;
};

struct JbStats {
    uint64_t received = 0, played = 0, lost = 0, concealed_underrun = 0, late = 0, duplicate = 0, reordered = 0,
             resyncs = 0, overflow = 0, silence_slots = 0;
};

// TODO: implement. Design (see worksheet Part A):
//  * push(): count 'received'; update the RFC 3550 jitter estimate from (arrival in clock units - rtp ts);
//    extend the 16-bit seq to a 64-bit key relative to the highest seen (use the signed 16-bit difference; handles wrap);
//    a jump of more than ~3000 packets = stream restart: clear, count 'resyncs', start prefilling again;
//    a packet whose slot has already been played = 'late' (discard); a key already buffered = 'duplicate';
//    a key below the highest = 'reordered'; keep at most max_depth packets (drop the oldest, count 'overflow').
//  * pop() once per packet time: Buffering until initial_depth packets are queued; then play packets in sequence order.
//      - next packet present -> Kind::Packet (an SID packet switches to silence)
//      - a hole but LATER packets are buffered -> Kind::Lost, advance past the hole (count 'lost')
//      - buffer completely empty (underrun) -> Kind::Lost but DO NOT advance (it may only be late), count 'concealed_underrun';
//        after max_conceal consecutive underruns -> Silence until new packets arrive
//      - when activity resumes after silence: prefill again, using an ADAPTED target depth (jitter-based) - only at talkspurt starts
class JitterBuffer {
public:
    explicit JitterBuffer(JbConfig c = {}) : cfg_(c), target_(c.initial_depth) {}
    void push(const RtpPacket& p, uint64_t arrival_us) { (void)p; (void)arrival_us; }
    PlayoutResult pop() { return PlayoutResult{}; }
    size_t depth() const { return 0; }
    uint32_t target_depth() const { return 0; }
    double jitter_ms() const { return -1.0; }            // RFC 3550 interarrival jitter converted to milliseconds
    const JbStats& stats() const { return stats_; }
    bool playing() const { return false; }
private:
    JbConfig cfg_;
    uint32_t target_;
    JbStats stats_;
    // TODO: add your state here (std::map<int64_t, RtpPacket> is a good container: ordered by extended sequence number)
};

// ============================================================================================
// Packet loss concealment
// ============================================================================================
// TODO: normalised autocorrelation r(lag) = sum x[i]*x[i-lag] / sqrt(sum x[i]^2 * sum x[i-lag]^2) over the last 2*max_lag samples, for lags
// min_lag..max_lag; best r < 0.5 -> 0 (not periodic/silent); otherwise the FIRST LOCAL MAXIMUM with r >= 0.9 * best (not the best lag: it may be
// a multiple of the period; not the first lag over the threshold: that is on the rising flank, e.g. 38 instead of 40).
// Needs x[i-lag] to exist: return 0 unless x.size() >= 3*max_lag.
inline size_t estimate_pitch_period(const std::vector<int16_t>& x, size_t min_lag = 20, size_t max_lag = 160) { (void)x; (void)min_lag; (void)max_lag; return 999; }
// TODO: n samples = last pitch period of 'history' repeated, times gain = max(0, 1 - 0.2*(loss_index+1)); zeros if no pitch found.
inline std::vector<int16_t> conceal_pitch(const std::vector<int16_t>& history, size_t n, unsigned loss_index, size_t min_lag = 20, size_t max_lag = 160) {
    (void)history; (void)loss_index; (void)min_lag; (void)max_lag; return std::vector<int16_t>(n, 12345);
}

// ============================================================================================
// VAD / DTX / comfort noise
// ============================================================================================
// TODO: 10*log10(mean(x^2) / 32768^2); -120 for empty or all-zero input
inline double frame_dbfs(const int16_t* x, size_t n) { (void)x; (void)n; return 0.0; }
class Vad {
public:
    explicit Vad(double threshold_dbfs = -45.0, unsigned hangover_frames = 5) : thr_(threshold_dbfs), hang_(hangover_frames) {}
    // TODO: frame louder than the threshold -> speech and reload the hangover counter; otherwise keep answering "speech"
    // for 'hangover' more frames, then false.
    bool process(const int16_t* x, size_t n) { (void)x; (void)n; return false; }
private:
    double thr_; unsigned hang_; unsigned left_ = 0;
};

enum class DtxAction { SendSpeech, SendSpeechMarker, SendSid, Nothing };
class DtxEncoder {
public:
    explicit DtxEncoder(Vad vad = Vad(), unsigned sid_interval_frames = 8) : vad_(vad), interval_(sid_interval_frames) {}
    // TODO per frame: speech -> SendSpeechMarker for the first speech frame after silence, SendSpeech afterwards.
    // Silence: ONE SendSid when entering silence, then Nothing, with a SendSid refresh every 'interval' silent frames.
    DtxAction next(const int16_t* x, size_t n) { (void)x; (void)n; return DtxAction::Nothing; }
private:
    Vad vad_; unsigned interval_;
};
// TODO: RFC 3389 level byte = -dBov rounded, clamped to 0..127 (0 = loudest)
inline uint8_t cn_level_dbov(const int16_t* x, size_t n) { (void)x; (void)n; return 255; }
// TODO: deterministic white noise (LCG: s = s*1664525 + 1013904223) with RMS = 32768 * 10^(-level/20); uniform in [-a, a] has RMS a/sqrt(3)
inline std::vector<int16_t> comfort_noise(uint8_t level_dbov, size_t n, uint32_t seed = 1) { (void)level_dbov; (void)seed; return std::vector<int16_t>(n, 0); }

// ============================================================================================
// Clock drift
// ============================================================================================
inline double drift_samples_per_second(double fs_hz, double ppm) { (void)fs_hz; (void)ppm; return -1.0; }              // TODO fs * ppm * 1e-6
// TODO: margin / |drift|; +infinity if there is no drift
inline double seconds_until_slip(double fs_hz, double ppm, double margin_samples) { (void)fs_hz; (void)ppm; (void)margin_samples; return -1.0; }
// TODO: (depth_end - depth_start) / (fs * seconds) * 1e6; positive = sender clock faster than ours (buffer grows)
inline double estimate_ppm(double depth_start_samples, double depth_end_samples, double seconds, double fs_hz) {
    (void)depth_start_samples; (void)depth_end_samples; (void)seconds; (void)fs_hz; return -1.0;
}
// TODO: out/in ratio that plays a too-fast sender at nominal speed: 1 / (1 + ppm*1e-6)
inline double resample_ratio_for_ppm(double ppm) { (void)ppm; return 0.0; }

// TODO: streaming linear-interpolation resampler, ratio = out/in. Keep a fractional read position and the last input sample between
// calls so that chunked processing equals one-shot processing. Hint: position of the next output measured in input samples,
// with the previous block's last sample at 0 and in[0] at 1; emit while pos < n; then pos -= n.
// With ratio 1.0 the output must equal the input (shifted by one sample: the first call returns n-1 samples).
class LinearResampler {
public:
    explicit LinearResampler(double ratio = 1.0) : step_(ratio == 0 ? 1.0 : 1.0 / ratio) {}
    void set_ratio(double ratio) { step_ = 1.0 / ratio; }
    std::vector<int16_t> process(const int16_t* in, size_t n) { (void)in; (void)n; return {}; }
    std::vector<int16_t> process(const std::vector<int16_t>& in) { return process(in.data(), in.size()); }
private:
    double step_;
};

// ============================================================================================
// Mixer
// ============================================================================================
// TODO: sum in int32 and CLAMP to int16; shorter streams count as zeros; output length = longest input
inline std::vector<int16_t> mix_saturating(const std::vector<std::vector<int16_t>>& streams) { (void)streams; return {}; }
// Given: the classic bug (wraps). Used by a test to show the damage.
inline std::vector<int16_t> mix_wrapping(const std::vector<std::vector<int16_t>>& streams) {
    size_t len = 0;
    for (const auto& s : streams) len = std::max(len, s.size());
    std::vector<int16_t> out(len, 0);
    for (size_t i = 0; i < len; ++i)
        for (const auto& s : streams) if (i < s.size()) out[i] = static_cast<int16_t>(static_cast<uint16_t>(out[i]) + static_cast<uint16_t>(s[i]));
    return out;
}
// TODO: what participant 'exclude' hears: saturating mix of everyone else
inline std::vector<int16_t> mix_minus(const std::vector<std::vector<int16_t>>& streams, size_t exclude) { (void)streams; (void)exclude; return {1}; }
// TODO: number of samples at +32767 or -32768
inline size_t count_clipped(const std::vector<int16_t>& v) { (void)v; return 999; }

// ============================================================================================
// Transcoder cost model
// ============================================================================================
// TODO: calls * streams_per_call * mips_per_stream / (core_mips * headroom)
inline double transcoder_cores(uint32_t calls, double mips_per_stream, double core_mips, double headroom = 0.7, uint32_t streams_per_call = 2) {
    (void)calls; (void)mips_per_stream; (void)core_mips; (void)headroom; (void)streams_per_call; return -1.0;
}
// TODO: floor(core_mips * headroom / (mips_per_stream * streams_per_call))
inline uint32_t max_calls_per_core(double mips_per_stream, double core_mips, double headroom = 0.7, uint32_t streams_per_call = 2) {
    (void)mips_per_stream; (void)core_mips; (void)headroom; (void)streams_per_call; return 999999;
}
// TODO: PCM arrives in chunks of a_ms and is consumed in frames of b_ms: worst-case extra wait = a - gcd(a, b)
inline uint32_t retime_worst_case_ms(uint32_t a_ms, uint32_t b_ms) { (void)a_ms; (void)b_ms; return 999; }
// TODO: sum
inline double latency_budget_ms(const std::vector<double>& parts) { (void)parts; return -1.0; }

}  // namespace ether
