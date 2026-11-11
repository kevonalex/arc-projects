# Pack 05 — Modulation: AM, FM, FSK/4FSK (C4FM), PSK/QPSK/π/4-DQPSK, QAM, OFDM

| | |
|---|---|
| Time | ~2.5 h |
| Needs | pen, calculator, C++17 + CMake for `code/` |
| Signals | synthetic only — all bit patterns come from counters/LFSRs in tests |
| Etherstack link | land-mobile-radio air interfaces: 4FSK/C4FM (P25 Phase 1), 4FSK (DMR), π/4-DQPSK (TETRA) |

> Rule for this pack: **a number marked VERIFY is quoted from memory of the standards and must be checked against the document before you rely on it in an interview.** The VERIFY list is at the end.

---

## 0. One model for everything: complex baseband

Every scheme here is a choice of how information changes the complex envelope z(t) = A(t) e^{jφ(t)} (Pack 02):

| Scheme | information in | envelope | concrete picture |
|---|---|---|---|
| AM | A(t) | varies | radius changes |
| FM / FSK | dφ/dt | constant | phase spins at a data-dependent rate |
| PSK | φ (at symbol times) | constant (ideal) | points on a circle |
| QAM | A and φ | varies | grid |
| OFDM | many QAM symbols on orthogonal subcarriers | Gaussian-like | sum of tones |

---

## 1. AM and FM

**AM.** s(t) = (1 + m·x(t)) cos ωc t with |x| ≤ 1, depth m ≤ 1 (m > 1 → over-modulation, envelope detector distorts). Baseband z = 1 + m·x: demodulate with |z|. Bandwidth 2 f_m,max.

**FM.** Instantaneous frequency f_i(t) = fc + kf·x(t) (kf = Hz of deviation per unit input). Phase is the *integral* of frequency (derivation: φ(t) = 2π∫f_i dt, because frequency is the rate of change of phase) ⇒ z[n] = e^{jφ[n]}, φ[n+1] = φ[n] + 2π kf x[n]/fs.
Demodulation = differentiate the phase: x[n] = arg(z[n+1]z*[n])·fs/(2π kf) (Pack 02).

**Carson's rule (heuristic, derived from Bessel-function sidebands):** significant sidebands extend ≈ (β+1) f_m on each side, β = Δf/f_m ⇒ BW ≈ 2(Δf + f_m).
*Example:* narrowband land-mobile FM, deviation Δf = 2.5 kHz, audio up to 3 kHz ⇒ BW ≈ 2(2.5 + 3) = 11 kHz which just fits a 12.5 kHz channel. (Typical LMR figures; **VERIFY** for the specific service.)

---

## 2. FSK, 4FSK and C4FM

### 2.1 Binary FSK: how far apart must the tones be? (derivation)
Two tones f₁, f₂ over a symbol of length T are orthogonal (don't interfere in a correlator) when

∫₀ᵀ cos(2πf₁t)·cos(2πf₂t) dt = ½∫₀ᵀ cos(2π(f₂−f₁)t) dt + ½∫₀ᵀ cos(2π(f₁+f₂)t) dt = 0.

The second term is ≈ 0 (fast oscillation). The first is sin(2πΔf T)/(4πΔf) = 0 when **Δf·T = k/2** ⇒ minimum spacing **Δf = 1/(2T)** for coherent detection (this is MSK; modulation index h = Δf·T = 0.5) and **1/T** for non-coherent detection (h = 1).

### 2.2 4FSK: four tones, two bits per symbol
Each symbol carries 2 bits (a *dibit*) chosen from four frequency offsets. Take a nominal deviation unit D: levels ±1D, ±3D.

**P25 Phase 1 C4FM** (as I recall from TIA-102.BAAA; **VERIFY every number**):

| quantity | value |
|---|---|
| symbol rate | 4800 symbols/s |
| bits per symbol | 2 |
| bit rate | 4800 × 2 = **9600 bit/s** |
| channel | 12.5 kHz |
| symbol → deviation | dibit 01 → +1800 Hz; 00 → +600 Hz; 10 → −600 Hz; 11 → −1800 Hz |
| deviation unit D | 600 Hz (levels ±1D = ±600, ±3D = ±1800) |
| spacing between adjacent levels | 1200 Hz ⇒ modulation index h = 1200/4800 = 0.25 |

**DMR 4FSK** (ETSI TS 102 361-1; **VERIFY**): same 4800 symbols/s and 9600 bit/s, but D = 648 Hz ⇒ ±648 Hz and ±1944 Hz.

*Self-consistency checks you can do without the standard:* 4800 × 2 = 9600 ✓; ±3D / ±1D ratio = 3 ✓; the outer deviation (1.8–1.94 kHz) plus the occupied bandwidth of the shaped symbols (≈ Rs/2 = 2.4 kHz either side) ≈ ±4.2 kHz → **8.4 kHz by a Carson-style estimate, inside 12.5 kHz** ✓.

### 2.3 Modulate and demodulate (what you will code)
* **Modulator:** hold the frequency for `sps` samples; accumulate phase φ += 2π f/fs so the phase is **continuous** (no jumps ⇒ compact spectrum). Constant envelope ⇒ can use an efficient, non-linear (class-C-ish) PA — a big reason LMR chose FM-type modulation.
* **Demodulator:** discriminator arg(z[n+1]z*[n]), average over each symbol, slice at 0 and ±2D. The decision thresholds are halfway between levels (±1D vs ±3D → 2D). A static CFO adds a DC offset to every level ⇒ estimate and subtract the mean (Pack 10).
* **Real systems** pre-filter the frequency pulses (raised-cosine/RRC shaped deviation, Pack 06) so that the spectrum fits the mask; our rectangular-pulse version is the simplest correct core.

---

## 3. PSK family

### 3.1 BPSK → QPSK
BPSK: s = ±1 (phase 0/π). QPSK: two BPSK streams on cos and sin (orthogonal) ⇒ 2 bits/symbol in the same bandwidth. Gray mapping: (b1 b0) → ((−1)^{b1} + j(−1)^{b0})/√2: neighbouring points differ by **one bit**, so the most likely error (adjacent point) causes 1 bit error, not 2. BER of QPSK per bit = BER of BPSK = Q(√(2Eb/N0)) (Pack 08).

### 3.2 Why π/4-DQPSK (TETRA; **VERIFY**: 18 ksym/s, 36 kbit/s, 25 kHz channel, RRC α = 0.35)
**Problem with QPSK:** a transition between opposite points (e.g. 00 → 11, a 180° phase change) goes straight through the origin ⇒ the envelope momentarily **zero** ⇒ huge envelope swings ⇒ needs a very linear (inefficient) power amplifier.

**Derivation of the fix.** Two unit-magnitude points separated by angle θ: a straight-line transition between them has its midpoint at distance cos(θ/2) from the origin.

| θ | midpoint amplitude |
|---|---|
| 180° (QPSK worst case) | cos 90° = **0** |
| 135° (π/4-DQPSK worst case) | cos 67.5° = **0.383** |
| 45° | cos 22.5° = 0.924 |

π/4-DQPSK uses **two interleaved QPSK constellations** (0, π/2, π, 3π/2 and π/4, 3π/4, …) and *alternates* between them: every symbol is a phase **increment** of ±π/4 or ±3π/4 relative to the previous symbol — never 0 or π. Hence no zero-crossings, lower envelope variation, and being *differential* it needs no absolute phase reference (robust in fading, simple non-coherent detection at the cost of ≈ 2–3 dB vs coherent detection — **VERIFY** figure).

TETRA dibit → phase increment (**VERIFY** against ETSI EN 300 392-2): 00 → +π/4; 01 → +3π/4; 11 → −3π/4; 10 → −π/4.
Encode: φ[n] = φ[n−1] + Δφ(dibit); transmit e^{jφ[n]}. Decode: Δφ = arg(s[n]·s*[n−1]) → nearest of the four. A constant phase offset θ cancels: arg(e^{jθ}s[n]·e^{−jθ}s*[n−1]).

*Numerical demo (π/4 units):* dibits 00, 01, 11, 10 ⇒ increments +1, +3, −3, −1 ⇒ phases 1, 4, 1, 0 (units of π/4) = 45°, 180°, 45°, 0° — alternately odd and even multiples of 45° ✓.

---

## 4. QAM

16-QAM: each of I and Q ∈ {−3, −1, +1, +3}/√10 (the √10 normalises average symbol energy to 1: E[I²+Q²] = 2·(1+9+1+9)/4 = 10 before scaling). Gray code per axis: 00 → −3, 01 → −1, 11 → +1, 10 → +3 (adjacent levels differ by one bit). 4 bits/symbol; minimum distance 2/√10 = 0.632 vs QPSK's √2 = 1.414 at the same average symbol energy Es = 1. Compare d²_min/Eb (the quantity that sets BER at high SNR): QPSK 2/0.5 = 4; 16-QAM 0.4/0.25 = 1.6; ratio 2.5 ⇒ 10log₁₀2.5 = **3.98 dB** more Eb/N0 for the same BER (asymptotically). Peak-to-average (corner): 18/10 = 1.8 → 2.55 dB.

---

## 5. OFDM: orthogonal subcarriers and the cyclic prefix

### 5.1 Orthogonality (derivation)
Subcarriers e^{j2πkt/T}, k = 0..N−1, spaced Δf = 1/T apart:
∫₀ᵀ e^{j2πkt/T} e^{−j2πlt/T} dt = T for k = l, else (e^{j2π(k−l)} − 1)/(j2π(k−l)/T) = 0. The spectra overlap but each is exactly zero at the others' centres: dense packing without interference.
**Generation:** x[n] = (1/N) Σₖ X[k] e^{j2πkn/N} — an **IDFT** (use an IFFT, Pack 03). Receiver: DFT.

### 5.2 Multipath and the cyclic prefix (derivation)
A channel with impulse response h[0..L−1] gives y = x * h (linear convolution). The DFT diagonalises **circular** convolution only: DFT{x ⊛ h} = X·H. Trick: copy the last C ≥ L−1 samples of the symbol to the front. Then, after discarding the first C received samples, linear convolution equals circular convolution (the CP "wraps" the tail).

*Tiny numeric demo.* x = [1,2,3,4], h = [1, 0.5], C = 1: tx = [4,1,2,3,4]. rx[n] = tx[n] + 0.5·tx[n−1] = [4, 3, 2.5, 4, 5.5]; drop the first (CP) sample → [3, 2.5, 4, 5.5]. Circular convolution of x with h: y[0] = 1 + 0.5·4 = 3; y[1] = 2 + 0.5·1 = 2.5; y[2] = 3 + 0.5·2 = 4; y[3] = 4 + 0.5·3 = 5.5 ✓ identical.

**Consequence: one complex multiply per subcarrier equalises the channel:** Y[k] = H[k] X[k] + noise ⇒ X̂[k] = Y[k]/H[k]. A single-carrier system would need a long time-domain equaliser. Cost: CP overhead C/(N+C) of the capacity and a loss of SNR (the CP carries no new energy); CP length must exceed the channel delay spread.

---

## 6. Misconceptions
1. **"4FSK has 4 bits per symbol."** 4 levels = 2 bits (log₂4). Bit rate = Rs × 2.
2. **"FSK needs a phase-coherent receiver."** The discriminator is non-coherent; this is a feature in fading.
3. **"A frequency offset only matters for PSK."** FSK sees a DC shift of every level; slicing thresholds must track it.
4. **"π/4-DQPSK has constant envelope."** No — the envelope still varies (min ≈ 0.38 before filtering; pulse shaping adds more), just far less than QPSK.
5. **"OFDM subcarriers don't overlap."** Their spectra overlap; they are *orthogonal* at the sampling instants.
6. **"The cyclic prefix is just a guard interval."** A guard interval of zeros would also stop ISI but would not turn the convolution circular, so no single-tap equaliser.
7. **"Gray coding improves the distance."** It reduces *bit* errors per symbol error; the symbol-error rate is unchanged.

## 7. Etherstack / radio context
* The 4FSK demodulator you write here is the front half of a P25 Phase 1 / DMR receiver: discriminator → symbol timing (Pack 07) → slicer → dibits → frame sync (48-bit sync search, Pack 07/10).
* π/4-DQPSK is what a TETRA modem does; the cheat-sheet for "why" is §3.2.
* OFDM: not used by the legacy narrowband LMR standards above, but central to broadband PMR (LTE/5G-based) — interviewers may ask the CP argument regardless.

---

## 8. Exercises

### Part A — pen & paper
1. 4FSK at 4800 sym/s: bit rate? Time per symbol? At fs = 48 kS/s how many samples per symbol? Phase advance per sample for the +3D (1800 Hz) level?
2. Map the dibit stream 01 00 10 11 to deviations (P25 convention) and sketch the instantaneous frequency vs time.
3. Modulation index for adjacent-level spacing 1200 Hz at 4800 sym/s? Compare with MSK's h = 0.5.
4. FM with Δf = 2.5 kHz and fm = 3 kHz: Carson bandwidth, β?
5. π/4-DQPSK: encode dibits 00, 01, 11, 10, 00 starting from phase 0. List phases (units of π/4) and the complex points.
6. For π/4-DQPSK, the receiver has an unknown constant phase offset of 0.4 rad. Show the decoded dibits are unaffected (except the first, relative to the reference).
7. 16-QAM Gray mapping: write all 16 points' normalised coordinates for I-bits 00,01,11,10; verify average energy 1.
8. OFDM: N = 64 subcarriers, 25 kHz-wide channel sampled at fs = 25 kHz. Subcarrier spacing? Useful symbol length? With CP = 16 samples what is the overhead and the maximum tolerable delay spread?
9. Prove that dropping the CP and using a zero guard interval would *not* let you equalise with Y/H.
10. (Challenge) Show that for two 4FSK levels differing by Δf = 1200 Hz the correlator outputs over one 208.33 µs symbol are *not* orthogonal (h = 0.25) — yet the discriminator receiver still works. Why?

### Part B — C++ (`code/`)
| # | Function | Concept |
|---|---|---|
| B1 | `fsk4_level`, `fsk4_modulate` | dibit → ±1/±3, phase-continuous FM |
| B2 | `fsk4_demodulate` | discriminator + average + slice |
| B3 | `qpsk_map`, `qpsk_demap` | Gray QPSK |
| B4 | `pi4dqpsk_modulate`, `pi4dqpsk_demodulate` | differential phase |
| B5 | `qam16_map` | Gray 16-QAM, normalisation |
| B6 | `ofdm_modulate`, `ofdm_demodulate`, `ofdm_equalise` | IDFT + CP, single-tap EQ |
| B7 | `fm_modulate`, `fm_demodulate`, `carson_bandwidth_hz` | FM |

`cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure` (stubs fail; `-DUSE_SOLUTIONS=ON` for reference).

### Part C — interview questions
1. How many bits per symbol in P25 C4FM and what is the bit rate? What are the deviations?
2. Why π/4-DQPSK instead of QPSK for TETRA?
3. Explain the cyclic prefix and why OFDM needs only one tap per subcarrier.
4. Why is constant-envelope modulation attractive for a handheld transmitter?
5. A 4FSK receiver sees every symbol shifted by +200 Hz. What happens and how do you fix it?

---

## 9. VERIFY list (check against the standards)
| item | quoted as | source to check |
|---|---|---|
| P25 Phase 1 C4FM symbol rate | 4800 sym/s | TIA-102.BAAA |
| bits/symbol, bit rate | 2, 9600 bit/s | TIA-102.BAAA |
| C4FM deviations | ±600 Hz, ±1800 Hz | TIA-102.BAAA |
| P25 dibit→symbol map | 01→+3, 00→+1, 10→−1, 11→−3 | TIA-102.BAAA |
| DMR 4FSK | 4800 sym/s, ±648 / ±1944 Hz | ETSI TS 102 361-1 |
| DMR/P25 channel | 12.5 kHz | respective standards |
| TETRA | 18 ksym/s, 36 kbit/s, 25 kHz, π/4-DQPSK, RRC α = 0.35 | ETSI EN 300 392-2 |
| TETRA dibit→Δφ | 00 +π/4, 01 +3π/4, 11 −3π/4, 10 −π/4 | ETSI EN 300 392-2 |
| DQPSK non-coherent penalty | ≈ 2–3 dB | textbook (Proakis) |
| NBFM deviation / BW | ±2.5 kHz, 11 kHz Carson | regulator / service rules |
| 16-QAM extra Eb/N0 vs QPSK | ≈ 4 dB | derive in Pack 08 |
