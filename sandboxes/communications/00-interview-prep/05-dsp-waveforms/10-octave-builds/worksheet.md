# Pack 10 — Model in Octave/MATLAB, Export Vectors, Match in C++

| | |
|---|---|
| Time | ~4 h (seven small builds) |
| Needs | C++17 + CMake; **Octave** for `octave/gen_vectors.m` (MATLAB also works if you move the local functions to the end of the file and delete the `1;` line) |
| Signals | synthetic only (deterministic generator in the script) |
| Etherstack link | Modem/DSP engineering practice: a readable *reference model* (Matlab/Octave) is written first, test vectors are exported from it, the production C/C++ (and later FPGA/fixed-point) is verified against those vectors in CI. Interview question: "How do you know your C++ implements the algorithm correctly?" |

> **Honesty note.** The authoring environment had no Octave, compiler or CMake, so neither `gen_vectors.m` nor the C++ was run. Analytic expectations and tolerances were derived by hand. The Octave-vector tests **skip** (not fail) until you generate the vectors. Fix any Octave-version issue you meet — the script is short and commented.

---

## 1. The workflow

```
algorithm idea → Octave model (clear, vectorised, plots) → vectors/*.txt → C++ (stub → your code)
        ↑ derive the model from the maths, NOT from the C++              ↓
        └──────────── compare: bit-exact (integers) / tolerance (floats) / statistics (noise)
```

Layout of this pack:
```
10-octave-builds/
  octave/gen_vectors.m   model + generator (deterministic, core Octave only)
  vectors/               generated *.txt (one number per line, %.17g)
  code/include/octave_builds.hpp   API you implement
  code/include/vector_io.hpp       provided loader
  code/stubs|solutions|tests
```

Commands:
```
octave --no-gui octave/gen_vectors.m                     # writes vectors/
cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure   # stubs FAIL by design
cmake -S code -B build -DUSE_SOLUTIONS=ON                 # reference solutions
```

---

## 2. What does "match" mean? — three kinds of check

### 2.1 Floating-point is not associative (numerical demo)
In IEEE-754 doubles:
* (0.1 + 0.2) + 0.3 = 0.6000000000000001
* 0.1 + (0.2 + 0.3) = 0.6000000000000000

Same mathematics, different last bit — because each addition rounds. Consequence: *two correct implementations of the same float algorithm differ in the last few bits*, so **a float test needs a tolerance**, chosen with care (below).

### 2.2 Choosing the tolerance (derivation)
One floating-point operation has relative error ≤ ε = 2⁻⁵³ ≈ 1.1×10⁻¹⁶. Summing N terms in any order has worst-case error ≲ (N−1)·ε·Σ|terms| (and typically ≈ √N·ε·rms).

| check | size of the computation | rounding error expected | tolerance used | why |
|---|---|---|---|---|
| FIR, 21 taps, \|x\|<1, Σ\|h\|≈1.2 | 21 products | ≲ 20·1.1e-16·1.2 ≈ 3e-15 | **1e-12** | 300× margin; any real bug is ≫ 1e-6 |
| FFT, N = 64 | log₂64 = 6 stages, \|X\|rms ≈ 3.3 | ≈ ε·6·3.3 ≈ 2e-15 | **1e-12** | float (32-bit) twiddles would give ≈ 1e-7: caught |
| 4FSK waveform (400 samples, accumulated phase) | 400 additions | ≈ 400·1.1e-16 ≈ 4e-14 | **1e-9** | loose; tighten as exercise |
| CFO estimate | 127 products + atan2 | ≈ 1e-15 | **1e-12** | |

Rule of thumb: *tolerance = (predicted rounding error) × (100…1000) — big enough to never flake, small enough that every plausible bug (wrong sign, off-by-one, wrong window) exceeds it by orders of magnitude.* A tolerance of 1e-3 "because it passes" hides bugs.

**FFT specifically**: your radix-2 and Octave's FFTW use different butterfly orders and twiddle tables, so their results are *not* bit-identical and never will be. Do not try to make them so; use the tolerance.

### 2.3 Integer paths: bit-exact
Hamming distances, symbol levels and Q15 FIR outputs are integers: **any** difference is a bug (a different rounding rule, a missing saturation, a wrong shift). The Octave reference computes the Q15 FIR with doubles holding integers (exact below 2⁵³; division by 2¹⁵ is exact in binary), so it is a legitimate bit-exact reference.

### 2.4 Random noise: statistics, never equality
`rand`/`randn` (Octave, Mersenne twister variant + Ziggurat) and `std::normal_distribution` (implementation-defined algorithm, differs between libstdc++/libc++/MSVC) can never produce the same noise stream. (Also: same seed ≠ same stream across languages.) Compare *statistics* with theory instead:

BER estimate from n bits with true rate p has relative standard deviation √((1−p)/(np)) ≈ 1/√(errors counted).

| errors counted | relative σ | typical use |
|---|---|---|
| 100 | 10 % | quick check |
| 400 | 5 % (95 % interval ±10 %) | the 10 % tolerance in the tests |
| 10 000 | 1 % | publication-quality curve |

At 6 dB, p = 2.4×10⁻³ ⇒ 2×10⁶ bits give 4 800 errors (1.4 %); at 10⁻⁵ you would need ≈ 4×10⁷ bits for 400 errors.

---

## 3. Octave ↔ C++ traps (checklist)

| topic | Octave/MATLAB | C++ |
|---|---|---|
| index base | 1 | 0 (the sync test embeds at Octave position 101 = lag 100) |
| `x(1:n)` | inclusive | `[0, n)` |
| `mod(-1,4)` | **3** (sign of divisor) | `-1 % 4 == -1` (sign of dividend) |
| `int16(40000)` | **saturates** to 32767 | wraps (UB-free but wrong) |
| `round(2.5)` | 3 (away from zero) = `std::lround` | `std::rint` = ties-to-even |
| `fft` | unscaled, `ifft` divides by N | whatever you write — define it |
| `hamming(N)`, `hann(N)` | symmetric / (hann can be periodic) | you choose: window symmetric for FIR design, periodic for spectral analysis |
| `sinc(x)` | sin(πx)/(πx) (**normalised**) | `std::sinc` does not exist in C++17; write it |
| `filter(h,1,x)` | direct form II transposed, zero state, same length | direct form: same maths, different rounding |
| `rand` seeds | Mersenne twister | `std::mt19937_64`; different streams |
| `angle` | atan2 in (−π, π] | `std::arg` same |
| printing | `%.17g` round-trips a double | `>>` reads it back exactly |

Why the generator avoids `rand`: it uses a Park–Miller LCG, x ← 16807·x mod (2³¹−1). Products < 2⁴⁶ < 2⁵³, so the arithmetic is exact in doubles and the stream is the same on every machine.

---

## 4. The seven builds

For each build: **spec → derivation/demo → Octave model → C++ points → checks**.

### Build 1 — FFT (vs Octave `fft`)
* Spec: `std::vector<cplx> fft(x)`, radix-2 DIT, N a power of two, unscaled, sign e^{−j2πkn/N}.
* Derivation (Pack 03): split X[k] = E[k] + W_N^k O[k], X[k+N/2] = E[k] − W_N^k O[k]; bit-reversal puts E/O in place; log₂N stages of N/2 butterflies.
* Demo: x = [1,2,3,4,0,0,0,0] ⇒ X = [10, −0.4142−7.2426j, −2+2j, 2.4142−1.2426j, −2, conj…]; Parseval: Σ|X|² = N Σ|x|² = 8·30 = 240.
* Octave: `X = fft(x)` for the 64-point vector from the LCG.
* C++ points: compute twiddles per stage with `std::polar` directly (not by repeated multiplication — error grows); throw on non-powers of two.
* Checks: impulse → all ones; tone in bin 5 → X[5] = N; match naive DFT (1e-12); match Octave (1e-12).

### Build 2 — `FirFilter` (streaming)
* Spec: class with `process(x)`, `process_block`, `reset`; y[n] = Σ h[k]x[n−k].
* Design of the taps (Pack 04): h[n] = 2f_c·sinc(2f_c(n − (N−1)/2))·w[n], N = 21, f_c = 0.1; centre tap = 0.2 (window = 1). The sinc is the inverse DTFT of the ideal low-pass; the window trims it.
* Octave: `y = filter(h, 1, x)`. Hand-written sinc and Hamming (no packages).
* C++ points: circular delay line; *streaming correctness*: results must not depend on how the input is chopped into blocks — the test uses irregular blocks. Real radios process 1–10 ms buffers, so state must persist.
* Checks: impulse response equals taps; block size irrelevant (bit-identical, same operations in same order); `reset` clears; empty taps throw; match Octave (1e-12).

### Build 3 — 4FSK modulator
* Spec (Pack 05; **VERIFY** all P25 C4FM numbers against TIA-102.BAAA): 4800 sym/s, 2 bits/symbol = 9600 bit/s; dibit 01 → +3, 00 → +1, 10 → −1, 11 → −3 times a 600 Hz deviation unit (levels ±600, ±1800 Hz). Here: **rectangular** frequency pulses, 10 samples/symbol at f_s = 48 kHz. (Real C4FM shapes the pulses; this pack builds the structure, not the filter.)
* Derivation: instantaneous frequency f[n] = level·600 Hz; phase φ[n+1] = φ[n] + 2πf[n]/f_s; out = e^{jφ}. Increment per sample = level · 2π·600/48000 = level · π/40 = level · 0.07854 rad; for level +3: 0.2356 rad/sample, 2.356 rad (135°) per symbol.
* Octave: `kron(lev, ones(1,sps))`, `cumsum`.
* Checks: unit magnitude; phase increment per symbol matches the level (test); levels bit-exact; waveform vs Octave (1e-9).

### Build 4 — 48-bit sync correlator (hard decisions)
* Spec: `sync_hamming_distances(bits, word)` → vector d[l]; `sync_detect(d, max_errors)`.
* Maths (Pack 07): for random bits, d ~ Binomial(48, ½): mean 24, σ = √12 = 3.46. P(d ≤ 4) = 213 053/2⁴⁸ = 7.57×10⁻¹⁰ per lag (≈ 2.7×10⁻⁷ over the 353 lags here). A true sync with BER 5 % has mean 2.4 errors, so a threshold of 4 catches ≈ 91 %.
* Word: P25 frame sync 0x5575F5FF77FF (**VERIFY** against TIA-102.BAAA). The generator embeds it with 2 bit errors at lag 100.
* Octave: `sum(xor(rx(l:l+47), word))` — Hamming distance per lag.
* Checks: **bit-exact** vs Octave; brute-force cross-check; exact-match search finds only the true lag.

### Build 5 — CFO demo: estimate + correct
* Model: rx[n] = k[n]·e^{j(2πf₀n+θ)} + noise, k = known ±1 symbols. r[n] = rx[n]k[n] is a tone; Σ r[n]r*[n−1] = (N−1)e^{j2πf₀}+noise ⇒ **f̂ = arg(Σ)/2π** (Pack 02/09). Unambiguous for |f₀| < ½ cycle/sample. f₀ = 0.0123 cycles/sample = 590 Hz at 48 kHz.
* Correction: multiply by e^{−j2πf̂n}. Afterwards out[n] ≈ k[n]e^{jθ}: the frequency is gone; a *constant* phase θ remains (to be removed by a phase estimate, Pack 07).
* Checks: noiseless estimate exact to 1e-12; negative f₀ = −0.3 within 1e-3 with noise; Octave match (est 1e-12, output 1e-9).

### Build 6 — QPSK over AWGN vs theory
* Theory per bit: Q(√(2E_b/N₀)) = ½erfc(√(E_b/N₀)) (Pack 08). Values: 0 dB 0.07865 · 2 dB 0.03750 · 4 dB 0.01250 · 6 dB 0.002388 · 8 dB 1.91×10⁻⁴.
* Simulation: Es = 1 (symbols (±1±j)/√2), E_b = ½, N₀ = E_b/(E_b/N₀), noise N(0, N₀/2) per component.
* Octave exports only the **theory** (`ber_theory.txt`) — noise cannot be matched (§2.4).
* Checks: 10 % agreement at 2, 4, 6 dB with 2×10⁶ bits; theory function equals Octave's `erfc` expression to 1e-12.

### Build 7 — Q15 FIR vs float
* Spec (Pack 09): y[n] = sat16((Σ h[k]x[n−k] + 2¹⁴) >> 15), int64 accumulator.
* Octave reference: the same integer arithmetic in doubles (`q15_fir_ref` in the script) → **bit-exact**. Taps [12000 −8000 20000 20000 −8000 12000] (Σ = 48000/32768 = 1.46 > 1), first four inputs forced to [32767 32767 −32768 −32768] so that output 3 saturates: Σ = 1 179 608 000 → /32768 = 35 999 → 32767.
* Float comparison: for unsaturated outputs the integer result differs from the exact real-valued filter by at most ½ LSB (a single rounding) — see Pack 09's test; Octave `filter` on dequantised data lets you plot this error.
* Checks: bit-exact vs Octave, including the saturated sample.

---

## 5. Misconceptions
1. **"Same seed, same random numbers."** Not across languages, libraries or even standard-library versions.
2. **"A passing tolerance of 1e-3 is fine."** It hides bugs; derive the tolerance (§2.2).
3. **"The Octave model proves the C++ is right."** Only if the model is *independently* derived. If you write the model from the C++ you copy its bugs. (Cross-check each against the maths — impulse, tone, Parseval, theory.)
4. **"Make FFT outputs bit-identical."** Unattainable between different FFT algorithms; bit-exactness is for integer pipelines.
5. **"Octave integers behave like C++ integers."** Octave saturates; C++ wraps or is UB.
6. **"Vectors in git forever."** Regenerate vectors from the model so the model, not a stale file, is the reference.
7. **"Testing = comparing with a golden file."** Also test properties (impulse response, Parseval, linearity, shift invariance, streaming = block) and theory — these work without Octave and catch shared bugs.

---

## 6. Etherstack / radio context
* Verification pyramid in a modem project: (a) analytic/property tests of each block, (b) vector tests against the reference model (bit-exact for the fixed-point datapath that will become FPGA/DSP code), (c) statistical end-to-end tests (BER/PER vs theory, then vs conformance sensitivity limits), (d) hardware-in-the-loop.
* Test vectors must be synthetic or from non-proprietary sources; reproducible generation (fixed seeds/LCG) lets anyone regenerate them.
* Conformance numbers (sensitivity, 5 % BER vs 1 % BER definitions, sync words) must come from the standards: **VERIFY**.

---

## 7. Exercises

### Part A — pen & paper
**A1.** Derive a tolerance for the 6-tap Q15-style reference if it were computed in double with |x| ≤ 1, Σ|h| = 1.5.
**A2.** Evaluate (0.1+0.2)+0.3 and 0.1+(0.2+0.3) in double; why do they differ?
**A3.** How many bits must you simulate to estimate a BER of 10⁻³ to ±5 % (σ), and 10⁻⁵?
**A4.** 4FSK per-sample phase increment for levels −3, −1, +1, +3 at 600 Hz units, f_s = 48 kHz. Phase advance per symbol (sps = 10)?
**A5.** False-alarm probability over the 353 lags of Build 4 for a threshold of 4 errors.
**A6.** CFO 0.0123 cycles/sample at f_s = 48 kHz in Hz; maximum unambiguous CFO of the lag-1 estimator in Hz.
**A7.** Compute the Q15 FIR output for x = [16384, 16384, −32768], h = [16384, 8192] by hand.
**A8.** Spot the bug: Octave `idx = mod(i-1, 4)` vs C++ `(i-1) % 4` with i = 0.

### Part B — C++ (`code/`)
* **B1** `fft` · **B2** `FirFilter` · **B3** `fsk4_levels`, `fsk4_modulate` · **B4** `sync_hamming_distances`, `sync_detect` · **B5** `estimate_cfo`, `correct_cfo` · **B6** `qpsk_ber_theory`, `simulate_qpsk_ber` · **B7** `q15_fir`.
* Then run `octave octave/gen_vectors.m` and re-run the tests: the `OctaveVectors.*` tests must go from SKIPPED to PASSED.
* **Stretch.** (i) Tighten the 4FSK tolerance to 1e-12 — does it still pass? (ii) Change the C++ FFT to `float` twiddles: which tolerance catches it? (iii) Add a P25 sync search with soft (correlation) decisions and compare its miss/false-alarm rates with the hard-decision one at BER 5 %.

### Part C — interview questions
1. How do you verify a C++ DSP block against a Matlab/Octave model?
2. Why can't you demand bit-exact equality for an FFT? Where *can* you?
3. How do you test a BER simulator without matching random streams?
4. Name five differences between Octave/Matlab and C++ that cause mismatches.
5. How would you structure vectors and tests so CI works on a machine without Octave?
6. Your fixed-point filter differs from the float model by 3 LSB at some sample. How do you decide whether that is a bug?

---

## 8. VERIFY list
| Item | Used here | Check against |
|---|---|---|
| P25 C4FM: 4800 sym/s, 2 bits/sym, ±600/±1800 Hz, dibit map | 4FSK build | TIA-102.BAAA |
| Frame sync 0x5575F5FF77FF (48 bits) | Build 4 | TIA-102.BAAA |
| Rectangular 4FSK ≠ real C4FM pulse shaping | simplification | standard's shaping filter |
| Q(x)=½erfc(x/√2) values in Build 6 | hand-computed | `erfc` in Octave |
| Tolerances in §2.2 | hand-derived | run the tests; tighten if margin is large |
| Octave function names/behaviour (`dec2bin`, `kron`, `filter`) | core Octave | your Octave version |
| Nothing was compiled or run | no toolchain available | build + run |
