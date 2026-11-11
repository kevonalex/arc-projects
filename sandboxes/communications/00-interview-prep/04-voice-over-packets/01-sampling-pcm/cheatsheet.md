# Cheatsheet — Sampling, PCM, sizing

## Sampling
* `x[n] = x(n/fs)`. Tones `f` and `f + k·fs` give **identical** samples; `fs − f` mirrors. Keep signal < **fs/2** (Nyquist) and **filter before** sampling/decimating.
* Fold: `f mod fs`, if `> fs/2` → `fs − r`. (5 kHz @ 8 kHz → 3 kHz.)
* Telephone 300–3400 Hz → fs 8 kHz (600 Hz transition band). Wideband 50–7000 → 16 kHz. Fullband → 44.1/48 kHz.
* Sample period: 8k = 125 µs · 16k = 62.5 µs · 48k = 20.83 µs.

## Quantisation
* Step `Δ = 2A/2^N`; error power `Δ²/12`; **SNR = 6.02·N + 1.76 dB** (full-scale sine). 8 b = 49.9 · 12 b = 74.0 · 16 b = 98.1.
* Quiet signals lose dB-for-dB: −30 dBFS → −30 dB SNR → need ≥ 12 linear bits for 40 dB → G.711 companding.
* float → int16: **round and clip**, never wrap. Mix in int32/float, clamp at the end.
* WAV: LE, 16-bit signed, 8-bit **unsigned**. RTP L16: **big-endian**.

## Packet sizing (memorise)
| Quantity | Formula |
|----------|---------|
| samples/packet | `fs × ptime_ms / 1000` |
| payload (PCM) | `samples × ch × bits / 8` |
| payload (codec) | `bitrate × ptime_ms / 8000` (round up) |
| bandwidth kbit/s | `bytes × 8 / ptime_ms` |
| header cost | `40 B` (v4) → `320/ptime_ms` kbit/s; v6 → `480/ptime_ms` |
| RTP ts step | `clock × ptime / 1000` |
| pps | `1000 / ptime_ms` |

Headers: RTP 12 · UDP 8 · IPv4 20 / IPv6 40 · Eth 14 (+4 VLAN) + FCS 4 · preamble 8 + IFG 12 · min frame 64.

**G.711 / 20 ms:** 160 samples · 160 B · ts +160 · IP 200 B = **80.0** kbit/s · frame 218 B = **87.2** · wire 238 B = **95.2**.

| ptime | 10 | 20 | 30 | 40 |
|-------|----|----|----|----|
| G.711 IP kbit/s | 96 | 80 | 74.7 | 72 |
| G.729 IP kbit/s | 40 | 24 | 18.7 | 16 |

## Traps
* 48 kHz stereo 16-bit 20 ms = 960×2×2 = **3840 B > MTU** → fragments (3), loss ×3, NAT/firewall trouble. Max ptime stereo 7 ms, mono 15 ms.
* 44.1 kHz: only multiples of 10/20 ms are whole samples. 48 kHz: every ms = 48.
* Opus: sample any rate, RTP clock **48 000**. G.722: sample 16 kHz, RTP clock **8000**.
* Serialisation: 1500 B on 64 kbit/s = 187.5 ms; 200 B = 25 ms.
* Backhaul counts: IP-level 80 kbit/s → 12 calls/Mbit; G.729 24 → 41/Mbit (minus RTCP ≈ 5 %).

## Safety
Synthetic sines/silence only. No live public-safety or operator audio.

## Build
`cmake -S code -B build && cmake --build build && ctest --test-dir build`
