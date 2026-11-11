# Pack 06/02 — Answers (4G LTE)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic data only. **VERIFY** = check 3GPP before quoting.

---

## Q1 — Calculate: 20 MHz carrier

**Answer.** 100 RB; FFT **2048**; sample rate `2048 × 15 kHz = 30.72 Msps`; occupied `100 × 12 × 15 kHz = 18 MHz`; guard `2/20 = 10 %`.

**Reasoning.** Guard bands let the transmit filter roll off without leaking into the adjacent carrier. FFT size is chosen as the smallest convenient value whose sample rate covers the occupied band with margin.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "20 MHz = 1333 subcarriers" | 20 MHz / 15 kHz. | Guard bands: 1200 used subcarriers (100 × 12). |
| "FFT 1024 is enough" | Nearest power of two to the 1200 used subcarriers. | 1024 × 15 kHz = 15.36 MHz < 18 MHz occupied: the sample rate must cover the band. |

## Q2 — Explain: T = 1/Δf

**Answer.** Subcarriers must be **orthogonal**: each completes an integer number of cycles in T, so the spacing is a multiple of 1/T. Smaller Δf → longer T → relatively smaller CP overhead and better multipath tolerance, but a Doppler shift becomes a larger fraction of Δf and breaks orthogonality (ICI). High-speed trains: larger spacing (NR does this — pack 03) or Doppler compensation.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Smaller spacing is always better" | Longer symbols are robust. | Doppler/phase-noise sensitivity grows. |

## Q3 — Calculate: CP and slot

**Answer.** FFT 128: first CP = 160 × 128/2048 = **10**, others **9** samples. FFT 2048: **160 / 144**. Slot = `160 + 6·144 + 7·2048 = 15 360` (128: 960). CP overhead `(160 + 864)/15 360 = 6.67 %`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "All CPs 144" | Most are. | Symbol 0 is longer so the slot totals exactly 0.5 ms (15 360). |
| "CP is 20 % overhead" | Thinking extended CP. | Normal CP ≈ 7 %. |

## Q4 — Explain: cyclic vs zero guard

**Answer.** A cyclic prefix makes the linear convolution with the channel look **circular** over the FFT window, so each subcarrier is just multiplied by one complex gain (equalised by a one-tap divide) and **orthogonality survives** the echoes. A zero gap prevents ISI but not the loss of orthogonality: echoes in the window are no longer a whole number of cycles ⇒ inter-carrier interference.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "CP is just a guard time" | The name. | Its *content* (a copy) is what preserves orthogonality. |

## Q5 — Calculate: 25 RB ceiling

**Answer.** `25 × 12 × 14 = 4200` RE/subframe; QPSK 2 bits, 1 layer: `4200 × 2 / 1000 = 8.4 Mbit/s`. Reality: reference signals, PDCCH, coding, retransmissions, sharing among users and SINR reduce this heavily.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "8.4 Mbit/s is what the user gets" | It's the output of a formula. | It's an upper bound before overhead and coding. |

## Q6 — Explain: OFDM PAPR

**Answer.** The time signal is a sum of N independent-phase subcarriers ⇒ approx. Gaussian; rare constructive alignment gives peaks of order N× average power (PAPR up to 10·log10 N, typically 8–12 dB observed). Uplink: the handset's PA must be linear at the peaks, so it backs off, wasting battery; downlink eNB has mains power and big PAs.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "High PAPR means more power" | "High". | It means peaks high relative to average; average power is the same. |
| "PAPR depends on the modulation order only" | QAM has peaks. | Number of subcarriers and their independence dominate. |

## Q7 — Derive: M = N identity

**Answer.** `x' = IDFT_N(DFT_N(x)) = x`. For M < N the IDFT of the mapped, zero-padded spectrum interpolates the original sequence, so the envelope follows the single-carrier symbol stream (plus interpolation ripple), keeping PAPR close to that of the QAM constellation itself.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "SC-FDMA has zero PAPR" | The M = N case is constant-envelope for QPSK. | With M < N pulse shaping/interpolation adds ripple and 16QAM has its own PAPR. |

## Q8 — Interview: why SC-FDMA uplink (model answer)

"The uplink is limited by the handset's battery and PA efficiency. OFDM's waveform has a high peak-to-average ratio, so the PA must back off several dB. SC-FDMA adds a DFT before the IFFT so the transmitted waveform is close to single-carrier with lower PAPR; the PA runs closer to saturation, more efficient, longer battery and range. We still keep frequency-domain multiple access and cheap frequency-domain equalisation at the eNB, where complexity is affordable. The downlink stays OFDMA because the eNB has power to spare and benefits from simple one-tap equalisation in the UE."

## Q9 — Classify: layers

**Answer.** Modulation → **PHY** (peer eNB); HARQ retransmission → **MAC** (eNB); Scheduling → **MAC**; ARQ/segmentation → **RLC** (eNB); Ciphering/ROHC → **PDCP** (eNB); Radio-bearer reconfiguration → **RRC** (eNB); Authentication/attach → **NAS** (**MME**).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "NAS peer is the eNB" | NAS rides over RRC to the eNB. | The eNB forwards it transparently; MME terminates it. |
| "Ciphering is PHY" | PHY 'protects' bits. | Ciphering is PDCP; PHY only does error-correcting coding. |
| "HARQ is RLC" | Both retransmit. | HARQ = MAC/PHY soft-combining; ARQ = RLC. |

## Q10 — Explain: HARQ + ARQ

**Answer.** HARQ is fast (every ~8 ms, with soft combining raising success per retry) but tuned for ≈10 % residual BLER and can give up; RLC ARQ is slower, byte-exact, and fixes the residual errors. Two layers because the cheap fast loop handles most errors with low delay, and the slow reliable loop handles the rest without making HARQ conservative.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Redundant: pick one" | Both retransmit. | Different timescale and reliability target. |

## Q11 — Explain: EPC nodes

**Answer.** **MME**: NAS signalling, attach, authentication orchestration, paging, bearer setup — control plane only. **HSS**: subscriber data/keys. **S-GW**: user-plane anchor during eNB handovers. **P-GW**: UE IP address, policy/charging enforcement, SGi to the PDN. Control plane (MME↔eNB S1-MME, HSS S6a) never carries user packets; user plane (eNB–S-GW–P-GW via GTP-U) never carries NAS (interface names VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "S-GW assigns the IP" | It sounds like the gateway. | The P-GW allocates the UE address. |
| "MME forwards user data" | "Mobility". | MME is signalling only. |

## Q12 — Calculate: GTP-U

**Answer.** Overhead 20 + 8 + 8 = **36 B**, **40 B** with optional fields. Inner MTU: **1464** / **1460**. Not secure: GTP-U is plain encapsulation; protect the backhaul with IPsec or a private transport, and radio with PDCP ciphering.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "GTP header is 12 bytes" | Mixing with GTPv1-C or optional fields. | Mandatory part is 8; with optionals 12 (total outer 40). |
| "Inner MTU 1500" | Same link. | The tunnel eats 36 bytes. |

## Q13 — Explain: QCI

**Answer.** A QCI selects a standardised treatment (resource type GBR/non-GBR, priority, packet delay budget, loss rate). MCPTT voice (QCI 65, 75 ms) and signalling (QCI 69, 60 ms) get high priority and low delay because PTT floor changes and speech start must be perceived instantly, and preemption of lesser traffic is allowed. Default data (QCI 9, 300 ms) tolerates buffering. (Values VERIFY in TS 23.203.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "QCI sets the bit rate" | QoS ⇒ rate. | GBR/MBR are separate parameters; QCI sets class. |

## Q14 — Calculate: VoLTE rate

**Answer.** Payload only `31×8/0.02 = 12 400 bit/s`; with 40-byte headers `(31+40)×8/0.02 = 28 400`; with ROHC 3 B `34×8/0.02 = 13 600`. Headers are 130 % overhead; ROHC makes it 10 %.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "12.2 kbit/s on the wire" | Codec name. | 12.2 is codec core; 31 B × 8/20 ms = 12.4 includes the payload header. |

## Q15 — Etherstack/Design: gateway at SGi

**Answer.** It sees IP packets of one UE per bearer (default or dedicated MCPTT), SIP/RTP, DSCP; sees nothing of PHY/MAC/RLC/PDCP/RRC, HARQ, radio quality, or the GTP tunnel (terminated at the P-GW). To learn radio health it needs a side channel (e.g. exposed RAN/EPC metrics or RTCP stats). Design consequence: the capstone gateway measures loss/jitter from RTP/RTCP, not from the radio.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The gateway can read the UE's CQI" | CQI is on the network. | It terminates in the eNB (MAC/PHY); not visible at SGi. |
