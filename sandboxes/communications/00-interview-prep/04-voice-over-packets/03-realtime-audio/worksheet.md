# 04-audio / 03 — Real-Time Audio: RTP Timing, Jitter Buffers, Loss Concealment, DTX, Drift, Mixing

**Time:** ~5–6 h · **Prereqs:** `03-net/04-rtp-sip`, `04-audio/01-sampling-pcm`, `04-audio/02-codecs` · **Code:** `code/include/realtime_audio.hpp` (stub), `code/solutions/realtime_audio.hpp`, `code/tests/test_realtime_audio.cpp` (28 tests)
**Format:** markdown with numeric tables. No notebook: the behaviours that matter (timelines of arrivals and playouts) are shown as small text tables, and every claim is also checked by a deterministic test.
**Etherstack lens:** an LMR/PTT gateway receives RTP from radios and dispatch consoles over IP networks that reorder, delay and drop packets, and plays them into a radio channel or a conference that **does not wait**. The interview questions are always some form of: *"the network is imperfect, the audio clock is not — what do you do, what does it cost, and how do you know?"*

> **Legal / safety:** all audio is synthetic (sines, silence, a deterministic noise generator, simulated arrival times). No live public-safety receive. Interception of radio traffic is regulated (in the UK: Wireless Telegraphy Act 2006 and the Investigatory Powers Act 2016 ⚠ VERIFY the exact sections with your legal contact) — build and test on synthetic data only.

---

## Part A — Timestamps count samples (derived, then used)

### A.1 What the fields mean
An RTP header (RFC 3550) carries: **sequence number** (16 bit, +1 per packet, detects loss/reorder), **timestamp** (32 bit, the *sampling instant* of the first sample in the payload, in units of the payload's *clock*), **SSRC** (32 bit, identifies one source in one RTP session), and a **marker bit** (profile-defined; for audio it flags the **first packet of a talkspurt**, RFC 3551 §4.1).

### A.2 Why the timestamp counts samples and not milliseconds
Start from what the receiver needs: *where on the audio timeline does this packet's first sample go?* If the timeline is the sample clock, then "position" is an integer number of samples.
* 20 ms at 8 kHz = `0.020 × 8000 = 160` samples: **ts += 160** per packet. An integer, exact, no drift.
* Counting milliseconds would put 20 per packet — but the sample clock then has to be *converted* each time, and sub-millisecond positions (e.g. a 2.5 ms Opus frame = 120 samples at 48 kHz) cannot be represented at all.
* The timestamp must be *monotonic in sampling time*, **not** in send time: two packets sent in a burst after a stall still carry timestamps 160 apart. That is exactly what lets the receiver measure jitter: `arrival time − timestamp` should be constant if the network is perfect.
* **During silence suppression the timestamp keeps running; the sequence number does not skip.** The first packet after silence has `ts` jumped by the *elapsed samples* and `seq` just +1, and marker = 1. Receiver logic: *seq gap = loss; ts gap with contiguous seq = silence*.

```text
 talkspurt 1                              silence (not sent)               talkspurt 2
 seq  100 101 102 ... 149 150(SID)                                        151(M) 152 153
 ts  16000 16160 ...  23840 24000                                         32000  32160 ...
                              ^ ts jumps by 8000 = 1 s of silence,  seq only +1, marker = 1
```
`T(1) + 8000` in the tests `SidSwitchesToSilenceNotConcealment`, `TargetDepthAdaptsAtTalkspurtBoundaries` is exactly this pattern.

### A.3 Wrap-around
`2^32 / 8000 = 536 871 s ≈ 149 h` for 8 kHz. At 48 kHz: `2^32/48000 = 89 478 s ≈ 24.9 h`. A gateway that runs for months **will** wrap. Always compute differences with unsigned/modular arithmetic: `int32_t(a − b)`. Sequence numbers wrap every `65 536 × 20 ms = 21.8 min` at 50 pkt/s — extend to 64 bits relative to the highest seen (`ext = highest + int16_t(seq − (highest & 0xFFFF))`).

> **Common misconceptions**
> 1. *"Timestamp = wall-clock send time."* No: it is the **sampling instant on the media clock**, with an arbitrary random starting value (RFC 3550 §5.1 — randomised for security).
> 2. *"Missing sequence numbers during silence mean loss."* With DTX the **sequence is contiguous**; it is the **timestamp** that jumps.
> 3. *"Two streams with the same timestamp are synchronised."* Timestamps of different SSRCs have unrelated random offsets; lip-sync uses **RTCP SR** (NTP↔RTP timestamp mapping).
> 4. *"Wrap-around is a once-in-a-lifetime event."* It is a once-a-day event at 48 kHz and a once-every-22-minutes event for sequence numbers.

**Q1.** Packets of 10, 20, 30 ms for G.711 (8 kHz), Opus (48 kHz), G.722 (clock 8 kHz!) and AMR-WB (16 kHz): timestamp increment each. Give three reasons the timestamp counts samples rather than milliseconds. *Etherstack:* a console sends 40 ms G.711 packets, the radio gateway expects 20 ms; what does `ts` do on each side?

**Q2.** (a) Time until the 32-bit timestamp wraps at 8, 16, 48 kHz and 90 kHz (video). (b) Time until the 16-bit sequence number wraps at 50 pkt/s and 100 pkt/s. (c) Write `bool ts_before(uint32_t a, uint32_t b)` that works across the wrap. *(Test `JitterBuffer.TimestampWrapDoesNotConfuseJitter`, `SequenceWrapAround`.)* *Etherstack:* a base-station gateway has 200-day uptime.

**Q3.** A source is silent for 1.0 s and then resumes (G.711, 20 ms). Give `seq`, `ts`, `marker` of the last packet before and first packet after. How does the receiver tell this from *50 consecutive lost packets*? *Etherstack:* a PTT radio releases the button; the dispatcher must not hear "packet loss" artefacts in the pause.

---

## Part B — Jitter: measure it, then build a buffer

### B.1 Interarrival jitter (RFC 3550 §6.4.1 / A.8)
For packets *i* and *j = i + 1*, with arrival times `R` and timestamps `S` (both in clock units):
`D(i,j) = (Rj − Ri) − (Sj − Si) = (Rj − Sj) − (Ri − Si)` — the change of *transit time*. The estimator is a one-pole low-pass of `|D|`:

```text
J ← J + ( |D| − J ) / 16
```
*Why 1/16?* A smoother with gain `g = 1/16` averages over ~16 packets (~320 ms) and is a shift in integer arithmetic. It follows slow changes but ignores single outliers; a larger gain reacts faster but wobbles. (Rationale per the RFC's appendix ⚠ VERIFY wording.) Mean `|D|` of a ±5 ms alternating delay converges to 5 ms (test `JitterEstimateMatchesRfc3550`).

**Worked example (8 kHz).** Packets sent every 20 ms (`S = 0, 160, 320, 480`). Arrivals at 0, 22, 38, 65 ms = `R = 0, 176, 304, 520` units.

| i | R | S | transit R−S | \|D\| | J after |
|---|---|---|-------------|-------|---------|
| 0 | 0 | 0 | 0 | — | 0 |
| 1 | 176 | 160 | 16 | 16 | 0 + (16−0)/16 = **1.000** |
| 2 | 304 | 320 | −16 | 32 | 1 + (32−1)/16 = **2.9375** |
| 3 | 520 | 480 | 40 | 56 | 2.9375 + (56−2.9375)/16 = **6.254** |

`6.254 units / 8 = 0.78 ms`. Note J is **only a statistic of *variation*** — a constant 300 ms delay gives J = 0.

> **Misconception box** — *"Jitter = latency."* No. Latency is the *absolute* delay; jitter is the **variation**. A constant satellite delay of 600 ms has zero jitter and terrible latency; a LAN with bursty queues has low latency and high jitter. The buffer **converts jitter into latency**.

### B.2 The jitter buffer: the idea
The playout device consumes exactly one frame every ptime, forever. The network delivers irregularly. Insert a queue: **hold the first packet for a while (prefill), then pop one per tick**.
* A packet is useful iff it arrives **before its playout slot**; the slot is `first_slot + (seq − first_seq) × ptime`.
* Extra delay = depth × ptime. Depth 3 at 20 ms = **60 ms added one-way latency** — to be compared with the end-to-end budget (ITU-T G.114: ≤ 150 ms one-way considered acceptable for most conversational use; 400 ms upper limit ⚠ VERIFY).
* The trade-off: *deeper → fewer late losses, more latency.* No setting is right for every network.

**Required behaviours (each is a test):**
| Situation | Buffer's decision | Test |
|-----------|-------------------|------|
| first packets arrive | **Prefill** until depth ≥ target → output `Buffering` | `PrefillsThenPlaysInOrder` |
| packet overtaken by a later one | insert in seq order; played in order | `ReorderingWithinDepthIsRepaired` |
| a **hole**: slot *n* missing but *n+1…* present | emit **Lost** for slot *n*, **advance** | `LostPacketIsConcealedAtItsOwnSlot` |
| a copy of a packet already queued | drop, count `duplicate` | `DuplicatesAreDropped` |
| a packet arrives **after its slot was played** | drop, count `late` (never replay out of order) | `ShuffledAndDroppedStreamEndToEnd` |
| **underrun**: buffer completely empty | emit **Lost** but **do not advance** (it may be merely late); after `max_conceal` slots switch to **Silence** | `UnderrunConcealsThenGoesSilentWithoutAdvancing` |
| packet marked **SID** was played | go to **Silence** (no concealment) | `SidSwitchesToSilenceNotConcealment` |
| buffer exceeds max depth | drop **oldest** (bounded delay) | `OverflowDropsOldest` |
| sequence jumps by > 3 000 | **resynchronise**: new stream | `HugeSequenceJumpResynchronises` |
| silence → marker packet | re-prefill; this is when depth may adapt | `TargetDepthAdaptsAtTalkspurtBoundaries` |

> **Misconception box**
> 1. *"A late packet should be played anyway."* It cannot be: the audio device already played (concealed) that slot. Playing it later makes the speaker's words repeat or the delay grow.
> 2. *"Hole and underrun are the same."* A **hole** with later packets present is *evidence of loss or reorder-past-the-window*: advance. An **underrun** (nothing in the buffer) is *absence of evidence*: do not skip a slot number, or every later arrival will look "late".
> 3. *"Adapt the depth continuously."* Changing playout delay in the middle of speech means **stretching or dropping audio** (audible). Adapt at **talkspurt boundaries** (silence is free to shorten/lengthen).
> 4. *"The jitter buffer fixes packet loss."* It fixes **reorder and jitter**. Loss needs concealment (Part C), FEC, or retransmission.

### B.3 Choosing the depth: adaptive rule
A simple, defensible rule: `target = clamp( ceil( k · J / ptime ), min_depth, max_depth )` with `k = 4` ("cover four mean deviations"). (Ramjee et al. 1994 use a running mean + 4 × variation for the same reason ⚠ VERIFY citation.)
* J = 30 ms → `4 × 30 / 20 = 6` packets = **120 ms**.
* J = 5 ms → `ceil(1.0) = 1` → clamped to `min_depth = 2` = 40 ms.
* A bursty network with `max_depth = 20` caps the extra delay at 400 ms and **drops the oldest** beyond that.

**Q4.** Hand-compute J for arrivals at 0, 21, 43, 58, 81 ms (sent every 20 ms, 8 kHz, units of samples then ms). Then say what `target_depth` the rule gives with `ptime = 20 ms`. *(Test `JitterEstimateMatchesRfc3550`.)* *Etherstack:* a microwave link between a hilltop site and the core flaps between two paths 12 ms apart.

**Q5.** Trace the buffer (depth 3, ptime 20 ms) for this arrival list and fill the playout table (`P`=Packet, `L`=Lost, `B`=Buffering): seq 0 at t=0, 2 at t=5, 1 at t=30, 3 at t=35, 5 at t=50, 6 at t=70, 7 at t=75 (nothing after that), playout ticks every 20 ms from t=60 to t=220. What does the tick at t=140 emit (seq 4 is never sent), and the tick at t=220 (nothing arrives after t=75)? Which counter increments if a copy of seq 3 arrives at t=200? *(Test `ShuffledAndDroppedStreamEndToEnd`, helper `simulate()`.)*

**Q6.** Implement `JitterBuffer::push/pop` in the stub. Deliver the 12 jitter-buffer tests. Explain in comments *why* the sequence extension is relative to the highest seen, and why `late` is checked against `next_` and not against arrival time.

**Q7.** Explain why the adaptation test needs *consistent timestamps* across silences (`160·(410+400)` etc.): what happens to the jitter estimate if the new talkspurt's `ts` jump does not match the elapsed wall-clock time? How does the buffer avoid a *sender restart* (ts jump + seq jump) polluting J? *(Test `HugeSequenceJumpResynchronises`.)* *Etherstack:* a radio gateway reboots and keeps the SSRC but re-randomises timestamp and sequence.

**Q8.** A user says "the audio has a 300 ms delay since last Tuesday". List three things in the pipeline that can *grow* playout delay without any loss (hint: depth target creeping up from jitter spikes, **clock drift** (Part E), device buffer). How would you diagnose from RTCP? *Etherstack:* dispatcher complaint.

---

## Part C — Packet loss concealment (PLC)

### C.1 First principles
A lost 20 ms frame leaves a gap of 160 samples. Options, cheapest first: (1) **silence** — audible click/dropout; (2) **repeat the previous frame** — a buzz, phase jump at the seam; (3) **repeat the last pitch period** — voiced speech is quasi-periodic, so continuing the waveform for 1–2 more periods is nearly inaudible; (4) **fade** — if the loss goes on, stop pretending: attenuate and mute after ~60 ms (a vowel does not last forever ⚠ typical, not a standard).

**Pitch from autocorrelation.** For lag `L`, `r(L) = Σ x[n]·x[n−L] / sqrt(Σx[n]² · Σx[n−L]²)` over the last window. A periodic signal of period `P` has `r ≈ 1` at `L = P, 2P, 3P …` — so take the **first local peak** that is close to the best, not the global maximum (it may be `2P`) and not "the first lag over a threshold" (that is on the rising flank — for a 200 Hz sine, 38 passes 0.9 before 40 does).
At 8 kHz, human pitch 50–400 Hz ⇒ lags **20…160** samples.

**Concealment:** `out[i] = gain · history[end − P + (i mod P)]` with `gain = max(0, 1 − 0.2·(loss_index+1))`: 0.8, 0.6, 0.4, 0.2, 0 — silent from the fifth consecutive lost frame (the fade lasts 80 ms; real systems mute somewhere in 60–120 ms ⚠ VERIFY). Because the repeated cycle starts where the history ended, the waveform is **phase-continuous**; with a period that divides 160 the concealed frame ends exactly in phase for the next real packet.

> **Misconception box** — *"Repeating the last frame is as good as pitch repeat."* A 20 ms frame is rarely a whole number of pitch periods; the seam is a discontinuity → click/buzz at 50 Hz. Also: *"PLC adds information"* — it does not; it **hides** missing information. It cannot restore consonants.

**Q9.** Derive why `r(L)` has peaks at multiples of the period. For a 125 Hz sine at 8 kHz give `P` and the correlation peaks inside lags 20…160. Why does the search window use `2 × max_lag` samples and why does the function need `3 × max_lag` of history? What does your function return on silence, on white noise, and on 100 samples of history? *(Test `Plc.PitchEstimate`.)*

**Q10.** Implement `estimate_pitch_period` and `conceal_pitch`. Show that with `loss_index = 0` and a 200 Hz sine the concealed 160 samples equal `0.8 × the true continuation` (within rounding). *(Test `Plc.ConcealmentContinuesTheWaveformAndFades`.)* Where would **codec-integrated PLC** (Opus, AMR-WB, G.711 Appendix I) beat this, and what state does it need that a bare buffer lacks? *Etherstack:* IMBE frames cannot be concealed in the PCM domain without the vocoder; what do P25 decoders do on a bad frame (frame repeat/mute — ⚠ VERIFY in TIA-102)?

---

## Part D — Silence suppression: VAD, DTX, comfort noise

### D.1 Why
In a conversation each party speaks roughly 35–40 % of the time (Brady 1969 ⚠ VERIFY; push-to-talk radio is more extreme: one talker, many listeners, long gaps). Sending nothing for 60 % of the time cuts bandwidth by ~60 % — at the cost of **dead-silent** pauses that make the listener think the line dropped. So: send nothing **but tell the receiver to synthesise background noise** (*comfort noise*, CN).

### D.2 Parts
1. **VAD (voice activity detection):** frame energy `10·log10(mean(x²)/32768²)` dBFS against a threshold (here −45), plus **hangover** (keep "speech" for N frames after the last loud one, typically 100–200 ms, so word endings are not chopped).
2. **DTX (discontinuous transmission) state machine:** talkspurt → send speech, **marker = 1 on its first packet**; on entering silence send **one SID** (silence-insertion descriptor: CN level and optionally spectral shape); stay quiet with **SID refresh** every N frames (noise changes); the next speech frame carries the marker again. *(Test `Dtx.ActionSequence`.)*
3. **Comfort noise (RFC 3389):** payload type 13 (8 kHz) — a 1-byte noise level in **−dBov** (0 = loudest, 127 = quietest), optionally followed by up to 8 spectral reflection coefficients ⚠ VERIFY. Full-scale square wave ≈ 0 dBov; silence → level 120+ (clamped at 127).
4. **Receiver:** on SID/silence, stop concealing; generate noise at the signalled level (here uniform white noise with `RMS = 32768 · 10^(−level/20)`; uniform on [−a, a] has RMS `a/√3`).

**Bandwidth example.** G.711 20 ms active = 80 kbit/s IP. 60 % silence with one 1-byte-level SID (RTP 12 + UDP 8 + IP 20 + 1 = 41 B) every 160 ms (8 frames): `41 B × 8 / 0.16 s = 2.05 kbit/s`. Average ≈ `0.4 × 80 + 0.6 × 2.05 = 33.2 kbit/s` (−59 %).

> **Misconception box**
> 1. *"Silence = digital zero."* Real silence has room noise; digital zero sounds **dead**. CN fills it.
> 2. *"VAD threshold = fixed."* Fixed thresholds fail in noisy vehicles (a fire-ground radio has −30 dBFS background); production VADs track the noise floor. Our fixed −45 is a teaching simplification.
> 3. *"DTX is free."* It adds clipping risk at word starts (**front-end clipping**; mitigated by look-back) and complicates jitter buffers, mixers and recorders (they must understand SID/marker).

**Q11.** Implement `frame_dbfs`, `Vad`, `DtxEncoder`, `cn_level_dbov`, `comfort_noise`. Compute by hand: dBFS of a sine with amplitude 16 384 (−9.03), of a full-scale square wave (0), of silence (−120 by convention). *(Tests `Vad.*`, `Dtx.*`, `ComfortNoise.*`.)* *Etherstack:* a PTT floor-control system sends "talk burst start/stop" in-band (e.g. via signalling); how does that compare with VAD-driven DTX, and which one should drive the marker bit?

---

## Part E — Clock drift

### E.1 Derivation
Sender's ADC and the receiver's DAC are driven by *different crystals*, each accurate to ±tens of ppm. With relative error `δ` (parts per million) at sample rate `fs`: surplus rate `= fs · δ · 10⁻⁶` samples/s.
* 8 kHz at **50 ppm** = **0.4 samples/s**. A 40 ms (320-sample) margin lasts `320 / 0.4 = 800 s ≈ 13 min`.
* Two devices at ±50 ppm each → worst relative 100 ppm → 0.8 samples/s → 400 s.
* 48 kHz at 100 ppm = 4.8 samples/s; 960-sample (20 ms) margin gone in **200 s**.

If the sender is **faster**, packets arrive faster than they play: **the buffer fills** (delay grows; eventually overflow drops audio). If **slower**: the buffer **drains** (underruns, concealment). *The sign of the depth trend tells you which clock is faster*: `ppm = (depth_end − depth_start) / (fs · seconds) · 10⁶` (test `Drift.Arithmetic`). Example: +8 samples in 100 s at 8 kHz = **10 ppm**.

### E.2 Fix
Every few seconds, **resample** the received audio by `ratio = 1/(1 + ppm·10⁻⁶)` (a stream that is 100 ppm fast is played at `0.99990001`), or add/remove one pitch period during silence (**WSOLA-style** time scaling). The linear interpolator `y(t) = x0 + frac·(x1−x0)` is the teaching version: relative error `≈ (2π f/fs)²/8` — for 440 Hz at 8 kHz `= 1.5 %` (−36 dB), unacceptable for HiFi, fine for a lesson; production code uses polyphase/sinc filters.

> **Misconception box**
> 1. *"Both ends use 8 kHz so there is no drift."* "8 kHz" is a nominal value of two independent oscillators.
> 2. *"Drift is a network problem."* It is a **clock-domain** problem; a perfect network still shows it.
> 3. *"NTP the endpoints and it goes away."* NTP disciplines the *system* clock; the audio device runs on its own crystal (unless the OS resamples).

**Q12.** (a) 8 kHz, 50 ppm: samples/s and time to slip one 20 ms packet. (b) Observed: buffer depth 4.0 → 3.5 packets in 10 min: ppm and which side is faster? (c) Ratio to correct a stream measured at +37 ppm. (d) The same for 48 kHz Opus. *Etherstack:* a long (hours-long) recorded incident-command conference suddenly gets 2 s of delay.

**Q13.** Implement `LinearResampler`. Show that streaming in chunks (1, 7, 160, 13 samples…) equals one-shot output, and that ratio 1.0 reproduces the input delayed by one sample. Why must state (the previous sample and the fractional position) persist across calls? *(Tests `Resampler.*`.)*

---

## Part F — Mixing and transcoding at a conference gateway

### F.1 Mixing
Summing N voices: the sum can reach `N × 32 767` and **does not fit** in 16 bits.
* **Wrap-around** (accumulate in `int16_t`): `30 000 + 30 000 = 60 000 − 65 536 = −5 536` — a loud positive peak becomes a negative one: **violent crackle**.
* **Saturate** (accumulate in `int32_t`, clamp to [−32 768, 32 767]): clipping distortion, but monotone.
* Better: **scale** (÷N, or per-source gain control/AGC) before summing; or mix only the loudest K speakers. Worst-case headroom `20·log10(N)`: 2 voices 6 dB, 4 voices 12 dB.
* **Mix-minus (N−1):** each participant hears everyone **except themselves** — otherwise they hear their own voice (echo) delayed by the round trip.
* Mix in **linear PCM**: you cannot add μ-law bytes (they are logarithmic!). Every conference therefore decodes (G.711 → linear), mixes, and re-encodes (+ extra quantisation noise).

### F.2 Transcoding cost model
`cores = calls × streams_per_call × MIPS_per_stream / (core_MIPS × headroom)`; a call transcodes **both directions** (2 streams); headroom ~0.7 avoids saturating a core. Example (illustrative ⚠ — measure!): 1 000 calls, 15 MIPS per stream, 3 000 MIPS per core: `1000 × 2 × 15 / (3000 × 0.7) = 14.3` cores; 70 calls/core.
**Latency** adds up serially: `capture 20 + jitter buffer 40 + transcode re-timing 20 + encode look-ahead 5 + network …`. Re-timing between frame sizes `a` and `b` ms costs up to `a − gcd(a, b)` ms: **30→20 ms: 20 ms**; **20→30: 10 ms**; 20→60: 0 (an integer multiple).

> **Misconception box**
> 1. *"Transcoding is just a table lookup."* Only G.711 μ-law ↔ A-law is. Anything else is decode + encode with algorithmic delay, licensing and quality loss (**tandem coding**).
> 2. *"Mixing is addition."* Addition with *headroom management* and *int32 accumulation*.
> 3. *"Two jitter buffers in series don't matter."* Delay adds; the second buffer sees *smoothed* arrival times and adapts to nothing.

**Q14.** Implement `mix_saturating`, `mix_wrapping` (the intentional bug), `mix_minus`, `count_clipped`. For two −3 dBFS sines (amplitude 23 197) computed in `int16_t` vs `int32_t`: how many samples clip/wrap per 1000? Why can't you mix μ-law bytes directly? *(Tests `Mixer.*`.)* *Etherstack:* a 10-way talkgroup patch (P25 ↔ DMR ↔ SIP phone).

**Q15.** Implement `transcoder_cores`, `max_calls_per_core`, `retime_worst_case_ms`, `latency_budget_ms`. A gateway bridges P25 IMBE (20 ms) to TETRA ACELP (30 ms) over a G.711 hub: draw the frame timeline for 60 ms (3 IMBE, 2 ACELP, 3 G.711 packets) and compute the worst-case re-timing delay in each direction. *(Tests `Transcoder.*`.)*

**Q16.** End-to-end budget: IMBE frame 20 ms + radio-link interleave ⚠ + gateway receive jitter buffer (depth 3 × 20) + transcode re-time 20 + G.711 packet 20 + network 15 + console jitter buffer 40 + DAC 10. Add up with `latency_budget_ms`; compare with 150 ms. Which items can you change, which are fixed by the standard?

---

## Part G — Say it out loud (interview drills)

1. "RTP timestamp counts **samples** on the media clock; it jumps over silence, sequence doesn't."
2. "Jitter buffer = queue that converts **variation into latency**: prefill, pop on the playout clock, drop late, conceal holes."
3. "Hole → advance. Underrun → don't advance, conceal a few slots, then silence."
4. "Adapt depth **between talkspurts**; target ≈ 4 × jitter."
5. "RFC 3550 jitter: `J += (|D| − J)/16`."
6. "PLC = repeat last pitch period with fade; hides, doesn't restore."
7. "DTX: marker on first packet, SID on entering silence, comfort noise at the receiver."
8. "Drift: `fs · ppm · 10⁻⁶` samples/s; buffer depth trend tells you who is faster; fix by resampling."
9. "Mix in linear int32, saturate, N−1 for each participant; never add μ-law bytes."

## VERIFY list for this pack
* ITU-T G.114 numbers (150 ms / 400 ms), Brady 1969 talk-spurt statistics (≈ 35–40 % activity), Ramjee et al. adaptation constant (k = 4), RFC 3389 spectral-parameter details, G.711 Appendix I PLC parameters, P25 bad-frame handling (TIA-102), MIPS per stream (illustrative only), the UK legal references.
* The jitter-buffer policy (hole vs underrun, adapt at talkspurt boundaries, `kJumpLimit = 3000`) is **a teaching design**, not a standard; production buffers (WebRTC NetEq, libjitter, Asterisk's `jitterbuffer`) differ in detail.
