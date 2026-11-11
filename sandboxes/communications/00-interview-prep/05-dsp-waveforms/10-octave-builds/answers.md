# Pack 10 — Answers (Octave models and C++ builds)

Hand-derived; nothing compiled or run. **VERIFY** marks items to check against standards.

## Part A

**A1.** Worst-case rounding error of a 6-term sum ≲ (N−1)·ε·Σ|h|·max|x| = 5 × 1.1×10⁻¹⁶ × 1.5 × 1 ≈ **8×10⁻¹⁶**. A tolerance of 1e-12 gives > 1000× margin yet is still far below any real defect (a wrong tap is ≥ 10⁻³).

**A2.** (0.1+0.2)+0.3 = **0.6000000000000001** (0.1+0.2 = 0.30000000000000004 rounds up; adding 0.3 rounds again); 0.1+(0.2+0.3) = 0.1 + 0.5 = **0.6**. Each addition rounds to 53 bits, so the result depends on the order — floating-point addition is commutative but **not associative**.

**A3.** Relative σ = 1/√(errors). For ±5 % (σ): 400 errors ⇒ n = 400/p. p = 10⁻³ ⇒ **4×10⁵ bits**; p = 10⁻⁵ ⇒ **4×10⁷ bits**. (For a 95 % interval of ±5 % you need ≈ 1 500 errors.)

**A4.** Increment = level · 2π·600/48000 = level · π/40 = level · 0.07854 rad: −3 → −0.2356, −1 → −0.0785, +1 → +0.0785, +3 → +0.2356. Per symbol (10 samples): ×10 → −2.356 (−135°), −0.785 (−45°), +0.785 (+45°), +2.356 (+135°).

**A5.** 353 lags × 7.57×10⁻¹⁰ = **2.7×10⁻⁷** per 400-bit record (Σ_{k≤4} C(48,k) = 213 053; 213 053/2⁴⁸ = 7.57×10⁻¹⁰).

**A6.** 0.0123 × 48 000 = **590.4 Hz**. The lag-1 estimator is unambiguous for |f| < 0.5 cycle/sample = **24 kHz** at 48 kHz (but the noise sensitivity grows with the range; for QPSK without data-aided removal it is ±R_s/8).

**A7.** y₀ = 16384·16384 = 268 435 456 → (+2¹⁴) >> 15 = **8192**. y₁ = 16384·16384 + 8192·16384 = 402 653 184 → **12288**. y₂ = 16384·(−32768) + 8192·16384 = −536 870 912 + 134 217 728 = −402 653 184 → **−12288**.

**A8.** Octave `mod(-1, 4) = 3` (result has the sign of the divisor, so it is a true modulo), but C++ `(-1) % 4 = -1` (sign of the dividend). With i = 0 the circular index differs: Octave 3, C++ −1 ⇒ out-of-bounds access (caught by ASan). Fix in C++: `((i - 1) % 4 + 4) % 4`, or avoid negative arguments by using `(i + n - 1) % n` as `FirFilter::process` does. (Also remember Octave is 1-based, C++ 0-based.)

## Part B — implementation notes
* **B1 fft**: size check (`n == 0 || (n & (n-1))`), bit-reverse copy, for len = 2, 4, …, N: twiddles `polar(1, −2π k/len)` for k < len/2, butterflies `u ± v·w`. Test values: impulse ⇒ ones; tone bin 5 ⇒ N; matches naive DFT to 1e-12; Parseval Σ|X|² = NΣ|x|².
* **B2 FirFilter**: delay line of N samples; write the newest sample at `pos_`; y = Σ_k h[k]·state[(pos_ + N − k) % N]; then `pos_ = (pos_+1) % N`. Do **not** shift the whole array each sample in a real implementation (O(N) copies). Streaming/blocking identical because `process_block` just loops `process`. Empty taps ⇒ throw.
* **B3 fsk4**: table {+1,+3,−1,−3} indexed by the dibit value (00,01,10,11). Phase accumulates *after* emitting each sample so sample 0 is e^{j0} = 1.
* **B4 sync**: double loop; use `!=` on ints so any non-binary value counts as an error (garbage in ⇒ visible). `sync_detect` is a threshold scan returning lags.
* **B5 cfo**: accumulate cur·conj(prev) with prev = rx[0]·k[0]; `arg(acc)/2π`. Correct with `polar(1, −2π f n)` — computing the angle directly per sample (not a recursive rotator) keeps rounding error from growing (a recursive rotator drifts in magnitude and needs renormalising).
* **B6 BER**: theory = 0.5·erfc(√γ). Simulation draws 1 random word per symbol for both bits (low two bits of the 64-bit output), sign decision per component. Statistical tolerance 10 % with 2×10⁶ bits.
* **B7 q15_fir**: identical to Pack 09's. Octave reference in `gen_vectors.m` is bit-exact because |terms| ≤ 2³⁰ and sums are ≪ 2⁵³.

**Tolerances used and why**: FFT 1e-12 (rounding ≈ 2e-15), FIR 1e-12, CFO estimate 1e-12 / corrected samples 1e-9, 4FSK waveform 1e-9, sync distances and Q15 outputs **exact**, simulated BER 10 % (≈ 1.4 % σ at 6 dB with 2×10⁶ bits, wider at lower SNR since more errors).

**Expected Octave-vector facts** (hand-checked): embedded word at 0-based lag 100 with two errors ⇒ d[100] = 2; Q15 output index 3 = 32767 (pre-saturation 35 999); `fsk4_levels` for dibits {0,1,2,3} = {+1,+3,−1,−3}; `ber_theory.txt` = 0.07865, 0.03750, 0.01250, 0.002388, 1.91e-4 at 0, 2, 4, 6, 8 dB.

## Part C — model answers
1. Property tests that do not need the model (impulse response, tone → single bin, Parseval, linearity/shift invariance, streaming = block, analytic theory); then export vectors from an *independently derived* reference model, with stimuli that exercise corners (full-scale, zeros, saturation, odd lengths); bit-exact for integer paths, derived tolerances for floats, statistics for noise; run in CI; keep the generator deterministic and versioned with the tests.
2. Different but equally correct FFT algorithms (radix-2, split-radix, FFTW's codelets) sum products in different orders with different twiddle values, so last-bit differences are inevitable; the right test is "error ≤ c·ε·log₂N·‖X‖". Bit-exact equality *is* demandable for integer/fixed-point pipelines (the specification fixes rounding and order), for hard-decision logic (Hamming distance, symbol decisions), and for the same binary on the same CPU.
3. Compare to the closed-form BER with a statistical tolerance derived from the error count (relative σ = 1/√errors, so ≥ 400 errors for ±10 % at 95 %); check subcomponents (noise power = N₀, signal energy = E_s); use fixed seeds for reproducibility within one toolchain; test degenerate cases (no noise ⇒ 0 errors; huge noise ⇒ 50 %).
4. 1-based vs 0-based indexing; `mod` vs `%` sign; integer saturation vs wrap; `round` (away from zero) vs `rint` (ties-to-even); normalised `sinc`; window definitions (symmetric vs periodic); FFT scaling (`ifft` divides by N); RNG streams; `filter` initial conditions/length; complex conjugate transposes (`'` conjugates, `.'` doesn't).
5. Skip, don't fail, when a vector file is missing (`GTEST_SKIP`), and keep analytic tests that need no external files; generate vectors in a separate CI job (or commit them with the generating script and a hash); pass the vector directory through a compile definition so tests don't depend on the working directory.
6. Compute the exact error budget: for a Q15 FIR with N taps the integer output equals the exact real value to within ½ LSB (one rounding), so > ½ LSB means the arithmetic is not what was specified (missing rounding term, truncation, intermediate rounding per tap, coefficient mismatch). If the *reference* is float run on the original (unquantised) taps, include coefficient-quantisation error: ≤ ½ LSB·Σ|x| worst-case. 3 LSB at a single sample with a stationary input of moderate amplitude is a bug; 3 LSB for a full-scale 64-tap filter with unquantised coefficients might not be.
