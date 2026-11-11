# Pack 07 — Answers (Receiver building blocks)

Hand-computed; nothing executed. **VERIFY** marks values that must be checked against a standard.

## Part A

**A1.** s = [2, −1, 1], E = 4 + 1 + 1 = 6, σ² = 0.5. Matched filter h = s reversed = **[1, −1, 2]**: y_s = Σ h[k]s[2−k] = 1·1 + (−1)(−1) + 2·2 = 6; Σh² = 6; SNR = 36/(0.5·6) = **12 = E/σ²**. Boxcar [1,1,1]: y_s = 2 − 1 + 1 = 2; Σh² = 3; SNR = 4/(0.5·3) = 2.67. Loss = 10 log₁₀(12/2.67) = **6.5 dB**.

**A2.** y_s = Σ h[k]u[k], u[k] = s[N−1−k] (complex). Cauchy–Schwarz in ℂᴺ: |Σ h u|² ≤ (Σ|h|²)(Σ|u|²) with equality iff h[k] = c·conj(u[k]). Complex white noise with E|w|² = σ² has output variance σ²Σ|h|² ⇒ SNR = |y_s|²/(σ²Σ|h|²) ≤ Σ|s|²/σ² = E_s/σ², reached at h[k] = conj(s[N−1−k]).

**A3.** d = 2: (1 + 48 + 1128)/2⁴⁸ = 1177/2.815×10¹⁴ = **4.18×10⁻¹²** per bit position. At 9600 bit/s: 3.456×10⁷ positions/hour × 4.18×10⁻¹² = 1.4×10⁻⁴ per hour ⇒ about one false alarm every **6900 hours** (≈ 290 days).

**A4.** f = 100/4800 = 0.02083 cycles/symbol. L = 24: |sin(π·24·0.02083)| = sin(π/2) = 1; 24·sin(π·0.02083) = 24·0.06540 = 1.570 ⇒ **0.637 → −3.9 dB**. L = 12: sin(π/4) = 0.7071; 12·0.0654 = 0.785 ⇒ **0.901 → −0.9 dB**. Shorter coherent blocks tolerate more CFO.

**A5.** e = (y[k] − y[k−1])·y[k−½] = (−1.0 − 0.9)(−0.3) = **+0.57 > 0 ⇒ late** (the "mid" sample has already crossed zero and shares the sign of the new symbol).

**A6.** Per-symbol factor 1 − k_pK_d/sps = 1 − 0.15·2.6/8 = **0.951**. To 5 %: ln 0.05/ln 0.951 = 60 symbols (time constant ≈ 20 symbols).

**A7.** θ = 30°: e = ½ sin 60° = **+0.433** → NCO phase increases toward the input (θ shrinks) ✓. θ = 120°: e = ½ sin 240° = **−0.433** → the NCO phase decreases, θ grows toward **180°** — the loop locks at the *other* equilibrium (π). Both are valid for BPSK; the sync word resolves the sign.

**A8.** B_nT = 0.01, ζ = 0.7071: θ_n = 0.01/1.06066 = 0.009428; d = 1 + 2(0.7071)(0.009428) + 8.9×10⁻⁵ = 1.01342; **α = 0.02631, β = 3.51×10⁻⁴**.

**A9.** Time constant 1/(μ r) = 1/(0.02·1) = **50 samples**. Starting at 0.001 the update is logistic: y(n) = 1/(1 + ((1−y₀)/y₀) e^{−μn}); y = 0.5 when e^{−μn}·999 = 1 ⇒ n = ln 999 / 0.02 = **≈ 345 samples**.

**A10.** CFO = 2×10⁻⁶ × 450×10⁶ = **900 Hz**; per symbol 900/4800 × 360° = **67.5°**. Clock at 2 ppm: 4800 × 2×10⁻⁶ = 0.0096 symbols/s = **34.6 symbols per hour** of slip.

## Part B — reference notes
* **B1** `matched_filter_output_snr`: y_s = Σ h[k]s[N−1−k]; SNR = y_s²/(σ²Σh²). The test draws 200 random h and verifies none beats E/σ².
* **B2a** `sync_search`: for each lag compute the complex correlation and the window energy, ρ = |c|²/(E_rx·E_ref); keep the first maximum. Invariant to complex scale/rotation (test uses ×0.8 e^{j0.5}). **B2b** `sync_cfo_loss` is the Dirichlet-kernel ratio; **B2c** accumulates C(n,k) iteratively (multiply by (n−k+1)/k) and divides by 2ⁿ via `ldexp`.
* **B3** `gardner_error` = (cur − prev)·mid. `gardner_recover`: floating strobe time, linear interpolation, PI correction; test uses a 0.2 T offset and requires ≥ 97 % sign agreement against the true symbols over 300 symbols after acquisition (symbol alignment is found by trying shifts 0–8 because the loop may lock to any symbol boundary).
* **B4** `loop_gains` as in worksheet §4.3. `costas_bpsk`: derotate, e = Re·Im, `freq += β e; phase += freq + α e`. The test accepts lock to either 0 or π.
* **B5** `Agc`: multiplicative log-domain update, clamped to [10⁻⁶, 10⁶] to avoid numeric blow-up.

## Part C — model answers
1. SNR = (Σ h u)²/(σ²Σh²). Cauchy–Schwarz bounds the numerator by (Σh²)(Σu²), so SNR ≤ E/σ², with equality for h ∝ u (time-reversed pulse). Intuition: weight each sample in proportion to how much signal it has (SNR contribution ∝ s²/σ²) — maximal-ratio combining.
2. Pick the threshold from two distributions: noise-only correlation (false alarms; for hard decisions P_fa = Σ C(n,k)/2ⁿ) and signal+noise correlation (misses; binomial with the expected BER). Choose d so false alarms/hour are acceptable *and* P(miss) is small at the minimum design SNR. CFO: coherent correlation over L symbols is multiplied by |sin(πLf)/(L sin πf)| (−13 dB at 300 Hz for 24 symbols at 4800 sym/s) ⇒ use shorter coherent blocks + non-coherent sum, differential correlation, correlate on discriminator output with mean removed, or pre-estimate and remove CFO.
3. Gardner compares the *middle* sample between two strobes with the slope (y[k] − y[k−1]) of the strobes. Mid-sample zero at correct timing; its sign relative to the slope says early/late. Two samples per symbol are needed because the mid-sample is the zero-crossing probe. Non-data-aided and insensitive to carrier phase (for complex signals use the real part).
4. A plain PLL needs a phase reference (a carrier or known preamble). BPSK data has a sign flip that a plain PLL cannot follow; the Costas detector multiplies I·Q so data²=1 cancels the modulation. Remaining ambiguity: π (BPSK) or π/2 (QPSK), fixed by the sync word or differential coding.
5. For constant input amplitude the log-domain error is linear in the log-gain, so the loop time constant is 1/(μ r), independent of the input level; a linear-gain update's time constant is 1/(μA), too slow for weak and unstable for strong signals. It also works in dB (matches the physical dB-per-volt gain steps of analogue VGAs).
