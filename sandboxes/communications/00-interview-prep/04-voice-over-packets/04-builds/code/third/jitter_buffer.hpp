// REFERENCE COPY of the JitterBuffer from 04-audio/03-realtime-audio (solutions/realtime_audio.hpp, lines up to the PLC section).
// Used here as a given component of the pipeline; its behaviour is specified and tested in pack 03. Do not edit.
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


}  // namespace ether
