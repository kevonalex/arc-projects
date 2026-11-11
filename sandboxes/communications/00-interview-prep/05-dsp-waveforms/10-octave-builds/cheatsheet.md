# Pack 10 cheatsheet — Octave model → C++ match

## Workflow
Derive model from the maths → Octave script writes `%.17g` vectors → C++ reads → compare.
Also: property tests (impulse, tone, Parseval, streaming = block) that need no model.

## Which check?
| path | check | tolerance |
|---|---|---|
| integers (Hamming dist., symbol levels, Q15 FIR) | **exact** | 0 |
| deterministic float (FIR, CFO) | abs/rel error | ≈ 100–1000 × (N·ε·Σ\|terms\|) → 1e-12 |
| FFT (different algorithm) | abs error | ≈ c·ε·log₂N·‖X‖ → 1e-12 (N = 64); never bit-exact |
| accumulated phase (4FSK) | abs | 1e-9 |
| noise / BER | statistics vs theory | rel σ = 1/√errors; ≥ 400 errors → ±10 % (95 %) |

ε = 2⁻⁵³ = 1.1e-16. (0.1+0.2)+0.3 = 0.6000000000000001 ≠ 0.6 = 0.1+(0.2+0.3).

## Formulas used
* FFT: X[k] = E[k] + W^k O[k]; X[k+N/2] = E[k] − W^k O[k]; W = e^{−j2π/N}.
* Windowed sinc: h[n] = 2f_c sinc(2f_c(n−(N−1)/2)) w[n].
* 4FSK: f = level·600 Hz, φ[n+1] = φ[n] + 2πf/fs; levels 01→+3, 00→+1, 10→−1, 11→−3 (VERIFY); fs/sps = 4800 sym/s → 9600 bit/s.
* Sync: d ~ Bin(48,½) (μ 24, σ 3.46); P(d≤4) = 7.57e-10.
* CFO: f̂ = arg(Σ r[n] r*[n−1])/2π, |f| < ½; correct with e^{−j2πf̂n}.
* QPSK BER/bit = ½erfc(√(Eb/N0)): 0 dB 0.0787, 2 dB 0.0375, 4 dB 0.0125, 6 dB 2.39e-3, 8 dB 1.9e-4.
* Q15 FIR: (Σ h x + 2¹⁴) >> 15, int64 acc, saturate.

## Octave ↔ C++ traps
1-based vs 0-based · `mod` vs `%` · int saturate vs wrap · `round` vs `rint` · `sinc` normalised · symmetric vs periodic windows · `ifft` scaling · RNG streams differ · `filter` state · `'` conjugates.

## Commands
```
octave --no-gui octave/gen_vectors.m
cmake -S code -B build [-DUSE_SOLUTIONS=ON] && cmake --build build && ctest --test-dir build --output-on-failure
```
Missing vector file ⇒ test SKIPPED, not failed.
