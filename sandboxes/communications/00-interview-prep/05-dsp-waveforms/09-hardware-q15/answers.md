# Pack 09 — Answers (Hardware reality and Q15)

Hand-computed; nothing executed. **VERIFY** marks items to check against datasheets/standards.

## Part A

**A1.** 0.2·32768 = 6553.6 → **6554**; −0.65·32768 = −21299.2 → **−21299**; 1.2·32768 = 39321.6 → saturate → **32767** (0.99997, an error of 0.2 in value).

**A2.** 0.6 → 19660.8 → 19661; −0.4 → −13107.2 → −13107. Product = −19661·13107 = −257 696 727 (Q30). Add 2¹⁴: −257 680 343. Divide by 2¹⁵ (arithmetic shift = floor): −7863.78 → **−7864** = −0.23999 (ideal −0.24 → −7864.3).

**A3.** 0.9 → 29491, 0.4 → 13107; sum 42598. Wrapped: 42598 − 65536 = **−22938 = −0.700** (a sign flip!). Saturated: **32767 = 0.99997**.

**A4.** Worst-case growth ⌈log₂ 3.2⌉ = **2 guard bits**. Output format Q2.13 (sign + 2 integer + 13 fraction bits) in 16 bits, or scale the taps by ¼ (cost: 12 dB of coefficient resolution), or keep a wider output / accept rare saturation if |x| rarely peaks together with the taps' sign pattern. Accumulator itself must be ≥ 32 + 2 bits (use 64 in C++).

**A5.** (a) 6.02·16 + 1.76 = **98.1 dB**. (b) −20 dBFS ⇒ **78.1 dB**. (c) ENOB = (62 − 1.76)/6.02 = **10.0 bits**.

**A6.** Δf = 1.5×10⁻⁶ × 460×10⁶ = **690 Hz**; rotation per symbol 360·690/4800 = **51.75°**. Levels after offset: +600 → 1290 (> 1200 ⇒ decided +1800 ✗), −600 → +90 (> 0 ⇒ +600 ✗), +1800 → 2490 (✓ +1800), −1800 → −1110 (> −1200 ⇒ −600 ✗). **Three of four levels are misdetected with zero noise**: the receiver *must* estimate and remove the mean frequency (AFC from preamble/sync).

**A7.** Exact: |1+1.05e^{j2°}|² = 4.2012; |1−1.05e^{−j2°}|² = 0.003779; ratio 1111.7 ⇒ **30.46 dB**. Approximation 4/(ε²+φ²) = 4/(0.0025+0.001218) = 1076 ⇒ 30.32 dB. (0.15 dB error; both inside the "25–40 dB typical analogue" range.)

**A8.** σ²f_s/2π = (4×10⁻⁴)(48 000)/6.283 = **3.06 Hz**.

**A9.** L ≤ floor − blocker − 10log₁₀B = −127 − (−40) − 40.97 = **−128 dBc/Hz** at the blocker offset.

## Part B — notes
* **B1** `float_to_q15`: scale, **clamp before** casting (casting 40000 to int16 is UB/wraps), then `lround`. Ties: 0.5 LSB → 1, −0.5 LSB → −1 (away from zero).
* **B2** Add in int32, clamp to [−32768, 32767].
* **B3** `q15_mul`: int32 product, `+ (1<<14)`, `>> 15`, clamp. The only overflow is (−32768)² (= 2³⁰ → 32768). Ties go toward +∞ (−1·16384 → 0 but +1·16384 → 1). `q31_mul` identical with int64 and shift 31.
* **B4** Per output: sum int64 products, add 2¹⁴, shift, clamp. Error against an exact real-valued reference is ≤ ½ LSB when nothing saturates (the test checks that against a double reference computed from the same integer taps — i.e. the *only* error is the single final rounding).
* **B5** `guard_bits`: Σ|h|, if ≤ 1 → 0; else `ceil(log2(sum))`. Boundary: Σ = 4 exactly gives 2 (the peak equals 4× full-scale ⇒ 2 bits).
* **B6** Mean of I and of Q.
* **B7** Sums sII, sIQ, sQQ; ρ = sIQ/sII; Q₁ = Q − ρI with power sQQ − ρ sIQ; scale by √(sII/p₁). Return the input if sII ≤ 0 or p₁ ≤ 0 (silent/degenerate input). For a tone with integer cycles the residual image is limited only by floating-point rounding (> 150 dB).
* **B8** Friis: keep running ΠG; F_total = F₁ + Σ (F_i − 1)/ΠG_{<i}. Test: 3.30 dB (filter first), 2.50 dB (LNA first).
* **B9** ppm·10⁻⁶·f_c (1 ppm @ 450 MHz = 450 Hz); 360·Δf/R_s (33.75°); σ²f_s/2π (1.222 Hz for σ = 0.01, f_s = 76.8 kHz); blocker + L + 10log₁₀B (−127.03 dBm).

## Part C — model answers
1. **Fixed vs float.** Fixed point is deterministic/bit-exact, small and fast in hardware (FPGA DSP slices, fixed-point DSP cores), low-power, and the signal range in a radio chain is bounded (AGC + ADC). Costs: scaling/overflow planning, noise from rounding, limited dynamic range per word. Float: easy, huge range, but non-associative (hard to get bit-exact against a reference), more power/area, and no hardware FPU on many low-end cores. Use float for algorithms with unbounded dynamic range (matrix solves, adaptive algorithms with large conditioning) and prototyping; use fixed for the high-rate datapath (DDC, filters, demod) and for anything that must match test vectors exactly.
2. Q15×Q15 → Q30 in a 32-bit register; shift 15 to return to Q15. +2¹⁴ makes it round-to-nearest (otherwise floor ⇒ −½ LSB bias ⇒ DC offsets in recursive structures). −1×−1 = +1 not representable ⇒ saturate, otherwise it wraps to −1 (a classic bug in speech/radio codecs).
3. Accumulate in ≥ 32 + ⌈log₂N⌉ bits (64-bit in C++, a 40-bit MAC on DSPs), round once at the end, saturate. Scale taps so Σ|h| ≤ 1 or provision guard bits = ⌈log₂Σ|h|⌉; count saturation events as a diagnostic (a saturating filter output is a symptom of poor AGC or scaling).
4. Direct conversion: (i) DC offset (LO self-mixing) → low-IF or DC estimation/removal; (ii) IQ imbalance → image of blockers → calibration/blind Gram–Schmidt; (iii) flicker (1/f) noise at DC → same low-IF cure; (iv) LO leakage radiating from the antenna (a regulatory/EMC issue) → buffer isolation/filtering; (v) even-order distortion (IIP2) → DC from a strong AM-like blocker → mixer design/high-pass calibration.
5. 1.5 ppm at 460 MHz = 690 Hz: 4FSK levels are mis-sliced (three of four, with zero noise) — you need AFC from preamble/sync, or a frequency tracker that uses the symbol-mean = 0 property. A 24-symbol coherent correlator suffers a large loss (e.g. −19.9 dB at 450 Hz); fix by correlating differential/segmented, searching a CFO grid, or correcting the CFO first with a coarse estimate from the preamble. Long-term: trim/AFC the reference (TCXO, reference from network/GPS).
6. Run the same inputs through a double/Octave model and the fixed-point C++, require bit-exact for integer pipelines and a tight tolerance (≤ ½ LSB × expected rounding count) when the reference is float; log saturation counts; include corner vectors (full-scale, −1×−1, zeros, impulses, random). See Pack 10.
