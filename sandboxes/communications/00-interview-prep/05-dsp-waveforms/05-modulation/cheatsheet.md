# Pack 05 cheatsheet — Modulation

## One-liners
* AM: z = 1 + m·x; demod |z|.  FM: φ[n+1] = φ[n] + 2π·kf·x[n]/fs; demod arg(z[n+1]z*[n])·fs/(2π kf).
* Carson: BW ≈ 2(Δf + fm).  NBFM 2.5 kHz / 3 kHz → 11 kHz (VERIFY service rules).
* Binary FSK orthogonal spacing: coherent 1/(2T) (h = 0.5, MSK), non-coherent 1/T (h = 1).

## 4FSK / C4FM (P25 Phase 1) — VERIFY against TIA-102.BAAA
* 4800 sym/s × 2 bit/sym = **9600 bit/s**, 12.5 kHz channel.
* Levels (P25): 01 → +1800 Hz, 00 → +600, 10 → −600, 11 → −1800 (D = 600 Hz, symbols ±1/±3).
* DMR 4FSK (VERIFY TS 102 361-1): 4800 sym/s, ±648 / ±1944 Hz.
* Slicer thresholds at 0, ±2D. CFO = DC shift of every level → subtract mean/AFC.
* h (adjacent spacing 1200 Hz) = 1200/4800 = 0.25.

## PSK
* QPSK Gray: (b1 b0) → ((−1)^b1 + j(−1)^b0)/√2.  BER_bit = Q(√(2Eb/N0)) (same as BPSK).
* Midpoint of straight transition between unit points θ apart: cos(θ/2). QPSK 180° → 0; π/4-DQPSK 135° → 0.383.
* π/4-DQPSK (TETRA, VERIFY): 00:+π/4, 01:+3π/4, 11:−3π/4, 10:−π/4. Alternates between two QPSK sets. Differential ⇒ constant phase offset cancels. TETRA: 18 ksym/s, 36 kbit/s, 25 kHz, RRC α 0.35.

## QAM
16-QAM: levels ±1, ±3 per axis /√10. dmin = 2/√10. +3.98 dB Eb/N0 vs QPSK (d²/Eb ratio 4 : 1.6). Gray 00,01,11,10 per axis.

## OFDM
* Subcarrier spacing Δf = 1/T_u = fs/N. Orthogonal: ∫ e^{j2π(k−l)t/T} dt = 0.
* Tx: IFFT (1/N), prepend last C samples. Rx: drop CP, FFT.
* CP ≥ L−1 turns linear conv into circular ⇒ Y[k] = H[k]X[k] ⇒ X̂ = Y/H.
* Overhead C/(N+C). Not "just a guard interval" — zeros don't give circularity.

## Misconceptions
4FSK ≠ 4 bit/sym · FSK needs no coherent LO · π/4-DQPSK isn't constant envelope · OFDM spectra overlap but are orthogonal · Gray reduces BER, not SER.
