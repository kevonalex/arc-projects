# Answers — 04-audio / 02 Codecs

Attempt first. ⚠ = verify against the standard named. Numbers marked "(run)" were produced by running the solution code.

---

**Q1.** (i) Want error ∝ signal: step `Δ(x) = k·x`. The number of steps between `x₀` and `x` is `∫ dx/(kx) = (1/k)·ln(x/x₀)` — i.e. equally spaced steps **in `ln x`**: compress with a log, quantise *uniformly*, expand with `exp`. (ii) `ln|x|` → −∞ at 0 and the range is unbounded below; adding 1 inside, `ln(1 + μ|x|)`, passes through 0 at x = 0, is linear near 0 (`≈ μx`: fine, uniform steps for tiny signals) and logarithmic for `μ|x| ≫ 1`. Normalising by `ln(1 + μ)` makes `F(1) = 1`. (iii) μ = 255: `F(0.001) = 0.0410`, `F(0.01) = 0.2285`, `F(0.1) = 0.5910`, `F(1) = 1` (run) → of the 128 magnitude codes: 5.2, 29.2, 75.7, 128. So 1 % of full scale uses 23 % of the code range and 10 % uses 59 %: small signals are quantised finely. Test `Mulaw.ContinuousCurveMatchesStandard` also shows the 8-segment standard tracks this curve within 6 codes.

**Q2.** (run to confirm)
| x | x + 132 | highest bit | E | M | byte (pre-inv → wire) | decoded | error |
|---|---------|-------------|---|---|-----------------------|---------|-------|
| 100 | 232 = 0xE8 | bit 7 (none above) | 0 | `232>>3 & 15` = 29 & 15 = 13 | 0x0D → **0xF2** | `(13·8 + 132) − 132` = 104 | +4 |
| 1000 | 1132 = 0x46C | bit 10 | 3 | `1132>>6 & 15` = 17 & 15 = 1 | 0x31 → **0xCE** | `140·8 − 132` = 988 | −12 |
| −1000 | same | bit 10 | 3 | 1 | sign|0x31 = 0xB1 → **0x4E** | −988 | +12 |
| 30000 | 30132 = 0x75B4 | bit 14 | 7 | `30132>>10 & 15` = 29 & 15 = 13 | 0x7D → **0x82** | `236·128 − 132` = 30076 | +76 |
| 0 | 132 = 0x84 | bit 7 | 0 | `132>>3 & 15` = 16 & 15 = 0 | 0x00 → **0xFF** | 0 | 0 |
| 8 | 140 = 0x8C | bit 7 | 0 | `140>>3 & 15` = 17 & 15 = 1 | 0x01 → **0xFE** | 8 | 0 |
| 32767 | clip 32635 + 132 = 32767 | bit 14 | 7 | `32767>>10 & 15` = 31 & 15 = 15 | 0x7F → **0x80** | 32 124 | −643 (clipped) |
The `& 15` removes the implicit leading 1 of the highest set bit (a "floating-point" view: mantissa without the hidden bit). Test `Mulaw.KnownCodes`.

**Q3.** `level(E, M) = ((M << 3) + 132) << E − 132`.
* E = 0: M=0 → 0; M=15 → 120 + 132 − 132 = **120**; step 8 (`1<<3`).
* E = 1: M=0 → `132·2 − 132` = **132**; M=15 → `(120+132)·2 − 132` = **372**; step 16.
* E = 2: M=0 → `132·4 − 132` = **396**; M=15 → `252·4 − 132` = **876**; step 32.
* E = 7: M=0 → `132·128 − 132` = **16 764**; M=15 → `252·128 − 132` = **32 124**; step 1 024.
The step doubles because `<< E` scales the whole `(M<<3)` ladder. **Why 132 = 128 + 4, not 128?** The **128** makes the segment boundaries of `x + bias` exact powers of two (`x + 132 ∈ [2^k, 2^(k+1))` ⇒ segment `k − 7`), so a single highest-set-bit search (count-leading-zeros) finds E and the 4 bits below the leading 1 are M. The extra **4 is half the smallest step (8)**: it makes the encoder *round to nearest* rather than truncate — segment 0 levels are `8M`, covering `8M − 4 … 8M + 4`; in segment 1 the level 132 covers 124–140, and so on. Without the extra 4 the decoder's output would sit at the bottom of each cell and the error would be up to a full step (and biased towards zero) instead of ± half a step.

**Q4.** (a) 256 codes but code `0xFF` (sign +, E=0, M=0) and `0x7F` (sign −, E=0, M=0) both decode to ±0 → 255 levels. (b) `decode(0x7F) = −0 = 0`, then `encode(0)` = 0xFF (positive); every other code survives because each level is a quantiser output and `encode(level)` lands in the same step. (c) The `+4` in the bias makes the encoder round to the nearest level, so within a segment the error is ≤ half the step `2^(E+3)`; the biggest step is 1 024 (segment 7) → ≤ 512, and the test confirms the worst case over all non-clipped inputs is exactly 512. (d) Both `E` and `M` increase with magnitude, so the code ordering is monotonic and decode preserves it: no code reordering. (e) The sign is a separate bit applied *before* the (symmetric) magnitude coding, so `encode(−x)` differs from `encode(x)` only by the sign bit — `^ 0x80` in the inverted domain (x ≥ 1). A gateway should treat `0x7F` as silence and normalise to `0xFF` (some encoders even emit `0x7F` for small negative values: `encode(−1) = 0x7F`). Tests: `Mulaw.ExhaustiveCodeRoundTrip`, `ExhaustiveInputQuantisationError`, `SignSymmetry`.

**Q5.** (run) 1 013 Hz sine: 
| level | μ-law | 8-bit linear |
|-------|-------|--------------|
| −10 dBFS | 37.2 | 39.7 |
| −15 | 36.9 | 34.7 |
| −20 | 38.2 | 29.8 |
| −30 | 34.9 | 20.6 |
| −40 | 35.2 | 11.1 |
| −50 | 30.1 | 0.0 |
Linear SNR falls dB-for-dB with level (`Δ²/12` is constant); μ-law steps scale with the signal so its SNR stays 30–38 dB over ~40 dB of dynamic range. The curves cross near **−12 dBFS**: above that (loud) linear is up to ≈ 7 dB better (μ-law's step at the top is 1 024 = coarse), below it companding wins by up to 30 dB. Speech has a high *crest factor* (peaks 12–20 dB above the average) and spends most of its time well below full scale, so the log law matches the signal statistics; the mid/low levels determine intelligibility. Test `Mulaw.CompandingBeatsLinearAtLowLevels`.

**Q6.** `x = 1000`: `v = 1000 >> 3 = 125`; segments end at 0x1F, 0x3F, 0x7F, … → `125 ≤ 0x7F` ⇒ seg 2; `a = 2<<4 | (125 >> 2) & 15 = 0x20 | 31&15 (=15) = 0x2F`; positive → mask 0xD5: `0x2F ^ 0xD5 = 0xFA`. Decode: `0xFA ^ 0x55 = 0xAF`; `t = (0xF)<<4 = 0xF0`; seg 2: `t += 0x108 → 0x1F8; t <<= 1 → 0x3F0 = 1008`. (μ-law gave 988: both within their step: A-law's 16-bit step in segment 2 is 32.) A-law silence byte **0xD5** decodes to **+8** (no true zero). Transcode: μ 0xFF (0) → A encode(0) = 0xD5 ✓; but A 0xD5 (+8) → μ encode(8) = **0xFE** (not 0xFF): so an A-law silent line becomes a μ-law line at +8 (−72 dBFS DC offset) — and `PCMU → PCMA → PCMU` is not the identity for **16 of the 256 codes**, all in the lowest segment, e.g. `0xFF → 0xD5 → 0xFE`, `0xFD(16) → 0xD4(24) → 0xFC(24)`, `0xFB(32) → 0xD7(40) → 0xFA(40)` (run): the two quantisers' lattices are offset by 8 at low levels, so each hop adds up to a step of error — **avoid repeated transcoding in a chain**. Tests `Alaw.KnownCodes`, `Interworking.Transcode`.

**Q7.** Encoder (≈ 10 lines): see `solutions/codecs.hpp::mulaw_encode`. Bugs: (1) `-x` for `x = −32768` overflows `int16_t` (do it in `int`); (2) no clip → `x + 132` overflows 15 bits → wrong segment/wrap for loud input; (3) forgetting the final `~` — silence becomes 0x00 and sounds like a full-scale negative clip; (4) right-shifting a negative `int` is implementation-defined (arithmetic usually); use magnitude (`|x|`) before shifting, or unsigned arithmetic. Table version: encode table = 65 536 × 1 B = **64 KiB** (fits L2 but not L1: one random-ish 64 KiB access per sample can thrash L1 with a jitter buffer around); decode table 256 × 2 B = **512 B** (always L1) → use a table for decode, a branchless count-leading-zeros (`__builtin_clz`) for encode, and SIMD (AVX2 `vpmaddwd`/vector lzcnt or gather for decode) for batch work. Load: 5 000 calls × 2 directions × 50 pps × 160 samples = **80 M samples/s** ≈ well below 1 core with a table decode (about 1 ns/sample) — so *not* the bottleneck; the network stack and jitter buffers are.

**Q8.**
| Codec | PT | Sampling | Clock | Bitrate | 20 ms payload | TS step (20 ms) |
|-------|----|----------|-------|---------|---------------|------------------|
| PCMU / PCMA | 0 / 8 | 8 k | 8000 | 64 k | 160 B | 160 |
| G.722 | 9 | 16 k | 8000 | 64 k | 160 B | **160** (not 320) |
| G.729 | 18 | 8 k | 8000 | 8 k | 2 × 10 = 20 B | 160 |
| GSM-FR | 3 | 8 k | 8000 | 13 k | 33 B (260 bits) | 160 |
| AMR 12.2 | dyn | 8 k | 8000 | 12.2 k | 31 B (244 bits) + header | 160 |
| AMR-WB 23.85 | dyn | 16 k | 16000 | 23.85 k | 60 B (477 bits) + header | 320 |
| Opus | dyn | ≤ 48 k | 48000 | VBR | variable | 960 |
G.722 carries twice the samples in the same 160 bytes (2 bits/sample for 16 kHz audio = 64 kbit/s). Tests `CodecTable.FramesAndTimestamps`, `NominalPayloadBytes`.

**Q9.** Reasons: (1) the clock must be constant for the session though Opus switches bandwidth/sampling mid-stream; (2) 48 kHz is the maximum and an **integer multiple of every Opus sampling rate** (8, 12, 16, 24, 48), so all frame sizes are an exact number of ticks; (3) the receiver's jitter/playout/RTCP maths then never needs renegotiation or rounding, and WebRTC mixers can treat all Opus streams alike. Increments: 2.5 ms **120**; 5 ms **240**; 10 ms **480**; 20 ms **960**; 40 ms **1 920**; 60 ms **2 880**. A receiver that computes jitter or playout from the 16 kHz it decodes at will mis-scale by 3×: jitter reported in "timestamp units" must be divided by **48 000**, and a fixed 16 kHz assumption puts the playout scheduler three times too early/late (underruns or growing buffers). G.722: 16 kHz sampling but clock 8000 → 160 ticks per 20 ms (decoder produces 320 samples). Mono narrowband Opus: `a=rtpmap:111 opus/48000/2` and `a=fmtp:111 maxplaybackrate=8000;stereo=0;sprop-stereo=0` ⚠ (parameters per RFC 7587). Test `CodecTable.ClockVersusSampleRate`.

**Q10.** G.729 payload = frames × 10 B: 10 ms → 10 B, 20 → 20, 30 → 30, 40 → 40. IP packet = payload + 40: 50, 60, 70, 80 B → 40.0, 24.0, 18.67, 16.0 kbit/s; header/payload ratio 4.0, 2.0, 1.33, 1.0. 20 ms is the compromise: delay (+10 ms vs 10 ms packets), loss unit (20 ms), overhead 24 kbit/s vs 40. Algorithmic delay 15 ms = 10 ms frame + 5 ms look-ahead (the encoder needs the next 5 ms to estimate the LPC filter) ⚠. Annex B (G.729B): VAD + DTX + CNG — during silence it sends a 2-byte SID frame ⚠ instead of speech frames; SDP: `a=fmtp:18 annexb=yes|no` — **both sides must agree** (a mismatch yields confusing silence or garbage); default is yes per RFC 3551 ⚠.

**Q11.** AMR-WB mode 12.65: 253 bits/frame → 31.6 B → **32 B** nominal payload; IP = 72 B; 72 × 8 / 20 = **28.8 kbit/s**. Mode 23.85: 477 bits → **60 B** (59.6 rounded up); IP = 100 B → **40 kbit/s**. The octet-aligned payload adds a **CMR** (4 bits + 4 padding = 1 B) and a **ToC** entry per frame (1 B ⚠): 32 + 2 = 34 B for 12.65 (bandwidth-efficient mode packs these in ~10 bits: ≈ 33 B) ⚠ RFC 4867. Under congestion the network/receiver picks a lower mode (e.g. 6.6 or 8.85 kbit/s) and tells the sender by putting the desired mode in the **CMR** field of the packets going the other way; the encoder follows. Tests `CodecTable.NominalPayloadBytes` (AMR-WB 20 ms → 60).

**Q12.** IMBE: 144 bits / 0.020 s = **7 200 bit/s**; voice 88 bits / 0.020 = **4 400**; FEC 56 / 0.020 = **2 800**; code rate = 88/144 = **0.611** (56 parity bits protect the most important voice bits using Golay/Hamming codes ⚠). LDU: 9 frames × 20 ms = **180 ms**; 9 600 bit/s × 0.18 = **1 728 bits**; voice+FEC = 9 × 144 = **1 296**; overhead = 1 728 − 1 296 = **432 bits = 25.0 %** (frame sync 48 bits, NID 64, status symbols 2 bits per 70, link control/low-speed data/encryption sync ≈ the rest ⚠). Tests `Vocoders.Imbe`, `Vocoders.P25LogicalLinkDataUnit`. (Voice message structure: HDU, LDU1, LDU2, … TDU; VERIFY in TIA-102.BAAA.)

**Q13.** AMBE+2 half-rate: 72 bits / 20 ms = **3 600 bit/s**; voice 49 bits → **2 450 bit/s**; FEC 23 bits → **1 150 bit/s**; 1 frame = 72/8 = **9 B**; a DMR burst of 3 frames = 216 bits = **27 B** (60 ms of speech). TETRA ACELP: 137 bits / 0.030 s = 4 566.67 → **4 567 bit/s**; 1 frame = 137 bits = 17.125 → **18 B** (7 padding bits); 2 frames = 274 bits = 34.25 → **35 B** (6 padding bits). Tests `Vocoders.AmbePlus2HalfRate`, `Vocoders.TetraAcelp`. (Check slot/superframe structure in ETSI EN 300 395-2 ⚠.)

**Q14.** (IPv4/UDP/RTP = 40 B)
| Case | Payload | IP packet | ptime | IP kbit/s | Header/payload |
|------|---------|-----------|-------|-----------|----------------|
| IMBE 1 frame | 18 B | 58 B | 20 ms | **23.2** | 2.2 |
| AMBE+2 1 frame | 9 B | 49 B | 20 ms | **19.6** | 4.4 |
| AMBE+2 3 frames | 27 B | 67 B | 60 ms | **8.93** | 1.5 |
| TETRA 1 frame | 18 B | 58 B | 30 ms | **15.47** | 2.2 |
| TETRA 2 frames | 35 B | 75 B | 60 ms | **10.0** | 1.14 |
For low-rate vocoders, the **headers dominate**: a 3.6 kbit/s AMBE+2 stream costs 19.6 kbit/s at one frame per packet (5.4× its rate!). Bundling 3 frames cuts that to 8.9 kbit/s at the price of +40 ms delay and 60 ms lost per lost packet — a real trade-off for radio backhaul (and the reason some links use header compression, e.g. ROHC/cRTP ⚠). Also remember L2 overhead (Ethernet 18 B) and SRTP tag.

**Q15.** Path: TETRA frames (137 bits / 30 ms, 2 per 60 ms slot) → **ACELP decode** → PCM 8 kHz (frames of 240 samples) → **re-time** into 160-sample (20 ms) blocks → **IMBE encode** (needs 20 ms of PCM + look-ahead ⚠) → 144-bit frames; reverse direction mirror image. Re-timing: a 30 ms frame boundary only coincides with a 20 ms boundary every **60 ms** (3 IMBE ↔ 2 ACELP), so a block FIFO of ≥ 1 frame is needed: **minimum added latency ≈ ACELP frame (30 ms) + IMBE frame (20 ms) + both codecs' look-ahead (few ms) + packetisation (20–60 ms) + jitter buffer (20–60 ms)** ≈ 100–180 ms one-way for the gateway alone (⚠ design-dependent). Quality: two lossy vocoders in tandem (artefacts compound; FEC protects only each radio leg) → keep PCM at 8 kHz, avoid a third codec. Licensing: both vocoders are licensed (DVSI/ETSI-IPR) → hardware DSP boards or licensed libraries; budget per-channel cost and CPU. Log: frame loss, re-timing underruns/overruns, codec errors, latency per leg. **Encrypted media:** the vocoder payload is encrypted end-to-end (AES on P25/TETRA E2EE ⚠); without the keys the gateway sees only noise-like bits and cannot decode or re-encode → options: key-holder gateway (high trust), or no interworking for that talkgroup (fail closed). Pack `03-net/05` Q22.

**Q16.** Free to use (⚠ VERIFY): **G.711** (no royalties); **Opus** (royalty-free, BSD-licensed reference); **G.729** (patents expired by 2017; implementations historically licensed ⚠); **AMR/AMR-WB** (patent pool licences; free reference code under restrictive terms ⚠); **IMBE, AMBE+2** (proprietary; DVSI hardware or licensed software; some older IMBE patents have expired ⚠, but the *implementation* is controlled); **TETRA ACELP** (ETSI standard with IPR declarations ⚠; commercial codecs exist). Test without live radio: (1) synthetic PCM (sines, chirps, silence, speech *you own*) → encode → decode and compare with PESQ/POLQA/SNR; (2) standards bodies' **conformance test vectors** (ITU-T G.191 STL for G.711/G.729/AMR; 3GPP TS 26.074/26.174 test sequences; ETSI/TIA vectors ⚠); (3) loopback through a licensed DVSI test board; (4) fuzz decoders with random frames. No live public-safety traffic (WT Act 2006).

---

## Self-check of the code
* `mulaw_encode(1000) = 0xCE → 988`; `alaw_encode(1000) = 0xFA → 1008`; decode ranges ±32 124 / ±32 256.
* Exhaustive μ-law: 255 distinct levels; max error 512; monotonic; sign-symmetric. Exhaustive A-law: 256 distinct levels, exact code round trip.
* IMBE 7 200 bit/s; LDU 1 728 / 1 296 / 432 bits; AMBE+2 3 600; ACELP 4 567.
* 25 tests pass on `solutions/` under ASan/UBSan (g++, WSL); the stubs compile and all 25 fail by design.
