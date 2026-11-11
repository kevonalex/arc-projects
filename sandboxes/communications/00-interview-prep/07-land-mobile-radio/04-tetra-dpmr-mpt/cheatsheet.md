# Pack 04 cheatsheet — TETRA, dPMR, MPT 1327 (every line cites the worksheet section)

Synthetic frames only; never live traffic. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Common yardstick: Hz per call → 6.25 kHz (DMR 12.5/2, P25p2 12.5/2, TETRA 25/4, dPMR 6.25/1) | §3.1, Q1 |
| TETRA: 25 kHz, π/4-DQPSK, 18 000 sym/s × 2 = 36 000 bit/s gross | §3.2 |
| Slot = 255 symbols = 510 bits = 14.167 ms | §3.2, Q3 |
| Frame = 4 slots = 56.667 ms; multiframe = 18 frames = 1.02 s; hyperframe = 60 mf = 61.2 s | §3.2, Q3 |
| Numbering is 1-based (TS 1–4, frame 1–18, MF 1–60) (VERIFY) | §3.2, Q7 |
| Frame 18 = control frame (5.6 % cost, guaranteed signalling) | §3.2, Q2 |
| Normal burst: 432 user bits + 78 overhead = 510 (VERIFY) | §3.3, Q3 |
| TCH/7.2 = 432 × 17 / 1.02 = 7200 bit/s per slot; 28 800 per carrier; 80 % of gross | §3.3, Q4 |
| ACELP 137 bits / 30 ms = 4.567 kbit/s; 2 frames per slot = 60 ms | §3.3 |
| 17 × 60 ms = 1020 ms = multiframe → why codec frame is 30 ms | §3.3, Q2 |
| Gross spectral efficiency TETRA 1.44 vs P25p2 0.96 vs DMR/P25p1 0.768 | §3.3, Q4 |
| π/4-DQPSK: phase steps ±π/4, ±3π/4; linear PA needed | §3.4 |
| Logical channels BSCH, BNCH, AACH, SCH/F/HD/HU, STCH, TCH; MCCH in TS1 (VERIFY) | §3.5 |
| TETRA colour code = scrambler seed (30-bit extended code), implicit; wrong CC ⇒ all CRCs fail | §3.5, Q6 |
| DMR colour code = explicit field; wrong CC ⇒ visible mismatch | Q6 |
| Layers: PHY, MAC, LLC, MLE, CMCE (calls/floor/SDS), MM (registration/auth/group attach) | §3.6, Q13 |
| Each layer hides one volatility: radio, link, mobility, services, subscriber | §3.6, Q13 |
| Example PDUs: MAC-RESOURCE, BL-ACK, D-NWRK-BROADCAST, U-TX DEMAND/D-TX GRANTED, U-LOCATION UPDATE DEMAND | §3.6, Q8 |
| Floor control in TETRA = CMCE (U-TX DEMAND → D-TX GRANTED / D-TX CEASED) | §3.6, Q8 |
| ITSI = MCC 10 + MNC 14 + SSI 24 = 48 bits (VERIFY) | §3.6 |
| TEA1–TEA4; 2023 TEA1 findings → algorithm & key mgmt matter (VERIFY) | §3.6 |
| dPMR: 6.25 kHz, 4FSK 2400 sym/s × 2 = 4800 bit/s, FDMA, 0.768 bit/s/Hz | §3.7, Q5 |
| dPMR = frequency-division twin of ONE DMR slot (4800 bit/s, 6.25 kHz per call) | §3.7, Q14 |
| FDMA: simple radios, no slot timing; cost: 1 TX per call, tight oscillators (±1 ppm = 450 Hz at 450 MHz = 7.2 %) | §3.7, Q5 |
| MPT 1327: 1200 bit/s FFSK; tones 1200/1800 Hz; spacing = baud/2 = 600 Hz (MSK) | §3.8, Q5 |
| FFSK tones: 1.0 and 1.5 cycles per bit | §3.8, §4(e) |
| MPT codeword = 64 bits = 48 info + 15 check + 1 parity = 53.33 ms; 900 bit/s info | §3.8, Q5 |
| g(x) = x¹⁵+x¹⁴+x¹³+x¹¹+x⁴+x²+1 = 0xE815 (VERIFY; real one complements checks) | §3.8, Q10 |
| CRC of degree r detects all bursts ≤ r (g(0)=1 ⇒ gcd(g, x^k) = 1) | §3.8, Q10 |
| SYNC 0xC4D7 (control), SYNCT 0x3B28 = ~SYNC (traffic) (VERIFY) | §3.8 |
| Bugs: symbol rate vs bit rate, 17 vs 18 traffic frames, 0- vs 1-based, tone spacing | Q9 |
| Gateway TETRA↔P25: ID tables, 30 ms vs 20 ms (lcm 60 ms) re-framing, floor mapping, no cipher mapping | Q12 |
| FDMA vs TDMA choice: repeater cost vs timing complexity | Q14 |
| MPT still deployed: bit-exact codewords + IWF for migration | Q15 |
