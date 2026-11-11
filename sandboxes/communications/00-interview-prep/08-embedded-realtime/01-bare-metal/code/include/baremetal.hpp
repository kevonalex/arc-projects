// baremetal.hpp  --  STUBS for Pack 08/01 (student version).  Reference: ../solutions/baremetal.hpp
// Everything runs on the HOST: "registers" are ordinary variables so logic is unit-testable under ASan/UBSan.
// Cortex-M facts (word 0 = initial SP, word 1 = reset vector, Thumb bit 0 set) are from the ARM ARM: VERIFY.
#pragma once
#include <cstddef>
#include <cstdint>

namespace bm {

// ---- vector table (W 3.1) ----
struct ResetInfo {
  uint32_t initial_sp;
  uint32_t reset_pc;  // address with the Thumb bit (bit 0) stripped
  bool thumb;         // bit 0 of the stored vector; must be 1 on Cortex-M
};
inline ResetInfo parse_reset([[maybe_unused]] const uint32_t* vt) { return {0, 0, false}; }  // TODO W 3.1
// Device IRQ n lives at word 16 + n (words 0..15 are system exceptions). Strip the Thumb bit.
inline uint32_t isr_handler_address([[maybe_unused]] const uint32_t* vt, [[maybe_unused]] int irq) { return 0; }  // TODO W 3.1

// ---- startup code (W 3.2) ----
// Copy .data's initial values from flash (src) to RAM (dst); zero .bss.  Touch exactly n bytes.
inline void copy_data([[maybe_unused]] uint8_t* dst, [[maybe_unused]] const uint8_t* src, [[maybe_unused]] size_t n) {}  // TODO W 3.2
inline void zero_bss([[maybe_unused]] uint8_t* dst, [[maybe_unused]] size_t n) {}  // TODO W 3.2

// ---- linker layout (W 3.3) ----
struct Layout {
  uint32_t text_start;
  uint32_t rodata_start;
  uint32_t data_lma;  // where the initial values of .data sit in flash
  uint32_t data_vma;  // where .data lives at run time (RAM)
  uint32_t bss_start;
  uint32_t flash_used;
  uint32_t ram_used;
  uint32_t stack_top;  // initial SP: top of RAM
};
inline uint32_t align_up(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }  // a is a power of two (given)
// Flash: [vector table][.text][.rodata][.data image]   RAM: [.data][.bss] ... [stack top].  All sections 4-byte aligned.
//   text_start = flash_base + vectors; rodata_start = after .text; data_lma = after .rodata;
//   data_vma = ram_base; bss_start = after .data; flash_used/ram_used measured from the bases, rounded up to 4.
inline Layout layout_sections([[maybe_unused]] uint32_t flash_base, [[maybe_unused]] uint32_t ram_base,
                              [[maybe_unused]] uint32_t ram_size, [[maybe_unused]] uint32_t vectors,
                              [[maybe_unused]] uint32_t text, [[maybe_unused]] uint32_t rodata,
                              [[maybe_unused]] uint32_t data, [[maybe_unused]] uint32_t bss) {
  return Layout{};  // TODO W 3.3
}

// ---- MMIO helpers (W 3.4) ----
inline void reg_set_bits([[maybe_unused]] volatile uint32_t* reg, [[maybe_unused]] uint32_t mask) {}    // TODO W 3.4
inline void reg_clear_bits([[maybe_unused]] volatile uint32_t* reg, [[maybe_unused]] uint32_t mask) {}  // TODO W 3.4
// Replace a field: mask is UNSHIFTED (0x3 for a 2-bit field); value is truncated to the mask.
inline void reg_modify([[maybe_unused]] volatile uint32_t* reg, [[maybe_unused]] uint32_t mask, [[maybe_unused]] uint32_t shift,
                       [[maybe_unused]] uint32_t value) {}  // TODO W 3.4
inline uint32_t reg_read_field([[maybe_unused]] const volatile uint32_t* reg, [[maybe_unused]] uint32_t mask,
                               [[maybe_unused]] uint32_t shift) {
  return 0xFFFFFFFFu;  // TODO W 3.4
}
// GPIO: write_bsrr: bits 0..15 SET the matching ODR bit, bits 16..31 RESET it; if both for one pin, SET wins.
struct GpioSim {
  volatile uint32_t odr = 0;
  void write_bsrr([[maybe_unused]] uint32_t v) {}  // TODO W 3.4
};
// Interleaving: main reads ODR; ISR sets isr_bit via read-modify-write; main writes (old | main_bit). Return ODR.
inline uint32_t rmw_with_interrupt_between([[maybe_unused]] GpioSim& g, [[maybe_unused]] uint32_t main_bit,
                                           [[maybe_unused]] uint32_t isr_bit) {
  return 0;  // TODO W 3.4: reproduce the lost update
}
// Same interleaving with BSRR on both sides. Return ODR.
inline uint32_t bsrr_with_interrupt_between([[maybe_unused]] GpioSim& g, [[maybe_unused]] uint32_t main_bit,
                                            [[maybe_unused]] uint32_t isr_bit) {
  return 0;  // TODO W 3.4
}
// UART model (given): reading DR has a side effect - it clears RXNE.
constexpr uint32_t kSrRxne = 1u << 5;
constexpr uint32_t kSrOre = 1u << 3;
struct UartSim {
  uint8_t rx = 0;
  bool rxne = false, ore = false;
  void inject(uint8_t b) {
    if (rxne) ore = true;
    rx = b;
    rxne = true;
  }
  uint32_t read_sr() const { return (rxne ? kSrRxne : 0u) | (ore ? kSrOre : 0u); }
  uint8_t read_dr() {
    rxne = false;
    return rx;
  }
};
// Poll once: if RXNE is set read DR exactly once into out and return true; else return false without touching DR.
inline bool uart_try_read([[maybe_unused]] UartSim& u, [[maybe_unused]] uint8_t& out) { return true; }  // TODO W 3.4

// ---- volatile is not atomic (W 3.5) ----
// 64-bit timer read as two 32-bit loads.  Read hi, lo, hi; retry while the two hi reads differ.
template <class HiFn, class LoFn>
inline uint64_t read64_consistent([[maybe_unused]] HiFn hi, [[maybe_unused]] LoFn lo) {
  return 0;  // TODO W 3.5
}

}  // namespace bm
