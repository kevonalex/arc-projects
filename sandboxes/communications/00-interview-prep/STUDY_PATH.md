# Study path (Phase 0)

Work packs in this order. Aim for one pack per sitting (2–4 h). Attempt the worksheet and failing stubs before opening `answers.md` or `code/solutions/`.

**Interview week priority:** **02 → 04 → 07 → 09 → 01 → 03-core**, then the rest.

## This week

1. `02-bits-and-parsing/` (all sub-packs)
2. `04-voice-over-packets/`
3. `07-land-mobile-radio/`
4. `09-leetcode/` (interleave evenings if needed)
5. `01-cpp-for-protocols/`
6. `03-networking-protocols/` packs `01`–`04` (“03-core”)

Then: remaining net, audio deepenings, DSP, cellular, embedded, capstone M1–M4.

## Full dependency-friendly order

### Block A — C++ foundation
1. `01-cpp-for-protocols/01-toolchain`
2. `01-cpp-for-protocols/02-types-basics`
3. `01-cpp-for-protocols/03-lifetime-move`
4. `01-cpp-for-protocols/04-containers-templates`
5. `01-cpp-for-protocols/05-ub-layout-bit`
6. `01-cpp-for-protocols/06-concurrency-rt`
7. `01-cpp-for-protocols/07-sm-builds`

### Block B — Bits & FEC
8. `02-bits-and-parsing/01-bit-ops`
9. `02-bits-and-parsing/02-endian-seq`
10. `02-bits-and-parsing/03-parse-headers`
11. `02-bits-and-parsing/04-crc-checksum`
12. `02-bits-and-parsing/05-block-fec`
13. `02-bits-and-parsing/06-conv-viterbi`

### Block C — Net + voice
14. `03-networking-protocols/01-headers-layering`
15. `03-networking-protocols/02-udp-tcp-qos`
16. `03-networking-protocols/03-sockets-poll`
17. `03-networking-protocols/04-rtp-sip`
18. `04-voice-over-packets/01-sampling-pcm`
19. `04-voice-over-packets/02-codecs`
20. `04-voice-over-packets/03-realtime-audio`
21. `04-voice-over-packets/04-builds`

### Block D — LMR core
22. `07-land-mobile-radio/01-p25-phase1`
23. `07-land-mobile-radio/02-p25-phase2`
24. `07-land-mobile-radio/03-dmr`
25. `07-land-mobile-radio/04-tetra-dpmr-mpt`
26. `07-land-mobile-radio/05-system-iwf`
27. `07-land-mobile-radio/06-builds`

### Block E — LeetCode patterns (parallel evenings)
`09-leetcode/01-bits` … `13-dp`. Prefer bits, queues-buffers, DP early.

### Block F — Tier 2
28. `03-networking-protocols/05-grpc-security`
29. `03-networking-protocols/06-cellular-encodings`
30–39. `05-dsp-waveforms/01` … `10`
40–41. `06-cellular-2g-6g/01-2g-3g`, `02-4g-lte`
42–45. `08-embedded-realtime/01` … `04`

### Block G — Tier 3 + capstone
46–47. `06-cellular-2g-6g/03-5g-6g`, `04-mcptt-iwf`
48–51. `10-capstone-ptt-gateway/M1` … `M4`

### Deepenings (scaffold — fill when ready)
- `04-voice-over-packets/05-jitter-as-quantile`
- `05-dsp-waveforms/11-ber-gaussian-tail`
- `05-dsp-waveforms/12-viterbi-as-hmm` (twin: `twins/hmm-viterbi.md`)

## Per-pack ritual
1. Prerequisites check — if shaky, jump to the linked pack.
2. Write your own one-sentence “why” before reading misconceptions boxes.
3. Worked example on paper.
4. Questions without answers open.
5. Implement until tests pass (`USE_SOLUTIONS` stays OFF while learning).
6. Cheatsheet from memory.
7. Tick VERIFY items (legacy list: `VERIFY.legacy.md`; also `pack.yaml`).

## Interview story bank
- Why UDP for voice (HOL blocking)
- Why 20 ms frames
- Why RTP timestamps count samples
- Why power-of-two ring buffers
- Why headers get stronger FEC than voice
- Why late entry needs repeated link control
- Why TDMA doubles capacity in 12.5 kHz
- Why acquire/release (concrete reorder)
- Why μ-law / 8 kHz telephony band
- Capstone M1–M4 design decisions
