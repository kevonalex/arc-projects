# Pack 06/01 — Answers (2G GSM and 3G WCDMA)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic data only; never live traffic. **VERIFY** = check 3GPP before quoting.

---

## Q1 — Calculate: GSM timeline

**Answer.** Bit rate `13 MHz / 48 = 270 833.33 bit/s`; bit period 3.692 µs; slot = 156.25 bit periods = **0.5769 ms (15/26 ms)**; frame = 8 slots = **4.615 ms**; the 148-bit burst plus 8.25 bit guard fills the 156.25.

**Reasoning.** Everything hangs off one 13 MHz reference. Slot/frame times are multiples of the bit period, which is why timing advance can be counted in bits (Q4).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Slot = 0.5 ms" | LTE slots are 0.5 ms. | GSM slot is 0.577 ms (15/26). |
| "156 bits / slot, no fraction" | Integers feel natural. | The guard is 8.25 bits: 148 + 8.25 = 156.25. |
| "Bit rate = 200 kbit/s (the bandwidth)" | Confusing Hz and bit/s. | GMSK at ~1.35 bit/s/Hz gives 270.8 kbit/s in 200 kHz. |

*Etherstack angle:* every TDMA stack (DMR 30 ms frames, TETRA 14.167 ms) is built from a clock-derived slot grid like this.

## Q2 — Explain: FDMA + TDMA

**Answer.** FDMA splits the band into 200 kHz carriers (limits interference between cells' channels and allows frequency reuse planning); TDMA splits each carrier into 8 slots so 8 calls share one transmitter/receiver chain.

**Reasoning.** Speech needs ≈13 kbit/s, the carrier moves 270 kbit/s. Time-slicing converts spare *speed* into *users*; the phone transmits 1/8 of the time (battery, simpler duplexing: it never sends and receives in the same instant — TX/RX are offset by 3 slots, VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "TDMA makes calls wait for a turn." | Words "turn-taking". | Interleaving makes the stream continuous; waiting is milliseconds. |
| "One carrier = one user." | Analog FM channel thinking. | One carrier = 8 slots = up to 8 full-rate calls. |

## Q3 — Calculate: ARFCN

**Answer.** UL = `890 + 0.2·n`: n=62 → **902.4 MHz**, DL 947.4; n=124 → UL 914.8, DL **959.8 MHz**. ARFCN 0 and 125 are outside P-GSM 1..124 ⇒ the function returns **−1.0** (explicit "invalid", never a silent wrong frequency).

**Reasoning.** Duplex spacing 45 MHz keeps TX and RX apart for a duplex filter. E-GSM extends the range (VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "UL = 890 + n × 0.2 for n=0" | Looks like the same formula. | ARFCN 0 is the E-GSM/guard channel, outside this plan. |
| "DL below UL" | Many FDD systems differ. | GSM-900 puts UL in the lower sub-band. |

## Q4 — Calculate: timing advance

**Answer.** One TA step = one bit period of **round trip** = `c · 3.692 µs / 2 = 553.46 m`. 6-bit field → 63 steps → `63 × 553.46 m = 34.87 km`.

**Reasoning.** Without TA a phone at distance d arrives `2d/c` late; if that exceeds the guard (~30 µs ≈ 8.25 bits, VERIFY) it corrupts the next slot. The BTS measures the offset and orders the phone to transmit early. It is a timing limit: more power does not help, unless the burst format changes.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "1 bit = 1107 m" | `c × 3.692 µs`. | Forgot the **round trip**: divide by 2. |
| "Limit comes from link budget" | Cell range usually does. | Here TA = 63 caps range even with power to spare. |

*Etherstack angle:* DMR/TETRA also bound range by slot guard — same reasoning, different numbers.

## Q5 — Calculate: coded rate

**Answer.** `456 bit / 20 ms = 22 800 bit/s`. Vocoder: 260 bit / 20 ms = 13 kbit/s. The extra 196 bits are **FEC and parity** (and interleaving makes bursts robust to fades).

**Reasoning.** Radio channels are error-prone; the receiver needs redundancy. Unequal protection: the most important speech bits (class 1a) get CRC + convolutional coding; the least important (class 2) none (VERIFY split).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "22.8 kbit/s is the speech rate" | It is the TCH rate. | Speech is 13 kbit/s; 22.8 includes coding. |

## Q6 — Calculate: reuse

**Answer.** N = i²+ij+j²: (1,1) → 3; (2,1) → 7; (2,2) → 12. `D/R = √(3N)`: N=3 → **3.0**; N=7 → **4.583**.

**Reasoning.** D is the distance between co-channel cells. Larger N pushes interferers farther away (better C/I) but divides the channels among more cells (lower capacity per cell). 7 is the classic compromise for analog; GSM used tighter reuse thanks to FEC/frequency hopping (VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "N can be any integer" | Seems flexible. | Only values `i²+ij+j²` tile a hexagonal grid: 1,3,4,7,9,12… |
| "Higher N = more capacity" | More cells in cluster. | Fewer carriers per cell ⇒ less capacity. |

## Q7 — Explain: GMSK

**Answer.** Constant envelope ⇒ the PA can run saturated (efficient, cheap, battery-friendly) and non-linearity doesn't distort the signal. MSK deviation `Rb/4 = 67.7 kHz`. The Gaussian filter narrows the spectrum so adjacent channels stay clean, at the cost of **deliberate ISI** that the receiver's equaliser removes.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "GMSK is amplitude modulation" | "Modulation". | Information is in the phase/frequency only. |
| "Gaussian filter removes ISI" | Filters are for cleaning. | It *adds* ISI (to save bandwidth). |

## Q8 — Explain: network boxes

**Answer.** MS ↔ **BTS** (transmits/receives on the air) → **BSC** (allocates channels, handles handover between its BTSs, power control) → **MSC** (call setup/switching, links to PSTN) ↔ **HLR/VLR** (who the subscriber is, where they currently are). 

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "BSC switches calls" | 'Controller'. | Switching is the MSC. |
| "HLR is per-site" | 'Location'. | HLR = home database; VLR = visited area copy. |

*Etherstack angle:* gateway vendors terminate interfaces between these boxes (A-bis, A — VERIFY).

## Q9 — Calculate: SF and gain

**Answer.** 15 ksym/s → SF **256**, 24.08 dB; 30 ksym/s → SF **128**, 21.07 dB; 960 ksym/s → SF **4**, 6.02 dB.

**Reasoning.** SF = chips/symbol = `3.84e6/Rs`; gain = `10 log10 SF` because despreading adds SF chips coherently (SF× signal power) while uncorrelated interference adds only incoherently.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Gain = 20·log10(SF)" | Amplitude vs power. | The relevant quantity is power: 10·log10. |
| "SF = symbols/chips" | Inverted. | Chips are faster: SF = chips/symbol ≥ 1. |

## Q10 — Derive: orthogonality separates users

**Answer.** `r = a·c1 + b·c2`; `r·c1 = a(c1·c1) + b(c2·c1) = 4a + 0` since `c1·c2 = 1+1−1−1 = 0`; dividing by 4 yields exactly `a`. User 2 cancels exactly, regardless of its data.

**Reasoning.** Correlation is linear; orthogonality makes the cross term zero for **every** value of b. All users occupy the same time and frequency; the code is the *only* separator.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Users are separated in frequency" | FDMA habit. | Same 5 MHz at the same time. |
| "Orthogonal ⇒ no interference ever" | Sounds absolute. | Only aligned and only vs codes outside the sub-tree (Q13). |

## Q11 — Calculate: OVSF

**Answer.** `C(4,0)=++++`, `C(4,1)=++−−`, `C(4,2)=+−+−`, `C(4,3)=+−−+`. Children of `C(4,1)`: `C(8,2)=++−−++−−`, `C(8,3)=++−−−−++`. Conflicts with `C(4,1)`: `C(8,3)` yes (child), `C(2,1)`? `C(2,1)=+−`; `C(4,1)` is *not* its child (children of C(2,1) are C(4,2),C(4,3)) ⇒ **no conflict**; `C(8,4)` no (child of C(4,2)). (`C(2,0)` would conflict: it is the parent.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "C(2,1) conflicts: smaller index = parent" | Looks related. | Parent of `C(4,1)` is `C(2,0)` (index k/2). |
| "All same-index codes conflict" | Index equality. | Conflict is ancestry in the tree, not equal index. |

## Q12 — Explain: code-tree blocking

**Answer.** Allocating a code reserves its whole sub-tree. Example: users on `C(8,0)`, `C(8,2)`, `C(8,4)`, `C(8,6)` (one under each SF-4 node) leave 4/8 SF-8 leaves free (1,3,5,7) but **every SF-4 node has a used child**, so an SF-4 request is blocked. Fix: allocate **contiguously** (pack allocations into one sub-tree), re-assign codes (costly), or use multiple codes.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Free leaves ⇒ capacity" | Counting free codes. | Position in the tree matters. |

## Q13 — Calculate/Explain: misalignment

**Answer.** Shift 0 → **0**; shift 1 → **−4**; shift 2 → **−8** (as strong as the wanted autocorrelation 8). Uplink users are not chip-aligned (different distances, no common clock at chip level), so OVSF orthogonality fails: UMTS scrambles the uplink with long codes whose cross-correlation is low *on average* and relies on power control.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Orthogonality is shift-invariant" | It holds at lag 0. | Walsh-type codes have poor off-peak correlation. |

## Q14 — Design/interview: TDMA vs CDMA (model answer)

"TDMA separates users in **time**: each gets a slot, so they never overlap; cost: fixed capacity per carrier, guard times and timing advance. CDMA separates by **code**: everyone transmits together, the receiver correlates with its code; orthogonal codes cancel *exactly* if aligned (downlink) and scrambling gives statistical separation otherwise (uplink); cost: interference-limited ('soft') capacity and tight power control. LTE/5G moved to OFDMA, which separates users in **frequency–time resource blocks** with no inter-user interference at all when synchronised and handles multipath much more simply (pack 02)."

## Q15 — Etherstack: slots in LMR

**Answer.** Carry over: slot grid derived from a symbol clock, bursts with training/sync, guard time, a base-station-controlled timing offset for outbound/inbound alignment, interleaved speech frames. Differ: DMR/TETRA slots are **long** (30 ms / 14.167 ms frames), ranges are larger (no 35 km cap issue in DMR direct mode), and many LMR systems have no timing advance on simplex/direct mode (VERIFY per standard).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "DMR is just GSM" | Both TDMA. | Different modulation (4FSK vs GMSK), vocoder, trunking logic. |
