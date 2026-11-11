# Answers — 04-audio / 01 Sampling, PCM and Packet Sizing

Attempt each question first. ⚠ = verify against the standard named.

---

**Q1.** (a) fs = 8 kHz, so `n/8` of a cycle per sample for 1 kHz:
| n | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| 1 kHz = sin(2πn/8) | 0 | .707 | 1 | .707 | 0 | −.707 | −1 | −.707 |
| 9 kHz = sin(2π·9n/8) | 0 | .707 | 1 | .707 | 0 | −.707 | −1 | −.707 |
| 7 kHz = sin(2π·7n/8) | 0 | −.707 | −1 | −.707 | 0 | .707 | 1 | .707 |
`9n/8 = n + n/8` so `sin(2πn + 2πn/8)` = the 1 kHz value; `7n/8 = n − n/8` gives `−sin(2πn/8)`. (b) 5 kHz → `8 − 5` = **3 kHz** (inverted phase): you'd hear a 3 kHz whistle where there was a 5 kHz burst — and nothing can tell them apart afterwards, so the filter must remove it *before* the ADC/decimator. (c) 7, 9, 15, 17 kHz (and 1 kHz itself). Tests: `Sampling.NyquistAndAliasing`, `Sampling.AliasedToneIsTheSameSamples`.

**Q2.** Voice band 300–3400 Hz; Nyquist for 3.4 kHz is 6.8 kHz, but the anti-alias filter needs a transition band: at 6.8 kHz the filter would have to drop from 0 dB to full rejection instantly (brick wall = infinite order, infinite delay). At fs = 8 kHz the cut-off may start at 3.4 kHz and must have reached the stop level at fs/2 = 4 kHz → **600 Hz** transition band: an affordable filter (and fs divides 1 s, 1 ms and 20 ms into integers: 125 µs/sample, 8 samples/ms, 160/20 ms). 7 kHz would leave only 100 Hz of transition (3.5 − 3.4): impractical; 6.8 kHz zero. Wideband 50–7000 Hz → 14 kHz minimum, choose **16 kHz** → transition 7→8 kHz = 1 kHz (comfortable, power of two multiple of 8 kHz → simple 2:1 resampling to/from telephony). Sample periods: 8 kHz **125 µs**, 16 kHz **62.5 µs**, 44.1 kHz **22.68 µs**, 48 kHz **20.83 µs**.

**Q3.** (a) See B.1: `Δ = 2A/2^N`, error power `Δ²/12`, sine power `A²/2` with `A = 2^(N−1)Δ` → `2^(2N−3)Δ²`; ratio `1.5·2^(2N)`; dB: `10log10 1.5 = 1.76`, `20N log10 2 = 6.02N`. (b) N=4 **25.84** dB; 8 **49.92**; 11 **67.98**; 13 **80.02**; 16 **98.08**; 24 **146.24**. (c) −30 dBFS signal: SNR = ideal − 30: 8-bit → **19.9 dB**. Want ≥ 40: `6.02N + 1.76 − 30 ≥ 40` → `N ≥ 11.6` → **12 bits** (linear) — hence G.711 companding (8 bits ≈ 13-bit linear dynamic range ⚠).

**Q4.** Measured (run in WSL): amplitude 0.999 @ 8 bit: 49.3 dB (ideal 49.9); 0.1 @ 12 bit: **53.9 dB** (prediction `74.0 − 20 log10(1/0.1)` = 54.0 ✓); 0.01 @ 8 bit: **11.1 dB** (predicted 9.9); 0.001 @ 8 bit: **0 dB** (predicted −10). With amplitude 0.001 the peak is 32.8 while half a step at 8 bits is 128 → *every sample rounds to 0*: the output is silence, so error = signal and SNR = 0 dB (the formula would claim −10 dB). The `Δ²/12` model assumes the signal is many steps big so the error looks random; for small signals error becomes **correlated with the signal** (dead zone, harmonic distortion) — the reason for **dither** and for companding.

**Q5.** `(int16_t)(1.0*32768)`: 32768 does not fit int16 → converting an out-of-range float to an integer is **undefined behaviour** (and `(int16_t)(int)32768` is implementation-defined wrap to −32768 pre-C++20): the loudest positive peak becomes the loudest *negative* — a click. x = 1.5 gives 49152 → wraps to −16384 (a big wrong sample). Correct: `round`, then clamp to [−32768, 32767] (`float_to_s16`; +1.0 → 32767, −1.0 → −32768). Mixer: `int16_t a = 30000, b = 30000; int16_t m = a + b;` — the addition happens in `int` (60 000) then narrows to `int16_t` → **−5536** (wrap: 60 000 − 65 536): crackling distortion. Fix: accumulate in `int32_t`, then clamp; or use float. Test: `Pcm.FloatToS16ClipsAndRounds`.

**Q6.** G.711 (8 B/ms):
| ptime | samples | payload | IP packet | IP kbit/s | Ethernet frame | Eth kbit/s |
|-------|---------|---------|-----------|-----------|----------------|------------|
| 10 ms | 80 | 80 B | 120 B | **96.0** | 138 B | **110.4** |
| 20 ms | 160 | 160 B | 200 B | **80.0** | 218 B | **87.2** |
| 30 ms | 240 | 240 B | 280 B | **74.67** | 298 B | **79.47** |
| 40 ms | 320 | 320 B | 360 B | 72.0 | 378 B | 75.6 |
For a 2 Mbit/s backhaul: 20 ms → `2000/87.2` = 22 calls per direction at 100 % (about 18 at 80 % utilisation); 30 ms only +2 calls (10 % better) but adds 10 ms delay and each loss costs 30 ms of audio. Choose **20 ms** unless capacity is critical; if it is, change the *codec* (G.729/AMBE/Opus-narrowband) rather than ptime.

**Q7.** Table (IP-level):
| ptime | pkts/s | G.711 payload | IP packet | IP kbit/s | G.729 payload | IP packet | IP kbit/s |
|-------|--------|---------------|-----------|-----------|---------------|-----------|-----------|
| 10 | 100 | 80 | 120 | 96.0 | 10 | 50 | 40.0 |
| 20 | 50 | 160 | 200 | 80.0 | 20 | 60 | 24.0 |
| 30 | 33.3 | 240 | 280 | 74.67 | 30 | 70 | 18.67 |
| 40 | 25 | 320 | 360 | 72.0 | 40 | 80 | 16.0 |
Proof: bandwidth = `(payload + 40) · 8 / p` = `payload·8/p + 320/p`; `payload·8/p` (bits per ms = kbit/s) is the codec rate. IPv6: 60 B of headers → `480/p`. SRTP 80-bit tag: +10 B → `+80/p` (at 20 ms: +4 kbit/s); IPv6+SRTP: `560/p`. Test `Sizing.SmallerPacketsCostMoreOverhead`.

**Q8.** G.729: 10 ms frames of 80 bits = 10 B; 20 ms → **20 B payload** (two frames), IP packet 20 + 40 = **60 B**, `60 × 8 / 20` = **24 kbit/s** = **3×** the 8 kbit/s codec rate. G.711 20 ms is 80 kbit/s: the codec is 8× smaller (64 → 8) but the call only **3.33×** smaller (80 → 24). Same 50 packets/s though: routers, firewalls, radio schedulers and SBCs care about **packets per second** (CPU/interrupts), not only bits.

**Q9.** Per direction each call needs 87.2 kbit/s (Ethernet-level). 100 Mbit/s × 50 % = 50 000 kbit/s → `50 000 / 87.2` = **573 calls**; 1 Gbit/s → **5 733**. Line-rate figure 95.2: **525** and **5 252** (more honest, because preamble and IFG consume capacity). A full-duplex link counts each direction separately; a call uses both, so the numbers are per direction. (Real limits come first from PPS/CPU of the gateway, QoS and redundancy.)

**Q10.** Budget 800 kbit/s, RTCP ≈ 5 % (RFC 3550 recommends a fraction of ~5 % of session bandwidth ⚠): usable for media ≈ 760 kbit/s. G.711/20 ms: 80 → `760/80` = **9 calls** (≈ 8 if you count Ethernet/MPLS overhead). G.729: 24 → **31 calls**. Opus 24 kbit/s / 20 ms: payload 60 B → IP 100 B → 40 kbit/s → **19 calls**. (Each call uses this per direction.) Lesson: on a thin backhaul codec choice dominates; ptime 40 ms saves another ~10 %.

**Q11.**
| Packet | 64 kbit/s | 1 Mbit/s | 1 Gbit/s |
|--------|-----------|----------|----------|
| 200 B | 25 ms | 1.6 ms | 1.6 µs |
| 1500 B | **187.5 ms** | 12 ms | 12 µs |
A voice packet behind one MTU-size data packet at 64 kbit/s waits up to 187.5 ms (a packet already being transmitted cannot be pre-empted by priority queueing) → jitter of 9 voice packets → bursts and late loss in the jitter buffer. Fixes: fragment/shape the data packets so a data frame is ≤ ~200 B on slow links (MLPPP LFI, lower MTU), DSCP EF priority queueing for what's *waiting*, rate-limiting bulk traffic, avoiding slow links for voice. Test `Link.SerializationDelay`.

**Q12.** Timestamp step = clock × ptime/1000: G.711 20 ms **160**; 30 ms **240**; 16 kHz wideband 20 ms **320** (if the clock = 16 kHz); Opus 20 ms **960** (clock always 48 000); 44.1 kHz stereo L16 20 ms **882** (a *sample frame* = both channels together counts as one clock tick). Different clock than sampling: **Opus** (sampling 8–48 kHz, clock fixed 48 kHz) and **G.722** (samples at 16 kHz but the RTP clock is 8000 for historical reasons ⚠ RFC 3551). Test `Sizing.G711WorkedExample` covers the first.

**Q13.** 44.1 kHz: 1 ms = 44.1 ✗; 5 ms = 220.5 ✗; 10 ms = 441 ✓; 20 ms = 882 ✓. 11.025 kHz: 10 ms = 110.25 ✗; 20 ms = 220.5 ✗; 40 ms = 441 ✓. 22.05 kHz: 20 ms = 441 ✓; 40 ms = 882 ✓. 48 kHz has 48 samples per ms — every whole ms is exact — which is why real-time engines and WebRTC use 48 kHz (10 ms = 480, 20 ms = 960). 44.1 kHz pipelines use 10 ms/20 ms (441/882 samples) or resample. Test `Sizing.SamplesExact`.

**Q14.** (a) 960 samples/channel × 2 ch × 2 B = **3840 B** payload; IP packet = 3840 + 12 + 8 + 20 = **3880 B**. (b) No: 3880 > 1500. (c) Without DF the sender's IP stack fragments: IP payload = 3860 B (UDP+RTP+data), max fragment payload `1500 − 20 = 1480` (a multiple of 8 ✓) → fragments of 1480 + 1480 + 900 bytes (IP packets 1500, 1500, 920): **3 fragments**; only the first has the UDP header, so many NAT/firewalls/QoS classifiers can't match the others and drop them. With DF set, `sendto` fails with `EMSGSIZE` (local MTU) or a router drops it and returns ICMP "fragmentation needed" (Type 3 Code 4). (d) Whole datagram survives only if all 3 do: `1 − 0.99³` = **2.97 %** loss instead of 1 %. (e) Stereo: 192 B/ms → **7 ms** (1344 + 40 = 1384 ≤ 1500; 8 ms = 1536 + 40 ✗); mono: 96 B/ms → **15 ms** (1480); IPv6 stereo: headers 60 → still **7 ms** (1344 + 60 = 1404). (f) Use a codec (Opus ≈ 24–64 kbit/s → 60–160 B per 20 ms), mono if acceptable, or ≤ 7 ms packets (poor overhead: 40 B of headers every 7 ms = 320/7 ≈ 45.7 kbit/s of headers alone). Test `Mtu.The48kStereoTrap`.

**Q15.** (a) G.711 is 8 B/ms; payload ≤ 1500 − 40 = 1460 B → `floor(1460/8)` = **182 ms** (183 ms = 1464 + 40 = 1504 ✗). (b) Not sensible: 182 ms of *packetisation delay alone* already exceeds the ~150 ms G.114 one-way target ⚠; one lost packet erases 182 ms ≈ a whole word; the jitter buffer must hold at least a packet interval; floor-grant to first-audio latency in PTT grows. (c) Systems pick 20 ms (sometimes 10/30/40): overhead drops with diminishing returns (320/ptime kbit/s), loss concealment works on short gaps, delay budget remains for network + jitter buffer + codec lookahead, and codec frame sizes (10/20/30 ms) divide evenly. Test `Mtu.The48kStereoTrap`.

**Q16.** Samples `[0, 1, −1, 0x1234, −32768]`:
```text
WAV (little-endian):   00 00 | 01 00 | FF FF | 34 12 | 00 80
RTP L16 (big-endian):  00 00 | 00 01 | FF FF | 12 34 | 80 00
```
RTP uses **network byte order** (big-endian) like every IETF header, so heterogeneous hosts agree without a negotiation; on a little-endian CPU (x86/ARM) the sender must `htons` and the receiver `ntohs` each sample (or use SIMD byte shuffles). Detecting a swap in a hex dump of a 440 Hz sine: with correct byte order the **high** byte (more significant) changes slowly and smoothly while the low byte looks noisy; if the roles are swapped the *slow* byte is in the wrong position — and audibly the result is loud, harsh noise whose loudness follows the sine's envelope.

**Q17.** A defensible design: keep **each leg at its native rate** (8 kHz for LMR vocoder/G.711 legs, 16 kHz for G.722/AMR-WB, 48 kHz for Opus) and **resample only at the edges of a mix/bridge**, choosing the mix rate as the highest rate that *some participant can actually use* (8 kHz if all legs are narrowband — the common LMR case; 16 kHz when any wideband participant exists). Upsampling (8 → 16/48 kHz) invents no information and requires an interpolation (anti-image) filter; downsampling (48 → 8 kHz) throws away 4–24 kHz of content and **requires an anti-alias low-pass first** (otherwise aliasing folds energy into the voice band). Cost: a polyphase resampler is a few MFLOPS per leg; adds the filter's group delay (≈ 1–3 ms ⚠ design-dependent). Don't convert needlessly: 8 kHz G.711 ↔ 8 kHz vocoder audio needs no resampling at all. A fixed 48 kHz bus is simplest to reason about but wastes CPU (6× more samples) for narrowband-only deployments.

---

## Part F drill answers
1. G.711 30 ms: **240** samples, **240 B** payload, **280 B** IP, **74.67 kbit/s**.
2. L16 16 kHz mono 20 ms: 320 samples × 2 = **640 B**; IP 680 B → **272 kbit/s**.
3. AMR-WB 12.65 kbit/s: 253 bits per 20 ms = 31.6 B → **32 B**; IP 72 B → **28.8 kbit/s** (the real RTP payload formats add CMR/ToC bits: octet-aligned ≈ 34 B ⚠ RFC 4867).
4. Opus 24 kbit/s: 60 B; IP 100 B → **40 kbit/s** (Opus is VBR; figures are the nominal target).
5. IMBE 7.2 kbit/s: 144 bits = **18 B**; IP 58 B → **23.2 kbit/s** (RTP payload format details ⚠ Pack 02).
6. 100 calls × 87.2 kbit/s = **8.72 Mbit/s per direction** (17.44 Mbit/s both); at line-rate figure 9.52 Mbit/s each way.
7. 30 ms → **33.3 pps**; 5 ms → **200 pps**.
8. 3 s G.711: **24 000** samples, **24 000 B**, **150** packets at 20 ms.
9. 5 s × 16 000 Hz × 2 B = **160 000 B** data; file = 160 000 + 44 = **160 044 B**.
10. 11 bits: 6.02 × 11 + 1.76 = **67.98 dB** (≈ 68).

---

## Self-check of the code
* G.711 worked example: 160 → 160 B → 200 B → 80.0 kbit/s; Ethernet frame 218 B → 87.2; wire 238 B → 95.2.
* 48 kHz stereo 16-bit 20 ms: 3840 B payload, 3880 B IP, max ptime 7 ms (mono 15 ms).
* Measured SNR of quantised sine within ~2 dB of `6.02N + 1.76` for N = 6…14 at near-full-scale (0.6 dB at 8 bits).
* 15 tests pass on `solutions/` under ASan/UBSan (g++, WSL); the stubs compile and fail by design.
