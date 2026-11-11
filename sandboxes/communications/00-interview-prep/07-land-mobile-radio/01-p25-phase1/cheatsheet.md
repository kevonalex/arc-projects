# Pack 01 cheatsheet — P25 Phase 1 (every line cites the worksheet section)

Synthetic frames only; never live public-safety traffic (worksheet header). **VERIFY** items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| FDMA, 12.5 kHz, one conversation per channel | §3.1 |
| C4FM: 4800 sym/s × 2 bit/sym = **9600 bit/s** gross | §3.1 |
| Dibit map `01=+3 00=+1 10=−1 11=−3` (Gray; VERIFY) | §3.1 |
| Constant envelope → non-linear PA → battery life | §3.1 |
| 9600 bit/s is gross; speech is 4400 bit/s | §3.1, §3.6 |
| Sync = 48 bits `0x5575F5FF77FF` (VERIFY); only ±3 symbols | §3.2 |
| False sync (m=4): 213 053/2⁴⁸ = 7.57e-10 per window; ≈ 76 h at 4800/s | §3.2, Q4 |
| NID = 64 bits = 12 NAC + 4 DUID → BCH(63,16) 47 parity + 1 parity | §3.3 |
| BCH: roots α¹…α²² ⇒ t=11, d≥23; cosets 6+6+6+6+3+6+6+6+2 = 47 | §3.3 |
| Encode = shift data by 47, append remainder mod g | §3.3, §4, Q10 |
| Extra parity bit → d=24 → correct 11 / detect 12 | §3.3, Q5 |
| NAC is a filter (like CTCSS), **not** security; 0xF7E "any" (VERIFY) | §3.3 |
| DUIDs: HDU0 TDU3 LDU1=5 TSBK7 LDU2=A PDU C TDULC F (VERIFY) | §3.3 |
| Status symbol = 1 per 35 data symbols; index k is status iff (k+1)%36==0 | §3.4, §4 |
| Strip status symbols BEFORE FEC | §3.4 |
| Call = HDU, (LDU1 LDU2)*, TDU/TDULC | §3.5 |
| LDU1+LDU2 = 360 ms superframe = 18 IMBE frames | §3.5 |
| LDU = 1728 bits = 864 symbols = 180 ms | §3.6 |
| LDU budget 48+64+1296+240+32+48 = 1728 | §3.6, Q3 |
| IMBE frame: 4×Golay(23,12)+3×Hamming(15,11)+7 = 144 bits; info 88; 20 ms | §3.6 |
| IMBE: 7200 bit/s on air, 4400 bit/s speech | §3.6, Q3 |
| Headers get stronger FEC: one error ruins a whole frame; voice errors are local | §3.6, Q2 |
| LC: 72 → RS(24,12) → 144 → Hamming(10,6) → 240 (VERIFY) | §3.6 |
| ES: 96 → RS(24,16) → 144 → Hamming(10,6) → 240 (VERIFY) | §3.6 |
| LDU1 = Link Control (TGID 16b, source 24b, service options, MFID) | §3.7, Q11 |
| LDU2 = Encryption Sync (MI 72b, ALGID 8b, KID 16b); ALGID 0x80 = clear | §3.7, Q11 |
| LC/ES repeat for **late entry**; worst-case 540 ms, mean 360 ms | §3.7, Q7 |
| ALGID = algorithm, KID = key name; keys never in clear over air | §3.7 |
| TSBK: 96 bits (1+1+6+8+64+16 CRC) → rate-½ trellis → 196 bits | §3.8 |
| FEC corrects, CRC checks — need both | §3.8 |
| CAI = air; ISSI = RFSS↔RFSS (SIP+RTP); CSSI = console↔RFSS (SIP+RTP); DFSI = fixed station | §3.9 |
| SIP = signalling, RTP = media; ISSI never touches a radio | §3.9, Q12 |
| NAC, status symbols, FEC are regenerated at the destination, not carried | Q12 |
| Stack pipeline: sync → NID → DUID dispatch → strip status → FEC → structs → state machine | §3.10, Q13 |
| Simulcast uses CQPSK/LSM-compatible reception (see pack 05) | §3.10 |
| Pitfalls: unmasked window, `<` vs `<=`, baud vs bit rate, status phase, shift-before-cast | Q8, Q9 |
| Test with synthetic vectors, round-trip, sanitizers; never live traffic | Q14, header |
