# Pack 08/04 cheatsheet — embedded builds (every line cites the worksheet section)

Emulated hardware; synthetic signals only. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Peripheral = register map + driver; driver templated on a Bus (model / MMIO / mock) | §3.1, Q1 |
| Radio map: CTRL 0x00, STATUS 0x04, DATA 0x08, FREQ 0x0C, IRQ_EN 0x10, IRQ_STAT 0x14 | §3.1 |
| FREQ in 25 Hz units: 446 006 250 Hz → 17 840 250 (0x0110387A); round to nearest | §3.1, Q2 |
| Valid carrier 30 MHz … 6 GHz; reject out-of-range and write nothing | §3.1, Q2 |
| DATA read pops FIFO: check RXRDY, read once | §3.1, Q4 |
| FIFO depth 8; overflow loses the newest, OVF sticky until cleared | §3.1, Q5 |
| IRQ_STAT is write-1-to-clear: write the handled bit, never RMW | §3.1, Q3 |
| Draining the FIFO does not ack the IRQ; forgetting the ack = storm | §3.1, Q3 |
| Receiver disabled (EN=0) drops samples | §3.1 |
| SPSC ring: N power of two, free-running head/tail, slot = index & (N−1), capacity N | §3.2, Q6, Q8 |
| One writer per index → no lock; release publish, acquire observe | §3.2, Q6, Q7 |
| SPSC ≠ MPMC; two producers need CAS | §3.2, Q6 |
| UART: TX ring (main→ISR), RX ring (ISR→main): each ring SPSC | §3.3 |
| write() returns how many fit; caller handles back-pressure | §3.3, Q9 |
| ISR: RXNE → read DR once → push or count drop; TXE&&TXEIE → pop one; empty → clear TXEIE | §3.3, Q9 |
| TXE is level-triggered: leaving TXEIE on with empty ring = interrupt storm | §3.3, Q10 |
| RX ring full: drop newest, count; oldest four survive | §3.3, Q11 |
| Audio pipeline: codec 3 / dsp 2 / logger 1; deadline 8 ticks | §3.4, Q12 |
| Without inheritance: codec finish 17, response 15 (MISS); with: finish 7, response 5 | §3.4, Q12 |
| Logger inherits priority 3 at t2, back to 1 at t5 | §3.4, Q12 |
| Real FreeRTOS: mutex (inheritance) vs binary semaphore on the POSIX sim (VERIFY) | §3.4 |
| Embedded Linux: device tree, drivers/UIO, not hard RT (PREEMPT_RT helps) | §3.5, Q14 |
| C++ on MCU: no exceptions, no RTTI, static alloc, careful init, constexpr/templates free | §3.6, Q15 |
| JTAG 4–5 wires + chain; SWD 2 wires (+SWO); connect under reset; locked chip → mass erase | §3.7, Q16 |
| FPGA for high-rate DSP; CPU controls via AXI-lite register map (same driver pattern) | §3.8, Q17 |
| CI without radio: unit + register-model + channel simulator + HIL with cabled signals; no live traffic | Q18 |
