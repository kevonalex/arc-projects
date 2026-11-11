# Pack 06/03 — 5G NR and a look at 6G: numerology, architecture, coding, link physics, "7G does not exist"

> **SAFETY / LEGAL.** Synthetic signals and arithmetic only. Never capture or decode live cellular traffic; never transmit on licensed
> spectrum. Test with your own isolated 5G test network (test SIM/PLMN, shielded) only. UK: Wireless Telegraphy Act 2006 — VERIFY for your jurisdiction.

Standards (from memory — **VERIFY**; I never invent clause numbers): 3GPP **TS 38.211** (physical channels, numerology), **38.212** (multiplexing and channel coding), **38.213/214**,
**TS 38.300** (NR overview), **TS 38.401** (RAN architecture, CU/DU), **TS 23.501** (5GS architecture), **TS 23.502** (procedures), **TS 33.501** (5G security), **TS 38.101-1/-2** (UE RF, channel-bandwidth/PRB tables),
**TR 38.811 / 38.821** (non-terrestrial networks). Timeline source: 3GPP Release 20/21 work plan (**VERIFY — dates below are from the brief and may move**).

---

## 1. Why this matters (Etherstack context)

Public-safety broadband is moving from LTE to 5G (standalone core, **slicing for MCPTT**, URLLC, NTN fallback). An engineer who joins a radio-stack company is expected to explain *what changed from LTE and why* —
not list acronyms. This pack gives you the four derivations that cover most interviews: **numerology** (why scaling the subcarrier spacing shortens symbols), **architecture** (what CU/DU, AMF/SMF/UPF and SBA buy),
**coding** (LDPC vs polar), and **link physics** (why mmWave is short-range and satellite is hard). It also teaches you to say **"there is no 7G"** — and why — without hedging.

## 2. Prerequisites

Pack 06/02 (§3.1–3.2: OFDM, CP, RB); logarithms; C++17/GoogleTest in WSL with sanitizers.

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
```

## 3. Concept build-up

### 3.1 Numerology — derive it

LTE had **one** subcarrier spacing (15 kHz). 5G NR is built for very different services: wide-area coverage at 700 MHz *and* 400 MHz-wide channels at 28 GHz *and* ultra-low latency. One spacing cannot suit all.
So NR uses **scalable numerology**: `SCS = 15 · 2^μ kHz` with μ = 0…4 → **15, 30, 60, 120, 240 kHz** (`scs_khz`; out of range → −1).

Why does doubling the SCS **shorten the symbol**? Orthogonality requires each subcarrier to complete a whole number of cycles in the useful symbol time `T`, so `T = 1/SCS` (`symbol_useful_us`):
**66.67 µs (μ=0), 33.33 µs (μ=1), 16.67 (μ=2), 8.33 µs (μ=3)**. *Doubling Δf halves T.* The subframe stays **1 ms** for every μ; so a subframe holds `2^μ` **slots**, each with **14 symbols** (normal CP):
`slot_ms = 1/2^μ`: 1, 0.5, 0.25, **0.125 ms** (`slots_per_subframe`, `slots_per_frame = 10·2^μ`, `symbols_per_slot = 14`). Including the CP, the **average** symbol is `slot/14`: **71.43 µs** (μ=0), 35.71 (μ=1), 8.93 µs (μ=3) (`symbol_avg_us`).

*What do you gain and lose by shortening symbols?*
| Larger SCS (μ↑) | Why |
|---|---|
| ✔ lower latency (slot = 125 µs at μ=3) | a transmission opportunity every 0.125 ms, shorter scheduling unit |
| ✔ tolerates more Doppler and phase noise | frequency error is a smaller fraction of Δf (§3.6) |
| ✘ shorter CP in absolute time (CP scales with T) | less multipath-delay tolerance: fine for small cells and mmWave beams |

> **Misconception box — "5G is faster because the symbols are shorter".** Shorter symbols mean **lower latency and robustness to Doppler**, not more bits per second by themselves. Throughput comes from **bandwidth** (up to 100 MHz FR1 / 400 MHz FR2 per carrier),
> **MIMO layers** and **modulation order**. A PRB is always 12 subcarriers, so at μ=3 it is **wider** (12 × 120 kHz = 1.44 MHz) and the PRB **count** in a given bandwidth falls (§3.2) — the same MHz carry about the same data.

### 3.2 Frequency ranges, PRB counts and TDD

* **FR1** (sub-7 GHz, nominally 410 MHz–7.125 GHz — VERIFY): data SCS 15/30/60 kHz. **FR2** (24.25–52.6 GHz, "mmWave" — VERIFY): 60/120 kHz (`numerology_allowed`; μ=3 is **not** allowed in FR1; μ=1 is not allowed in FR2). Why? FR2 needs wide SCS because phase noise is worse at high carrier frequency and Doppler scales with frequency.
* A **PRB** = 12 subcarriers. `prb_count(bw, μ)` table (TS 38.101 — **VERIFY**): 20 MHz @15 kHz → **106**; 20 MHz @30 kHz → **51**; 100 MHz @30 kHz → **273**; 100 MHz @120 kHz → **66**; 400 MHz @120 kHz → **264**. Check: `106 × 12 × 15 kHz = 19.08 MHz` (95 % usage — better than LTE's 90 %); `273 × 12 × 30 kHz = 98.28 MHz`.
* **TDD pattern:** `slots_in_period(period_ms, μ) = period × 2^μ`: the classic 2.5 ms **DDDSU** at 30 kHz SCS = **5 slots** (D = downlink, S = special/flexible, U = uplink); 5 ms at 120 kHz = 40 slots.

### 3.3 Architecture: CU/DU, 5GC, SBA, slicing

**RAN: gNB split.** The gNB is decomposed into **CU** (Central Unit: RRC + PDCP, possibly split CU-CP / CU-UP), **DU** (Distributed Unit: RLC + MAC + high-PHY), and **RU** (radio unit: low-PHY + RF). CU–DU interface **F1**; DU–RU is eCPRI/fronthaul (VERIFY split 7.2x).
*Why split?* Time-critical, per-cell processing (MAC scheduling every 125 µs) stays near the antenna; non-real-time, per-UE functions (PDCP, RRC) can be **pooled in a datacentre** serving many DUs, which also eases virtualisation and mobility between DUs.

**Core (5GC), by function — LTE ↔ 5G mapping:**
| 5GC NF | Does | LTE ancestor |
|---|---|---|
| **AMF** | access & mobility, **NAS** termination, registration, paging | MME (signalling half) |
| **SMF** | **session** management, IP allocation, UPF control | MME (bearer) + S/P-GW control plane |
| **UPF** | user-plane forwarding, GTP-U, QoS enforcement, anchor | S-GW + P-GW user plane |
| **UDM / AUSF** | subscriber data / authentication | HSS |
| **PCF, NRF, NSSF** | policy, **NF registry/discovery**, slice selection | PCRF, — , — |

**Control-/user-plane separation (CUPS) is mandatory** in 5GC: AMF/SMF never touch user packets; UPFs are placeable at the edge (low latency, local breakout — useful for a mission-critical gateway on-premises).
**SBA (service-based architecture):** NFs expose **services** over **HTTP/2 + JSON** (`Namf_Communication`, `Nsmf_PDUSession`, …); the **NRF** lets one NF discover another. Contrast LTE's point-to-point reference interfaces with bespoke protocols (GTP-C, Diameter).
User plane stays on GTP-U over **N3** (gNB–UPF); N2 = NGAP gNB–AMF; N4 = PFCP SMF–UPF (VERIFY).
**Network slicing:** a **slice** = a logical end-to-end network (specific NFs + resources) selected by **S-NSSAI = SST + SD** (SST 1 eMBB, 2 URLLC, 3 MIoT — VERIFY). A public-safety operator can run an **MCPTT slice** with guaranteed capacity and a local UPF while consumers share another slice.

> **Misconception box — "slicing is a QoS marking".** QoS flows (5QI) differentiate packets *within* a session. A slice differentiates **network resources and functions**; it can have its own SMF/UPF, its own admission control and SLA.

### 3.4 Channel coding: LDPC for data, polar for control

LTE used **turbo** codes for data and tail-biting convolutional for control. NR: **LDPC** for PDSCH/PUSCH; **polar** for PDCCH/PBCH (`channel_code_for`).
*Why LDPC for data?* Peak rates of several Gbit/s need **massively parallel decoding**: LDPC's sparse parity-check graph decodes with many independent node updates per iteration, and it scales to large blocks and high rates with low complexity.
*Why polar for control?* Control messages are **short** (tens to hundreds of bits). Polar codes (capacity-achieving; list decoding with CRC) are excellent at short block lengths where LDPC/turbo lose efficiency.
**Two base graphs.** BG1 (large, up to 8448-bit blocks, rate ≥ 1/3) and BG2 (smaller, 3840 bits, rate ≥ 1/5): selection rule (`ldpc_base_graph`, TS 38.212 §7.2.2 — **VERIFY**): **BG2 if A ≤ 292, or R ≤ 0.25, or (A ≤ 3824 and R ≤ 0.67)**; else BG1.
**Segmentation:** if the transport block (plus 24-bit CRC) `B > K_cb`, split into `ceil(B/(K_cb − 24))` code blocks (each gets a CRC-24; `num_code_blocks`): BG1: 8448 → 1, **8449 → 2**, 16848 → 2, **16849 → 3**; BG2: 3840 → 1, 10000 → `ceil(10000/3816) = 3`.

### 3.5 Link physics 1: path loss and why mmWave is short range

Free-space path loss: `FSPL(dB) = 20·log10(4π·d·f / c)` (`fspl_db`). **Two rules of thumb you can derive:** doubling the distance adds **+6.02 dB**; multiplying the frequency by 10 adds **+20 dB**.
Numbers: 2.4 GHz @ 1 m = **40.05 dB**; 3.5 GHz @ 1 km = **103.33 dB**; 28 GHz @ 1 km = **121.39 dB** (18 dB worse for the same distance).
*Why is the higher frequency worse in free space if the wave "doesn't care"?* The path loss formula is really *a fixed-size receive antenna aperture*: `λ²/4π` shrinks as `f` grows, so a fixed-size antenna catches less. A **phased array with many elements** (massive MIMO) brings the aperture back (beamforming gain) — at the price of narrow beams that must be **steered and tracked**, and extra losses (rain, foliage, walls, body blocking) that FR2 suffers badly.
Shannon bound: `C = B·log2(1 + SNR)` (`shannon_capacity_bps`): 20 MHz at 20 dB → **133.2 Mbit/s**; 100 MHz at 10 dB → **345.9 Mbit/s** — bandwidth is a *linear* lever, SNR only logarithmic: **more MHz beats more dB**; this is why 5G chases wide mmWave channels.

> **Misconception box — "the wave itself attenuates more at high frequency in vacuum".** Free-space loss at fixed *antenna gain* is frequency-independent; the extra loss comes from the receiving aperture size (and, in air, from absorption/blockage).

### 3.6 Link physics 2: delay, Doppler, non-terrestrial networks (NTN)

Delay = distance/c (`one_way_delay_ms`): LEO at 550 km → **1.83 ms** one way; GEO 35 786 km → **119.4 ms** (a round trip ≈ 240 ms: **HARQ timers and RACH windows must be extended**). Doppler `f_d = v·f/c` (`max_doppler_hz`): train at 500 km/h on 3.5 GHz → **1.62 kHz** (5.4 % of 30 kHz — OK); LEO satellite at 7.56 km/s on 2 GHz → **50.4 kHz** — more than 3× the 15 kHz SCS!
Consequence: NTN needs **UE (GNSS-based) pre-compensation** of Doppler and timing, extended timers, and large TA ranges. FSPL: LEO @2 GHz 550 km = **153.28 dB**; GEO = **189.54 dB**: satellite links are *power-hungry* and rely on directional antennas.
*Etherstack angle:* public-safety coverage extension (remote areas, disaster relief) is the NTN story; hand-held *direct-to-device* is limited by exactly these numbers (VERIFY the Rel-17/18/19 NTN features).

### 3.7 Generations: 6G and why there is no 7G

* **1G** analogue, **2G** GSM (pack 01), **3G** WCDMA (pack 01), **4G** LTE (pack 02), **5G** NR (this pack) — "G" is an ITU/industry marketing label for a 3GPP *era*, not a spec number.
* **6G** (IMT-2030): in **3GPP Release 20** the study phase (first study items began 2025) and **Release 21** is intended to hold the first 6G **normative** specs.
  Brief's timeline — **VERIFY against the current 3GPP work plan before quoting**: **Rel-21 timeline freeze ≈ March 2027**, **specifications complete ≈ March 2029**, **commercial deployment ≈ 2030**.
  Themes: **AI-native** air interface/network (AI/ML for PHY, scheduling, CSI), **integrated sensing and communication (ISAC)** — the network as radar, **non-terrestrial networks** as first-class citizens, sub-THz/new mid-band (7–24 GHz "cmWave") spectrum, energy efficiency, security/quantum-safe crypto (VERIFY each).
* **7G: there is no standard, no 3GPP work item, no ITU programme.** Anyone describing "7G" is speculating or marketing.

**How to say it confidently in an interview:** "There's no 7G — nothing is defined or scheduled. 3GPP is only now standardising 6G. If you mean research directions beyond that, I'd be guessing and I'd say so." *Do not invent a feature list.* (`generation_status(7) == NoStandard`.)

> **Misconception box — "each G is a clean break".** 5G NSA ran on an LTE core and anchor; 6G will coexist with 5G for decades; releases evolve the "same G" (5G-Advanced = Rel-18+, VERIFY).

## 4. Worked examples

**(a) μ=3 slot:** slot 0.125 ms, 14 symbols → 8.93 µs avg; 5 ms TDD period → 40 slots.
**(b) Is 30 kHz SCS safe at 500 km/h?** `f_d = 138.9 × 3.5e9 / 3e8 = 1.62 kHz`; ratio 1.62/30 = 5.4 % — OK. At 15 kHz: 10.8 % — marginal. Hence FR1 high-mobility prefers 30 kHz.
**(c) Code blocks:** a 30 000-bit TB, rate 0.5 → BG1 (A > 3824) → `ceil(30000/8424) = 4` blocks.
**(d) Link budget intuition:** moving from 3.5 GHz to 28 GHz at 1 km costs 18 dB; to recover it you need a 64-element array (≈ 18 dB gain) *at each end or on one side with beam management*.

## 5. Questions

1. **Q1 (Calculate)** — SCS, slots per frame, slot duration, useful and average symbol length for μ = 0…4. (W 3.1)
2. **Q2 (Explain)** — Why does scaling SCS shorten symbols? Give the formula and the intuition. (W 3.1)
3. **Q3 (Explain)** — What do you give up when you choose μ=3 instead of μ=0? Why is μ=3 not allowed in FR1? (W 3.1–3.2)
4. **Q4 (Calculate)** — PRB counts and occupancy for 20 MHz @15/30 kHz, 100 MHz @30 kHz, 100 MHz @120 kHz. Why do PRBs per MHz fall with larger SCS? (W 3.2)
5. **Q5 (Calculate)** — TDD: slots in a 2.5 ms and 5 ms period for μ = 1 and 3. (W 3.2)
6. **Q6 (Explain)** — CU/DU split: what runs where and why? What is F1? What stays at the antenna? (W 3.3)
7. **Q7 (Explain)** — AMF, SMF, UPF vs the LTE MME/S-GW/P-GW. Why separate AMF and SMF? (W 3.3)
8. **Q8 (Explain)** — SBA in three sentences. What problem do NRF and HTTP/2 solve? (W 3.3)
9. **Q9 (Design)** — Design a slice for MCPTT: what NFs, QoS, where is the UPF, how does admission work? (W 3.3)
10. **Q10 (Explain)** — LDPC vs polar vs turbo: who gets which channel and why. (W 3.4)
11. **Q11 (Calculate)** — Base graph choice for (A=200, R=0.9), (3000, 0.5), (3000, 0.8), (20000, 0.5), (10000, 0.2). (W 3.4)
12. **Q12 (Calculate)** — Code blocks for B = 8448, 8449, 16848, 16849 (BG1), 10000 (BG2). (W 3.4)
13. **Q13 (Calculate)** — FSPL at 3.5 and 28 GHz over 1 km; prove +6.02 dB per distance doubling and +20 dB per 10× frequency. (W 3.5)
14. **Q14 (Calculate)** — Shannon capacity for 20 MHz @ 20 dB and 100 MHz @ 10 dB; what does this say about bandwidth vs SNR? (W 3.5)
15. **Q15 (Calculate/Explain)** — GEO and LEO one-way delay, FSPL and Doppler; which NR features are needed for NTN? (W 3.6)
16. **Q16 (Interview)** — "What's 7G?" Give your 20-second answer. And: "When is 6G coming and what is it about?" (W 3.7)
17. **Q17 (Etherstack)** — Where would a public-safety gateway attach in a 5G SA network, and what can it NOT see? (W 3.3)

## 6. Self-assessment

- [ ] Reproduce the μ-table (SCS, slot, symbol) from memory by doubling/halving.
- [ ] Draw gNB (CU/DU/RU) + AMF/SMF/UPF with N2/N3/N4/F1 and say what each carries.
- [ ] Explain why FR2 needs bigger SCS and beams.
- [ ] State the BG selection rule and the segmentation formula.
- [ ] Say the 6G timeline **with a "verify" caveat** and say "no 7G".

**VERIFY list:** 6G dates (Rel-21 freeze Mar 2027, specs Mar 2029, commercial ~2030) against the 3GPP work plan; FR1/FR2 boundaries; PRB table (TS 38.101-1/-2); base-graph rule and K_cb (TS 38.212);
S-NSSAI SST values; interface names F1/N2/N3/N4; "5G-Advanced = Rel-18+"; NTN features by release; list of 6G themes and spectrum candidates.
