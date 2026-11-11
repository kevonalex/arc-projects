# Pack 04 cheatsheet — Filters

## Convolution
* y[n] = Σₖ h[k] x[n−k];  length Lx + Lh − 1.
* H(ω) = Σ h[k] e^{−jωk}.  Time conv ↔ frequency product.
* Moving average (L taps): nulls at f = m/L, DC gain 1 (with 1/L weights).

## FIR windowed-sinc low-pass
```
m = n − (N−1)/2
h[n] = 2fc · sinc(2fc·m) · w[n],   sinc(x)=sin(πx)/(πx),   normalise Σh = 1
```
* sinc = inverse DTFT of a rectangle: sin(ωc n)/(πn).
* Rect window: Gibbs ≈ 9 % (−21 dB), independent of N.
* Hamming −53 dB, transition ≈ 3.3/N; Hann −44 dB ≈ 3.1/N; Blackman −74 dB ≈ 5.5/N.
* Symmetric h ⇒ linear phase, group delay (N−1)/2 samples.
* fc = 0.25 → half-band (every 2nd tap 0).

## Biquad (TDF-II)
```
y  = b0 x + s1
s1 = b1 x − a1 y + s2
s2 = b2 x − a2 y
```
Stable ⇔ |a2| < 1 and |a1| < 1 + a2.
RBJ low-pass: α = sin ω0/(2Q); b0=b2=(1−cos ω0)/2, b1=1−cos ω0; a0=1+α, a1=−2cos ω0, a2=1−α; divide by a0.
f0=0.1, Q=0.7071 → b=[0.06746 0.13491 0.06746], a=[1 −1.14298 0.41280].
DC blocker: y = x − x₁ + αy₁ ; f₃dB ≈ (1−α)·fs/2π.

## Rate change
* Decimate ÷M: filter (cutoff ≤ 0.5/M of input rate) then keep every Mth.
* Polyphase: E_p[q] = h[qM+p]; y[m] = Σ_p Σ_q E_p[q] x[(m−q)M − p]. Cost N per output sample.
* Interpolate ×L: zero-stuff, LPF with **gain L**, cutoff 0.5/L. y[nL+p] = Σ_q h[qL+p] x[n−q].

## Misconceptions
* More rect-window taps ≠ lower ripple.
* Linear phase ≠ zero delay.
* Decimation needs the filter first.
* Polyphase = same output, fewer MACs.
