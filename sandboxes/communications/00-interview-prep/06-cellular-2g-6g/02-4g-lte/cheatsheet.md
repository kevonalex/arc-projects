# Pack 06/02 cheatsheet — 4G LTE (every line cites the worksheet section)

Synthetic data only. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| SCS 15 kHz; RB = 12 subcarriers = 180 kHz; T = 1/Δf = 66.67 µs | §3.1, Q1, Q2 |
| 1.4/3/5/10/15/20 MHz = 6/15/25/50/75/100 RB (7 MHz → −1) | §3.1, Q1 |
| 20 MHz: 100 RB → 18 MHz used, FFT 2048, 30.72 Msps | §3.1, Q1 |
| FFT sizes 128/256/512/1024/1536/2048 for 6/15/25/50/75/100 RB | §3.1 |
| Normal CP: 160 (symbol 0) / 144 samples at FFT 2048; 5.208 / 4.6875 µs | §3.2, Q3 |
| Slot = 7 symbols = 15 360 samples = 0.5 ms; CP overhead ≈ 6.7 % | §3.2, Q3 |
| RE = 1 subcarrier × 1 symbol; subframe has nRB·12·14 (16 800 for 100 RB) | §3.2, Q5 |
| Raw ceiling 100.8 Mbit/s (64QAM, 1 layer) is NOT user throughput | §3.2, Q5 |
| CP makes channel convolution circular → one-tap equalisation | §3.2, Q4 |
| OFDM waveform ≈ Gaussian → high PAPR (8–12 dB) → PA back-off → battery | §3.3, Q6 |
| SC-FDMA = DFT-spread OFDM; M = N gives identity; lower PAPR | §3.3, Q7 |
| Downlink OFDMA (eNB has power), uplink SC-FDMA (handset PA) | §3.3, Q8 |
| PHY transmits, MAC schedules+HARQ, RLC ARQ, PDCP cipher+ROHC, RRC connection, NAS attach | §3.4, Q9 |
| Peers: PHY…RRC → eNB; NAS → MME (transparent via eNB) | §3.4, Q9 |
| HARQ fast soft-combining; RLC ARQ slow and exact | §3.4, Q10 |
| MME = signalling; HSS = subscriber DB; S-GW = mobility anchor; P-GW = IP anchor | §3.5, Q11 |
| Control-plane / user-plane split | §3.5, Q11 |
| QCI 1: 100 ms, 5: 100, 9: 300, 65 (MCPTT voice): 75, 69: 60 (VERIFY) | §3.5, Q13 |
| GTP-U: outer IP 20 + UDP 8 (port 2152) + GTP 8 = 36 B (40 w/ optionals) | §3.5, Q12 |
| Inner MTU on 1500 link = 1464 (1460) | §3.5, §4(c), Q12 |
| GTP-U is not encrypted (IPsec / PDCP do that) | §3.5, Q12 |
| VoLTE: QCI 1 bearer, AMR 31 B / 20 ms = 12.4 kbit/s payload | §3.6, Q14 |
| RTP+UDP+IPv4 = 40 B (IPv6 60); ×2.3 rate; ROHC → ≈3 B | §3.6, Q14 |
| A SGi-side gateway sees IP/RTP, not radio layers | §3.4–3.6, Q15 |
