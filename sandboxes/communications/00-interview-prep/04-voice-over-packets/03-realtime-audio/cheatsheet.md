# Cheatsheet — 04-audio / 03 Real-Time Audio

## RTP fields in one breath
| Field | Meaning | Gotcha |
|-------|---------|--------|
| seq (16 b) | +1 per packet | wraps every 21.8 min @ 50 pps; extend to 64 b relative to highest seen |
| ts (32 b) | **sampling instant, in media-clock ticks** | step = clock × ptime (G.711 20 ms = 160, Opus 20 ms = 960, G.722 20 ms = 160, AMR-WB 20 ms = 320); wraps 149 h @ 8 k, 24.9 h @ 48 k |
| marker | first packet of a talkspurt | re-prefill/adapt point |
| SSRC | one source | change ⇒ new stream |
**Silence (DTX): seq stays contiguous, ts jumps.** Loss: seq gap *and* ts gap in proportion. Compare timestamps modularly: `int32_t(a − b) < 0`.

## Jitter
```text
D(i,j) = (Rj − Ri) − (Sj − Si)        J ← J + (|D| − J)/16        (RFC 3550, units = clock ticks; ms = J·1000/clock)
target depth = clamp( ceil( 4·J / ptime ), min, max )      J=30 ms, 20 ms packets → 6 packets = 120 ms
```
Jitter ≠ latency: **variation**, not absolute delay. Buffer = converts jitter into latency (depth × ptime).

## Jitter-buffer decisions
| Event | Action | Counter |
|-------|--------|---------|
| first packets | **prefill** until depth ≥ target, output `Buffering` | |
| in-order / reordered within window | insert by ext-seq, play in order | `reordered` |
| **hole** (later packets present) | emit **Lost**, **advance** | `lost` |
| **underrun** (buffer empty) | **Lost**, do **not** advance; after `max_conceal` ⇒ **Silence** | `concealed_underrun` |
| arrives after its slot played | **drop** (still feed jitter stats) | `late` |
| duplicate | drop | `duplicate` |
| over `max_depth` | drop **oldest** | `overflow` |
| SID played | → Silence (don't conceal) | |
| seq jump beyond ±3000 | **resync**: flush + reset transit baseline | `resyncs` |
| marker after silence | re-prefill, **adapt depth here only** | |
`push()` on network events, `pop()` exactly once per ptime. No threads, no clock inside ⇒ deterministic tests.

## PLC
`r(L) = Σx[n]x[n−L] / √(Σx[n]²·Σx[n−L]²)`; peaks at P, 2P, 3P. 8 kHz pitch: lags **20…160**. Take the **first local max ≥ 0.9·best** (not the global max = maybe 2P; not first lag > threshold = rising flank, 38 vs 40). `r_best < 0.5` ⇒ no pitch ⇒ silence. History needed: **3·max_lag = 480 samples** (window 2·max_lag + lag).
Concealment: repeat the last period, `gain = max(0, 1 − 0.2·(k+1))`: 0.8, 0.6, 0.4, 0.2, 0 (k = consecutive loss index). Hides, doesn't restore. Codec-integrated PLC (Opus/AMR-WB/G.711 App. I) beats PCM-domain PLC. IMBE/AMBE: frame repeat/mute inside the vocoder ⚠.

## DTX / CN
`dBFS = 10·log10(mean(x²)/32768²)` (full scale square 0, sine amp 0.5 ⇒ −9.03, zero ⇒ −120 by convention).
VAD = threshold + **hangover** (100–200 ms). Wire: speech(marker on first) → **one SID** → nothing → SID refresh every N → speech(marker).
CN level (RFC 3389): 1 B, **−dBov**, 0 loudest … 127 quietest. Uniform noise on [−a,a] has RMS a/√3. Bandwidth: ~60 % saving at 40 % activity (⚠ talk-spurt statistics).

## Drift
`samples/s = fs · ppm · 10⁻⁶`   · 8 kHz @ 50 ppm = **0.4/s**  · slip one 20 ms packet = 400 s · 40 ms margin = 800 s
`ppm = Δdepth_samples / (fs · Δt) · 10⁶`  (depth ↑ = sender faster) · `resample ratio = 1/(1 + ppm·10⁻⁶)` (+100 ppm ⇒ 0.99990001)
Drift in *seconds* is independent of `fs`. Linear-interp error `≈ (2πf/fs)²/8` (440 Hz @ 8 k = 1.5 %); production = polyphase/sinc.

## Mix / transcode
* Sum in **int32**, then clamp or scale; `int16` wrap: `30000+30000 → −5536` (spike), saturate → 32767 (flat top). Headroom `20·log10 N` dB.
* **Mix-minus**: each listener gets all but themselves. Never add μ-law bytes: decode → linear → mix → encode.
* `cores = calls · 2 · MIPS / (core_MIPS · 0.7)`; re-timing `a − gcd(a,b)`: 30→20 = 20 ms, 20→30 = 10 ms, 20→60 = 0.
* Latency sums serially: packet + jitter buffer + transcode + look-ahead + network; target ≈ 150 ms one-way (G.114 ⚠), PTT tolerates more but not floor-control lag.

## Likely traps
1. Clock = sample rate (not for Opus/G.722). 2. Seq gap during DTX (there is none). 3. Playing late packets. 4. Not advancing on a hole / advancing on an underrun. 5. Adapting depth mid-speech. 6. Mixing in `int16`. 7. Using drift-free assumption "both are 8 kHz". 8. `2·max_lag` history for the pitch search (out-of-bounds read). 9. Jitter estimate polluted by ts discontinuity (restart, inconsistent silence timestamps).

## Run
```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure   # student.* fail until stubs are implemented
```
