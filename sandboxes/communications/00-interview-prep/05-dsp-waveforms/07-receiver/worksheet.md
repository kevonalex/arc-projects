# Pack 07 — Receiver Building Blocks: Matched Filter, Sync Correlation, Gardner Timing, PLL/Costas, AGC

| | |
|---|---|
| Time | ~2.5 h |
| Needs | calculator, pen, C++17 + CMake for `code/` |
| Signals | synthetic only |
| Etherstack link | the receive chain behind every narrowband LMR modem: AGC → channel filter → matched filter → timing recovery → carrier/frequency recovery → slicer → frame sync (48-bit sync word) |

---

## 1. Matched filter: the maximum-SNR filter

### 1.1 Concrete first (3-sample pulse)
Received r[n] = s[n] + w[n], s = [1, 2, 3], white noise variance σ² = 1. We decide using one filter-output sample taken when the whole pulse has entered the filter (output index N−1 = 2). Output signal value:

y_s = Σ_k h[k]·s[N−1−k]; noise variance = σ²·Σh².

| filter h | y_s | Σh² | SNR = y_s²/(σ²Σh²) |
|---|---|---|---|
| [3, 2, 1] (= s reversed) | 9+4+1 = 14 | 14 | **14** |
| [1, 1, 1] (boxcar) | 1+2+3 = 6 | 3 | 12 (−0.67 dB) |
| [1, 0, 0] (one sample) | 3 | 1 | 9 (−1.9 dB) |

The reversed pulse wins, and 14 = E_s/σ² (E_s = 1+4+9 = 14).

### 1.2 Derivation (Cauchy–Schwarz)
SNR = (Σ h[k] u[k])² / (σ² Σ h[k]²) with u[k] = s[N−1−k]. By Cauchy–Schwarz, (Σ h u)² ≤ (Σh²)(Σu²), so

**SNR ≤ Σu²/σ² = E_s/σ²**, with equality iff h ∝ u, i.e. h[k] = s[N−1−k]: **the matched filter is the time-reversed (and for complex signals conjugated) pulse**. Its output is the *correlation* of the received signal with the known pulse.

Key consequences:
* The achievable SNR depends only on pulse **energy** over noise density, *not* on pulse shape → this is where "E_b/N0" comes from (Pack 08).
* After an RRC transmit filter, the matched filter is the receive RRC (Pack 06).
* Detection of a known sync word = matched filtering = correlation (§2).

---

## 2. Sync-word correlation

### 2.1 Idea
Frame sync words are fixed patterns the receiver searches for. Slide the known reference over the received stream and compute the correlation at each lag; the peak marks frame/slot start (and gives coarse timing and phase).

For complex I/Q: c[l] = Σₖ r[l+k]·conj(ref[k]). Normalised metric (always in [0,1], by Cauchy–Schwarz):

ρ[l] = |c[l]|² / ( Σₖ|r[l+k]|² · Σₖ|ref[k]|² ).

ρ = 1 means "the window is exactly a scaled/rotated copy of ref".

### 2.2 Hard-decision (bit-level) correlation and false alarms — numerical demo
For a 48-bit sync word, count bit errors (Hamming distance) between the last 48 received bits and the word; declare sync if errors ≤ d. For random data each of the 2⁴⁸ patterns is equally likely:

P_fa(d) = Σ_{k=0}^{d} C(48,k) / 2⁴⁸.

* d = 0: 1/2⁴⁸ = 3.55×10⁻¹⁵.
* d = 4: C(48,0..4) = 1 + 48 + 1128 + 17 296 + 194 580 = 213 053 ⇒ P_fa = 213 053 / 2.815×10¹⁴ = **7.57×10⁻¹⁰** per position.
* At 9600 bit/s and testing every bit position: 9600 × 7.57×10⁻¹⁰ = 7.3×10⁻⁶ per second ≈ **one false alarm every 38 hours** — but you'd also gain missed-detection robustness: with BER 5 % the number of errors in a 48-bit window is Binomial(48, 0.05), mean 2.4, so P(≤4 errors) ≈ 0.91.
Trade-off: larger d ⇒ fewer missed syncs, more false alarms. *Always compute both.*

### 2.3 Carrier-frequency offset kills long coherent correlations (derivation)
A CFO of f (cycles/symbol) rotates symbol k by e^{j2πfk}. The coherent sum over L symbols becomes a geometric series with magnitude

|Σ e^{j2πfk}| = |sin(πLf)/sin(πf)|, i.e. a loss factor **|sin(πLf)/(L sin πf)|** relative to f = 0.

*Example (L = 24 symbols, 300 Hz at 4800 sym/s ⇒ f = 0.0625):* sin(π·1.5) = −1, L sin(πf) = 24 × 0.1951 = 4.68 ⇒ **0.214 = −13.4 dB**. A coherent I/Q correlator over 24 symbols is nearly blind at 300 Hz error. Remedies: correlate over shorter blocks and add magnitudes (non-coherent combining), correlate on the **discriminator output** (frequency-domain data where CFO is a DC offset, so remove the mean first), correlate differentially, or estimate/correct CFO first. FSK receivers typically use the last option.

---

## 3. Symbol-timing recovery: the Gardner detector

### 3.1 Why it is needed
The receiver's sample clock has an unknown phase relative to the symbols and a slightly different frequency (±ppm). With 8 samples per symbol and the wrong phase, the sampled value is off-peak and ISI is no longer zero (eye diagram, Pack 06). We need a loop that steers the sampling instants.

### 3.2 The Gardner timing-error detector (TED)
Needs **2 samples per symbol**: one at the symbol strobe y[k] and one midway y[k−½]:

**e[k] = (y[k] − y[k−1]) · y[k−½]** (real signals; for complex use the real part of the product with the conjugate).

*Intuition / derivation (BPSK):* when two consecutive symbols differ (+1 → −1) the waveform crosses zero midway between them if the timing is right, so y[k−½] ≈ 0 ⇒ e = 0. If we sample **late** by τ the "midpoint" sample is already past the zero-crossing: it has the sign of the *new* symbol, which is the same as the sign of (y[k] − y[k−1]) ⇒ e > 0 (late). If early, the sign flips ⇒ e < 0. If consecutive symbols are equal, y[k] − y[k−1] ≈ 0 ⇒ e ≈ 0 (no information, no noise injected). Gardner needs no decisions (non-data-aided) and is insensitive to carrier phase for real BPSK/for the complex version.

*Linearised gain:* RC β = 0.35: slope h′(0.5T) = −1.31/T (derivative of ln h at 0.5T: −2 − 0.674 + 0.558 = −2.115; times h(0.5T) = 0.619 ⇒ −1.31). A transition (prob ½) has y[k]−y[k−1] ≈ −2 and y[k−½] ≈ 2τh′(0.5T) ⇒ e ≈ 5.2τ/T; averaged over data (half the pairs are transitions) K_d ≈ 2.6 per symbol period (**sign convention: e > 0 ⇒ sampling late**).

### 3.3 The loop
Strobe times t_k; after computing e[k]: t_{k+1} = t_k + sps − (k_p e[k] + I[k]), I[k+1] = I[k] + k_i e[k]. Linearised with τ in samples: e = K_d τ/sps; so per symbol τ ← τ(1 − k_p K_d / sps). Choose k_p K_d/sps ≈ 0.05 ⇒ time constant ≈ 20 symbols: with sps = 8, K_d = 2.6 ⇒ **k_p ≈ 0.15**. The integrator (k_i ≈ k_p/100) tracks a symbol-clock *frequency* offset (a ramp of τ). Second-order characteristic polynomial: z² − (2 − K_p′)z + (1 − K_p′ + K_i′) with K_p′ = k_pK_d/sps, K_i′ = k_iK_d/sps. For K_p′ = 0.049, K_i′ = 0.00049 the roots are 0.986 and 0.965 — inside the unit circle ✓.

*Interpolation:* the strobe time is fractional; we use **linear interpolation** between neighbouring samples (cheap; fine at ≥ 8 sps; use a cubic/Farrow interpolator for 4 sps).

---

## 4. Carrier recovery: PLL and Costas loop

### 4.1 A PLL in four lines
Phase detector → loop filter → NCO (numerically controlled oscillator). Digital second-order loop for phase error e:

```
freq += β·e;       phase += freq + α·e;       y = x·e^{−j·phase}
```
α (proportional) fixes the damping; β (integral) lets the loop **track a frequency offset with zero steady-state phase error** (type-2 loop).

### 4.2 Costas loop for BPSK (derivation)
For BPSK data a = ±1 and residual phase θ: y = a e^{jθ}. Then Re(y) = a cos θ, Im(y) = a sin θ and

e = Re(y)·Im(y) = a² cos θ sin θ = ½ sin 2θ ≈ θ (small θ).

The data sign cancels (a² = 1), so no knowledge of the symbols is needed. Stable equilibria at θ = 0 **and θ = π** (sin 2θ = 0, positive slope): a **180° ambiguity**, resolved by the sync word or differential coding. (QPSK Costas: 4-fold ambiguity.)

### 4.3 Loop gains from the noise bandwidth (numerical demo)
With loop noise bandwidth B_nT (per sample), damping ζ (0.707), detector gain 1:
θn = B_nT/(ζ + 1/(4ζ)); d = 1 + 2ζθn + θn²; **α = 4ζθn/d; β = 4θn²/d** (standard result, e.g. Rice, *Digital Communications: A Discrete-Time Approach*).
For B_nT = 0.02: θn = 0.02/(0.7071+0.3536) = 0.01886; d = 1.02702; **α = 0.05193, β = 0.001385**. Smaller B_nT → less noise, slower acquisition, narrower pull-in range.

*Pull-in:* a second-order loop reliably locks when |Δω| is below a few times the loop's natural frequency (≈ θn rad/sample). For bigger CFO do a coarse FFT-based/M-th-power estimate first (Pack 03, 10).

### 4.4 FM/FSK receivers
A 4FSK discriminator receiver has no carrier phase loop at all (CFO appears as a DC offset removed by averaging). That's why legacy narrowband FM/FSK radios are simple and robust — at the cost of ≈ 1–3 dB sensitivity vs. coherent.

---

## 5. Automatic gain control (AGC)

**Why:** received power varies by 60+ dB (distance, fading); ADC range and loop detector gains (Costas detector gain ∝ A², Gardner ∝ A²) need constant amplitude.

**Log-domain loop (derivation of why it is nice):** g ← g·exp(μ(r − |y|)), y = g·x. Let u = ln g. For constant input amplitude A: |y| = A e^{u}; near the fixed point u* = ln(r/A) the error is |y| − r ≈ r(u − u*), so u − u* decays by (1 − μ r) per sample: **time constant 1/(μ r) samples, independent of input level A**. A linear update g ← g + μ(r − |y|) has time constant 1/(μ A): slow for weak signals, unstable for strong ones. Numerical sanity check: μ = 0.01, r = 1: time constant 100 samples whether A = 0.01 or A = 100.

Design points: fast attack/slow decay (protects ADC, avoids pumping on speech pauses), freeze during preamble if the loops depend on a known level, never AGC *after* a filter that destroys the amplitude information you need (e.g. FM limiters).

---

## 6. Misconceptions
1. **"A matched filter maximises peak amplitude."** It maximises *SNR* at the sampling instant, for white noise.
2. **"Matched filter = same as the transmit filter."** Time-reverse (and conjugate). Equal only for symmetric real pulses.
3. **"Longer sync word ⇒ always better."** Longer coherent correlation is more sensitive to CFO (§2.3).
4. **"Gardner needs the symbol decisions."** It is non-data-aided.
5. **"Costas resolves the carrier phase fully."** It locks to a multiple of π (BPSK) or π/2 (QPSK).
6. **"Loop gain = speed only."** Faster loops also pass more noise (jitter) — trade-off via B_nT.
7. **"AGC is for convenience."** Several loops (and the slicer thresholds) are only calibrated for a nominal amplitude.

## 7. Etherstack / radio context
* **48-bit frame sync** in P25/DMR-style framing — correlate on symbols (4 levels) or dibits; compute your own false-alarm and miss rate for the threshold chosen (§2.2). *(Sync pattern values: P25 frame sync is commonly cited as 0x5575F5FF77FF and uses only the outer ±3 symbols; DMR defines several 48-bit syncs (e.g. BS-sourced voice 0x755FD7DF75F7) — **VERIFY both against TIA-102.BAAA / ETSI TS 102 361-1** before relying on them. All code in this pack uses synthetic words.)*
* **Symbol clock:** 4800 sym/s with a 5 ppm clock error drifts 4800 × 5×10⁻⁶ = 0.024 symbols/s ⇒ one whole symbol of slip every 1/0.024 ≈ 42 s without tracking (a 100-symbol burst accumulates only 0.0005 symbol, so bursts mostly need *phase* acquisition, long calls need a *frequency*-tracking loop); real systems also have fading and burst starts so a loop (or per-burst timing from the sync correlation) is needed.
* **Frequency offset:** handsets and base stations have ppm-level error: at 400 MHz, 1 ppm = 400 Hz; at 4800 sym/s that is 0.083 cycles/symbol — **well beyond a coherent 24-symbol sync correlation** (§2.3).

---

## 8. Exercises

### Part A — pen & paper
1. s = [2, −1, 1], σ² = 0.5. Find the matched filter, output SNR, and SNR for h = [1, 1, 1]. Loss in dB?
2. Show that a filter-output SNR bound E/σ² holds for **complex** signals with h[k] = conj(s[N−1−k]).
3. Compute P_fa for a 48-bit sync with d = 2 (C(48,2) = 1128). Expected false alarms per hour at 9600 bit/s testing every bit.
4. 4800 sym/s, CFO = 100 Hz, 24-symbol coherent correlation: loss in dB? Repeat for 12 symbols.
5. Gardner: signs. Symbols +1, −1; samples y[k−1] = +0.9, y[k−½] = −0.3, y[k] = −1.0. Compute e. Early or late?
6. A Gardner loop with sps = 8, K_d = 2.6, k_p = 0.15: per-symbol decay factor of a timing error? How many symbols to decay to 5 %?
7. Costas BPSK: input phase 30° above NCO phase. e = ? (use e = ½ sin2θ). What if 120°? Which equilibrium does it fall to?
8. Compute α, β for B_nT = 0.01, ζ = 0.707.
9. AGC: μ = 0.02, target 1: time constant? Initial |y| = 0.001 at gain 1: roughly how many samples to reach |y| ≈ 0.5 (use growth factor e^{μ(r−|y|)} ≈ e^{0.02} per sample at the start)?
10. An oscillator error of 2 ppm at 450 MHz: CFO in Hz? Per-symbol rotation at 4800 sym/s (degrees)? Symbol-clock error in symbols per hour if the *clock* has the same 2 ppm.

### Part B — C++ (`code/`)
| # | Function | Concept |
|---|---|---|
| B1 | `matched_filter_output_snr` | Cauchy–Schwarz demo in code |
| B2 | `sync_search`, `sync_cfo_loss`, `sync_false_alarm_prob` | normalised correlation, CFO loss, P_fa |
| B3 | `gardner_error`, `gardner_recover` | TED + PI loop with linear interpolation |
| B4 | `loop_gains`, `costas_bpsk` | PLL gains and Costas loop |
| B5 | `Agc` | log-domain AGC |

### Part C — interview questions
1. Derive why the matched filter maximises SNR.
2. How do you choose a sync-word detection threshold? What goes wrong with CFO?
3. Explain how the Gardner detector works and why it needs 2 samples/symbol.
4. What does a Costas loop do that a plain PLL can't? What ambiguity remains?
5. Why log-domain AGC?

---

## 9. VERIFY list
* P25 frame-sync word 0x5575F5FF77FF and DMR sync 0x755FD7DF75F7 are quoted from memory — **VERIFY** (TIA-102.BAAA; ETSI TS 102 361-1). Not used by the code or tests.
* Costas/PLL gain formulas are standard textbook results; the numeric α, β values (0.05193, 0.001385) were computed by hand.
* Gardner K_d ≈ 2.6 is a hand-derived linearisation for RC β = 0.35; tests use a loose (97 %) decision-agreement criterion rather than a gain value.
* No compiler was available when this pack was written: the C++ solutions and tests were **not built or run** — see the top-level README for first-run advice.
