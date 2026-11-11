# Pack 06/02 — 4G LTE: OFDMA/SC-FDMA, the protocol stack, the EPC, GTP-U and VoLTE

> **SAFETY / LEGAL.** Synthetic signals and arithmetic only. **Never** capture, decode or analyse live cellular traffic
> (including with an SDR + open-source LTE decoders) unless it is your own isolated test network with test SIMs, in a
> shielded or licensed setup. Interception is unlawful in most jurisdictions (UK: Wireless Telegraphy Act 2006,
> Investigatory Powers Act 2016 — VERIFY for yours). Studying **srsRAN**'s architecture is fine; do not copy its code (pack 04).

Standards (from memory — **VERIFY**; I do not invent clause numbers): 3GPP **TS 36.211** (physical channels and modulation),
**36.212/213**, **TS 36.300** (E-UTRAN overview), **TS 36.321 MAC**, **36.322 RLC**, **36.323 PDCP**, **36.331 RRC**,
**TS 24.301** (EPS NAS), **TS 23.401** (EPC architecture), **TS 29.281** (GTP-U), **TS 23.203** (QoS, QCI table), **TS 26.114 / GSMA IR.92** (VoLTE/IMS).

---

## 1. Why this matters (Etherstack context)

Etherstack builds software stacks and **gateways between LMR and broadband**. Broadband public safety (FirstNet, ESN) runs on **LTE** (and increasingly 5G): an MCPTT
call is carried on an LTE **dedicated bearer** (pack 04). Interviewers ask three things: *why OFDM, why SC-FDMA in the uplink, and what does each box and layer do*. You answer by deriving, not by reciting acronyms.

## 2. Prerequisites

Pack 06/01 §3.1/§3.5; DFT/IDFT as "sum of rotating phasors"; `dB = 10 log10(power ratio)`; C++17/GoogleTest in WSL with sanitizers.

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
```

## 3. Concept build-up

### 3.1 Concrete first: a 20 MHz LTE carrier

Numbers first. Subcarrier spacing **15 kHz**; a **resource block (RB)** = **12 subcarriers = 180 kHz** (`kScsHz`). A 20 MHz channel holds **100 RB** → `100 × 180 kHz = 18 MHz`
occupied; the remaining 2 MHz are **guard bands** (90 % usage — the filter skirts need room). Standard widths (`rb_for_bandwidth_mhz`): 1.4 → 6, 3 → 15, 5 → 25, 10 → 50, 15 → 75, **20 → 100**. Anything else (7 MHz) → −1.

The transmitter creates the signal with an **IFFT**. Pick the smallest power of 2 (or 1536) with `FFT × 15 kHz ≥ occupied` and a margin: 100 RB → **2048**, sample rate `2048 × 15 kHz = 30.72 Msps` (`sample_rate_hz`); 6 RB → 128 → 1.92 Msps; 75 RB → **1536** (a non-power-of-two FFT: LTE accepts it).
One OFDM **useful symbol** lasts `1/15 kHz = 66.667 µs` (`useful_symbol_us`). *Why that spacing?* Subcarrier spacing Δf and symbol time T are tied: `T = 1/Δf`. Smaller spacing → longer symbols → more robust to multipath, but more sensitive to Doppler and phase noise (§3.2).

> **Misconception box — "20 MHz means 20 MHz of subcarriers".** Only 18 MHz carry data; the rest is guard.

### 3.2 Cyclic prefix, resource elements, and a peak-rate ceiling

Multipath: a radio wave arrives via several paths with delays up to a few µs. If symbols just follow each other, an echo of symbol *n* lands on symbol *n+1* (inter-symbol interference).
Fix: copy the **last part of each symbol to its front** — the **cyclic prefix (CP)**. As long as the echo delay < CP, the receiver sees a *circular* convolution, which one complex multiply per subcarrier undoes. (This is why OFDM equalisation is so cheap — the whole point.)

Normal CP: first symbol in a 0.5 ms slot has 160 samples (at FFT 2048), the other six have 144: `cp_samples`. Durations: **5.208 µs** and **4.6875 µs**.
Slot = `160 + 6·144 + 7·2048 = 15 360 samples = 0.5 ms` (`slot_samples`). *Cost:* CP is ≈ 7 % overhead — time the receiver cannot use for data. Extended CP (16.67 µs, 6 symbols/slot) exists for big cells/MBSFN (VERIFY).

A **resource element (RE)** is one subcarrier × one symbol. Per 1 ms subframe: `nRB × 12 × 14` → **16 800 RE** for 100 RB (`resource_elements_per_subframe`).
`raw_peak_mbps` multiplies REs per ms × bits/symbol × layers: 64QAM (6 bits), 1 layer: **100.8 Mbit/s**; 4 layers: **403.2 Mbit/s**. This is a *ceiling*, not a throughput: the real rate is lower because of reference
signals, control channels (PDCCH), coding rate, overhead (≈ 25–30 %), and Cat-limits (VERIFY). **Never quote this figure as "LTE speed".**

> **Misconception box — "peak rate = what users get".** Users get the ceiling × (1 − overhead) × coding rate × (their share of the RBs) × (their SINR fraction).

### 3.3 OFDMA downlink vs SC-FDMA uplink — derive PAPR

**OFDMA (downlink).** Many subcarriers each carry a QPSK/QAM symbol; time signal = sum of N phasors with random phase. By the central limit theorem the time waveform is **Gaussian-like**:
mostly small, with occasional big peaks when many subcarriers line up. **PAPR** = `peak power / average power` (`papr_db`). OFDM PAPR is typically **8–12 dB** for hundreds of subcarriers.
Why does that hurt? A power amplifier is linear only up to a limit. Peaks must fit inside the linear region, so the PA runs **backed off** by roughly the PAPR: the average output power is 8–12 dB below the PA's capacity. PA efficiency collapses with back-off — fine for a mains-powered tower, **bad for a battery handset**.

**SC-FDMA (uplink)** = "DFT-spread OFDM". Put an **M-point DFT** *before* the IFFT. The IFFT then undoes the DFT (when mapped onto contiguous bins) and the time signal is essentially the **original single-carrier symbol sequence**, which has a low PAPR (constant envelope for QPSK, a few dB for 16QAM).
`scfdma_modulate(q, N, first_bin)`: DFT(M) → place on M bins → IDFT(N). `Test ScFdma.NoSpreadingWhenMEqualsN`: M = N ⇒ the output **equals the input**. `Test LowerPaprThanOfdmOnAverage`: 64 QPSK symbols on a 256-point IFFT: OFDM mean PAPR > 7 dB; SC-FDMA at least 1.5 dB lower.

| | OFDMA | SC-FDMA |
|---|---|---|
| Used in | LTE downlink | LTE uplink (NR uplink: either, §3.7 pack 03) |
| Time waveform | Gaussian-like | ≈ single carrier |
| PAPR | high (≈ 8–12 dB) | lower (≈ 4–7 dB for QPSK/16QAM; VERIFY) |
| Why chosen | cheap equalisation, flexible RB scheduling | cheaper/efficient handset PA, longer battery |
| Cost | power-hungry PA | slightly more complex receiver (needs frequency-domain equalisation + IDFT) |

> **Misconception box — "SC-FDMA is a different multiplexing scheme".** It is OFDM with one extra DFT. Users still occupy distinct RBs (frequency multiple access), which is why it is *also* "FDMA".
> **Misconception box 2 — "lower PAPR = lower power".** It means the PA can run closer to saturation with the same linearity → **more efficient**, not "less energy by magic".

### 3.4 The radio protocol stack — one verb per layer

A rule that makes every layer easy to remember: **a layer talks to its peer entity** (same layer on the other side); everything below is "someone else's problem".

| Layer | Purpose (one verb) | Peer lives in | Example in this pack |
|---|---|---|---|
| **PHY** | transmit bits over the air (modulation, coding, HARQ combining) | eNB | `Modulation` |
| **MAC** | multiplex logical channels, **schedule**, HARQ retransmission | eNB | `Scheduling`, `HarqRetransmission` |
| **RLC** | segment/reassemble, **ARQ** (retransmit lost PDUs) | eNB | `ArqAndSegmentation` |
| **PDCP** | **cipher**, integrity, header compression (ROHC), in-order delivery | eNB | `CipheringAndRohc` |
| **RRC** | radio **connection** control: setup, bearers, handover, measurements | eNB | `RadioBearerReconfiguration` |
| **NAS** | **attach, authentication**, session management — *transparent to the eNB* | **MME** | `AuthenticationAndAttach` |

`layer_for`, `peer_node`: encode this table. Key insight: the **eNB terminates PHY–RRC; the MME terminates NAS**. NAS messages ride **inside RRC** across the air but are opaque to the eNB. *Why two retransmission layers (HARQ and ARQ)?*
HARQ (MAC) is fast (8 ms RTT), retries a few times with soft combining, but occasionally gives up; RLC ARQ catches the rest. *Fast and cheap first, slow and sure second.*

### 3.5 EPC and GTP-U

| Node | Role |
|---|---|
| **eNB** | the base station (radio + scheduling) |
| **MME** | signalling brain: NAS, attach, paging, tracking area, bearer setup (`NasSignallingAndPaging`) |
| **HSS** | subscriber database and authentication vectors (`SubscriberDatabase`) |
| **S-GW** | **local mobility anchor**: user-plane tunnel endpoint that stays while the UE hands over between eNBs (`HandoverUserPlaneAnchor`) |
| **P-GW** | the **IP anchor**: assigns the UE its IP, applies policy/charging, connects to the Internet/IMS (`UeIpAddressAndInternetGateway`) |

**Control plane vs user plane split:** the MME never touches user packets; S-GW/P-GW never do NAS. This split is the template that 5G sharpens (AMF vs UPF, pack 03).

**QCI** = QoS Class Identifier: a label for a bearer's treatment. Packet delay budgets (`qci_delay_budget_ms`, TS 23.203 — **VERIFY**): QCI 1 (conversational voice) **100 ms**, QCI 5 (IMS signalling) 100 ms, QCI 9 (default best-effort) **300 ms**,
QCI 65 (**MCPTT voice**) **75 ms**, QCI 69 (MCPTT signalling) 60 ms. Unlisted → −1.

**GTP-U (GPRS Tunnelling Protocol, user plane).** User IP packets are **tunnelled** between eNB ↔ S-GW ↔ P-GW so the *core* need not know the UE's changing location: `outer IP | UDP (port 2152) | GTP-U header | inner user IP packet`.
Overhead: outer IPv4 20 + UDP 8 + GTP-U 8 = **36 bytes**; with optional fields (sequence number / N-PDU / extension header) +4 = **40** (`gtpu_overhead_bytes`). On a 1500-byte link the inner MTU is **1464** (1460 with optionals) (`gtpu_inner_mtu`) — the reason tunnelled networks need MTU planning or MSS clamping.
The 8-byte base header carries version/PT/flags (E,S,PN), message type (**0xFF = G-PDU**), length and the 32-bit **TEID** (tunnel endpoint ID). Parsing it is Pack 04's exercise.

> **Misconception box — "GTP-U is encrypted".** It is **not**: it is plain encapsulation. Confidentiality across the backhaul needs IPsec; radio-side confidentiality is PDCP (§3.4).

### 3.6 VoLTE — voice is just a QCI-1 data flow

LTE has no circuit-switched voice. **VoLTE** = voice over IMS/SIP + RTP with a **dedicated bearer** (QCI 1) next to the default bearer. Sizing a call: **AMR-NB 12.2 kbit/s** = a 31-byte payload every 20 ms.
Headers: RTP 12 + UDP 8 + IPv4 20 = **40 bytes** (IPv6: **60**; `rtp_udp_ip_header_bytes`). Rate at IP: payload-only `31 × 8 / 20 ms = 12.4 kbit/s`; with headers `71 × 8 / 20 ms = **28.4 kbit/s**` — headers more than **double** the load (the payload is smaller than its envelope!).
**ROHC** (in PDCP) compresses RTP/UDP/IP down to ~3 bytes on the air: `34 × 8 / 20 = 13.6 kbit/s` (`ip_voice_bitrate_bps`). Also: **SPS** (semi-persistent scheduling) avoids signalling a grant every 20 ms; **TTI bundling** repeats uplink subframes for cell-edge voice (VERIFY).

> **Misconception box — "bigger payload = better for voice".** Larger ptime lowers header overhead but adds **delay** and makes a lost packet cost more audio. 20 ms is the voice default (capstone M2 §3.2).

## 4. Worked examples

**(a) PAPR of a spike:** samples `(2,0,0,0)`: peak power 4, mean `4/4 = 1` → `10 log10 4 = 6.02 dB`. A constant-envelope QPSK ring: peak = mean → 0 dB.
**(b) 10 MHz MCPTT cell:** 50 RB → FFT 1024 → 15.36 Msps; REs/subframe = 8400; raw QPSK 1 layer = `8400 × 2 / 1000 = 16.8 Mbit/s` ceiling.
**(c) Tunnel MTU:** a phone sends a 1460-byte IP packet; add 36 bytes GTP-U/UDP/IP = 1496 ≤ 1500: OK. At 1500 inner it would fragment: 1536 > 1500.

## 5. Questions

1. **Q1 (Calculate)** — *20 MHz carrier.* RBs, FFT size, sample rate, occupied bandwidth, guard-band fraction. (`rb_*`, `fft_*`, `sample_rate_hz`, W 3.1)
2. **Q2 (Explain)** — Why does `T = 1/Δf`? What would you change for a very high-speed train (Doppler)? (W 3.1)
3. **Q3 (Calculate)** — CP lengths at FFT 128 and 2048; slot sample count; CP overhead percentage. (W 3.2)
4. **Q4 (Explain)** — Why a **cyclic** prefix and not silence (a guard interval of zeros)? (W 3.2)
5. **Q5 (Calculate)** — RE count and raw ceiling for 25 RB QPSK 1 layer; why this is not the user's throughput. (W 3.2)
6. **Q6 (Explain)** — Why does OFDM have a high PAPR? Why does the PAPR matter more in the uplink? (W 3.3)
7. **Q7 (Derive)** — Show that DFT-spread-then-IDFT with M = N returns the input; what does that say about PAPR? (W 3.3)
8. **Q8 (Explain/Interview)** — "Why SC-FDMA for the uplink?" — a 45-second answer. (W 3.3)
9. **Q9 (Classify)** — Map seven functions to PHY/MAC/RLC/PDCP/RRC/NAS and name each layer's peer node. (W 3.4)
10. **Q10 (Explain)** — Why both HARQ and RLC ARQ? (W 3.4)
11. **Q11 (Explain)** — MME vs S-GW vs P-GW vs HSS: one sentence each; draw the control/user-plane split. (W 3.5)
12. **Q12 (Calculate)** — GTP-U overhead and inner MTU on a 1500-byte link (with and without optionals). Why is GTP-U not "secure"? (W 3.5)
13. **Q13 (Explain)** — What is a QCI and why does MCPTT voice (QCI 65) have a tighter budget than default data? (W 3.5; VERIFY table)
14. **Q14 (Calculate)** — VoLTE AMR-NB 12.2: IP-layer bit rate without headers, with 40-byte headers, with ROHC (3 bytes). (W 3.6)
15. **Q15 (Etherstack/Design)** — A gateway sits on the P-GW SGi side: what does it see of the radio layers, and what can it NOT see? (W 3.4–3.6)

## 6. Self-assessment

- [ ] Derive 100 RB → 2048-pt FFT → 30.72 Msps → 15 360-sample slot from scratch.
- [ ] Say in one sentence each what PHY, MAC, RLC, PDCP, RRC, NAS do and where their peers are.
- [ ] Explain PAPR → PA back-off → battery in three sentences.
- [ ] Draw eNB/MME/HSS/S-GW/P-GW with S1-MME, S1-U, S5/S8, SGi labelled (VERIFY interface names).
- [ ] Compute VoLTE header overhead and say why ROHC exists.

**VERIFY list:** channel-bandwidth/RB table (TS 36.101); CP lengths at each FFT; typical OFDM/SC-FDMA PAPR figures; QCI 1/5/9/65/69 delay budgets (TS 23.203); GTP-U header fields (TS 29.281); HARQ RTT of 8 ms (FDD); extended CP duration; interface names (S1-MME, S1-U, S5/S8, SGi, S6a); VoLTE profile details (GSMA IR.92).
