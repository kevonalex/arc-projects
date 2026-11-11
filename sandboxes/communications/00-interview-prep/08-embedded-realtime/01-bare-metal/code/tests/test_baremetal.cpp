// Tests for Pack 08/01. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "baremetal.hpp"

using namespace bm;

// ---------------------------------------------------------------- vector table (W 3.1)
TEST(VectorTable, ResetAndStack) {
  uint32_t vt[32] = {};
  vt[0] = 0x20005000u;
  vt[1] = 0x08000101u;  // address 0x08000100 with the Thumb bit set
  auto r = parse_reset(vt);
  EXPECT_EQ(r.initial_sp, 0x20005000u);
  EXPECT_EQ(r.reset_pc, 0x08000100u);
  EXPECT_TRUE(r.thumb);
  vt[1] = 0x08000100u;  // forgetting the Thumb bit is a classic HardFault-on-reset bug
  EXPECT_FALSE(parse_reset(vt).thumb);
}
TEST(VectorTable, DeviceIrqsStartAtWordSixteen) {
  uint32_t vt[64] = {};
  vt[16 + 0] = 0x08000201u;
  vt[16 + 5] = 0x08000341u;
  EXPECT_EQ(isr_handler_address(vt, 0), 0x08000200u);
  EXPECT_EQ(isr_handler_address(vt, 5), 0x08000340u);
}

// ---------------------------------------------------------------- startup (W 3.2)
TEST(Startup, CopyDataTouchesExactlyN) {
  uint8_t flash[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t ram[10];
  for (auto& b : ram) b = 0xEE;
  copy_data(ram + 1, flash, 8);
  EXPECT_EQ(ram[0], 0xEE);  // guard before
  for (int i = 0; i < 8; ++i) EXPECT_EQ(ram[1 + i], i + 1);
  EXPECT_EQ(ram[9], 0xEE);  // guard after
}
TEST(Startup, ZeroBssClearsGarbageOnly) {
  uint8_t ram[8];
  for (auto& b : ram) b = 0xA5;  // power-on RAM contents are undefined
  zero_bss(ram + 2, 4);
  EXPECT_EQ(ram[1], 0xA5);
  for (int i = 2; i < 6; ++i) EXPECT_EQ(ram[i], 0);
  EXPECT_EQ(ram[6], 0xA5);
}

// ---------------------------------------------------------------- linker layout (W 3.3)
TEST(Linker, AlignUp) {
  EXPECT_EQ(align_up(0, 4), 0u);
  EXPECT_EQ(align_up(1, 4), 4u);
  EXPECT_EQ(align_up(4, 4), 4u);
  EXPECT_EQ(align_up(5, 8), 8u);
}
TEST(Linker, SectionAddresses) {
  // 20 KiB RAM; 0xC0-byte vector table; .text 0x1234, .rodata 0x56, .data 0x21, .bss 0x103.
  Layout l = layout_sections(0x08000000u, 0x20000000u, 0x5000u, 0xC0u, 0x1234u, 0x56u, 0x21u, 0x103u);
  EXPECT_EQ(l.text_start, 0x080000C0u);
  EXPECT_EQ(l.rodata_start, 0x080012F4u);
  EXPECT_EQ(l.data_lma, 0x0800134Cu);   // .rodata ends at ...4A, rounded up to 4
  EXPECT_EQ(l.data_vma, 0x20000000u);   // run address is in RAM, load address is in flash
  EXPECT_EQ(l.bss_start, 0x20000024u);  // .data ends at ...21, rounded up to 4
  EXPECT_EQ(l.flash_used, 0x1370u);
  EXPECT_EQ(l.ram_used, 0x128u);        // .bss ends at ...127, rounded up to ...128
  EXPECT_EQ(l.stack_top, 0x20005000u);
  EXPECT_NE(l.data_lma, l.data_vma);
}

// ---------------------------------------------------------------- MMIO helpers (W 3.4)
TEST(Mmio, SetClearModify) {
  volatile uint32_t r = 0;
  reg_set_bits(&r, 0x0000000Fu);
  EXPECT_EQ(r, 0x0Fu);
  reg_clear_bits(&r, 0x3u);
  EXPECT_EQ(r, 0x0Cu);
  reg_modify(&r, 0x3u, 4, 2);  // 2-bit field at bit 4 := 0b10
  EXPECT_EQ(r, 0x2Cu);
  reg_modify(&r, 0x3u, 4, 7);  // value wider than the field is truncated, neighbours untouched
  EXPECT_EQ(r, 0x3Cu);
  EXPECT_EQ(reg_read_field(&r, 0x3u, 4), 3u);
  EXPECT_EQ(reg_read_field(&r, 0xFu, 0), 0xCu);
}
TEST(Mmio, BsrrSetResetAndSetWins) {
  GpioSim g;
  g.write_bsrr(0x0005u);  // set pins 0 and 2
  EXPECT_EQ(g.odr, 0x5u);
  g.write_bsrr(0x0001u << 16);  // reset pin 0
  EXPECT_EQ(g.odr, 0x4u);
  g.write_bsrr(0x0002u);  // set pin 1: pin 2 untouched
  EXPECT_EQ(g.odr, 0x6u);
  g.write_bsrr(0x00040004u);  // set AND reset pin 2 in one write: SET wins
  EXPECT_EQ(g.odr, 0x6u);
  g.write_bsrr(0x00060000u);  // reset pins 1 and 2
  EXPECT_EQ(g.odr, 0x0u);
}
TEST(Mmio, ReadModifyWriteLosesTheInterruptsUpdate) {
  GpioSim g;
  EXPECT_EQ(rmw_with_interrupt_between(g, 0x1u, 0x80u), 0x1u);  // 0x80 vanished
}
TEST(Mmio, BsrrIsImmuneToTheSameInterleaving) {
  GpioSim g;
  EXPECT_EQ(bsrr_with_interrupt_between(g, 0x1u, 0x80u), 0x81u);
}
TEST(Mmio, ReadingDrHasASideEffect) {
  UartSim u;
  uint8_t b = 0;
  EXPECT_FALSE(uart_try_read(u, b));  // nothing there: must not touch DR
  u.inject(0x41);
  ASSERT_TRUE(uart_try_read(u, b));
  EXPECT_EQ(b, 0x41);
  EXPECT_FALSE(uart_try_read(u, b));  // RXNE cleared by the one DR read
  u.inject(0x42);
  u.inject(0x43);  // 0x42 never read -> overrun, 0x42 lost
  EXPECT_NE(u.read_sr() & kSrOre, 0u);
  ASSERT_TRUE(uart_try_read(u, b));
  EXPECT_EQ(b, 0x43);
}

// ---------------------------------------------------------------- volatile is not atomic (W 3.5)
namespace {
struct Timer64 {  // each lo() read lets "time pass" by `step` ticks, like a real free-running counter
  uint64_t v;
  uint64_t step;
  uint32_t hi() const { return static_cast<uint32_t>(v >> 32); }
  uint32_t lo() {
    const uint32_t r = static_cast<uint32_t>(v);
    v += step;
    return r;
  }
};
}  // namespace
TEST(Volatile, SimpleReadWithoutRollover) {
  Timer64 t{0x0000000100001000ull, 0x10};
  EXPECT_EQ(read64_consistent([&] { return t.hi(); }, [&] { return t.lo(); }), 0x0000000100001000ull);
}
TEST(Volatile, RolloverBetweenTheHalvesIsDetectedAndRetried) {
  Timer64 t{0x00000000FFFFFFF0ull, 0x20};
  // First attempt: hi=0, lo=0xFFFFFFF0, then the timer rolls over, hi'=1 -> mismatch -> retry.
  uint64_t got = read64_consistent([&] { return t.hi(); }, [&] { return t.lo(); });
  EXPECT_EQ(got, 0x0000000100000010ull);
  // A naive "lo then hi" read of the same situation yields 0x1_FFFFFFF0: off by 4 billion ticks.
  Timer64 t2{0x00000000FFFFFFF0ull, 0x20};
  const uint32_t lo = t2.lo();
  const uint32_t hi = t2.hi();
  EXPECT_EQ((static_cast<uint64_t>(hi) << 32) | lo, 0x00000001FFFFFFF0ull);
}
