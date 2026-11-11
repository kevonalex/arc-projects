# Pack 06 cheatsheet — builds (every line cites the worksheet section)

Synthetic frames only; never live traffic; no ciphers. OP25 / dsd-fme / SDRTrunk = optional study, never copy. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Method: contract → hand example → test first → smallest version → exhaustive/property tests → sanitizers → seeded data | §3.1 |
| Golay g(x) = x¹¹+x¹⁰+x⁶+x⁵+x⁴+x²+1 = 0xC75 (weight 7) | §3.2, §4(a), Q2 |
| Systematic encode: info·x¹¹ plus remainder mod g; encode(1) = g = 0xC75 | §3.2, §4(a), Q2 |
| encode(2) = 0x149F (weight 8); encode(3) = x·g = 0x18EA | §4(a), Q2 |
| x¹¹…x¹⁵ mod g = 0x475, 0x49F, 0x54B, 0x6E3, 0x1B3 | §4(a), Q2 |
| Syndrome of codeword + error = syndrome of the error alone | §3.2 |
| Perfect: 1+23+253+1771 = 2048 = 2¹¹ syndromes; 4096 × 2048 = 2²³ | §3.2, Q1 |
| Distinct patterns of weight ≤ 3 share no syndrome (difference would be a codeword of weight ≤ 6 < 7) | §3.2, Q1 |
| Perfect code never reports failure: 4 errors miscorrect (all 8855) | §3.2, Q1 |
| Weight distribution 1, 253, 506, 1288, 1288, 506, 253, 1 (weights 0, 7, 8, 11, 12, 15, 16, 23) | Q4 |
| Golay24 = codeword23 << 1 + overall parity (even weight); d_min 8 | §3.2, Q3 |
| Golay24 decode: total = e + m; ≤ 3 accept, ≥ 4 uncorrectable (m uses the CORRECTED word's parity) | §3.2, Q3, Q13 |
| 4 errors: (3,1) gives e=3, m=1; (4,0) gives c′ at distance 3, d(c,c′)=7 odd ⇒ parity differs ⇒ m=1 | Q3 |
| Parser order: find sync (raw) → strip status (relative to frame start) → pack NID → candidate match → dispatch | §3.3, Q5 |
| Status symbol iff (rel + 1) % 36 == 0; data index k → air index k + k/35 | §3.3, §4(b), Q5 |
| First status after 35 symbols > 24-symbol sync ⇒ raw sync search is safe | §3.3, Q5 |
| LDU: 24+32+784 = 840 data symbols; 24 status; 864 air = 1728 bits = 180 ms | §3.3, §4(b), Q5 |
| symbols_to_bits is MSB-first: {1,2,3,0} → 0x6C | §4(b), Q7 |
| Sync false-accept ≈ Σ C(48,k)/2⁴⁸: tolerance 4 → 7.6·10⁻¹⁰; tolerance 8 → 1.7·10⁻⁶ | §3.3, Q16 |
| NID candidate matching = bounded-distance decoding because d_min(64-bit NID code) ≥ 24 (13 > 11) | §3.3, Q6 |
| Cost: 7 encodes (one NAC) or 28 672 (all NACs) | Q6 |
| NID false accept at tolerance 11: 3.5·10⁻⁷ (one NAC), 1.4·10⁻³ (all NACs); at tolerance 5: 3.2·10⁻¹² / 1.3·10⁻⁸ | Q6 |
| Tie in candidates ⇒ not ok (ties need distance ≥ 12) | Q6 |
| False sync ⇒ retry at pos + 1; truncated ⇒ nullopt, never zero-pad; fixed-size frames truncate trailing data | §3.3, Q7, Q8 |
| DUID dispatch: HDU header, LDU1/2 voice, TSBK control, PDU data, TDU/TDULC terminator; LDU payload 784 symbols (VERIFY) | §3.3 |
| DMR slot = 24 gap + 264 burst = 288 bits = 30 ms; burst = 108 + 48 + 108 | §3.4, Q9 |
| Voice = 3 × 72 = 216 = 2 × 108; frame 1 straddles the 48-bit middle; middle = sync or embedded | §3.4, Q9 |
| Middle starts at slot bit 132; phase = (sync bit − 132) mod 288 | §3.4, §4(c), Q9 |
| Phase search: most sync hits over 288 phases, ties → smallest phase | §3.4, Q10 |
| False hit per (phase, slot) ≈ 3.0·10⁻⁹ (4 words, tolerance 4); 3456 trials ≈ 1·10⁻⁵ | Q10 |
| Alignment needs one sync burst: ≤ one superframe (360 ms) + 30 ms | Q10 |
| Weakness: single sync ties with a random false hit; require ≥ 2 hits or validate | Q10 |
| Floor FSM: Idle / Active / Hang; events press, release, tick; talker 0 = nobody | §3.5 |
| Floor table: Idle press → Granted; Active duplicate → none; other press → Preempted (emergency over normal) or Denied | §3.5, §4(d), Q11 |
| Hang press = Granted without new channel grant; hang expiry → Idle | §3.5, Q11 |
| Every grant/preempt resets the talk timer; emergency flag ends with the call | §3.5, Q11 |
| One transition per tick; big ticks drop time | §3.5, Q12 |
| Same-instant press vs hang expiry: input before timer avoids a needless grant | Q12 |
| Stuck PTT re-acquires from Hang → add lock-out until release | §3.5, Q12, Q17 |
| Golay bugs: loop bound, mask, table weights, ≥ 4 threshold, corrected-word parity | Q13 |
| Parser bugs: strip before sync, status off-by-one, LSB-first packing, first-candidate, zero-padding | Q7 |
| Test bench: generator → noise → parser → FEC → call control, with counters | Q14 |
| BER 7 %: sync alone loses ≈ 24 % at tolerance 4 (P(≤4 errors in 48) = 0.756) | Q16 |
| Testing without live traffic: golden vectors, exhaustive, properties, seeded noise, fuzz, sanitizers, differential, model-based | §3.1, Q18 |
| OP25 / dsd-fme / SDRTrunk: study structure, do not copy code, do not receive live traffic | header, §6 |
