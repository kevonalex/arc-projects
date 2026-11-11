# Cheatsheet — 04-audio / 04 Builds

## WAV (RIFF) — little-endian
```text
0  "RIFF"  4  <file size − 8>   8 "WAVE"   12 "fmt " 16 <fmt=1 ch rate byterate blockalign bits>   36 "data" <n bytes> samples…
byte rate = rate × channels × bytes/sample   block align = channels × bytes/sample   0x1234 is stored 34 12
```
Chunks: `[id 4][size 4 LE][body]`, body padded to **even**; walk with `pos += size + (size & 1)`. Guard with `size > remaining` (subtract, don't add). Reject: short, bad magic, non-PCM16, 0 channels, data before fmt, no data, truncated data, partial frames. Ignore the RIFF size field (0 / 0xFFFFFFFF in streamed files). Unknown chunks (`LIST`, `fact`) are skipped.

## μ-law exhaustive test
Domain = 65 536 inputs + 256 codes ⇒ test **all**. Properties: known vectors (`0→0xFF`, `1000→0xCE→988`, `32767→0x80`, `−32768→0x00`, decode `0x80→32124`) · `enc(dec(c)) == c` for c ≠ 0x7F · `|clip(x) − dec(enc(x))| ≤ step/2`, step = `8 << segment` (8…1024), max error 512 · monotone · odd symmetry for 1…32767.
**Test the test with mutants** — no bias, no clip, `int16_t` negate of −32768 (1 bad input in 65 536), bias 128 (only the error bound sees it), no inversion (vectors), sign error, decoder without `−bias`.

## RTP bytes (big-endian)
```text
80 | M<<7|PT | seq(2) | ts(4) | ssrc(4) | [CSRC 4·CC] | [ext: 4-byte hdr, words in last 16 bits] | payload | [pad … count]
```
`ssrc=0xCAFE seq=500 ts=123456` ⇒ `80 80 01 F4 00 01 E2 40 00 00 CA FE`; next packet `80 00 01 F5 00 01 E2 E0 …` (seq+1, ts+160, M=0).
Parser: len ≥ 12, `(b0>>6)==2`, bounds-check after each add (CSRC, ext, pad); pad count 1…payload. Packetiser: `push` one packet per complete frame (keeps remainder), `flush` pads `0xFF`, `skip_silence(n)`: ts += n, **seq unchanged**, marker armed. ts/seq wrap naturally in `uint32_t/uint16_t`.

## Pipeline
`WAV → PCM → PcmuPacketiser → RTP → simulate_network (seeded LCG: loss draw, delay draw; reordering falls out when spread > spacing) → run_receiver (tick every ptime: push all arrived, rtp_parse (bad ⇒ counted), pop once, 160 samples/slot; Lost/Silence/Buffering = zeros)`.
Map output frames by **timestamp** (`ts / 160`), not by tick. Buffer depth ≳ `⌈delay spread / ptime⌉` (5–40 ms ⇒ 2, 5–80 ms ⇒ 4); each extra packet = +20 ms latency. Perfect network ⇒ **bit-exact** μ-law round trip.

## No-calculator sizing (headers 40 B IPv4, 60 B IPv6, +18 Ethernet, +20 wire)
| Codec @ 20 ms | payload | IP kbit/s | Ethernet kbit/s |
|---------------|---------|-----------|-----------------|
| G.711 | 160 B | **80** | 87.2 |
| G.711 10 ms / 30 ms | 80 / 240 | 96 / 74.7 | |
| G.729 | 20 B | **24** (40 ms: 16) | |
| Opus 24 kbit/s | 60 B | 40 | |
| G.711 over IPv6 | 160 B | 88 | |
`samples = fs·t` · `bytes = bitrate·t/8` · `ts step = clock·t` · 1500-MTU: payload ≤ 1460, G.711 ≤ 182 ms, 48 k stereo16 ≤ 7 ms (20 ms = 3840 B ✗) · wraps: ts 149 h @ 8 k, seq 21.8 min @ 50 pps · drift `fs·ppm·10⁻⁶` (8 k @ 50 ppm = 0.4/s) · SNR `6.02N + 1.76` (16 b 98 dB, 8 b 50 dB) · alias `|f − fs|` (5 kHz @ 8 k → 3 kHz) · mix headroom `20·log₁₀N` · J: `+|D|/16`.

## Run
```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure    # student.* fail until implemented; solution.* pass
```
