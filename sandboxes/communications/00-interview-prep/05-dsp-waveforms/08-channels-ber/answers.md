# Pack 08 — Answers (Channels and BER)

Hand-computed; nothing executed. **VERIFY** marks items to check against standards/datasheets.

## Part A

**A1.** Q(√(2γ)) = 10⁻³ ⇒ √(2γ) = 3.09 ⇒ γ = 3.09²/2 = 4.774 ⇒ **6.79 dB**.

**A2.** SNR = Eb/N0 − 10log₁₀(B/Rb) = 9.6 − 1.146 = **8.45 dB**. Noise floor in 12.5 kHz: −174 + 40.97 = **−133.0 dBm**; with NF 6 dB: **−127.0 dBm**; sensitivity (AWGN, SNR 8.45 dB) ≈ −127.0 + 8.45 = **−118.6 dBm**.

**A3.** v = 120/3.6 = 33.33 m/s; f_d = 33.33 × 800×10⁶/2.998×10⁸ = **88.9 Hz**; T_c ≈ 0.423/88.9 = **4.76 ms** (≈ 23 symbols at 4800 sym/s).

**A4.** Rayleigh: 1 − e⁻⁰·⁰¹ = **0.00995 (≈ 1 %)**. Rician K = 6 dB = 3.98: LOS power fraction K/(K+1) = **0.80**; Var|h|² = (1+2K)/(1+K)² = 8.96/24.8 = **0.36**; the power pdf at zero is (K+1)e⁻ᴷ = 4.98 × 0.0187 = 0.093, so P(|h|² < 0.01) ≈ 9×10⁻⁴ — **10× fewer** deep fades than Rayleigh.

**A5.** Rayleigh at 20 dB (γ = 100): ½(1 − √(100/101)) = ½(1 − 0.99504) = **2.48×10⁻³**, versus AWGN Q(√200) = Q(14.1) ≈ 10⁻⁴⁵ (effectively zero). For 10⁻³ in Rayleigh: √(γ/(1+γ)) = 0.998 ⇒ γ = 249.5 ⇒ **24.0 dB**, i.e. **+17.2 dB** over the 6.8 dB needed in AWGN (the fade margin).

**A6.** |H(f)| = |1 + 0.5e^{−j2πf}|: peak 1.5 (**+3.5 dB**) at f = 0, notch 0.5 (**−6.0 dB**) at f = ±0.5 (notch spacing = 1/delay = 1 cycle/sample, i.e. one notch at Nyquist). Total swing 9.5 dB.

**A7.** Noise filling the sampling bandwidth: σ² = N₀f_s, signal power per sample P = E_sR_s ⇒ SNR_sample = E_sR_s/(N₀f_s) = (E_s/N₀)/sps. QPSK 8 dB E_b/N₀ → E_s/N₀ = 8 + 3.01 = 11.01 dB → per-sample SNR at sps = 4: 11.01 − 6.02 = **4.99 dB**. (Filtering to the signal bandwidth recovers 6 dB — the matched filter's processing gain.)

**A8.** 16-QAM needs ≈ 10.5 dB vs QPSK 6.8 dB at 10⁻³: **≈ +3.7 dB** (asymptotically 3.98 dB from d²_min/E_b).

## Part B — reference notes
* **B1**: Q(x) = 0.5·erfc(x/√2) (use `std::erfc`, never an integral). `bpsk_ber_awgn(dB)` = Q(√(2·10^(dB/10))). `rayleigh_bpsk_ber` = ½(1 − √(g/(1+g))).
* **B2**: Eb/N0 = SNR + 10log₁₀(B/Rb). Noise floor −174 + 10log₁₀B + NF. 
* **B3**: f_d = v f_c / c with c = 299 792 458 m/s; T_c = 0.423/f_d.
* **B4**: noise per component = N(0, N0/2). `simulate_qpsk_ber`: Es = 1 → Eb = 0.5; N0 = Eb/(Eb/N0); decisions by sign; count both bits. The unit test compares against Q(√(2γ)) to 10 % with 2×10⁶ bits (≈ 1.4 % sampling noise at BER ~ 2×10⁻³).
* **B5**: per-bit independent CN(0,1) gain (ideal interleaving; otherwise errors cluster and the *average* BER still converges to the same value as long as the fading process is stationary, but needs many more fades to average), coherent detection z = Re(h* r).
* **B6**: Clarke sum of 64 sinusoids with random arrival angles/phases; Rician = LOS phasor with its own Doppler + scaled diffuse part (powers K/(K+1) and 1/(K+1)). A finite sum of sinusoids is not exactly Gaussian (kurtosis 2 − 1/M for power): use M ≥ 32.

## Part C — model answers
1. r = a√E_b + n, n ~ N(0, N₀/2). P(error | a = +1) = P(n < −√E_b) = ∫_{√E_b}^{∞} (πN₀)^{-1/2} e^{−x²/N₀}dx = Q(√E_b/√(N₀/2)) = Q(√(2E_b/N₀)).
2. Eb/N0 = SNR · B/R_b (dB: add 10log₁₀(B/R_b)). BER vs E_b/N0 is modulation-comparable and independent of bandwidth/rate/oversampling choices; SNR depends on the noise bandwidth you happened to pick.
3. At the same *average* SNR the instantaneous SNR varies exponentially; the error rate is dominated by the rare deep fades: P_b = E[Q(√(2γ))] ≈ 1/(4γ̄) — only 1 decade per 10 dB instead of the AWGN waterfall. A deep fade costs a burst of errors, so fade margin (+17 dB at 10⁻³) or diversity/coding are required.
4. T_c ≈ 0.423/f_d: how long the channel stays correlated (fast fading if T_symbol > T_c, slow if ≪). B_c ≈ 1/(5σ_τ): how wide a frequency band fades together (flat if signal BW ≪ B_c, frequency-selective ⇒ ISI if BW ≳ B_c). 12.5 kHz LMR is usually flat/slow; OFDM broadband is selective.
5. (i) Compare against closed-form theory at several Eb/N0 (and confirm the slope); (ii) check that measured noise power matches N0 (Pack 08 AWGN test) and that the signal power gives the claimed Es; (iii) count ≥ 100 errors per point (relative std 10 %); (iv) test with a known-good seed + different seeds; (v) test degenerate cases (noise-free → zero errors; huge noise → 50 %); (vi) check the unit conversions independently; (vii) compare QPSK vs BPSK equivalence.
