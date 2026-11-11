# Pack 05 — Answers (Modulation)

Hand-computed; nothing executed. Standard-derived numbers are marked **VERIFY** (see the worksheet's VERIFY table).

## Part A

**A1.** Bit rate = 4800 × 2 = **9600 bit/s**. Symbol time = 1/4800 = **208.33 µs**. At 48 kS/s: 48 000/4800 = **10 samples/symbol**. Phase step at +1800 Hz: 2π·1800/48000 = 0.2356 rad = **13.5°/sample** (135° per symbol).

**A2.** (P25 convention, VERIFY) 01 → +1800 Hz, 00 → +600 Hz, 10 → −600 Hz, 11 → −1800 Hz. Each held for 208.3 µs: staircase +1800, +600, −600, −1800 Hz. (Dibit mapping is Gray-like: adjacent levels differ in one bit: 01 ↔ 00 ↔ 10 ↔ 11 — each neighbouring pair differs by one bit ✓.)

**A3.** h = spacing × T = 1200 Hz / 4800 = **0.25** (the "modulation index" for 4FSK is defined via the spacing between adjacent levels). MSK is 0.5 for binary FSK (coherent orthogonality). 4FSK uses a *smaller* index → narrower spectrum but tones are not orthogonal, so it relies on the discriminator rather than correlators.

**A4.** Carson: 2(2.5 + 3) = **11 kHz**. β = Δf/fm = 2.5/3 = **0.83** (narrow-band FM, β < 1).

**A5.** Increments (π/4 units): 00:+1, 01:+3, 11:−3, 10:−1, 00:+1 → phases **1, 4, 1, 0, 1** (×45°) = 45°, 180°, 45°, 0°, 45°. Points: (0.707, 0.707), (−1, 0), (0.707, 0.707), (1, 0), (0.707, 0.707). Odd, even, odd, even, odd multiples of 45° ✓.

**A6.** Receiver sees s′[n] = s[n]e^{jθ}. Then s′[n]·conj(s′[n−1]) = s[n]e^{jθ}·conj(s[n−1])e^{−jθ} = s[n]·conj(s[n−1]) — θ cancels. Only the first symbol is affected because it is compared with the (un-rotated) reference.

**A7.** Per-axis levels: 00 → −3/√10 = −0.9487; 01 → −0.3162; 11 → +0.3162; 10 → +0.9487. Point (b3b2, b1b0) = (I, Q) from those levels, e.g. 0000 → (−0.949, −0.949), 1011 → (+0.949, +0.316). Average energy: E[I²] = (2·0.9 + 2·0.1)/4 = 0.5 per axis (0.9 = 9/10, 0.1 = 1/10) → total **1.0** ✓.

**A8.** Δf = 25 000/64 = **390.6 Hz**; T_u = 64/25 000 = **2.56 ms** (= 1/Δf ✓). CP 16 samples = 0.64 ms; overhead 16/(64+16) = **20 %**; channel must satisfy L − 1 ≤ 16, i.e. up to 17 taps ≈ **delay spread ≤ 0.64 ms**.

**A9.** With an N-sample window after a zero guard, the window contains only the linear convolution's *head*, while the tail spills into the next symbol's slot. The window therefore equals the circular convolution minus its wrapped tail — not H·X — so Y[k]/H[k] leaves an error (the missing tail). (Zero-padded OFDM can be rescued by overlap-adding the tail onto the head — extra processing, and the noise on the tail is also added.) A cyclic prefix makes the *channel itself* do the wrap.

**A10.** Correlation of two tones 1200 Hz apart over T = 208.33 µs: sinc-type value sin(2πΔfT)/(2πΔfT) = sin(π/2)/(π/2) = **0.64 ≠ 0** → not orthogonal, a bank of correlators would have cross-talk. The FM discriminator, however, measures the instantaneous frequency (phase derivative) and slices it; separation 1200 Hz vs the frequency-estimate noise is what matters, not orthogonality. This is the trade: a simple non-coherent receiver, with a few dB less sensitivity than optimal.

## Part B — reference notes
* **B1**: `fsk4_level` via switch on `dibit & 3` (00:+1, 01:+3, 10:−1, 11:−3). Modulator: emit `polar(1, phase)` *then* `phase += 2π·level·D/fs` so the first sample of every symbol has the phase accumulated from previous symbols (continuous phase). Wrap phase occasionally to keep precision.
* **B2**: per-symbol average of `arg(x[n+1]·conj(x[n]))` over the symbol's `sps` samples (the last one uses the first sample of the next symbol, so the final symbol has `sps−1` diffs). Slice thresholds at 0 and ±2D. Test shows ±200 Hz CFO is harmless and ±700 Hz breaks the slicer — mean-removal/AFC needed.
* **B3**: Gray QPSK: sign of I ↔ bit 1, sign of Q ↔ bit 0.
* **B4**: keep phase as an integer in units of π/4 modulo 8 (no float drift); the demodulator compares `arg(s·conj(prev))` to each of the four allowed increments (wrap-aware) — hard decision.
* **B5**: `levels = {00: −3, 01: −1, 11: +1, 10: +3}` per axis, scaled 1/√10.
* **B6**: `ofdm_modulate` = IDFT (1/N included) + CP; `ofdm_demodulate` = drop CP + DFT; `ofdm_equalise` divides by DFT of zero-padded channel. Test verifies exactness (< 1e-9) for CP ≥ L−1 and failure for CP too short.
* **B7**: FM and its discriminator are exact inverses (sample-by-sample), as the test checks.

## Part C — model answers
1. 2 bits/symbol, 4800 sym/s ⇒ 9600 bit/s in a 12.5 kHz channel; deviations ±600 Hz (inner) and ±1800 Hz (outer) for the 4 levels (**VERIFY** TIA-102.BAAA).
2. QPSK's 180° transitions pass through the origin (envelope → 0) ⇒ large envelope variation/PAPR ⇒ needs a linear, inefficient PA. π/4-DQPSK limits transitions to ±45°/±135° (min envelope ≈ 0.38 before filtering) and is differential (simple, robust non-coherent detection, no phase recovery, fading-tolerant) — at ≈ 2–3 dB sensitivity cost vs coherent.
3. The channel acts as linear convolution; the DFT turns *circular* convolution into multiplication. Copying the last L−1 samples to the front (CP ≥ channel memory) makes the channel's linear convolution look circular for the N samples kept ⇒ Y[k] = H[k]X[k] ⇒ one complex divide per subcarrier. Costs C/(N+C) overhead.
4. Constant envelope ⇒ the PA can be run saturated (class C/E) at ≈ 60–70 % efficiency instead of backed-off linear — big battery-life and heat benefit; no AM-to-PM distortion issue; spectral regrowth is not a concern.
5. Every demodulated level is shifted by +200 Hz (DC offset on the discriminator output). For D = 600 Hz every level sits 600 Hz from its nearest slicing threshold (0 or ±1200), so a +200 Hz shift still decodes but the worst-case margin drops from 600 to 400 Hz (−33 %, ≈ 3.5 dB less noise immunity); at +600 Hz or more the slicer starts making errors. Fix by AFC: estimate the mean/midpoint of the symbol levels from a preamble or sync (or running average of the discriminator) and subtract before slicing, plus correct the LO.
