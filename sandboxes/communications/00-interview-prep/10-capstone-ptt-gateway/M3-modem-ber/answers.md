# Capstone M3 — Answers (4FSK modem, channel, sync, BER)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic signals only. **VERIFY** = check a reference before quoting.

For the **Story** questions (Q13–Q17) there is no single right answer: a *model answer* lists the points a strong answer must hit. Rewrite it in your own words with your own numbers.

---

## Q1 — Gray mapping

**Answer.** Pairs `10 01 11 00` → tone indices **3, 1, 2, 0** → frequencies **+1.5, −0.5, +0.5, −1.5** cycles/symbol. `10 → 11` is tone 3 → tone 2 (adjacent): one bit flips (`10` vs `11` differ in b0): **1 bit error**. Natural binary (00, 01, 10, 11 → tones 0–3) would map tone 1 (`01`) ↔ tone 2 (`10`) with **2** bit errors on the same adjacent-tone mistake.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Gray mapping saves bandwidth" | It's a "code". | It changes only which bits a symbol error flips, not the signal. |
| Tone index = binary value of the pair | Simplest. | Gives 2-bit damage for some adjacent-tone errors. |

## Q2 — Orthogonality

**Answer.** Correlating tone *a* with tone *k* over 8 samples sums `e^{j2π(a−k)n/8}`: a geometric series over exactly `(a−k)` full cycles, which is 0 for integer `a−k ≠ 0` (the phasors are symmetric around the circle and cancel) and 8 for `a = k`. With spacing 0.8 cycles/symbol the sum is `|sin(0.8π)/sin(0.1π)| = 0.588/0.309 = 1.90`, so leakage energy `1.90² / 64 = 5.7 %`. Tones are no longer orthogonal; the detector sees crosstalk even without noise.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Orthogonal means far apart in frequency" | Intuition of channels. | Orthogonality over *the symbol window* needs integer cycle differences; close spacing (1/T) is fine. |

## Q3 — Calibration numbers

**Answer.** `Es = sps × |x|² = 8`. `σ_comp = √(0.5 × 8 / g)`: 0 dB (g = 1) → **2.0**; 10 dB (g = 10) → **0.632**; 20 dB (g = 100) → **0.2**. Correlator output SNR = signal amplitude 8 → power 64, noise variance `8σ²` with `σ² = 8/g` → `64 / (64/g) = g`: i.e. **0 dB, 10 dB, 20 dB** (equal to Es/N0).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `σ_comp = √(8/g)` | Forgetting I/Q split. | That is the *total* complex σ; each of I and Q has half the variance. |
| Es = 1 | Unit-amplitude sample. | Es sums over the 8 samples of the symbol. |

## Q4 — The missing ½

**Answer.** Noise variance would be 2× too large, so the actual Es/N0 is **3 dB lower** than intended. Symptom: measured BER is far above theory at every point (nominal 10 dB behaves like 7 dB: ≈ 6×10^-2 against 6×10^-3, ten times worse), and the sweep curve is parallel to theory but shifted 3 dB to the right. The noise-power test (`MeasuredNoisePowerMatchesEsN0`) catches it directly.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "6 dB error" | Factor 2 in amplitude. | Variance (power) doubles = 3 dB. |
| "Only affects low SNR" | Small effect at high SNR. | A constant dB shift at every point; at high SNR it changes BER by orders of magnitude. |

## Q5 — CFO numbers

**Answer.** Per-sample rotation `2π × 0.12 / 8 = 0.0942 rad = 5.4°`. `sinc²(0.12) = (sin(0.377)/0.377)² = 0.954 → −0.21 dB`. At 4 800 sym/s: `0.12 × 4800 = 576 Hz`. At δ = 0.5: `sinc² = 0.405 → −3.9 dB` and the neighbouring tone's correlation `sinc²(0.5)` is **identical**: the decision is a coin-flip between two tones (catastrophic).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| CFO in Hz = 0.12 × sample rate | Confusing units. | δ is cycles per *symbol*; multiply by the symbol rate. |

## Q6 — Why 0.6 breaks and tone 3 behaves differently

**Answer.** A CFO of 0.6 moves tone *k* to `k − 1.5 + 0.6`: 0.4 below tone *k+1* and 0.6 above tone *k*. The correlation with the next tone up is larger (`sinc²(0.4) = 0.57`) than with its own (`sinc²(0.6) = 0.25`), so the decision slides **up one tone**. Tone 3 has no tone above it: its energy at its own tone remains the largest of the four (`0.25` vs `0.04` for tone 2), so it's *still correct*. So about 3 of 4 symbols are wrong (test: > 60 %).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Every symbol is wrong" | CFO breaks things. | The edge tone has no neighbour to slide to. |
| "0.6 > 0.5 so only half" | Mixed threshold. | Three of four tones have a closer neighbour above. |

## Q7 — Sync metric

**Answer.** Aligned, no noise: each window correlates to 64 → sum `12 × 64 = 768`; denominator `8 × (12 × 8 = 96) = 768` → **1.0**. Noise only: a random window's energy is spread evenly over the `sps = 8` orthogonal directions of the window, so the expected fraction that lands on the one expected tone is `1/sps = 0.125` (the metric's noise floor). 0.5 sits four times above the noise mean and well below 1.0, so noise alone rarely reaches it (test `NoiseAloneDoesNotTrigger`) while a signal at 10 dB with CFO 0.1 reaches > 0.6. Lowering it to 0.4 catches weaker or CFO-degraded syncs (the chain test) at the price of more false alarms in noise, which then must be rejected by M1's header decode and CRC.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Threshold 0.9 for safety" | Wants certainty. | SNR/CFO loss lowers the true peak; you'd miss frames at exactly the SNR where they're decodable. |

## Q8 — CFO estimator

**Answer.** `r[n+1] conj(r[n]) = s[n+1] conj(s[n]) · e^{j2πδ/sps}` (carrier phase and the common `e^{jθ}` cancel). Multiplying by `conj(s[n+1] conj(s[n]))` (unit modulus) leaves `e^{j2πδ/sps}` for every n, plus noise; summing coherently averages the noise; the angle is `2πδ/sps = 0.0942 rad` for δ = 0.12; multiply by `sps/(2π)` → **0.12 cycles/symbol**. The angle is unambiguous within ±π → `|δ| < sps/2 = 4` cycles/symbol.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Take the angle of the sum of r" | Simplest. | The data modulation rotates r by up to ±1.5 cycles/symbol; you must remove it with the known symbols first. |

## Q9 — Chain order

**Answer.** (1) Find sync (metric, non-coherent, tolerates CFO) → (2) estimate CFO using the sync's known symbols at that start → (3) derotate a copy → (4) demodulate `total_bits/2` symbols from start → (5) Gray decode. Estimation needs the start; demodulation needs CFO removed. At δ = 0.45 the right tone has `sinc²(0.45) = 0.49` and the neighbour `sinc²(0.55) = 0.33`: any noise flips the decision, so a large fraction of symbols is wrong. Derotate a **copy** so the caller's samples stay intact (re-run with another threshold, log the raw capture, reuse the same buffer in tests, no hidden side-effects).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "CFO is small, demodulate first" | Simpler. | The decision margin vanishes near 0.5. |
| "Derotate in place to save memory" | 9 352 samples is small. | Aliasing bugs; callers surprised by modified data. |

## Q10 — Closed form at 8 dB

**Answer.** `g = 10^0.8 = 6.31`. `e^{−g/2} = e^{−3.155} = 0.04268` → `1.5× = 0.06402`; `e^{−2g/3} = e^{−4.207} = 0.014893`; `e^{−3g/4} = e^{−4.732} = 0.008806` → `0.25× = 0.002202`. `Ps = 0.06402 − 0.014893 + 0.002202 = 0.05133`. `Pb = (2/3) Ps = 0.03422` ✓ (table: 3.418×10^-2). Eb/N0 = 8 − 3.01 = **4.99 dB**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Using g = 8 | Mixing dB with linear. | Convert: `g = 10^(dB/10)`. |
| Pb = Ps | Forgetting the bit/symbol factor. | A symbol error flips 2 of 3 equally likely wrong symbols' bits on average: `Pb = 2/3 Ps`. |

## Q11 — Bit budgets

**Answer.** `bits = 100 / BER`.

| BER | bits | M1 frames (÷ 2338) | Audio time at 20 ms |
|---|---|---|---|
| 10^-3 | 10^5 | ≈ 43 | 0.9 s |
| 10^-6 | 10^8 | ≈ 42 800 | ≈ 14.3 min |
| 10^-9 | 10^11 | ≈ 4.28×10^7 | ≈ 9.9 days |

At 100 simulated frames/s the 10^-9 test takes `4.28×10^7 / 100 = 4.28×10^5 s ≈ 5 days`. Lesson: you cannot *measure* 10^-9 by brute-force simulation; use importance sampling, extrapolation from a tested model, or confirm theory at measurable BERs and trust it below.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "10 × 1/BER bits are enough" | 10 errors feels enough. | 10 errors gives ±30 %+; ~100 is the conventional minimum for ±20 %. |

## Q12 — Zero errors at 14 dB

**Answer.** Theory at 14 dB is 3.5×10^-6, so 2×10^5 bits gives an *expected* 0.7 errors; seeing 0 has probability `e^{−0.7} ≈ 50 %`. You can't conclude BER = 0. The rule of three: with 0 errors in N bits, BER < 3/N at 95 % confidence = `3/(2×10^5) = 1.5×10^-5`, which is above theory, so the run is consistent with it but uninformative. To expect 100 errors you need `100 / 3.5×10^-6 = 2.9×10^7` bits.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The modem is perfect above 14 dB" | The table shows 0. | Absence of evidence at a sample size too small. |

## Q13 — Story: non-coherent 4FSK

**Model answer.** *Constant envelope* lets the PA run saturated (efficient; handheld battery life, low heat) without spectral regrowth that would hurt adjacent channels; *non-coherent detection* removes carrier-phase recovery (simple, robust to fast fades and cheap oscillators); orthogonal spacing means detection is a bank of 4 correlators. Cost: ~1 dB penalty vs coherent (VERIFY) and low spectral efficiency (toy uses ~4–5 × the symbol rate of bandwidth; roughly 0.4 bit/s/Hz). Alternatives: QPSK/16-QAM (spectral efficiency ×2–4 but need linear PA and phase tracking), narrow-deviation CPFSK/GMSK (spectrum efficient, discriminator detection, not orthogonal). Choice depends on whether the system is power-limited (FSK) or spectrum-limited (linear). *What I'd change:* reduce the deviation toward a standard-like value and re-derive BER by simulation.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "FSK because it's old" | Legacy. | Constant envelope + no phase recovery are engineering reasons. |

## Q14 — Story: Gray mapping with orthogonal tones

**Model answer.** Derivation: orthogonal equal-energy tones in AWGN make all `M−1 = 3` wrong symbols equally likely; for a given bit, 2 of the 3 wrong symbols flip it → `Pb = (M/2)/(M−1) × Ps = 2/3 Ps`: independent of the mapping, so Gray doesn't improve *theory* here. It helps when errors are *not* equiprobable: CFO leakage to the neighbour (Q6), non-orthogonal narrow deviation (adjacent tones nearly indistinguishable), discriminator receivers. Cost is zero. It also feeds M1 well: more single-bit errors per symbol = fewer double errors inside one Hamming word. *What I'd say honestly in the interview:* "I used Gray because it's free insurance; the test `GrayCodeAndRoundTrip` pins the property, but in the simulated AWGN BER the mapping doesn't change the average."

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Gray reduces BER by 2× here" | Textbook for PSK. | True for PSK/QAM with adjacent-symbol errors; for orthogonal AWGN it's neutral. |

## Q15 — Story: CFO tolerance

**Model answer.** Hard limit: half the tone spacing (δ = 0.5 cycles/symbol = half the symbol rate); practical limit well below, because `sinc²` loss (−0.58 dB at 0.2) eats the noise margin and leakage adds crosstalk. Source: oscillator tolerance × carrier, e.g. ±2 ppm at 450 MHz = ±900 Hz at each end (up to ~1.8 kHz relative; VERIFY real figures), which is only acceptable if the tone spacing (= symbol rate) is several kHz. Design: estimate from the known sync (differential estimator, unambiguous to ±4 cycles/symbol, accuracy ~0.03 at 15 dB), derotate, then decode. Residual: estimate error and *drift during the frame* (9352 samples) — the open item; a decision-directed tracking loop or re-estimation from pilot/sync insertion is the fix. I verified with the `CfoEstimateIsAccurate` and chain tests at ±0.15 and +0.3.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "ppm is tiny, ignore" | 2 ppm sounds small. | At 450 MHz it's a kilohertz. |

## Q16 — Story: Es/N0 vs Eb/N0 and proof of correctness

**Model answer.** *Units:* simulator calibrated in Es/N0 (Es = 8 = sps, `N0 = σ²`), comparisons across modulations in Eb/N0 = Es/N0 − 3.01 dB for 4FSK. *Proof:* (1) calibration test: measured noise power within 3 % of `8/g`; (2) closed-form BER values pinned (8 dB → 3.42×10^-2; 12 dB → 3.46×10^-4); (3) simulated BER within 0.8–1.25 × theory at 40 000 bits (≈ 640 errors at 9 dB, ≈ 3900 at 6 dB, so ±4 % standard error); (4) deterministic xorshift/Box-Muller RNG so CI is reproducible across platforms (not `std::normal_distribution`, which differs between libraries). *Traps I name:* factor-2 per component, zero errors ≠ zero BER (rule of three), Eb vs Es.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use `std::normal_distribution` for noise" | Standard library. | Implementation-defined sequences; tests become platform-dependent. |

## Q17 — Story: sync threshold and receiver structure

**Model answer.** Noise floor of the metric ≈ 0.125; true peak ≈ 1.0 minus losses (≈ −0.2 dB CFO, noise at 10 dB reduces the peak to 0.6–0.9); threshold 0.5 sits between with margin on both sides; 0.4 for the low-SNR test accepts weaker peaks (more false alarms, which M1 rejects via Golay/CRC, cost = CPU). Structure: one-shot sync at the start, one CFO estimate from 96 samples (1 % of the frame): cheap, but vulnerable to drift and to a missed sync (lose whole frame). Alternatives: re-sync each frame (we do — every frame has its own sync), mid-frame pilots, a PLL/AFC. The CRC remains the final arbiter: a false sync, a mis-estimated CFO and a noisy channel all end in `CrcFail`, never silent wrong audio.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Higher metric = always better frame" | Peak 1.0 feels right. | The threshold is a false-alarm vs miss trade; CRC covers the residual. |

## Q18 — Integration

**Answer.** Need BER ≲ 10^-3 for M1 frame loss < 1 % (0.7 %) → **Es/N0 ≈ 11.4 dB** (Eb/N0 ≈ 8.4 dB); with margin for sync misses, header errors and burst effects (not modelled), plan for ≈ 12 dB (0.08 % for Hamming alone). Export to M4: sync metric (link quality), CFO estimate (oscillator health/alignment), per-frame `payload_corrections + header_corrections` (a BER proxy, since corrected errors are counted) and `CrcFail`/`SyncFail` counts; compute `ber = bit_errors / bits_checked`. CI: seeded channel simulator (AWGN + CFO + delay), golden BER table within tolerance, ASan/UBSan, fuzz the receiver with random IQ and noise-only buffers. **No live public-safety signals ever:** unlawful or restricted in many jurisdictions (UK Wireless Telegraphy Act 2006), unnecessary because the channel is simulated, and non-reproducible.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Add a recorded capture to the regression set" | More realistic. | Legal and ethical exposure, not reproducible, and a simulator is sufficient for the logic. |
