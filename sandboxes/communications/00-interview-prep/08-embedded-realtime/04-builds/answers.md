# Pack 08/04 — Answers (embedded builds)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Emulated hardware, synthetic signals only. **VERIFY** = check your platform.

---

## Q1 — Draw: layers

**Answer.** `DSP simulator (tone generator) → push_rx_sample() → RadioModel (registers + FIFO + IRQ line) ⇄ read32/write32 ⇄ RadioDriver<Bus> → application/stack`.
* Simulator knows only `push_rx_sample`. * Model knows registers, not the driver or application. * Driver knows the register map **only** (via the Bus interface), never model internals. * Application knows driver methods, not registers.
Result: swap the model for a `volatile` MMIO bus on hardware without touching the driver.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Driver calls `model.fifo_`" | Convenient in tests. | Couples the driver to the simulator; breaks on silicon. |

## Q2 — Calculate: frequency register

**Answer.** `446 006 250 / 25 = 17 840 250` (`0x0110387A`). `446 006 260 / 25 = 17 840 250.4` → rounds to **17 840 250** (reads back 446 006 250 Hz). Rejected: anything `< 30 MHz` (e.g. 1000 Hz) or `> 6 GHz` (7 GHz); on rejection **nothing is written**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "FREQ holds Hz" | Natural unit. | It holds 25 Hz steps: 32-bit register covers up to 107 GHz at that resolution. |
| "Clamp out-of-range to the limit" | Friendly. | Silent retune to a different frequency is dangerous (regulatory/interference); reject loudly. |

## Q3 — Explain: W1C trap

**Answer.** IRQ_STAT = `0b11` (RX and OVF). Wrong: `write(IRQ_STAT, read(IRQ_STAT) & ~1)` or `|= 1` — on a W1C register, writing a `1` clears, so writing back `0b11` (or even `~1 = ...11110` with other bits set) **clears OVF as well**: the overflow evidence is gone. Right: `write(IRQ_STAT, IRQ_RX)` — only the bit you handled. Test `InterruptHandshake`: draining doesn't ack; `ack_rx_irq()` writes `IRQ_RX`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Read, clear the bit, write back" | The standard RMW idiom. | W1C semantics invert it: you must write 1 only to what you want cleared. |

## Q4 — Explain: DATA pops

**Answer.** `if (STATUS & RXRDY) { out = DATA (one read); return true; } return false;` Never read DATA to peek: the read removes the sample (data loss) and on empty FIFO returns 0/junk indistinguishable from a valid 0 sample. `drain` loops until RXRDY is clear and returns the count.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Read DATA twice to be sure" | Defensive. | Two pops = two samples consumed. |

## Q5 — Design: FIFO full policy

**Answer.** Hardware here drops the **newest** and latches OVF (sticky): the oldest data is intact and contiguous, and the **gap** is known to be after the last good sample. Dropping the oldest keeps the latest data (good for latency-critical control) but silently discards what you had. Back-pressure is impossible on an RF/ADC stream (the source can't wait). For a radio RX path: *count and flag*, then **resynchronise** (discard the burst/frame in progress). Needed: sticky OVF flag + driver counter + log, and clear only after handling.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Ignore overflow, it's rare" | It is, in the lab. | Under load it's exactly when failures matter; silent corruption. |

## Q6 — Explain: why SPSC needs no lock

**Answer.** Invariant: `0 ≤ head − tail ≤ N`. Only the **producer** writes `head`, only the **consumer** writes `tail`; each reads the other's index but never writes it, so there is no write–write race. Producer owns slots `[tail+size…]` free area; consumer owns `[tail…head)` filled area; the regions don't overlap as long as the indices are observed correctly (full/empty checks).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "It's safe with 2 producers" | Same code. | Two writers to `head` race: needs CAS. |

## Q7 — Explain: memory orders

**Answer.** Producer: write element, then `head.store(h+1, release)`. Consumer: `head.load(acquire)`, then read element. Release/acquire creates *happens-before*: if the consumer sees the new head it also sees the element. Likewise consumer `tail.store(t+1, release)` / producer `tail.load(acquire)` so the producer doesn't overwrite a slot still being read. Relaxed everywhere → consumer may see the new head but a stale element (or the producer overwrites a slot early) — a rare, load-dependent corruption (shows up on ARM/weakly-ordered CPUs, or after compiler reordering, not necessarily on x86).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Works on my x86, so it's right" | Tests pass. | x86 is strongly ordered; the compiler can still reorder, and ARM won't forgive. |

## Q8 — Calculate: SPSC arithmetic

**Answer.** size = 13 − 9 = **4**; free = 8 − 4 = **4**; next push slot `13 & 7 = 5`; next pop slot `9 & 7 = 1`. Counters wrap modulo 2^64; since 8 divides 2^64, `index & 7` stays consistent across the wrap, and `head − tail` (unsigned) is still the right size.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "slot = 13 % 8 works for any N" | `%` is general. | Mask works only for powers of two (that's why N=2^k is required — and why the template static_asserts). |

## Q9 — Trace: UART TX

**Answer.** `write(6 bytes)` → pushes 6 → returns 6 → `txeie = 1`. Interrupt 1: pop byte 1 → DR; … interrupt 6: pop byte 6 → DR; ring now empty → **clear TXEIE**. The wire sees the six bytes in order; `irq_pending()` is false.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "ISR sends all 6 in one go" | Efficient. | Blocks other ISRs, and the data register accepts one byte per TXE. |

## Q10 — Explain: TXEIE storm

**Answer.** TXE means "data register empty" — a **level**, true whenever idle. With TXEIE enabled and no data, the interrupt is *always pending*: ISR runs, has nothing to do, returns, re-enters — 100 % CPU in the ISR, main never runs. Detect: CPU usage/starved main loop, watchdog reset, scope shows constant ISR entry, interrupt counter exploding. Fix: arm TXEIE when queuing data, disarm when the ring empties. (Test: `service()` caps re-entry at 64 so a buggy ISR fails the test rather than hanging it.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Clear the pending bit in NVIC" | NVIC has one. | The peripheral re-asserts instantly: the *source* must be masked. |

## Q11 — Design: RX ring full

**Answer.** Policy: drop the **newest** byte (ring unchanged: oldest survive) and increment `rx_dropped`. The application learns by polling the counter (or an event flag) — add a high-water mark and flow control (RTS/CTS, XOFF) when possible; size the ring for worst-case latency × byte rate. (Test: 6 bytes into a 4-slot ring → dropped 2; reads give 0..3.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Overwrite the oldest silently" | Ring buffers do that. | Silent loss; also breaks SPSC ownership (the ISR would write the consumer's index). |

## Q12 — Predict + Calculate: pipeline

**Answer.** Without inheritance: logger runs t0–2 (3 ticks); codec arrives t2, blocks; DSP at t3 preempts logger and runs t3–12; logger t13–14 (finishes critical section), unlocks at 15; codec t15–16 → finish **17**, response **15 > 8 = MISS**. With inheritance: logger effective priority 1 → **3** from t2 (codec blocked on its mutex); runs t2–4, unlocks at t5 (priority back to 1); codec t5–6, finish **7**, response **5 ≤ 8**; DSP runs t7–16. 

| Tick | 0–1 | 2 | 3–4 | 5–6 | 7+ |
|---|---|---|---|---|---|
| logger's effective prio | 1 | **3** (codec blocks) | 3 | back to 1 (done) | – |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "DSP at priority 2 can't affect codec at 3" | Priorities compare directly. | It starves the *logger*, which holds what the codec needs. |

## Q13 — Design: avoiding the problem

**Answer.** (1) **Shorten** the critical section (logger copies to a buffer, then locks only to enqueue: 5 → 1 tick). (2) **Eliminate sharing:** logger task owns the flash/bus; the codec sends messages via a queue (no lock). (3) **Ceiling** protocol on the mutex. (4) **Give the codec its own resource** (separate bus) or DMA. (5) Move logging to **idle priority and batch**. Inheritance remains the safety net but design away the contention first.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Make the DSP task shorter" | Reduces the delay. | Doesn't remove the inversion; any new medium task re-creates it. |

## Q14 — Compare: Linux vs RTOS

| Criterion | Embedded Linux | RTOS on MCU |
|---|---|---|
| Latency/determinism | soft real-time (PREEMPT_RT helps) | hard real-time, µs jitter |
| Footprint/boot | MBs, seconds | KBs, ms |
| Ecosystem | networking, filesystems, gRPC, SSH | minimal, you build it |
| Power | higher | very low |
| Failure surface | kernel + userland updates | small, certifiable |

User-space peripheral access: kernel **driver** (`/dev`, ioctl/sysfs), **UIO** (mmap registers, IRQ via read), or `/dev/mem` mmap for prototyping only. A gateway typically uses Linux for IP/gRPC and an MCU/FPGA for the hard-real-time radio side (VERIFY per product).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Linux can read registers via pointers like bare metal" | Same C code. | MMU/virtual addressing: you need a mapping or a driver. |

## Q15 — Design: C++ firmware rules (seven)

1. `-fno-exceptions`: code size/determinism. 2. `-fno-rtti`: flash; use templates/CRTP. 3. **No heap on the hot path**: determinism, fragmentation, failure handling. 4. **Static/pool allocation** and `std::array`. 5. **Beware global constructors/init order** — run in startup, order undefined. 6. **No `iostream`/hidden allocators** (`std::function`, `std::string` growth). 7. **`volatile` for MMIO, `std::atomic` for sharing**; `constexpr` for tables. (Also: check map file for size; `--gc-sections`.)

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "C++ is too heavy for MCUs" | Folklore. | Templates/constexpr are free; discipline avoids the heavy parts. |

## Q16 — Explain: JTAG vs SWD

**Answer.** JTAG: 4–5 wires, scan chain (multiple devices, boundary scan), universal. SWD: 2 wires (+SWO trace) on Arm cores, same debug features. "Won't connect": (1) chip read-protected/locked → mass erase; (2) core in deep sleep or clocks gated → connect **under reset**; (3) pins repurposed by firmware (SWD pins as GPIO) or wrong voltage/missing ground/long wires; (4) wrong target config in OpenOCD (VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "SWD is just a slower JTAG" | Fewer wires. | Different protocol; comparable functionality on Arm; often faster per pin. |

## Q17 — Decide: 20 Msps filter

**Answer.** Compute: samples/s × MACs/sample (e.g. 128-tap complex FIR: 4 real MACs per complex tap → 20e6 × 128 × 4 ≈ 10 GMAC/s) vs a CPU's ≈ 1–10 GMAC/s peak (SIMD) shared with the stack ⇒ FPGA (DSP slices) or a DSP block; consider latency determinism and power. Interface: AXI-lite register map (coefficient RAM, enable, gain, status/overflow flags, IRQ) + AXI-stream samples via DMA — the same **register map + driver** pattern as build 1.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Always FPGA for DSP" | Faster. | Cost/time; a decimated lower rate may fit a CPU/DSP. |

## Q18 — Etherstack: CI without radio

**Answer.** Layer it: (1) **unit tests** of codecs/parsers with synthetic vectors; (2) **driver tests against the register-map model** with a **DSP simulator** feeding generated IQ/audio (this pack); (3) **channel simulator** (AWGN, CFO, delay) for modem tests (capstone M3); (4) RTOS behaviours via the tick simulator; (5) **hardware-in-the-loop** on a bench with cabled, attenuated, shielded test signals only — **never live public-safety traffic**. Run all under ASan/UBSan (and TSan separately).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Record live traffic as test vectors" | Realistic. | Unlawful in many places and unnecessary; synthetic vectors are reproducible. |
