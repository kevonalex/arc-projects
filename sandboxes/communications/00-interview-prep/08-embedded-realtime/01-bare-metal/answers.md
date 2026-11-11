# Pack 08/01 — Answers (bare metal)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Host-simulated registers; no hardware. **VERIFY** = check the ARM ARM / reference manual.

---

## Q1 — Decode: reset words

**Answer.** SP = `0x20005000` (top of 20 KiB RAM); reset PC = `0x08000100`; Thumb = yes (bit 0 = 1). With `0x08000100` stored (bit 0 = 0) the CPU would be asked to enter ARM state → fault at reset.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "PC = 0x08000101" | Read the table literally. | Bit 0 is a mode flag, not part of the address. |
| "Word 0 is the reset handler" | First = start. | Word 0 is the initial stack pointer. |

## Q2 — Explain: Thumb bit

**Answer.** Cortex-M implements only the Thumb instruction set. Branch/exception-return use bit 0 to choose state; loading an address with bit 0 = 0 requests ARM state, which doesn't exist → UsageFault/HardFault (often immediately at reset: board "dead", debugger shows PC in the fault handler). Linkers set bit 0 on function symbols for Thumb; hand-built tables or scripts that take the raw address forget it.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Alignment requirement" | Odd address looks misaligned. | Bit 0 is stripped; instructions are 2-byte aligned, the bit is a mode flag. |

## Q3 — Calculate: handler addresses

**Answer.** Device IRQ 5 → `vt[16+5] = vt[21]`, byte offset `21 × 4 = 0x54`; strip bit 0. SysTick is exception 15 → `vt[15]`, offset `0x3C` (VERIFY: the system exception numbering). 

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "vt[5]" | IRQ number. | Words 0–15 are SP + system exceptions; device IRQs start at 16. |

## Q4 — Explain: before `main()`

**Answer.** (Clocks/VTOR/FPU) → copy `.data` (flash→RAM) → zero `.bss` → static constructors → `main`. Skip copy: initialised globals contain garbage (`int x = 5;` ≠ 5; also config tables/vtables in RAM). Skip zeroing: `static` counters, buffers and "NULL" pointers start random; the bug is intermittent because RAM garbage varies after power cycles.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The compiler initialises globals" | Works on PCs. | On bare metal the startup code performs what the OS loader would. |
| "Zeroing .bss is optional: RAM is zero at power-up" | Sometimes true on a bench. | Not guaranteed; soft resets keep old contents. |

## Q5 — Calculate: other part's layout

| Item | Result |
|---|---|
| text_start | `0x080001C0` |
| rodata_start | align4(`0x080001C0 + 0x2001 = 0x080021C1`) = `0x080021C4` |
| data_lma | align4(`0x080021C4 + 0x101 = 0x080022C5`) = `0x080022C8` |
| data_vma | `0x20000000` |
| bss_start | align4(`0x20000033`) = `0x20000034` |
| flash_used | align4(`0x080022C8 + 0x33 = 0x080022FB`) − base = `0x22FC` |
| ram_used | align4(`0x20000034 + 0x2A1 = 0x200002D5`) − base = `0x2D8` |
| stack_top | `0x20008000` |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "flash_used excludes the .data image" | Image isn't code. | It occupies flash: the initial values must be stored. |
| "ram_used includes the stack" | Stack is in RAM. | ram_used here is static data; stack is budgeted separately (stack_top − ram_used ≥ max depth). |

## Q6 — Explain: LMA vs VMA

**Answer.** `.data` objects must be writable (RAM = run address/VMA) but must hold initial values at power-on (flash = load address/LMA). Startup copies LMA→VMA. `.bss` is all-zero, so no image is needed in flash; startup memset replaces it (saving flash).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Flash is executed in place, so RAM copy is unnecessary" | XIP works for code. | Flash can't be written at run time by `x++`. |

## Q7 — Classify

| Object | Place |
|---|---|
| `const int k = 5;` (global, address taken) | flash `.rodata` (no RAM) |
| `int a = 5;` | RAM `.data` (+ 4 bytes image in flash) |
| `int b;` | RAM `.bss` |
| `static const char s[] = "hi";` | flash `.rodata` |
| local `int c;` | stack (or register) |
| `constexpr` inlined | nowhere (immediate in code), unless odr-used |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "const globals use RAM" | PC habit. | On MCUs `.rodata` stays in flash. |
| "Uninitialised global is in .data" | 'Data'. | `.bss`; zero by startup. |

## Q8 — Calculate: reg_modify

**Answer.** `0xF0F0` → clear bits 11:8 (already 0) and insert `0x5` → `0xF5F0`; clear bit 15 (`& ~0x8000`) → **`0x75F0`**.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "0xF5F0 OR 0x8000" | Wrong direction. | Clearing means AND with the inverse mask. |
| Mask `0x500` | Shifted mask. | `reg_modify` takes an **unshifted** mask (0xF) + shift 8. |

## Q9 — Explain: RMW race

**Answer.**

| Step | main | ISR | ODR |
|---|---|---|---|
| 1 | load ODR → 0 | | 0 |
| 2 | | load 0, OR 0x80, store | 0x80 |
| 3 | OR 0x1 → 0x1, store | | **0x01** (0x80 lost) |

BSRR: main writes `0x0001`, ISR writes `0x0080`, each a single store that only sets its own pin → ODR = **0x81** regardless of order.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Make ODR volatile" | Common advice. | Volatile prevents caching, not the interleaving of load/modify/store. |
| "Disable interrupts always" | Works. | Costly/jitter; BSRR needs no lock. |

## Q10 — Explain: side-effecting reads

**Answer.** Many status/data registers model FIFOs: reading DR *pops* the byte and clears RXNE. A debugger watch window or an extra "just checking" read can **eat** data. `uart_try_read` therefore checks `SR.RXNE` first (reads SR, which has no side effect here), reads DR exactly once if set, and returns false without touching DR otherwise. Overrun (ORE) means a byte arrived before the previous one was read.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Read DR to see if there's data" | One register. | Pops the byte and may be zero/stale if empty. |

## Q11 — Interview: volatile (model answer)

"`volatile` tells the compiler each access to that object is observable: it can't be removed, merged, cached in a register or reordered against other volatile accesses. I need it for memory-mapped registers and for variables changed by an ISR that a polling loop reads. It does **not** make operations atomic — `x |= 1` is still load/modify/store and a 64-bit access on a 32-bit core can tear; it does not insert hardware memory barriers; and in C++ it is not a synchronisation primitive, so for data shared between threads or ISR and main I use `std::atomic` or a critical section."

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "volatile = atomic" | Seen in old code. | No atomicity guarantee. |
| "volatile flushes caches" | Sounds like 'memory'. | It affects only the compiler. |

## Q12 — Calculate: torn read

**Answer.** Counter `0x0000_0000_FFFF_FFF0`: lo read `0xFFFFFFF0`, then counter increments past rollover → hi reads `1` → combined `0x1_FFFFFFF0` (≈ 4.29×10⁹ ticks off). hi–lo–hi: reads hi=0, lo=0xFFFFFFF0, hi'=1 → mismatch → retry: hi=1, lo=0x10, hi=1 → `0x1_00000010`. Disabling interrupts doesn't help: the **hardware counter** keeps running regardless of interrupts.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Read lo first then hi" | Order feels irrelevant. | Reordering doesn't remove the race; need a consistency check. |

## Q13 — Design: shared flag

**Answer.** `volatile bool` set by an ISR and polled by main is enough on a single core when the flag is a single-word, one-writer/one-reader, no *other data* depends on it. Not enough if the flag publishes other data (the data writes may be reordered before the flag on cores/DMA/compiler barriers) or if multiple writers/RMW are involved. `std::atomic<bool>` with release (set) / acquire (read) guarantees that data written before the flag is visible after seeing it, and gives atomic RMW.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Atomic is slower so use volatile" | Folk wisdom. | On Cortex-M, atomic load/store of word size compiles to plain loads + barriers where needed. |

## Q14 — Design: copy loop constraints

**Answer.** Runs with no heap, no initialised globals, no constructors, no exceptions, no stack protector/canary initialised from `.data`, possibly no FPU, and before clocks configured. Must be freestanding code: simple loops (or assembly) touching only linker symbols; `memcpy` from libc is OK only if it has no dependency on initialised data. Avoid `std::` features needing the runtime.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use std::copy so it's safe" | Standard library. | Templates inline fine, but anything with `.data` dependencies or exceptions breaks. |

## Q15 — Etherstack: prod-only hard fault (ordered suspects)

1. **Thumb bit / vector table** wrong (linker/VTOR after bootloader). 2. **Stack top** (linker script RAM size) or stack overflow due to the optimised build's inlining. 3. **`.data` not copied / `.bss` not zeroed** (script changed, `-ffunction-sections --gc-sections` dropped symbols). 4. **Clock/flash wait-states** (optimised code runs faster than the flash can supply). 5. **Unaligned/atomic access** or peripheral clock not enabled before access. Also: optimiser exposing missing `volatile`. Method: halt in HardFault, read CFSR/stacked PC (VERIFY register names).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Compiler bug" | Only prod differs. | UB/missing volatile/wrong linker script are far likelier. |
