# Pack 04 — Answers (Filters)

Hand-computed; nothing executed.

## Part A

**A1.** [1,2,3] * [1,−1] = [1, 2−1, 3−2, −3] = **[1, 1, 1, −3]** (length 3+2−1 = 4). [1, −1] is a first difference (differentiator/high-pass): H(ω) = 1 − e^{−jω}, |H| = 2|sin(ω/2)|: zero at DC, gain 2 at Nyquist, rising monotonically.

**A2.** Nulls at f = m/L → first null 1/5 cycle/sample × 48 kHz = **9.6 kHz**. DC gain = Σh = 5 × 1/5 = **1**.

**A3.** (N−1)/2 = 31 samples = 31/48000 = **0.646 ms**.

**A4.** N ≈ 3.3/0.02 = **165 taps** (use 165, odd ⇒ integer delay of 82 samples). Making it cheaper: a 4× polyphase decimator evaluates 165 MAC per *output* sample (at 12 kS/s) = 1.98 MMAC/s instead of 7.92 MMAC/s; or split into a cheap wide-transition first stage (÷2 or ÷4) followed by a sharper stage at the lower rate (the second stage needs far fewer taps in *absolute* count because the transition is measured in cycles/sample at the lower rate).

**A5.** h_d[n] = sin(2π·0.25·n)/(π n): n = 0 → 2fc = **0.5**; n = ±1 → sin(π/2)/π = **0.3183**; n = ±2 → sin(π)/(2π) = **0**; n = ±3 → sin(3π/2)/(3π) = **−0.1061**. (fc = ¼ is a half-band filter: every second tap is zero — which is why half-band decimators are cheap.)

**A6.** Triangle test: |a2| = 0.4128 < 1 ✓; |a1| = 1.143 < 1 + a2 = 1.413 ✓ ⇒ **stable**. Poles: z² − 1.14298z + 0.4128 = 0 → discriminant 1.30639 − 1.6512 = −0.3448 → z = 0.5715 ± j0.2936. Radius √a2 = **0.642**, angle atan(0.2936/0.5715) = **27.2°** (0.0756 cycles/sample).

**A7.** Naive (filter at input rate): 64 × 48 000 = **3.072 MMAC/s**. Polyphase: 64 × 12 000 = **0.768 MMAC/s** (4× fewer).

**A8.** Branch p (p = 0,1,2): E_p = { h[p], h[p+3], h[p+6], h[p+9] }: E0 = {h0,h3,h6,h9}, E1 = {h1,h4,h7,h10}, E2 = {h2,h5,h8,h11}; y[3n+p] = Σ_q E_p[q]·x[n−q]. Each output costs 4 MAC (12/3). The filter's DC gain must be **3** (= L) to restore the amplitude lost by zero-stuffing.

**A9.** H(ω) = a + b e^{−jω} + c e^{−j2ω} + b e^{−j3ω} + a e^{−j4ω} = e^{−j2ω}[c + 2b cos ω + 2a cos 2ω]; bracket is real ⇒ phase −2ω (linear), **group delay 2 samples** (= (N−1)/2 for N = 5). (Where the bracket is negative there is a ±π jump — a zero crossing — not a delay change.)

**A10.** H(z) = (1 − z⁻¹)/(1 − αz⁻¹): zero at z = 1 (DC), pole at z = α. |H(ω)|² = (2−2cos ω)/(1 − 2α cos ω + α²) ≈ ω²/((1−α)² + αω²) for small ω; equals ½ when ω ≈ 1−α. α = 0.99, fs = 8 kHz: ω = 0.01 rad/sample → f = 0.01·8000/(2π) = **12.7 Hz**.

## Part B — reference notes
* **B1 `convolve`**: output size nx + nh − 1; either loop over output and clip the k-range, or accumulate `y[i+j] += x[i]·h[j]`. Empty input → empty output.
* **B2 `hamming`**: symmetric form 0.54 − 0.46cos(2πn/(N−1)); special-case N = 1 (return {1}).
* **B3 `windowed_sinc_lowpass`**: m = n − (N−1)/2 (works for even N too: half-sample delay), h = 2fc·sinc(2fc·m)·w[n], sinc(0) = 1, then divide by Σh. Expected taps for (11, 0.1): centre 0.2370, ±1: 0.2022, ±2: 0.1224.
* **B4 `Biquad`**: Transposed DF-II (two states). Coeffs from the cookbook; coefficients for (0.1, 0.7071) are b = [0.06746, 0.13491, 0.06746], a1 = −1.14298, a2 = 0.41280; impulse response starts 0.06746, 0.21201.
* **B5 `decimate_polyphase`**: for each output m, accumulate over phases p and q with idx = mM − (qM + p); stop when idx < 0. Output length ⌈nx/M⌉; must equal "full convolution, then every Mth sample" — the test checks it.
* **B6 `interpolate_polyphase`**: for each input n and phase p: y[nL+p] = Σ_q h[qL+p]·x[n−q]. Output length nx·L, equal to "zero-stuff, convolve, truncate to nx·L".

## Part C — model answers
1. If h[n] = h[M−n], pairing terms h[n]e^{−jωn} + h[M−n]e^{−jω(M−n)} gives e^{−jωM/2}·2h[n]cos(ω(n−M/2)), a real amplitude times a pure delay: phase −ωM/2. Benefit: no phase distortion, so the symbol waveform shape is preserved and sync/timing circuits see a constant, known delay; allows matched/RRC pairs with exactly symmetric responses.
2. The ideal rectangular frequency response; its inverse DTFT is sin(ωc n)/(πn), i.e. a sinc. We truncate and window it because it is infinite and non-causal.
3. Decimation keeps every Mth sample, folding everything above fs/(2M) onto the wanted band (aliasing is irreversible). Cheaper: polyphase (compute only needed outputs), multi-stage (cheap CIC/half-band at high rate, sharp FIR at the low rate), half-band filters (half the taps are zero).
4. DC blocker: IIR (single pole/zero: a handful of MACs, linear phase not needed, very narrow notch at DC would need thousands of FIR taps). 12.5 kHz channel filter: FIR (linear phase, bounded fixed-point behaviour, symmetric ⇒ half the multiplies, easily polyphase/decimated). Mention exceptions: very CPU-limited → IIR with phase equaliser.
5. 50 samples at 96 kS/s = **0.52 ms**. A sync correlator after the filter will peak 0.52 ms late relative to the antenna signal; slot/frame timing derived from the sync peak must be corrected by that constant (e.g. 0.52 ms vs 30 ms DMR slot ≈ 1.7 %); with a symmetric FIR the delay is *exactly* constant — an IIR's frequency-dependent delay would smear the peak. (DMR slot length: VERIFY.)
