# Capstone M2 cheatsheet — μ-law, RTP, jitter buffer, multicast (every row cites the worksheet)

Synthetic signals only. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| μ-law: bias 0x84, clip 32635, 3-bit exponent + 4-bit mantissa, byte inverted | §3.1, Q1 |
| encode(0) = 0xFF, encode(32767) = 0x80, encode(−32768) = 0x00, encode(−1) = 0x7F | §3.1 |
| decode(0x80) = 32124, decode(0xFF) = decode(0x7F) = 0 | §3.1 |
| encode(1000) = 0xCE → 988; encode(100) = 0xF2 → 104 | §3.1, Q1 |
| Step size per segment = 8 << e (8 … 1024); error ∝ level | §3.1, Q2 |
| μ-law = 64 kbit/s, ≈ 13-bit linear quality; not a vocoder; PCMU = PT 0 (A-law PT 8) | §3.1 |
| RTP fixed header 12 B: V2 P X CC4 / M PT7 / seq16 / ts32 / ssrc32, big-endian | §3.2, Q3 |
| Marker = first packet of a talk-spurt (key-up); seq = order/loss, ts = timing | §3.2 |
| Parse with checks after every variable field: CSRC 4·CC, ext 4 + 4·len, pad count | §3.2, Q4, Q5 |
| 16-bit seq wraps in 65536 × 20 ms = 21.8 min | §3.3 |
| newer(a, b) ⇔ (a − b) mod 65536 ∈ 1…32767; 0x8000 is not newer | §3.3, Q6 |
| Unwrap with signed 16-bit delta; stragglers go backwards | §3.3, Q6 |
| expected = highest − first + 1; lost = expected − received (may be < 0); fraction clamped ≥ 0 | §3.3, Q7 |
| Jitter D = (Rj − Ri) − (Sj − Si); J += (abs(D) − J)/16; first update records only | §3.4, Q8 |
| Example D = 0, 80, −80, 0 → J = 0, 5, 9.6875, 9.08203125 (1.1353 ms) | §3.4, Q8 |
| Constant delay ⇒ zero jitter; 1/16 ≈ 320 ms time constant | §3.4, Q9 |
| Buffer: Late / Duplicate / TooEarly; concealed = repeat previous (or 0xFF silence at start) | §3.5, Q10 |
| Start playout when `target` frames stored, at the smallest seq | §3.5 |
| Target = ceil(k·jitter/ptime), k = 3, floor 2 frames | §3.5, Q15 |
| Deeper buffer = more latency, fewer late drops | §3.5, §4(b) |
| Demo: jitter ≈ 15 ms; target 1 → 49/100 played; target ≥ 2 → 94 played, 0 late | §4(b), Q15 |
| Timestamp step = ptime × clock: 160 at 20 ms/8 kHz | §3.6, Q11 |
| IP rate = (payload + 40) × 8 / ptime: 96 / 80 / 72 kbit/s at 10 / 20 / 40 ms | §3.6, Q11 |
| Ethernet adds 38 B: 95.2 kbit/s at 20 ms | §3.6, Q11 |
| IPv4 multicast = 224.0.0.0/4; MAC = 01:00:5E + low 23 bits of the group | §3.7, Q12 |
| 5 bits dropped ⇒ 32 groups share one MAC (239.1.1.1 vs 239.129.1.1) | §3.7, Q12 |
| TG N → 239.192.(N>>8).(N&255); TTL 1; IGMP snooping | §3.7, Q17 |
| Why UDP: deadline, head-of-line blocking, multicast; RTP adds ts/seq/SSRC | Q13 |
| Why 20 ms: speech frame, overhead vs latency vs loss granularity | Q14 |
| M2 → M4: loss, jitter, concealed, late → CallStats; marker → reset | Q18 |
