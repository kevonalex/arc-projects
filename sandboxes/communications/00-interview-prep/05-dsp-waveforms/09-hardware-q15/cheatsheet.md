# Pack 09 cheatsheet — Hardware reality and Q15

## Q formats
* Q15: int16/2¹⁵, range [−1, 0.99997], LSB 3.05e-5. Q31: int32/2³¹. **−1 exists, +1 doesn't.**
* x → Q15: `clamp(round(32768 x))`.
* SQNR = 6.02 N + 1.76 dB (N = word length incl. sign, full-scale sine): 16 b = 98.1 dB; at amplitude A subtract 20log₁₀(1/A) (0.5 → 92.1 dB).
* **Add**: wraps (0.75+0.5 → −0.75) → use saturating add.
* **Multiply**: Q15×Q15 = Q30 → `(a*b + (1<<14)) >> 15`; **−1×−1 → saturate to 32767**. Truncation biases −½ LSB.
* **FIR**: accumulate wide (int64), round **once**, saturate. Guard bits = ⌈log₂ Σ|h|⌉ (0 if Σ|h| ≤ 1). Peak out = Σ|h|·peak in.
* Q31 for IIR states, NCO phase, narrow-filter coefficients.
* Use fixed: bounded range, bit-exact, cheap. Use float: unbounded range, prototyping.

## RF chain
LNA → mixer → IF filter → ADC → DDC (NCO, CIC, FIR).
* **Friis**: F = F₁ + (F₂−1)/G₁ + (F₃−1)/(G₁G₂) + … ; loss before LNA adds dB-for-dB.
* Noise floor: −174 dBm/Hz + 10log B + NF (12.5 kHz: −133.0 dBm + NF).
* ADC: SQNR 6.02N + 1.76; processing gain 10log(fs/2B).
* CIC growth: N log₂(R M) bits. NCO resolution fs/2^W.

## Impairments
| impairment | model | size/formula | cure |
|---|---|---|---|
| CFO | z·e^{j2πΔf n/fs} | Δf = ppm·f_c; rotation/sym = 360Δf/R_s | AFC from preamble, mean-frequency tracking, M-th power (range R_s/2M) |
| DC | + d_I + j d_Q | spike at 0 | low-IF; subtract mean; 1-pole blocker (fc ≈ (1−α)fs/2π) |
| IQ imbalance | Q' = g(Q cosφ + I sinφ) | IRR = \|1+ge^{jφ}\|²/\|1−ge^{−jφ}\|² ≈ 4/(ε²+φ²); phase only cot²(φ/2) | Gram–Schmidt: ρ = E[IQ]/E[I²]; Q₁ = Q − ρI; rescale |
| Phase noise | Wiener walk | Δν = σ² fs/2π; reciprocal mixing floor = P_blk + L(Δf) + 10log B | better synthesiser |
| Quantisation | uniform ±½ LSB | SQNR above | dither, more bits, oversampling |

Numbers to remember: 1 ppm @ 450 MHz = 450 Hz = 33.75°/symbol @ 4800 sym/s; 4FSK threshold at ±1200 Hz so ≥ 600 Hz offset breaks C4FM slicing; IRR 1° → 41 dB, 1 % gain → 46 dB; (g 1.1, 5°) → 23.8 dB.

## Misconceptions
Saturation ≠ no error · Q15 multiply needs shift+round · round once · DC-block not harmless on zero-IF FM · CFO shifts thresholds · 98 dB is for full scale.
