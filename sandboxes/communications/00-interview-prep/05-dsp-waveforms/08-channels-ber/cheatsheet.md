# Pack 08 cheatsheet — Channels & BER

## AWGN
* n ~ CN(0, N0): each of I, Q has variance **N0/2**.
* BPSK/QPSK (per bit): **P_b = Q(√(2Eb/N0))**, Q(x) = ½ erfc(x/√2). QPSK SER ≈ 2P_b.
* Eb/N0 for BER: 1e-2 → 4.3 dB, 1e-3 → 6.8 dB, 1e-5 → 9.6 dB.
* 16-QAM Gray: P_b ≈ ¾ Q(√(0.8 Eb/N0)); +3.7…4 dB vs QPSK.

## SNR ↔ Eb/N0
* Eb = P/Rb, Es = P/Rs, SNR = P/(N0 B).
* **Eb/N0 = SNR · B/Rb**,  Es/N0 = SNR · B/Rs = Eb/N0 · log2 M.
* Per sample (noise fills fs): SNR_sample = (Es/N0)/sps.
* 9600 bit/s in 12.5 kHz: Eb/N0 = SNR + 1.15 dB (VERIFY rates).
* Noise floor: −174 dBm/Hz + 10 log10 B + NF. 12.5 kHz → −133.0 dBm.

## Fading
* Multipath: y = Σ h_l x[n−l]. Coherence BW ≈ 1/(5σ_τ); flat if BW_signal ≪ B_c.
* **Rayleigh**: h ~ CN(0,1), |h|² ~ Exp(1): P(|h|²<x) = 1 − e^{−x}. 10 % → −10 dB … 1 % → −20 dB.
* **Rician**: h = √(K/(K+1)) e^{jθ} + √(1/(K+1)) CN(0,1); Var|h|² = (1+2K)/(1+K)²; K=0 Rayleigh, K→∞ AWGN.
* **Doppler** f_d = v f_c/c; Clarke autocorr J0(2π f_d τ); T_c ≈ 0.423/f_d (VERIFY).
  100 km/h @ 400 MHz → 37.06 Hz, T_c ≈ 11.4 ms.
* BPSK Rayleigh coherent: P_b = ½(1 − √(γ/(1+γ))) → 1/(4γ). 10 dB: 0.0233 (AWGN 3.9e-6). 1e-3 needs ≈ 24 dB (+17 dB).
* Fixes: diversity, interleaving+FEC, equalisation/OFDM, hopping, power control.

## BER-simulation hygiene
≥ 100 errors/point · fixed seeds · check N0/2 per component · compare vs closed form · test 0 noise & huge noise.

## Misconceptions
SNR ≠ Eb/N0 · QPSK BER = BPSK BER (per bit) · average SNR not enough in fading · Rayleigh = no LOS (Rician with LOS).
