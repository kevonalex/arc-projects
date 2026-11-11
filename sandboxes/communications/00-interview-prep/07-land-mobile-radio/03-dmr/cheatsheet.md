# Pack 03 cheatsheet — DMR (every line cites the worksheet section)

Synthetic frames only; never live traffic. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| 12.5 kHz, 4FSK, 4800 sym/s × 2 = 9600 bit/s gross | §3.1 |
| Dibit map 01=+3 00=+1 10=−1 11=−3 (Gray; VERIFY) | §3.1 |
| 2-slot TDMA: two logical channels (TS1/TS2), 30 ms slots | §3.1 |
| Slot = 288 bits (30 ms); burst = 264 bits (27.5 ms); gap = 24 bits (CACH down / guard up) | §3.2, §4(b) |
| Burst = 108 + 48 + 108 | §3.3 |
| Data burst = 98 + 10 slot-type + 48 + 10 slot-type + 98 | §3.3 |
| Voice burst = 216 voice bits = 3 × 72-bit AMBE+2 frames; frame 1 straddles the middle | §3.3, §3.4, §4(c) |
| Sync in the middle (midamble): training ≤ 108 bits from any data bit | §3.3, Q1 |
| 4 sync words: BS voice 755FD7DF75F7, BS data DFF57D75DF5D, MS voice 7F7D5DD57DFD, MS data D5D7F77FD757 (VERIFY) | §3.3 |
| Sync says WHO sent (BS/MS) and WHAT (voice/data); BS voice vs data differ in 24/48 bits | §3.3 |
| Frames per burst = slot repeat (60 ms) ÷ frame (20 ms) = 3 | §3.4, Q2 |
| Superframe A–F = 6 × 60 ms = 360 ms = 18 voice frames | §3.4, §4(d) |
| A = voice sync; B–E = embedded LC fragments (4×32 = 128 bits ← 77-bit LC+CRC); F = other | §3.4 |
| EMB = CC4 + PI1 + LCSS2 + QR(16,7,6) parity 9 (VERIFY) | §3.4 |
| Late entry: worst 540 ms, mean 360 ms, best 180 ms (first-fragment-first rule) | §3.4, Q7 |
| Slot type = CC (4) + data type (4) → Golay(20,8) → 20 bits | §3.5 |
| Colour code = co-channel filter (16 values), NOT security | §3.5, Q15 |
| Data types: 0 PI, 1 VoiceLC hdr, 2 Term+LC, 3 CSBK, 4 MBC hdr, 5 MBC cont, 6 DataHdr, 7 R½, 8 R¾, 9 Idle, 10 R1 (VERIFY) | §3.5 |
| Golay(23,12) perfect: 1+23+253+1771 = 2048 = 2¹¹ syndromes | §3.5, Q4 |
| (20,8) = shorten + overall parity: corrects 3, detects 4; ≈ 33 % of noise accepted | §3.5, Q4, Q5 |
| Worked slot type CC=1, DT=1 → 0x11B8C | §4(a) |
| 96 info bits = 72 LC + 24 RS(12,9) parity (or CSBK 80 + CRC16) | §3.6 |
| BPTC(196,96): 9×11 info → 13×15 = 195, +1 reserved = 196 = 98 + 98 | §3.6 |
| Row Hamming(15,11), column Hamming(13,9); product d = 9 (VERIFY decoder) | §3.6 |
| Interleave (a·181) mod 196; inverse 13; gcd = 1 → permutation | §3.6, Q8 |
| Spread: tx neighbours are 13 apart in matrix; 15 apart become adjacent | Q8 |
| Headers get stronger FEC: BPTC rate 0.49, slot type 0.4 vs voice 0.68 | §3.7 |
| LC: PF R FLCO FID + svc opts 8 + dst 24 + src 24; IDs are 24-bit | §3.8 |
| Voice share of gross: DMR 75 % vs P25p2 60 %; both 2 calls per 12.5 kHz | Q4 |
| Burst bugs: contiguous frame 1, LSB-first pack, 5 vs 6 bursts, nibble swap, 30 vs 27.5 ms | Q9 |
| Flywheel: timing predicted, sync loss declared after several misses | Q6 |
| Gateway DMR↔P25: ID tables (TGID 16 vs 24), stateful LC regeneration, transcoding | Q13 |
