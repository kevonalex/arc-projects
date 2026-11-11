# Capstone M2 — Answers (μ-law, RTP, jitter buffer, multicast)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic signals only. **VERIFY** = check the RFC/standard before quoting.

For the **Story** questions (Q13–Q17) there is no single right answer. A *model answer* is given with the points a strong answer must hit; rewrite it in your own words with your own numbers.

---

## Q1 — μ-law encode 1000 and 100

**Answer.** `pcm = 1000`: `1000 + 132 = 1132 = 0x46C = 100 0110 1100b`; highest set bit is bit 10 → exponent `10 − 7 = 3`; mantissa `(1132 >> 6) & 15 = 17 & 15 = 1`; byte `~(0x30 | 0x01) = ~0x31 = 0xCE`. Decode `0xCE`: `~ = 0x31` → e = 3, m = 1 → `((1<<3) + 132) << 3 − 132 = 140×8 − 132 = 988`. Error 12 (1.2 %).
`pcm = 100`: `100 + 132 = 232 = 1110 1000b` → highest bit 7 → exponent 0; mantissa `(232 >> 3) & 15 = 29 & 15 = 13`; byte `~0x0D = 0xF2`; decodes to `(13×8 + 132) − 132 = 104` (error 4, 4 %).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Forgetting the bias → exponent from 1000 directly | Bias looks like noise. | Bias makes the segments line up; 1000 alone gives exponent 2, mantissa 15: a different code. |
| Not inverting the byte | Looks "done". | G.711 transmits the one's complement (`0xFF` = zero). |

## Q2 — Why companding

**Answer.** Hearing judges *relative* error; uniform 8-bit gives a constant absolute step, so quiet speech has a poor SNR and loud speech a wasteful one. Companding makes steps proportional to the level, giving roughly constant percentage error across the range. Step size in this pack's 16-bit units: segment 0 = **8**, segment 7 = **1024** (128:1). A quiet sample near 100 has error ≤ 4 (≈ 4 %); a loud sample near 20 000 has error ≤ 512 (≈ 2.5 %). Same relative quality, with 8 bits instead of 13+.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "It compresses the data rate by modelling speech" | Sounds like a codec. | It only drops 16 → 8 bits by non-uniform quantisation. |

## Q3 — Parse the hex

**Answer.** `80 80 00 10 00 00 0A 00 12 34 56 78 | FF FF`. Byte0 `0x80` → V=2, P=0, X=0, CC=0. Byte1 `0x80` → M=1, PT=0 (PCMU). Seq `0x0010` = **16**. Timestamp `0x00000A00` = **2560** = 16 × 160 → 16 frames of 20 ms. SSRC `0x12345678`. Payload `FF FF` (μ-law silence). Seq 16 and 16 frames of timestamp are consistent (one 160-tick step per packet); the marker says "first packet of a talk-spurt" — after silence the timestamp would normally jump *more* than the sequence number. (Starts are random per RFC 3550; VERIFY.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Seq = `0x10 00`? little-endian | Host byte order. | RTP is big-endian (network order). |
| PT = `0x80` | Reading the whole byte. | Top bit is the marker; PT is the low 7 bits. |

## Q4 — The 35-byte packet

**Answer.** Fixed header 12 bytes; **CSRCs** `4 × 2 = 8` → offset 20; **extension** (X=1): 4-byte header (`BE DE 00 01`) + `1 × 4` data bytes = 8 → offset **28**; **padding** (P=1): last byte `0x04` = 4 padding bytes (including itself), so payload ends at `35 − 4 = 31`. Payload length = **3** (`AA BB CC`).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Payload offset 12 | Forgetting optional parts. | CC and X are in the first byte for a reason. |
| Padding count excluded (len 4) | Count doesn't include itself? | The count includes the count byte itself. |

## Q5 — Safe parsing

**Answer.** (1) `len ≥ 12` (else read past a short packet); (2) `version == 2`; (3) `12 + 4·CC ≤ len` (CC=15 with nothing following); (4) if X: the 4-byte extension header fits, *then* `4 × length` words fit (length 9 in a 16-byte packet), computed in `size_t` to avoid overflow; (5) if P: `count ≠ 0` and `count ≤ remaining` (count 9 on a 2-byte payload; count 0 would never terminate/underflow). Security: the data is unauthenticated network input; an out-of-bounds read leaks memory or crashes the gateway, and a remote sender triggers it with one packet. ASan/UBSan and fuzzing catch this class.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Check the total length once" | One check feels sufficient. | Each variable-length field extends the offset; check after each. |
| "Trusted LAN, so no checks" | Multicast stays local. | Any host on the LAN can send, and misbehaving devices do. |

## Q6 — Serial arithmetic

**Answer.** `seq_newer(100, 65000)`: `(100 − 65000) mod 65536 = 636` ∈ 1..32767 → **true**. `seq_newer(0x8000, 0)`: difference 32768, outside 1..32767 → **false** (ambiguous by convention). `seq_newer(65535, 0)`: difference 65535 → **false** (65535 is *older* than 0). Unwrapper: `65534, 65535, 65536, 65537`, then straggler `65535` → **65535** (delta −2), then `2` → **65538**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| `a > b` | Obvious comparison. | Wrong across the wrap (0 is newer than 65535). |
| 0x8000 is newer | Symmetric intuition. | Exactly half the space is ambiguous; RFC 1982 leaves it undefined, we say not newer. |

## Q7 — Loss

**Answer.** `{10,11,12,15,16}`: expected = `16 − 10 + 1 = 7`, received 5, lost **2**, fraction `2/7 = 28.6 %`. `{65534,65535,0,1,1}`: extended 65534…65537, expected = `65537 − 65534 + 1 = 4`, received 5, lost **−1**, reported fraction **0** (clamped). Negative because the duplicate counts as received but not as expected.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Expected = 5 (count packets) | "Expected = received". | Expected comes from the sequence span, not from what arrived. |
| Clamp `lost` to 0 in storage | Negative looks like a bug. | RFC 3550 keeps the signed value (it signals duplicates); only the *fraction* is clamped. |

## Q8 — Jitter estimator

**Answer.** After updates: 1: records transit, J = 0. 2: D = 0 → J = 0. 3: D = +80 → `0 + 80/16 = 5`. 4: D = −80 → `5 + (80 − 5)/16 = 9.6875`. 5: D = 0 → `9.6875 − 9.6875/16 = 9.08203125`. In ms: `9.08203125 / 8 = 1.1353 ms`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| J = 80 / 8 = 10 ms | Simply "10 ms late". | Jitter is a smoothed mean of abs(D), not the peak. |
| Skipping the abs() (use D) | Algebra shortcut. | Sign would average out: early and late must both count. |

## Q9 — Constant delay and the gain

**Answer.** A constant delay means `Rj − Ri = Sj − Si` for every pair → D = 0 → J = 0: a fixed 777-tick offset needs no buffer, and the sender/receiver clocks' offset cancels. Gain 1/16 is a time constant ≈ 16 packets = **320 ms**; 1/2 (40 ms) would chase every outlier and make the buffer size oscillate; 1/128 (2.56 s) is longer than a typical PTT transmission, so the estimate would never converge before the call ends. RFC 3550 chose 1/16 as a compromise (VERIFY the rationale text).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Delay 777 ticks is jitter 777" | Confusing latency with variation. | Only the *change* in delay matters. |

## Q10 — Buffer trace

**Answer.** target 2: `push 5` → Accepted; `push 7` → Accepted (stored {5, 7}); `pop` → starts at the smallest stored seq, **present 5**; `pop` → slot 6 empty → **concealed** (data = copy of frame 5, `seq = 6`); `push 6` → **Late** (its slot passed); `pop` → **present 7**. Counters: received 2, late 1, concealed 1, duplicates 0, too_early 0. `reset()` marks a new talk-spurt: re-pre-buffer, clear `next` and counters, so a new call that starts at an unrelated sequence number isn't rejected as Late/TooEarly against the old state.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Late packet is inserted after the gap | It "fills the hole". | The audio for that slot already played; inserting would reorder the output. |
| Late counts as received | It arrived. | `received` counts packets accepted for playout; a late drop is counted separately. |

## Q11 — Bit rates

**Answer.** IP level `(payload + 40) × 8 / ptime`; Ethernet adds 38 B.

| ptime | payload | IP-level | Ethernet-level | TS step |
|---|---|---|---|---|
| 10 ms | 80 B | 96 000 bit/s | 126 400 bit/s | 80 |
| 20 ms | 160 B | 80 000 bit/s | 95 200 bit/s | 160 |
| 40 ms | 320 B | 72 000 bit/s | 79 600 bit/s | 320 |

Header overhead at 20 ms: `40 / 200 = 20 %` at IP level (`78 / 238 = 32.8 %` on Ethernet).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Voice is 64 kbit/s" | The codec rate. | Headers add 25 % (IP) to 49 % (Ethernet) at 20 ms. |

## Q12 — Multicast MAC

**Answer.** `239.130.5.9`: second octet 130 = `1000 0010`; the MAC keeps the low 23 bits, dropping that top bit → `0x02` → MAC **`01:00:5E:02:05:09`**. Sharing set: first octet any of 224…239 (4 free bits = 16 values) × second octet 2 or 130 (1 dropped bit = 2 values) = **32 groups**: `{224..239}.{2,130}.5.9`. `239.192.18.52`: `18 × 256 + 52 = 4660` = talkgroup **0x1234**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| MAC `01:00:5E:82:05:09` | Copy octets directly. | The top bit of the second octet is dropped (`0x82 → 0x02`). |
| 16 groups share a MAC | Counting only the first octet. | 5 dropped bits → 2^5 = 32. |

## Q13 — Story: why UDP/RTP

**Model answer — hits:** (1) **Deadline:** voice has a ~150 ms mouth-to-ear budget (VERIFY ITU-T G.114); a retransmit costs ≥ 1 RTT and arrives after its slot. (2) **Head-of-line blocking:** TCP delivers in order, so one lost segment stalls everything behind it, i.e. speech freezes then bursts. (3) **Multicast** needs UDP. (4) **RTP adds** what UDP lacks: timestamps (timing/jitter), sequence (loss/reorder), SSRC (stream identity), payload type, marker. (5) **Instead of retransmission:** PLC, FEC/redundant frames, jitter buffer, RTCP loss reports. (6) Use TCP/QUIC for signalling/control (M4 gRPC), configuration, recordings. (7) **Risks accepted:** no congestion control (rate-limit), no encryption (SRTP), NAT/firewall traversal.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "UDP is faster" | Folklore. | Speed isn't the point; *not waiting* for lost data is. |
| "TCP guarantees quality" | Reliable delivery. | Reliable ≠ timely; late audio is as bad as lost. |

## Q14 — Story: why 20 ms

**Model answer.** Speech is quasi-stationary over ~20 ms, so vocoders frame at 20 ms and RTP follows. Trade-off table: 10 ms = 96 kbit/s (+20 %), 20 ms = 80, 40 ms = 72 (−10 %) but +20 ms latency and each lost packet erases 40 ms (more audible). Also matches the 20 ms radio frame (M1) and gives the jitter buffer a 20 ms resolution. For a vocoder payload of 7 bytes, headers dominate: `40 / 47 = 85 %` overhead, so you'd bundle frames (ptime 40–60 ms), use header compression (ROHC; VERIFY) or accept the cost on a LAN. *What I'd change:* make ptime configurable per link (low-bandwidth backhaul vs LAN).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "20 ms because that's the standard" | Often true. | Gives no reasoning; interviewers want the trade. |

## Q15 — Story: why this jitter target

**Model answer.** Measured (demo): jitter ≈ 15 ms with 0–44 ms uniform delay. `choose_target_frames(15.28) = ceil(3 × 15.28/20) = 3` frames = 60 ms. Evidence: target 1 → 49/100 played, 44 late (the buffer is smaller than the spread); target 2 → 94 played, 0 late; targets 3 and 5 identical but with 20–60 ms more latency. `k = 3` is a safety margin on the *mean* deviation (the tails of delay distributions are heavier than a mean suggests; the demo's delays are bounded, real ones aren't), floor 2 frames covers a single reordering. Latency budget: 60 ms buffer + 20 ms packetisation + network must fit the PTT budget. Fixed vs adaptive: adapt target between talk-spurts (not mid-word), shrink slowly, grow fast. Validate: plot late-drop % against added delay per link type (LAN, Wi-Fi, LTE).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Set it to the maximum observed delay" | Zero loss. | Latency grows with an outlier and never shrinks. |
| "Zero buffer for lowest latency" | PTT wants low latency. | Every jitter spike becomes an audible gap. |

## Q16 — Story: G.711 and repeat-last PLC

**Model answer.** G.711 is the lowest-common-denominator codec: universal interoperability with consoles/telephony, no licence, trivial CPU. Cost: 64 kbit/s vs 2–8 kbit/s for a vocoder; and if the radio leg already used a vocoder, decoding to PCM then encoding μ-law is harmless, but the *reverse* (re-encoding to another vocoder = **tandem coding**) degrades quality noticeably. PLC: repeating the previous frame is fine for a single 20 ms loss and creates a buzz for runs; add fade-out after ~2 frames and comfort noise at the end of a burst. Better: waveform-substitution PLC (G.711 App. I; VERIFY), FEC/RED (redundant copy in the next packet).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Always insert silence on loss" | Simple. | Audible dropouts; repeating the last frame hides short losses. |

## Q17 — Story: multicast and talkgroup mapping

**Model answer.** One send, N receivers: sender and gateway uplink are independent of the number of listeners; dispatch consoles join the group they need. Mapping `239.192.(N>>8).(N&255)` is deterministic (no allocation table), human-readable and inside the organisation-local scope (RFC 2365; VERIFY). **Aliasing:** a MAC keeps 23 bits of the 28 group bits, so 32 groups share each MAC; with the second octet fixed at 192 all 65 536 talkgroups get distinct MACs, but another application using `239.x` in the same L2 domain can alias with ours. **Operations:** TTL 1, IGMP snooping (otherwise the switch floods), a querier on the segment, Wi-Fi multicast at the basic rate with no ACK (loss), and WAN/cloud typically without multicast (unicast fan-out or tunnel). **Security:** no authentication; use network isolation and SRTP (VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Multicast is free scalability" | It is on a LAN. | Needs network support; fails across most WAN/cloud paths. |
| "Use 224.0.0.x for our groups" | Valid multicast. | 224.0.0.0/24 is link-local control traffic (not forwarded; reserved). |

## Q18 — From M2 to M4 stats

**Answer.** `LossCounter` → `lost` / `loss_pct` (lost/(packets+lost)); `JitterEstimator::jitter_ms` → `jitter_ms` in the streaming `CallStats`; jitter-buffer `late()`/`concealed()` → additional counters (late drops, PLC count); M1's `payload_corrections` plus M3 BER → `ber`. RTP **marker** = key-up (start of talk-spurt) → `JitterBuffer::reset()` and a floor `request`; **EndCall**/floor release → stop playout and flush. CI: drive the buffer with deterministic arrival lists (as `SimulatedChannelWithJitterAndLoss` does: seeded LCG delays, fixed drops), assert exact counters; no sockets, no radio; the multicast demo stays manual because it depends on host networking.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Test with the real network in CI" | More realistic. | Non-deterministic (jitter changes per run) and host-dependent. |
