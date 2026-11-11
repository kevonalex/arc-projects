// SOLUTION - 04-audio/03-realtime-audio. Pure C++17, header-only, deterministic (no threads, no wall clock).
// Tests: tests/test_realtime_audio.cpp
//   - JitterBuffer: reorder, target depth, adapt, late-packet decision; driven by push(packet, arrival_us) and pop() once per ptime
//   - PLC (pitch-period repeat with fade), VAD / DTX / comfort noise, clock-drift resampling, saturating mixer, transcoder cost model
// Synthetic audio only.
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
    bool marker = false;        // first packet of a talkspurt (after silence)
    bool sid = false;           // comfort-noise (SID) packet: the sender is about to go silent
    Bytes payload;
};

struct JbConfig {
    uint32_t clock_hz = 8000;
    uint32_t ptime_samples = 160;     // RTP timestamp units per packet (20 ms at 8 kHz)
    uint32_t min_depth = 2;           // packets
    uint32_t max_depth = 20;
    uint32_t initial_depth = 3;
    uint32_t max_conceal = 10;        // consecutive underrun slots we conceal (PLC) before switching to silence
    uint32_t jitter_multiplier = 4;   // target depth = ceil(multiplier * jitter / ptime), clamped
};

struct PlayoutResult {
    enum class Kind { Buffering, Packet, Lost, Silence } kind = Kind::Buffering;
    uint32_t ts = 0;                  // RTP timestamp of the slot (extrapolated for Lost)
    Bytes payload;                    // for Packet
    bool sid = false;
};

struct JbStats {
    uint64_t received = 0, played = 0, lost = 0, concealed_underrun = 0, late = 0, duplicate = 0, reordered = 0,
             resyncs = 0, overflow = 0, silence_slots = 0;
};

class JitterBuffer {
public:
    explicit JitterBuffer(JbConfig c = {}) : cfg_(c), target_(clamp_depth(c.initial_depth)) {}

    // ---- input side: one call per received RTP packet. arrival_us = local monotonic receive time in microseconds.
    void push(const RtpPacket& p, uint64_t arrival_us) {
        ++stats_.received;

        // Sequence number: extend 16 -> 64 bit relative to the highest seen so far (handles wrap in both directions).
        int64_t ext;
        if (!have_seq_) { have_seq_ = true; highest_ = ext = p.seq; }
        else {
            int16_t d = static_cast<int16_t>(static_cast<uint16_t>(p.seq - static_cast<uint16_t>(highest_ & 0xFFFF)));
            ext = highest_ + d;
            if (d > kJumpLimit || d < -kJumpLimit) {                              // sender restart / SSRC change / huge gap
                have_transit_ = false;                                            // the timestamp base changed: don't count that as jitter
                update_jitter(p, arrival_us);
                resync(p);
                return;
            }
        }
        update_jitter(p, arrival_us);

        if (state_ != State::Prefill && state_ != State::Silence && ext < next_) { ++stats_.late; return; }   // missed its slot
        if (buf_.count(ext)) { ++stats_.duplicate; return; }
        if (ext < highest_) ++stats_.reordered;
        if (ext > highest_) highest_ = ext;
        buf_[ext] = p;

        if (state_ == State::Silence) { state_ = State::Prefill; }              // activity resumes: re-prefill with a fresh depth target
        // Overflow protection: a burst must not grow the delay without bound
        while (buf_.size() > cfg_.max_depth) {
            buf_.erase(buf_.begin());
            ++stats_.overflow;
            if (state_ == State::Playing && !buf_.empty() && buf_.begin()->first > next_) next_ = buf_.begin()->first;
        }
    }

    // ---- output side: call exactly once per packet time (e.g. every 20 ms) by the playout clock.
    PlayoutResult pop() {
        PlayoutResult r;
        if (state_ == State::Prefill) {
            if (buf_.size() >= effective_target()) {
                if (ever_started_) target_ = adaptive_target();                 // adapt at talkspurt boundaries only
                if (buf_.size() >= target_) { start_playing(); }
            }
            if (state_ == State::Prefill) {
                r.kind = ever_started_ ? PlayoutResult::Kind::Silence : PlayoutResult::Kind::Buffering;
                if (ever_started_) ++stats_.silence_slots;
                return r;
            }
        }
        if (state_ == State::Silence) { r.kind = PlayoutResult::Kind::Silence; ++stats_.silence_slots; return r; }

        // Playing
        auto it = buf_.find(next_);
        if (it != buf_.end()) {
            r.kind = PlayoutResult::Kind::Packet;
            r.ts = it->second.ts; r.payload = std::move(it->second.payload); r.sid = it->second.sid;
            last_ts_ = it->second.ts; have_ts_ = true;
            buf_.erase(it);
            ++next_; ++stats_.played; underrun_run_ = 0;
            if (r.sid) state_ = State::Silence;                                 // sender announced silence: stop concealing
            return r;
        }
        if (!buf_.empty()) {                                                     // a hole: later packets are here, this one is not
            r.kind = PlayoutResult::Kind::Lost;
            r.ts = last_ts_ + cfg_.ptime_samples;
            last_ts_ = r.ts;
            ++next_; ++stats_.lost; underrun_run_ = 0;
            return r;
        }
        // Underrun: nothing at all. Do NOT advance next_ (the packet may just be late); conceal for a while, then go silent.
        if (++underrun_run_ > cfg_.max_conceal) { state_ = State::Silence; r.kind = PlayoutResult::Kind::Silence; ++stats_.silence_slots; return r; }
        r.kind = PlayoutResult::Kind::Lost;
        r.ts = last_ts_ + cfg_.ptime_samples;
        last_ts_ = r.ts;
        ++stats_.concealed_underrun;
        return r;
    }

    size_t depth() const { return buf_.size(); }
    uint32_t target_depth() const { return target_; }
    // RFC 3550 interarrival jitter in milliseconds
    double jitter_ms() const { return jitter_units_ * 1000.0 / cfg_.clock_hz; }
    const JbStats& stats() const { return stats_; }
    bool playing() const { return state_ == State::Playing; }

private:
    enum class State { Prefill, Playing, Silence };
    static constexpr int kJumpLimit = 3000;       // ~60 s of 20 ms packets

    uint32_t clamp_depth(uint32_t d) const { return std::max(cfg_.min_depth, std::min(cfg_.max_depth, d)); }
    uint32_t effective_target() const { return ever_started_ ? cfg_.min_depth : target_; }
    uint32_t adaptive_target() const {
        double need = cfg_.jitter_multiplier * jitter_units_ / cfg_.ptime_samples;
        return clamp_depth(static_cast<uint32_t>(std::ceil(need)));
    }
    void start_playing() {
        state_ = State::Playing;
        ever_started_ = true;
        next_ = buf_.begin()->first;
        underrun_run_ = 0;
        if (have_ts_ == false) { last_ts_ = buf_.begin()->second.ts - cfg_.ptime_samples; }
    }
    void resync(const RtpPacket& p) {
        ++stats_.resyncs;
        buf_.clear();
        highest_ = p.seq;
        have_ts_ = false;
        state_ = State::Prefill;
        buf_[highest_] = p;
    }
    void update_jitter(const RtpPacket& p, uint64_t arrival_us) {
        // transit = arrival (in clock units) - rtp timestamp; jitter J += (|D| - J)/16   (RFC 3550 A.8)
        int64_t arrival_units = static_cast<int64_t>(arrival_us * cfg_.clock_hz / 1000000ull);
        int32_t transit = static_cast<int32_t>(static_cast<uint32_t>(arrival_units) - p.ts);
        if (have_transit_) {
            double d = std::fabs(static_cast<double>(transit - last_transit_));
            jitter_units_ += (d - jitter_units_) / 16.0;
        }
        have_transit_ = true; last_transit_ = transit;
    }

    JbConfig cfg_;
    uint32_t target_;
    State state_ = State::Prefill;
    bool ever_started_ = false, have_seq_ = false, have_ts_ = false, have_transit_ = false;
    int64_t highest_ = 0, next_ = 0;
    int32_t last_transit_ = 0;
    double jitter_units_ = 0;
    uint32_t last_ts_ = 0, underrun_run_ = 0;
    std::map<int64_t, RtpPacket> buf_;
    JbStats stats_;
};

// ============================================================================================
// Packet loss concealment: repeat the last pitch period, fading out
// ============================================================================================
// Normalised autocorrelation pitch search. Returns the first local maximum of the correlation (min_lag..max_lag) that is within 10 % of
// the best, or 0 if the signal is silent / not periodic (best correlation < 0.5).
inline size_t estimate_pitch_period(const std::vector<int16_t>& x, size_t min_lag = 20, size_t max_lag = 160) {
    const size_t n = x.size();
    if (min_lag == 0 || min_lag > max_lag || n < 3 * max_lag) return 0;   // need win + max_lag samples: x[i - lag] must exist
    const size_t win = max_lag * 2;                       // compare the last 'win' samples with themselves shifted back by 'lag'
    std::vector<double> r(max_lag + 1, 0.0);
    double best = 0;
    for (size_t lag = min_lag; lag <= max_lag; ++lag) {
        double xy = 0, xx = 0, yy = 0;
        for (size_t i = n - win; i < n; ++i) {
            double a = x[i], b = x[i - lag];
            xy += a * b; xx += a * a; yy += b * b;
        }
        r[lag] = (xx > 0 && yy > 0) ? xy / std::sqrt(xx * yy) : 0.0;
        best = std::max(best, r[lag]);
    }
    if (best < 0.5) return 0;
    // First LOCAL MAXIMUM that is within 10 % of the best: a smooth peak makes its neighbours qualify too, so "smallest lag >= 0.9*best"
    // would return a lag on the rising flank (38 instead of 40), and the best lag alone may be a multiple of the period.
    for (size_t lag = min_lag; lag <= max_lag; ++lag) {
        double left = lag > min_lag ? r[lag - 1] : -2.0, right = lag < max_lag ? r[lag + 1] : -2.0;
        if (r[lag] >= 0.9 * best && r[lag] >= left && r[lag] >= right) return lag;
    }
    return 0;
}
// Produce n samples to replace lost audio. 'history' = the most recent decoded samples (including earlier concealment on a burst).
// loss_index = 0 for the first lost frame, 1 for the second ... gain = max(0, 1 - 0.2*(loss_index+1)); silence if no pitch found.
inline std::vector<int16_t> conceal_pitch(const std::vector<int16_t>& history, size_t n, unsigned loss_index,
                                          size_t min_lag = 20, size_t max_lag = 160) {
    std::vector<int16_t> out(n, 0);
    size_t period = estimate_pitch_period(history, min_lag, max_lag);
    if (period == 0) return out;
    double gain = std::max(0.0, 1.0 - 0.2 * (loss_index + 1));
    const size_t base = history.size() - period;
    for (size_t i = 0; i < n; ++i) out[i] = static_cast<int16_t>(std::lround(history[base + (i % period)] * gain));
    return out;
}

// ============================================================================================
// VAD / DTX / comfort noise
// ============================================================================================
inline double frame_dbfs(const int16_t* x, size_t n) {
    if (n == 0) return -120.0;
    double e = 0;
    for (size_t i = 0; i < n; ++i) e += static_cast<double>(x[i]) * x[i];
    e /= static_cast<double>(n);
    if (e <= 0.0) return -120.0;
    return 10.0 * std::log10(e / (32768.0 * 32768.0));
}
class Vad {
public:
    explicit Vad(double threshold_dbfs = -45.0, unsigned hangover_frames = 5) : thr_(threshold_dbfs), hang_(hangover_frames) {}
    // true = speech (including the hangover after the last loud frame: avoids chopping word endings)
    bool process(const int16_t* x, size_t n) {
        if (frame_dbfs(x, n) > thr_) { left_ = hang_; return true; }
        if (left_ > 0) { --left_; return true; }
        return false;
    }
private:
    double thr_; unsigned hang_; unsigned left_ = 0;
};

enum class DtxAction { SendSpeech, SendSpeechMarker, SendSid, Nothing };
class DtxEncoder {
public:
    explicit DtxEncoder(Vad vad = Vad(), unsigned sid_interval_frames = 8) : vad_(vad), interval_(sid_interval_frames) {}
    // Called once per frame. First speech frame after silence carries the RTP marker bit. On entering silence ONE SID is sent,
    // then silence is signalled by sending nothing, with a SID refresh every 'interval' frames.
    DtxAction next(const int16_t* x, size_t n) {
        if (vad_.process(x, n)) {
            bool first = !in_speech_;
            in_speech_ = true; since_sid_ = 0; sid_sent_ = false;
            return first ? DtxAction::SendSpeechMarker : DtxAction::SendSpeech;
        }
        in_speech_ = false;
        if (!sid_sent_) { sid_sent_ = true; since_sid_ = 0; return DtxAction::SendSid; }
        if (++since_sid_ >= interval_) { since_sid_ = 0; return DtxAction::SendSid; }
        return DtxAction::Nothing;
    }
private:
    Vad vad_; unsigned interval_; bool in_speech_ = false, sid_sent_ = false; unsigned since_sid_ = 0;
};
// RFC 3389 noise level byte: -dBov, 0 (loudest) .. 127 (quietest). Full scale 16-bit ~ 0 dBov.
inline uint8_t cn_level_dbov(const int16_t* x, size_t n) {
    double d = -frame_dbfs(x, n);
    if (d < 0) d = 0;
    if (d > 127) d = 127;
    return static_cast<uint8_t>(std::lround(d));
}
// Deterministic uniform white noise with RMS = 32768 * 10^(-level/20)
inline std::vector<int16_t> comfort_noise(uint8_t level_dbov, size_t n, uint32_t seed = 1) {
    double rms = 32768.0 * std::pow(10.0, -static_cast<double>(level_dbov) / 20.0);
    double a = rms * std::sqrt(3.0);                      // uniform in [-a,a] has RMS a/sqrt(3)
    std::vector<int16_t> out(n);
    uint32_t s = seed ? seed : 1;
    for (size_t i = 0; i < n; ++i) {
        s = s * 1664525u + 1013904223u;                   // LCG (Numerical Recipes)
        double u = (static_cast<double>(s >> 8) / 8388608.0) - 1.0;       // 24 bits -> [-1, 1)
        double v = std::floor(u * a + 0.5);
        out[i] = static_cast<int16_t>(std::max(-32768.0, std::min(32767.0, v)));
    }
    return out;
}

// ============================================================================================
// Clock drift
// ============================================================================================
inline double drift_samples_per_second(double fs_hz, double ppm) { return fs_hz * ppm * 1e-6; }
inline double seconds_until_slip(double fs_hz, double ppm, double margin_samples) {
    double d = std::fabs(drift_samples_per_second(fs_hz, ppm));
    return d == 0.0 ? std::numeric_limits<double>::infinity() : margin_samples / d;
}
// sender faster than receiver  =>  jitter buffer depth grows  =>  positive ppm
inline double estimate_ppm(double depth_start_samples, double depth_end_samples, double seconds, double fs_hz) {
    return seconds <= 0 ? 0.0 : (depth_end_samples - depth_start_samples) / (fs_hz * seconds) * 1e6;
}
// ratio = out_rate / in_rate that makes a stream whose sender is 'ppm' too fast play at the nominal rate
inline double resample_ratio_for_ppm(double ppm) { return 1.0 / (1.0 + ppm * 1e-6); }

// Streaming linear-interpolation resampler (ratio = out/in, close to 1). State carries across calls, so chunked and
// one-shot processing give identical output. Ratio 1.0 reproduces the input exactly (delayed by one sample).
class LinearResampler {
public:
    explicit LinearResampler(double ratio = 1.0) : step_(1.0 / ratio) {}
    void set_ratio(double ratio) { step_ = 1.0 / ratio; }
    std::vector<int16_t> process(const int16_t* in, size_t n) {
        std::vector<int16_t> out;
        if (n == 0) return out;
        out.reserve(static_cast<size_t>(n / step_) + 2);
        const double nn = static_cast<double>(n);
        while (pos_ < nn) {
            size_t i0 = static_cast<size_t>(pos_);
            double frac = pos_ - static_cast<double>(i0);
            double s0 = (i0 == 0) ? prev_ : in[i0 - 1];
            double s1 = in[i0];
            out.push_back(static_cast<int16_t>(std::lround(s0 + (s1 - s0) * frac)));
            pos_ += step_;
        }
        pos_ -= nn;
        prev_ = in[n - 1];
        return out;
    }
    std::vector<int16_t> process(const std::vector<int16_t>& in) { return process(in.data(), in.size()); }
private:
    double step_;            // input samples consumed per output sample
    double pos_ = 1.0;       // position of the next output in input-sample units (x[-1] = prev_ is at 0, in[0] at 1)
    double prev_ = 0.0;
};

// ============================================================================================
// Mixer
// ============================================================================================
inline std::vector<int16_t> mix_saturating(const std::vector<std::vector<int16_t>>& streams) {
    size_t len = 0;
    for (const auto& s : streams) len = std::max(len, s.size());
    std::vector<int16_t> out(len);
    for (size_t i = 0; i < len; ++i) {
        int32_t acc = 0;
        for (const auto& s : streams) if (i < s.size()) acc += s[i];
        out[i] = static_cast<int16_t>(std::max<int32_t>(-32768, std::min<int32_t>(32767, acc)));
    }
    return out;
}
// THE BUG: accumulate into int16_t. Kept to demonstrate wrap-around distortion.
inline std::vector<int16_t> mix_wrapping(const std::vector<std::vector<int16_t>>& streams) {
    size_t len = 0;
    for (const auto& s : streams) len = std::max(len, s.size());
    std::vector<int16_t> out(len, 0);
    for (size_t i = 0; i < len; ++i)
        for (const auto& s : streams) if (i < s.size()) out[i] = static_cast<int16_t>(static_cast<uint16_t>(out[i]) + static_cast<uint16_t>(s[i]));
    return out;
}
// Conference "N-1" mix: what participant 'exclude' hears = everybody except themselves (no echo of own voice)
inline std::vector<int16_t> mix_minus(const std::vector<std::vector<int16_t>>& streams, size_t exclude) {
    std::vector<std::vector<int16_t>> others;
    for (size_t i = 0; i < streams.size(); ++i) if (i != exclude) others.push_back(streams[i]);
    return mix_saturating(others);
}
inline size_t count_clipped(const std::vector<int16_t>& v) {
    size_t c = 0;
    for (int16_t s : v) if (s == 32767 || s == -32768) ++c;
    return c;
}

// ============================================================================================
// Transcoder cost model
// ============================================================================================
// Cores needed = streams * MIPS per stream / (MIPS per core * usable fraction). A call transcodes in BOTH directions (2 streams).
inline double transcoder_cores(uint32_t calls, double mips_per_stream, double core_mips, double headroom = 0.7, uint32_t streams_per_call = 2) {
    return static_cast<double>(calls) * streams_per_call * mips_per_stream / (core_mips * headroom);
}
inline uint32_t max_calls_per_core(double mips_per_stream, double core_mips, double headroom = 0.7, uint32_t streams_per_call = 2) {
    return static_cast<uint32_t>(std::floor(core_mips * headroom / (mips_per_stream * streams_per_call)));
}
// Worst-case extra wait to re-frame PCM that arrives in chunks of 'a_ms' into encoder frames of 'b_ms': a - gcd(a, b)
inline uint32_t retime_worst_case_ms(uint32_t a_ms, uint32_t b_ms) {
    uint32_t x = a_ms, y = b_ms;
    while (y) { uint32_t t = x % y; x = y; y = t; }
    return a_ms - x;
}
inline double latency_budget_ms(const std::vector<double>& parts) { double s = 0; for (double p : parts) s += p; return s; }

}  // namespace ether
