# Pack 03 — Answers (DFT, FFT, resolution, windowing)

Hand-computed; no code was run.

## Part A

**A1.** X[k] = Σ x[n](−j)^{kn} with x = [1,0,−1,0]: X[0] = 1 − 1 = 0; X[1] = 1 + (−1)(−j)² = 1 + (−1)(−1) = 2; X[2] = 1 + (−1)(−j)⁴ = 0; X[3] = 1 + (−1)(−j)⁶ = 1 + (−1)(−1) = 2. **X = [0, 2, 0, 2].** x[n] = cos(2πn/4): one cycle per 4 samples → bins 1 and N−1 = 3, each N/2 = 2 ✓.

**A2.** (a) 48 000/1024 = **46.875 Hz**. (b) 1000/46.875 = 21.33 → nearest bin 21, offset δ = 0.33 bin → rectangular-window amplitude |sinc(0.33)| = sin(π/3)/(π/3) = 0.827 → **−1.65 dB** (the worst case, δ = 0.5, is −3.92 dB). (c) Two tones 100 Hz apart need ≥ 2 bins with Hann → Δf ≤ 50 Hz → N ≥ 48 000/50 = 960 → **N = 1024** (46.9 Hz → 2.13 bins).

**A3.** W_N^{N/2} = e^{−jπ} = −1 ⇒ W_N^{k+N/2} = −W_N^k. W_N² = e^{−j4π/N} = e^{−j2π/(N/2)} = W_{N/2}. These two facts are the entire FFT.

**A4.** Naive: 1024² = 1 048 576 complex mults × 4 = **4 194 304** real mults. FFT: (1024/2)·10 = 5120 complex mults × 4 = **20 480** (≈ 205× fewer; the real-multiplication count drops to about 3 per complex mult with the Gauss trick, and trivial twiddles ±1, ±j are free).

**A5.** x = [1,1,1,1,0,0,0,0]. E = DFT[1,1,0,0] = [2, 1−j, 0, 1+j] and O = DFT[1,1,0,0] = [2, 1−j, 0, 1+j] (odd samples are the same pattern).
* X[0] = 2+2 = 4, X[4] = 2−2 = 0.
* X[1] = E[1] + W¹O[1] = (1−j)(1 + 0.7071 − 0.7071j) = (1−j)(1.7071 − 0.7071j) = **1 − 2.4142j**; X[5] = E[1] − W¹O[1] = (1−j)(1 − 0.7071 + 0.7071j) = (1−j)(0.2929+0.7071j) = **1 + 0.4142j**.
* X[2] = E[2] + W²O[2] = 0; X[6] = 0.
* X[3] = E[3] + W³O[3] = (1+j)(1 + (−0.7071−0.7071j)) = (1+j)(0.2929−0.7071j) = **1 − 0.4142j**; X[7] = (1+j)(1.7071+0.7071j) = **1 + 2.4142j**.

X = [4, 1−2.414j, 0, 1−0.414j, 0, 1+0.414j, 0, 1+2.414j]. Parseval: |X|² = 16 + 2(6.828) + 2(1.172) = 16 + 13.657 + 2.343 = **32 = 8 × 4** ✓.

**A6.** Scalloping at δ = 0.5: rectangular |sin(π/2)/(π/2)| = 0.6366 → 20log₁₀ = **−3.92 dB**; Hann 0.6366/(1−0.25) = 0.8488 → **−1.42 dB**.

**A7.** N = 16: 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15.

**A8.** Bin spacing 25 600/256 = 100 Hz. Bin 200 > N/2 = 128, so it is a negative frequency: (200 − 256)·100 = **−5.6 kHz**. A real signal has the mirror at +5.6 kHz (bin 56), so the real tone is **5.6 kHz**.

## Part B — reference notes
* **B1 `dft`**: double loop, `X[k] += x[n]·polar(1, −2πkn/N)`. Reduce kn mod N before the angle for accuracy at large N.
* **B2 `bit_reverse(i, bits)`**: shift bits out of `i` into the result; e.g. (1, 3) → 4, (3, 3) → 6.
* **B3 `fft_radix2`**: permute by bit reversal, then for len = 2, 4, …, N and each block: `u = a[i]; v = a[i+len/2]·w^k; a[i] = u+v; a[i+len/2] = u−v`. Inverse: conjugate-twiddle (or conj/forward/conj) and divide by N. Throw `std::invalid_argument` for non-power-of-two. Validate against `dft` — the unit tests do.
* **B4 `hann_periodic`**: `0.5 − 0.5·cos(2πn/N)` (denominator N, not N−1 — the *periodic* form is the right one for DFT analysis; the *symmetric* N−1 form is for FIR filter design).
* **B5 `peak_bin_estimate`**: argmax |X|, then δ = 0.5(α−γ)/(α−2β+γ) on log-magnitudes, indices wrap modulo N so negative-frequency tones work. Expected error ≲ 0.02 bin with Hann, hence the 0.1 tolerance in tests.

## Part C — model answers
1. Split the sum into even/odd n: X[k] = E[k] + W_N^k O[k]; periodicity of E, O (N/2) and W_N^{k+N/2} = −W_N^k give X[k+N/2] = E[k] − W_N^k O[k]. Each butterfly = 1 complex mult + 2 complex adds; N/2 per stage × log₂N stages ⇒ (N/2) log₂ N complex mults.
2. The DFT sees the frame periodically extended; a non-integer-cycle tone has a jump at the frame edge ⇒ the rectangular window's sinc/Dirichlet sidelobes (−13 dB, −6 dB/oct). Tapering the edges to zero (Hann: −31 dB, −18 dB/oct) removes the jump. Price: wider main lobe (±2 bins), ENBW 1.5 bins (worse noise-floor/resolution), amplitude scaling (coherent gain 0.5) to correct.
3. No — resolution is set by the physical observation length T = N/fs (Δf = 1/T). Zero-padding is interpolation of the DTFT; it helps peak picking/scalloping, not separating close tones.
4. fs = 2.4 MS/s, channel raster 12.5 kHz: choose N so fs/N ≲ 3 kHz, e.g. N = 1024 → 2.34 kHz; apply Hann, average |X|² over many frames (Welch) to stabilise the noise floor, integrate bins within each channel (bins = 12.5 kHz / 2.34 kHz ≈ 5.3 per channel), compare to a threshold set from the noise floor estimate. Use overlap (50–75 %) with Hann to recover the window-lost data.
5. X[N−k] = X[k]* (Hermitian symmetry): only half the spectrum is unique; use a real-to-complex FFT (half the work/memory) or pack two real signals into one complex FFT (x₁ + j x₂ and separate by symmetry).
