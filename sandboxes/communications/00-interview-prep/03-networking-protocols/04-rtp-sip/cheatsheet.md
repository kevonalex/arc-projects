# Cheatsheet — 03-net/04 RTP/RTCP, SIP/SDP
*Each line cites the worksheet part that derived it. ⚠ = VERIFY.*

## RTP header (B)
`V2 P X CC4 | M PT7 | seq16 | ts32 | SSRC32 | CSRC×CC | [ext: profile16 len16(words) data]`
- 12 B fixed. First byte 0x80 for plain; 128–191 range ⚠ for demux vs STUN/DTLS.
- seq: +1/packet, random start, wraps 21.8 min @50 pps → extended seq. `seq_newer = (int16)(a−b) > 0`.
- ts: sample-clock ticks of first octet; G.711 +160/20 ms (8 kHz); Opus +960 (48 kHz ALWAYS); G.722 +160 (clock 8000 ⚠); AMR-WB +320 (16 kHz). Wrap 6.2 d @8 kHz, 24.9 h @48 kHz. `ts_diff = (int32)(a−b)`.
- M: first packet of talkspurt (audio) → safe point to re-base playout. PT 0 PCMU, 8 PCMA, 9 G722, 13 CN, 18 G729; 96–127 dynamic via SDP.
- SSRC random 32 b; collision ≈ N²/2³³; on collision BYE + new SSRC. CSRC = contributors listed by a **mixer**; translator keeps SSRC.
- P: last byte = pad count (≥1, incl. itself). X: BEDE one-byte ext ⚠.

## Silence/DTX (C)
- seq contiguous, ts jumps (gap = last frame + silence ticks), M=1. Needs BOTH seq and ts.

## RTCP (D)
- Port RTP+1 or rtcp-mux. SR=200, RR=201, SDES=202, BYE=203, APP=204.
- SR: 4 hdr + 4 SSRC + 20 sender info (NTP64, RTP ts, pkts, octets); RR no sender info; each block 24 B: `ssrc | frac(8) | cumlost(24, signed) | ext_hi_seq | jitter | LSR | DLSR`. length = words − 1 (SR+1 blk = 12 → 52 B).
- Fraction lost = (lost·256)/expected over the interval (clamp 255). Cum lost = expected − received (can be < 0).
- Jitter (A.8): `D = (R_i−R_{i−1}) − (S_i−S_{i−1})`; `J += (|D|−J)/16`; units = ts ticks; ms = J·1000/clock.
- RTT = A − LSR − DLSR (1/65536 s, middle-32 NTP). LSR=0 → unknown.
- RTCP ≤ 5 % of session bw; Tmin 5 s ⚠ → slow feedback; faster = AVPF (RFC 4585).

## Signalling vs media (E)
- SIP/SDP: reliable, text, via proxies, KBs. RTP: late=lost, binary, direct, 80 kbit/s. Separate for scale, QoS, failure, flexibility. Cost: NAT (IPs in SDP), two things to debug.

## SIP (F)
- INVITE → 100 → 180 → 200 → **ACK** (3-way; UAS resends 200 until ACK) → media → BYE → 200.
- Dialog = Call-ID + From tag + To tag. Transaction = top Via branch (z9hG4bK…) + method. CSeq orders. Response copies Via, From, To(+tag), Call-ID, CSeq; no new tag if To already has one.
- Classes 1xx..6xx; 486 busy, 488 not acceptable here. T1 500 ms, T2 4 s, 64·T1 ⚠.
- Parse: header block ends CRLFCRLF; names case-insensitive; compact v f t i m l c; body = Content-Length bytes.

## SDP offer/answer (G)
- `v o s c t m a`; `m=audio PORT RTP/AVP pt…` preference order; `a=rtpmap:pt name/clock[/ch]` (static PT none); `a=ptime`, direction.
- Answer: same # of m-lines, subset of offered formats, own IP/port, port 0 = reject; match name + clock (case-insens.).
- Hold: re-INVITE `a=sendonly` (answer `recvonly`), `o=` version +1. No common codec → 488.
- Media-level `c=` overrides session-level.
