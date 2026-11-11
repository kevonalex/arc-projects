# Cheatsheet — Codecs

## G.711 μ-law (PCMU, PT 0)
* Why log: error ∝ signal ⇒ uniform quantiser in `ln x`. `F(x) = ln(1+μ|x|)/ln(1+μ)`, μ = 255. F(0.01) = 0.23, F(0.1) = 0.59.
* Byte (pre-inversion): `S EEE MMMM`; **all bits inverted** on the wire. Silence = **0xFF**; 0x7F = −0 (255 levels).
* Encode: `m = min(|x|, 32635) + 132` → `E` = highest set bit (14..7) − 7 → `M = (m >> (E+3)) & 15` → `~(S<<7 | E<<4 | M)`.
* Decode: `((M<<3) + 132) << E − 132`, sign. Range ±32 124.
* Segments (first..last, step): 0: 0..120 (8) · 1: 132..372 (16) · 2: 396..876 (32) · 3: 924..1884 (64) · 4: 1980..3900 (128) · 5: 4092..7932 (256) · 6: 8316..15996 (512) · 7: 16764..32124 (1024).
* Error ≤ half step (max 512). Example 1000 → `0xCE` → 988.
* SNR ≈ 30–38 dB from −10 to −50 dBFS (8-bit linear: 40 → 0 dB). Crossover ≈ −12 dBFS.
* Bias 132 = 128 (power-of-two boundaries) + 4 (round to nearest).

## G.711 A-law (PCMA, PT 8)
13-bit input (`x >> 3`), no bias, segments 0 and 1 same step, XOR **0x55** (positive mask 0xD5), silence **0xD5** (= +8; no zero level), 256 levels, range ±32 256. 1000 → `0xFA` → 1008.
PCMU ↔ PCMA via linear is lossy (16/256 codes don't round-trip; lattices offset by 8): avoid chains.

## Codec table
| | PT | sample | **RTP clock** | rate | frame | 20 ms payload |
|-|----|--------|---------------|------|-------|---------------|
| PCMU/PCMA | 0/8 | 8k | 8000 | 64k | sample | 160 B |
| G.722 | 9 | **16k** | **8000** | 64k | sample | 160 B |
| GSM-FR | 3 | 8k | 8000 | 13k | 20 ms/260 b | 33 B |
| G.729 | 18 | 8k | 8000 | 8k | **10 ms/80 b** | 20 B (2 fr) |
| AMR 12.2 | dyn | 8k | 8000 | 12.2k | 20 ms/244 b | 31 B + hdr |
| AMR-WB 23.85 | dyn | 16k | 16000 | 23.85k | 20 ms/477 b | 60 B + hdr |
| Opus | dyn | ≤48k | **48000 always** (`opus/48000/2`) | VBR 6–510k | 2.5–60 ms | variable |
| L16 | 10/11 | 44.1k | 44100 | 1411k/705k | sample | big-endian |
| CN | 13 | — | 8000 | — | — | RFC 3389 |
* Opus clock 48 kHz: constant for the session though bandwidth switches; multiple of all Opus rates (8/12/16/24/48) → exact ticks: 2.5 ms = 120, 10 = 480, 20 = 960, 60 = 2880.
* G.722 20 ms: ts +160 (not 320).
* G.729: 15 ms algorithmic delay (10 + 5 look-ahead); Annex B DTX, `annexb=` must match.

## Radio vocoders (VERIFY)
| | Frame | Bits | Rate | Split |
|-|-------|------|------|-------|
| IMBE (P25 Ph1) | 20 ms | 144 | **7.2 kb/s** | 88 voice (4.4k) + 56 FEC (2.8k) |
| AMBE+2 half-rate | 20 ms | 72 | **3.6 kb/s** | 49 voice (2.45k) + 23 FEC (1.15k) |
| TETRA ACELP | 30 ms | 137 | **4.567 kb/s** | 2 frames = 274 b / 60 ms |
* P25 LDU: **9 IMBE frames / 180 ms**; 9.6 kb/s × 0.18 = **1728 b** = 1296 voice+FEC + **432 overhead** (25 %).
* DMR burst: 3 AMBE+2 frames = 60 ms = 216 b.
* Re-timing 20 ms ↔ 30 ms: LCM 60 ms (3 ↔ 2).
* Payload: IMBE 18 B; AMBE+2 9 B (×3 = 27); ACELP 18 B (×2 = 35 B).
* Headers dominate: AMBE+2 1 frame/20 ms = 19.6 kbit/s IP for a 3.6 kbit/s codec.
* No transcoding without a licensed vocoder (DVSI); tandem coding degrades; encrypted = opaque.

## Safety / legal
Synthetic audio and published test vectors only. No live public-safety RX.

## Build
`cmake -S code -B build && cmake --build build && ctest --test-dir build`
