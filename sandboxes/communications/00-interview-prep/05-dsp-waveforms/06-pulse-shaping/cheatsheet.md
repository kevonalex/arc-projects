# Pack 06 cheatsheet — Pulse shaping

## ISI & Nyquist
* Sample s(nT) = a_n g(0) + Σ_{k≠n} a_k g((n−k)T).  ISI-free ⇔ g(mT) = δ[m].
* Frequency: Σₖ G(f − k/T) = T.  Minimum BW = R_s/2 (sinc).
* Vestigial (odd) symmetry about 1/(2T) allows excess bandwidth.

## Raised cosine (β = roll-off, 0..1)
```
h_RC(t) = sinc(t/T) · cos(πβt/T) / (1 − (2βt/T)²)         sinc(x) = sin(πx)/(πx)
limit at |t| = T/(2β):  (π/4) sinc(1/(2β))
```
* Occupied BW: (1+β)R_s (passband, two-sided); (1+β)R_s/2 baseband one-sided.
* Tails ~ 1/t³ (β > 0) vs 1/t (sinc).
* h_RC(0.5T) at β=0.35 ≈ 0.619.

## Root raised cosine
```
h_RRC(t) ∝ [ sin(πt/T(1−β)) + 4βt/T · cos(πt/T(1+β)) ] / [ πt/T · (1 − (4βt/T)²) ]
t=0:        1 − β + 4β/π
|t|=T/(4β): (β/√2)[(1+2/π) sin(π/4β) + (1−2/π) cos(π/4β)]
```
* RRC ∗ RRC = RC (zero ISI); RRC alone has ISI (h_RRC(T)/h_RRC(0) ≈ −8 % at β = 0.35).
* Rx RRC = matched filter. Normalise Σh² = 1 → cascade centre = 1.
* Truncation: span ≥ 6–8 symbols each side.

## Eye diagram
* Overlay 2-symbol windows. Vertical opening = noise margin; horizontal = timing margin.
* Half-symbol sampling phase = closed eye (crossings), normal.
* Timing error τ: worst-case height = 2[h(τ) − Σ_{m≠0} |h(τ+mT)|].

## Standards numbers — VERIFY
Roll-offs: P25/DMR ≈ 0.2 (?), TETRA 0.35 (?) — from memory, check standards.

## Misconceptions
RRC alone ≠ zero ISI · BW ≠ R_s · sinc not practical · bigger β = more BW, more timing tolerance.
