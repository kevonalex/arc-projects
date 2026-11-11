# 04-audio / 01 — Sampling, PCM and Packet Sizing

**Time:** ~2–3 h · **Prereqs:** `03-net/01-headers-layering` (header sizes) · **Code:** `code/include/audio_sizing.hpp` (stub), `code/solutions/audio_sizing.hpp`, `code/tests/test_audio_sizing.cpp`
**Format:** markdown with numeric tables (no notebook: nothing here needs a plot — the derivations are algebra and tables).
**Etherstack lens:** every gateway decision starts with the same question — *"how many bytes, how often, and does it fit?"* An LMR channel is 8 kHz audio; an LTE/WebRTC client may speak 48 kHz Opus; the gateway must size packets, jitter buffers and backhaul links for both. Interviewers love the no-calculator version of this.

> **Legal / safety:** all signals in this pack are **synthetic** (sine waves, silence, test patterns). Do not record or process live public-safety or operator audio (UK Wireless Telegraphy Act 2006; data protection).

---

## Part A — From sound to numbers

### A.1 Why sampling is enough (derive Nyquist from aliasing)
A computer cannot store a continuous signal, so we measure its voltage at regular instants `t = n / fs` (the **sampling period** `T = 1/fs`). The question: *how often must we measure so nothing is lost?*

Take a tone `x(t) = sin(2π f t)` and sample it: `x[n] = sin(2π f n / fs)`. Now take a *different* tone of frequency `f + fs`:
```text
sin(2π (f + fs) n / fs) = sin(2π f n/fs + 2π n) = sin(2π f n/fs)        (n is an integer, so 2πn adds whole turns)
```
**The samples are identical.** After sampling, the two tones are indistinguishable: a tone above fs/2 **aliases** (folds) onto a lower frequency. Likewise `sin(2π (fs − f) n/fs) = −sin(2π f n/fs)` — the mirror image. So the unambiguous range is `0 … fs/2`, the **Nyquist frequency**. To keep a signal intact, **everything above fs/2 must be removed before sampling** (anti-alias low-pass filter), and the signal's bandwidth must be below fs/2.
Folding rule (used in `alias_hz`): reduce `f mod fs`, and if the result exceeds `fs/2` replace it by `fs − result`. Example at fs = 8 kHz: 1 kHz → 1 kHz; 5 kHz → 3 kHz; 7 kHz → 1 kHz (inverted); 8 kHz → 0 Hz (DC); 9 kHz → 1 kHz.

### A.2 Why telephone audio is 300–3400 Hz sampled at 8 kHz
* Speech energy spans roughly 100 Hz–10 kHz, but **intelligibility** lives mostly between ~300 Hz and ~3.4 kHz (formants, consonants). Cutting there costs naturalness (sibilants "s/f" sound dull, "thin" voice) but keeps words understandable — and every kHz of bandwidth doubles the data.
* Below ~300 Hz: little information for speech, lots of mains hum/rumble; telephone line transformers and microphones didn't pass it anyway.
* Above 3400 Hz: the analogue telephone network filtered here (FDM channel spacing of 4 kHz: 300–3400 Hz voice band + guard bands).
* **Why 8 kHz, not 6.8 kHz?** The Nyquist limit for 3.4 kHz is 6.8 kHz, but real anti-alias filters are not brick walls: they need a **transition band** to roll off. With fs = 8 kHz the cut-off may begin at 3.4 kHz and must have fallen enough by 4 kHz: a **600 Hz transition band** is practical. fs = 8 kHz also divides nicely into time (125 µs sample period; 1 ms = 8 samples; 20 ms = 160).
* Wider bands follow the same logic: **wideband** 50–7000 Hz → 16 kHz (1 kHz transition); **fullband** 20 Hz–20 kHz → 44.1 or 48 kHz (CD at 44.1 kHz inherited from video-tape recorders, 48 kHz from professional/video sync ⚠ VERIFY history).

> **Common misconceptions**
> 1. *"Sampling at twice the highest frequency is exactly enough."* Only with an ideal (impossible) filter; practical systems keep a margin.
> 2. *"Higher sample rate = better quality always."* Only if the *signal* contains the extra bandwidth: upsampling 8 kHz speech to 48 kHz adds data, not audio.
> 3. *"Aliasing can be fixed after sampling."* Never — the information is already merged. Filter *before* decimation (also when resampling 48 → 8 kHz in software!).
> 4. *"Samples are a staircase."* They are points; reconstruction filtering produces a smooth waveform.
> 5. *"1 ms is always a whole number of samples."* At 44.1 kHz it is 44.1 samples (see Part C).

**Q1.** (a) Show with a table of the first 8 samples that 1 kHz and 9 kHz tones are identical at fs = 8 kHz, and that 7 kHz is 1 kHz with the sign flipped. (b) What does a 5 kHz tone (say a data burst on a radio's audio output) turn into at fs = 8 kHz, and what do you hear? (c) List four frequencies below 20 kHz that alias to exactly 1 kHz at 8 kHz. *(Tests `Sampling.NyquistAndAliasing`, `Sampling.AliasedToneIsTheSameSamples`.)*

**Q2.** Explain, with the *transition band* argument, why telephony uses 8 kHz and not 7 kHz or 6.8 kHz; repeat for wideband (50–7000 Hz) and show why 16 kHz is the natural choice. Give the sample period in µs for 8, 16, 44.1 and 48 kHz.

---

## Part B — Quantisation: how many bits?

### B.1 Derive the "6 dB per bit" rule
A PCM sample is rounded to one of `2^N` levels. With full-scale range `±A` the step is `Δ = 2A / 2^N`. The rounding error is (for busy signals) uniform in `[−Δ/2, +Δ/2]`, with **power** `Δ²/12`.
A full-scale sine of amplitude `A = 2^(N−1) Δ` has power `A²/2 = 2^(2N−3) Δ²`. So
```text
SNR = signal / noise = 2^(2N−3) Δ² / (Δ²/12) = 1.5 · 2^(2N)
SNR(dB) = 10 log10(1.5) + 20 N log10(2) = 1.76 + 6.02 N
```
8 bits → 49.9 dB, 12 → 74.0 dB, 16 → 98.1 dB (`ideal_snr_db`). **Each extra bit halves Δ, quarters the noise power: +6.02 dB.**

### B.2 Why speech needs more than 8 *linear* bits
Voices vary by 30–40 dB between a whisper and a shout (and callers move away from microphones). The error `Δ²/12` is *fixed*, so a talker 40 dB below full scale gets `49.9 − 40 ≈ 10 dB` SNR — terrible. Linear PCM needs ~12–13 bits for telephone-grade quality across that range; G.711 reaches it in 8 bits by **companding** (log-spaced steps: small signals get small steps) — Pack 02. Sample size as stored: 16-bit signed little-endian (WAV, most CPUs); **network order for RTP L16 is big-endian** ⚠ VERIFY RFC 3551.

### B.3 Practical PCM rules
* Convert float → int16 with **rounding and clipping** (never wrap): +1.0 would overflow to −32768 and produce an ear-splitting click (`float_to_s16`).
* 8-bit WAV PCM is **unsigned** (silence = 0x80) whereas 16-bit is signed (silence = 0) ⚠ VERIFY; μ-law/A-law bytes are neither (Pack 02).
* Add (or at least understand) **dither** when reducing bit depth: without noise, quantisation error is correlated with the signal (distortion); dither converts it to benign hiss.
* Keep the internal bus at 16 bit (or float) and reduce only at the edges.

**Q3.** (a) Derive `6.02 N + 1.76` yourself, line by line. (b) Compute `ideal_snr_db` for N = 4, 8, 11, 13, 16, 24. (c) A talker at −30 dBFS, 8-bit linear: SNR? What N gives ≥ 40 dB at −30 dBFS? *(Test `Pcm.EmpiricalSnrFollowsSixDbPerBit` measures it on a synthetic sine.)*

**Q4.** Why does the measured SNR of a quantised sine in the test land within ~2 dB of the formula, but a *very quiet* sine does not (try amplitude 0.01 and 0.001 at 8 bits: the formula predicts ≈ 10 dB and −10 dB)? Also predict the SNR for amplitude 0.1 at 12 bits (hint: `ideal − 20·log10(1/amp)`). What does this tell you about the assumption "error is uniform and uncorrelated"?

**Q5.** Float-to-int16: what does `(int16_t)(x*32768)` do for x = 1.0 and x = 1.5 (undefined/implementation-defined vs clipped)? What happens in a **mixer** that adds two near-full-scale samples in `int16_t`? (Preview Pack 03.) *(Tests `Pcm.FloatToS16ClipsAndRounds`.)*

---

## Part C — Packetisation arithmetic (derive, then memorise)

Why packets at all? A call is a *stream*, but IP delivers *datagrams*. Collect `ptime` milliseconds of audio per packet. Small ptime → low delay but huge header overhead; large ptime → efficient but laggy and more audio lost per lost packet.

### C.1 Three formulas
| Quantity | Formula | G.711, 20 ms |
|----------|---------|--------------|
| samples per packet | `fs · t` = `fs · ptime_ms / 1000` | 8000 × 0.020 = **160** |
| payload bytes (raw PCM) | `samples · channels · bits / 8` | 160 × 1 × 8 / 8 = **160 B** |
| payload bytes (coded) | `bitrate · t / 8` = `bitrate · ptime_ms / 8000` | 64 000 × 20 / 8000 = **160 B** |
| packet rate | `1000 / ptime_ms` | **50 pps** |
| RTP timestamp step | `clock · ptime / 1000` | **+160** per packet |
| bandwidth | `bytes × 8 / ptime_ms` = **kbit/s** (bits per millisecond *is* kilobits per second) | 200 × 8 / 20 = **80 kbit/s** |

### C.2 Header budget
| Layer | Bytes | Why |
|-------|-------|-----|
| RTP | 12 | seq, timestamp, SSRC, flags (pack `03-net/04`) |
| UDP | 8 | ports, length, checksum |
| IPv4 / IPv6 | 20 / 40 | addresses, TTL, … |
| Ethernet header | 14 | MACs + EtherType |
| 802.1Q tag | +4 | VLAN id + priority (DSCP's L2 cousin PCP) |
| Ethernet FCS | 4 | CRC-32 |
| Preamble + SFD | 8 | receiver clock sync (not in frame size) |
| Inter-frame gap | 12 | minimum idle time on the wire |
Minimum Ethernet frame is **64 B** (incl. FCS): short frames are padded.

**G.711 worked example (20 ms):**
```text
payload            160 B      (64 kbit/s)
+ RTP 12 + UDP 8 + IPv4 20   = 200 B IP packet      → 200 × 8 / 20 = 80.0 kbit/s      "IP bandwidth"
+ Ethernet 14 + FCS 4        = 218 B frame          → 218 × 8 / 20 = 87.2 kbit/s      "Ethernet bandwidth"
+ preamble 8 + IFG 12        = 238 B on the wire    → 238 × 8 / 20 = 95.2 kbit/s      "line-rate bandwidth"
```
(`ip_packet_bytes`, `eth_frame_bytes`, `eth_wire_bytes`, `bandwidth_kbps`.)

### C.3 Sweep: how ptime changes the bill (fill, then check against `answers.md`)
| ptime | pkts/s | G.711 payload | IP packet | IP kbit/s | G.729 payload | IP packet | IP kbit/s |
|-------|--------|---------------|-----------|-----------|----------------|-----------|-----------|
| 10 ms | | | | | | | |
| 20 ms | | | | | | | |
| 30 ms | | | | | | | |
| 40 ms | | | | | | | |
The pattern: bandwidth = codec rate + `40 B × 8 / ptime` = codec rate + `320 / ptime_ms` kbit/s. **Headers cost 32 kbit/s at 10 ms, 16 at 20 ms, 8 at 40 ms** — independent of the codec. That is why G.729 (8 kbit/s) at 20 ms costs **24 kbit/s**, three times its codec rate.

### C.4 The delay/overhead/loss triangle
* Delay: a packet cannot leave before its *last* sample was captured → at least `ptime` of algorithmic delay (plus codec look-ahead, jitter buffer, network). ITU-T G.114 guidance: ≤ 150 ms one-way mouth-to-ear for good interactive speech ⚠ VERIFY. PTT radio users tolerate more but *talk-over* and floor latency matter.
* Loss: one lost packet loses `ptime` of audio — 40 ms is a syllable.
* Overhead: above. Typical VoIP choice 20 ms; radio vocoders frame at 20 ms (IMBE) or 30 ms (TETRA ACELP) ⚠ (Pack 02), and bundling several frames per packet trades latency for overhead.

> **Common misconceptions**
> 1. *"A 64 kbit/s call uses 64 kbit/s of network."* It uses 80 at IP level, 87–95 at Ethernet level, plus RTCP (~5 %) and SRTP tags.
> 2. *"Payload = bitrate / 50."* Only at 20 ms (and mind `/8` bytes vs bits): `64000/50 = 1280 bits = 160 B`.
> 3. *"Halving ptime doubles delay cost."* It halves it but doubles the packet rate and overhead.
> 4. *"Link speed = codec speed."* An 8 kbit/s codec on a 64 kbit/s link with 20 ms packets already uses 24 kbit/s.
> 5. *"Ethernet overhead is 18 B"*: 14 + 4 FCS, **plus 20 B** preamble/gap on the wire, **plus 4 B per VLAN tag**.

**Q6.** Do the G.711 worked example for **30 ms** and **10 ms**: samples, payload, IP packet, IP kbit/s, Ethernet kbit/s. Which would you choose for an LMR gateway with a 2 Mbit/s backhaul, and why?

**Q7.** Fill the C.3 table. Then prove: `kbit/s = codec_rate + 320/ptime_ms` (IPv4). What is the formula for IPv6? For SRTP with an 80-bit tag? *(Test `Sizing.SmallerPacketsCostMoreOverhead`.)*

**Q8.** G.729 at 20 ms: payload bytes (two 10 ms frames of 10 bytes), IP packet, IP bandwidth, ratio to the codec rate. Compare with G.711 20 ms: the codec is 8× smaller, the call is how many × smaller? *(Context: why backhaul planners care about packet rate, not only codec rate.)*

**Q9.** Ethernet budget: with 87.2 kbit/s per direction (Ethernet-level G.711 20 ms), how many *simultaneous* calls fit on 100 Mbit/s and on 1 Gbit/s if you allow 50 % utilisation? Redo with line-rate figure 95.2. (Both directions: full-duplex links count each direction separately.)

**Q10.** A remote radio site is connected over a **1 Mbit/s** microwave/LTE link (budget 800 kbit/s for voice). How many G.711/20 ms calls (IP-level)? How many G.729/20 ms? How many Opus 24 kbit/s/20 ms? Don't forget both the voice *and* RTCP (~5 % of session bandwidth ⚠ VERIFY RFC 3550 default).

**Q11.** Serialisation delay: how long does a 200-byte and a 1500-byte packet take to clock onto a 64 kbit/s link, 1 Mbit/s and 1 Gbit/s? What happens to a voice packet queued behind one 1500-byte data packet on the 64 kbit/s link? (Why 1990s links used fragmentation + interleaving, and why a slow radio backhaul still needs small packets and DSCP priority — pack `03-net/02`.) *(Test `Link.SerializationDelay`.)*

**Q12.** RTP timestamp increments per packet for: G.711 20 ms; G.711 30 ms; 16 kHz wideband (clock 16 000) 20 ms; Opus (clock 48 000) 20 ms; PCM 44.1 kHz stereo 20 ms. Which of these have a clock rate different from the sampling rate? (Preview Pack 02: G.722, Opus.)

**Q13.** Exactness. Which of these ptimes give a whole number of samples: 44.1 kHz × {1, 5, 10, 20} ms; 11.025 kHz × {10, 20, 40}; 22.05 kHz × {20, 40}? Why do real-time engines use 20 ms (or 10 ms) with 48 kHz and what do the 44.1 kHz pipelines do?

---

## Part D — The MTU trap

**Q14.** **The 48 kHz stereo trap.** A developer sends raw 16-bit stereo PCM at 48 kHz in 20 ms RTP packets.
(a) samples, payload bytes, IP packet size. (b) Does it fit a 1500 B MTU? (c) What happens on the wire — IP fragmentation (how many fragments? what's each one's size — recall fragment payload is a multiple of 8 and the first carries the UDP header), or, with DF set, `EMSGSIZE`/ICMP "fragmentation needed"? (d) If the packet loss per fragment is `p = 1 %`, what is the loss probability of the whole datagram? (e) What is the largest ptime that fits for stereo? for mono? For IPv6? (f) What would you do instead (codec, smaller ptime, mono)? *(Test `Mtu.The48kStereoTrap`.)*
Etherstack context: a WebRTC-side client sends 48 kHz Opus; a test tool that "just plays a WAV" as L16 will hit exactly this.

**Q15.** (a) Largest G.711 ptime that fits 1500 B MTU. (b) Is it *sensible*? Put numbers on the delay, the loss-per-packet and the jitter-buffer implications. (c) Why does every real system choose 20 ms (or 10/30/40) instead of the maximum?

---

## Part E — PCM on disk and wire (for the WAV and RTP builds)

| Format | Sample | Byte order | Silence |
|--------|--------|------------|---------|
| WAV PCM 16-bit | signed 16 | little-endian | 0 |
| WAV PCM 8-bit | **unsigned** 8 | n/a | 0x80 ⚠ |
| RTP L16 | signed 16 | **big-endian** (network order) | 0 |
| G.711 μ-law byte | companded | n/a | 0xFF (⚠ −0) / A-law 0xD5 ⚠ |
**Q16.** Write the byte sequence of the samples `[0, 1, −1, 0x1234, −32768]` for WAV (LE) and for RTP L16 (BE). Why does RTP use network byte order? Which side must swap on a little-endian CPU? How do you detect a swap bug from a hex dump of a 440 Hz sine (hint: look at the *envelope* of the bytes)?

**Q17.** (Design) Choose the gateway's internal audio format for: P25/DMR/TETRA vocoder audio (8 kHz), Opus (48 kHz), G.722 (16 kHz), SIP trunk G.711. Options: fixed 8 kHz mono; fixed 16 kHz; native per-leg with resamplers. State where resampling happens, which direction loses information (and what you lose), and what that does to CPU and latency. *(Pack 03 and 04 Parts B/D use this.)*

---

## Part F — No-calculator drills

Mental shortcuts (derived above):
* `bytes/ms = fs/1000 × channels × bytes/sample`; 8 kHz mono 8-bit = **8 B/ms**; 20 ms = ×20.
* **kbit/s = bytes × 8 / ptime_ms**; at 20 ms: `kbit/s = bytes × 0.4`.
* Header cost: `320 / ptime_ms` kbit/s (IPv4+UDP+RTP).
* Ethernet adds `18 B` (frame) or `38 B` (wire) per packet: at 20 ms that is +7.2 or +15.2 kbit/s.
* Samples per 20 ms = `fs / 50`.
1. G.711, 30 ms: samples, payload, IP bytes, IP kbit/s.
2. L16 mono 16 kHz, 20 ms: payload, IP kbit/s.
3. AMR-WB 12.65 kbit/s, 20 ms: payload bytes (round up), IP kbit/s.
4. Opus 24 kbit/s, 20 ms: payload, IP kbit/s.
5. IMBE 7.2 kbit/s (144 bits per 20 ms frame): payload, IP kbit/s.
6. 100 simultaneous G.711/20 ms calls through a gateway: Ethernet-level Mbit/s *each direction*.
7. Packets per second for 30 ms ptime; for 5 ms.
8. 3 s of G.711 audio: samples, bytes, packets at 20 ms.
9. A 5 s mono 16 kHz 16-bit WAV: data bytes and total file size (44-byte header).
10. SNR of 11-bit linear at full scale.
*(Answers in `answers.md`; the larger drill sheet with 30 items lives in `04-builds`.)*

---

## Part G — Build tasks (`code/`)

| Step | Functions in `audio_sizing.hpp` | Tests |
|------|---------------------------------|-------|
| 1 | `samples_per_packet`, `samples_exact`, `pcm_payload_bytes`, `codec_payload_bytes` | `Sizing.*` |
| 2 | `ip_packet_bytes`, `eth_frame_bytes`, `eth_wire_bytes`, `bandwidth_kbps`, `packets_per_second` | `Sizing.G711WorkedExample`, `HeaderVariants` |
| 3 | `fits_mtu`, `max_ptime_ms_for_mtu` | `Mtu.*` |
| 4 | `rtp_ts_increment`, `serialization_delay_us` | `Sizing`, `Link.*` |
| 5 | `nyquist_hz`, `alias_hz`, `ideal_snr_db` | `Sampling.*` |
| 6 | `float_to_s16`, `sine_s16`, `quantise_bits`, `snr_db` | `Pcm.*` |

```bash
cmake -S code -B build && cmake --build build
./build/sampling_tests_student     # red until include/audio_sizing.hpp is implemented
./build/sampling_tests_solution
```
Stretch: write a tiny program that prints the C.3 table for ptime 10–60 ms and for G.711, G.729, AMR-WB, Opus, IMBE.

---

## ⚠ VERIFY list (this pack)
1. RTP L16 byte order (network order) and static PT 10/11 (RFC 3551).
2. G.114 150 ms one-way delay guidance; RTCP default 5 % bandwidth share (RFC 3550).
3. 8-bit WAV unsigned; μ-law silence byte 0xFF / A-law 0xD5.
4. History of 44.1 vs 48 kHz; telephone band edges and 600 Hz transition-band argument (a teaching model, not the standard's wording).
5. Linear bits needed for toll quality (12–13) vs G.711's 8-bit companded.
6. Ethernet minimum frame 64 B, preamble 8 B, IFG 12 B (IEEE 802.3).
