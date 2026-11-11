# Pack 08/02 — Interrupts, NVIC, and peripherals: GPIO, UART, SPI, I2C, timers, ADC/DAC, DMA double buffering

> **SAFETY.** Host-simulated peripherals only: arithmetic, shift registers and a software ring that behaves like a DMA engine. No hardware, no radio.
> Register conventions (STM32-style USART_BRR, "top bits implemented" NVIC priorities, ISR entry cycles) are from memory — **VERIFY** against your part's reference manual.

---

## 1. Why this matters (Etherstack context)

A radio stack on an MCU is a **real-time pipeline**: an ADC fills a buffer every 20 ms, a DSP task decodes it, a UART/SPI/I2C talks to codec, flash or host, and an ISR must run *now*. Interview questions in this area test whether you can
**do the arithmetic that gets hardware configured correctly** (baud-rate error, timer reload, buffer deadlines) and **reason about concurrency** between ISR, DMA and CPU. Both are in this pack, with tests.

## 2. Prerequisites

Pack 08/01 (MMIO, `volatile`, startup). Binary arithmetic and rounding. Sampling basics (pack 05-dsp).

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
```

## 3. Concept build-up

### 3.1 Interrupts and the NVIC (Cortex-M)

**Why interrupts?** Polling wastes CPU and misses events; an interrupt lets the peripheral *demand attention* exactly when something happens (byte received, timer expired, DMA half-full). On Cortex-M the **NVIC** (nested vectored interrupt controller) picks the handler from the **vector table** (pack 08/01 §3.1) and *nests* them by priority.

**Priority = a number, lower is more urgent.** Cortex-M implements only the **top N bits** of the 8-bit priority field (N = 3 on many parts, 4 on STM32 — VERIFY): `level = raw >> (8 − N)`:
raw `0x80` with 4 bits → level 8; `0x10` → 1; `0xFF` → 15; `0xE0` with 3 bits → 7; with 8 implemented bits `0x20` → `0x20`. (`nvic_level`.) *Why top bits?* So that code written for a 8-bit part works on a 3-bit part: the unimplemented low bits read as zero.
**Preemption:** a pending interrupt preempts the running one iff it is **strictly** more urgent (`nvic_preempts(0x10, 0x80)` true; equal priority → **no preemption**: it waits and is **tail-chained** — run right after the first returns, skipping a full pop/push of registers). Among several pending, the NVIC runs the **lowest raw priority first; ties → lowest IRQ number** (`nvic_pick`).

**ISR hygiene:** keep it short; clear the peripheral's flag (otherwise it re-fires immediately); no blocking, no `malloc`, no `printf`; communicate through `volatile`/atomics, a queue (SPSC ring: pack 08/04) or a flag + deferred work (task notification, §3.6 pack 08/03). Entry costs ~12 cycles for stacking (Cortex-M3/4, VERIFY), tail-chain ~6.

> **Misconception box — "a high-priority number is more urgent".** On Cortex-M **0 is the most urgent**; "priority 15" is the *least*. (FreeRTOS **task** priorities go the other way: higher number = more urgent — pack 08/03!)

### 3.2 UART — derive the baud-rate register

A UART sends bytes asynchronously: **start bit (0), 8 data bits (LSB first), stop bit (1)** = 10 bit times for **8N1** (`uart_byte_time_us(115200)` = **86.8 µs**; 9600 baud: **1041.7 µs**; 11-bit frames (8E1 / 8N2): **95.5 µs**). No shared clock: both sides agree on the rate and the receiver **re-synchronises on every start bit**, sampling mid-bit (16× oversampling).
The baud generator divides the peripheral clock: `BRR = round(fclk / baud)` (16× oversampling; `uart_brr`). The **actual** baud is `fclk / BRR` and the error accumulates over a frame: tolerance is roughly **±2–3 %** end-to-end (so ≈ ±1.5 % per side — VERIFY):

| fclk | baud | `fclk/baud` | BRR | actual baud | error |
|---|---|---|---|---|---|
| 72 MHz | 115200 | 625.0 | 625 | 115200 | **0 %** |
| 16 MHz | 115200 | 138.89 | 139 | 115 107.9 | **−0.08 %** |
| 8 MHz | 9600 | 833.33 | 833 | 9 603.8 | +0.04 % |
| 8 MHz | 115200 | 69.44 | 69 | 115 942 | **+0.644 %** (OK alone, risky if the peer is also off) |

*Why a crystal and not an RC oscillator?* A ±1 % internal RC oscillator plus ±0.6 % rounding exceeds the budget.

### 3.3 SPI and I2C — two ways to talk to chips

**SPI**: four wires (SCK, MOSI, MISO, CS). **Full duplex shift register ring**: each clock, the master shifts its MSB out to the slave's LSB while the slave's MSB comes into the master's LSB. After **8 clocks** the two bytes have **swapped** (`spi_clock_n`: master 0xA5 ↔ slave 0x3C).
That means **every SPI read is also a write** (send a dummy byte to clock data in). **Modes:** CPOL (idle level) and CPHA (sample edge): mode = `CPOL·2 + CPHA`; the sampling edge is **rising iff CPOL == CPHA** (mode 0 and 3 sample on rising; modes 1 and 2 on falling). Both ends must match: **the #1 SPI bug**.
Time: `bytes × 8 / SCK` — 4 bytes at 1 MHz = **32 µs** (`spi_transfer_time_us`). CS per device; fast; no addressing; no ACK.

**I2C**: two wires (SCL, SDA), open-drain with pull-ups, **addressed** devices on a shared bus, ACK after every byte. First byte after START = `addr7 << 1 | R/W` (`i2c_addr_byte`: 0x50 write → **0xA0**, read → **0xA1**; 0x68 write → **0xD0**; address masked to 7 bits).
Time: each byte is **9 bit times** (8 + ACK): address + N data bytes at 100 kHz, N=2 → `3 × 9 / 100 kHz = 270 µs`; at 400 kHz with no data: `9 / 400 k = 22.5 µs` (ignoring START/STOP; `i2c_transfer_time_us`).
Why slower than SPI? Open-drain rising edges are RC-limited (pull-up × bus capacitance), plus address/ACK overhead. Why use it? Two wires for many devices (sensors, codecs, EEPROM).

> **Misconception box — "I2C and SPI are interchangeable".** SPI: fast, more wires, no addressing, full duplex. I2C: fewer wires, slower, addressed, half-duplex, can **hang** when a slave holds SDA low (need bus recovery clocks — VERIFY).

### 3.4 Timers — derive the prescaler/reload

A timer counts clock ticks: `f_update = fclk / ((PSC + 1) × (ARR + 1))` (`timer_update_hz`): 72 MHz, PSC 71, ARR 999 → **1 kHz**; PSC 0, ARR 8999 → **8 kHz** (a classic audio-sample tick).
PSC and ARR are **16-bit** (≤ 65535). `timer_cfg_for(fclk, target)` finds the pair minimising `|actual − target|`, ties → smallest PSC (less granularity loss):
* 8 kHz at 72 MHz: PSC 0, ARR 8999 (exact).
* 1 kHz: PSC 0 would need ARR = 71 999 > 65 535, so **PSC 1, ARR 35 999** (exact).
* 1 Hz: `(PSC+1)(ARR+1) = 72 000 000` with both factors ≤ 65 536 ⇒ `PSC+1 ≥ 1099`; the smallest divisor of 72 000 000 that is ≥ 1099 is 1125 ⇒ **PSC 1124, ARR 63 999** (exact; found by the reference solution).
Uses: periodic tick (RTOS tick, sampling trigger for ADC/DAC), PWM (duty = CCR/ARR), input capture, watchdog.

### 3.5 ADC and DAC

**ADC:** quantises voltage to a code. `LSB = Vref / 2^bits`; `V = code × LSB` (`adc_to_mv`): 12-bit, Vref 3.3 V: LSB = **0.806 mV**; code 2048 → **1650 mV**; code **4095 → 3299.19 mV** — *never reaches Vref* (full scale is one LSB short).
The inverse `mv_to_adc` **rounds to nearest and clamps** to [0, 2^bits − 1] (−50 mV → 0; 5000 mV → 4095).
**Ideal SNR = 6.02·N + 1.76 dB** (`ideal_snr_db`): 8 bit **49.92**, 12 bit **74.0**, 16 bit **98.08 dB** — each extra bit buys **6 dB**. Real converters measure **SINAD**; `ENOB = (SINAD − 1.76) / 6.02`: SINAD 62 dB → **10 bits** (a "12-bit" ADC that delivers 10 effective bits: noise and distortion).
**DAC:** the inverse: code → voltage (same LSB formula), held constant between updates (**zero-order hold**: staircase), so a **reconstruction low-pass filter** is needed; its output rate comes from a timer (§3.4) so the sample clock is jitter-free.
**Sample-rate chain for a PTT voice path:** mic → anti-alias filter → ADC (8 kHz, 12-bit) triggered by a timer → DMA → CPU (codec) → ... → DAC. Jitter in the trigger = noise; hence timer-triggered, not software-triggered.

### 3.6 DMA double buffering — a timeline

**Problem.** The ADC delivers a sample every 125 µs (8 kHz). If the CPU takes an interrupt per sample, you pay ~12 cycles of stacking **plus** the handler, 8000 times a second; with a 48 kHz audio stream it becomes serious.
**DMA** moves samples from peripheral to memory **without the CPU**. With a **circular buffer split into two halves (ping-pong)** the DMA interrupts the CPU only when **a half is full** ("half-transfer" and "transfer-complete" interrupts):

```
time →      0 ms       20 ms      40 ms      60 ms      80 ms     (half = 160 samples @ 8 kHz = 20 ms)
DMA writes  [ half 0 ][ half 1 ][ half 0 ][ half 1 ][ half 0 ]...
IRQ         .         HT: half0 TC: half1  HT: half0  TC: half1
CPU reads             [process half 0 | must finish by 40 ms]
                                 [process half 1 | by 60 ms] ...
```
`dma_half_period_ms(160, 8000) = 20 ms`; 256 samples at 48 kHz = **5.33 ms**. **Deadline rule:** the CPU must finish processing half *h* before the DMA wraps around and starts overwriting it (one half-period later). If it doesn't, **overrun**: samples are silently corrupted.
The tested model `DmaRing`: `feed(sample)` writes the next sample; before the **first** sample of a half it checks whether the CPU still holds that half (`cpu_begin` without `cpu_done`) → `overruns++`; after the **last** sample of a half it fires `on_half_ready(h)`; after half 1 the index wraps to 0.
Test scenario: callbacks come in order 0, 1, 0, with exact contents `{0..3}, {4..7}, {8..11}`; a **slow CPU** (never calls `cpu_done`) causes exactly one overrun when DMA wraps onto half 0 (sample 8).
*Why two halves and not one?* With one buffer, the CPU can't read while DMA writes. With two, **one side writes while the other reads** — the fundamental pattern behind ping-pong audio, radio sample capture and display refresh.
**Cache coherence caveat** on Cortex-M7/A cores: DMA bypasses the CPU cache — invalidate/clean the buffer region around the transfers (VERIFY).

> **Misconception box — "DMA makes the CPU free".** It removes per-sample overhead, not the **deadline**. The CPU must still process every half before it is overwritten, and bus contention between DMA and CPU can slow *both*.

## 4. Worked examples

**(a) UART:** 16 MHz, 115200: `16e6/115200 = 138.89 → BRR 139 → 115 107.9 baud` → error `(115107.9−115200)/115200 = −0.080 %`.
**(b) Timer:** target 8 kHz from 72 MHz: `72e6/8000 = 9000 = (PSC+1)(ARR+1)` → PSC 0, ARR 8999.
**(c) DMA deadline:** 48 kHz stereo (2 channels) with 256-frame halves: one half = 256 / 48000 = **5.33 ms**; your DSP must finish each half in < 5.33 ms including interrupt overhead; keep the *worst case* well inside that (say ≤ 70 % of it) so a cache miss or a higher-priority ISR does not cause an overrun.
**(d) SPI:** reading 2 bytes from a sensor at 2 MHz SCK: `16 / 2e6 = 8 µs` (+ CS setup).

## 5. Questions

1. **Q1 (Calculate)** — NVIC priority levels for raw `0x80` (4 bits), `0x10`, `0xFF`, `0xE0` (3 bits). Which preempts which? (W 3.1)
2. **Q2 (Explain)** — Equal-priority interrupt arrives while another ISR runs: what happens? What is tail-chaining? (W 3.1)
3. **Q3 (Calculate)** — Pending: (IRQ 5, 0x40), (IRQ 3, 0x20), (IRQ 9, 0x20). Which runs first, then next? (W 3.1)
4. **Q4 (Design)** — Write the ISR rules checklist (6 items) and say why each exists. (W 3.1)
5. **Q5 (Calculate)** — BRR, actual baud and % error for 72 MHz/115200, 16 MHz/115200, 8 MHz/9600, 8 MHz/115200. Which are risky? (W 3.2)
6. **Q6 (Calculate)** — How long does one byte take at 115200 8N1? At 9600? At 8E1? How many bytes per second best case? (W 3.2)
7. **Q7 (Calculate)** — SPI: shift `master=0xA5, slave=0x3C` for 8 clocks; for the 1-clock partial test `master=0b10110000, slave=0b01000000`. (W 3.3)
8. **Q8 (Explain)** — CPOL/CPHA: tabulate modes 0–3 and the sampling edge; what symptom shows a mode mismatch? (W 3.3)
9. **Q9 (Calculate)** — I2C address bytes for 0x50 and 0x68 read/write; transfer time for 2 data bytes at 100 kHz and 400 kHz. (W 3.3)
10. **Q10 (Calculate)** — Timer PSC/ARR for 8 kHz, 1 kHz and 1 Hz at 72 MHz. (W 3.4)
11. **Q11 (Calculate)** — 12-bit ADC at 3.3 V: LSB; the voltage of codes 0, 2048, 4095; code for 1000 mV; ideal SNR for 8/12/16 bits; ENOB for SINAD 62 dB. (W 3.5)
12. **Q12 (Explain)** — Why does a DAC need a reconstruction filter? Why is the ADC trigger a timer? (W 3.5)
13. **Q13 (Timeline)** — Draw the DMA ping-pong timeline for 160-sample halves at 8 kHz; state the deadline for the CPU; mark where the overrun appears if processing takes 25 ms. (W 3.6)
14. **Q14 (Design)** — Compare per-sample ISR, half-buffer DMA, and quarter-buffer DMA for latency and CPU load on a 48 kHz stream. (W 3.6)
15. **Q15 (Design)** — Where can the DMA/CPU race still bite even with ping-pong (cache, bus contention, slow callback, missed IRQ)? How do you detect it (overrun counter)? (W 3.6)
16. **Q16 (Etherstack)** — A radio front-end delivers 2×16-bit IQ at 192 kHz: pick a half size and the CPU budget per half; what do you log? (W 3.6)

## 6. Self-assessment

- [ ] Derive BRR, error, byte time; know the ±2–3 % rule.
- [ ] State "lower priority number = more urgent; equal doesn't preempt".
- [ ] Explain SPI modes by CPOL/CPHA and compute I2C transfer times.
- [ ] Compute timer PSC/ARR within 16-bit limits.
- [ ] Draw the DMA half/half timeline and state the deadline.

**VERIFY list:** number of implemented priority bits on your part; ISR entry/tail-chain cycle counts; USART oversampling and BRR format (STM32 RM); baud tolerance budget; SPI mode table vs your device's datasheet; I2C open-drain bus recovery; DMA half/complete interrupt names; cache maintenance rules on M7.
