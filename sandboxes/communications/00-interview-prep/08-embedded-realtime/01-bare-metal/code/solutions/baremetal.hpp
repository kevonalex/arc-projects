// baremetal.hpp  --  REFERENCE SOLUTION for Pack 08/01 (reset vector, startup, linker layout, MMIO, volatile).
// Everything runs on the HOST: "registers" are ordinary variables so the logic is unit-testable under ASan/UBSan.
// Cortex-M vector-table facts (word 0 = initial SP, word 1 = reset vector with Thumb bit) are from the ARM ARM: VERIFY.
#pragma once
#include <cstddef>
#include <cstdint>

namespace bm {

// ------------------------------------------------------------------ vector table (W 3.1)
struct ResetInfo {
  uint32_t initial_sp;
  uint32_t reset_pc;  // address with the Thumb bit (bit 0) stripped
  bool thumb;         // bit 0 of the stored vector; must be 1 on Cortex-M
};
inline ResetInfo parse_reset(const uint32_t* vt) { return {vt[0], vt[1] & ~1u, (vt[1] & 1u) != 0}; }
// Exceptions 0..15 are system entries (SP, reset, NMI, HardFault, ...); device IRQ n is word 16 + n.
inline uint32_t isr_handler_address(const uint32_t* vt, int irq) { return vt[16 + irq] & ~1u; }

// ------------------------------------------------------------------ startup code (W 3.2)
// Copy the initialised-data image from its load address (flash) to its run address (RAM).
inline void copy_data(uint8_t* dst, const uint8_t* src, size_t n) {
  for (size_t i = 0; i < n; ++i) dst[i] = src[i];
}
// Zero .bss.
inline void zero_bss(uint8_t* dst, size_t n) {
  for (size_t i = 0; i < n; ++i) dst[i] = 0;
}

// ------------------------------------------------------------------ linker layout (W 3.3)
struct Layout {
  uint32_t text_start;
  uint32_t rodata_start;
  uint32_t data_lma;  // where the initial values of .data sit in flash
  uint32_t data_vma;  // where .data lives at run time (RAM)
  uint32_t bss_start;
  uint32_t flash_used;
  uint32_t ram_used;
  uint32_t stack_top;  // initial SP: top of RAM (full-descending stack)
};
inline uint32_t align_up(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }
inline Layout layout_sections(uint32_t flash_base, uint32_t ram_base, uint32_t ram_size, uint32_t vectors, uint32_t text,
                              uint32_t rodata, uint32_t data, uint32_t bss) {
  Layout l{};
  l.text_start = align_up(flash_base + vectors, 4);
  l.rodata_start = align_up(l.text_start + text, 4);
  l.data_lma = align_up(l.rodata_start + rodata, 4);
  l.data_vma = ram_base;
  l.bss_start = align_up(l.data_vma + data, 4);
  l.flash_used = align_up(l.data_lma + data, 4) - flash_base;
  l.ram_used = align_up(l.bss_start + bss, 4) - ram_base;
  l.stack_top = ram_base + ram_size;
  return l;
}

// ------------------------------------------------------------------ MMIO helpers (W 3.4)
inline void reg_set_bits(volatile uint32_t* reg, uint32_t mask) { *reg = *reg | mask; }
inline void reg_clear_bits(volatile uint32_t* reg, uint32_t mask) { *reg = *reg & ~mask; }
// Replace a multi-bit field: (mask is unshifted, e.g. 0x3 for a 2-bit field).
inline void reg_modify(volatile uint32_t* reg, uint32_t mask, uint32_t shift, uint32_t value) {
  *reg = (*reg & ~(mask << shift)) | ((value & mask) << shift);
}
inline uint32_t reg_read_field(const volatile uint32_t* reg, uint32_t mask, uint32_t shift) { return (*reg >> shift) & mask; }

// GPIO model with an ODR and a write-1-to-act BSRR (bits 0..15 set, bits 16..31 reset; set wins if both).
struct GpioSim {
  volatile uint32_t odr = 0;
  void write_bsrr(uint32_t v) {
    const uint32_t set = v & 0xFFFFu, reset = v >> 16;
    odr = (odr & ~reset) | set;
  }
};
// Main code does a read-modify-write on ODR; an interrupt fires BETWEEN the read and the write and sets isr_bit
// with its own read-modify-write.  Return the final ODR.  (The ISR's bit is lost: W 3.4.)
inline uint32_t rmw_with_interrupt_between(GpioSim& g, uint32_t main_bit, uint32_t isr_bit) {
  const uint32_t tmp = g.odr;  // main: read
  g.odr = g.odr | isr_bit;     // ISR: read-modify-write, completes
  g.odr = tmp | main_bit;      // main: write back a stale value
  return g.odr;
}
// Same interleaving, but both sides use BSRR (a single store, no read): nothing is lost.
inline uint32_t bsrr_with_interrupt_between(GpioSim& g, uint32_t main_bit, uint32_t isr_bit) {
  g.write_bsrr(main_bit);
  g.write_bsrr(isr_bit);
  return g.odr;
}

// UART status/data model: reading DR has a SIDE EFFECT (clears RXNE) - why the compiler must not reorder/elide reads.
constexpr uint32_t kSrRxne = 1u << 5;
constexpr uint32_t kSrOre = 1u << 3;
struct UartSim {
  uint8_t rx = 0;
  bool rxne = false, ore = false;
  void inject(uint8_t b) {
    if (rxne) ore = true;  // previous byte never read: overrun, old byte lost
    rx = b;
    rxne = true;
  }
  uint32_t read_sr() const { return (rxne ? kSrRxne : 0u) | (ore ? kSrOre : 0u); }
  uint8_t read_dr() {
    rxne = false;
    return rx;
  }
};
// Poll once: if a byte is waiting, read DR exactly once and return true.
inline bool uart_try_read(UartSim& u, uint8_t& out) {
  if (!(u.read_sr() & kSrRxne)) return false;
  out = u.read_dr();
  return true;
}

// ------------------------------------------------------------------ volatile is not atomic (W 3.5)
// A 64-bit free-running timer read as two 32-bit halves on a 32-bit MCU.  `hi()`/`lo()` are the two loads.
// Read hi, lo, hi again; if hi changed, the low word rolled over in between -> retry.
template <class HiFn, class LoFn>
inline uint64_t read64_consistent(HiFn hi, LoFn lo) {
  uint32_t h1, l, h2;
  do {
    h1 = hi();
    l = lo();
    h2 = hi();
  } while (h1 != h2);
  return (static_cast<uint64_t>(h1) << 32) | l;
}

}  // namespace bm
