# Capstone M1 cheatsheet — framing, CRC, FEC (every row cites the worksheet)

Toy format, synthetic data only. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Frame = sync 24 + header 46 + body 2268 = **2338 bits** | §3.1, Q1 |
| Body = (160 B + 2 B CRC) × 2 nibbles × 7 bits = 324 Hamming words | §3.1, Q1 |
| Efficiency 1280 / 2338 = 54.7 %; raw rate 116.9 kbit/s | §3.1, Q1 |
| CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflect, no final XOR; "123456789" → 0x29B1 | §3.2 |
| CRC(0x00) = 0xE1F0 (hand trace in the table) | §3.2 |
| XOR byte into the HIGH byte; shift-left; xor poly only if top bit was 1 | §3.2, Q2 |
| CRC detects (bursts ≤ 16, odd weights), never corrects; random miss ≈ 2^-16 | §3.2, Q3 |
| Init 0xFFFF so leading zeros are visible | §3.2, Q2 |
| Golay(23,12,7): corrects 3, detects 6; g = 0xC75; encode(1) = 0xC75; encode(0xABC) = 0x55E11E | §3.3 |
| Perfect: 1 + 23 + 253 + 1771 = 2048 = 2^11 | §3.3, Q7 |
| Perfect code ⇒ 4+ errors give a confident wrong word; no failure signal | §3.3, Q7 |
| Hamming(7,4): layout p1 p2 d1 p4 d2 d3 d4; encode(0xB) = 0x55, encode(0x6) = 0x33, encode(0xC) = 0x61 | §3.4, Q4 |
| Syndrome (as a binary number) = position of the single error | §3.4, Q5 |
| Two errors in one Hamming word → miscorrection, `corrected = true` | §3.4, Q6 |
| Header word: version 2 / type 2 / talkgroup 8 / seq 12 = 24 bits; example 0x6A59AB | §3.5 |
| CRC covers header + payload (so a Golay miscorrection is still caught) | §3.5, Q10 |
| Parse order: Truncated → SyncFail → BadHeader → (Hamming) → CrcFail | §3.5, Q14 |
| Sync 0x02B31E, worst sidelobe 3 (peak 24), tolerance 3 mismatches | §3.6, Q12 |
| False sync per offset 2325 / 2^24 = 1.39×10^-4 ≈ 0.32 per 2338 offsets | §3.6, Q12 |
| find_sync = fewest mismatches, ≤ tolerance, first on ties | §3.6 |
| At BER 10^-3: header unprotected fails 2.4 %; Golay word ~9×10^-9; Hamming word 2.1×10^-5; frame CrcFail ≈ 0.68 % | §4(c), Q8 |
| Header gets stronger FEC: higher consequence, tiny cost, CRC backstop | Q9 |
| 12-bit seq wraps at 4096 × 20 ms = 81.92 s | Q13 |
| Sync flips don't count as corrections; every other single flip counts as exactly 1 | Q15 |
| Real air interfaces add vocoder, interleaving, scrambling, link control, encryption | Q16 |
