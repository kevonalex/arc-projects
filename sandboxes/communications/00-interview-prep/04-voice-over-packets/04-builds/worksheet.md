# 04-audio / 04 — Builds: WAV, Exhaustive μ-law Test, RTP Packetiser, Lossy Pipeline + Sizing Drill Sheet

**Time:** ~5–6 h (+ 30 min for the drill sheet) · **Prereqs:** packs `04-audio/01…03`, `03-net/04-rtp-sip` · **Code:** `code/include/builds.hpp` (stub), `code/solutions/builds.hpp`, `code/tests/test_builds.cpp` (22 tests), `code/third/{mulaw,jitter_buffer}.hpp` (given: reference copies of the pack-02 codec and the pack-03 jitter buffer)
**Format:** markdown. The drill sheet in Part F is a no-calculator oral/written exam, answers in `answers.md`.
**Etherstack lens:** these are the four tools you will actually write in the first week of an audio/gateway job: *read and write test audio, prove a codec correct, turn audio into RTP, and replay it through a hostile network.* An interviewer who asks "write a WAV reader" or "how would you test a μ-law codec" is checking whether you think about **bounds, hostile input and exhaustive evidence**.

> **Legal / safety:** synthetic audio only (sines, silence). No live public-safety receive; do not use recordings of real emergency-services traffic as test material. (UK: Wireless Telegraphy Act 2006 / Investigatory Powers Act 2016 ⚠ VERIFY.)

**How the pieces fit**
```text
 WAV bytes --wav_read_pcm16--> int16 PCM --PcmuPacketiser--> RTP packets --simulate_network--> arrivals (lost, late, reordered)
                                  ^ mulaw_encode (given; verified by Build 2)                             |
                                                                                                          v
 PCM out  <--mulaw_decode--  JitterBuffer.pop() per 20 ms tick  <--JitterBuffer.push()  <--rtp_parse  (run_receiver)
```

---

## Build 1 — WAV reader/writer (RIFF, PCM16)

### 1.1 Format from first principles
A WAV file is the simplest "container": a **RIFF** envelope of *chunks*, each `[4-byte ASCII id][4-byte little-endian size][body]`. The canonical file has 44 bytes of header:

| Offset | Size | Field | Value (mono, 8 kHz) |
|--------|------|-------|---------------------|
| 0 | 4 | `"RIFF"` | |
| 4 | 4 | RIFF size = **file size − 8** | `36 + data_bytes` |
| 8 | 4 | `"WAVE"` | |
| 12 | 4 | `"fmt "` (note the space) | |
| 16 | 4 | fmt chunk size | 16 |
| 20 | 2 | format tag (1 = integer PCM, 3 = float, 0xFFFE = extensible) | 1 |
| 22 | 2 | channels | 1 |
| 24 | 4 | sample rate | 8000 = `40 1F 00 00` |
| 28 | 4 | byte rate = rate × block align | 16000 |
| 32 | 2 | block align = channels × bytes/sample | 2 |
| 34 | 2 | bits per sample | 16 |
| 36 | 4 | `"data"` | |
| 40 | 4 | data size in bytes | `2 × n` |
| 44 | … | samples, **little-endian**, interleaved by channel | |
**Little-endian:** the sample `0x1234` is stored `34 12`; `-2` is `FE FF`. Network headers (RTP, IP) are the opposite — **big-endian**. Writing both formats in one pack is deliberate: *endianness errors are the commonest bug at the file/network boundary.*

**Chunk padding rule:** chunk bodies are **padded to an even size** (the pad byte is *not* counted in the chunk's size field). A reader that walks chunks must do `pos += size + (size & 1)`.

### 1.2 Hostile-input reader (the real exercise)
The reader receives bytes from outside: it must never read out of bounds, never trust a size field, and answer *"not a file I can handle"* with `nullopt`. Guard every access: `size > remaining` ⇒ reject (subtract, don't add — `pos + size` can overflow 32-bit arithmetic). Reject: short input, bad magic, bits ≠ 16, non-PCM, zero channels, data before fmt, no data chunk, declared data > bytes present, data not a whole number of frames. Accept: unknown chunks (`LIST`, `fact`) before or after `fmt`, empty data.

> **Common misconceptions**
> 1. *"Header = 44 bytes always."* Only the *canonical* writer output. Real files have `LIST`/`bext`/`fact` chunks, a 18- or 40-byte `fmt`, and sometimes a wrong RIFF size (streams written before the length is known: 0 or 0xFFFFFFFF).
> 2. *"sample_rate × 2 = byte rate."* Only for mono 16-bit: it is `rate × channels × bytes/sample`.
> 3. *"The data size is trustworthy."* Always compare with what is actually there.
> 4. *"Odd chunk ⇒ no padding."* Odd-sized chunks **do** have a pad byte, uncounted.

**Q1.** Implement `wav_write_pcm16` and `wav_read_pcm16`. Satisfy `Wav.*` (7 tests). *Etherstack:* a test-vector WAV from a vendor has a `LIST` chunk of odd length before `data` — show that your reader handles it and explain what would happen with a reader that ignores padding.
**Q2.** Fuzz thought-experiment: list five classes of malformed input and the single check that rejects each. Which would an address sanitizer catch that a unit test would not?
**Q3.** Why does the RIFF header size field equal `file size − 8` and what do you do when you meet a file where it says 0? (Reader here ignores it; justify.)

---

## Build 2 — Exhaustive μ-law verification (and verifying the verifier)

### 2.1 Why exhaustive
The input space is **only 65 536 values** (16 bit) and **256 codes**: running *all* of them takes < 1 ms. When you can enumerate the whole domain, sampling it is malpractice — a bug at exactly −32 768 or at a segment edge passes any random test. (Contrast: a 32-bit function has 4×10⁹ inputs ≈ seconds; still feasible. A 64-bit one is not — you then need properties and fuzzing.)

### 2.2 Properties to assert (derived from pack 02)
1. **Known vectors** (hand-derived): `0 → 0xFF`, `1000 → 0xCE → 988`, `32767 → 0x80`, `−32768 → 0x00`; decode `0x80 → +32124`, `0x00 → −32124`.
2. **Decode-then-encode is the identity** on the 255 distinct levels: `encode(decode(c)) == c` for all `c` except `0x7F` (negative zero decodes to 0 and re-encodes to `0xFF`).
3. **Error bound:** after clipping `x` to ±32 635, `|x − decode(encode(x))| ≤ step/2`, where `step = 8 << segment` of the *chosen* code (8 … 1024). The max over all inputs is ≤ 512 and *nearly reached* — a bound that is never approached proves nothing.
4. **Monotonic:** `decode(encode(x))` never decreases as `x` increases (a quantiser must not reverse order).
5. **Odd symmetry:** `decode(encode(−x)) = −decode(encode(x))` for `x = 1…32 767` (`−32 768` has no positive twin).

### 2.3 Mutation testing: test the test
A checker that cannot fail is worthless. The test file contains **broken codecs**, each the reference with one mistake: *no bias*, *no clipping*, *`int16_t` negation of −32 768*, *bias 128 instead of 132*, *bit inversion forgotten*, *sign error for large negatives*, *decoder forgets to subtract the bias*. `EveryBrokenVariantIsRejected` demands that your harness flags **all** of them; `ReportCountsPointAtTheBug` checks that the counters distinguish bug classes (error violations vs vector failures vs monotonic violations).

> **Misconception box**
> 1. *"Random testing with 1 000 samples is enough."* The classic μ-law bugs live at −32 768, ±32 635 and the 8 segment edges.
> 2. *"A round-trip test proves correctness."* A codec that is *its own inverse but wrong* (e.g. forgetting the inversion in both directions) round-trips perfectly — **known vectors** and **error bounds** are what expose it.
> 3. *"If the tests pass, the tests are good."* Only if they fail on mutants.

**Q4.** Implement `mulaw_verify_exhaustive` (function pointers, so it can test *any* codec). Make `MulawExhaustive.*` pass. Report the `max_error` and explain why it is not larger than 512.
**Q5.** For each mutant in the test file, predict *before running* which counter(s) become non-zero. Then run and compare. *Etherstack:* an RTP gateway ships a hand-optimised table-driven μ-law; how would you prove it equal to the reference over every input and every code?
**Q6.** Add your own mutant (e.g. rounding toward zero in the encoder) and show that the harness rejects it. Which property catches it?

---

## Build 3 — RTP builder, parser and PCMU packetiser

### 3.1 Header
Reuse pack 03-net/04 but with **bytes**:
```text
byte 0   : V(2)=10 | P | X | CC(4)        0x80 for "no padding, no extension, no CSRC"
byte 1   : M | PT(7)                      PCMU: PT 0;  marker = first packet of a talkspurt
bytes 2-3: sequence number (big-endian)   +1 per packet, wraps 65535 → 0
bytes 4-7: timestamp (BE)                 += samples per packet (160 for 20 ms @ 8 kHz), wraps 2³²
bytes 8-11: SSRC (BE)
[CSRC list 4·CC bytes] [extension: 4-byte header with length in 32-bit words, then that many words] payload [padding; last byte = pad count]
```
**Parser rules** (defensive again): `size ≥ 12`; version == 2; CSRC/extension must fit; the padding count must be `1 … payload size`. The jitter buffer is only as robust as this function — on a real network, *anything* can arrive on your port.

### 3.2 Packetiser state machine
`push(pcm)`: μ-law-encode each sample (1 byte/sample, 8 kHz), cut into frames of `ptime` bytes, one packet per **complete** frame, keep the remainder. `flush()`: pad the last partial frame with `0xFF` (μ-law silence) and send. `skip_silence(n)`: **ts += n, seq unchanged**, marker armed — the DTX rules from pack 03.

**Q7.** Implement `rtp_build`, `rtp_parse`, `PcmuPacketiser`, `wav_to_rtp`. Make `Rtp.*`, `Packetiser.*`, `WavToRtp.*` pass (10 tests). *Etherstack:* the console wants 40 ms packets; what changes (`ptime_samples = 320`, `ts` step, payload size, bandwidth)?
**Q8.** Show on paper the first three packets of `wav_to_rtp` with `ssrc = 0xCAFE, seq0 = 500, ts0 = 123456` for 480 samples: hex of the 12-byte header of each. (Hint: 500 = `01 F4`, 123456 = `00 01 E2 40`.)
**Q9.** Where exactly is the endianness decision made in this pack (WAV vs RTP)? Write the one-line helper that makes each unmistakable.
**Q10.** Padding bit: when do real senders set it (SRTP/encryption block alignment) and what is the classic parser bug (trusting the pad count)?

---

## Build 4 — Hostile network + receiver = the pipeline

### 4.1 The emulator
A **deterministic** (seeded LCG) network model: for packet *k* (sent at `k·spacing`): a loss draw, then a delay draw uniform in `[min, max]`. Because the spread (e.g. 35 ms) is bigger than the spacing (20 ms), later packets overtake earlier ones: **reordering needs no extra code**, it falls out of "delay is random". Sort by arrival time (stable by index) to get the arrival schedule. *Determinism is a feature:* the same seed reproduces the same failure forever, which is what lets a test assert on exact packet fates.

### 4.2 The receiver
A **playout clock** ticks every ptime (20 ms). Before each tick, hand every packet that has arrived to the jitter buffer (`rtp_parse` first: garbage is counted, never fatal), then `pop()` once. Output exactly `ptime_samples` samples per tick: decoded audio for `Packet`, **zeros** for `Lost / Silence / Buffering` (PLC is an extension — pack 03, Part C).

**What the tests then prove end to end:**
* perfect network ⇒ the output is **bit-exact** `decode(encode(x))` for every frame, in order, preceded by the prefill delay;
* 5 % loss + 5–40 ms jitter ⇒ played timestamps strictly increase (reordering repaired, no duplicates); every played frame equals the μ-law round trip of **its own** source frame (the frame index comes from the RTP timestamp, not from the tick number); every network loss produced a concealed slot; SNR of the played frames > 30 dB;
* 2 garbage packets ⇒ `bad_packets == 2`, no crash, the other 10 frames play.

> **Misconception box**
> 1. *"The output frame index equals the tick."* Not with loss/reorder: map by **timestamp** (`ts / 160` here).
> 2. *"Reordering needs its own simulation."* It is a consequence of jitter larger than packet spacing.
> 3. *"Drop-in tests with `rand()` are fine."* Flaky tests teach nothing; seed everything.

**Q11.** Implement `simulate_network` and `run_receiver`. Make `NetworkEmulator.*` and `Pipeline.*` pass. Then change the delay spread to 5–80 ms and run with `initial_depth` 2, 3 and 4: predict (before running) what happens to `stats.late` at each depth and what the fix costs (depth ↑ ⇒ latency).
**Q12.** With `loss_percent = 5, spread 5–40 ms`, find by experiment the smallest `initial_depth` with `late == 0` over 20 seeds. Compare with the adaptive rule `4·J/ptime` from pack 03.
**Q13.** Extend the receiver: on `Lost`, repeat the previous decoded frame at half amplitude. Does the SNR over *all* slots improve? Over *played* slots only? (Hint: the second metric must not change; explain why that test is the wrong one for PLC.)
**Q14.** What is *not* modelled here that would matter on a real gateway (clock drift, duplicated packets, SSRC change, payload-type changes, DTX/SID)? Pick two and sketch the test.

---

## Part F — No-calculator sizing drill sheet (35 items)

*Rules: no calculator, answer in ≤ 20 s each. Headers: RTP 12 + UDP 8 + IPv4 20 = **40 B** unless stated; Ethernet adds **18 B** (14 header + 4 FCS); wire adds 20 B more (preamble + IFG). Answers in `answers.md`; every one is also derivable from `04-audio/01-sampling-pcm`.*

**Samples and payload**
1. 8 kHz, 20 ms → samples per packet?
2. 16 kHz, 20 ms → samples?
3. 48 kHz, 10 ms → samples?
4. 44.1 kHz, 10 ms → samples?
5. G.711, 30 ms → payload bytes?
6. G.729 (8 kbit/s), 20 ms → payload bytes?
7. Opus at 24 kbit/s, 20 ms → payload bytes (CBR)?
8. AMR-WB 23.85 kbit/s: bits per 20 ms frame (477), payload bytes rounded up?

**Bandwidth (IPv4 unless stated)**
9. G.711, 20 ms → IP kbit/s?
10. G.711, 10 ms → IP kbit/s?
11. G.711, 30 ms → IP kbit/s (to one decimal)?
12. G.729, 20 ms → IP kbit/s?
13. G.729, 40 ms → IP kbit/s?
14. Opus 24 kbit/s, 20 ms → IP kbit/s?
15. G.711, 20 ms on Ethernet → kbit/s (frame, no preamble)?
16. G.711, 20 ms over IPv6 (40-byte IP header) → IP kbit/s?

**MTU and packet rates**
17. Largest G.711 payload in a 1 500-byte MTU with 40 B of headers? Max ptime (ms)?
18. 48 kHz stereo 16-bit PCM, 20 ms → bytes? Fits in 1 500?
19. Same format: largest whole-millisecond ptime that fits?
20. Packets per second at 30 ms?

**Time**
21. RTP timestamp step for G.722, 20 ms?
22. Opus 2.5 ms → timestamp step?
23. 8 kHz timestamp wrap time (hours)?
24. Sequence wrap at 50 pkt/s (minutes)?
25. 50 ppm at 8 kHz → samples/s?
26. Two devices ±50 ppm: worst relative drift, and time to slip 160 samples?

**Quantisation and bits**
27. Ideal SNR of 16-bit PCM (6.02 N + 1.76)?
28. Of 8-bit linear?
29. Why do telephone systems sample at 8 kHz for 300–3 400 Hz?
30. 5 kHz tone sampled at 8 kHz — what frequency do you hear?
31. N voices mixed: worst-case headroom for N = 2, 4, 8 in dB?
32. Largest μ-law step and the maximum quantisation error?

**Jitter and systems**
33. First |D| = 16 units from J = 0 → J after one update?
34. Jitter buffer depth 3 × 20 ms: added latency? Target for J = 30 ms with k = 4?
35. Cores needed: 100 calls, 2 streams each, 10 MIPS per stream, 2 000 MIPS per core at 50 % headroom?

---

## VERIFY list for this pack
* The 44-byte WAV header layout for **mono PCM16** is canonical; `WAVE_FORMAT_EXTENSIBLE` (0xFFFE) and 24/32-bit PCM are *rejected here by design* — real files may use them ⚠ (also check the RIFF size convention for streamed files).
* Ethernet overhead (14 + 4 FCS) and wire overhead (preamble 8 + IFG 12) as used in drills 15 ⚠ VERIFY if you quote "87.2 kbit/s".
* AMR-WB bit counts (477 bits at 23.85 kbit/s) and payload-header bytes (octet-aligned) ⚠ — RFC 4867.
* Opus bitrate → payload size uses CBR for the drill; real Opus is VBR ⚠.
* μ-law vectors (`1000 → 0xCE → 988`, ±32 124) derived from the reference algorithm in pack 02 — cross-check against ITU-T G.711 / the SoX or libsndfile implementation ⚠.
* `CMakeLists.txt` was **not** run locally (no cmake available); tests were verified by compiling directly with g++ (ASan + UBSan).
