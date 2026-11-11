# Capstone M2 — μ-law, RTP, jitter buffer, multicast (voice over IP)

**Tier:** Capstone, milestone 2 of 4. **Time:** 5–7 h. **Code:** `code/` (stubs `include/rtp_jitter.hpp`, reference `solutions/rtp_jitter.hpp`, tests `tests/test_rtp_jitter.cpp`, optional UDP-multicast loopback demo `demo/rtp_mcast_demo.cpp`).
**Safety/legal:** synthetic tones only. No live public-safety reception, ever (in the UK that also runs into the Wireless Telegraphy Act 2006). The demo uses multicast TTL 1 so packets never leave the local network.

---

## §1 Why this milestone exists

The capstone gateway receives a radio frame (M1/M3) and must turn it into **IP voice** that dispatch consoles and other gateways can play. Pipeline position:

```
WAV in -> 20 ms frames -> [M1] frame+FEC -> [M3] 4FSK -> AWGN+CFO -> demod/decode -> [M1] parse
       -> [M2] mu-law RTP packets over UDP multicast -> network (jitter, loss, reorder)
       -> [M2] jitter buffer -> playout -> WAV out        (+ [M4] gRPC StartCall/EndCall, stats)
```

M2 is where **time** enters: the radio delivers frames on a steady clock, the IP network does not. The jitter buffer converts irregular arrivals back into a steady 20 ms stream at the price of latency, and every design constant (20 ms, 3 frames, 160 bytes) is a trade you must defend. This is also the layer where your numbers (jitter, loss) feed the M4 gRPC stats stream.

By the end you can: implement G.711 μ-law, pack/parse RTP safely, extend 16-bit sequence numbers, compute RFC 3550 loss and jitter, build and size a jitter buffer, work out voice bit rates, and explain multicast addressing including the **32:1 MAC aliasing** trap.

## §2 Prerequisites

* Bytes/big-endian, bit fields. UDP/IP basics (ports, TTL). Capstone M1 for the 20 ms frame.
* Build: `cmake -S code -B build -DCMAKE_CXX_FLAGS="-Wall -Wextra -fsanitize=address,undefined"`, `cmake --build build`, `ctest --test-dir build`. `student.*` fail until you implement; `solution.*` pass.

## §3 Concept build-up

### §3.1 G.711 μ-law: 8 bits that behave like 13 (W 3.1)

Speech has a wide dynamic range (whispers to shouts) but human hearing judges *relative* error. Uniform 8-bit PCM would give loud sounds a great SNR and quiet sounds a terrible one. **Companding** spaces the quantisation levels logarithmically: small steps near zero, large steps at high amplitude, so the *relative* error is roughly constant. Result: 64 kbit/s (8 kHz × 8 bits) with the quality of about 13-bit linear PCM (VERIFY the exact SNR figure, roughly 35–38 dB, before quoting).

The classic algorithm (the one the tests encode):

1. sign = top bit; magnitude = |pcm| clipped to 32635; add bias `0x84` (132).
2. exponent = position of the highest set bit among bits 14..7 (so 0–7).
3. mantissa = `(magnitude >> (exponent + 3)) & 0xF`.
4. byte = `~(sign | exponent << 4 | mantissa)`; the bit-inversion is a historic line-coding choice.

Worked example `pcm = 0`: `0 + 132 = 0x84 = 1000 0100`; highest bit is bit 7 → exponent 0; mantissa `(132 >> 3) & 15 = 16 & 15 = 0`; byte `~0x00 = 0xFF`. ✔ (test).
`pcm = 32767`: clip → 32635 + 132 = 32767 = `0x7FFF`; exponent 7, mantissa `(32767 >> 10) & 15 = 31 & 15 = 15`; `~(0x70|0x0F) = 0x80`. ✔
`pcm = −1`: sign set, magnitude 1 → 133 → exponent 0, mantissa 0 → `~0x80 = 0x7F`. And `0x7F` decodes to 0 ("negative zero").

Decode: `((mantissa << 3) + 0x84) << exponent − 0x84`, sign applied. `0x80` → inverted `0x7F` → `((15<<3)+132) << 7 − 132 = 252×128 − 132 = 32124`.

Step sizes: in segment e the step is `8 << e`: **8** near zero up to **1024** at the top (8, 16, 32 … 1024). A mid-level sample of 1000 encodes to `0xCE` and decodes to 988 (error 12, about 1.2 %).

> **Misconception box.** *"μ-law is compression like MP3."* It is a fixed 2:1 reduction (16 → 8 bits) by non-uniform quantisation, no model of speech. A vocoder reaches 2–8 kbit/s by modelling the vocal tract. *"μ-law and A-law are the same."* Same idea, different curve and different regions (μ-law: North America/Japan, PT 0 PCMU; A-law: Europe, PT 8 PCMA; VERIFY).

### §3.2 The RTP header (W 3.2)

RTP (RFC 3550) puts timing and sequence on top of UDP. Fixed 12-byte header, network (big-endian) byte order:

```
 byte0: V(2)=2 | P(1) | X(1) | CC(4)
 byte1: M(1)    | PT(7)
 bytes2-3: sequence number (16)
 bytes4-7: timestamp (32)         <- sampling instants, in codec clock units (8000 Hz for PCMU)
 bytes8-11: SSRC (32)             <- the stream's random identity
 [CSRC list: 4*CC bytes] [extension: 4 bytes + 4*length] payload [padding, last byte = count]
```

Worked example from the test: `80 00 00 01 00 00 00 A0 DE AD BE EF AA BB` → V=2, no P/X, CC=0, M=0, PT=0 (PCMU), seq=1, timestamp=`0xA0`=160 (one 20 ms frame at 8 kHz), SSRC=`0xDEADBEEF`, payload `AA BB`. With M=1 byte 1 is `0x80`. **Marker** conventionally flags the first packet of a talk-spurt (voice after silence), which is exactly a PTT "key-up".

Sequence number = **order and loss**. Timestamp = **timing**. They are separate: during silence suppression the timestamp jumps but sequence numbers are consecutive.

Safe parsing is a *security* exercise. Packets arrive from the network; attackers and bugs both send `CC=15` with no CSRC bytes, extension length 9 in a 16-byte packet, or padding count 9 on a 2-byte payload. Every skip needs a bounds check **before** the read: `TooShort` (< 12), `BadVersion` (≠ 2), `Truncated` (runs past the end or pad count 0). In the test `B2 00 00 05 …` (V=2, P=1, X=1, CC=2) the payload begins at `12 + 8 + 8 = 28` and has length `35 − 28 − 4 = 3`.

### §3.3 Sequence numbers, loss and serial arithmetic (W 3.3)

A 16-bit sequence number wraps in `65536 × 20 ms = 21.8 minutes`. Comparing `a > b` breaks at the wrap (is 0 after 65535?). **RFC 1982 serial arithmetic:** `a` is newer than `b` iff `(a − b) mod 65536` is in `1 … 32767`. So `seq_newer(0, 65535)` is true (difference 1), `seq_newer(100, 65000)` true (636), and exactly half the space (`0x8000`) is by convention *not* newer (ambiguous).

An **unwrapper** extends to 64 bits: add the *signed* 16-bit delta from the previous value, so a straggler from before the wrap goes back (65535 after 65536), not forward by 65536. Test: 65534, 65535, 0, 1 → 65534, 65535, 65536, 65537, then 65535 again → 65535.

**Loss (RFC 3550 A.3):** `expected = highest − first + 1`, `lost = expected − received`. Packets {10, 11, 12, 15, 16}: expected 7, received 5, lost 2, fraction 2/7 = 28.6 %. Duplicates make `lost` negative (−1 in the wrap test), which RFC 3550 allows; the *reported fraction* is clamped at 0.

### §3.4 Jitter: what it is and the RFC 3550 estimator (W 3.4)

**Jitter** is the *variation* in network delay, not the delay itself. Constant delay of 500 ms has zero jitter and needs no buffer beyond itself. For packets i, j with RTP timestamps S and arrival times R (same units):

`D = (Rj − Ri) − (Sj − Si)`, `J ← J + (|D| − J) / 16`

The first update only records the transit time. The 1/16 is a low-pass filter (a time constant of about 16 packets = 320 ms), chosen to smooth noise without lagging too much.

Worked example (the test): 20 ms packets (160 ticks), the 3rd arrives 80 ticks (10 ms) late:

| Update | D | J (ticks) |
|---|---|---|
| 1 | – (records transit) | 0 |
| 2 | 0 | 0 |
| 3 | +80 | `0 + (80 − 0)/16 = 5` |
| 4 | −80 | `5 + (80 − 5)/16 = 9.6875` |
| 5 | 0 | `9.6875 + (0 − 9.6875)/16 = 9.08203125` |

In milliseconds: `9.08203125 / 8 = 1.1353 ms`. Notice the late packet causes *two* large D values (+80 then −80): the estimator sees both the lateness and the recovery. Use unsigned 32-bit subtraction so it survives the timestamp wrap at 2^32.

### §3.5 The jitter buffer (W 3.4–3.5)

The receiver holds packets for a while, then plays them on a fixed 20 ms clock. **Deeper buffer = more latency, fewer late drops. Shallower = less latency, more concealment.** Push-to-talk tolerates less than telephony (people talk over each other), but media must still be intelligible.

Rules implemented in the pack:

* **push(seq):** unwrap; if playout started and the slot already passed → **Late** (counted, discarded); already stored → **Duplicate**; more than `max_frames` ahead of the oldest/next → **TooEarly** (bounds memory and defeats a bogus far-future sequence); else **Accepted**.
* **pop():** before start, return "nothing" until `target_frames` are stored, then start at the smallest stored sequence. After start, play `next`; **present** if stored, else **concealed** (PLC: repeat previous frame, or 160 × `0xFF` silence at start); always advance.
* **Target sizing:** `ceil(k × jitter / ptime)`, `k = 3`, floor 2 frames.

Timeline (target 2, ptime 20 ms):

| Event | Buffer | Output |
|---|---|---|
| push 0, pop | {0}: below target | nothing, not started |
| push 2 | {0, 2} | – |
| pop | starts at 0 | **present 0** |
| pop (slot 1 empty) | {2} | **concealed** (copy of frame 0), `seq = 1` |
| push 1 (arrives after its slot) | – | **Late**, discarded |
| pop | {2} | present 2 |

Why PLC "repeat previous frame": it's the cheapest concealment, with an acceptable result for a single 20 ms loss, and it degrades to a buzz when several frames are lost (production systems fade out or use model-based PLC; VERIFY G.711 Appendix I).

Simulation test: 100 packets, network delay 0–59 ms, 5 dropped (i % 20 == 13). With target 4 frames (80 ms), `late = 0`, `present = 95`, `concealed = 5`.

### §3.6 Packet sizing: ptime, timestamp step, bit rate (W 3.6)

* RTP timestamp step per packet = `ptime × clock rate`: 20 ms × 8000 = **160**; 10 ms → 80; at 16 kHz 20 ms → 320.
* IP-level bit rate = `(payload + 12 RTP + 8 UDP + 20 IPv4) × 8 / ptime`. PCMU 20 ms: `200 B × 8 / 0.02 = 80 kbit/s` (the 64 kbit/s payload plus 25 % headers). On Ethernet add 38 bytes (14 header + 4 FCS + 8 preamble + 12 inter-frame gap, as the test counts them; VLAN tags would add 4 more) → 238 B → **95.2 kbit/s**.
* ptime 10 ms → 96 kbit/s (120 B headers+payload per 10 ms: *same payload rate, 20 % more total*). ptime 40 ms → 72 kbit/s but one lost packet = 40 ms of speech and +20 ms packetisation latency.

### §3.7 Multicast: one transmission, many listeners (W 3.7)

A talkgroup is a *one-to-many* conversation, a natural fit for IP multicast: the sender transmits once; the network replicates. **IPv4 multicast** = `224.0.0.0/4` (first four bits `1110`). Ethernet maps a group to a MAC with `01:00:5E` + the **low 23 bits** of the address (RFC 1112).

Concrete: `224.0.0.251 → 01:00:5E:00:00:FB`; `239.192.0.1 → 01:00:5E:40:00:01` (second octet 192 = `1100 0000`; the top bit is dropped, leaving `0x40`).

The trap: a group address has 28 variable bits but the MAC keeps only 23. **5 bits are discarded → 32 different groups share each MAC.** `239.1.1.1` and `239.129.1.1` have the identical MAC (test). A host listening to one receives NIC-level frames of the other and the IP stack must filter them (CPU waste, or worse, leaks to an unintended app).

Talkgroup mapping used here: **TG N → `239.192.(N>>8).(N&255)`** (the `239.192.0.0/14` organisation-local scope, RFC 2365; VERIFY scope ranges). Because the second octet is the constant 192, all 65 536 talkgroups map to **distinct MACs** inside the pack's scheme (no aliasing between talkgroups). Operationally: **TTL 1** keeps it on the local network; switches need IGMP snooping or they flood; multicast over Wi-Fi uses basic rates with no ACKs and over WAN/cloud it is typically blocked, so a gateway needs unicast fan-out or tunnelling for those paths (VERIFY for your deployment).

### §3.8 Misconceptions summarised

| Claim | Reality |
|---|---|
| Jitter = latency | Jitter = variation of latency. |
| UDP is "unreliable so it's bad for voice" | A late retransmit is worthless for voice; loss concealment beats retransmission. |
| Bigger jitter buffer is always safer | It adds mouth-to-ear delay one-for-one; PTT turn-taking suffers. |
| RTP timestamp = wall clock | It's the sampling clock in codec ticks, random start. |
| Sequence gaps = loss | Reordering and duplicates exist; loss = expected − received over a window. |
| Multicast MAC is unique per group | 32 groups share each MAC. |

## §4 Worked examples

**(a) A late packet in numbers.** ptime 20 ms, target 3. Playout starts once 3 frames are stored; slot *n* then plays at `start + 20n` ms. Rule of thumb: a packet has about `(target − 1) × 20 ms` of slack relative to the earliest frame in the buffer (40 ms for target 3, 20 ms for target 2, none for target 1) before it misses its slot → Late + concealment. A 45 ms delay spread therefore wants a target of about 3 frames; the exact slack depends on which packets arrive first (confirm with the simulation, not the rule).

**(b) Demo measurement** (real UDP multicast loopback in WSL; 100 packets, uniform 0–44 ms network delay, packets 9, 26, 43, 60, 77, 94 dropped): jitter estimate ≈ **15 ms** (theory: for two independent uniform delays on width W, `E|D| = W/3 = 15 ms`). Target 1 frame: **49/100 played, 44 late**; target 2: 94 played, 0 late; target 3 and 5: 94, 0 late. (The target-1 figure depends on host timing; re-run to see the shape, not the exact number.) `choose_target_frames(15.28) = ceil(3 × 15.28/20) = 3`.

**(c) Bit rate decision table.**

| ptime | payload | IP bit rate | Packets/s | Added delay |
|---|---|---|---|---|
| 10 ms | 80 B | 96 kbit/s | 100 | 10 ms |
| 20 ms | 160 B | 80 kbit/s | 50 | 20 ms |
| 40 ms | 320 B | 72 kbit/s | 25 | 40 ms |

## §5 Questions

Do §5 on paper first. **Story** questions are written answers of 150–250 words that become interview stories. Use: *context → decision → alternatives rejected → numbers → what I'd change*.

**Q1 — Calculate.** μ-law encode `pcm = 1000` and `pcm = 100` by hand (bias, exponent, mantissa, inverted byte). Then decode `0xCE` and report the absolute and relative error.
*Context: W 3.1.*

**Q2 — Explain.** In three sentences, why does companding beat uniform 8-bit quantisation for speech? Give the step size in the lowest and highest segments and say what that means for a quiet vs loud sample.
*Context: W 3.1.*

**Q3 — Parse.** Decode `80 80 00 10 00 00 0A 00 12 34 56 78 FF FF` field by field (V, P, X, CC, M, PT, seq, timestamp, SSRC, payload). How many 20 ms frames of audio does the timestamp represent, and what does that say about the sequence number?
*Context: W 3.2.*

**Q4 — Predict.** For the packet starting `B2 00 00 05` (V=2, P=1, X=1, CC=2) with a 35-byte buffer, give the payload offset and length, and explain each skip.
*Context: W 3.2, test `CsrcExtensionAndPaddingAreSkipped`.*

**Q5 — Debug.** List the five bounds checks a safe `rtp_parse` needs and, for each, a malformed input that would read out of bounds without it. Why is this a security issue and not only a robustness issue?
*Context: W 3.2.*

**Q6 — Calculate.** Compute `seq_newer(100, 65000)`, `seq_newer(0x8000, 0)` and `seq_newer(65535, 0)` using `(a − b) mod 65536`. Then run the unwrapper on `65534, 65535, 0, 1, 65535, 2`.
*Context: W 3.3.*

**Q7 — Calculate.** Packets with sequence numbers `{10, 11, 12, 15, 16}` arrive: expected, lost, loss fraction. Then `{65534, 65535, 0, 1, 1}`: expected, received, lost, reported fraction, and why `lost` is negative.
*Context: W 3.3.*

**Q8 — Calculate.** Compute the RFC 3550 jitter after each update for `D = 0, +80, −80, 0`, and convert the final value to ms at 8 kHz.
*Context: W 3.4.*

**Q9 — Explain.** Why does a constant 777-tick network delay give `J = 0`? Why is the filter gain 1/16 rather than 1/2 or 1/128?
*Context: W 3.4.*

**Q10 — Trace.** With target 2: push 5, push 7, pop, pop, push 6, pop. Give each output (present / concealed / late) and the counters. Then explain what `reset()` is for in a PTT system (talk-spurts).
*Context: W 3.5, test `MissingFirstFrameIsSilenceAndResetRestarts`.*

**Q11 — Calculate.** For PCMU compute IP-level and Ethernet-level bit rate for ptime 10, 20, 40 ms. What is the header overhead fraction at 20 ms? What is the RTP timestamp step for each ptime?
*Context: W 3.6.*

**Q12 — Calculate.** Give the Ethernet MAC for `239.130.5.9`, and the set of IPv4 groups that share it (describe it as a bit pattern, say how many). Which talkgroup number maps to `239.192.18.52`?
*Context: W 3.7.*

**Q13 — Story (design).** *Why UDP (and RTP) for voice, not TCP?* Cover: head-of-line blocking, retransmit usefulness vs deadline, multicast needing UDP, what RTP adds (timestamp, sequence, SSRC), what you do instead of retransmission (FEC, PLC, redundancy), and a case where you *would* use TCP/QUIC (signalling, gRPC control, recorded-audio transfer).

**Q14 — Story (design).** *Why a 20 ms packet?* Cover: speech codec frame/stationarity, the packetisation-delay vs header-overhead trade (use the table: 72/80/96 kbit/s), loss granularity (a lost packet kills ptime of speech), jitter buffer resolution, matching the radio frame from M1, and what changes for a vocoder at 20 ms with 7–20 bytes of payload (overhead ratio explodes: compute 40/(40+7)).

**Q15 — Story (design).** *Why this jitter-buffer target?* Cover: measured jitter (≈ 15 ms in the demo), the `k = 3` safety factor (≈ 3σ-style margin; justify or challenge), the floor of 2 frames, the latency budget for PTT, **fixed vs adaptive** buffers (adapt between talk-spurts rather than mid-word), and how you would validate the choice (late-drop rate vs added delay curve, per link type).

**Q16 — Story (design).** *Why G.711 μ-law and "repeat last frame" PLC in the gateway?* Cover: interoperability and zero licensing at the gateway, 64 kbit/s vs a vocoder (and the transcoding quality hit if the radio side already uses a vocoder: tandem coding), PLC behaviour for one vs several lost frames, and what you would add (comfort noise, fade-out, redundancy).

**Q17 — Story (design).** *Why multicast, and how do you map talkgroups to groups?* Cover: replication in the network, `239.192/14` scope, TTL 1, the 32:1 aliasing and why `239.192.x.y` avoids it between talkgroups, IGMP snooping, Wi-Fi/WAN limitations and the unicast fallback, and a security note (multicast has no authentication — SRTP and network isolation; VERIFY).

**Q18 — Etherstack.** Describe how M2's outputs feed M4: which counters become the `CallStats` jitter/loss fields, what marker bit and `reset()` mean on key-up and key-down, and how you'd test the whole path in CI with no radio and no real network (deterministic simulated arrivals, as in the `SimulatedChannelWithJitterAndLoss` test).

## §6 Self-assessment

You are done when you can, without notes:

* [ ] Encode/decode μ-law for 0, 100, 1000, ±32767 and explain each step.
* [ ] Draw the RTP header with bit widths and parse a hex dump.
* [ ] Compute loss, jitter (with the 1/16 update) and bit rates by hand.
* [ ] Walk the jitter-buffer timeline and name Late / Duplicate / TooEarly / concealed.
* [ ] Compute a multicast MAC and state the 32:1 aliasing rule.
* [ ] Tell the "why UDP", "why 20 ms", "why this target" stories in 90 seconds each, with numbers.
* [ ] `ctest` shows all `solution.*` green, ASan/UBSan clean, `student.*` red until you implement.

**VERIFY list (not tested here):**

1. The G.711 μ-law SNR figure and the A-law/μ-law regional assignments, and G.711 Appendix I PLC.
2. RTP payload type numbers (PCMU 0, PCMA 8) and RFC 3551's default ptime for PCMU.
3. RFC 2365 administrative scope ranges (`239.192.0.0/14` is organisation-local) and IGMP snooping behaviour of your switches.
4. Wi-Fi multicast behaviour (basic rate, no ACK) and cloud/WAN multicast availability.
5. The 1/16 jitter filter constant is from RFC 3550 §6.4.1 and A.8; confirm against the RFC text.
6. Demo numbers (jitter ≈ 15 ms; target 1 → 49 played) depend on the host and WSL network config.
