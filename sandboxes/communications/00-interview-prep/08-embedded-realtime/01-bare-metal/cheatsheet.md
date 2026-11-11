# Pack 08/01 cheatsheet — bare metal (every line cites the worksheet section)

Host-simulated registers. VERIFY items: worksheet §6.

| Fact / rule | Worksheet |
|---|---|
| Vector table word 0 = initial SP; word 1 = reset vector; device IRQ n at word 16+n | §3.1, Q1, Q3 |
| Reset vector has bit 0 = 1 (Thumb); parse strips it; missing bit → HardFault at reset | §3.1, Q1, Q2 |
| Table holds addresses of handlers, not code | §3.1 |
| Startup: (clocks) → copy .data → zero .bss → static ctors → main | §3.2, Q4 |
| copy_data / zero_bss touch exactly n bytes (guard-byte tests) | §3.2 |
| Startup copy runs before the C++ runtime: freestanding code only | §3.2, Q14 |
| .text/.rodata in flash; .data in RAM with image in flash; .bss in RAM, no image | §3.3, Q6, Q7 |
| .data has LMA (flash) ≠ VMA (RAM) | §3.3, Q6 |
| Layout: text = flash+vectors; rodata after text; data_lma after rodata; all 4-aligned | §3.3, Q5 |
| Layout example: 0x080000C0 / 0x080012F4 / 0x0800134C / 0x20000024; flash 0x1370, RAM 0x128 | §3.3 |
| Stack grows down from stack_top = RAM base + size | §3.3 |
| const globals stay in flash (no RAM) | §3.3, Q7 |
| MMIO: registers are addresses; reads/writes can have side effects | §3.4, Q10 |
| Reading UART DR pops the byte and clears RXNE; check SR first, read DR once | §3.4, Q10 |
| Overrun: byte arrives before previous read → ORE, one byte lost | §3.4 |
| reg_modify(mask unshifted, shift, value) truncates value to the mask | §3.4, Q8 |
| Read-modify-write (`reg \|= bit`) is load/OR/store: an ISR's update can be lost | §3.4, Q9 |
| BSRR: low 16 bits set, high 16 reset, single write, set wins (VERIFY) | §3.4, Q9 |
| volatile: compiler may not remove/merge/reorder those accesses | §3.5, Q11 |
| volatile ≠ atomic, ≠ barrier, ≠ thread-safe | §3.5, Q11, Q13 |
| Torn 64-bit read: hi,lo,hi and retry on hi mismatch | §3.5, Q12 |
| Use std::atomic (acquire/release) or a critical section for shared state | §3.5, Q13 |
| Prod-only hard fault suspects: Thumb bit, stack/linker, .data/.bss, clocks/wait states, volatile | Q15 |
