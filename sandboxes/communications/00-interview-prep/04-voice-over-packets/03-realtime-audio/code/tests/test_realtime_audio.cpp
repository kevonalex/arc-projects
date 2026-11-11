// Tests for 04-audio/03-realtime-audio. Same file builds against the student stubs and the solutions.
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <vector>
#include "realtime_audio.hpp"

using namespace ether;
using Kind = PlayoutResult::Kind;

// ---------------------------------------------------------------- helpers
static RtpPacket pk(uint16_t seq, uint32_t ts, bool marker = false, bool sid = false) {
    RtpPacket p; p.seq = seq; p.ts = ts; p.marker = marker; p.sid = sid; p.payload = {static_cast<uint8_t>(seq & 0xFF)};
    return p;
}
static uint32_t T(int k) { return 16000u + 160u * static_cast<uint32_t>(k); }       // ts of "packet k" (seq 100+k in the simple tests)
static uint64_t ideal_arrival(int k) { return 1000000ull + 20000ull * static_cast<uint64_t>(k); }
static std::vector<int16_t> sine(double f, size_t n, double amp = 8000.0, double fs = 8000.0, size_t start = 0) {
    std::vector<int16_t> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = static_cast<int16_t>(std::lround(amp * std::sin(6.283185307179586 * f * (start + i) / fs)));
    return v;
}
static uint32_t lcg(uint32_t& s) { s = s * 1664525u + 1013904223u; return s >> 8; }

// Event-driven simulation: packets arrive at their arrival times, the playout clock pops every 20 ms from tick0.
struct Arrival { uint64_t t_us; RtpPacket pkt; };
static std::vector<PlayoutResult> simulate(JitterBuffer& jb, std::vector<Arrival> arr, uint64_t tick0_us, size_t ticks, uint64_t tick_us = 20000) {
    std::stable_sort(arr.begin(), arr.end(), [](const Arrival& a, const Arrival& b) { return a.t_us < b.t_us; });
    std::vector<PlayoutResult> out;
    size_t i = 0;
    for (size_t k = 0; k < ticks; ++k) {
        uint64_t now = tick0_us + k * tick_us;
        while (i < arr.size() && arr[i].t_us <= now) { jb.push(arr[i].pkt, arr[i].t_us); ++i; }
        out.push_back(jb.pop());
    }
    return out;
}

// ============================================================ jitter buffer
TEST(JitterBuffer, PrefillsThenPlaysInOrder) {
    JitterBuffer jb;                                           // initial depth 3
    jb.push(pk(100, T(0)), ideal_arrival(0));
    EXPECT_EQ(jb.pop().kind, Kind::Buffering);
    jb.push(pk(101, T(1)), ideal_arrival(1));
    EXPECT_EQ(jb.pop().kind, Kind::Buffering);                 // 2 < 3: still prefilling
    jb.push(pk(102, T(2)), ideal_arrival(2));
    auto r = jb.pop();
    ASSERT_EQ(r.kind, Kind::Packet);
    EXPECT_EQ(r.ts, T(0)); ASSERT_EQ(r.payload.size(), 1u); EXPECT_EQ(r.payload[0], 100);
    jb.push(pk(103, T(3)), ideal_arrival(3));
    EXPECT_EQ(jb.pop().ts, T(1));
    EXPECT_EQ(jb.pop().ts, T(2));
    EXPECT_EQ(jb.pop().ts, T(3));
    EXPECT_EQ(jb.stats().played, 4u); EXPECT_EQ(jb.stats().lost, 0u);
    EXPECT_TRUE(jb.playing());
    auto u = jb.pop();                                         // nothing left: underrun -> conceal
    EXPECT_EQ(u.kind, Kind::Lost);
    EXPECT_EQ(u.ts, T(4));                                     // extrapolated timestamp
    EXPECT_EQ(jb.stats().concealed_underrun, 1u);
}
TEST(JitterBuffer, ReorderingWithinDepthIsRepaired) {
    JitterBuffer jb;
    jb.push(pk(100, T(0)), ideal_arrival(0));
    jb.push(pk(102, T(2)), ideal_arrival(1));
    jb.push(pk(101, T(1)), ideal_arrival(2));
    EXPECT_EQ(jb.stats().reordered, 1u);
    EXPECT_EQ(jb.pop().ts, T(0));
    EXPECT_EQ(jb.pop().ts, T(1));
    EXPECT_EQ(jb.pop().ts, T(2));
    EXPECT_EQ(jb.stats().lost, 0u);
}
TEST(JitterBuffer, LostPacketIsConcealedAtItsOwnSlot) {
    JitterBuffer jb;
    jb.push(pk(100, T(0)), ideal_arrival(0));
    jb.push(pk(101, T(1)), ideal_arrival(1));
    jb.push(pk(103, T(3)), ideal_arrival(3));                  // 102 never arrives
    EXPECT_EQ(jb.pop().ts, T(0));
    EXPECT_EQ(jb.pop().ts, T(1));
    auto hole = jb.pop();
    EXPECT_EQ(hole.kind, Kind::Lost);                          // later packet present -> this slot is a real loss
    EXPECT_EQ(hole.ts, T(2));
    auto r = jb.pop();
    ASSERT_EQ(r.kind, Kind::Packet);
    EXPECT_EQ(r.ts, T(3));
    EXPECT_EQ(jb.stats().lost, 1u); EXPECT_EQ(jb.stats().played, 3u);
    jb.push(pk(102, T(2)), ideal_arrival(5));                  // it finally shows up: too late, its slot is gone
    EXPECT_EQ(jb.stats().late, 1u);
    EXPECT_EQ(jb.depth(), 0u);
}
TEST(JitterBuffer, DuplicatesAreDropped) {
    JitterBuffer jb;
    jb.push(pk(100, T(0)), ideal_arrival(0));
    jb.push(pk(100, T(0)), ideal_arrival(0) + 500);
    EXPECT_EQ(jb.depth(), 1u);
    EXPECT_EQ(jb.stats().duplicate, 1u);
    EXPECT_EQ(jb.stats().received, 2u);
}
TEST(JitterBuffer, SequenceWrapAround) {
    JitterBuffer jb;
    jb.push(pk(65534, 1000), ideal_arrival(0));
    jb.push(pk(65535, 1160), ideal_arrival(1));
    jb.push(pk(0, 1320), ideal_arrival(2));
    jb.push(pk(1, 1480), ideal_arrival(3));
    EXPECT_EQ(jb.pop().ts, 1000u);
    EXPECT_EQ(jb.pop().ts, 1160u);
    EXPECT_EQ(jb.pop().ts, 1320u);
    EXPECT_EQ(jb.pop().ts, 1480u);
    EXPECT_EQ(jb.stats().lost, 0u);
    // and reordering ACROSS the wrap
    JitterBuffer jb2;
    jb2.push(pk(65535, 1160), ideal_arrival(1));
    jb2.push(pk(0, 1320), ideal_arrival(2));
    jb2.push(pk(65534, 1000), ideal_arrival(0));               // older, from before the wrap
    EXPECT_EQ(jb2.stats().reordered, 1u);
    EXPECT_EQ(jb2.pop().ts, 1000u);
    EXPECT_EQ(jb2.pop().ts, 1160u);
    EXPECT_EQ(jb2.pop().ts, 1320u);
}
TEST(JitterBuffer, TimestampWrapDoesNotConfuseJitter) {
    JitterBuffer jb;
    for (int k = 0; k < 50; ++k) jb.push(pk(static_cast<uint16_t>(k), 0xFFFFFF00u + 160u * k), 20000ull * k);   // ts crosses 2^32
    EXPECT_LT(jb.jitter_ms(), 0.1);
}
TEST(JitterBuffer, UnderrunConcealsThenGoesSilentWithoutAdvancing) {
    JbConfig c; c.initial_depth = 2; c.max_conceal = 3;
    JitterBuffer jb(c);
    jb.push(pk(10, T(0)), ideal_arrival(0));
    jb.push(pk(11, T(1)), ideal_arrival(1));
    EXPECT_EQ(jb.pop().kind, Kind::Packet);
    EXPECT_EQ(jb.pop().kind, Kind::Packet);
    EXPECT_EQ(jb.pop().kind, Kind::Lost);                      // 1
    EXPECT_EQ(jb.pop().kind, Kind::Lost);                      // 2
    EXPECT_EQ(jb.pop().kind, Kind::Lost);                      // 3 = max_conceal
    EXPECT_EQ(jb.pop().kind, Kind::Silence);                   // PLC for more than ~60 ms sounds robotic: stop
    EXPECT_EQ(jb.pop().kind, Kind::Silence);
    EXPECT_EQ(jb.stats().concealed_underrun, 3u);
    EXPECT_EQ(jb.stats().lost, 0u);                            // nothing was a hole: no later packet existed
    // Sender resumes with the packet we were waiting for (the stall was on the network side).
    jb.push(pk(12, T(2)), ideal_arrival(8));
    jb.push(pk(13, T(3)), ideal_arrival(9));
    auto r = jb.pop();
    ASSERT_EQ(r.kind, Kind::Packet);
    EXPECT_EQ(r.ts, T(2));                                     // not skipped: the buffer did not run ahead during the stall
    EXPECT_EQ(jb.pop().ts, T(3));
}
TEST(JitterBuffer, SidSwitchesToSilenceNotConcealment) {
    JbConfig c; c.initial_depth = 2;
    JitterBuffer jb(c);
    jb.push(pk(1, T(0)), ideal_arrival(0));
    jb.push(pk(2, T(1), false, true), ideal_arrival(1));       // SID: "I'm going quiet"
    EXPECT_EQ(jb.pop().kind, Kind::Packet);
    auto sid = jb.pop();
    ASSERT_EQ(sid.kind, Kind::Packet);
    EXPECT_TRUE(sid.sid);
    for (int i = 0; i < 20; ++i) EXPECT_EQ(jb.pop().kind, Kind::Silence);        // silence = comfort noise, NOT packet loss
    EXPECT_EQ(jb.stats().lost, 0u);
    EXPECT_EQ(jb.stats().concealed_underrun, 0u);
    EXPECT_EQ(jb.stats().silence_slots, 20u);
    // Talkspurt resumes: RTP timestamp jumps by the silence duration (1 s = 8000 units) but the sequence number continues.
    jb.push(pk(3, T(1) + 8000, true), ideal_arrival(51));
    EXPECT_EQ(jb.pop().kind, Kind::Silence);                   // prefilling: 1 packet < min depth
    jb.push(pk(4, T(1) + 8000 + 160), ideal_arrival(52));
    auto r = jb.pop();
    ASSERT_EQ(r.kind, Kind::Packet);
    EXPECT_EQ(r.ts, T(1) + 8000);
    EXPECT_EQ(jb.stats().lost, 0u);                            // a timestamp jump with consecutive sequence numbers is silence, not loss
}
TEST(JitterBuffer, OverflowDropsOldest) {
    JbConfig c; c.max_depth = 8;
    JitterBuffer jb(c);
    for (int k = 0; k < 50; ++k) jb.push(pk(static_cast<uint16_t>(100 + k), T(k)), ideal_arrival(0));      // a 1-second burst
    EXPECT_EQ(jb.depth(), 8u);
    EXPECT_EQ(jb.stats().overflow, 42u);
    auto r = jb.pop();
    ASSERT_EQ(r.kind, Kind::Packet);
    EXPECT_EQ(r.ts, T(42));                                    // oldest were discarded; we play the freshest 8
}
TEST(JitterBuffer, HugeSequenceJumpResynchronises) {
    JitterBuffer jb;
    jb.push(pk(100, T(0)), ideal_arrival(0));
    jb.push(pk(101, T(1)), ideal_arrival(1));
    jb.push(pk(102, T(2)), ideal_arrival(2));
    EXPECT_EQ(jb.pop().kind, Kind::Packet);
    jb.push(pk(40000, 5000000), ideal_arrival(3));             // sender restarted / new SSRC
    EXPECT_EQ(jb.stats().resyncs, 1u);
    EXPECT_EQ(jb.depth(), 1u);
    EXPECT_FALSE(jb.playing());
    jb.push(pk(40001, 5000160), ideal_arrival(4));
    jb.push(pk(40002, 5000320), ideal_arrival(5));
    auto r = jb.pop();
    ASSERT_EQ(r.kind, Kind::Packet);
    EXPECT_EQ(r.ts, 5000000u);
    EXPECT_EQ(jb.stats().lost, 0u);
}
TEST(JitterBuffer, JitterEstimateMatchesRfc3550) {
    // Packets 20 ms apart; every other packet arrives 5 ms late: |D| alternates 5 ms (= 40 units) -> J converges towards 5 ms.
    JitterBuffer jb;
    for (int k = 0; k < 400; ++k) {
        uint64_t late = (k % 2) ? 5000 : 0;
        jb.push(pk(static_cast<uint16_t>(k), 160u * k), 20000ull * k + late);
    }
    EXPECT_NEAR(jb.jitter_ms(), 5.0, 0.3);
    JitterBuffer calm;
    for (int k = 0; k < 400; ++k) calm.push(pk(static_cast<uint16_t>(k), 160u * k), 20000ull * k);
    EXPECT_NEAR(calm.jitter_ms(), 0.0, 1e-9);
}
TEST(JitterBuffer, TargetDepthAdaptsAtTalkspurtBoundaries) {
    JbConfig c; c.initial_depth = 3; c.min_depth = 2; c.max_depth = 30;
    JitterBuffer jb(c);
    uint32_t rs = 12345;
    // Talkspurt 1: send times k*20 ms, network delay uniform 0..120 ms (heavy jitter).
    std::vector<Arrival> a;
    for (int k = 0; k < 300; ++k) a.push_back({20000ull * k + (lcg(rs) % 120000ull), pk(static_cast<uint16_t>(k), 160u * k)});
    simulate(jb, a, 300000, 320);
    EXPECT_GT(jb.jitter_ms(), 20.0);
    // The talkspurt ends with a SID; the buffer drains to silence.
    jb.push(pk(300, 160u * 300, false, true), 20000ull * 300 + 5000);
    for (int i = 0; i < 40; ++i) jb.pop();
    // A new talkspurt starts: prefill happens with the jitter-based target.
    jb.push(pk(301, 160u * 400, true), 20000ull * 400);
    jb.push(pk(302, 160u * 401), 20000ull * 401);
    jb.pop();
    EXPECT_GE(jb.target_depth(), 6u);
    EXPECT_LE(jb.target_depth(), 30u);
    // Later the network calms down completely: 400 packets with constant delay. Jitter decays, depth returns to the minimum.
    uint16_t seq = 303; uint64_t t0 = 20000ull * 410;
    std::vector<Arrival> calm;
    for (int k = 0; k < 400; ++k) calm.push_back({t0 + 20000ull * k, pk(static_cast<uint16_t>(seq + k), static_cast<uint32_t>(160u * 410 + 160u * k))});
    simulate(jb, calm, t0 + 100000, 420);
    jb.push(pk(static_cast<uint16_t>(seq + 400), 160u * (410 + 400), false, true), t0 + 20000ull * 400);
    for (int i = 0; i < 40; ++i) jb.pop();
    jb.push(pk(static_cast<uint16_t>(seq + 401), 160u * (410 + 500), true), t0 + 20000ull * 500);
    jb.push(pk(static_cast<uint16_t>(seq + 402), 160u * (410 + 501)), t0 + 20000ull * 501);
    jb.pop();
    EXPECT_LT(jb.jitter_ms(), 1.0);
    EXPECT_EQ(jb.target_depth(), 2u);
}
TEST(JitterBuffer, ShuffledAndDroppedStreamEndToEnd) {
    JbConfig c; c.initial_depth = 4;
    JitterBuffer jb(c);
    uint32_t rs = 777;
    std::vector<Arrival> a;
    std::set<int> dropped;
    for (int k = 0; k < 500; ++k) {
        if (lcg(rs) % 100 < 5) { dropped.insert(k); continue; }                       // 5 % network loss
        uint64_t delay = 5000 + (lcg(rs) % 35000);                                    // 5..40 ms: packets overtake each other
        a.push_back({20000ull * k + delay, pk(static_cast<uint16_t>(k), 160u * k)});
    }
    auto out = simulate(jb, a, 140000, 520);
    std::vector<uint32_t> played_ts;
    size_t lost = 0;
    for (const auto& r : out) {
        if (r.kind == Kind::Packet) played_ts.push_back(r.ts);
        if (r.kind == Kind::Lost) ++lost;
    }
    EXPECT_TRUE(std::is_sorted(played_ts.begin(), played_ts.end()));                  // playout is strictly in order...
    EXPECT_TRUE(std::adjacent_find(played_ts.begin(), played_ts.end()) == played_ts.end());   // ...and has no duplicates
    EXPECT_GT(jb.stats().reordered, 20u);                                             // the network really did shuffle
    EXPECT_EQ(jb.stats().duplicate, 0u);
    EXPECT_GE(lost, dropped.size() - 1);                                              // every dropped packet got a concealed slot
    EXPECT_LE(jb.stats().late, 10u);                                                  // depth 80 ms covers delays up to 40 ms
    EXPECT_GE(played_ts.size() + jb.stats().late + jb.depth(), a.size() - 3);         // everything delivered was played or accounted for
}

// ============================================================ PLC
TEST(Plc, PitchEstimate) {
    EXPECT_EQ(estimate_pitch_period(sine(200.0, 1000)), 40u);      // 8000/200
    EXPECT_EQ(estimate_pitch_period(sine(125.0, 1000)), 64u);
    EXPECT_EQ(estimate_pitch_period(sine(100.0, 1000)), 80u);
    EXPECT_EQ(estimate_pitch_period(std::vector<int16_t>(1000, 0)), 0u);               // silence
    std::vector<int16_t> noise(1000); uint32_t s = 99;
    for (auto& v : noise) v = static_cast<int16_t>(static_cast<int>(lcg(s) % 20000) - 10000);
    EXPECT_EQ(estimate_pitch_period(noise), 0u);                   // not periodic
    EXPECT_EQ(estimate_pitch_period(sine(200.0, 100)), 0u);        // too little history
    EXPECT_EQ(estimate_pitch_period(sine(200.0, 479)), 0u);        // needs 3*max_lag = 480 samples (x[i-lag] must exist: no out-of-bounds read)
    EXPECT_EQ(estimate_pitch_period(sine(200.0, 480)), 40u);
}
TEST(Plc, ConcealmentContinuesTheWaveformAndFades) {
    auto hist = sine(200.0, 1000);
    auto truth = sine(200.0, 160, 8000.0, 8000.0, 1000);
    auto c0 = conceal_pitch(hist, 160, 0);
    ASSERT_EQ(c0.size(), 160u);
    for (size_t i = 0; i < 160; ++i) EXPECT_NEAR(c0[i], 0.8 * truth[i], 3.0) << i;      // phase-coherent continuation at 80 % gain
    auto c1 = conceal_pitch(hist, 160, 1);
    for (size_t i = 0; i < 160; ++i) EXPECT_NEAR(c1[i], 0.6 * truth[i], 3.0) << i;
    auto c4 = conceal_pitch(hist, 160, 4);
    EXPECT_EQ(*std::max_element(c4.begin(), c4.end()), 0);         // 1 - 0.2*5 = 0: silent after five consecutive losses
    EXPECT_EQ(*std::min_element(c4.begin(), c4.end()), 0);
    EXPECT_EQ(conceal_pitch(hist, 7, 0).size(), 7u);
    auto none = conceal_pitch(std::vector<int16_t>(1000, 0), 40, 0);
    EXPECT_EQ(none, std::vector<int16_t>(40, 0));
}

// ============================================================ VAD / DTX / CN
TEST(Vad, FrameLevel) {
    std::vector<int16_t> z(160, 0);
    EXPECT_DOUBLE_EQ(frame_dbfs(z.data(), z.size()), -120.0);
    EXPECT_DOUBLE_EQ(frame_dbfs(z.data(), 0), -120.0);
    auto s = sine(1000.0, 8000, 16384.0);                          // amplitude 0.5 -> RMS 0.3536 -> -9.03 dBFS
    EXPECT_NEAR(frame_dbfs(s.data(), s.size()), -9.03, 0.05);
    std::vector<int16_t> sq(160, 32767);
    EXPECT_NEAR(frame_dbfs(sq.data(), sq.size()), 0.0, 0.001);
}
TEST(Vad, ThresholdAndHangover) {
    Vad v(-45.0, 3);
    std::vector<int16_t> quiet(160, 0), loud = sine(1000.0, 160, 3000.0);
    EXPECT_FALSE(v.process(quiet.data(), quiet.size()));
    EXPECT_TRUE(v.process(loud.data(), loud.size()));
    EXPECT_TRUE(v.process(quiet.data(), quiet.size()));            // hangover 1
    EXPECT_TRUE(v.process(quiet.data(), quiet.size()));            // 2
    EXPECT_TRUE(v.process(quiet.data(), quiet.size()));            // 3
    EXPECT_FALSE(v.process(quiet.data(), quiet.size()));           // hangover exhausted
    EXPECT_TRUE(v.process(loud.data(), loud.size()));              // and it re-arms
    auto n50 = comfort_noise(50, 160), n40 = comfort_noise(40, 160);
    Vad w(-45.0, 0);
    EXPECT_FALSE(w.process(n50.data(), n50.size()));
    EXPECT_TRUE(w.process(n40.data(), n40.size()));
}
TEST(Dtx, ActionSequence) {
    DtxEncoder dtx(Vad(-45.0, 2), 4);
    std::vector<int16_t> loud = sine(1000.0, 160, 3000.0), quiet(160, 0);
    std::vector<DtxAction> got;
    for (int i = 0; i < 3; ++i) got.push_back(dtx.next(loud.data(), loud.size()));
    for (int i = 0; i < 2 + 9; ++i) got.push_back(dtx.next(quiet.data(), quiet.size()));
    for (int i = 0; i < 2; ++i) got.push_back(dtx.next(loud.data(), loud.size()));
    using A = DtxAction;
    std::vector<DtxAction> want = {
        A::SendSpeechMarker, A::SendSpeech, A::SendSpeech,                       // talkspurt: marker on the first frame only
        A::SendSpeech, A::SendSpeech,                                            // 2 hangover frames are still sent as speech
        A::SendSid, A::Nothing, A::Nothing, A::Nothing, A::SendSid, A::Nothing, A::Nothing, A::Nothing, A::SendSid,   // SID, then refresh every 4
        A::SendSpeechMarker, A::SendSpeech};                                     // new talkspurt: marker again
    ASSERT_EQ(got.size(), want.size());
    for (size_t i = 0; i < want.size(); ++i) EXPECT_EQ(static_cast<int>(got[i]), static_cast<int>(want[i])) << "frame " << i;
}
TEST(ComfortNoise, LevelByteAndGenerator) {
    auto n60 = comfort_noise(60, 8000);
    EXPECT_EQ(cn_level_dbov(n60.data(), n60.size()), 60);
    auto n30 = comfort_noise(30, 8000, 7);
    EXPECT_EQ(cn_level_dbov(n30.data(), n30.size()), 30);
    EXPECT_NEAR(frame_dbfs(n60.data(), n60.size()), -60.0, 0.5);
    double mean = 0; for (int16_t v : n60) mean += v; mean /= n60.size();
    EXPECT_LT(std::fabs(mean), 0.1 * 32768.0 * std::pow(10.0, -3.0));     // zero-mean
    EXPECT_EQ(comfort_noise(60, 100, 5), comfort_noise(60, 100, 5));       // deterministic
    EXPECT_NE(comfort_noise(60, 100, 5), comfort_noise(60, 100, 6));
    std::vector<int16_t> z(160, 0);
    EXPECT_EQ(cn_level_dbov(z.data(), z.size()), 120);                     // digital silence: -120 dBFS -> level 120
    std::vector<int16_t> sq(160, 32767);
    EXPECT_EQ(cn_level_dbov(sq.data(), sq.size()), 0);
    EXPECT_TRUE(comfort_noise(60, 0).empty());
}

// ============================================================ clock drift
TEST(Drift, Arithmetic) {
    EXPECT_NEAR(drift_samples_per_second(8000, 50), 0.4, 1e-12);
    EXPECT_NEAR(seconds_until_slip(8000, 50, 320), 800.0, 1e-9);                // 40 ms of margin lasts 13 minutes
    EXPECT_NEAR(seconds_until_slip(8000, -50, 320), 800.0, 1e-9);
    EXPECT_TRUE(std::isinf(seconds_until_slip(8000, 0, 320)));
    EXPECT_NEAR(seconds_until_slip(48000, 100, 960), 200.0, 1e-9);
    EXPECT_NEAR(estimate_ppm(0, 8, 100, 8000), 10.0, 1e-9);
    EXPECT_NEAR(estimate_ppm(100, 60, 200, 8000), -25.0, 1e-9);                // buffer shrinking: sender slower
    EXPECT_NEAR(estimate_ppm(0, 8, 0, 8000), 0.0, 1e-12);
    EXPECT_NEAR(resample_ratio_for_ppm(100), 0.99990001, 1e-8);
    EXPECT_NEAR(resample_ratio_for_ppm(-100), 1.00010001, 1e-8);
    EXPECT_DOUBLE_EQ(resample_ratio_for_ppm(0), 1.0);
}
TEST(Resampler, RatioOneIsIdentity) {
    std::vector<int16_t> in(100);
    for (size_t i = 0; i < in.size(); ++i) in[i] = static_cast<int16_t>(i * 37 - 1000);
    LinearResampler r(1.0);
    auto out = r.process(in);
    ASSERT_EQ(out.size(), in.size() - 1);                                     // the last sample is held back until the next call
    for (size_t i = 0; i < out.size(); ++i) EXPECT_EQ(out[i], in[i]) << i;
    auto more = r.process(std::vector<int16_t>{5, 6});
    ASSERT_EQ(more.size(), 2u);
    EXPECT_EQ(more[0], in.back()); EXPECT_EQ(more[1], 5);
}
TEST(Resampler, ChunkedEqualsOneShot) {
    auto in = sine(440.0, 5000);
    for (double ratio : {0.8, 1.0001, 1.25, 1.5, 0.5}) {
        LinearResampler a(ratio), b(ratio);
        auto whole = a.process(in);
        std::vector<int16_t> chunked;
        size_t pos = 0, sizes[] = {1, 7, 160, 13, 1, 400};
        for (size_t k = 0; pos < in.size(); ++k) {
            size_t n = std::min(sizes[k % 6], in.size() - pos);
            auto part = b.process(in.data() + pos, n);
            chunked.insert(chunked.end(), part.begin(), part.end());
            pos += n;
        }
        ASSERT_NEAR(static_cast<double>(whole.size()), static_cast<double>(chunked.size()), 1.0) << ratio;
        size_t m = std::min(whole.size(), chunked.size());
        for (size_t i = 0; i < m; ++i) ASSERT_NEAR(whole[i], chunked[i], 1.0) << "ratio " << ratio << " i " << i;
    }
}
TEST(Resampler, OutputLengthFollowsRatio) {
    std::vector<int16_t> in(10000, 100);
    LinearResampler up(1.001), down(0.999);
    EXPECT_NEAR(static_cast<double>(up.process(in).size()), 10010.0, 3.0);     // 0.1 % more samples
    EXPECT_NEAR(static_cast<double>(down.process(in).size()), 9990.0, 3.0);
    EXPECT_TRUE(LinearResampler(1.0).process(std::vector<int16_t>{}).empty());
}
TEST(Resampler, SineSurvivesRateConversion) {
    auto in = sine(440.0, 2000, 10000.0, 8000.0);
    LinearResampler r(1.5);                                                    // 8 kHz -> 12 kHz
    auto out = r.process(in);
    auto ref = sine(440.0, out.size(), 10000.0, 12000.0);
    ASSERT_GT(out.size(), 2900u);
    double worst = 0;
    for (size_t i = 3; i < out.size() - 3; ++i) worst = std::max(worst, std::fabs(static_cast<double>(out[i]) - ref[i]));
    EXPECT_LT(worst, 300.0);                                                   // linear interpolation error ~ (2*pi*f/fs)^2/8 = 1.5 % of amplitude
}

// ============================================================ mixer
TEST(Mixer, SaturatesInsteadOfWrapping) {
    std::vector<std::vector<int16_t>> s = {{30000, -30000, 100, 0}, {30000, -30000, 100}};
    auto good = mix_saturating(s);
    ASSERT_EQ(good.size(), 4u);
    EXPECT_EQ(good[0], 32767); EXPECT_EQ(good[1], -32768); EXPECT_EQ(good[2], 200); EXPECT_EQ(good[3], 0);
    auto bad = mix_wrapping(s);
    EXPECT_EQ(bad[0], -5536);                                                   // 60000 - 65536: a loud positive peak becomes a negative one
    EXPECT_EQ(bad[1], 5536);
    EXPECT_EQ(bad[2], 200);
    EXPECT_EQ(count_clipped(good), 2u);
    EXPECT_EQ(count_clipped(bad), 0u);
    EXPECT_TRUE(mix_saturating({}).empty());
    EXPECT_EQ(mix_saturating({{5, 6, 7}}), (std::vector<int16_t>{5, 6, 7}));
    EXPECT_EQ(mix_saturating({{32767}, {32767}, {32767}, {-32768}}), (std::vector<int16_t>{32767 * 3 - 32768 > 32767 ? int16_t(32767) : int16_t(0)}));
}
TEST(Mixer, MixMinusRemovesTheListenersOwnVoice) {
    std::vector<std::vector<int16_t>> s = {{1000, 2000, 3000}, {10, 20, 30}, {100, 200, 300}};
    EXPECT_EQ(mix_minus(s, 0), (std::vector<int16_t>{110, 220, 330}));
    EXPECT_EQ(mix_minus(s, 1), (std::vector<int16_t>{1100, 2200, 3300}));
    EXPECT_EQ(mix_minus(s, 2), (std::vector<int16_t>{1010, 2020, 3030}));
    EXPECT_TRUE(mix_minus({{1, 2, 3}}, 0).empty());                              // nobody else to hear
    EXPECT_EQ(mix_minus({{20000}, {20000}, {20000}}, 0), (std::vector<int16_t>{32767}));    // still saturates
}

// ============================================================ transcoder cost model
TEST(Transcoder, CostModel) {
    EXPECT_NEAR(transcoder_cores(1000, 15.0, 3000.0), 1000 * 2 * 15.0 / (3000.0 * 0.7), 1e-9);   // 14.3 cores
    EXPECT_NEAR(transcoder_cores(0, 15.0, 3000.0), 0.0, 1e-12);
    EXPECT_EQ(max_calls_per_core(15.0, 3000.0), 70u);                                          // 2100 / 30
    EXPECT_EQ(max_calls_per_core(15.0, 3000.0, 1.0), 100u);
    EXPECT_EQ(max_calls_per_core(15.0, 3000.0, 0.7, 1), 140u);                                 // one direction only
    EXPECT_EQ(max_calls_per_core(5000.0, 3000.0), 0u);                                         // one stream does not fit
}
TEST(Transcoder, RetimingAndLatency) {
    EXPECT_EQ(retime_worst_case_ms(30, 20), 20u);          // ACELP 30 ms chunks -> IMBE 20 ms frames
    EXPECT_EQ(retime_worst_case_ms(20, 30), 10u);          // IMBE chunks -> ACELP frames
    EXPECT_EQ(retime_worst_case_ms(20, 20), 0u);
    EXPECT_EQ(retime_worst_case_ms(20, 60), 0u);
    EXPECT_EQ(retime_worst_case_ms(20, 40), 0u);
    EXPECT_EQ(retime_worst_case_ms(10, 25), 5u);
    EXPECT_DOUBLE_EQ(latency_budget_ms({30, 20, 5, 20, 40}), 115.0);
    EXPECT_DOUBLE_EQ(latency_budget_ms({}), 0.0);
}
