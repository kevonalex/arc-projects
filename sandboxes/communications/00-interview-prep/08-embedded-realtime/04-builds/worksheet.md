# Pack 08/04 — Embedded builds: an emulated radio peripheral, an SPSC ring, an interrupt-driven UART, and the priority-inversion pipeline — plus Linux, C++ on MCUs, JTAG/SWD and FPGA awareness

> **SAFETY.** All hardware is **emulated** in software (register map + models) and fed by a **synthetic DSP simulator** (a generated tone). No hardware, no antenna, no live radio signals; nothing here receives or decodes real traffic.
> The register layout is **invented** for this exercise (not any real chip's). Board/RTOS/Linux facts are from memory — **VERIFY** for your platform.

---

## 1. Why this matters (Etherstack context)

Etherstack-style products meet hardware at the **register level**: a radio front-end or modem is a memory-mapped block, firmware is a **driver** over that register map, and a **DSP simulator** stands in for the antenna so the stack can be tested without RF.
This pack makes you build that loop end to end: *peripheral model → driver → interrupt → lock-free hand-off → RTOS pipeline → fix a deadline miss*. Then it widens to the questions interviewers add: embedded **Linux**, **C++ without exceptions/RTTI**, **JTAG/SWD**, and **when to offload to an FPGA**.

## 2. Prerequisites

Packs 08/01–03 (MMIO, volatile, ISR, DMA, RTOS tasks/mutexes). C++17 templates; `std::atomic` memory orders at "release publishes, acquire observes".

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
./build/pipeline_trace    # demo (reference solution): the audio-pipeline timeline without / with priority inheritance
```

## 3. Concept build-up

### 3.1 Build 1 — A peripheral = register map + driver (+ a simulator behind it)

**Concrete first.** Here is the emulated radio's register map (`common/radio_model.hpp`; byte offsets):

| Reg | Offset | Bits / meaning |
|---|---|---|
| `CTRL` | 0x00 | bit0 **EN**, bit1 TXSTART |
| `STATUS` | 0x04 | bit0 TXBUSY, bit1 **RXRDY** (FIFO not empty), bit2 **RXOVF** (sticky) |
| `DATA` | 0x08 | **read: pops one RX sample** (side effect!); write: TX sample |
| `FREQ` | 0x0C | carrier in units of **25 Hz** (`kFreqStepHz`) |
| `IRQ_EN` | 0x10 | bit0 RX, bit1 OVF |
| `IRQ_STAT` | 0x14 | **write-1-to-clear** |

RX path: the DSP simulator calls `push_rx_sample(s)`; if EN is off the sample is **dropped**; the FIFO holds **8** samples (`kRxFifoDepth`); when full, a new sample **sets OVF** (sticky) and is lost — the **newest** are lost, the oldest survive.
The model only knows `read32/write32`; the driver (`RadioDriver<Bus>`) is a **template on the bus** so the same code can run against the model, a `volatile uint32_t*` mapping, or a mock.

**Driver rules** (each is a test):
* `set_frequency_hz(hz)`: valid range **30 MHz … 6 GHz**; store `round(hz / 25)` in FREQ: 446 006 250 Hz → `17 840 250`; 446 006 260 Hz rounds to the **same** 446 006 250. Out of range → return `false` and **write nothing** (`FrequencyRoundTripAndLimits`).
* `read_sample`: **check RXRDY first; read DATA exactly once** (the read pops). Reading DATA when empty is a bug in real hardware (may return junk or underflow). `drain(sink)` loops `read_sample` and returns how many.
* `overflowed()` reads STATUS.RXOVF; draining **doesn't clear it** (sticky) — `clear_overflow()` **writes** `IRQ_OVF` to IRQ_STAT.
* **Write-1-to-clear (W1C) registers are written, never read-modify-written.** If IRQ_STAT holds `RX|OVF = 0b11` and you do `IRQ_STAT |= IRQ_RX` you write `0b11` → you just **cleared the overflow flag you never looked at**. Writing exactly the bit you handled (`0b01`) clears only that one.
* **Interrupt handshake:** `enable_rx_irq(true)` sets `IRQ_EN = RX|OVF`; the line asserts when `IRQ_STAT & IRQ_EN` is non-zero; **draining the FIFO does not de-assert it** — `ack_rx_irq()` must write `IRQ_RX` to IRQ_STAT. Forget the ack → the ISR re-enters forever (interrupt storm). Masked (IRQ_EN = 0) → no line.

> **Misconception box — "reading a register is harmless".** `DATA` pops; `IRQ_STAT` may clear on read in other chips; check the manual for each register's *access type* (RO, RW, W1C, RC "read-clears", WO).
> **Misconception box 2 — "drivers only know hardware".** A good driver has *no* knowledge of the simulator: that's what lets the same driver run unchanged on silicon.

### 3.2 Build 2 — A single-producer/single-consumer ring (lock-free)

**Problem.** An ISR produces bytes; the main loop consumes them. Locks are forbidden in ISRs (08/03 §3.4), disabling interrupts adds jitter. If there is **exactly one producer context and one consumer context**, a ring buffer needs **no lock at all**.
**Design (`SpscRing<T, N>`, N a power of two, usable capacity N):**
* `head_` (written only by the producer), `tail_` (written only by the consumer): **free-running** counters that just keep incrementing (wrap harmlessly at 2^64 because N divides it); slot = `index & (N−1)` (a mask instead of a modulo — hence N = 2^k).
* `size = head − tail`; **empty** if 0; **full** if `size == N` (no "waste one slot" trick needed with free-running counters).
* `push`: load own `head` (relaxed), load `tail` with **acquire**; if full → false; write the element; **store `head + 1` with release**. `pop` is symmetric.
*Why release/acquire?* The **release** store of `head` guarantees the *element write* is visible to anyone who sees the new head; the consumer's **acquire** load of `head` pairs with it. Without that ordering the consumer could see the new index but a stale element (reordering by compiler or core).
*Why not a plain `volatile size_t`?* `volatile` gives no ordering for the *other* memory (the element). It's the pack 08/01 §3.5 lesson, applied.

Tests: FIFO order and full/empty; **wraps 1000×**; and `TwoThreadsPreserveOrderAndLoseNothing`: producer thread pushes 200 000 integers, consumer pops them — checks **order and no loss** (bounded spin so a broken ring fails instead of hanging; the thread is always joined before asserting). Run it under **TSan too** if available (VERIFY: `-fsanitize=thread` can't be combined with ASan).

> **Misconception box — "lock-free means works with any number of threads".** SPSC works because each index has **one writer**. Two producers need CAS (MPSC/MPMC) — different, harder algorithms.
> **Misconception box 2 — "relaxed atomics everywhere is faster and fine".** Your *own* index may be relaxed; the *other side's* index needs acquire, and your publish needs release.

### 3.3 Build 3 — Interrupt-driven UART over two SPSC rings

Two rings, and the beautiful part: **each ring has exactly one producer context and one consumer context**:

| Ring | Producer | Consumer |
|---|---|---|
| **TX ring** | main code (`write`) | **ISR** (sends one byte per TXE interrupt) |
| **RX ring** | **ISR** (stores each received byte) | main code (`read`) |

`write(p, n)`: push as many bytes as fit, **return that count** (the caller handles back-pressure: `write` of 10 into a 4-slot ring returns **4**), then **enable TXEIE** so the ISR starts. `read(b)`: pop from the RX ring.
`isr()` (the model's `irq_pending()` is what the interrupt controller watches):
* **RXNE set:** read DR **once**, push to the RX ring; if full → **drop and count** (`rx_dropped()`): the **oldest** four survive (test), the newest are lost — a deliberate, *visible* policy.
* **TXE and TXEIE:** pop one byte → write DR; **if the TX ring is now empty, clear TXEIE (mask the interrupt)**. *Why?* TXE (transmit data register empty) is **level-triggered**: with nothing to send it stays asserted, so a still-enabled TXEIE would re-enter forever → **interrupt storm**, starving main (`TransmitsEverythingInOrderThenMasksItsInterrupt` asserts `txeie == false` at the end; the test's `service()` caps ISR re-entries at 64 so a bug fails instead of hanging).
Full-duplex test: TX traffic and RX bytes interleaved must not disturb each other (wire sees `1 2 3 4 5`; RX ring yields `0xA2, 0xA5`).

> **Misconception box — "the ISR should keep the TX interrupt enabled, it's simpler".** An enabled level interrupt with no work is a livelock. **Arm on demand, disarm when idle.**

### 3.4 Build 4 — The audio pipeline: priority inversion, then the fix

Scenario (a *design* exercise, then the solution numbers): three tasks, **1 tick = 1 ms**:

| Task | Priority | Starts | Does | Role |
|---|---|---|---|---|
| `codec_out` | **3** (highest) | 2 | lock bus mutex, run 2, unlock | **deadline-critical**: must finish within **8 ticks** of release |
| `dsp_resample` | 2 | 3 | run 10 | CPU-bound, takes no lock |
| `logger` | **1** (lowest) | 0 | lock bus mutex, run 5, unlock | flash logging |

**Predict first:** logger holds the lock t0–; codec arrives t2 and blocks; DSP arrives t3, preempts logger (2 > 1) and runs 10 ticks; logger then needs 2 more ticks; codec runs after that.
**Result (`./pipeline_trace`):**
```
OFF: runs : l l l d d d d d d d d d d l l c c      codec finish 17, response 15  -> MISSED (deadline 8)
ON : runs : l l l l l c c d d d d d d d d d d      codec finish  7, response  5  -> met
```
With inheritance the logger inherits priority 3 at t=2 when the codec blocks, so DSP cannot preempt it; logger unlocks at t=5, codec runs t5–6. DSP finishes last (17) — it was never entitled to delay the codec.
**Mapping to real FreeRTOS** (exercise, not built here — **VERIFY** the port layout): run the three tasks on the **FreeRTOS POSIX/Linux simulator port** (kernel's `portable/ThirdParty/GCC/Posix`), using `xSemaphoreCreateMutex()` (inheritance on) versus `xSemaphoreCreateBinary()` (no inheritance), `vTaskDelay`/busy loops of known length, and print a tick trace through the trace hook. Expect the same two outcomes.

### 3.5 Embedded Linux basics (what changes when you leave the MCU)

* **Boot chain:** ROM → bootloader (U-Boot, VERIFY) → kernel + **device tree** (describes hardware: addresses, IRQs, clocks) → root filesystem → init (systemd/BusyBox). Built by Yocto/Buildroot.
* **User space cannot touch registers directly:** the **MMU** gives each process its own virtual address space; hardware is accessed through the **kernel**: a **driver** (character device in `/dev`, `ioctl`, sysfs), or **UIO** (map a device's registers into user space, interrupts via `read()` on `/dev/uioN`), or **`/dev/mem` + `mmap`** (prototyping only — unsafe).
* **Real-time:** stock Linux is **not hard real-time**; `PREEMPT_RT`, CPU isolation, `SCHED_FIFO`, `mlockall`, and moving the time-critical bit to an **MCU/FPGA** are the options (VERIFY).
* **Trade-offs vs MCU/RTOS:** boot time (seconds vs ms), memory (MBs vs KBs), power, full networking/filesystems/tooling, and a *bigger attack surface and jitter*.
* Our builds map cleanly: the **register map + driver** you wrote is the **kernel driver's logic**; the **SPSC ring** is what you'd use between an IRQ thread and a worker; the **sim** replaces `/dev/uio0`.

### 3.6 C++ on microcontrollers

Why is "C++ on an MCU" a recurring interview topic? Because default C++ drags in features that need a runtime, heap or large code:

| Rule | Why |
|---|---|
| **`-fno-exceptions`** | unwinding tables cost flash; throw paths are non-deterministic; no hard-real-time bound. Use status codes / `std::optional` / `expected`-like types (the parsers in packs 04/capstone do) |
| **`-fno-rtti`** | `typeid`/`dynamic_cast` metadata costs flash; prefer static polymorphism (templates/CRTP, as `RadioDriver<Bus>`) |
| **Static allocation; no heap on the hot path** | `new`/`malloc` are non-deterministic, fragment, can fail; use fixed `std::array`, pools, placement-new, rings (`SpscRing`) |
| **Careful with static initialisation** | global constructors run in the startup code (pack 08/01); order across translation units is undefined; prefer `constexpr`/trivial init |
| **Avoid iostream, `std::string` growth, `std::function` allocation** | code size and hidden heap use |
| **Templates and `constexpr` are free** | they run at compile time: zero-cost abstraction (register wrappers, bit-fields, unit types) |
| **`volatile` for MMIO, `std::atomic` for sharing** | pack 08/01 §3.5 |

Linker flags: `-ffunction-sections -fdata-sections -Wl,--gc-sections` drop unused code; `-Os` vs `-O2`; map file for size accounting (VERIFY toolchain flags).

### 3.7 JTAG and SWD — how you debug without a printf

Both are **debug access ports** into the chip: halt the core, read/write memory and registers (so also flash programming), set breakpoints/watchpoints, single-step. **JTAG**: IEEE 1149.1 — **4–5 signals** (TCK, TMS, TDI, TDO, optional TRST) — a scan chain that can daisy-chain many devices (also boundary scan: test PCB solder joints).
**SWD** (Arm Serial Wire Debug): **2 pins** (SWCLK, SWDIO), same capabilities for Arm cores, fewer pins on small packages; optional **SWO** pin for trace/printf output. Tools: probe (ST-Link, J-Link, CMSIS-DAP) + **OpenOCD** + **GDB**. Real-world lessons: a **locked/read-protected** chip needs a mass erase; a core stuck in WFI or a clock gated in low-power can't be reached — **connect under reset**; debug *and* production pins shouldn't be repurposed carelessly (VERIFY).

### 3.8 FPGA offload awareness (when software isn't enough)

Radio DSP (channelisation, wideband FIR/FFT, modem inner loops at tens of Msps) can exceed what a CPU does in real time or within a deterministic latency. An **FPGA** (or an SoC with FPGA fabric: Zynq-style) implements the pipeline as parallel hardware; the CPU controls it through an **AXI-lite register map** — exactly the register-map-plus-driver model of §3.1 (**the same driver pattern**, a different engine behind it). 
Decide by: **sample rate × operations per sample** vs CPU capacity; **deterministic latency** (fabric: cycles; CPU: scheduling jitter); power/area/cost; development cost (HDL/HLS, longer builds, harder debug); flexibility (software is easy to update). Typical split: **FPGA** for the high-rate front-end (DDC, filtering, FFT), **CPU** for protocol logic. Etherstack-style stacks stay mostly software; know the hand-off point (VERIFY product architecture).

## 4. Worked examples

**(a) Frequency register:** 446.00625 MHz → `446 006 250 / 25 = 17 840 250` = `0x0110387A` (the value the test reads back from `REG_FREQ`). Reverse: `FREQ × 25`.
**(b) W1C trap:** IRQ_STAT = `0b11`; wrong: `write(IRQ_STAT, read(IRQ_STAT) | 1)` writes `0b11` (clears both). Right: `write(IRQ_STAT, 1)`.
**(c) SPSC arithmetic:** N = 4, head = 7, tail = 5 → size 2; push writes slot `7 & 3 = 3`, head becomes 8; pop reads slot `5 & 3 = 1`.
**(d) UART TX timeline:** write(6 bytes) into 16-slot ring: returns 6, TXEIE on; each TXE interrupt sends one byte; after the 6th, ring empty → ISR clears TXEIE; wire shows 6 bytes in order.

## 5. Questions

1. **Q1 (Draw)** — Draw the layers: DSP simulator → radio model (register map) → `RadioDriver<Bus>` → application. What does each layer know about the others? (W 3.1)
2. **Q2 (Calculate)** — FREQ register for 446.00625 MHz and for 446 006 260 Hz; the first rejected frequency below/above the limits. (W 3.1)
3. **Q3 (Explain)** — The W1C trap: show the wrong and right way to acknowledge the RX interrupt when OVF is also pending. (W 3.1)
4. **Q4 (Explain)** — `DATA` pops the FIFO: write the correct `read_sample` logic; what goes wrong if you read DATA to "peek"? (W 3.1)
5. **Q5 (Design)** — FIFO full: drop newest (hardware) vs drop oldest vs back-pressure: pick one for a radio RX path and justify; what flag/counter must exist? (W 3.1)
6. **Q6 (Explain)** — Why does an SPSC ring need no lock? State the invariant and who writes which index. (W 3.2)
7. **Q7 (Explain)** — Which loads/stores need acquire/release and what bug appears if they're relaxed? (W 3.2)
8. **Q8 (Calculate)** — N = 8, head = 13, tail = 9: size, free slots, the slot index of the next push and pop. Why does wraparound of the counters not matter? (W 3.2)
9. **Q9 (Trace)** — UART TX: ring of 16, `write` of 6 bytes: list ISR events and when TXEIE is cleared. (W 3.3)
10. **Q10 (Explain)** — What happens if the ISR leaves TXEIE enabled with an empty ring? How would you detect it on hardware? (W 3.3)
11. **Q11 (Design)** — RX ring full: counts and policy; what does the application learn and when? (W 3.3)
12. **Q12 (Predict + Calculate)** — Predict, then compute, the codec response without/with inheritance; show per-tick the logger's effective priority. (W 3.4)
13. **Q13 (Design)** — Tune the scenario: what change makes the *fix* unnecessary (e.g. shorter critical section, ceiling, different lock)? (W 3.4)
14. **Q14 (Compare)** — Embedded Linux vs RTOS-on-MCU for a radio gateway board: three criteria each; how do you access a peripheral from user space? (W 3.5)
15. **Q15 (Design)** — Write seven rules for C++ firmware (exceptions, RTTI, heap, init order, …) with a one-line reason each. (W 3.6)
16. **Q16 (Explain)** — JTAG vs SWD: wires, capabilities, three practical problems when a board "won't connect". (W 3.7)
17. **Q17 (Decide)** — A new channel filter at 20 Msps: CPU, DSP core, or FPGA? State the numbers you would compute first and the interface you'd expose to firmware. (W 3.8)
18. **Q18 (Etherstack)** — How would you test radio firmware in CI with **no radio and no live signals**? (W 3.1, 3.8)

## 6. Self-assessment

- [ ] Describe register map → driver → ISR → ring → task, in that order, from memory.
- [ ] Explain W1C vs RMW and level-triggered TXE.
- [ ] Justify release/acquire in the SPSC ring.
- [ ] Run the priority-inversion trace without/with inheritance.
- [ ] Give the "no exceptions, no RTTI, no heap on the hot path" rationale; compare JTAG and SWD; name when an FPGA is justified.

**VERIFY list:** register layout is invented (not a real chip); `-fsanitize=thread` incompatibility with ASan; FreeRTOS POSIX port path and trace-hook names; UIO/`/dev/mem` behaviour on your kernel; PREEMPT_RT status;
SWD/JTAG pin counts and probe capabilities for your board; Zynq/AXI-lite specifics; toolchain flags (`--gc-sections`, `-fno-exceptions`) for your compiler; U-Boot/Yocto/Buildroot details.
