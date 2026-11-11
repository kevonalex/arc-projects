# Pack 02 — I/Q and Complex Baseband, from the ground up

| | |
|---|---|
| Time | ~90 min |
| Needs | pen, calculator, C++17 + CMake (see `code/`) |
| Signals | synthetic only |
| Etherstack link | every modem stage downstream of the ADC works on I/Q: 4FSK discriminators, CFO correction, symbol timing, Costas loops |

---

## 0. The problem I/Q solves (concrete first)

A local oscillator (LO) sits at **100.000 MHz**. Two real-world signals arrive:

* signal A at **100.010 MHz** (10 kHz *above* the LO)
* signal B at **99.990 MHz** (10 kHz *below* the LO)

A **real** mixer multiplies the incoming voltage by cos(2π·100 MHz·t). Trig identity:

```
cos(a)·cos(b) = ½ [ cos(a−b) + cos(a+b) ]
```

For A: ½cos(2π·10 kHz·t) + (sum term at ~200 MHz, filtered away).
For B: ½cos(2π·(−10 kHz)·t) = ½cos(2π·10 kHz·t) — **identical**, because cosine is even.

> A real signal cannot tell +f from −f. The negative-frequency half of a real signal's spectrum is just the mirror image of the positive half; it carries no extra information, but it also means "above carrier" and "below carrier" collide.

Why that is fatal for radios: **FM/FSK information *is* the sign of the frequency offset.** In 4FSK a symbol of +600 Hz and one of −600 Hz must be distinguishable (see Pack 05). A single real channel cannot do it. We need two channels — **I (in-phase)** and **Q (quadrature)** — i.e. a complex number per sample.

### Numerical demo: samples of +f and −f (fs = 8, f = 1 cycle per 8 samples)

| n | cos(+2πn/8) | cos(−2πn/8) | sin(+2πn/8) | sin(−2πn/8) |
|---|---|---|---|---|
| 0 | 1 | 1 | 0 | 0 |
| 1 | 0.707 | 0.707 | 0.707 | −0.707 |
| 2 | 0 | 0 | 1 | −1 |
| 3 | −0.707 | −0.707 | 0.707 | −0.707 |
| 4 | −1 | −1 | 0 | 0 |

The cosine columns are equal; the sine columns have opposite signs. **Having both cos and sin recovers the sign of f.** That pair is exactly (I, Q).

---

## 1. Euler's formula and complex exponentials

### 1.1 Derivation (Taylor series)
e^x = Σ xᵏ/k!. Put x = jθ and use j² = −1, j³ = −j, j⁴ = 1:

```
e^{jθ} = (1 − θ²/2! + θ⁴/4! − …)  +  j (θ − θ³/3! + θ⁵/5! − …)
       =  cos θ                    +  j sin θ
```

So z[n] = e^{j2πfn/fs} = cos(2πfn/fs) + j sin(2πfn/fs) is **(I, Q) = (cos, sin)**: a point moving around the unit circle.
* f > 0: counter-clockwise. f < 0: clockwise. The direction *is* the sign of the frequency.
* Phase advance per sample: Δφ = 2πf/fs. Frequency estimate: f = fs·Δφ/(2π).

### 1.2 Multiplication = rotation (derivation)
Multiply z = x + jy by e^{jφ} = cos φ + j sin φ:

```
(x + jy)(cos φ + j sin φ) = (x cos φ − y sin φ) + j (x sin φ + y cos φ)
```

which is the 2×2 rotation matrix [[cos φ, −sin φ],[sin φ, cos φ]] acting on (x, y). Length preserved, angle increased by φ. **Everything in a modem — CFO, phase offset, mixing — is rotation.**

### 1.3 A real cosine is two counter-rotating phasors
```
cos θ = ½ e^{jθ} + ½ e^{−jθ}
```
Both +f and −f present, each at half amplitude. That is the ambiguity of §0 written in one line. Taking only the e^{+jθ} half (complex baseband) discards the redundant mirror.

---

## 2. Mixing as multiplication by a complex exponential

**Shift theorem (derivation).** If z[n] = e^{j2πf₁n}, then z[n]·e^{−j2πf₀n} = e^{j2π(f₁−f₀)n}. Frequencies add under multiplication of exponentials, so multiplying *any* signal by e^{−j2πf₀n} translates its whole spectrum **down by f₀** — one copy, no mirror, no image.

**Real mixer vs complex mixer.** Real: cos×cos gives *both* f₁−f₀ and f₁+f₀, and a signal at f₀−Δ lands on the same output frequency as one at f₀+Δ (the **image**). Complex: single translation. Quadrature down-conversion builds the complex mixer from two real mixers:

```
I(t) = LPF{  2 x(t) cos(2πf_c t) }
Q(t) = LPF{ −2 x(t) sin(2πf_c t) }          z = I + jQ = LPF{ 2 x(t) e^{−j2πf_c t} }
```

**Derivation with x = A cos(2π(f_c+Δ)t + θ):**
* 2x cos(ω_c t) = A cos(Δt+θ) + A cos((2ω_c+Δ)t+θ) → after LPF: **I = A cos(Δt+θ)**
* −2x sin(ω_c t): use 2cos(a)sin(b) = sin(a+b) − sin(a−b) → −A sin((2ω_c+Δ)t+θ) + A sin(Δt+θ) → after LPF: **Q = A sin(Δt+θ)**
* z = A e^{j(Δt+θ)}. For Δ < 0 (signal below LO) it rotates clockwise — **A and B of §0 are now distinguishable.**

The 2× gain is because cos×cos halves the amplitude (the ½ in the identity). The LPF must remove the 2f_c term (and also anything near −f_c-image positions): its quality sets how clean the result is — Pack 04.

### Complex baseband sample rate = bandwidth
A real signal of bandwidth B needs fs ≥ 2B (Pack 01). Its complex baseband envelope occupies [−B/2, +B/2] and needs complex fs ≥ B. Same number of real numbers per second (2 per complex sample × B = 2B).

---

## 3. What you can read off an I/Q stream

| Quantity | Formula | Radio use |
|---|---|---|
| Envelope / magnitude | \|z\| = √(I²+Q²) | AM demod, AGC, RSSI |
| Phase | atan2(Q, I) | PSK demod, CFO |
| Instantaneous frequency | arg(z[n+1]·conj(z[n]))·fs/2π | FM / 4FSK discriminator |
| Power | I² + Q² | squelch, level |

**Why arg(z[n+1]·z*[n]) and not atan2 differences?** The product removes the absolute phase and wraps automatically into (−π, π]; differencing two atan2 outputs requires manual unwrapping. Derivation: z[n+1]z*[n] = A²e^{j(φ[n+1]−φ[n])}.

---

## 4. Misconceptions

1. **"I and Q are two separate signals."** They are the real and imaginary parts of *one* complex signal; neither alone has the information.
2. **"Negative frequency is not real."** The *physical* signal is real; negative frequency is a representation. But in a complex baseband stream it is real information: "below the carrier".
3. **"Q is the signal shifted by 90° so I could just delay I."** Delay by T/4 works for one frequency only; the Hilbert transform (or the quadrature mixer) gives −90° at *all* frequencies.
4. **"I/Q sample rate must be 2× bandwidth."** For complex data fs ≥ B suffices.
5. **"The mixer's 2f_c output is harmless."** Without a good LPF it aliases back after decimation.
6. **"IQ imbalance is just a gain error."** Gain *and* phase errors create an image (mirror) copy of the signal at −f. Image rejection for gain error g (ratio) and phase error φ: IRR ≈ 4/(ε² + φ²) (ε = g−1, rad), e.g. ε = 0.1, φ = 5° → ≈ −23.6 dB image. (Derivation: Pack 09.)

---

## 5. Etherstack / radio context

* The SDR front end delivers I/Q at some sample rate. A P25-style 4FSK/C4FM demodulator is an FM discriminator on I/Q: arg(z[n+1]z*[n]) → average per symbol → slice to ±1/±3 (Pack 05). **VERIFY** the standard's symbol rate and deviations (Pack 05 VERIFY list).
* **Carrier frequency offset (CFO)** = a residual LO difference Δf: the I/Q constellation spins at Δf. At 4800 symbols/s a 300 Hz offset rotates the signal **22.5° per symbol** — enough to wreck PSK, but a mere DC shift of the discriminator output for FSK (so FSK tolerates CFO better, you remove it by subtracting the mean).
* **Zero-IF (direct conversion)** receivers put the carrier at DC, so DC offset and 1/f noise land in-band (Pack 09); low-IF receivers avoid that at the cost of an image problem — which brings back image rejection.

---

## 6. Exercises

### Part A — pen & paper
1. Compute (3 + 4j)·e^{jπ/2} directly and via the rotation matrix. What is \|·\|?
2. A tone at 100.010 MHz and one at 99.990 MHz hit a quadrature receiver with LO 100 MHz. Write z(t) for each. What does I alone show? What does I + jQ show?
3. z[n] = e^{j2π·0.05·n}, fs = 48 kHz. Frequency in Hz? Phase per sample in radians and degrees? Repeat for e^{−j2π·0.1·n}.
4. LO error: receiver LO is 300 Hz **above** the true carrier. What frequency does the baseband tone have? Rotation per symbol at 4800 sym/s (in degrees)? After 10 symbols?
5. Superhet: RF 150.000 MHz, IF 10.7 MHz, low-side LO. LO frequency? Image frequency? Why does a real mixer need an RF filter, while a complex (quadrature) mixer in an ideal world does not?
6. AM: x(t) = (1 + 0.5 cos(2π·1 kHz·t)) cos(2π f_c t). Write I, Q after quadrature down-conversion (fc exact). What is |z| and arg z?
7. Show cos(θ) = (e^{jθ}+e^{−jθ})/2 and use it to find the complex baseband of cos(2π(f_c + Δ)t) after multiplication by 2e^{−j2πf_c t} (before LPF). Which terms does the LPF remove, which stay?
8. x[n] = cos(2π·0.26·n) is processed with f_c = 0.25 cycles/sample and a 4-tap boxcar LPF. What is the wanted baseband frequency? Where is the unwanted term before the LPF, and roughly how much does a 4-tap boxcar attenuate it? (Boxcar response |sin(πLf)/(L sin πf)|.)

### Part B — C++ (see `code/`)
| # | Function | Concept |
|---|---|---|
| B1 | `mix` | complex exponential multiplication (shift theorem) |
| B2 | `iq_downconvert` | quadrature mixer + LPF on a real input |
| B3 | `estimate_freq_cps` | arg(Σ z[n+1]z*[n]) — lag-1 autocorrelation frequency estimator |
| B4 | `instantaneous_freq` | FM discriminator |
| B5 | `image_rejection_db` | measure +f vs −f energy |

`cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure` — stubs fail; add `-DUSE_SOLUTIONS=ON` for the reference.

### Part C — interview questions
1. Why do radios use I/Q instead of a single real channel?
2. Explain how a quadrature mixer rejects the image that a real mixer does not.
3. What does a frequency offset look like in I/Q? How would you estimate it from the data alone?
4. A colleague says "we sample the I and Q at 1 MS/s so the bandwidth is 500 kHz". Correct them.
5. Draw (describe) what happens to a constellation with a 5° phase error and 10 % gain error in Q.

---

## 7. VERIFY list
* 4FSK/C4FM symbol rate and deviations quoted in §5 are cross-referenced to Pack 05; **VERIFY against TIA-102.BAAA / ETS(I) documents before quoting in an interview**.
* All trig identities and the quadrature-mixer derivation above are textbook-standard; the numeric rotation example (22.5°/symbol) is computed: 300 Hz / 4800 sym/s = 0.0625 cycles = 22.5°.
* The test thresholds in `code/tests` are derived by hand (boxcar rejection ≈ 3 %), **not run** on the author's machine (no compiler available) — expect to adjust if a test fails for numerical reasons.
