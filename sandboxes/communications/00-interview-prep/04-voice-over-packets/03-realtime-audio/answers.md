# 04-audio / 03 — Answers: Real-Time Audio

Numbers marked ⚠ should be VERIFIED against the cited standard before you quote them in an interview. Everything computed by code has a test in `code/tests/test_realtime_audio.cpp`.

---

## Part A — Timestamps

### Q1 — Timestamp increments
`ts_step = clock_hz × ptime`. The *clock*, not the sampling rate (G.722) and not the bandwidth (Opus):

| Codec (RTP clock) | 10 ms | 20 ms | 30 ms |
|-------------------|-------|-------|-------|
| G.711 (8 000) | 80 | 160 | 240 |
| Opus (48 000) | 480 | 960 | 1 440 |
| G.722 (clock **8 000**, samples at 16 kHz) | 80 | 160 | 240 |
| AMR-WB (16 000) | 160 | 320 | 480 |

**Three reasons for samples, not milliseconds:**
1. **Exactness:** integer sample counts have no rounding; ms would not represent a 2.5 ms Opus frame (120 samples at 48 kHz) or 22.5 µs sample spacing.
2. **Jitter/drift maths needs the media clock:** `arrival − ts` is constant for a perfect network *only if* ts advances on the same clock the audio plays on; RFC 3550's jitter formula divides nothing by "ms".
3. **Silence and gaps are representable:** the sender simply adds the elapsed samples, so playout position is reconstructible with no extra field (the receiver does not need to know the packet duration — it can *measure* it from the ts difference).

*Etherstack:* console sends 40 ms G.711 packets: `ts += 320` per packet, payload 320 B. The gateway that re-packetises into 20 ms packets emits two packets `ts` and `ts + 160`, each 160 B, and must **rewrite seq** (+1 each) while keeping the source's timestamp *base*. If it keeps a single SSRC it must keep the timestamps monotonic across both.

### Q2 — Wrap times
(a) `2³²/clock`: 8 kHz → 536 871 s = **149.1 h** (6.2 days); 16 kHz → 268 435 s = **74.6 h**; 48 kHz → 89 478 s = **24.9 h**; 90 kHz → 47 721 s = **13.25 h**.
(b) `65 536 / pps`: 50 pkt/s → 1 310.7 s = **21.8 min**; 100 pkt/s → 655 s = **10.9 min**.
(c)
```cpp
inline bool ts_before(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b) < 0; }   // modular: valid while |a-b| < 2^31
```
The subtraction is done in unsigned (well-defined wrap), then reinterpreted as signed. Valid while the two values are < 2³¹ ticks apart (3.1 days at 8 kHz; plenty). For sequence numbers: `int16_t(a - b) < 0`. Tests: `TimestampWrapDoesNotConfuseJitter`, `SequenceWrapAround`.
*Etherstack:* 200-day uptime ⇒ ~32 wraps of the 8 kHz timestamp and ~13 000 wraps of the sequence number per stream; a test that never wraps proves nothing, so the pack tests start near 0xFFFF / 0xFFFFFFFF.

### Q3 — Silence on the wire
Last packet before: seq `n`, ts `t`, marker 0 (often a SID). First packet after 1.0 s of silence: seq **`n+1`**, ts **≈ `t + 8000`** (+160 if you measure from the end of the last packet's audio rather than from its start), **marker = 1**.
Distinguish from loss: 50 lost packets would show **seq gap of 50 *and* ts gap `= 160 × 51`** — ts and seq advance *in proportion*. Silence shows **contiguous seq with a big ts gap**, plus the marker. (A marker alone is not proof: the bit can be lost with its packet, but ts/seq proportionality still is.)
*Etherstack:* the jitter buffer must not conceal across such a gap: that is why a SID (or ts gap) switches the output to **Silence** state, not to PLC.

---

## Part B — Jitter and the buffer

### Q4 — Hand computation
Arrivals 0, 21, 43, 58, 81 ms → `R = 0, 168, 344, 464, 648`; `S = 0, 160, 320, 480, 640`; transit `R−S = 0, 8, 24, −16, 8`; `|D| = 8, 16, 40, 24`.

| step | J |
|------|---|
| after pkt 1 | 0 + (8−0)/16 = 0.5 |
| after pkt 2 | 0.5 + (16−0.5)/16 = 1.46875 |
| after pkt 3 | 1.46875 + (40−1.46875)/16 = 3.877 |
| after pkt 4 | 3.877 + (24−3.877)/16 = **5.135 units** |

`5.135 / 8 = 0.64 ms`. Target depth: `ceil(4 × 5.135 / 160) = ceil(0.128) = 1` → clamped to `min_depth = 2` (40 ms).
*Etherstack:* a path flip between two routes 12 ms apart gives **one** `|D| = 96 units` step, not a steady 12 ms of jitter — J jumps by 96/16 = 6 units (0.75 ms) and decays by 15/16 per packet. A steady flapping every few packets would show J → ~12 ms.
The test `JitterEstimateMatchesRfc3550` checks the converged value of ±5 ms alternation (5 ms) and that a constant-delay stream has J = 0.

### Q5 — Trace
Output of the reference implementation (run for this answer):

| tick (ms) | in buffer before the pop | output |
|-----------|--------------------------|--------|
| 60 | 0, 1, 2, 3, 5 (size 5 ≥ 3: prefill done) | **P seq 0** (ts 0) |
| 80 | 1, 2, 3, 5, 6, 7 | **P seq 1** |
| 100 | 2, 3, 5, 6, 7 | **P seq 2** |
| 120 | 3, 5, 6, 7 | **P seq 3** |
| 140 | 5, 6, 7 | **L** ts 640 — a *hole*: seq 4 is missing, later packets are present ⇒ advance |
| 160 | 5, 6, 7 | P seq 5 |
| 180 | 6, 7 | P seq 6 |
| 200 | 7 (+ a copy of seq 3 arrives *before* the pop) | P seq 7; copy of 3: `late` = 1 (its slot is long gone) |
| 220 | empty | **L** (ts 1280) — an *underrun*: concealed but `next_` does **not** advance; after `max_conceal` = 10 such slots → Silence |

Counter: `stats().late == 1` (a copy of an already-played seq is "late" first, "duplicate" only while it is still queued).

### Q6 — Implementation notes
* **Sequence extension relative to the highest seen:** `d = int16_t(seq − (highest & 0xFFFF)); ext = highest + d`. With the diff interpreted as signed 16 bit, both a forward wrap (0xFFFF → 0) and a reordered packet from before the wrap (0 → 0xFFFF) get the right 64-bit position. An absolute "wrap count" breaks on reordering near the wrap.
* **`late` is decided by sequence position vs the playout cursor (`ext < next_`), not by arrival time:** the *decision of "too late"* is "its slot already played", an event in the media timeline. Wall-clock arrival is noisy and only enters the *jitter statistic*. A packet whose slot has not played is still useful even if it arrived "late" relative to a nominal schedule.
* `pop()` is driven by the playout clock and must be called **exactly once per ptime**; `push()` is driven by the network. Keeping them separate and passing `arrival_us` explicitly makes the buffer deterministic: no threads, no `now()`.

### Q7 — Why timestamps must be consistent across silences
Transit time = `arrival − ts`. If a new talkspurt's ts jump is 90 packets (14 400 units) larger than the elapsed wall-clock, the first `|D|` is 14 400 and `J += 14 400/16 = 900 units = 112 ms`. J then decays only as `(15/16)ⁿ`: > 100 packets to forget it. A test stream with inconsistent talkspurt timestamps shows exactly this (J ≈ 99 ms, target depth stuck at 20). J is only meaningful across a silence if the sender's ts jump equals the elapsed time; real senders keep ts consistent with their sampling clock (A.2), so this holds in practice — and you must make test streams obey it.
Sender restart (new random ts + new seq base): the buffer detects `|Δseq| > 3000` → `resync()` (flush, `resyncs++`, re-prefill) **and** `have_transit_ = false` so that the ts discontinuity is not counted as jitter. Without that reset J explodes (restart ts is unrelated to the old one), the target depth goes to 20 and the buffer sits in prefill (test `HugeSequenceJumpResynchronises`). (A new SSRC would be a separate source in a real stack; the same logic applies when the buffer keys by SSRC.)

### Q8 — "300 ms delay since Tuesday"
Things that grow delay **without loss**:
1. **Jitter-driven depth increase** (target depth rose after a bad spell and never relaxed — check the adaptation rule shrinks at talkspurt boundaries).
2. **Clock drift: sender faster than receiver ⇒ buffer fills** at `fs·ppm·10⁻⁶` samples/s: 50 ppm ⇒ 0.4 samples/s; to add 300 ms (2 400 samples) takes **6 000 s = 1.7 h**; it grows until `max_depth` overflow drops the oldest.
3. **Device/driver buffers** (ALSA/WASAPI period × periods), plus a second buffer downstream (two jitter buffers in series).
Diagnose: RTCP RR gives *jitter* and *loss*; if both are low and delay still grows, it is drift or a buffer. Plot depth vs time: a **ramp** ⇒ drift (slope = ppm via `estimate_ppm`); a **staircase** ⇒ adaptation; a constant offset ⇒ device buffers/network path. Fix: resample (E), time-compress silence, or reset at talkspurt boundaries.

---

## Part C — PLC

### Q9 — Autocorrelation
If `x[n] = x[n−P]` (periodic), then at `L = P` the numerator `Σx[n]x[n−P] = Σx[n]²` equals the denominator (the two windows are identical): `r = 1`. The same holds for `L = 2P, 3P …` ⇒ peaks at all multiples. For a 125 Hz sine at 8 kHz: `P = 8000/125 = 64`; in the range 20…160 the peaks are **64 and 128** (192 is out of range). The best may be either (equal up to windowing) ⇒ a rule "first local maximum within 10 % of best" returns 64.
**Why the rules matter:**
* the *global* maximum may be `2P`;
* "first lag with `r ≥ 0.9·best`" is on the **rising flank**: 200 Hz sine, `r(38) = cos(2π·2/40) = 0.951 ≥ 0.9` ⇒ returns 38, not 40 (a naive implementation does exactly this; `Plc.PitchEstimate` catches it, and the concealment then ends out of phase so the waveform test fails too);
* requiring a **local maximum** excludes it.
**Window:** the comparison window is `2·max_lag` samples (two periods of the longest pitch, so that every candidate lag sees ≥ 2 full cycles) and each `x[i−lag]` must exist, so history ≥ `win + max_lag = 3·max_lag = 480` samples = 60 ms. (A guard of `2·max_lag` is a classic **out-of-bounds read** for 320–479 samples; ASan only catches it if a test supplies such an input — hence the 479/480 boundary assertions.)
**Edge returns:** silence ⇒ `xx = 0` ⇒ r = 0 ⇒ best < 0.5 ⇒ **0**; white noise ⇒ all |r| small ⇒ **0**; short history ⇒ **0**. Callers treat 0 as "no pitch: output silence".

### Q10 — Concealment
```cpp
size_t P = estimate_pitch_period(history);        // 0 => silence
double gain = std::max(0.0, 1.0 - 0.2 * (loss_index + 1));
for (size_t i = 0; i < n; ++i) out[i] = lround(history[history.size() - P + (i % P)] * gain);
```
200 Hz: `P = 40`, 160 samples = 4 periods, so output sample `i` = history sample `end − 40 + (i mod 40)` = true signal at `end + i` (periodic) times 0.8 — the **phase-continuous** continuation; the test checks `|c0[i] − 0.8·truth[i]| ≤ 3` (rounding) and the same for `0.6` at `loss_index = 1`, and `0` at `loss_index = 4`.
On a burst you must pass **the concealed audio as the new history** (otherwise every frame restarts at the same phase and the gain tells the story only by index) — the *loss_index* gives the fade.
*Codec-integrated PLC* (Opus/AMR-WB/G.711-App.I) beats this because it has the **decoder state** (LPC filter memory, pitch lag history, excitation), so it continues *the model's* excitation, not the *output samples*, and can cross-fade into the first good frame. A jitter buffer has only bytes and (after decode) PCM; you need the decoder to cooperate.
*Etherstack:* IMBE/AMBE are parametric: you cannot repeat PCM-domain pitch; P25 decoders **repeat the previous frame's parameters or mute** based on error status of the frame (⚠ VERIFY in TIA-102). The vocoder library does it internally.

---

## Part D — DTX

### Q11 — Hand numbers and design
* sine, amplitude 16 384: RMS = `16384/√2 = 11 585` → `20·log10(11585/32768) = −9.03` dBFS.
* full-scale square (32 767): RMS = 32 767 → ≈ **0.00 dBFS** (the test `Vad.FrameLevel` uses 32 767 vs 32 768: −0.0003).
* digital zero: `log10(0)` = −∞ ⇒ clamp to **−120** by convention.
**Hangover test** (`ThresholdAndHangover`): quiet (F) → loud (T) → 3 quiet frames still T (hangover 3) → 4th quiet F → loud T. The *DTX test* shows: 3 speech frames (marker only on the first), 2 hangover frames still speech, **SID** on the first non-speech frame, `Nothing` ×3, **SID refresh** every 4th frame, marker again after the next loud frame.
**CN level byte:** `level = round(−dBFS)` clamped to 0..127 (`cn_level_dbov`); the generator: uniform white noise on `[−a, a]` with `a = RMS·√3` because the RMS of uniform is `a/√3`; `RMS = 32768·10^(−level/20)` ⇒ round-trip `cn_level(comfort_noise(60)) = 60` (test).
**Floor control vs VAD:** a PTT system knows the talk burst start/stop *authoritatively* (key-up/key-down), so the **marker bit should follow the floor control event**, not an energy detector — no front-end clipping, no hangover guess. VAD is the fallback when the source has no explicit PTT (SIP phone, microphone). Both should produce the same wire signals (marker, SID).
**Bandwidth check** (worksheet D.2): SID packet `12 + 8 + 20 + 1 = 41 B`; every 160 ms ⇒ `41·8/0.16 = 2.05 kbit/s`; average `0.4·80 + 0.6·2.05 = 33.2` kbit/s ⇒ −59 %. (Assumes the 40 % activity ⚠ VERIFY.)

---

## Part E — Drift

### Q12
(a) `8000 × 50×10⁻⁶ = 0.4` samples/s; a 20 ms packet is 160 samples ⇒ **400 s** to slip one packet (≈ 6.7 min) (test: 320 samples ⇒ 800 s).
(b) depth 4.0 → 3.5 packets = −0.5 × 160 = −80 samples in 600 s: `ppm = −80/(8000×600) ×10⁶ = −16.7 ppm` ⇒ the buffer drains ⇒ the **sender is slower** than the receiver's playout clock (16.7 ppm).
(c) ratio `= 1/(1 + 37×10⁻⁶) = 0.99996300` (the stream is fast ⇒ play at slightly *fewer* output samples per input sample).
(d) 48 kHz: the same ppm gives 6× the samples/s (`48 000 × 50×10⁻⁶ = 2.4` samples/s) but the same **time**: slip of 20 ms (960 samples) = `960/2.4 = 400 s`. *Drift in seconds is independent of the sample rate*; only the amount of resampling work scales.
*Etherstack:* a long conference gets +2 s: `2 s / (50×10⁻⁶) = 40 000 s = 11 h` at 50 ppm; at 100 ppm 5.6 h — consistent with an all-day incident room bridge that was never restarted. Resample every few seconds (or time-compress silence) and cap with `max_depth`.

### Q13 — Resampler
Streaming interpolation needs `x[−1]` (the last sample of the previous chunk) and the fractional read position (`pos_`) — both persist as members; otherwise every chunk boundary glitches. With ratio 1.0 the output is the input **delayed by one sample** because the interpolator needs the left neighbour (the last sample is held until the next call: `out.size() == in.size() − 1` on the first call; the next call emits it first).
Error of linear interpolation: for `f(t) = sin ωt` the max error is `(ωT)²/8` (second-derivative bound `h²f''/8`): 440 Hz, fs 8 kHz ⇒ `ωT = 0.3456` ⇒ **1.5 %** of amplitude (test bound: 300 of 10 000 = 3 %). That is −36 dB: usable for a lesson, not for production (use a windowed-sinc/polyphase).
Tests: `ChunkedEqualsOneShot` (±1 LSB, ±1 sample length) and `OutputLengthFollowsRatio` (0.1 % more samples at ratio 1.001).

---

## Part F — Mix and transcode

### Q14
`int16_t` accumulation: `30 000 + 30 000 = 60 000 → −5 536` (test values: 60 000 − 65 536). Saturating gives 32 767.
Two in-phase −3 dBFS sines (A = 23 197): sum peaks at 46 394 > 32 767 whenever `|sin| > 0.7063`, i.e. for angles above `asin(0.7063) = 0.784 rad` in each quarter-wave of `π/2 = 1.571` ⇒ about half the time ⇒ **≈ 500 of 1 000 samples** are out of range. With saturation they clip to ±32 767 (flat tops, harmonic distortion); with `int16_t` wrap they **flip sign** (`+46 394 → −19 142`): sharp full-scale spikes: much more audible. (Quick numeric check: `46 394 − 65 536 = −19 142`.)
**Why not add μ-law bytes:** the byte is a *logarithmic code*: `μ(a) + μ(b) ≠ μ(a + b)`; adding bytes also overflows the sign/exponent bits. Always decode → linear → sum in `int32_t` → clamp/scale → re-encode (the second quantisation adds noise: tandem coding).
Tests: `SaturatesInsteadOfWrapping`, `MixMinusRemovesTheListenersOwnVoice` (`mix_minus(s, k)` = sum of all but `k`; with one participant ⇒ empty).
*Etherstack:* 10-way patch: use mix-minus with **one mix per listener** = N−1 additions each, or compute the total once and subtract each listener's own stream (cheaper: `total − own`, in int32 before clamp). Also: only mix the loudest K active talkers.

### Q15 — P25 (20 ms) ↔ TETRA (30 ms) over G.711 (20 ms)
60 ms timeline (the LCM of 20 and 30):
```text
IMBE  |--20--|--20--|--20--|           3 frames
ACELP |----30----|----30----|          2 frames
G.711 |--20--|--20--|--20--|           3 packets
      0     20    40    60 ms
```
Re-timing buffer: `retime_worst_case_ms(a, b) = a − gcd(a, b)`:
* ACELP (30) → IMBE (20): `30 − 10 = 20 ms` worst-case extra wait.
* IMBE (20) → ACELP (30): `20 − 10 = 10 ms`.
* `(20, 60) = 0`, `(20, 40) = 0`, `(10, 25) = 5` (test).
Cost: `transcoder_cores(1000, 15, 3000) = 1000·2·15/(3000·0.7) = 14.29` cores; `max_calls_per_core = floor(3000·0.7/30) = 70`; with headroom 1.0 ⇒ 100; one direction only ⇒ 140. (MIPS are illustrative ⚠ — measure on the target.) Remember that IMBE/ACELP need a licensed vocoder engine (DVSI chip or software ⚠).

### Q16 — Budget
`20 + 60 + 20 + 20 + 15 + 40 + 10 = 185 ms` **over 150 ms** (G.114 one-way target ⚠). Plus the unlisted radio-link interleave. Fixed by standards: vocoder frame (20), G.711 packet if the far end demands 20 ms, the DAC. Adjustable: **depth 3 → 2 at the gateway (−20 ms)**, console buffer 40 → 20 via better path stability, 10 ms packets between gateway and console (−10), avoid the extra re-timing by choosing 60 ms packets only if bandwidth matters more than latency (it increases delay by 40!). PTT radio is half-duplex, so conversational interactivity is less delay-sensitive than telephony: land-mobile systems commonly tolerate a few hundred ms ⚠ — but *talk-over* and *floor-control* latency do matter. Say so rather than quoting 150 ms blindly.

---

## Part G — Likely follow-up questions

| Question | One-line answer |
|----------|-----------------|
| Why can't you play late packets? | The slot was already output (concealed); playing it now means out-of-order audio or growing delay; count it and use it only for jitter statistics. |
| Why adapt only at talkspurt boundaries? | Changing the playout delay mid-speech needs time-scale modification; across silence it is inaudible. |
| Hole vs underrun? | Hole ⇒ later packets exist ⇒ advance. Underrun ⇒ may just be late ⇒ stay, conceal a few slots, then silence. |
| Is jitter = latency? | No: variation vs absolute delay. |
| How to detect drift? | Buffer depth trend; `ppm = Δdepth/(fs·Δt)·10⁶`. |
| What does a SID packet do? | Announces silence + CN level; tells the receiver to stop concealing and start comfort noise. |
| Marker bit? | First packet of a talkspurt: a safe point to re-prefill and re-adapt. |
| How many wraps in a long-running gateway? | Seq every 22 min, ts every 25 h at 48 kHz. |
| WebRTC equivalent? | NetEq: adaptive jitter buffer + time-scale + PLC in one (⚠ VERIFY details). |
