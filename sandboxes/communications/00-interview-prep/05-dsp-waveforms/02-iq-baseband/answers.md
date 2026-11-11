# Pack 02 — Answers (I/Q and complex baseband)

All values hand-computed (no code run).

## Part A

**A1.** (3+4j)·j = 3j + 4j² = **−4 + 3j**. Matrix with φ = 90°: x′ = 3cos90° − 4 sin90° = −4; y′ = 3 sin90° + 4cos90° = 3 ✓. |·| = 5 before and after (rotation preserves length).

**A2.** Signal A (above LO): z(t) = A·e^{+j2π·10 kHz·t}, counter-clockwise. Signal B (below LO): z(t) = A·e^{−j2π·10 kHz·t}, clockwise. I alone = A cos(2π·10 kHz·t) for both → indistinguishable. I + jQ → distinguishable by rotation sense / sign of the spectrum peak.

**A3.** 0.05 × 48 000 = **2400 Hz**; Δφ = 2π·0.05 = 0.3142 rad = **18°** per sample. Second: −0.1 × 48 000 = **−4800 Hz**; Δφ = −0.6283 rad = **−36°**.

**A4.** LO above the carrier by 300 Hz ⇒ baseband tone at f_c − f_LO = **−300 Hz**. Rotation per symbol = −300/4800 × 360° = **−22.5°** (clockwise). After 10 symbols −225°. A PSK constellation would be completely smeared unless corrected; a 4FSK discriminator just sees a DC offset of −300 Hz.

**A5.** LO = 150 − 10.7 = **139.3 MHz**; image = 139.3 − 10.7 = **128.6 MHz** (mirror of the wanted channel around the LO). A real mixer outputs |f − f_LO| so both 150 and 128.6 MHz land at 10.7 MHz → need an RF pre-selector. An ideal quadrature mixer separates them (the image lands at −10.7 MHz instead of +10.7 MHz), but real I/Q gain/phase error leaks a fraction back (IRR typically tens of dB), and RF filtering is still needed for blockers and ADC headroom.

**A6.** 2x·cos(ω_c t) = (1+0.5cos ω_m t)(1 + cos 2ω_c t) → LPF → I = 1 + 0.5 cos(2π·1 kHz·t). −2x sin(ω_c t) = −(1+…) sin 2ω_c t → LPF → Q = 0. |z| = I (the envelope = message + DC), arg z = 0 (since I > 0). AM demodulation = take |z| and subtract DC.

**A7.** 2cos(ω_c+Δ)t · e^{−jω_c t} = (e^{j(ω_c+Δ)t} + e^{−j(ω_c+Δ)t})e^{−jω_c t} = **e^{jΔt}** + e^{−j(2ω_c+Δ)t}. The LPF keeps e^{jΔt} (low frequency Δ) and removes the term at −(2f_c+Δ).

**A8.** Wanted: 0.26 − 0.25 = **+0.01 cycles/sample**. Unwanted term: e^{−j2π·0.51·n} (frequency −0.51 ≡ +0.49). Boxcar gain at 0.51: |sin(4π·0.51)/(4 sin(π·0.51))| = |sin 6.4088| / (4 · 0.9995) = 0.1253 / 3.998 = **0.0313 ≈ −30 dB**. At 0.01: 0.1253/(4 · 0.03141) = 0.9974 (0.02 dB droop). This is why the C++ test tolerates 3 % magnitude ripple but frequency is estimated to 10⁻³.

## Part B — reference notes
* **B1 `mix`**: `y[n] = x[n]·polar(1, 2π f n + φ₀)`. Positive f shifts the spectrum **up**; to down-convert pass −f. Accumulating the phase incrementally is cheaper but drifts in float; recomputing from n costs a sin/cos per sample (use a recurrence/NCO in production code).
* **B2 `iq_downconvert`**: raw I = 2x cos, raw Q = −2x sin; causal FIR h applied to both. The filter adds a delay of (M−1)/2 samples = a constant phase offset (2π f_Δ · delay) but **no change in frequency** — which is what B3 measures. Output length = input length (zero initial state).
* **B3 `estimate_freq_cps`**: `arg( Σ z[n+1]·conj(z[n]) )/2π`. Summing before taking arg (rather than averaging per-sample args) weights strong samples more and avoids wrap problems; unambiguous only for |f| < 0.5 cycles/sample. This is the same estimator used in the CFO exercise of Pack 10.
* **B4 `instantaneous_freq`**: per-sample version, length N−1.
* **B5 `image_rejection_db`**: wanted = |Σ z e^{−j2πfn}|, image = |Σ z e^{+j2πfn}|; IRR = 20 log₁₀(wanted/image). For I = cos θ, Q = sin(θ+φ) (g = 1) the exact answer is 20 log₁₀(cot(φ/2)): φ = 0.1 rad → 26.0 dB (derivation: z = ½(1+e^{jφ})e^{jθ} + ½(1−e^{−jφ})e^{−jθ}).

## Part C — model answers
1. A real signal's spectrum is conjugate-symmetric, so +f and −f are indistinguishable; with I/Q we keep the single-sided complex envelope, so frequency/phase *direction* is observable. Also halves the required sample rate per channel relative to real IF sampling of the same band.
2. The quadrature mixer multiplies by e^{−jω_c t} (cos and −sin). The wanted band moves to Δ, while the image at f_c − Δ moves to −Δ, which is a *different* complex frequency. A real mixer folds both to |Δ|. Rejection is limited by I/Q gain/phase mismatch.
3. A CFO Δf multiplies the baseband by e^{j2πΔf t}: constellation rotates at Δf; a 4FSK discriminator output has a DC offset. Estimate: lag-1 autocorrelation arg(Σ z[n+1]z*[n])/2π·fs (works for unmodulated tone/preamble), or for PSK use the M-th power method; for FSK, subtract the mean/midpoint of the demodulated symbol levels.
4. For complex samples, fs = bandwidth (Nyquist for complex: fs ≥ B). 1 MS/s complex = 1 MHz of bandwidth (±500 kHz). They're quoting the real-sample rule.
5. 10 % gain error + 5° phase error → circle becomes an ellipse tilted by the phase error, i.e. the QPSK points are no longer on a square; image at ≈ −23.6 dB relative to the wanted signal (see worksheet §4 item 6).
