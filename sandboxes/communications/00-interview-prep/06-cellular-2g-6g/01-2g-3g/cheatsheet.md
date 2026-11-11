# Pack 06/01 cheatsheet — 2G GSM and 3G WCDMA (every line cites the worksheet section)

Synthetic data only; never live traffic. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| GSM bit rate = 13 MHz / 48 = 270.833 kbit/s; bit = 3.692 µs | §3.1, Q1 |
| Slot = 156.25 bit periods = 0.5769 ms (15/26); frame = 8 slots = 4.615 ms | §3.1, Q1 |
| Normal burst 148 bits + 8.25 guard | §3.1, §4(a), Q1 |
| TDMA turns spare speed into users; phone transmits 1 slot in 8 | §3.1, Q2 |
| P-GSM 900: UL = 890 + 0.2·n MHz, DL = UL + 45, n = 1…124 (out of range → −1) | §3.2, Q3 |
| TA step = one bit period round trip = 553.46 m; 6-bit TA → 34.87 km max | §3.2, Q4 |
| 35 km is a timing limit, not a power limit | §3.2, Q4 |
| Vocoder 13 kbit/s → coded TCH 456 bit / 20 ms = 22.8 kbit/s | §3.3, Q5 |
| Cluster N = i² + ij + j² (3, 4, 7, 12); reuse D/R = √(3N) (N=7 → 4.583) | §3.3, Q6 |
| Bigger N = better C/I, fewer channels per cell | §3.3, Q6 |
| GMSK = constant envelope; MSK deviation = Rb/4 = 67.7 kHz; Gaussian adds controlled ISI | §3.3, Q7 |
| MS → BTS → BSC → MSC → PSTN; HLR/VLR hold subscriber data | §3.3, Q8 |
| WCDMA chip rate 3.84 Mcps in 5 MHz; SF = chip rate / symbol rate | §3.4, Q9 |
| 15 ksym/s → SF 256 (24.08 dB); 30 → 128; 960 → 4 (6.02 dB) | §3.4, Q9 |
| Processing gain = 10·log10(SF) (power!) | §3.4, Q9 |
| Orthogonal codes: c1·c2 = 0 ⇒ other user cancels exactly (aligned) | §3.5, Q10 |
| OVSF: C(2n,2k) = [C,C]; C(2n,2k+1) = [C,−C] | §3.5, Q11 |
| Same-SF codes are orthogonal; conflict = ancestor/descendant | §3.5, Q11 |
| C(4,1) = (+,+,−,−) → children C(8,2), C(8,3) | §3.5, Q11 |
| Code-tree blocking: free leaves ≠ usable code | §3.5, Q12 |
| Misalignment: C(8,1)·C(8,3) = 0, −4, −8 at shifts 0, 1, 2 | §3.6, Q13 |
| Downlink aligned (OVSF works); uplink scrambled + power control (near–far) | §3.6, Q13 |
| CDMA = interference-limited "soft" capacity | §3.6, Q14 |
| TDMA = time; CDMA = code; OFDMA = frequency–time blocks | §3.7, Q14 |
| TDMA slot grid ideas reappear in DMR/TETRA | §3.1–3.2, Q15 |
