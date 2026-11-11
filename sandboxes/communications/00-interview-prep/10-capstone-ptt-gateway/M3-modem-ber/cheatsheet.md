# Capstone M3 cheatsheet — 4FSK modem, channel, sync, BER (every row cites the worksheet)

Synthetic signals only. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Gray map: 00→0, 01→1, 11→2, 10→3; tones −1.5, −0.5, +0.5, +1.5 cycles/symbol | §3.1, Q1 |
| 2 bits per symbol; odd trailing bit ignored | §3.1 |
| sps = 8; unit amplitude ⇒ Es = 8; constant envelope | §3.2 |
| Continuous phase: one running phase accumulator, no jumps at symbol edges | §3.2 |
| Tone spacing 1/T ⇒ orthogonal: correlations 64 on the right tone, 0 elsewhere | §3.2, Q2 |
| Non-coherent detect = max of squared correlation magnitude over 4 tones; no carrier-phase recovery | §3.2 |
| Spacing 0.8 ⇒ 5.7 % leakage (not orthogonal) | Q2 |
| Noise: Es/N0 = sps/σ² ⇒ σ_comp = √(σ²/2) = √(0.5·sps/g) | §3.3, Q3 |
| σ_comp = 2.0 at 0 dB, 0.632 at 10 dB, 0.2 at 20 dB (sps 8) | Q3 |
| Forgetting the ½ ⇒ 3 dB worse than intended | Q4 |
| Correlator output SNR = Es/N0 | §3.3, Q3 |
| CFO δ (cycles/symbol) rotates sample n by e^{j2πδn/sps} | §3.3 |
| CFO loss sinc²(δ): −0.21 dB @0.12, −0.58 dB @0.2, −3.9 dB @0.5; neighbour tone = own at 0.5 | §3.3, Q5 |
| Breaks at half the tone spacing; 0.2 harmless, 0.6 ⇒ ~3/4 of symbols slide up | §3.3, Q6 |
| Channel (given) = delay zeros + 4·sps trailing zeros + CFO + AWGN | §3.3 |
| Sync = 24 bits = 12 symbols; metric 1.0 aligned, ≈ 1/sps = 0.125 on noise | §3.4, Q7 |
| Threshold 0.5 (0.4 at low SNR): false alarm vs miss; CRC is the final arbiter | §3.4, Q7, Q17 |
| Sync peak within ±1 sample (53 ± 1 in the test) | §3.4, §3.6 |
| CFO estimate: angle(Σ r[n+1]r*[n] · conj(s[n+1]s*[n]))·sps/2π; range ±4 cycles/symbol | §3.5, Q8 |
| Receive: sync → estimate CFO → derotate a COPY → demod total_bits/2 → Gray decode | §3.6, Q9 |
| 2338-bit frame @ 16 dB, CFO 0.12, delay 53: zero bit errors | §3.6 |
| Eb/N0 = Es/N0 − 3.01 dB (4FSK, 2 bits/symbol) | §3.7 |
| Ps = 3/2 e^{−g/2} − e^{−2g/3} + 1/4 e^{−3g/4}; Pb = (2/3) Ps | §3.7, Q10 |
| Why 2/3: orthogonal ⇒ 3 equiprobable wrong tones, 2 of 3 flip a given bit | §3.7, Q14 |
| Theory: 0 dB 0.343, 6 dB 9.8e-2, 8 dB 3.42e-2, 10 dB 5.98e-3, 12 dB 3.46e-4, 14 dB 3.5e-6 | §3.7 table |
| ber_sweep matches theory (8 dB: 3.422e-2 vs 3.418e-2) | §3.7 |
| bits_needed = 100 / BER: 1e5 @1e-3, 1e8 @1e-6 (≈ 14.3 min of frames) | §3.7, Q11 |
| Zero errors ≠ zero BER: rule of three, BER < 3/N | §3.8, Q12 |
| M1 design point BER 1e-3 ⇒ Es/N0 ≈ 11.4 dB (Eb/N0 ≈ 8.4 dB) | §3.8, Q18 |
| Frame loss vs Es/N0 (Hamming only): 10 dB 21 %, 11 dB 2 %, 12 dB 0.08 %, 14 dB 1e-7 | §3.8 |
| Gray is neutral for orthogonal AWGN; matters for adjacent-tone errors | §3.1, Q14 |
| Why non-coherent 4FSK: constant envelope, no phase recovery, robust; cost bandwidth/~1 dB | Q13 |
| Deterministic RNG (xorshift + Box-Muller) ⇒ platform-independent CI | Q16 |
