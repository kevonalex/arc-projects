# Pack 03 cheatsheet — DFT / FFT

## Definitions
* X[k] = Σₙ x[n] e^{−j2πkn/N},  x[n] = (1/N) Σₖ X[k] e^{+j2πkn/N}
* Bin k ↔ k·fs/N (k < N/2); k ≥ N/2 ↔ (k−N)·fs/N.  Δf = fs/N = 1/T.
* Real x ⇒ X[N−k] = X[k]*.   Parseval: Σ|x|² = (1/N) Σ|X|².
* Orthogonality: Σₙ e^{j2π(m−k)n/N} = N·δ[m−k].
* Tone at bin m: |X[m]| = A·N (complex) or A·N/2 (real cosine, each of ±m).

## Radix-2 DIT
```
E = DFT(even), O = DFT(odd), W = e^{−j2π/N}
X[k]     = E[k] + W^k O[k]
X[k+N/2] = E[k] − W^k O[k]       (W^{k+N/2} = −W^k)
```
Cost (N/2)·log₂N complex mults (vs N²). Input order = bit-reversed.
8-pt bit-reverse: 0 4 2 6 1 5 3 7.   W8^k: 1, (1−j)/√2, −j, (−1−j)/√2.

## Worked 8-pt: x=[1,2,3,4,0…]
X = [10, −0.414−7.243j, −2+2j, 2.414−1.243j, −2, 2.414+1.243j, −2−2j, −0.414+7.243j]

## Windows (leakage)
| window | main lobe (±bins) | 1st sidelobe | far roll-off | ENBW | scallop (δ=.5) |
|---|---|---|---|---|---|
| rect | 1 | −13 dB | 6 dB/oct | 1.00 | −3.92 dB |
| Hann | 2 | −31 dB | 18 dB/oct | 1.50 | −1.42 dB |
* Hann (periodic): w[n] = 0.5 − 0.5 cos(2πn/N). Coherent gain 0.5.
* Zero padding = interpolation, not resolution.
* Fractional peak: δ = ½(α−γ)/(α−2β+γ) on log |X|.

## Misconceptions
* FFT ≠ approximation; same result as DFT.
* Window does not remove leakage, it reshapes it.
* Resolution needs longer data, not zeros.
* Don't forget amplitude correction for the window gain.
