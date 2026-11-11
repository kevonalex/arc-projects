# Pack 06/04 cheatsheet — MCPTT/MCX and the LMR IWF (every line cites the worksheet section)

Synthetic data only; never live traffic. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| GTP-U byte 0: version (7-5) = 1, PT (4), spare, E (2), S (1), PN (0); 0x30 = plain | §3.1, Q4 |
| Message type 0xFF = G-PDU; length counts bytes after the first 8 | §3.1, Q5 |
| Mandatory header 8 bytes; any of E/S/PN adds 4 optional bytes (seq, N-PDU, next-ext) | §3.1, Q5 |
| Extension header: length in 4-byte units, next type 0 ends; 0x85 = PDU session container (VERIFY) | §3.1, Q5 |
| header_len = 8 (+4) + Σ ext; plain 8 / seq 12 / 1 ext 16 / 2 ext 20 | §3.1, Q5 |
| Parser defences: bounds, version/PT, length check, ext validity, status not UB | §3.1, Q6 |
| Zero-length extension = infinite loop → BadExtension | §3.1, Q6 |
| Build: 30 FF len TEID(BE) [seq 00 00] payload; seq adds 4 to length | §3.2, Q7 |
| parse(build(x)) == x is the first test to write | §3.2 |
| numerology_row: occupied = PRBs·12·SCS; 20 MHz/μ0 → 106 PRB 19.08 MHz; μ1 → 51, 18.36; 100 MHz/μ3 → 66, 95.04 | §3.3, Q8 |
| Unknown → −1 sentinel / nullopt, not 0 or exception | §3.3, Q8, Q9 |
| Rel-13 MCPTT; Rel-14 MCData/MCVideo; Rel-15 LMR IWF TS 23.283 (VERIFY) | §3.4, Q2 |
| SIP/IMS signalling; RTP media; MBCP floor (TS 24.380) | §3.4, Q1 |
| QCI 65 voice / 69 signalling (VERIFY); eMBMS broadcast; ProSe off-network | §3.4, Q1 |
| QoS = bandwidth/delay; floor control = who may talk | §3.4, Q3 |
| IWF maps floor, identity, voice, security | §3.5, Q13 |
| PttPressed→FloorRequest, PttReleased→FloorRelease, Emergency→RequestEmergency, ChannelBusy→None | §3.5, Q10 |
| Granted→TxPermit, Denied/Taken→BusyTone, Revoked→TxTerminate, Idle→ClearChannel | §3.5, Q10 |
| Revoked means transmitter is on air: terminate immediately | §3.5, Q10 |
| Group URI model sip:tg<ID>@domain; strict parse (digits, ≤ 65535, `@`) | §3.5, Q12 |
| Talkgroup map is one-to-one; duplicates refused, never overwritten | §3.5, Q11 |
| Transcoding adds latency + tandem loss; floor/identity bugs are worse | §3.5, Q13 |
| srsRAN: read upstream, don't copy (AGPL-3.0, VERIFY); write own from spec | §3.6, Q14 |
| No live traffic; no decryption; test with synthetic streams | header, Q16 |
