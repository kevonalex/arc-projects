# Pack 06/03 cheatsheet — 5G NR and 6G (every line cites the worksheet section)

Synthetic data only. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| SCS = 15·2^μ kHz, μ = 0…4 → 15/30/60/120/240 | §3.1, Q1 |
| T_useful = 1/SCS (66.67, 33.33, 16.67, 8.33 µs); doubling SCS halves symbols | §3.1, Q2 |
| Subframe = 1 ms always; slots/subframe = 2^μ; 14 symbols per slot (normal CP) | §3.1, Q1 |
| Slot = 1, 0.5, 0.25, 0.125 ms; avg symbol 71.43 / 35.71 / 8.93 µs (μ = 0/1/3) | §3.1, Q1 |
| Bigger SCS: lower latency, Doppler/phase-noise robust; shorter CP, coarser PRBs | §3.1, Q3 |
| More MHz, not shorter symbols, gives throughput | §3.1 |
| FR1: SCS 15/30/60; FR2: 60/120 (VERIFY) | §3.2, Q3 |
| PRBs: 20 MHz → 106 (15 kHz) / 51 (30 kHz); 100 MHz@30 → 273; 100 MHz@120 → 66; 400 MHz@120 → 264 | §3.2, Q4 |
| PRB = 12 subcarriers; wider at higher SCS ⇒ fewer PRBs | §3.2, Q4 |
| TDD slots in period = ms × 2^μ; 2.5 ms @30 kHz = 5 (DDDSU) | §3.2, Q5 |
| gNB = CU (RRC, PDCP) + DU (RLC, MAC, high PHY) + RU; CU–DU = F1 | §3.3, Q6 |
| AMF = mobility/NAS; SMF = sessions; UPF = user plane (GTP-U, N3) | §3.3, Q7 |
| SBA = HTTP/2 + JSON services, NRF discovery; user plane not SBA | §3.3, Q8 |
| Slice = S-NSSAI (SST + SD); resources and NFs, not just QoS marking | §3.3, Q9 |
| LDPC for PDSCH/PUSCH; polar for PDCCH/PBCH | §3.4, Q10 |
| BG2 if A ≤ 292 or R ≤ 0.25 or (A ≤ 3824 and R ≤ 0.67); else BG1 | §3.4, Q11 |
| K_cb = 8448 (BG1) / 3840 (BG2); blocks = ceil(B / (K_cb − 24)) when B > K_cb | §3.4, Q12 |
| FSPL = 20·log10(4π d f / c): ×2 distance = +6.02 dB; ×10 frequency = +20 dB | §3.5, Q13 |
| FSPL: 3.5 GHz @1 km 103.33 dB; 28 GHz 121.39 dB | §3.5, Q13 |
| Shannon C = B·log2(1+SNR): linear in B, log in SNR (20 MHz@20 dB = 133.2 Mbit/s) | §3.5, Q14 |
| LEO 550 km: 1.83 ms, FSPL 153.28 dB; GEO: 119.4 ms, 189.54 dB | §3.6, Q15 |
| Doppler = v·f/c: train 1.62 kHz @3.5 GHz; LEO 50.4 kHz @2 GHz | §3.6, Q15 |
| NTN needs pre-compensation, extended timers | §3.6, Q15 |
| 6G: Rel-20 study, Rel-21 normative; freeze ~Mar 2027, specs ~Mar 2029, commercial ~2030 (VERIFY) | §3.7, Q16 |
| 6G themes: AI-native, sensing (ISAC), NTN, new spectrum | §3.7, Q16 |
| 7G: NO standard — say so confidently | §3.7, Q16 |
| Gateway sits behind the UPF; cannot see RAN internals | §3.3, Q17 |
