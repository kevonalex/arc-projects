# 04-audio / 02 — Codecs: G.711 μ-law (implemented), Voice Codecs, Radio Vocoders

**Time:** ~3–4 h · **Prereqs:** `01-sampling-pcm` · **Code:** `code/include/codecs.hpp` (stub), `code/solutions/codecs.hpp`, `code/tests/test_codecs.cpp`
**Format:** markdown with numeric tables (no notebook: the companding curve is described by a table and a formula; a plot would not add information the table lacks).
**Etherstack lens:** an LMR↔IP gateway sees *three families* of audio: toll codecs on the IP side (G.711, AMR-WB, Opus), radio vocoders on the LMR side (IMBE, AMBE+2, TETRA ACELP), and the transcoding between them. The interview questions are: *how does μ-law work and why logarithmic? why is Opus's RTP clock 48 kHz? what are the frame sizes of the radio vocoders, and what does that do to a gateway?*

> **Legal / safety:** all audio here is synthetic (sines, silence, published test vectors). No live public-safety receive. IMBE/AMBE+2 are proprietary (DVSI) — this pack implements **frame arithmetic only**, not the vocoders.

---

## Part A — Why codecs

Raw 8 kHz × 16-bit PCM is 128 kbit/s. Three families of ways to spend fewer bits:
| Family | Idea | Examples | Typical rate |
|--------|------|----------|--------------|
| **Waveform** | code the signal sample by sample, smarter quantisation | G.711 (log PCM), G.726 ADPCM | 64 / 16–40 kbit/s |
| **Vocoder (parametric)** | transmit a *model* of the vocal tract + excitation: pitch, gain, spectral envelope, voiced/unvoiced | IMBE, AMBE+2 (multi-band excitation), LPC-10 | 2–8 kbit/s |
| **Hybrid (analysis-by-synthesis)** | model + search for the excitation that best matches the waveform | CELP family: G.729, AMR, AMR-WB, TETRA ACELP | 4–24 kbit/s |
| **Transform / switched** | MDCT + LPC modes | Opus (SILK + CELT), AAC-LD | 6–510 kbit/s |
Radio channels are narrow (12.5 kHz → 6.25 kHz-equivalent), so radio vocoders push to 3–7 kbit/s **and add forward error correction (FEC)**, because radio bit errors are frequent and vocoders are very sensitive to them. IP codecs rely on packet loss concealment instead.

---

## Part B — G.711 μ-law: derived, then implemented

### B.1 The problem and the idea
From pack 01: a linear quantiser has constant step `Δ`, so noise `Δ²/12` is fixed — quiet speech (−40 dBFS) gets 10 dB SNR at 8 bits. Human hearing, however, judges *relative* change (Weber–Fechner: loudness is ~logarithmic). So make the error **proportional to the signal**: step `Δ(x) ∝ |x|`. A quantiser with step ∝ x is a *uniform quantiser applied to `y = ln|x|`*. Therefore: **compress** with a log, quantise uniformly, **expand** at the receiver — *companding*.

`ln` is infinite at 0, so use `ln(1 + μ|x|)`:
```text
F(x) = sign(x) · ln(1 + μ|x|) / ln(1 + μ),    |x| ≤ 1,   μ = 255
 F(0)=0, F(1)=1;   F(0.01) = 0.2285   F(0.1) = 0.591   F(0.5) = 0.876
```
1 % of full scale already gets 23 % of the code range: small signals are quantised finely, large ones coarsely. For tiny |x|, `F ≈ μ x / ln(1+μ)` — linear (fine steps); for large |x|, logarithmic. (`mu_compress`, `mu_expand`.)

### B.2 The standard's piecewise-linear version
Computing `ln` per sample was expensive in 1972 hardware, so G.711 **approximates the curve with 8 straight segments, each with 16 equal steps; the step doubles every segment**. Byte layout (before the final inversion):
```text
bit 7     bits 6..4      bits 3..0
sign      segment E      step within segment M          (all bits inverted on the wire)
```
Decoded levels (16-bit linear units), positive side — from the reference implementation:
| Segment E | first level | last level | step |
|-----------|-------------|------------|------|
| 0 | 0 | 120 | 8 |
| 1 | 132 | 372 | 16 |
| 2 | 396 | 876 | 32 |
| 3 | 924 | 1 884 | 64 |
| 4 | 1 980 | 3 900 | 128 |
| 5 | 4 092 | 7 932 | 256 |
| 6 | 8 316 | 15 996 | 512 |
| 7 | 16 764 | 32 124 | 1 024 |
Formula: `level = ((M << 3) + 0x84) << E − 0x84`. The **bias 0x84 (132 = 128 + 4)** is added before encoding: the 128 makes the segment boundaries fall on powers of two (so "find the highest set bit" gives the segment), the 4 is half the smallest step so the encoder *rounds to nearest*; it is removed on decoding. Clip |x| at **32 635** (any bigger value would overflow 15 bits after adding the bias).

**Encoding algorithm:**
1. `sign = x < 0`; `m = |x|` (beware `−(−32768)` does not fit in `int16_t`).
2. `m = min(m, 32635) + 132`.
3. `E` = position of the highest set bit among bits 14..7, minus 7 (exponent 0 if none above bit 7).
4. `M = (m >> (E + 3)) & 0xF`.
5. `byte = ~(sign<<7 | E<<4 | M)`.

**Worked example `x = 1000`:** `1000 + 132 = 1132 = 0b100_0110_1100`; highest set bit is bit 10 → `E = 3`; `M = (1132 >> 6) & 15 = 17 & 15 = 1`; pre-inversion `0 011 0001 = 0x31`; inverted **`0xCE`**. Decode: `((1<<3)+132) << 3 − 132 = 140·8 − 132 = 988`; error 12 ≤ half a step (32).

**Why invert all bits?** Historical line coding: long silence (many zero samples) should produce many 1-bits to keep T1 repeaters' clock recovery alive. A consequence: **silence = `0xFF`**, a −0 (`0x7F`) also decodes to 0 → only **255 distinct levels**.

**What you gain:** error ≤ half a step: ≤ 4 at small levels, ≤ 512 at the loudest; relative error stays ≈ 3–6 % across ~48 dB; dynamic range of ≈ 13-bit linear in 8 bits (⚠ VERIFY: often quoted 12–14 bits).
SNR of a 1 013 Hz sine, measured in the tests (μ-law vs *8-bit linear*):
| level | μ-law | 8-bit linear |
|-------|-------|--------------|
| −6 dBFS | 36.7 dB | 44.0 dB |
| −26 dBFS | 38.0 dB | 23.9 dB |
| −46 dBFS | 30.7 dB | 2.1 dB |
Companding **loses a little at the top** (coarser steps) and **wins hugely at the bottom** — "roughly constant SNR across the dynamic range".

### B.3 A-law (PCMA, RTP PT 8)
Same idea, European variant: 13-bit input, **no bias**, segments 0 and 1 share the same step (so 7 distinct step sizes), and every even bit is inverted (XOR `0x55`, silence byte **0xD5**). A-law has **no zero level** (smallest are ±8) — all 256 codes distinct. A-law's top level is 32 256. μ-law (North America, Japan) and A-law (Europe, rest of world) interwork through a gateway that decodes to linear and re-encodes — a lossy step (`mulaw_to_alaw`).

> **Common misconceptions**
> 1. *"μ-law compresses the data."* The byte count is the same as linear 8-bit; what changes is *where the 256 levels are placed*. The compression is 16 → 8 bits.
> 2. *"G.711 is lossy only because of the 8 kHz sampling."* It is lossy mainly through its 8-bit log quantiser.
> 3. *"μ-law and A-law are interchangeable."* Different levels, bias, and silence bytes; mixing them yields noise or sign errors.
> 4. *"The decode/encode round trip is lossless."* Decode→encode of a *code* is (except −0), but encode→decode of a *sample* is not.
> 5. *"Companding is a 1970s hack."* The same idea (non-uniform quantisation by perceptual weighting) is in every modern codec's quantiser.
> 6. *"G.711 sounds worse than G.729 because it has more bits."* No: G.711 has *better* quality (MOS ≈ 4.1 vs ≈ 3.9 ⚠ VERIFY) at 8× the bit rate and no algorithmic delay.

**Q1.** Derive μ-law from the requirement "relative quantisation error constant". Show (i) why `Δ(x) ∝ x` means a uniform quantiser in `ln x`, (ii) why `ln(1 + μ|x|)` instead of `ln|x|`, (iii) the values `F(0.001)`, `F(0.01)`, `F(0.1)`, `F(1)` for μ = 255, and what fraction of the 128 magnitude codes each uses. *(Test `Mulaw.ContinuousCurveMatchesStandard`.)*

**Q2.** Hand-encode to a μ-law byte (and decode again, noting the error): `x = 100`, `x = 1000` (given), `x = −1000`, `x = 30000`, `x = 0`, `x = 8`, and `x = 32767` (which is clipped to 32 635 before the bias). Show the highest-bit search for each. *(Test `Mulaw.KnownCodes`.)*

**Q3.** Derive the segment table: given the formula, compute the first and last decoded level of segments 0, 1, 2 and 7 and show where the step doubling comes from (`<< E`). Why does segment 1 start at **132** and not 128? What would break without the bias?

**Q4.** The exhaustive tests show (a) 256 codes → 255 levels, (b) `0x7F → 0xFF` is the only code that does not round-trip, (c) the largest error over all 65 536 inputs is exactly 512, (d) decoded output is monotonic, (e) `encode(−x) = encode(x) ^ 0x80`. Explain each *from the algorithm*. What should a gateway do on receiving `0x7F`? *(Tests `Mulaw.Exhaustive*`, `Mulaw.SignSymmetry`.)*

**Q5.** Reproduce the SNR table: why does μ-law give a *worse* SNR than 8-bit linear for a loud signal but a far better one for a quiet one? At which level do the two curves cross (approximately), and where does speech spend most of its time? *(Test `Mulaw.CompandingBeatsLinearAtLowLevels`.)*

**Q6.** A-law: hand-encode `x = 1000` (`v = x >> 3`; segment; mantissa; XOR mask), decode it, and compare with the μ-law result. What is A-law's silence byte and what does it decode to? Transcode μ-law `0xFF` to A-law and back: what happens? Give two concrete samples where `PCMU → PCMA → PCMU` does **not** return the original code. *(Tests `Alaw.*`, `Interworking.Transcode`.)*

**Q7 (implementation notes).** Write the μ-law encoder from memory. List four classic bugs (negation of −32768, forgetting the clip, forgetting the final inversion, arithmetic vs logical shift of negative numbers). Then design the **table** version: encode table size (65 536 × 1 B = 64 KiB) vs decode table (256 × 2 B = 512 B), cache effects, and a SIMD version. Which one would you ship in a gateway handling 5 000 calls × 2 directions × 50 packets/s × 160 samples?

---

## Part C — Voice codecs on the IP side

| Codec | RTP PT | Sampling | **RTP clock** | Bitrate | Frame | 20 ms payload | Notes |
|-------|--------|----------|---------------|---------|-------|---------------|-------|
| **G.711 PCMU** | 0 | 8 kHz | 8000 | 64 kbit/s | sample-based | 160 B | μ-law; no delay; baseline for interworking |
| **G.711 PCMA** | 8 | 8 kHz | 8000 | 64 kbit/s | sample-based | 160 B | A-law |
| **G.722** | 9 | **16 kHz** | **8000** ⚠ | 64 kbit/s | sample-based | 160 B | wideband (50–7000 Hz), SB-ADPCM; the clock mismatch is a historical error kept for compatibility |
| **GSM-FR** | 3 | 8 kHz | 8000 | 13 kbit/s | 20 ms = 260 bits | 33 B | legacy |
| **G.729** | 18 | 8 kHz | 8000 | 8 kbit/s | **10 ms = 80 bits** (10 B) | 20 B (2 frames) | CS-ACELP, 5 ms look-ahead; Annex B = VAD/DTX/CNG; patents expired ⚠ |
| **AMR** (NB) | dynamic | 8 kHz | 8000 | 4.75–12.2 kbit/s | 20 ms (12.2: 244 bits) | 31 B + header | 3GPP, VoLTE/GSM; mode adaptation |
| **AMR-WB** | dynamic | 16 kHz | 16000 | 6.6–23.85 kbit/s | 20 ms (23.85: 477 bits) | 60 B + header | HD voice; licensed ⚠ |
| **Opus** | dynamic | 8–48 kHz (adaptive) | **48000 always** | 6–510 kbit/s, VBR | 2.5–60 ms | variable | SILK+CELT, in-band FEC, DTX, royalty-free; RFC 6716/7587 |
| **L16** | 10 (stereo) / 11 (mono) | 44.1 kHz | 44100 | 1 411 / 705.6 kbit/s | sample-based | 3 528 B stereo 20 ms | uncompressed, big-endian |
| **CN** (comfort noise) | 13 | — | 8000 | — | — | 1–10 B | RFC 3389 |
(⚠ = verify in RFC 3551 / the codec's recommendation.)

### C.1 Why is Opus's RTP clock *always* 48 kHz?
* **Opus switches audio bandwidth (narrow → fullband) and mode mid-stream** without renegotiating SDP; the RTP timestamp clock must stay **fixed for the whole session** (jitter, RTCP, playout all use it).
* 48 kHz is the **highest** rate Opus supports and an **integer multiple of all of them** (8, 12, 16, 24, 48 kHz) — so a 2.5 ms frame is exactly 120 ticks, 20 ms is 960, 60 ms is 2 880; no rounding error for any frame size.
* Therefore `a=rtpmap:111 opus/48000/2` — *always* "48000/2" even for a mono 16 kHz stream (the "2" says the decoder *may* output stereo; actual channel count is in `a=fmtp:… stereo=…`). The **sampling rate is not the clock**: never compute timestamps from "what bandwidth we're using".
* Contrast **G.722**: samples at 16 kHz but RTP clock 8000 (an early RFC error preserved for interop): timestamp step per 20 ms packet is **160**, not 320. A receiver computing jitter from the *sampling rate* gets a factor of 2 wrong.

### C.2 Details worth knowing
* **G.729**: 10 ms frames (80 bits = 10 B); one RTP packet usually carries 2 frames (20 ms, 20 B payload) or 3; algorithmic delay 15 ms (10 ms frame + 5 ms look-ahead); G.729B SID frames (2 B) during silence ⚠. Packet = 20 B payload + 40 B headers = 60 B → 24 kbit/s (3× the codec rate).
* **AMR-WB**: nine modes (6.6, 8.85, 12.65, 14.25, 15.85, 18.25, 19.85, 23.05, 23.85 kbit/s); RTP payload **octet-aligned** or **bandwidth-efficient** format with CMR (codec mode request) and ToC fields (RFC 4867 ⚠): payload slightly larger than `bits/8`.
* **Opus**: VBR — packet sizes vary with the content and the encoder target; **do not assume a fixed payload size** (`codec_payload_bytes_nominal` returns 0 for it). In-band FEC (LBRR) lets a following packet carry a low-bitrate copy of the previous one — loss concealment at the *codec* layer (pack 03).
* **Licensing**: G.711 and Opus free; G.729 patents expired (2017, ⚠ VERIFY); AMR/AMR-WB via patent pools; **IMBE/AMBE+2 proprietary** (DVSI hardware/software licence).

> **Common misconceptions**
> 1. *"RTP clock = sample rate."* True for most, false for Opus (48 k always) and G.722 (8 k vs 16 k).
> 2. *"A 20 ms packet always holds one frame."* G.729 holds two; some codecs use 30/60 ms frames.
> 3. *"Opus payload is fixed-size."* VBR.
> 4. *"Lower bit rate = lower quality = lower delay."* Hybrid codecs trade *delay* for rate (look-ahead).
> 5. *"Dynamic payload types are the same on both ends."* PT numbers ≥ 96 are *negotiated per session* in SDP.

**Q8.** Fill the table's last two columns for G.711, G.722, G.729, GSM, AMR (12.2), AMR-WB (23.85), Opus at 20 ms using `codec_payload_bytes_nominal` rules (frames × ceil(bits/8) or bitrate × t / 8). Then the timestamp step per 20 ms packet from the *clock*. *(Tests `CodecTable.*`.)*

**Q9.** Opus: explain the three reasons for the 48 kHz clock, give timestamp increments for 2.5, 5, 10, 20, 40, 60 ms, and describe a bug if a receiver derives jitter or playout time from the 16 kHz it decoded at. Contrast with G.722. What is the `rtpmap` line for a mono narrowband Opus call?

**Q10.** G.729: payloads for 10/20/30/40 ms packets, IP bandwidth for each, and the overhead ratio (header bytes / payload bytes). Why is 20 ms the common choice? Where do the 15 ms of algorithmic delay come from? What does Annex B do and how do you signal it in SDP (`annexb=yes/no`, `a=fmtp:18 annexb=no`)? ⚠

**Q11.** AMR-WB at modes 12.65 and 23.85 kbit/s: bits per 20 ms frame (253, 477), nominal payload bytes, IP bandwidth at 20 ms, and the extra bytes of the octet-aligned payload header (CMR 1 B + ToC 1 B ⚠). Which mode would the network choose under congestion, and how does it learn (CMR)?

---

## Part D — Radio vocoders (frame arithmetic only; **VERIFY each figure**)

| Vocoder | System | Frame | Bits/frame | Rate | Voice / FEC | Notes |
|---------|--------|-------|-----------|------|-------------|-------|
| **IMBE** (full rate) | P25 Phase 1 (C4FM, 12.5 kHz) | **20 ms** | **144** | **7.2 kbit/s** | 88 bits (4.4 k) + 56 bits (2.8 k) | channel 9.6 kbit/s total |
| **AMBE+2 half rate** | P25 Phase 2 (TDMA), DMR, NXDN ⚠ | 20 ms | 72 | 3.6 kbit/s | 49 bits (2.45 k) + 23 bits (1.15 k) | |
| **TETRA ACELP** | TETRA | **30 ms** | **137** | **4.567 kbit/s** | speech bits before channel coding | 2 frames = 274 bits per 60 ms |
**P25 Phase 1 voice** is sent in **Logical Link Data Units (LDU1/LDU2)**: each holds **9 IMBE frames** = 180 ms. At the channel's 9 600 bit/s: `9 600 × 0.18 s = 1 728` bits per LDU. Of these, `9 × 144 = 1 296` bits are voice + FEC; the remaining `432` bits (25 %) are frame sync, network ID (NID), link control / low-speed data (LC/LSD), encryption sync (in LDU2), and status symbols ⚠. **DMR**: each voice burst carries **3 AMBE+2 frames** (216 bits = 60 ms of speech) in a TDMA slot structure (two slots interleaved, 30 ms bursts) ⚠. **TETRA**: a traffic slot carries **2 ACELP frames (274 bits)** per 60 ms speech block before channel coding (→ 432 coded bits per block) ⚠.

**Gateway consequences**
* **Frame boundaries differ:** IMBE/AMBE+2 at 20 ms, ACELP at 30 ms, IP codecs at 20 ms (10/30/60 variants). Re-timing needs a buffer of up to the LCM (20 vs 30 → **60 ms**: 3 IMBE ↔ 2 ACELP).
* **No transcoding without a licensed codec**: decode IMBE→PCM→AMR-WB/Opus needs a vocoder engine (DVSI AMBE-3000/3003 chip, software licence). Quality suffers from *tandem coding*: two lossy models in series (artefacts compound), plus the FEC is dropped/re-added.
* **Pass-through (opaque)** possible only between *same-vocoder* endpoints (e.g. P25 IMBE site ↔ P25 IMBE site over IP: the RTP payload carries IMBE frames, formats per TIA-102 ⚠; no IETF RTP payload type for IMBE).
* **Encryption** (P25 AES-256 / DMR AES, etc.) applies to the vocoder frame; a gateway that cannot decrypt cannot transcode (Pack `03-net/05` Q22).
* **FEC is channel business**: on IP, you carry the 88 voice bits (or the whole 144) and the transport has its own loss handling — never assume radio bit errors on IP.

**Q12.** Derive from the numbers: IMBE 144 bits per 20 ms = 7.2 kbit/s; voice 88 bits = 4.4 kbit/s; FEC 56 bits = 2.8 kbit/s; code rate `88/144`. P25 LDU: frames, duration, total bits at 9.6 kbit/s, voice bits, overhead bits and percentage. *(Tests `Vocoders.Imbe`, `Vocoders.P25LogicalLinkDataUnit`.)*

**Q13.** AMBE+2 half-rate: 72 bits per 20 ms → rate; 49 + 23 split → rates; bytes for 1 frame and for the 3-frame DMR burst. TETRA ACELP: 137 bits per 30 ms → rate (round), bytes for 1 and for 2 frames (and the padding bits). *(Tests `Vocoders.AmbePlus2HalfRate`, `Vocoders.TetraAcelp`.)*

**Q14.** Packetising radio vocoder frames over RTP/UDP/IPv4 (40 B headers): IMBE 1 frame per 20 ms packet; AMBE+2 1 frame (20 ms) vs 3 frames (60 ms); TETRA 1 frame (30 ms) vs 2 frames (60 ms). For each: payload bytes, IP packet bytes, IP kbit/s, overhead ratio. What does it say about choosing ptime for *low-rate* codecs? *(Use pack 01 formulas.)*

**Q15.** (Design) The gateway bridges a TETRA talkgroup and a P25 talkgroup. Draw the media path (decode ACELP → PCM 8 kHz → encode IMBE), where re-timing happens (30 ms vs 20 ms frames), the **minimum added latency** (frame buffering + codec look-ahead + packetisation + jitter buffer), tandem-coding quality loss, licensing, and what you would log/measure. Why is this impossible with encrypted media unless the gateway holds the keys? *(Context: Etherstack interoperability gateways.)*

**Q16.** Legal/IP: which of G.711, G.729, AMR-WB, Opus, IMBE, AMBE+2, TETRA ACELP can you ship in a product without a licence fee (⚠ VERIFY per codec, per jurisdiction)? How do you test a transcoder **without** live radio traffic? (Synthetic PCM → encode → decode; published test vectors from the standards bodies ⚠; loopback through licensed DVSI test boards.)

---

## Part E — Build tasks (`code/`)

| Step | Functions in `codecs.hpp` | Tests |
|------|---------------------------|-------|
| 1 | `mulaw_encode`, `mulaw_decode`, `mulaw_segment`, `mulaw_step` | `Mulaw.*` (incl. exhaustive 65 536-input test) |
| 2 | `alaw_encode`, `alaw_decode`, transcoding, buffers | `Alaw.*`, `Interworking.*` |
| 3 | `mu_compress`, `mu_expand` | `Mulaw.ContinuousCurveMatchesStandard` |
| 4 | `codec_table`, `find_codec*`, `frames_per_packet`, `codec_ts_step`, `codec_payload_bytes_nominal`, `opus_valid_frame_tenth_ms` | `CodecTable.*` |
| 5 | `VocoderInfo`, `imbe_full_rate`, `ambe_plus2_half_rate`, `tetra_acelp`, `ldu_*`, `vocoder_payload_bytes` | `Vocoders.*` |

```bash
cmake -S code -B build && cmake --build build
./build/codecs_tests_student     # red until include/codecs.hpp is implemented
./build/codecs_tests_solution
```
Stretch: print the 256-entry μ-law decode table next to the continuous curve and compute max/min relative error per segment.

---

## ⚠ VERIFY list (this pack)
1. G.711 details: bias 0x84, clip 32 635, 8 segments × 16 steps, A-law XOR 0x55, silence bytes 0xFF / 0xD5, top levels 32 124 / 32 256 (ITU-T G.711; reference "g711.c" in G.191 STL).
2. G.722 RTP clock 8000 vs 16 kHz sampling (RFC 3551); L16 PT 10/11 and byte order; GSM-FR PT 3, 260 bits.
3. G.729: 10 ms / 80 bits / 5 ms look-ahead; Annex B SID size; patent expiry; AMR modes and bit counts (12.2 → 244, AMR-WB 12.65 → 253, 23.85 → 477); RFC 4867 payload header sizes.
4. Opus: `opus/48000/2` always, frame durations (RFC 6716), clock reasoning (RFC 7587).
5. IMBE 7.2 kbit/s = 4.4 kbit/s voice (88 bits) + 2.8 kbit/s FEC (56 bits); 144 bits / 20 ms; LDU 9 frames / 180 ms / 1 728 bits at 9 600 bit/s; breakdown of the 432 overhead bits (TIA-102.BAAA).
6. AMBE+2 half-rate 72 bits (49 + 23) / 20 ms = 3.6 kbit/s; which systems use it (P25 Phase 2, DMR, NXDN); DMR 3 frames per burst.
7. TETRA ACELP 137 bits / 30 ms = 4.567 kbit/s; 274 bits per 60 ms; 432 coded bits (ETSI EN 300 395-2).
8. MOS figures (≈ 4.1 G.711, ≈ 3.9 G.729) and "13-bit equivalent" dynamic range for G.711.
9. Licensing statements (royalty-free Opus/G.711, expired G.729 patents, DVSI/IMBE terms).
