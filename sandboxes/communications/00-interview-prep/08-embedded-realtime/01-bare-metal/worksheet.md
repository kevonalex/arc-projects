# Pack 08/01 — Bare metal: boot, vector table, startup code, linker sections, MMIO, and why `volatile` is not atomic

> **SAFETY.** Everything runs on the **host** with simulated registers (ordinary variables) so you can test under ASan/UBSan. No hardware, no radio, no live signals.
> Real-target addresses/bit positions below are from memory (STM32/Cortex-M style) — **VERIFY** against the reference manual and the ARM Architecture Reference Manual before relying on them.

---

## 1. Why this matters (Etherstack context)

Etherstack's protocol stacks end up on customers' hardware: radios, modems, FPGA-SoC boards, sometimes small MCUs. The people who ship them are expected to understand **what happens before `main()`**, why a global
variable has the value it has, how a driver talks to a register, and why "I made it `volatile`" does not fix a race. These are classic embedded interview questions because each one separates *people who've read the datasheet* from
*people who've only used a HAL*.

## 2. Prerequisites

Binary/hex, pointers, structs, C/C++ storage classes (`static`, `const`, globals). Pack 03/07 (memory model) helps for §3.5.

```bash
cd code
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -R "^student\." --output-on-failure   # your stubs: FAIL until implemented
ctest --test-dir build -R "^solution\."                      # reference solution: PASS
```

## 3. Concept build-up

### 3.1 Concrete first: what the CPU does at reset (Cortex-M)

At power-on the CPU has no instructions in its pipeline and no stack. It does exactly two memory reads from the **vector table** at the start of flash (address 0x0800_0000 on STM32; aliased to 0x0 — VERIFY):

| Word | Meaning |
|---|---|
| 0 | **initial stack pointer** (MSP) — typically the top of RAM, e.g. `0x20005000` for 20 KiB at `0x20000000` |
| 1 | **reset vector** — address of `Reset_Handler`, **with bit 0 = 1** (the Thumb bit) |
| 2…15 | system exceptions (NMI, HardFault, SVCall, PendSV, SysTick …) |
| 16 + n | **device interrupt n** (so IRQ 5 is word 21) |

So the table entry `0x08000101` means "start executing Thumb code at **0x08000100**": `parse_reset` strips the Thumb bit (`reset_pc = 0x08000100`, `thumb = true`). `isr_handler_address(vt, n)` reads `vt[16+n]` and strips it too.

*Why bit 0 = 1?* Cortex-M executes **only Thumb** instructions. Branch-with-link and interworking use bit 0 to select the instruction set; on Cortex-M, an address with bit 0 = 0 would request ARM mode → **UsageFault/HardFault on reset**.
Forgetting the `|1` (a hand-written or script-generated table) is a classic "dead board" bug (test `ResetAndStack`).

> **Misconception box — "the vector table holds code".** It holds **addresses** of handlers. And index 0 is *not* a handler at all, it's the stack pointer value.

### 3.2 What runs before `main()` — the startup code

`Reset_Handler` has to build the C/C++ world that `main` assumes:

1. (Optionally set up clocks, FPU, VTOR.) 2. **Copy `.data`** (initialised globals) from flash to RAM. 3. **Zero `.bss`** (zero-initialised globals). 4. Run **static constructors** (`.init_array`, C++ globals; VERIFY symbol names). 5. Call `main`.

`copy_data(dst, src, n)` and `zero_bss(dst, n)` must touch **exactly n bytes** — tests put guard bytes before and after (off-by-one at either end corrupts a neighbouring variable: the classic "my UART buffer changes when I update the display" bug).

*Why do we need this at all on a PC but never write it?* On a PC the OS loader does these steps. On bare metal **you are the loader**.

### 3.3 Linker sections and why `.data` is copied from flash

| Section | Contains | Where it **lives** at run time | Where its **initial bytes** are stored |
|---|---|---|---|
| `.text` | code | flash | flash |
| `.rodata` | `const` data, string literals | flash | flash |
| `.data` | globals with non-zero initialiser (`int x = 5;`) | **RAM** (read/write) | **flash** (an image after `.rodata`) |
| `.bss` | globals initialised to zero / uninitialised | **RAM** | nowhere (just zeroed; saves flash) |

**Why is `.data` copied from flash?** RAM is **volatile memory** — it forgets everything when power drops. The initial value `5` must survive in non-volatile **flash**, but the variable has to be **writable**, so its run-time home is RAM.
Hence each `.data` object has **two addresses**: **LMA** (load address, in flash) and **VMA** (virtual/run address, in RAM). The startup copy moves LMA → VMA. `.bss` needs no flash image: "all zero" is easy to regenerate (and saves flash).
(`const` globals stay in flash only: no RAM cost — important on a 20 KiB part; but a `const` object with a constructor may land in RAM — VERIFY.)

**Worked layout (`layout_sections`).** Flash base `0x08000000`, RAM base `0x20000000`, 20 KiB RAM (`0x5000`), vector table `0xC0`, `.text` 0x1234, `.rodata` 0x56, `.data` 0x21, `.bss` 0x103; all sections 4-byte aligned:

| Item | Computation | Result |
|---|---|---|
| `text_start` | flash + vectors | `0x080000C0` |
| `rodata_start` | text_start + 0x1234 | `0x080012F4` |
| `data_lma` | rodata_start + 0x56 = `...134A` → align4 | `0x0800134C` |
| `data_vma` | RAM base | `0x20000000` |
| `bss_start` | RAM + 0x21 → align4 | `0x20000024` |
| `flash_used` | (`data_lma` + 0x21) − flash base = 0x136D → align4 | `0x1370` |
| `ram_used` | (`0x24 + 0x103` = 0x127) → align4 | `0x128` |
| `stack_top` | RAM base + 0x5000 | `0x20005000` |

*Check:* `data_lma ≠ data_vma` — the whole point. The stack grows **down** from `stack_top` toward `.bss`; `ram_used` + max stack depth must be ≤ RAM size (a linker `ASSERT` or a stack-painting check catches overflow).

> **Misconception box — "globals live where I declared them".** The *name* is one thing; `.data`'s bytes sit in flash until startup copies them. Skip `copy_data` and your `int x = 5;` reads **random RAM garbage**; skip `zero_bss` and `static int counter;` starts at garbage.

### 3.4 Memory-mapped I/O (MMIO)

Peripherals appear as **memory addresses**: writing `0x40010C10` may switch a pin; reading `0x40004804` may *take a byte out of the UART*. So registers are not like variables:

* **Reads can have side effects.** Reading the UART data register (DR) **clears RXNE** (the "byte available" flag). Read it twice and the second read returns the same stale byte with RXNE clear; read it when nothing arrived and you've *poked* hardware for nothing. `uart_try_read`: check SR first, read DR **exactly once**, never otherwise. If the next byte arrives before you read the previous one → **overrun** (ORE set, one byte lost; `0x42` is lost in the test).
* **Writes can have side effects** and some bits are *write-1-to-clear*.
* **Access ordering and width matter.**

**Bit manipulation helpers:** `reg_set_bits`, `reg_clear_bits`, `reg_modify(reg, mask, shift, value)` (mask is *unshifted*; the value is truncated to the mask — field 4..5 := 7 gives 3, neighbours untouched), `reg_read_field`.

**Read-modify-write (RMW) is a race.** `ODR |= 1;` compiles to *load, OR, store*. Interleaving: main loads ODR (=0); an interrupt does its own `ODR |= 0x80` and returns; main stores `0 | 1 = 1`. **The ISR's bit vanished** — `rmw_with_interrupt_between` returns 0x1 (expected 0x81).
**Fix: BSRR** (bit set/reset register, STM32-style): the lower 16 bits *set* pins, the upper 16 *reset* them, in a **single write** with no read: pins you don't mention aren't touched. Same interleaving → `0x81`. (If a pin is both set and reset in one write, **set wins** — VERIFY on your part.)
GPIO shown here is the simplest peripheral: a direction register, an output data register (ODR), an input data register (IDR), BSRR. Pull-ups, alternate functions and speed bits live in the other registers.

> **Misconception box — "a single C statement is a single bus access".** `x |= 1` is three steps. Only designs with atomic set/clear registers (BSRR, "bit-banding" aliases, LDREX/STREX loops) are race-free.

### 3.5 What `volatile` does — and the things it does not

**What it does:** tells the **compiler** that every read/write of this object is an observable side effect: it may not remove, merge, cache in a register, or reorder it *relative to other volatile accesses*.
Without `volatile`, `while (!(UART->SR & RXNE)) {}` is rewritten as an infinite loop on a cached value.

**What it does not do:**
1. **Not atomic.** A 32-bit volatile load is one instruction on Cortex-M, but `|=` is three, and a **64-bit** volatile read on a 32-bit CPU is *two* loads — **torn read** (below).
2. **No memory barrier for the CPU/bus.** It doesn't order writes against the CPU's own write buffer or other cores/DMA — use `DSB`/`DMB` (or the platform's barrier macros, VERIFY).
3. **No mutual exclusion**, no thread/ISR safety. In C++ it is *not* a synchronisation primitive: use `std::atomic` (or disable interrupts / use a lock) for shared state (pack 03/07 for acquire/release).

**Torn 64-bit read.** A 64-bit free-running counter read as hi/lo 32-bit halves: read `lo = 0xFFFFFFF0`; the counter rolls over; read `hi = 1`: result `0x1_FFFFFFF0` is **4 billion ticks too late**. Safe pattern: **read hi, lo, hi again; retry while the two `hi` reads differ**
(`read64_consistent`). Test: timer at `0x00000000_FFFFFFF0` stepping 0x20 per `lo()` read → naive read gives `0x00000001_FFFFFFF0`; the retry gives `0x00000001_00000010`.

> **Misconception box — "volatile makes it thread-safe / atomic / fast-to-read".** It prevents *compiler optimisation* of the access. That is all. (Interview gold: be able to say exactly this.)

## 4. Worked examples

**(a) Parse reset:** `vt[0]=0x20005000, vt[1]=0x08000101` → SP `0x20005000`, PC `0x08000100`, Thumb yes. `vt[16+5]=0x08000341` → handler `0x08000340`.
**(b) BSRR arithmetic:** write `0x0005` → set pins 0,2 (ODR `0x5`); write `1<<16` → reset pin 0 (ODR `0x4`); write `0x00040004` → pin 2 both set and reset → **set wins** → stays 1.
**(c) Field insert:** r=0x0C; `reg_modify(r, 0x3, 4, 2)` → bits 5:4 := 10 → `0x2C`; then value 7 → truncated to 3 → `0x3C`.

## 5. Questions

1. **Q1 (Decode)** — Given `vt[0]=0x20005000, vt[1]=0x08000101`: initial SP, reset PC, instruction set. What if `vt[1]=0x08000100`? (W 3.1)
2. **Q2 (Explain)** — Why must the reset vector have bit 0 set on Cortex-M? What symptom do you see if it doesn't? (W 3.1)
3. **Q3 (Calculate)** — Where is the handler address for device IRQ 5? For the SysTick exception (VERIFY its index)? (W 3.1)
4. **Q4 (Explain)** — List what `Reset_Handler` does before `main()`; what happens if you skip (a) `copy_data` (b) `zero_bss`? (W 3.2)
5. **Q5 (Calculate)** — Reproduce the layout table above for a different part: flash `0x0800_0000`, RAM `0x2000_0000`/32 KiB, vectors 0x1C0, `.text` 0x2001, `.rodata` 0x101, `.data` 0x33, `.bss` 0x2A1. (W 3.3)
6. **Q6 (Explain)** — Why does `.data` have two addresses? Why does `.bss` have none in flash? (W 3.3)
7. **Q7 (Classify)** — Where do these live (flash/RAM/stack/none): `const int k = 5;`, `int a = 5;`, `int b;`, `static const char s[] = "hi";`, a local `int c;`, a `constexpr` that the compiler inlines. (W 3.3)
8. **Q8 (Calculate)** — `reg_modify` on `r = 0xF0F0`: set bits 11:8 to `0b0101`; then clear bit 15; give the final value. (W 3.4)
9. **Q9 (Explain)** — Draw the RMW interleaving that loses the ISR's update; show the same interleaving with BSRR. (W 3.4)
10. **Q10 (Explain)** — Why can reading a status/data register change hardware state? Give the UART example and what `uart_try_read` must therefore do. (W 3.4)
11. **Q11 (Explain/Interview)** — "What does `volatile` do? What doesn't it do?" Give a 30-second answer with an example for each. (W 3.5)
12. **Q12 (Calculate)** — Show a torn 64-bit read numerically and explain the hi–lo–hi loop. Why can't you just disable interrupts here? (W 3.5)
13. **Q13 (Design)** — A shared `volatile bool flag` between ISR and main: when is it enough? When is it not? Use `std::atomic<bool>` — what does it add? (W 3.5)
14. **Q14 (Design)** — Your `.data` copy loop runs *before* the C++ runtime exists: what must it not use (heap, exceptions, `memcpy` that needs initialised data, global constructors)? (W 3.2)
15. **Q15 (Etherstack)** — A customer's radio board hard-faults at boot only in the production build: list five boot-time suspects in order (Thumb bit, stack top/linker script, `.data` not copied, clocks not set, VTOR). (W 3.1–3.3)

## 6. Self-assessment

- [ ] Say what words 0 and 1 of the vector table are, and why the Thumb bit.
- [ ] Draw flash/RAM memory maps with `.text/.rodata/.data image` and `.data/.bss/heap/stack`.
- [ ] Explain LMA vs VMA in one sentence.
- [ ] Show the RMW race and the BSRR fix.
- [ ] Say precisely what `volatile` does and does not do.

**VERIFY list:** vector-table layout and Thumb requirement (ARMv6-M/v7-M ARM); flash alias at 0x0 and VTOR relocation; `.init_array` symbol names and startup order; BSRR set-wins rule and register offsets (STM32 RM); SysTick exception index (15);
which `const` objects are placed in RAM; memory barrier instructions/macros for your core; behaviour of overrun flag clearing sequence.
