# Pack 08/02 — Answers (interrupts, peripherals, DMA)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Host-simulated; no hardware. **VERIFY** = check your part's reference manual.

---

## Q1 — Calculate: NVIC levels

**Answer.** `0x80` (4 bits) → 8; `0x10` → 1; `0xFF` → 15; `0xE0` (3 bits) → 7. Level 1 preempts level 8 (`nvic_preempts(0x10, 0x80)` true); 8 never preempts 1; equal never preempts.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "0x80 is the highest priority" | Big number. | Lower number = more urgent. |
| "level = raw & 0xF" | Low bits. | Only the **top** bits are implemented: `raw >> (8−N)`. |

## Q2 — Explain: equal priority

**Answer.** It stays pending; when the running ISR returns the NVIC **tail-chains** into it without unstacking/restacking registers (≈ 6 cycles vs ≈ 12 + 12). It never preempts equal priority — nesting needs strictly higher urgency. This also prevents unbounded stack growth.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Equal priority interrupts nest" | Both are interrupts. | Would allow infinite nesting; equal waits. |

## Q3 — Calculate: pick order

**Answer.** IRQ **3** first (raw 0x20, lowest IRQ number among ties), then IRQ **9** (0x20), then IRQ **5** (0x40). Empty list → −1.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "IRQ 5 first: first pending" | Arrival order. | NVIC arbitrates by priority, then number. |

## Q4 — Design: ISR checklist

1. **Short** — latency of everything lower-priority grows with it. 2. **Clear the flag** — else it re-enters forever. 3. **No blocking calls** — no mutex/`delay`; deadlock/priority inversion risk. 4. **No `malloc`/`printf`** — non-deterministic, non-reentrant. 5. **Shared data via `volatile`/atomics/SPSC ring** (08/01 §3.5, 08/04) — not plain variables. 6. **Defer work** to a task (notification/queue "from ISR" APIs) — keep the ISR a doorbell.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Do the processing in the ISR so latency is minimal" | Fast response. | Starves other IRQs, loses bytes elsewhere. |

## Q5 — Calculate: baud rates

| fclk/baud | `fclk/baud` | BRR | actual | error | Verdict |
|---|---|---|---|---|---|
| 72 M / 115200 | 625.0 | 625 | 115 200 | 0 % | perfect |
| 16 M / 115200 | 138.89 | 139 | 115 107.9 | −0.08 % | fine |
| 8 M / 9600 | 833.33 | 833 | 9 603.8 | +0.04 % | fine |
| 8 M / 115200 | 69.44 | 69 | 115 942 | +0.644 % | risky: uses a big share of the ±2–3 % budget; combined with peer error/clock drift it can fail |

**Reasoning.** Error = rounding of the divisor; at small divisors one count is a big fraction (1/69 = 1.4 %).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "BRR = fclk / (16·baud)" | Mix with oversampling formula. | In this model BRR already includes the ×16 (`fclk/baud`); the 16× is inside the hardware divider (VERIFY your part's format). |
| "Truncate the divisor" | Integer division. | Rounding to nearest halves the worst-case error. |

## Q6 — Calculate: byte time

**Answer.** 8N1 at 115200: `10/115200 = 86.8 µs` → 11 520 B/s max; 9600: **1041.7 µs** → 960 B/s; 8E1/8N2 (11 bits) at 115200: **95.5 µs** → ≈ 10 473 B/s. Real throughput is lower (gaps, protocol overhead).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "115200/8 = 14 400 B/s" | 8 data bits. | Start and stop bits cost 2 of 10. |

## Q7 — Calculate: SPI shift

**Answer.** 8 clocks: master `0x3C`, slave `0xA5` (swapped). 1 clock from `master=10110000, slave=01000000`: master's MSB `1` enters slave LSB, slave's MSB `0` enters master LSB → master `01100000`, slave `10000001`. 0 clocks → no change.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Master sends, slave replies afterwards" | Request/response thinking. | Full duplex: both shift simultaneously. |

## Q8 — Explain: SPI modes

| Mode | CPOL | CPHA | Idle SCK | Sample edge |
|---|---|---|---|---|
| 0 | 0 | 0 | low | rising |
| 1 | 0 | 1 | low | falling |
| 2 | 1 | 0 | high | falling |
| 3 | 1 | 1 | high | rising |

Sample on rising iff CPOL == CPHA. Mismatch symptom: data shifted by a bit/half-bit (e.g. 0x55 reads as 0xAA or 0x2A), "works on the scope, garbage on the MCU", first/last bit wrong.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "CPHA = 1 means sample on rising" | Intuition. | CPHA selects the *second* edge: rising only if CPOL = 1. |

## Q9 — Calculate: I2C

**Answer.** 0x50 → write `0xA0`, read `0xA1`; 0x68 → `0xD0` / `0xD1`. 2 data bytes at 100 kHz: `(1+2)×9/100 k = 270 µs`; at 400 kHz: `27/400 k = 67.5 µs`. (START/STOP ignored; clock stretching adds more.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "0x50 write = 0x50" | The datasheet address. | Address is shifted left; bit 0 = R/W. (Some datasheets quote the 8-bit form — check.) |
| "8 bits per byte" | Data is 8 bits. | The ACK bit makes it 9. |

## Q10 — Calculate: timer reload

**Answer.** 8 kHz: PSC 0, ARR 8999. 1 kHz: PSC 1, ARR 35 999 (PSC 0 → ARR 71 999 doesn't fit 16 bits). 1 Hz: `(PSC+1)(ARR+1) = 72 000 000` → PSC 1124, ARR 63 999 (the smallest PSC giving an exact solution).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "PSC = 72e6/target" | Same divide. | Must split between two 16-bit registers and add 1 offset. |
| "ARR = 8000 for 8 kHz" | Off-by-one habit. | The counter counts 0…ARR: `ARR+1` ticks. |

## Q11 — Calculate: ADC

**Answer.** LSB = 3300/4096 = **0.8057 mV**. Code 0 → 0 mV; 2048 → 1650 mV; 4095 → **3299.19 mV**. 1000 mV → `round(1000/0.8057) = 1241`. Ideal SNR: 8-bit 49.92 dB, 12-bit 74.0 dB, 16-bit 98.08 dB. ENOB(62 dB) = (62 − 1.76)/6.02 = **10.0**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Code 4095 = 3300 mV" | Full scale. | Full scale code is Vref − 1 LSB. |
| "SNR = 6 dB × N" | Rule of thumb. | Plus 1.76 dB constant for a full-scale sine. |

## Q12 — Explain: reconstruction filter, timer trigger

**Answer.** A DAC output is a staircase (zero-order hold): its spectrum contains images around multiples of fs and a sinc droop; a low-pass reconstruction filter removes images (otherwise they radiate/alias at the next stage). The ADC/DAC sample clock must have **low jitter**: software triggers inherit interrupt-latency jitter, which becomes noise (error ∝ slew rate × timing error). A hardware timer trigger is exact.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Reconstruction filter smooths quantisation noise" | It smooths. | It removes spectral images; quantisation noise is mostly in-band. |

## Q13 — Timeline

**Answer.** Halves of 20 ms: half 0 fills 0–20 ms (IRQ at 20), half 1 20–40 (IRQ at 40), half 0 again 40–60. CPU gets half 0 at 20 ms and must be done by **40 ms** (deadline = one half-period). With 25 ms processing, it would finish at 45 ms, but DMA begins rewriting half 0 at 40 ms ⇒ **overrun at 40 ms**; the first 5 ms of the following capture overwrite data still in use.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Deadline = 20 ms (the processing slot)" | Equal to the period. | The CPU also has the *other* half's time: deadline = a full half-period after the IRQ. |

## Q14 — Design: strategies

| Strategy | Latency added | IRQs/s at 48 kHz | CPU overhead | Note |
|---|---|---|---|---|
| per-sample ISR | ≈ 21 µs | 48 000 | high (stacking + handler each time) | simplest, worst |
| half-buffer (256) | 5.33 ms | ≈ 188 (two per buffer) | low | classic ping-pong |
| quarter buffer (128) | 2.67 ms | ≈ 375 | medium | lower latency, more IRQs |

Choice depends on the latency budget (voice PTT: a few ms OK) and CPU margin.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Bigger buffers are always better" | Fewer interrupts. | Latency and memory grow; deadline stretches but so does jitter. |

## Q15 — Design: remaining races

**Answer.** (1) Slow callback (CPU still holds the half) → overrun counter. (2) **Cache** on M7/A: stale data unless invalidated before reading, clean before DMA reads. (3) **Bus contention** from other DMA/CPU slows both: measure worst-case. (4) **Missed/merged IRQ** (higher-priority ISR keeps the core busy; a half flag overwritten): track a sequence counter, compare with expected. (5) Process *in place* while DMA writes the other half is fine, but writing into the half DMA reads (TX DMA) needs the same deadlines. Detection: overrun counter, per-half timestamp deltas, max processing time.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "ping-pong removes all races" | Two buffers. | Only if CPU meets the deadline and cache/ordering are handled. |

## Q16 — Etherstack: IQ at 192 kHz

**Answer.** 4 bytes per IQ pair → 768 kB/s. Half = 1024 pairs = 5.33 ms (4 KiB); two halves 8 KiB. CPU budget < 5.33 ms/half, aim ≤ 3.7 ms (70 %) worst case incl. ISR and DMA contention. Log: overrun count, max/mean processing time per half, sequence-number gaps, and the buffer's slack (time between `cpu_done` and next DMA wrap). Synthetic IQ only in tests.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use a 64-sample half to minimise latency" | Latency matters. | 0.33 ms deadline → ISR overhead dominates; overruns likely. |
