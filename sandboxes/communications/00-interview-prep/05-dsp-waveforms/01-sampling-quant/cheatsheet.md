# Pack 01 cheatsheet — Sampling & Quantisation

## Aliasing
* Samples of cos(2π f n/fs) are identical for f, f ± fs, f ± 2fs … (and −f for cosine).
* Apparent frequency of a real tone: `r = |f| mod fs; f_a = (r <= fs/2) ? r : fs - r`.
* Need bandwidth **< fs/2** (strict). Exactly fs/2: sin → all zeros.
* Even Nyquist zones ⇒ spectral inversion (tone moves the wrong way).
* Aliasing is irreversible ⇒ analogue anti-alias filter first; digital filter before every decimation.

## Band-pass sampling  (band [fL, fH], B = fH − fL)
`2fH/k ≤ fs ≤ 2fL/(k−1)`, k integer, k_max = ⌊fH/B⌋, always fs ≥ 2B.
Cost: sharp analogue filter, tight clock, jitter at fH.

## Quantisation (N bits, full-scale ±FS)
* Δ = 2·FS / 2ᴺ; error ∈ [−Δ/2, Δ/2], power **Δ²/12**.
* Sine SQNR = 1.5·2^(2N) → **6.02 N + 1.76 dB** (exact 6.0206 N + 1.7609).
* Back-off b dB ⇒ subtract b. High-PAPR signal ⇒ subtract PAPR.
* Oversample ×M ⇒ +10 log₁₀ M dB in-band (white error), 3 dB/octave.
* Bits needed: N = (SNR − 1.76) / 6.02.
* Jitter limit: SNR = −20 log₁₀(2π f σₜ).
* dBFS: 20 log₁₀(|x| / FS).

## Quick numbers
| N | 8 | 10 | 12 | 14 | 16 |
|---|---|----|----|----|----|
| SNR dB | 49.9 | 62.0 | 74.0 | 86.0 | 98.1 |

## Misconceptions
* "6.02N+1.76 is always true" → only full-scale sine, white error.
* "Rounding vs truncation same" → truncation adds DC Δ/2.
* "Higher ADC bits fix everything" → ENOB, jitter, noise figure, AGC.
* "Periodic signal ⇒ white quantisation noise" → no, harmonics (dither fixes).

## Interview one-liners
* Why filter before sampling? Because aliasing can't be undone.
* Why +6 dB per bit? Δ halves, noise power quarters.
* Why 1.76? Sine peak/RMS = √2 → 3/2 factor → 1.76 dB.
