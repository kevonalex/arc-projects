# Pack 06/01 — 2G GSM and 3G UMTS/WCDMA: sharing one radio band between many users

> **SAFETY / LEGAL.** Everything here uses **synthetic** numbers and chips you generate in unit tests.
> **Never** receive, record or decode live cellular, public-safety or commercial radio traffic with an SDR or anything else
> from this pack. Intercepting calls is unlawful in most places (e.g. UK Wireless Telegraphy Act 2006 / Investigatory Powers
> Act 2016 — VERIFY for your jurisdiction). GSM/UMTS here is arithmetic and a code-tree exercise, not a sniffer.

Standards (numbers from memory — **VERIFY**; I never invent clause numbers): 3GPP **TS 45.001/45.002** (GSM physical layer,
multiframe structure), **TS 45.005** (RF: ARFCN plan, GMSK), **TS 45.010** (timing advance/synchronisation), **TS 25.213**
(UMTS spreading and modulation, OVSF), **TS 25.211** (physical channels), GSM architecture **TS 23.002**. All numbers in the
worked tables below are reproduced by the tests in `code/tests/test_cell2g3g.cpp`.

---

## 1. Why this matters (Etherstack context)

Etherstack builds **radio protocol stacks and gateways**. Before LTE and 5G (packs 02–03) there were two generations whose
*ideas* every later system either reused or reacted against: **2G** shares the band by **time** (TDMA, also what DMR and
TETRA do — pack 07), **3G** shares it by **code** (CDMA — the ancestor of spread-spectrum thinking you will meet in
synchronisation and in the 4FSK correlators of the capstone). An interviewer who asks "why does LTE not use CDMA?" wants to hear
you can derive why users separate under each scheme, not recite names. Also: LMR/MCPTT gateways (pack 04) bridge *to* these
cellular cores, so you should know where the BTS, BSC and MSC sit.

## 2. Prerequisites

- Inner product of two ±1 vectors; powers of two; `dB = 10·log10(power ratio)`.
- C++17, GoogleTest, WSL/Linux with `-Wall -Wextra -fsanitize=address,undefined`.

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
```

(One test file is built twice: `student_tests` against `include/`, `solution_tests` against `solutions/`. Offline? see the top of `CMakeLists.txt`.)

## 3. Concept build-up

### 3.1 Concrete first: the GSM timeline

Start with the numbers, then explain them. A GSM carrier is **200 kHz wide** and carries **270.833 kbit/s** of channel bits.
Where does 270.833 come from? The system clock is **13 MHz**; the bit rate is `13 MHz / 48 = 270 833.33 bit/s`
(`gsm_bit_rate_bps`). One bit lasts 3.692 µs.

The carrier is chopped into **8 timeslots** ("TDMA": time-division multiple access). One slot is **156.25 bit periods**
= `156.25 / 270 833 = 0.5769 ms = 15/26 ms` (`gsm_slot_ms`). Eight slots make a **TDMA frame of 4.615 ms** (`gsm_frame_ms`).

| Quantity | Derivation | Value |
|---|---|---|
| Bit rate | 13 MHz / 48 | 270.833 kbit/s |
| Slot length | 156.25 bits × 3.692 µs | 0.5769 ms |
| Frame | 8 × slot | 4.615 ms |
| Bits actually in a normal burst | modulated data, tails, training sequence | 148 bits (+ 8.25 bit guard = 156.25) |

*Why 8 users per carrier?* A voice call needs a trickle (≈13 kbit/s coded, §3.3) but a radio channel is fast (270 kbit/s). So a
phone transmits **one 0.577 ms burst every 4.615 ms** and sleeps the other seven slots — **statistical idleness turned into
sharing**. A phone's whole life is "wake up for my slot, burst, sleep" — which is also why GSM handsets last days on a battery.

> **Misconception box — "TDMA means users take turns waiting".** Nobody waits audibly. The burst is *compressed in time*: 20 ms of
> speech is coded into 456 bits and sent as eight half-bursts spread over **several** frames (interleaving) at 270 kbit/s. The
> user perceives a continuous stream; the radio sees a train of short bursts.

### 3.2 Frequency plan, timing advance (why a cell can't be bigger than ~35 km)

**FDMA on top of TDMA.** The band is cut into **200 kHz channels (ARFCN)**. In P-GSM 900: uplink `890 + 0.2·n MHz`, downlink = uplink + **45 MHz**
(duplex spacing), `n = 1…124` (`gsm_ul_mhz`, `gsm_dl_mhz`). So ARFCN 1 → UL 890.2 / DL 935.2 MHz; ARFCN 62 → UL 902.4 MHz.
Different phones in different slots *and* carriers = the TDMA/FDMA hybrid. (E-GSM / DCS 1800 plans differ — VERIFY.)

**Timing advance.** Slots must not overlap *at the base station*. A phone 30 km away transmits `30/300 000 = 100 µs` late,
and the BTS's reply is another 100 µs coming back: a **200 µs** round trip = 54 bit periods, enough to smash into the neighbouring slot. So the network
tells every phone "transmit **N bit periods early**". One step = one bit period of **round trip**, i.e. half a bit period of one-way
distance: `c · 3.692 µs / 2 = 553.46 m` (`gsm_ta_step_m`). TA is a **6-bit** number (0…63) ⇒ max `63 × 553.46 = 34.87 km`
(the famous ~35 km GSM range limit; extended-range cells use a trick — VERIFY).

> **Misconception box — "35 km is a power limit".** It is a **timing** limit: the TA field and the slot guard are too short. You can
> have power for 100 km and still get no service.

### 3.3 Speech, coding, reuse, GMSK

**Coding.** The full-rate vocoder produces **260 bits per 20 ms (13 kbit/s)**; channel coding and parity expand this to **456 bits / 20 ms = 22.8 kbit/s**
(`gsm_tch_coded_rate_bps`; the split of class I/II bits is VERIFY). The 456 bits are what one user's 20 ms of speech occupies on the air (spread over several bursts, §4(a)).

**Frequency reuse.** A frequency can't be used in adjacent cells (co-channel interference), but can be reused far away. Hexagonal cells and a
**cluster of N cells** with `N = i² + ij + j²` (`cluster_size`): (1,1) → 3, (2,0) → 4, (2,1) → **7**, (2,2) → 12. Reuse distance `D/R = √(3N)`: N = 7
→ **4.58** cell radii (`reuse_distance_ratio`). Bigger N ⇒ less interference, fewer channels per cell. This is the *capacity vs quality* dial of every cellular system.

**GMSK.** The carrier stays **constant-envelope** (frequency-modulated), so cheap non-linear power amplifiers work. MSK
uses frequency deviation `±Rb/4` (`msk_deviation_hz`: **67.7 kHz** at 270.833 kbit/s); the **Gaussian** pre-filter (GSM uses BT = 0.3, VERIFY) smooths the
frequency pulses to keep the spectrum inside 200 kHz at the cost of controlled inter-symbol interference that the equaliser (Viterbi) undoes.

**Network.** `Phone (MS) — BTS (radio, antenna) — BSC (controls many BTSs: handover, channel allocation) — MSC (switching, call setup) — HLR/VLR (subscriber databases) — PSTN`.
BTS = *the radio*, BSC = *the radio manager*, MSC = *the switch*. (Packet data on 2G: GPRS adds SGSN/GGSN — VERIFY details; out of scope here.)

> **Misconception box — "GSM is analogue/CDMA/old so irrelevant".** GSM is fully digital (TDMA/GMSK). And the TDMA/burst/timing-advance ideas
> reappear in DMR, TETRA (pack 07) and LTE's uplink timing alignment (pack 02).

### 3.4 3G: sharing by code — the spreading factor

WCDMA keeps everybody in the **same 5 MHz carrier at the same time**. Each user's data symbols are multiplied by a faster ±1 **code** sequence
("spreading"). Chip rate is fixed at **3.84 Mchip/s**. If the data symbol rate is `Rs`, each symbol is replaced by `SF = chip rate / Rs` chips
(`spreading_factor`): 15 ksym/s → **SF 256**; 30 ksym/s → **SF 128**; 960 ksym/s → **SF 4**.

**Processing gain** `10·log10(SF)` (`processing_gain_db`): SF 256 → **24.08 dB**; SF 4 → 6.02 dB. This is the amount by which despreading lifts the wanted
signal above other users' interference — *the price of sharing time and frequency completely*. High data rate (low SF) = low gain = needs higher SNR.

### 3.5 Why orthogonal codes separate users (derive it)

Take two users with 4-chip codes `c1 = (+1,+1,+1,+1)` and `c2 = (+1,+1,−1,−1)`. User 1 sends symbol `a`, user 2 sends `b`. Air: `r = a·c1 + b·c2`. The receiver of user 1
**despreads**: correlate with `c1` and divide by the length:
`(r·c1)/4 = (a·(c1·c1) + b·(c2·c1))/4 = (4a + 0)/4 = a`. `c1·c2 = 1+1−1−1 = 0`: **the other user vanishes exactly, not approximately.** That is what *orthogonal* means.
Test `Spreading.TwoUsersShareOneChipStreamAndSeparate` does this with two real users.

**OVSF (orthogonal variable spreading factor) codes** let users have *different* SFs and still stay orthogonal. They live on a **binary tree**: code `C(n,k)` has children
`C(2n,2k) = [C, C]` and `C(2n,2k+1) = [C, −C]` (`ovsf_code`). Examples: `C(4,1) = (+,+,−,−)` and its children `C(8,2) = (+,+,−,−,+,+,−,−)` and `C(8,3) = (+,+,−,−,−,−,+,+)`.

Rules (derive from the prefix property): codes of the **same SF are mutually orthogonal**; a code is orthogonal to every code **except its ancestors and descendants**
(`ovsf_conflict`). Using `C(4,1)` blocks its entire sub-tree: eight SF-256 descendants' worth of capacity. So a high-rate user (low SF) **consumes a big slice of the tree** —
**code-tree blocking**: the tree can be fragmented, so a new SF-8 user can be refused even though 'enough' leaves are free.

> **Misconception box — "orthogonal codes remove all interference".** Only if **perfectly chip-aligned** and only against codes *outside* your sub-tree. §3.6.

### 3.6 Where orthogonality breaks (and why the uplink scrambles)

`cross_correlation(a, b, shift)` computes the cyclic correlation after a delay of `shift` chips. For `C(8,1)` and `C(8,3)`: shift 0 → **0**; shift 1 → **−4**; shift 2 → **−8** — a
2-chip timing error makes the interferer as strong as the wanted signal. Downlink: one BTS sends everything chip-aligned, so OVSF works. **Uplink:** phones are not synchronised to chip level, so
orthogonality is lost. UMTS therefore **scrambles** each link with a long pseudo-random sequence; users become separable *statistically* (low cross-correlation), not exactly (TS 25.213 — VERIFY details).
Also **multipath** produces delayed copies ⇒ the same loss ⇒ **RAKE receivers** combine echoes (idea: each echo is a separate despreading "finger").

> **Misconception box — "CDMA has no capacity limit, just more noise".** It is *interference-limited*: every user is noise to the others (**soft capacity**), and **power control**
> (the near–far problem: a close phone swamps a far one unless it lowers its power ~1500 times per second — VERIFY) is mandatory.

### 3.7 Summary table: 2G versus 3G in one line each

| | Separates users by | Cost | Where the idea reappears |
|---|---|---|---|
| GSM | time slot (+ carrier) | fixed capacity per slot; timing advance | DMR/TETRA slots; LTE UL timing |
| WCDMA | spreading code | interference-limited; power control; code-tree blocking | spread-spectrum sync / correlators in the capstone modem |

## 4. Worked examples

**(a) Slot budget.** A normal burst carries 148 bits in 0.5769 ms. A user gets one burst per 4.615 ms frame, so the raw rate is `148 bits / 4.615 ms = 32.1 kbit/s` per slot (the burst's 114 data bits ≈ 24.7 kbit/s — VERIFY burst split). The coded TCH payload is **22.8 kbit/s**; the remainder is training sequence, tail bits and signalling overhead (FACCH/SACCH — VERIFY). Eight such slots × 32.1 kbit/s ≈ 257 kbit/s ≈ the 270.8 kbit/s carrier less guard time.

**(b) Reuse.** 7-cell cluster ⇒ each cell may use 1/7 of the carriers; with a 25 MHz band (125 carriers) that is ~17 carriers ×8 slots ≈ 136 channels per cell, minus control channels.

**(c) Spreading by hand.** User A: `sym = (+1, −1)`, `code = (+,+,−,−)`. Chips: `(+,+,−,−, −,−,+,+)`. Despread: block 1 · code = 4 → +1; block 2 · code = −4 → −1. ✓.

## 5. Questions

Each question has a type tag and a one-line context. Answers in `answers.md`.

1. **Q1 (Calculate)** — *GSM timeline.* Derive bit rate, slot time, frame time and bits per slot from the 13 MHz clock. (`gsm_*`, W 3.1)
2. **Q2 (Explain)** — Why does GSM use **both** FDMA and TDMA? What is gained from 8 slots per carrier? (W 3.1)
3. **Q3 (Calculate)** — ARFCN 62 and 124: uplink and downlink frequencies; what about ARFCN 0 and 125 in `gsm_ul_mhz`? (W 3.2)
4. **Q4 (Calculate)** — Timing advance step in metres and the maximum cell range; show why it is a timing limit. (W 3.2)
5. **Q5 (Calculate)** — Coded TCH rate 456 bits / 20 ms; why is it larger than the 13 kbit/s vocoder rate? (W 3.3)
6. **Q6 (Calculate)** — Cluster sizes for (i,j) = (1,1), (2,1), (2,2) and reuse distance ratio for N = 3, 7. (W 3.3)
7. **Q7 (Explain)** — Why GMSK? What does "constant envelope" buy, and what does the Gaussian filter cost? (W 3.3)
8. **Q8 (Explain)** — BTS, BSC, MSC, HLR/VLR: draw the chain and give each box one verb. (W 3.3)
9. **Q9 (Calculate)** — Spreading factor and processing gain for 15, 30 and 960 ksym/s on a 3.84 Mchip/s carrier. (W 3.4)
10. **Q10 (Derive)** — Prove that orthogonal codes separate two users; show the cancellation with `(+,+,+,+)` and `(+,+,−,−)`. (W 3.5)
11. **Q11 (Calculate)** — Write out `C(4,0..3)` and the two children of `C(4,1)`. Which of `C(8,4)`, `C(8,3)`, `C(2,1)` conflict with `C(4,1)`? (W 3.5)
12. **Q12 (Explain)** — What is code-tree blocking? Give a concrete allocation that blocks an SF-4 request though 4/8 SF-8 leaves are free. (W 3.5)
13. **Q13 (Calculate/Explain)** — Cross-correlation of `C(8,1)` and `C(8,3)` at shifts 0, 1, 2. Why does it matter for the uplink? (W 3.6)
14. **Q14 (Design/interview)** — "TDMA vs CDMA in 60 seconds": write the answer you would give in an interview. (W 3.7)
15. **Q15 (Etherstack)** — DMR has 2 slots per 12.5 kHz, TETRA 4 per 25 kHz: which GSM ideas carry over, which don't (burst length, timing advance)? (W 3.1–3.2)

## 6. Self-assessment

Tick when you can do it **without notes**:

- [ ] Derive 270.833 kbit/s, 0.577 ms, 4.615 ms from 13 MHz.
- [ ] Explain timing advance and why 35 km is a timing limit.
- [ ] Compute `N = i²+ij+j²` and `D/R = √(3N)`.
- [ ] Show on a whiteboard why orthogonal codes cancel a second user.
- [ ] Draw the OVSF tree to SF 4 and mark a blocked sub-tree.
- [ ] Say why uplink CDMA needs scrambling and power control.

**VERIFY list (look these up before quoting in an interview):** GMSK BT = 0.3; 260→456 bit class split; P-GSM vs E-GSM ARFCN ranges; extended-range TA trick; UMTS power-control
rate (1500 Hz); exact UMTS uplink scrambling code structure; HLR/VLR/GPRS node names; which TS clause defines which table.
