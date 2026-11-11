# Pack 05 cheatsheet — system architecture & interworking (every line cites the worksheet section)

Synthetic frames only; never live traffic. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Trunking = pooled channels, assigned per call (grant); conventional = fixed channel per group | §3.1 |
| Erlang B: B(E,0)=1; B(E,m)=E·B(m−1)/(m+E·B(m−1)) | §3.1, Q1 |
| 2 E on 4 pooled = 9.5 %; 4 × 0.5 E separate = 33.3 %; pooled minus a CC channel = 21.1 % | §3.1, Q1 |
| Trunking adds no spectrum; costs a control channel + grant latency + single point of failure | §3.1 |
| Control channel: continuous broadcast; idle radios park on it; inbound is slotted random access | §3.2, Q2 |
| P25 CC = TSBKs (96 → 196 bits); DMR Tier III logical CC; TETRA MCCH in timeslot 1 (VERIFY) | §3.2 |
| Registration = who I am / present; affiliation = which group I listen to; grant = group → channel | §3.3, Q2 |
| Affiliation exists so a call uses channels only at sites with members | §3.3, Q2 |
| A unit has one affiliation; affiliate() overwrites | §3.3, Q11 |
| Request outcomes: Granted, Joined, Queued, Preempted, Denied | §3.4, Q7 |
| Rule order: not affiliated → group already active (Joined) → free channel → emergency preempt → queue | §3.4, Q7, Q11 |
| Emergency queues ahead of normal calls; cannot preempt another emergency | §3.4, Q11 |
| Preemption victim = lowest group id among non-emergency (this pack's deterministic tie-break; real = priority levels, VERIFY) | §3.4, Q8 |
| Hang time: channel held after PTT release to catch the reply; trades capacity for latency | §3.4, §4(c), Q8 |
| Hang time can starve a queue if replies keep resetting it; fixes: max call time, queue-aware hang | §3.4, Q8 |
| Talk timeout stops a stuck PTT from holding a channel (implemented in pack 06) | §3.4 |
| Headers get stronger FEC: error = lost/misrouted/mis-decrypted call, no ARQ, no redundancy, cheap on short words | §3.5, Q6 |
| Voice errors: 20 ms glitch, concealed; strong FEC on the continuous stream would halve vocoder bits | §3.5, Q6 |
| Unequal error protection: FEC by consequence per bit | §3.5, Q6 |
| Late entry: receivers join any time and can't ask → LC (LDU1) and ES (LDU2) repeat every 360 ms | §3.5, Q5, Q6 |
| acquire_time = T/2 + p/(1−p)·T (+ 180 ms LDU to receive it); p=0 → 360 ms total | §3.5, Q5 |
| Halving the repeat rate halves overhead and doubles the mean wait | §3.5, Q5, Q6 |
| Uplink-limited: handheld ≪ tower power | §3.6, Q17 |
| Simulcast = downlink, many transmitters, same bits/frequency/time; needs GPS timing + delay equalisation | §3.6, Q3, Q17 |
| Voting = uplink, many receivers, comparator picks best frame; needs CRC and a quality metric | §3.6, Q4, Q17 |
| Radio delay 3.3356 µs per km | §3.6, Q3 |
| Symbol periods: P25p1 208.33 µs, P25p2 166.67 µs, TETRA 55.56 µs | §3.6, Q3 |
| ¼-symbol rule of thumb: 15.6 km (P25p1), 12.5 km (P25p2), 4.2 km (TETRA) (VERIFY) | §3.6, §4(b), Q3 |
| Voting all-lose probability = pⁿ (0.1 → 0.1, 0.01, 0.001, 0.0001); independence is optimistic | §3.6, Q4 |
| Vote: crc_ok only, fewest corrected errors, tie → lowest receiver id, none → −1 | §3.6, Q4 |
| Misaligned simulcast overlap creates a null where equal signals meet | §3.6, Q3, Q17 |
| OFB: keystream from the IV, ciphertext = plaintext XOR keystream; 1 bit error → 1 bit error | §3.7, Q14 |
| DES-OFB 56-bit (2⁵⁶ ≈ 20 h at 10¹² keys/s); AES-256 infeasible | §3.7, Q14 |
| ALGID, KID, MI travel in clear in the encryption sync; the voice is the secret | §3.7, Q14 |
| Same key + same MI twice → C1 XOR C2 = P1 XOR P2 (two-time pad) | §3.7, §4(d), Q14 |
| OTAR: traffic keys sent under a per-radio KEK from a KMF; KEK/KMF is the crown jewel | §3.7, Q14 |
| This pack implements no cipher; use vetted libraries | header, §3.7 |
| MCPTT: SIP signalling, floor control (TS 24.380), RTP voice (AMR-WB/EVS), TS 23.283 MC–LMR interworking (VERIFY) | §3.8 |
| IWF maps four things: identity, floor, voice, security | §3.8, Q15 |
| P25 unit ID = WACN 20 + SysID 12 + Unit 24 = 56 bits = 14 hex digits | §3.8, §4(e), Q9 |
| URI must carry all three fields (injective mapping); parse strictly | §3.8, Q9, Q13 |
| Floor mapping is a bijection over 8 events (PttPress↔FloorRequest … ChannelIdle↔FloorIdle) | §3.8, Q10 |
| Granted goes to the requester, Taken goes to listeners | Q10, Q13 |
| Transcoding adds latency, tandem-coding loss, DSP cost per call | §3.8, Q12, Q15 |
| 20 ms vs 30 ms frames: lcm 60 ms; 8 kHz → 160 vs 240 samples | §4(f), Q12 |
| Framing delay = in − gcd(in, out): 30→20 = 20 ms, 20→30 = 10 ms | §4(f), Q12 |
| E2E encryption breaks at a transcoder: the IWF is an endpoint | §3.8, Q16 |
| Options: trusted IWF (HSM), no transcoding, clear bridging by policy, keep sensitive groups LMR-only | Q16 |
| Trunked 6-channel fleet: 33.3 % vs 11 % (CC) vs 5.2 % (integrated CC) blocking | Q18 |
| Bugs: 32-bit shift, small buffers, partial-key compare, catch-all switch, lax parse | Q13 |
