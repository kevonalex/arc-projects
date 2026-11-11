# 04-audio / 04 — Answers: Builds

Everything marked "(test)" is asserted by `code/tests/test_builds.cpp` and was run against `code/solutions/builds.hpp` (22 tests, g++ `-Wall -Wextra -fsanitize=address,undefined`). ⚠ = verify against the cited standard before quoting.

---

## Build 1 — WAV

### Q1 — Implementation notes
* **Writer:** 44 bytes + `2·n`; RIFF size field `36 + 2n` (= file size − 8); byte rate `rate × block_align`; every multi-byte value little-endian via `b.push_back(v & 0xFF); b.push_back(v >> 8)` (never `memcpy` a `uint32_t` — that would be host-endian). (tests `HeaderLayoutIsCanonical`, `StereoHeader`)
* **Reader:** `pos = 12; while (pos + 8 <= size) { id, sz; pos += 8; … pos += sz + (sz & 1); }`. Guard with `sz > b.size() − pos` (subtraction, never `pos + sz`, which can wrap). `fmt ` must precede `data`; return as soon as `data` is parsed (trailing chunks irrelevant).
* **`LIST` chunk of odd length before `data`:** body 5 bytes + 1 pad byte, size field says 5. The reader advances `5 + 1`. A reader that ignores padding lands one byte early, sees `"\0dat"` as a chunk id, then garbage sizes ⇒ `nullopt` or (worse) a plausible but garbage chunk walk. (test `ReaderSkipsUnknownChunksWithPadding`)

### Q2 — Hostile inputs
| Malformed input | Check that rejects it |
|-----------------|-----------------------|
| < 12 bytes / wrong magic | length + `memcmp` of `RIFF`/`WAVE` |
| 8-bit, float, extensible | `format == 1 && bits == 16` |
| data chunk declares more bytes than the file has | `size > b.size() − pos` |
| data size not a multiple of `2 × channels` | modulo check (else the last frame is half a sample) |
| `fmt ` chunk smaller than 16 or claiming huge size | `size < 16 || size > remaining` |
| data before fmt, no data | `have_fmt` flag, fall-through return `nullopt` |
ASan catches the *reads past the end* that a "good" test file never triggers (e.g. a `data` size of `0xFFFFFFF0` with a small file) — only if a test supplies such input: that is why `ReaderRejectsMalformedFiles` includes a declared size of 100 on a 6-byte body and a file cut 3 bytes short.

### Q3 — RIFF size
RIFF size = *bytes after the first 8* so that `RIFF` + size lets a reader know where the file ends without seeking. Streaming writers (microphones, `ffmpeg -f wav pipe:`) do not know the length when the header is written and put 0 or 0xFFFFFFFF; robust readers **ignore it** and trust the actual chunk walk and the data chunk (itself clamped by the bytes available). This reader ignores the RIFF size entirely.

---

## Build 2 — μ-law exhaustive

### Q4 — The harness
```cpp
for c in 0..255 except 0x7F:  roundtrip_failures += enc(dec(c)) != c
for x in -32768..32767:
    code = enc(x); y = dec(code); xc = clamp(x, ±32635);
    step = 8 << (((~code) >> 4) & 7);              // segment from the wire code (de-inverted)
    error_violations += 2*|xc − y| > step;  monotonic_violations += y < prev;  max_error = max(max_error, |xc − y|)
for x in 1..32767: symmetry_violations += dec(enc(−x)) != −dec(enc(x))
```
Plus the ten known vectors. `max_error ≤ 512` because the largest step is 1024 (segment 7) and the quantiser **rounds to nearest** (bias = half-step alignment), so error ≤ step/2 (test asserts `500 ≤ max_error ≤ 512` — nearly reached, so the check is not vacuous). `inputs_checked == 65536` and `codes_checked == 255` are in `ok()` so a harness that silently skips a range cannot pass.

### Q5 — Mutant → counters (measured with the reference harness)
| Mutant | What breaks | Measured counters (vec / roundtrip / error / monotonic / symmetry; max error) |
|--------|-------------|----------------------|
| **none (reference)** | — | 0 / 0 / 0 / 0 / 0; **max error 512** |
| **no bias** (`s += 0`) | segment boundaries not at powers of two; rounding to floor vs a decoder that still removes 132 | 2 / 160 / 20 346 / 2 / 0; 643 |
| **no clip** | `32767 + 132` overflows 15 bits ⇒ wrong exponent | 2 / 0 / 265 / 4 / 0; 32 635 |
| **`int16_t` negate** | `−(−32768) = −32768` ⇒ negative "magnitude" ⇒ garbage code (**a single input!**) | 1 / 0 / **1** / 1 / 0; 32 635 |
| **bias 128** | rounding is truncation, not nearest | **0 / 0 / 762 / 0 / 0**; 515 — *only the error bound sees it* |
| **no inversion** | everything is `~` of the right value ⇒ silence ≠ 0xFF | 11 / 1 / 57 737 / 0 / 0 |
| **sign error for large negatives** | `x < −20000` encoded as `+|x|` | 1 / 12 / 12 768 / 13 / 12 767; 64 759 |
| **decoder forgets `− bias`** | all levels +132 | 5 / 191 / 20 599 / 0 / 0; 644 |
Two lessons from the numbers: the `int16_t` negate bug is **one bad input out of 65 536** — a random test would miss it with probability > 99.99 % per sample; and the *bias 128* mutant is invisible to vectors, round-trip, monotonicity and symmetry.
**Lesson (bias 128):** a round-trip test alone passes; only the *half-step error bound* detects it — the "decode(encode(c)) = c" property tests the *codes*, the error bound tests the *rounding*.
*Etherstack:* for a table-driven optimisation, run `mulaw_verify_exhaustive` on both (table-driven and reference) **and** compare `table_encode(x) == ref_encode(x)` for all 65 536 `x` and `table_decode(c) == ref_decode(c)` for all 256 `c`: complete equivalence in ~1 ms. Put it in CI.

### Q6 — Own mutant
Encoder rounding toward zero (truncate instead of `+ bias`: `s = |x|` then segment by `s + 128`'s highest bit, mantissa from `s >> …`): the **error bound** and **vector** checks catch it (e.g. `1000` maps to a different mantissa/ code than `0xCE`). Always say which property caught it: it shows you understand *why* the property exists.

---

## Build 3 — RTP

### Q7 — Notes
* `rtp_build`: `0x80`, `(marker << 7) | (pt & 0x7F)`, then big-endian 16/32/32. `rtp_parse`: `(b[0] >> 6) == 2`; `hdr = 12 + 4·CC`; if X: `hdr += 4 + 4·ext_words` (**read the length from the 2 bytes at `hdr+2` before adding**); if P: `padlen = b.back()`, must be `1 ≤ padlen ≤ end − hdr`. Every addition is followed by a bounds check.
* Packetiser: PCMU is 1 byte per sample so `ptime` bytes = `ptime` samples; `seq_++` / `ts_ += ptime` naturally wrap in `uint16_t/uint32_t` (test `WrapAroundOfSequenceAndTimestamp`: ts `0xFFFFFFF0 → 0x90`). `skip_silence(n)`: `ts_ += n`, `seq_` untouched, marker armed (test `SilenceSkipsTimestampNotSequenceAndRearmsMarker`). `flush()` pads with `0xFF`, the **μ-law code for 0** (so the padded samples decode to silence, not a click).
* 40 ms packets: `ptime_samples = 320`, payload 320 B, `ts += 320`, header overhead per second halves (25 pkt/s × 40 B = 8 kbit/s instead of 16) ⇒ 72 kbit/s instead of 80 — at the price of +20 ms packetisation delay and bigger loss bursts.

### Q8 — Bytes on the wire (`ssrc = 0xCAFE`, `seq0 = 500`, `ts0 = 123 456`, 480 samples)
```text
packet 0: 80 80 01 F4  00 01 E2 40  00 00 CA FE   (M=1, seq 500, ts 123456)
packet 1: 80 00 01 F5  00 01 E2 E0  00 00 CA FE   (M=0, seq 501, ts 123616 = +160)
packet 2: 80 00 01 F6  00 01 E3 80  00 00 CA FE   (M=0, seq 502, ts 123776)
```
(`500 = 0x01F4`; `123 456 = 0x0001E240`; `+160 = +0xA0` ⇒ `0x1E2E0`, `0x1E380`). Payload = 160 bytes each; total 172 bytes.

### Q9 — Endianness
WAV: little-endian; RTP/IP/UDP: big-endian ("network byte order"). Make it unmistakable with named helpers: `put_le16/put_le32` (file) vs `put_be16/put_be32` (network), never raw shifts inline. In this pack's solution: `detail::put16/put32` are LE (WAV), RTP is written byte-by-byte high-to-low. A test that uses an *asymmetric* value (0x0102 / 0x01020304 / 0xABCD) catches swaps; symmetric values like 0x0101 hide them.

### Q10 — Padding
Senders set P when a cipher or alignment needs the payload extended to a block boundary (historically SRTP/AES-CBC style encryption, some RTP-over-DTLS paths ⚠); the last byte of the packet says how many bytes (including itself) to drop. Parser bugs: trusting it (`padlen > payload` ⇒ negative length ⇒ huge unsigned size ⇒ out-of-bounds copy), accepting `padlen == 0`, and forgetting that the pad count comes **after** the extension header is parsed (so it must not eat the header). Tests: `ParseHandlesCsrcExtensionAndPadding`.

---

## Build 4 — Network and receiver

### Q11 — Implementation and prediction
`simulate_network`: for each packet one LCG draw for loss, one more for delay (only for kept packets — so a different `loss_percent` shifts later draws; that is fine, determinism is per `(seed, model)`). Arrival time = `send + delay`; stable sort by `(arrival, index)`.
`run_receiver`: tick `t` at `first_tick + t·20 ms`; deliver all arrivals ≤ tick time; `pop()`; append 160 samples.
Rule of thumb: the buffer must cover the **delay spread**, `depth ≥ ⌈(max_delay − min_delay)/ptime⌉` packets. Measured with this pack's code (250 packets, 5 % loss, mean over 20 seeds; `late` per run):

| delay spread | depth 2 | depth 3 | depth 4+ |
|--------------|---------|---------|----------|
| 5–40 ms (35 ms ⇒ ⌈35/20⌉ = 2) | 0.0 | 0.0 | 0.0 |
| 5–80 ms (75 ms ⇒ ⌈75/20⌉ = 4) | **1.5** | 0.1 | 0.0 |

So for 5–80 ms at depth 4 *nothing* is late — to see lateness you must shrink the buffer to 2 or 3. Each late packet is also a concealed slot (`Lost`), and each extra depth step **costs +20 ms mouth-to-ear latency**. (The baseline of ≈ 22.4 `Lost` slots per run is ≈ 12.4 real network losses + ≈ 10 concealed underrun slots after the stream ends — the test run of 300 ticks outlasts the 250-packet stream.)

### Q12 — Experiment guidance
Using the same sweep: smallest depth with `late == 0` in all 20 seeds is **2** for 5–40 ms and **4** for 5–80 ms, exactly `⌈spread/ptime⌉`. (Note `JbConfig::min_depth = 2` clamps a requested depth 1 up to 2.) The adaptive rule `⌈4·J/ptime⌉` with `J ≈ 10 ms` for a uniform 35 ms spread (mean |D| ≈ spread/3 ≈ 12 ms ⇒ target ⌈2.4⌉ = 3) gives a bit more margin than the worst case needs: it costs 20 ms to be safe against variation you have not yet seen. Say which you'd ship for a radio console (tolerant of 100+ ms, wants no dropouts) vs a telephony bridge (wants low latency) (⚠ numbers depend on seeds and the model; measure, don't assert).

### Q13 — PLC extension
Let the signal have power `P` and be repeated with gain `g` at a phase offset φ from the true continuation: error power `= P·(1 + g² − 2g·cos φ)`, versus `P` for silence. Averaged over unknown φ the cross term vanishes and the error is `P(1 + g²) > P` — **a blind frame repeat is on average *worse* than silence** by that metric; it only helps when `cos φ > g/2`. Here a 20 ms frame of a 440 Hz sine is 8.8 periods, so the repeat is 0.8 cycle (φ = 288°, cos φ = 0.31) out of phase: with `g = 0.5` error `= 1 + 0.25 − 0.31 = 0.94 P` — a negligible gain; with a 500 Hz sine (exactly 10 periods) φ = 0 and error `= 0.25 P`. *That is the argument for pitch-synchronous repeat (pack 03) over frame repeat.* Over **played** slots only the SNR cannot change — it ignores concealment entirely, so it is the wrong metric for PLC. Use whole-timeline measures (SNR as above, or perceptual metrics such as PESQ/POLQA — licensed, ITU-T P.862/P.863 ⚠).

### Q14 — Not modelled
* **Clock drift** (buffer depth ramps; test: feed packets at 20 ms·(1 − 100 ppm) and assert depth growth `≈ 0.8 samples/s`).
* **Duplicates** (add a duplicate-probability to `NetworkModel`; assert `duplicate` counter and no double playout).
* **SSRC / payload-type change** (the receiver ignores `pt` and `ssrc` — an attacker or a misconfigured peer can inject; production code keys state by SSRC and checks the negotiated PT).
* **DTX/SID** (the packetiser's `skip_silence` exists; the receiver needs SID handling to switch to comfort noise; test: ts jump with contiguous seq must produce `Silence`, not `Lost`).

---

## Part F — Drill sheet key

| # | Answer | Work |
|---|--------|------|
| 1 | **160** | 8000 × 0.020 |
| 2 | **320** | 16000 × 0.02 |
| 3 | **480** | 48000 × 0.01 |
| 4 | **441** | 44100 × 0.01 |
| 5 | **240 B** | 1 B/sample, 8000 × 0.03 |
| 6 | **20 B** | 8000 bit/s × 0.02 / 8 (two 10-byte frames) |
| 7 | **60 B** | 24000 × 0.02 / 8 |
| 8 | **60 B** | 477/8 = 59.6 → 60 (⚠ + payload header in octet-aligned mode) |
| 9 | **80 kbit/s** | (160 + 40) × 8 × 50 |
| 10 | **96 kbit/s** | (80 + 40) × 8 × 100 |
| 11 | **74.7 kbit/s** | 280 B × 8 = 2240 bits per 30 ms |
| 12 | **24 kbit/s** | (20 + 40) × 8 × 50 |
| 13 | **16 kbit/s** | (40 + 40) B = 640 bits per 40 ms |
| 14 | **40 kbit/s** | (60 + 40) × 8 × 50 |
| 15 | **87.2 kbit/s** | (200 + 18) × 8 × 50 = 218 × 400 |
| 16 | **88 kbit/s** | (160 + 12 + 8 + 40) = 220 B × 400 |
| 17 | **1 460 B; 182 ms** | 1500 − 40; 1460/8 B per ms = 182.5 → 182 |
| 18 | **3 840 B; no** | 48000 × 4 × 0.02; > 1 500 |
| 19 | **7 ms** | 192 B/ms; 1460/192 = 7.6 |
| 20 | **33.3 pkt/s** | 1/0.03 |
| 21 | **160** | G.722 clock is 8000 |
| 22 | **120** | 48000 × 0.0025 |
| 23 | **≈ 149 h** | 2³² / 8000 = 536 871 s |
| 24 | **≈ 21.8 min** | 65536 / 50 = 1311 s |
| 25 | **0.4 samples/s** | 8000 × 50 × 10⁻⁶ |
| 26 | **0.8 samples/s; 200 s** | 100 ppm relative; 160 / 0.8 |
| 27 | **≈ 98 dB** | 6.02 × 16 + 1.76 = 98.08 |
| 28 | **≈ 50 dB** | 6.02 × 8 + 1.76 = 49.9 |
| 29 | **Nyquist** | band-limit 3.4 kHz ⇒ ≥ 6.8 kHz; 8 kHz leaves a transition band for the anti-alias filter |
| 30 | **3 kHz** | alias = \|5 − 8\| = 3 kHz |
| 31 | **6 / 12 / 18 dB** | 20·log₁₀ N |
| 32 | **1 024; 512** | segment 7 step; half a step |
| 33 | **1.0** | J += (16 − 0)/16 |
| 34 | **60 ms; 6 packets (120 ms)** | 3 × 20; 4 × 30 / 20 |
| 35 | **2 cores** | 100 × 2 × 10 / (2000 × 0.5) = 2000/1000 |

Targets: ≥ 30 correct in ≤ 12 minutes; the ones people miss are 11 (decimals), 15 (Ethernet 18 vs 14), 16 (IPv6), 19 (floor, not round) and 29.
