# Pack 06/03 — Answers (5G NR and 6G)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic data only. **VERIFY** = check 3GPP before quoting.

---

## Q1 — Calculate: numerology table

| μ | SCS | slots/subframe | slots/frame | slot | useful symbol | avg symbol (slot/14) |
|---|---|---|---|---|---|---|
| 0 | 15 kHz | 1 | 10 | 1 ms | 66.67 µs | 71.43 µs |
| 1 | 30 | 2 | 20 | 0.5 ms | 33.33 µs | 35.71 µs |
| 2 | 60 | 4 | 40 | 0.25 ms | 16.67 µs | 17.86 µs |
| 3 | 120 | 8 | 80 | 0.125 ms | 8.33 µs | 8.93 µs |
| 4 | 240 | 16 | 160 | 62.5 µs | 4.17 µs | 4.46 µs |

μ outside 0..4 → −1 (explicit invalid). **Reasoning.** Everything doubles/halves; the subframe is fixed at 1 ms and frame at 10 ms for every μ, which keeps NR and LTE time-aligned.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Slot is always 14 symbols × 66.7 µs" | LTE habit. | Symbol length scales with 1/SCS. |
| "μ=4 is used for data in FR2" | Table has it. | 240 kHz is for SSB only (VERIFY). |

## Q2 — Explain: why SCS scaling shortens symbols

**Answer.** Orthogonal subcarriers each fit an integer number of cycles in the useful symbol time T, so neighbouring subcarriers are `1/T` apart: `SCS = 1/T`. Double the spacing → half the symbol. *Intuition:* a symbol is a window; the narrower the window in time, the wider its spectral main lobe, so the subcarriers must be spread farther apart to stay orthogonal.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Higher SCS = more subcarriers = longer" | More subcarriers sounds like more time. | For fixed bandwidth, wider spacing = **fewer** subcarriers. |

## Q3 — Explain: cost of μ=3; why not in FR1

**Answer.** Gain: 125 µs slots (low latency), Doppler/phase-noise robustness. Cost: shorter CP in absolute time (≈ 0.59 µs at μ=3 vs 4.7 µs), so little delay-spread tolerance; wider PRBs (1.44 MHz) give coarser scheduling granularity; bigger FFT/ADC sample rates. FR1 cells are big with long delay spreads, so large SCS would give ISI; FR2 uses beams (small delay spread) and has severe phase noise — hence 60/120 kHz there.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "μ=3 is simply better" | Lower latency. | Trades away multipath robustness and granularity. |

## Q4 — Calculate: PRBs

**Answer.** 20 MHz@15 → **106** PRB (19.08 MHz, 95.4 %); @30 → **51** (18.36 MHz, 91.8 %); 100 MHz@30 → **273** (98.28 MHz, 98.3 %); 100 MHz@120 → **66** (95.04 MHz, 95 %). A PRB is 12 subcarriers; each is wider at larger SCS, so fewer PRBs fit.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "100 MHz / 180 kHz = 555 PRB" | LTE PRB width. | NR PRB width = 12 × SCS; guard bands also apply. |

## Q5 — Calculate: TDD slots

**Answer.** `slots = period_ms × 2^μ`. 2.5 ms: μ1 → **5**, μ3 → **20**. 5 ms: μ1 → **10**, μ3 → **40**. DDDSU = 3 DL, 1 special, 1 UL slot per 2.5 ms at 30 kHz.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "2.5 ms = 2.5 slots" | LTE 1 slot/ms thinking. | Slots shrink with μ. |

## Q6 — Explain: CU/DU

**Answer.** CU: RRC + PDCP (non-real-time, per-UE; may split CU-CP/CU-UP). DU: RLC, MAC, high PHY (hard real-time scheduling). RU: low PHY + RF/antenna. F1 = CU–DU (F1-C control, F1-U GTP-U user). Why: pool the non-real-time software in a cloud, keep deadline-critical MAC/PHY near the antenna (fronthaul bandwidth/latency limits), enable virtualisation and multi-vendor interop. (Functional splits VERIFY.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "CU = core network" | 'Central'. | CU is still RAN. |

## Q7 — Explain: AMF/SMF/UPF vs LTE

**Answer.** AMF = registration, mobility, NAS (MME's mobility half); SMF = PDU-session management and UPF control (MME's bearer half + control plane of S/P-GW); UPF = user plane (S/P-GW user planes). Separation: scale/deploy independently — many UPFs at the edge, few central AMF/SMFs; mobility changes don't touch session logic; one session may have several UPFs.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "AMF forwards data" | Gateways forward. | Only UPF forwards user packets. |

## Q8 — Explain: SBA

**Answer.** Network functions are **producers/consumers of services** exposed as REST-like APIs (HTTP/2, JSON, TLS/OAuth). The **NRF** registry lets NFs register and discover each other dynamically, so you can scale, add or replace NFs without re-wiring point-to-point reference interfaces with bespoke protocols (Diameter, GTP-C in LTE). Fit for cloud-native deployments.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "SBA applies to the user plane" | 'Network'. | UP stays GTP-U/PFCP-controlled; SBA is the control plane. |

## Q9 — Design: MCPTT slice

**Answer (one design).** Dedicated S-NSSAI (e.g. SST custom/URLLC-like, VERIFY); subscribers via UDM; dedicated SMF/UPF at the edge (local breakout to the MCPTT server / gateway); 5QI for MC voice (e.g. 65/69 in 5G, VERIFY) with preemption/ARP; reserved PRB share and admission control at gNB; isolation by slice-specific policies. Failure mode: slice congested → emergency preemption, fallback to LMR via IWF.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "A slice is a VLAN" | Isolation word. | Slices include RAN resources, NFs, SLAs. |

## Q10 — Explain: coding choices

**Answer.** Data: **LDPC** — parallel decoding, high throughput, good for large blocks and high rates. Control: **polar** — near-optimal at short lengths, CRC-aided list decoding. LTE used turbo (data) and convolutional (control); turbo's serial decoding limited throughput at Gbit/s.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Polar for PDSCH since it's capacity-achieving" | All are 'optimal'. | Practical complexity/latency at big blocks favours LDPC. |

## Q11 — Calculate: base graphs

**Answer.** (200, 0.9) → **BG2** (A ≤ 292). (3000, 0.5) → **BG2** (A ≤ 3824 and R ≤ 0.67). (3000, 0.8) → **BG1** (R > 0.67). (20000, 0.5) → **BG1**. (10000, 0.2) → **BG2** (R ≤ 0.25).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Small block → always BG2" | Size is the headline. | High-rate small blocks use BG1 (BG2's min rate/structure unsuited). |

## Q12 — Calculate: segmentation

**Answer.** BG1 (K_cb 8448): 8448 → **1**; 8449 → **2**; 16848 → **2** (2 × 8424); 16849 → **3**. BG2 (3840): 10000 → ceil(10000/3816) = **3**. After segmentation each block has a 24-bit CRC, hence `K_cb − 24`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "ceil(B / 8448)" | Obvious. | Per-block CRC reduces payload: use 8424. |

## Q13 — Calculate: FSPL

**Answer.** 3.5 GHz @ 1 km = **103.33 dB**; 28 GHz = **121.39 dB** (+18.06 = 20·log10 8). `FSPL = 20log10(d) + 20log10(f) + 20log10(4π/c)`: d×2 → 20·log10 2 = **6.02 dB**; f×10 → **20 dB**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "10·log10 for distance" | Power ratio. | Field ∝ 1/d, power ∝ 1/d² → 20·log10 d. |

## Q14 — Calculate: Shannon

**Answer.** `20e6 × log2(1+100) = 133.2 Mbit/s`; `100e6 × log2(1+10) = 345.9 Mbit/s`. Capacity is linear in bandwidth, logarithmic in SNR: 5× more bandwidth beats 10 dB less SNR. Hence wide mmWave channels. (Shannon is an upper bound: real systems reach ~70–80 % with MCS/overhead, VERIFY.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "SNR dB goes straight into log2" | dB look like exponents. | Convert dB → linear first. |

## Q15 — Calculate/Explain: NTN

**Answer.** LEO 550 km: 1.83 ms one way, FSPL 153.3 dB (2 GHz), Doppler up to ≈ 50 kHz. GEO: 119.4 ms one way, 189.5 dB; Doppler ≈ 0 (nearly static). NR needs: UE GNSS-assisted timing/frequency pre-compensation, extended timers (HARQ, RACH, TA range), more HARQ processes / HARQ disabling, beam/cell handling for moving satellites (VERIFY Rel-17/18/19 details).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "GEO has the biggest Doppler" | Far = big effect. | GEO's relative velocity ≈ 0; LEO moves at 7.5 km/s. |

## Q16 — Interview: 7G and 6G

**Answer (7G).** "There's no 7G: no 3GPP work item, no ITU framework, nothing scheduled — 3GPP is only starting 6G. Anything about 7G is speculation, and I'd rather say that than make something up."
**Answer (6G).** "6G (IMT-2030) is in 3GPP study in Rel-20, with normative work in Rel-21; the plan I've seen is timeline freeze around March 2027, specs about March 2029, and commercial around 2030 — I'd re-check the work plan before quoting dates. Themes: AI-native air interface and network, integrated sensing and communication, non-terrestrial integration, new spectrum (cmWave / sub-THz), energy efficiency." (**VERIFY** every date.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "7G will use terahertz and AI" | Sounds plausible. | Unfounded; inventing features is worse than "no 7G". |
| "6G is already deployed" | Vendors' marketing. | Not standardised yet. |

## Q17 — Etherstack: where does a gateway attach

**Answer.** User plane: behind the UPF (N6/SGi-like), seeing IP/RTP/SIP, plus any local-breakout UPF in the slice. Control: via IMS/MC servers and SBA APIs (NEF/PCF) for QoS requests (Rx/N5). It cannot see the RAN: gNB/CU/DU details, HARQ, beams, or the N3 tunnel; for radio KPIs it needs O&M interfaces.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Attach at the gNB" | Closest to radio. | gNB interfaces are standardised for core, not for third-party gateways. |
