# Pack 04 — Filters: Convolution, FIR (windowed-sinc), IIR Biquads, Linear Phase, Decimation/Interpolation

| | |
|---|---|
| Time | ~2 h |
| Needs | calculator, pen; C++17 + CMake for `code/` |
| Signals | synthetic only |
| Etherstack link | channel-select filters, DDC decimation chains, matched/RRC filters (Pack 06), DC blockers, group-delay budgeting in sync/timing |

---

## 1. Convolution = a weighted moving average

### 1.1 Concrete first
A 3-point moving average smooths a noisy step. Input x = [0, 0, 6, 0, 0, 6, 6, 6], weights h = [⅓, ⅓, ⅓]:

```
y[n] = ⅓ ( x[n] + x[n-1] + x[n-2] )
x = 0 0 6 0 0 6 6 6
y = 0 0 2 2 2 2 4 6      (n = 0..7, zero initial state)
```
The "weights" **are** the filter; moving the window along and summing the products is convolution.

### 1.2 Derivation
Any input is a sum of scaled, shifted impulses: x[n] = Σₘ x[m] δ[n−m]. A linear time-invariant (LTI) system answers δ[n] with h[n] and δ[n−m] with h[n−m]; by linearity

```
y[n] = Σₘ x[m] h[n−m] = Σₖ h[k] x[n−k]           (convolution)
```
"Flip and slide": reverse h, slide across x, multiply-add. Output length of a full convolution = Lₓ + L_h − 1.

**Frequency response from convolution.** Feed x[n] = e^{jωn} (an *eigenfunction*): y[n] = Σₖ h[k] e^{jω(n−k)} = e^{jωn}·Σₖ h[k]e^{−jωk}. So the output is the input times H(ω) = Σₖ h[k]e^{−jωk}. Therefore convolution in time = multiplication in frequency (the convolution theorem), which is why fast convolution via FFT works (Pack 03).

**Moving-average response (geometric series).** For h[k] = 1/L, k = 0..L−1:
H(ω) = (1/L)(1−e^{−jωL})/(1−e^{−jω}) = e^{−jω(L−1)/2}·sin(Lω/2)/(L sin(ω/2)).
Nulls at ω = 2π m/L, i.e. f = m/L. A 3-point average nulls f = 1/3 and 2/3; it is a *terrible* low-pass (sidelobes of only −10 dB) but an excellent illustration.

---

## 2. FIR design: windowed-sinc — where the sinc comes from

### 2.1 Derivation
Ideal low-pass with cutoff ωc = 2πfc (fc in cycles/sample): H_d(ω) = 1 for |ω| < ωc, else 0. The impulse response is the inverse DTFT:

```
h_d[n] = (1/2π) ∫_{−ωc}^{ωc} e^{jωn} dω = (1/2π) · (e^{jωc n} − e^{−jωc n})/(jn)
       = sin(ωc n)/(π n) = 2fc · sinc(2fc n)        with sinc(x) = sin(πx)/(πx),  h_d[0] = 2fc
```
So **the sinc is simply the inverse transform of a rectangle in frequency.** It is infinite and non-causal. Two fixes:
1. **Truncate** to M+1 taps (n = −M/2..M/2) and **delay** by M/2 samples → causal.
2. Raw truncation = multiplying by a rectangular window = convolving the ideal response with a Dirichlet kernel (Pack 03): **Gibbs ripple** ≈ 9 % (−21 dB) near the edge that does **not** shrink with more taps. A smooth window (Hamming, Blackman) trades a wider transition band for far lower ripple.

### 2.2 Numeric demo: 11 taps, fc = 0.1 cycles/sample, Hamming
Taps before window, h_d[m] = sin(2π·0.1·m)/(π m), centre m = 0 value 0.2:

| m | 0 | ±1 | ±2 | ±3 | ±4 | ±5 |
|---|---|---|---|---|---|---|
| ideal h_d | 0.2000 | 0.18711 | 0.15138 | 0.10091 | 0.04678 | 0 |
| Hamming w (0.54−0.46cos(2πn/10), n = m+5) | 1.0000 | 0.91215 | 0.68215 | 0.39785 | 0.16785 | 0.08 |
| product | 0.2000 | 0.17067 | 0.10327 | 0.04015 | 0.00785 | 0 |

Sum = 0.2 + 2(0.17067+0.10327+0.04015+0.00785) = 0.84388 → divide each tap by it for unity DC gain: centre **0.2370**, ±1: **0.2022**, ±2: 0.1224, ±3: 0.0476, ±4: 0.0093.

### 2.3 Design rules (textbook; Oppenheim & Schafer Table 7.1 — VERIFY if quoting)
| window | peak stopband | transition width (cycles/sample) |
|---|---|---|
| rectangular | −21 dB | ≈ 0.9 / N |
| Hann | −44 dB | ≈ 3.1 / N |
| Hamming | −53 dB | ≈ 3.3 / N |
| Blackman | −74 dB | ≈ 5.5 / N |

So N ≈ 3.3/Δf for a Hamming design; halving the transition width doubles the taps.

### 2.4 Linear phase and group delay (derivation)
If h[n] = h[M−n] (symmetric), pair the terms: for M = 2, h = [a, b, a]:
H(ω) = a + b e^{−jω} + a e^{−j2ω} = e^{−jω}(2a cos ω + b). The bracket is real ⇒ **phase = −ω·(M/2)**, exactly linear. In general

```
φ(ω) = −ωM/2,        group delay  τ_g = −dφ/dω = M/2 samples  (constant)
```
A constant group delay = every frequency delayed equally ⇒ **no waveform distortion**, just a delay. A 63-tap symmetric FIR delays by 31 samples (0.65 ms at 48 kHz). Why it matters for radio: an FIR in front of a sync correlator shifts the timing of the sync peak by τ_g — compensate it. IIR filters have **non-constant** group delay (distortion near the band edge) which is why symbol-shaping filters are always FIR.

---

## 3. IIR biquads

### 3.1 Difference equation and transfer function
```
y[n] = b0 x[n] + b1 x[n−1] + b2 x[n−2] − a1 y[n−1] − a2 y[n−2]
H(z) = (b0 + b1 z⁻¹ + b2 z⁻²) / (1 + a1 z⁻¹ + a2 z⁻²)
```
The feedback gives a long (infinite) memory with only 5 coefficients. Poles are the roots of z² + a1 z + a2: **stable iff |a2| < 1 and |a1| < 1 + a2** (triangle condition; derivation: both roots inside the unit circle). Real filters of order > 2 are cascaded biquads because high-order direct forms are numerically fragile (coefficient sensitivity).

**Transposed Direct Form II** (what you will code; two state variables):
```
y   = b0·x + s1
s1' = b1·x − a1·y + s2
s2' = b2·x − a2·y
```
Check by substitution: y[n] = b0x[n] + s1[n−1], s1[n−1] = b1x[n−1] − a1y[n−1] + s2[n−2], s2[n−2] = b2x[n−2] − a2y[n−2] — the original equation ✓.

### 3.2 Design by cookbook (bilinear transform; Audio-EQ "RBJ" biquad) — numeric demo
Low-pass at f₀ (cycles/sample), quality factor Q (Q = 0.7071 ⇒ maximally flat Butterworth):
ω₀ = 2πf₀, α = sin ω₀/(2Q); b0 = b2 = (1−cos ω₀)/2, b1 = 1−cos ω₀, a0 = 1+α, a1 = −2cos ω₀, a2 = 1−α; then divide all by a0.

Example f₀ = 0.1, Q = 0.70711: ω₀ = 0.62832, cos = 0.80902, sin = 0.58779, α = 0.41563, a0 = 1.41563 →
**b = [0.06746, 0.13491, 0.06746], a1 = −1.14298, a2 = 0.41280.**
Checks: DC gain = (b0+b1+b2)/(1+a1+a2) = 0.26983/0.26982 = 1 ✓; Nyquist (z = −1): numerator b0 − b1 + b2 = 0 ✓ (a zero at Nyquist, a property of the bilinear transform); stability: |a2| = 0.41 < 1, |a1| = 1.143 < 1.413 ✓; pole radius = √a2 = 0.642.

**Why not FIR always?** A Butterworth-like cut with an FIR needs dozens of taps; a biquad does it with 5 coefficients — attractive for DC blockers, audio de-emphasis, notch filters, and for keeping CPU low on embedded DSPs. FIR for linear-phase/data-path filtering; IIR for control-ish low-cost filtering.

**DC blocker (one-pole/one-zero) example:** y[n] = x[n] − x[n−1] + αy[n−1], zero at z = 1 (kills DC), pole at α. −3 dB around ω ≈ 1 − α rad/sample ⇒ α = 0.995 gives ≈ 0.005/(2π)·fs ≈ 38 Hz at 48 kHz.

---

## 4. Decimation, interpolation and polyphase

### 4.1 Decimation by M
Keeping every Mth sample compresses the spectrum by M: anything above 0.5/M (cycles per *input* sample) aliases (Pack 01). So **filter first, then discard** with cutoff ≈ 0.5/M.

Naive cost: N taps at the *input* rate. But we only need every Mth output:

```
y[m] = Σₖ h[k] x[mM − k]
```
Split taps by k = qM + p (p = 0..M−1 the *phase*): E_p[q] = h[qM+p]:

```
y[m] = Σ_p Σ_q E_p[q] · x[(m−q)M − p]
```
Each branch runs at the **output** rate ⇒ N multiply-accumulates per *output* sample instead of per input sample: **M× cheaper**, mathematically identical. That is a polyphase decimator.

### 4.2 Interpolation by L
Insert L−1 zeros between samples (spectrum images appear at multiples of fs_in), then low-pass at 0.5/L (cycles per *output* sample). Zero-stuffing divides the average amplitude by L ⇒ the filter must have **DC gain L**. Polyphase: the zero samples contribute nothing, so

```
y[nL + p] = Σ_q h[qL + p] · x[n − q],        p = 0..L−1
```
Tiny example L = 2, h = [h0,h1,h2,h3]: y[2n] = h0x[n] + h2x[n−1]; y[2n+1] = h1x[n] + h3x[n−1]. Each output needs only N/L multiplies.

### 4.3 DDC chain (SDR)
ADC at several MS/s → mix to baseband → cascade of decimators (CIC or half-band stages cheap at high rate, then a sharp FIR at low rate) → ~4–8 samples/symbol for the modem. Rule: put the cheap, wide filters first where the rate is high and the sharp filter where the rate is low.

---

## 5. Misconceptions
1. **"Convolution is just multiplication."** In time it is slide-multiply-add; only in *frequency* is it plain multiplication.
2. **"More taps = lower ripple" (rectangular window).** Gibbs ripple stays ≈ 9 %; only the window choice reduces it.
3. **"Linear phase means zero delay."** It means *constant* delay of (N−1)/2 samples.
4. **"IIR is always better because it is cheaper."** Non-linear phase, limit cycles in fixed point, stability limits.
5. **"Decimation = take every Mth sample."** Without the anti-alias filter you alias.
6. **"Polyphase changes the result."** Bit-for-bit the same sums reorganised (up to float summation order).
7. **"Cutoff = −3 dB point for windowed-sinc."** The windowed-sinc half-amplitude (−6 dB) point is at fc; the −3 dB point is slightly higher.

## 6. Etherstack / radio context
* **Channel selection:** from MHz-rate I/Q down to a 12.5 kHz (or 25 kHz) channel needs a decimation chain; the channel filter's stopband must reject the adjacent channels (60+ dB in typical receiver specs — **VERIFY** actual spec in the standard/product).
* **Group delay:** a 101-tap FIR at 48 kS/s delays by 50 samples = 1.04 ms — relevant when a stack measures slot timing against a TDMA frame (e.g. DMR 30 ms slots; **VERIFY** timing values in ETSI TS 102 361-1 before quoting).
* **Fixed-point:** FIR accumulators need guard bits (Pack 09); biquads can limit-cycle or overflow in Q15.

---

## 7. Exercises

### Part A — pen & paper
1. Convolve [1, 2, 3] with [1, −1]. What kind of filter is [1, −1]? Sketch its magnitude response.
2. A 5-point moving average runs at 48 kHz. Where is its first spectral null (Hz)? What is its DC gain?
3. A symmetric 63-tap FIR runs at 48 kHz. Group delay in samples and ms?
4. Estimate Hamming taps for a low-pass with transition 0.02 cycles/sample (≈ 960 Hz at 48 kHz). How would you cut the cost with a 4× decimator?
5. Compute the ideal h_d[n] for fc = 0.25 for n = 0, ±1, ±2, ±3.
6. Is the biquad a1 = −1.14298, a2 = 0.4128 stable? Find the pole radius and angle.
7. Decimate 48 kHz → 12 kHz with a 64-tap anti-alias filter. MACs/s for naive vs polyphase?
8. Interpolate by 3 with 12 taps. Write the 3 polyphase branches in terms of h[0..11]. What DC gain is required?
9. Show that h = [a, b, c, b, a] has linear phase and find its group delay.
10. Derive the zero-location of the DC blocker and its approximate −3 dB frequency for α = 0.99 at 8 kHz.

### Part B — C++ (`code/`)
| # | Function | Concept |
|---|---|---|
| B1 | `convolve` | definition |
| B2 | `hamming` | symmetric window (denominator N−1) |
| B3 | `windowed_sinc_lowpass` | sinc × window, centred, DC-normalised |
| B4 | `biquad_lowpass` + `Biquad` | cookbook coefficients and transposed DF-II |
| B5 | `decimate_polyphase` | M× cheaper, same output |
| B6 | `interpolate_polyphase` | zero-stuff + filter without the zeros |

### Part C — interview questions
1. Why does an FIR filter have linear phase when symmetric? What does that buy a radio?
2. Where does the sinc in a windowed-sinc filter come from?
3. Why must you filter before decimating, and how do you make it cheaper?
4. FIR or IIR for a DC blocker? For a 12.5 kHz channel filter? Why?
5. What is the group delay of a 101-tap FIR at 96 kS/s and how would that affect your sync-word timing?

## 8. VERIFY list
* Window table (§2.3) is textbook (Oppenheim & Schafer, *Discrete-Time Signal Processing*, window design table) — check before quoting exact dB figures.
* Channel-filter rejection figures and DMR 30 ms slot timing are context only and **must be checked** against the standard/product spec.
* Hand-computed numbers (taps, biquad coefficients) were checked against the identities in the text but **not executed**; the C++ tests use 1e-3/1e-4 tolerances accordingly.
