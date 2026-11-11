# Pack 09 — SDR Hardware Reality: Front-End Impairments and Fixed-Point (Q15/Q31) Arithmetic

| | |
|---|---|
| Time | ~3 h |
| Needs | calculator, pen, C++17 + CMake for `code/` |
| Signals | synthetic only (no real captures, no proprietary waveforms) |
| Etherstack link | A radio is not a Matlab script: the antenna signal passes an LNA, mixer and ADC that add noise, DC, IQ error, frequency error and phase noise; the DSP that follows usually runs in 16-bit fixed point on a DSP core/FPGA. Interviewers ask "why Q15?", "what happens at overflow?" and "what is wrong with a direct-conversion receiver?" |

> **Honesty note.** All numbers below were derived by hand. Device figures (NF, IIP3, ENOB, phase-noise masks, standard test conditions) are *illustrative*; every figure marked **VERIFY** must be checked against the real datasheet/standard before you quote it.

---

## 1. The receive chain, concretely

```
antenna → preselector filter → LNA → mixer (LO) → IF/anti-alias filter → ADC → DDC (NCO, mixer, CIC, FIR) → baseband I/Q → modem
```

* **LNA**: sets the noise figure (NF) of the whole chain; must tolerate strong blockers (linearity).
* **Mixer + LO**: moves the channel to IF/baseband. LO frequency error → CFO (§3), LO noise → phase noise (§6), LO leakage → DC (§4).
* **ADC**: turns volts into integers: quantisation noise (Pack 01), full-scale clipping, finite dynamic range.
* **DDC** (digital down-converter): a numerically-controlled oscillator (NCO) mixes the channel to 0 Hz, a CIC + FIR low-pass/decimate (Pack 04) leaves a few samples per symbol (e.g. 16 sps at 4800 sym/s).

### 1.1 Cascaded noise figure (Friis) — concrete first
Four stages (illustrative values): preselector filter 1 dB loss; LNA +15 dB, NF 1.5 dB; mixer −7 dB, NF 7 dB; IF amp +20 dB, NF 3 dB.

Linear: F₁ = 1.259 (G₁ = 0.794), F₂ = 1.413 (G₂ = 31.6), F₃ = 5.012 (G₃ = 0.1995), F₄ = 1.995.

F = F₁ + (F₂−1)/G₁ + (F₃−1)/(G₁G₂) + (F₄−1)/(G₁G₂G₃)
 = 1.259 + 0.519 + 0.160 + 0.199 = **2.137 → NF = 3.30 dB**.

Put the LNA first (filter after it): F = 1.413 + 0.008 + 0.160 + 0.199 = 1.779 → **2.50 dB**. The 1 dB filter loss in front of the LNA costs exactly 1 dB NF, but the filter protects the LNA from blockers — a real trade-off (and the answer to "why is the NF of a lossy filter equal to its loss?").

**Derivation.** A stage with noise factor F adds input-referred noise (F−1)kTB. Noise added by stage 2, referred to the input of stage 1, is divided by G₁ (signal and noise are both amplified by G₁ but the stage-2 noise was *not*). Summing gives the formula: **early gain suppresses later noise; early loss adds directly.**

Sensitivity for 12.5 kHz at NF 3.3 dB: −174 + 40.97 + 3.3 = **−129.7 dBm** noise floor; add the required SNR (Pack 08).

### 1.2 ADC dynamic range and bit growth in the DDC
*Illustrative:* 12-bit ADC at fs = 61.44 MS/s (VERIFY your actual part). A full-scale sine has SQNR = 6.02·12 + 1.76 = **74.0 dB** over fs/2 = 30.72 MHz. The wanted channel is 12.5 kHz wide, so the noise falls in 12.5 kHz/30.72 MHz of the band: processing gain 10log₁₀(30.72×10⁶/12.5×10³) = **33.9 dB** ⇒ ≈ 108 dB in-channel quantisation SNR relative to a full-scale sine. Real parts have ENOB < N (e.g. 10.5 bits ⇒ 65 dB; **VERIFY datasheet SINAD/ENOB**), and you also need headroom (PAPR/blocker back-off, Pack 01).

Decimating 61.44 MS/s by R = 800 gives 76.8 kS/s = 16 samples per 4800 sym/s symbol. A 4-stage CIC decimating by 50 grows the word by **N·log₂(R·M) = 4·log₂50 = 22.6 → 23 bits** (Hogenauer), so a 12-bit input needs a 35-bit register — the same "guard bits" idea as §7.5.

### 1.3 NCO (the digital local oscillator)
Phase accumulator of W = 32 bits: φ[n+1] = φ[n] + M (mod 2³²), frequency f = M·fs/2³². Resolution fs/2³² = 61.44×10⁶/4.295×10⁹ = **0.0143 Hz**. To tune 10 MHz: M = 10/61.44·2³² = 699 050 666.67 → 699 050 667, a frequency error of 0.33 × 0.0143 = **0.005 Hz** (negligible). Only the top P bits (e.g. 12) address the sine look-up table; the truncated bits give periodic phase error and spurs at roughly −6.02·P dBc (**VERIFY:** rule of thumb; 12 bits ⇒ ≈ −72 dBc, plus LUT amplitude quantisation).

---

## 2. (Overview) Why impairments matter to a 12.5 kHz radio
12.5 kHz channels are *narrow*, so a few hundred Hz is a big fraction of the signal; adjacent channels are strong (70 dB blockers); and the wanted signal can be at the thermal floor. Each impairment below breaks a specific block of the Pack 07 receiver.

---

## 3. Carrier-frequency offset (CFO)

### 3.1 Source and size — numbers first
Crystal tolerance in ppm becomes Hz at RF: Δf = ppm·10⁻⁶·f_c.

| tolerance | f_c | Δf | rotation per symbol at 4800 sym/s |
|---|---|---|---|
| 1 ppm | 450 MHz | 450 Hz | 360·450/4800 = **33.75°** |
| 1.5 ppm | 460 MHz | 690 Hz | **51.75°** |
| 2.5 ppm | 800 MHz | 2.0 kHz | 150° |

### 3.2 What it does
A baseband sample sequence with CFO is z[n] = x[n]·e^{j2πΔf n/f_s}.
* **4FSK/C4FM** (frequency-coded): CFO adds directly to the demodulated frequency. Levels ±600/±1800 Hz are separated by 1200 Hz with thresholds at 0, ±1200 Hz. **A 690 Hz offset moves the +600 Hz symbol to 1290 Hz — across the +1200 Hz threshold: symbol errors with no noise at all.** Fix: estimate the mean frequency from the preamble/sync (the received mean of a balanced symbol stream should be 0) and subtract it (AFC).
* **PSK/QAM**: the constellation rotates; the carrier loop (Costas, Pack 07) must pull it in. Pull-in range of the M-th-power estimator is ±R_s/(2M) (QPSK: ±R_s/8 = ±600 Hz at 4800 sym/s — **already smaller than the CFO of row 2**).
* **Sync correlation**: Pack 07 showed the coherent correlation loss |sin(πLf)/(L sin πf)| with f = Δf/R_s. For L = 24 symbols and Δf = 450 Hz (f = 0.09375): sin(2.25π) = 0.7071; 24·sin(0.2945) = 6.967 ⇒ 0.101 = **−19.9 dB**. The detector must search a CFO grid, or correlate in shorter segments and combine non-coherently.

### 3.3 Estimating it (lag-1 autocorrelation)
For z[n] = A e^{j(2πfn+θ)} + w, the sum Σ z[n+1]z*[n] = (N−1)A²e^{j2πf} + noise-terms, so **f̂ = arg(Σ z[n+1]z*[n])/2π** (Pack 02). Unambiguous for |f| < 0.5 cycles/sample, and for a data-bearing signal one first removes the modulation using known symbols (data-aided) or the M-th power (non-data-aided, range ÷M).

---

## 4. DC offset

### 4.1 Where it comes from
Direct-conversion ("zero-IF") mixers have LO-to-RF leakage; the leaked LO self-mixes with itself giving a constant (DC) at the mixer output; ADC and amplifier offsets add more. So z[n] = x[n] + (d_I + j d_Q): a **spike at bin 0**. A 5 % of full-scale offset steals 5 % of the ADC range.

### 4.2 Why you can't always notch DC
The averaged C4FM/FM signal has energy right at 0 Hz (a mean-free symbol stream *is* centred there). A DC-blocking high-pass distorts the signal when the signal sits on DC. Remedies:
1. **Low-IF**: put the channel at ±(12.5…25) kHz away from DC, notch DC, then digitally shift (the DDC of §1.3). Costs the image problem of §5.
2. Estimate DC over a window when the signal is balanced and subtract (`remove_dc`, mean subtraction).
3. A single-pole DC blocker y[n] = x[n] − x[n−1] + αy[n−1] (Pack 04), −3 dB at ≈ (1−α)fs/2π. At fs = 76.8 kS/s, α = 0.999 ⇒ 12.2 Hz.

---

## 5. IQ imbalance (derivation + numbers)

### 5.1 Model
The two mixer arms have gain mismatch g (Q arm relative to I) and phase error φ (the 90° isn't exactly 90°):

I' = I, Q' = g(Q cos φ + I sin φ).

For a complex tone z = e^{jθ}: I' = cos θ, Q' = g sin(θ+φ). Write x' = I' + jQ':

x' = ½(1 + g e^{jφ}) e^{jθ} + ½(1 − g e^{−jφ}) e^{−jθ}.

Derivation: cos θ = (e^{jθ}+e^{−jθ})/2 and j g sin(θ+φ) = (g/2)(e^{j(θ+φ)} − e^{−j(θ+φ)}); collect e^{jθ} and e^{−jθ}. **The ideal signal has only e^{+jθ}; the mirror term at −f is the *image*.**

**Image rejection ratio**: IRR = |1 + g e^{jφ}|² / |1 − g e^{−jφ}|².

* g = 1, φ = 0 → infinite.
* Phase only: IRR = cot²(φ/2) ≈ 4/φ².  1° ⇒ cot²(0.5°) = 13 131 ⇒ **41.2 dB**.
* Gain only: IRR = ((g+1)/(g−1))². g = 1.01 (0.09 dB) ⇒ (2.01/0.01)² = 40 401 ⇒ **46.1 dB**.
* Both small (g = 1+ε): **IRR ≈ 4/(ε²+φ²)**.
* g = 1.1, φ = 5°: numerator (2.0958)² + (0.0959)² = 4.4016; denominator (0.0958)² + (0.0959)² = 0.01837 ⇒ 239.6 ⇒ **23.8 dB** (approximation: 4/(0.01+0.00762) = 227 ⇒ 23.6 dB).

An analogue front-end typically achieves 25–40 dB (**VERIFY** per device). A strong adjacent-channel signal 60–70 dB above the wanted one then leaves its image *on top of* the wanted channel when the wanted channel is its mirror — hence digital correction.

### 5.2 Blind correction (Gram–Schmidt)
Assume the true signal is *circular*: E[I²] = E[Q²], E[IQ] = 0 (true for a tone and for QPSK/noise-like signals; **false** for a real BPSK signal — the algorithm would "correct" a legitimate asymmetry).
With the measured I', Q':
1. ρ = E[I'Q']/E[I'²] = g sin φ (because E[cos θ·sin(θ+φ)] = ½ sin φ and E[I'²] = ½).
2. Q₁ = Q' − ρI' = g cos φ · sin θ (the I leakage is removed — orthogonalised).
3. Rescale Q₁ so E[Q₁²] = E[I'²]: Q = Q₁/(g cos φ) = sin θ. **Perfect restoration for the tone** (in practice limited by estimation noise and by frequency-dependent imbalance).

---

## 6. Phase noise

### 6.1 Model and linewidth (derivation)
Oscillator phase as a random walk (Wiener process): φ[n] = φ[n−1] + w[n], w ~ N(0, σ²). Over τ samples the phase difference has variance τσ², so E[e^{jΔφ}] = e^{−τσ²/2}: an exponentially decaying autocorrelation e^{−|τ|/τ_c}, τ_c = 2/σ² samples. Its spectrum is a **Lorentzian** with full width at half-maximum

**Δν = σ² f_s/(2π)**.

Example: σ = 0.01 rad/sample, f_s = 76.8 kHz ⇒ σ² = 10⁻⁴ ⇒ **1.22 Hz** linewidth. (σ = 0.02 rad at 48 kHz ⇒ 3.06 Hz.)

### 6.2 Effects
* A random rotation of the constellation: tolerable for 4FSK, damaging for dense QAM and OFDM (common phase error + inter-carrier interference).
* **Reciprocal mixing (the one that kills radio sensitivity).** A strong blocker at offset Δf mixes with the LO's noise skirt L(Δf) [dBc/Hz] and lands in the channel as noise: 

 N_rm = P_blocker + L(Δf) + 10log₁₀(B).

 Example: adjacent-channel blocker −48 dBm (≈ 70 dB above a −118 dBm sensitivity; **VERIFY the ACS test condition in TIA-102.CAAA / ETSI TS 102 361-2**), channel 12.5 kHz, L(25 kHz) = −120 dBc/Hz ⇒ −48 − 120 + 40.97 = **−127.0 dBm** — equal to the 6 dB-NF noise floor (Pack 08). Anything worse than −120 dBc/Hz *degrades the sensitivity* in the presence of the blocker. The synthesiser phase-noise spec at 25 kHz offset is therefore set by adjacent-channel selectivity, not by the modulation.

---

## 7. Fixed point: Q15 and Q31

### 7.1 Why fixed point
Deterministic (bit-exact on every run/device), cheap (an int16×int16 MAC is a few gates/one cycle on a DSP or FPGA DSP slice), low power, and no FPU needed. Floating point costs area/power, has non-associative rounding (so it is hard to make *bit-identical* — Pack 10), but keeps relative accuracy over a huge dynamic range with no scaling work. Rule: *signal chains with known bounded range → fixed-point; algorithms with huge or unknown dynamic range (matrix inversion, spectral estimation) → float.*

### 7.2 Number format
Q15: a signed 16-bit integer r represents **r/2¹⁵** ∈ [−1, 1 − 2⁻¹⁵ = 0.999969]. LSB Δ = 2⁻¹⁵ = 3.05×10⁻⁵. Q31: int32/2³¹. (Note the asymmetry: −1 exists, +1 does not.)

Quantising x: r = round(32768x), then saturate:

| x | 32768x | Q15 raw | value | error |
|---|---|---|---|---|
| 0.1 | 3276.8 | 3277 | 0.100006 | +0.2 LSB |
| 0.3 | 9830.4 | 9830 | 0.299988 | −0.4 LSB |
| 0.5 | 16384 | 16384 | 0.5 | 0 |
| 0.7 | 22937.6 | 22938 | 0.700012 | +0.4 LSB |
| 0.999 | 32735.2 | 32735 | | |
| 1.0 | 32768 | **32767** (saturated) | 0.999969 | −1 LSB |
| −1.0 | −32768 | −32768 | −1.0 | 0 |

Error is uniform in ±½ LSB (noise power Δ²/12), so (Pack 01) SQNR = 6.02·N + 1.76 with N = 16 bits (sign included) = **98.1 dB** for a *full-scale* sine (amplitude 1). Each halving of amplitude costs 6 dB: amplitude 0.5 gives 10log₁₀(1.5·2³⁰) = **92.1 dB** (this is what the unit test checks). Always state which amplitude you are quoting — "16-bit = 98 dB" assumes full scale.

### 7.3 Addition and overflow
Two's-complement addition **wraps**: 0.75 + 0.5 → 24576 + 16384 = 40960 = 40960 − 65536 = −24576 = **−0.75**. A huge sign-flip error (a loud signal becomes a large *opposite* value: a click/blast, and in a feedback loop possibly a latch-up). **Saturation** clamps to 32767 (0.99997): an error but a bounded one (clipping distortion, correctable by AGC). Radio DSP code saturates (DSP instructions `ADDS`, `QADD16`, ...).

### 7.4 Multiplication (derivation)
Values a = A/2¹⁵ and b = B/2¹⁵. Product = AB/2³⁰: a **Q30** number in a 32-bit register (|AB| ≤ 2³⁰, so one sign bit is redundant). To return to Q15, divide by 2¹⁵ = shift right 15:

y = (A·B + 2¹⁴) >> 15 (the +2¹⁴ = ½ LSB makes it *round to nearest* rather than floor).

* 0.5×0.5: 16384² = 268 435 456; +16384 ⇒ >>15 = 8192 = 0.25 ✓.
* 0.7×0.7: 22938² = 526 151 844 ⇒ (+16384) >> 15 = 16057 = 0.49002 (ideal 0.49 → raw 16056.3; the 0.68 LSB difference is mostly the *input* quantisation of 0.7).
* **(−1)×(−1)**: (−32768)² = 2³⁰ ⇒ >>15 = 32768, which is **not representable** ⇒ must saturate to 32767; without saturation it wraps to −32768 = −1: "−1 × −1 = −1".
* Truncation (no +2¹⁴) biases every result by −½ LSB on average ⇒ DC offset in a recursive filter. Rounding here is "ties toward +∞"; convergent (round-half-even) removes the tie bias — **VERIFY which your DSP uses**.
* Right-shift of negative signed integers is arithmetic (floor) on all mainstream compilers/DSPs but implementation-defined before C++20 — document it.

### 7.5 FIR in fixed point and guard bits
y[n] = Σ_k h[k]x[n−k]. Each product is Q30; summing N of them grows the sum by up to log₂N bits. **Accumulate wide** (64-bit in C++; a 40-bit DSP accumulator has 8 guard bits; an FPGA DSP slice has 48), **round and saturate once at the end** — never after each product (that adds N independent rounding errors).

Worst-case output peak = (Σ|h[k]|)·max|x|. So if Σ|h| = 3.2 the output may need ⌈log₂3.2⌉ = **2 extra integer bits** (Q2.13) or you scale the taps down by 2 bits (losing 12 dB of coefficient resolution) or accept saturation on rare peaks. Typical 64-tap unity-DC-gain low-pass: Σ|h| ≈ 1.2–1.6 ⇒ 1 guard bit.

Steady-state demo: h = [0.25, 0.5, 0.25] = [8192, 16384, 8192], x = 0.5 = 16384: acc = 16384·32768 = 2²⁹ ⇒ (2²⁹ + 2¹⁴) >> 15 = 16384 = 0.5 ✓ (unity DC gain preserved exactly).

### 7.6 Q31 and coefficients
Q31 (int32, 64-bit products): ≈ 186 dB SQNR; used for IIR state/coefficients (poles close to the unit circle need many coefficient bits), NCO phase, accumulators. A Q15 biquad with a pole radius 0.999 has coefficient quantisation error of 3×10⁻⁵ which moves the pole by the same order — often the dominant error at low cutoff-to-fs ratios. Prefer Q31 states or error feedback for narrow filters.

---

## 8. Misconceptions

1. **"Saturation fixes overflow."** It bounds the error but clips: harmonic distortion and wrong values still occur. Choose scaling so saturation is *rare*, and count it.
2. **"A Q15 multiply is just `a*b`."** It needs the 15-bit shift (and rounding), a wider intermediate, and special handling of −1×−1.
3. **"Rounding after every tap is more accurate."** The reverse: accumulate wide, round once.
4. **"Floating point is always better."** It is not bit-exact across platforms/optimisations and costs power/area; fixed point can be exactly reproducible.
5. **"Subtract DC with a high-pass and forget it."** On a zero-IF FM/4FSK receiver this removes signal; use low-IF or block estimation.
6. **"IQ imbalance only affects the signal's quality."** Its image is a *mirror copy of the strongest blocker* in the band — a fixed IRR means the blocker image is a fixed dB below the blocker, however weak the wanted signal.
7. **"CFO is just a phase rotation."** For frequency-coded signals it is a threshold shift, and for correlators it is a loss that grows with correlation length.
8. **"16-bit audio means 98 dB SNR."** Only for a full-scale sine; at −20 dBFS it is 78 dB.

---

## 9. Etherstack / radio context
* A narrowband radio modem receive chain: ADC → DDC → channel filter → demodulator, usually in an FPGA/DSP at 16-bit data and wider accumulators. Test vectors from an Octave/Matlab float model are compared bit-for-bit with the fixed-point model (Pack 10).
* Stated requirements (sensitivity, adjacent-channel selectivity, frequency stability) come from the conformance documents (TIA-102.CAAA for P25, ETSI TS 102 361-2 for DMR; **VERIFY** exact figures and test conditions) — they in turn set NF, LO phase noise, crystal tolerance and ADC bits.
* "Why isn't the FFT of my loopback test clean?" → check DC spike, image tone, and quantisation floor first.

---

## 10. Exercises

### Part A — pen & paper
**A1.** Convert 0.2, −0.65, 1.2 to Q15 (with rounding and saturation).
**A2.** Compute 0.6 × (−0.4) in Q15 exactly as the hardware does (raw values, product, +2¹⁴, shift).
**A3.** 0.9 + 0.4 in Q15: wrapped result and saturated result.
**A4.** A 10-tap FIR has Σ|h| = 3.2. Guard bits? Output format?
**A5.** (a) SQNR of a full-scale 16-bit sine. (b) At −20 dBFS. (c) A datasheet quotes SINAD 62 dB: ENOB?
**A6.** 1.5 ppm at 460 MHz, 4800 sym/s C4FM (levels ±600/±1800 Hz): CFO in Hz? Per-symbol rotation? Which symbols are misdetected?
**A7.** Image rejection for g = 1.05, φ = 2°: exact and with the small-error approximation.
**A8.** Phase-noise linewidth for σ = 0.02 rad/sample at 48 kHz.
**A9.** Blocker −40 dBm, floor target −127 dBm, 12.5 kHz channel: required L(Δf)?

### Part B — C++ (`code/`)
Stubs are in `code/stubs/hardware_q15.cpp`; tests in `code/tests/`. Build (stubs fail by design):

```
cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure
cmake -S code -B build -DUSE_SOLUTIONS=ON      # reference solutions
```
* **B1** `float_to_q15`, `q15_to_float` — round-to-nearest (ties away), saturate.
* **B2** `q15_add_sat`.
* **B3** `q15_mul`, `q31_mul` — rounding shift and −1×−1.
* **B4** `q15_fir` — int64 accumulator, round once, saturate.
* **B5** `guard_bits`.
* **B6** `remove_dc`.
* **B7** `correct_iq_imbalance` — Gram–Schmidt; test measures the image before/after (provided `apply_iq_imbalance`, `measure_image_rejection_db`).
* **B8** `cascade_nf_db` — Friis.
* **B9** `ppm_to_hz`, `cfo_phase_per_symbol_deg`, `phase_noise_linewidth_hz`, `reciprocal_mixing_floor_dbm`.

### Part C — interview questions
1. Why would a radio modem use Q15 instead of float? When would you choose float?
2. Walk through a Q15 multiply. What happens with −1 × −1? Why add 2¹⁴?
3. 64-tap Q15 FIR: how wide is the accumulator, where do you round, how do you choose scaling, what do you do about saturation?
4. What problems does a direct-conversion receiver have and how do you mitigate each (DC, IQ imbalance, 1/f noise, LO leakage)?
5. The crystal is 1.5 ppm off. What happens to a 4FSK receiver? To a 24-symbol sync correlator? How do you cope?
6. How do you verify that your fixed-point C++ matches the reference model? (Pack 10.)

---

## 11. VERIFY list
| Item | Used here | Check against |
|---|---|---|
| Stage gains/NF in §1.1; ADC 12 bit/61.44 MS/s/ENOB | illustrative | real front-end datasheets |
| NCO spur rule ≈ −6.02·P dBc | rule of thumb | NCO/DDS vendor notes |
| CIC growth N·log₂(R·M) | Hogenauer | filter-design reference |
| ACS ≈ 70 dB / blocker −48 dBm | illustrative | TIA-102.CAAA; ETSI TS 102 361-2 |
| Required LO phase noise −120 dBc/Hz @ 25 kHz | follows from above | synthesiser datasheet |
| Typical analogue IRR 25–40 dB | rule of thumb | mixer/transceiver datasheet |
| 4FSK deviations ±600/±1800 Hz, 4800 sym/s, 9600 bit/s | C4FM (Pack 05) | TIA-102.BAAA |
| Rounding mode of the target DSP/FPGA | ties toward +∞ here | ISA manual |
| Nothing in this pack was compiled or executed | no toolchain in the authoring environment | run the tests |
