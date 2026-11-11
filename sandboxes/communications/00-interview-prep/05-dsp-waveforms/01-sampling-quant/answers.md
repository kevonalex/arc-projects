# Pack 01 — Answers (Sampling, Aliasing, Quantisation)

Try the worksheet first. Numbers below were worked by hand; no code was executed to produce them.

## Part A

**A1. Alias table, fs = 8 kHz**  (rule: r = |f| mod fs; if r > fs/2 → fs − r)

| true f | r | apparent |
|---|---|---|
| 3 kHz | 3 | 3 kHz |
| 5 kHz | 5 > 4 | 8−5 = 3 kHz |
| 7 kHz | 7 > 4 | 1 kHz |
| 13 kHz | 13 mod 8 = 5 | 3 kHz |
| 16.5 kHz | 16.5 mod 8 = 0.5 | 0.5 kHz |
| 4 kHz | 4 (= fs/2) | 4 kHz (phase-dependent amplitude!) |

**A2.** fs = 48 kHz: 30 > 24 → 48 − 30 = **18 kHz**. fs = 44.1 kHz: 30 kHz > 22.05 → 44.1 − 30 = **14.1 kHz**. Lesson: the alias location depends on the clock, so a spur that moves when you change the sample rate is an alias, not a real signal.

**A3.** Δ = 2 V / 2¹² = 4.883×10⁻⁴ V. Noise power Δ²/12 = 2.384×10⁻⁷ / 12 = 1.987×10⁻⁸ V². Signal power A²/2 = 0.125 V². SQNR = 0.125 / 1.987×10⁻⁸ = 6.29×10⁶ → **68.0 dB**. Theory for full scale = 6.02·12+1.76 = 74.0 dB; half-amplitude costs 20 log₁₀(0.5) = −6.02 dB → 68.0 dB ✓.

**A4.** Full-scale sine: N ≥ (90 − 1.76)/6.02 = 14.66 → **15 bits**. With 12 dB back-off: (90 + 12 − 1.76)/6.02 = 16.65 → **17 bits**.

**A5.** Condition 2f_H/k ≤ fs ≤ 2f_L/(k−1):
* k = 5: 52/5 = 10.4 ≤ fs ≤ 42/4 = 10.5 MHz (100 kHz wide window).
* k = 4: 52/4 = 13 ≤ fs ≤ 42/3 = 14 MHz → **valid** (wider window = easier clock/filter, but 30 % higher rate).
* (k = 3: 17.33–21 MHz; k = 2: 26–42 MHz; k = 1: ≥ 52 MHz.)
Trade-off: lower fs ⇒ narrower window ⇒ tighter analogue filter + clock accuracy.

**A6.** sin(π n + φ) = (−1)ⁿ sin φ. φ = 0 → all zeros. φ = π/2 → ±1 (a cosine). Moral: at exactly fs/2 amplitude is phase-dependent, hence "strictly greater than 2f".

**A7.** OSR = 16 → 10 log₁₀ 16 = 12.04 dB ≈ 2 equivalent bits (12.04/6.02 = 2). Only valid when the error is white and the signal band is B = fs/(2·OSR).

**A8.** Truncation error e ∈ [−Δ, 0], uniform: mean −Δ/2, variance Δ²/12, **mean-square (power) = Δ²/3** (E[e²] = ∫₋Δ⁰ e²/Δ de = Δ²/3). The AC noise power is still Δ²/12, but there is now a **DC offset of Δ/2**. In radio, DC offset matters (Pack 09: it makes a spur at the carrier of a zero-IF receiver), which is why rounding (mid-tread/mid-rise with unbiased rounding) is preferred.

## Part B — notes on the reference solutions (`code/solutions/`)

* **B1** `alias_frequency`: `r = fmod(fabs(f), fs); if (r > fs/2) r = fs - r;`. `fabs` is legitimate for a real tone because cos is even and sin only flips sign.
* **B2** `sample_sine`: `sin(2π f n / fs + phase)`. Compute `n/fs` as a double; do not accumulate phase in float for long runs (rounding drift) — a theme in Pack 03/05.
* **B3** `quantise_uniform`: step Δ = 2·FS/2^bits; `k = floor(x/Δ + 0.5)`; clamp k to [−2^(N−1), 2^(N−1)−1]. Note the asymmetric range: +FS itself is **not** representable (max = FS − Δ) — same as two's complement.
* **B4** `measured_sqnr_db` uses amplitude A = FS − Δ so the peak never clips, and a frequency of 0.123456789 cycles/sample (incommensurate with the clock ⇒ error ≈ white) to match the assumptions of the uniform-noise model. With 65 536 samples the result lands within ~0.1 dB of 6.02N+1.76 − 20log₁₀(A/FS).
* **B5** `bandpass_fs_min`: loop k from ⌊f_H/B⌋ down to 1, return first 2f_H/k that does not exceed 2f_L/(k−1). Edge case k = 1 means plain baseband sampling (fs ≥ 2f_H).

## Part C — model answers

1. **Aliasing**: sampling makes the spectrum periodic with period fs; energy above fs/2 lands on top of lower frequencies, indistinguishably, as f + k·fs share the same sample values. Prevent with an analogue low-pass (or band-pass) filter ahead of the ADC attenuating everything above fs/2 by more than the required dynamic range, then oversample so the filter transition band is cheap; in the digital domain use a filter before every decimation.
2. **Derivation**: Δ = 2A/2ᴺ; uniform error → Δ²/12; sine power A²/2; ratio = 1.5·2^(2N); in dB 1.76 + 6.02 N. State assumptions (busy signal, no clipping, white error).
3. **72 dB on a 14-bit ADC (86 dB ideal)**: (i) signal not full scale / AGC mis-set; (ii) PAPR headroom; (iii) ADC thermal/kT/C noise; (iv) aperture jitter (limits SNR at high input frequency: SNR_jitter = −20 log₁₀(2π f σ_t)); (v) clock/power-supply noise & spurs; (vi) DNL/INL ⇒ ENOB < N; (vii) measuring over a wider bandwidth than the signal.
4. **Band-pass sampling**: signal occupies a narrow band [f_L, f_H]; sampling at 2f_H/k ≤ fs ≤ 2f_L/(k−1) folds it intact (no overlap with its mirror) to a low IF; savings in rate, cost: needs a sharp analogue band-pass filter (all other bands also fold in), low aperture jitter (the ADC front-end must still track f_H), tight clock window, possible spectral inversion.
5. **Phantom tone moving opposite to the tuning knob**: the true tone is above fs/2 in an even Nyquist zone, so folding inverts frequency: as the true f rises, the apparent |f − k·fs| falls. Confirm by changing fs: a true signal does not move, an alias does.
