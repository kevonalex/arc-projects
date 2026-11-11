# Pack 02 cheatsheet — I/Q & complex baseband

## Identities
* e^{jθ} = cos θ + j sin θ  (Taylor series)
* cos θ = (e^{jθ} + e^{−jθ})/2, sin θ = (e^{jθ} − e^{−jθ})/(2j)
* (x+jy)·e^{jφ} = (x cos φ − y sin φ) + j(x sin φ + y cos φ)   → rotation by φ
* e^{ja}·e^{jb} = e^{j(a+b)} → multiplication adds frequencies (mixing)

## Real vs complex
* Real signal ⇒ X(−f) = X*(f). +f and −f indistinguishable.
* Real mixer: cos·cos = ½[cos(a−b)+cos(a+b)] → image problem.
* Complex mixer: multiply by e^{−j2πf₀n} → single shift down by f₀.

## Quadrature down-conversion
```
I = LPF{ 2 x cos(2π fc t) }      Q = LPF{ −2 x sin(2π fc t) }      z = I + jQ
x = A cos(2π(fc+Δ)t + θ)  →  z = A e^{j(Δt+θ)}
```

## Readouts
| | formula |
|---|---|
| magnitude | √(I²+Q²) |
| phase | atan2(Q, I) |
| inst. freq (cycles/sample) | arg(z[n+1]·z*[n]) / 2π |
| Hz | × fs |
| lag-1 CFO est. | arg(Σ z[n+1]z*[n]) / 2π |

## Numbers to remember
* 300 Hz CFO @ 4800 sym/s → 22.5°/symbol (360·Δf/Rs).
* Gain error ε, phase error φ (rad): image/wanted amplitude ≈ ½√(ε²+φ²); IRR ≈ 4/(ε²+φ²). ε=0.1, φ=5° → −23.6 dB.
* Complex fs ≥ B (not 2B).
* Superhet image = f_c − 2·f_IF (low-side LO). 150 MHz, 10.7 MHz → 128.6 MHz.

## Misconceptions
* I and Q are two signals → one complex signal.
* Delay of I by T/4 gives Q → only at one frequency.
* LPF after mixer optional → no, 2f_c term aliases.
* 1 MS/s IQ = 500 kHz BW → = 1 MHz.
* Phase differences via atan2 − atan2 → use arg(z[n+1]·z*[n]).
