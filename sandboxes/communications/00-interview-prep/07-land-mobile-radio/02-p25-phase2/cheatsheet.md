# Pack 02 cheatsheet — P25 Phase 2 (every line cites the worksheet section)

Synthetic frames only; never live public-safety traffic. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Phase 1: 12.5 kHz, 9600 bit/s gross, 7200 bit/s voice per call → 1 call | §3.1 |
| TDMA = users take turns in time on one frequency | §3.2 |
| Phase 2: 6000 sym/s × 2 = **12 000 bit/s** gross | §3.2 |
| Slot 30 ms = 180 symbols = 360 bits (VERIFY) | §3.2, Q3 |
| 12 slots = 360 ms superframe; 6 slots per logical channel | §3.2, Q3 |
| Capacity doubles because the vocoder halves: IMBE 7200 → AMBE+2 3600 bit/s | §3.3, Q1 |
| 2 × IMBE (14 400) cannot fit in 12 000 | §3.3, Q4 |
| Gross rate up only 25 %; spectral efficiency 0.768 → 0.96 bit/s/Hz | §3.3, Q4 |
| Overhead share 25 % → 40 % (guard, ramp, sync, signalling) | §3.3, Q4 |
| Voice share of gross: 75 % → 60 % | §3.3, Q4 |
| Duty cycle 50 % → battery / heating benefit | §3.3, Q4 |
| TDMA ≠ doubled bit rate | §3.3 |
| DQPSK: info in phase CHANGE; table 00:+π/4 01:+3π/4 10:−π/4 11:−3π/4 (VERIFY) | §3.4, §4 |
| π/4 family: steps are odd multiples of π/4 → two alternating constellations | §3.4, §4 |
| Differential decode: constant phase offset cancels | §3.4, Q5 |
| Frequency offset adds δ = 2πf/R_s per symbol; fails beyond R_s/8 = 750 Hz | Q5 |
| Downlink H-DQPSK linear (base can afford it, spectral efficiency) | §3.4, Q2 |
| Uplink H-CPM constant envelope (handheld non-linear PA) | §3.4, Q2 |
| 4V burst = 4 × 72 bits AMBE+2 = 288 of 360; 72 overhead (VERIFY) | §3.5, Q3 |
| Teaching pattern per channel: 4V 4V 4V 4V 2V SACCH = 18 frames = 360 ms | §3.5, Q7 |
| FACCH steals a voice burst for urgent signalling; SACCH is slow/regular | §3.5 |
| TDMA latency: buffer 4 frames (80 ms) + wait for slot (≤ 60 ms) | §3.5, Q7 |
| ESS repeated in voice bursts for late entry (VERIFY split) | §3.6 |
| Phase 1 control channel can serve Phase 2 voice channels (VERIFY) | §3.7, Q11 |
| IMBE/AMBE+2 are licensed vocoders; mixed calls need transcoding (lossy tandem) | §3.7, Q11 |
| Slot math: slot_abs = sym/180; sf = slot_abs/12; slot = slot_abs%12; channel = slot%2 | §3.8, §4, Q6 |
| Use a 64-bit symbol counter; uint32 wraps in 8.3 days and slips 76 symbols | §3.8, Q8 |
| Bugs: wrong rate constant, ms/s units, ×2 bits, `/` vs `%`, counter width | Q8 |
| Port Phase 1 → 2: schedule, bursts, logical channels, vocoder, modulation, dual mode | Q13 |
| Calls that fit = floor((gross − overhead) / per-call) | Q4, Q9 |
