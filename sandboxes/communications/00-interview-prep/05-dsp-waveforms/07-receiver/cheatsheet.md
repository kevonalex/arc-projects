# Pack 07 cheatsheet — Receiver

## Matched filter
* SNR_max = E_s/σ² (real, white noise N0/2 → 2E/N0); h[k] = conj(s[N−1−k]).
* Cauchy–Schwarz: (Σ h u)² ≤ Σh² Σu².
* Example s=[1,2,3]: matched 14, boxcar 12, single-tap 9 (σ²=1).

## Sync correlation
* ρ[l] = |Σ r[l+k] ref*[k]|² / (Σ|r|² Σ|ref|²)  ∈ [0,1].
* Hard decisions: P_fa(n=48, d) = Σ_{k≤d} C(48,k) / 2⁴⁸.  d=0: 3.55e-15; d=4: 7.57e-10 (≈ 1 false alarm / 38 h @ 9600 bit/s).
* CFO loss (L symbols, f cycles/symbol): |sin(πLf)/(L sin πf)|. L=24, f=0.0625 → 0.214 (−13.4 dB).
* P25 FS 0x5575F5FF77FF / DMR BS voice 0x755FD7DF75F7 — **VERIFY**.

## Gardner TED
```
e = (y[k] − y[k−1]) · y[k−½]       e > 0 ⇒ sampling late
t_{k+1} = t_k + sps − (kp·e + Σ ki·e)
```
* Needs 2 sps, non-data-aided, ignores carrier phase.
* K_d ≈ 2.6 per T for RC β=0.35; sps=8 → kp ≈ 0.15, ki ≈ kp/100.
* Linear interp OK at ≥ 8 sps; cubic/Farrow at 4.

## PLL / Costas (BPSK)
```
y = x e^{−jφ};  e = Re(y)·Im(y) = ½ sin2θ;  f += β e;  φ += f + α e
θn = BnT/(ζ + 1/4ζ);  d = 1 + 2ζθn + θn²;  α = 4ζθn/d;  β = 4θn²/d
```
BnT = 0.02, ζ = 0.707 → α = 0.0519, β = 0.001385. π ambiguity (BPSK), π/2 (QPSK).
Pull-in ≈ few × θn rad/sample; coarse CFO first if bigger.

## AGC (log domain)
`g ← g·exp(μ(r − |g x|))`, time constant 1/(μ r), level-independent. Attack fast / decay slow.

## Numbers
1 ppm @ 400 MHz = 400 Hz = 0.083 cycles/symbol @ 4800 sym/s (30°/symbol).
5 ppm symbol clock @ 4800 sym/s → 0.024 sym/s.

## Misconceptions
MF ≠ peak maximiser · MF = time-reversed conj pulse · longer coherent sync hurts with CFO · Gardner needs no decisions · Costas leaves π ambiguity.
