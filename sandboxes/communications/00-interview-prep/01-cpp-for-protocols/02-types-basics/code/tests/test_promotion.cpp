#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <type_traits>

#include <es_types/promotion.hpp>

using namespace es::types;

// A language fact, checked by the compiler (passes even with the stubs):
static_assert(std::is_same_v<decltype(std::uint8_t{1} + std::uint8_t{1}), int>,
              "uint8_t + uint8_t is int, not uint8_t");
static_assert(std::is_same_v<decltype(std::uint16_t{1} * std::uint16_t{1}), int>,
              "uint16_t * uint16_t is int too (and 65535*65535 overflows int!)");
static_assert(std::is_same_v<decltype(1 + 1u), unsigned>, "int + unsigned -> unsigned");

TEST(AddU8, WrapsModulo256) {
    EXPECT_EQ(add_u8_wrap(200, 100), 44);  // 300 - 256
    EXPECT_EQ(add_u8_wrap(255, 1), 0);
    EXPECT_EQ(add_u8_wrap(1, 2), 3);
    EXPECT_EQ(add_u8_wrap(255, 255), 254);  // 510 - 256
}

TEST(AddU8, OverflowDetection) {
    EXPECT_TRUE(u8_add_overflows(200, 100));
    EXPECT_TRUE(u8_add_overflows(255, 1));
    EXPECT_FALSE(u8_add_overflows(255, 0));
    EXPECT_FALSE(u8_add_overflows(100, 100));
    EXPECT_FALSE(u8_add_overflows(128, 127));  // exactly 255
    EXPECT_TRUE(u8_add_overflows(128, 128));   // exactly 256
}

TEST(ReadBe, Be16) {
    const std::uint8_t a[] = {0x12, 0x34};
    EXPECT_EQ(read_be16(a), 0x1234u);
    const std::uint8_t b[] = {0xFF, 0xFE};
    EXPECT_EQ(read_be16(b), 0xFFFEu);
}

TEST(ReadBe, Be32HighBitSet) {
    const std::uint8_t a[] = {0x80, 0x00, 0x00, 0x00};
    EXPECT_EQ(read_be32(a), 0x80000000u);
    const std::uint8_t b[] = {0xFF, 0x00, 0x00, 0x01};
    EXPECT_EQ(read_be32(b), 0xFF000001u);
    const std::uint8_t c[] = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(read_be32(c), 0x01020304u);
}

TEST(ReadBe, Be48NoSignExtensionNoOversizedShift) {
    const std::uint8_t a[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x01};
    EXPECT_EQ(read_be48(a), 0x800000000001ull);
    const std::uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_EQ(read_be48(b), 0xFFFFFFFFFFFFull);  // NOT 0xFFFFFFFFFFFFFFFF
    const std::uint8_t c[] = {0x00, 0x00, 0x80, 0x00, 0x00, 0x00};  // byte 2 >= 0x80: the int<<24 trap
    EXPECT_EQ(read_be48(c), 0x000080000000ull);
}

TEST(SignExtend, TwelveBitField) {
    EXPECT_EQ(sign_extend(0xFFF, 12), -1);
    EXPECT_EQ(sign_extend(0x800, 12), -2048);
    EXPECT_EQ(sign_extend(0x7FF, 12), 2047);
    EXPECT_EQ(sign_extend(0x000, 12), 0);
}

TEST(SignExtend, EdgeWidths) {
    EXPECT_EQ(sign_extend(1, 1), -1);
    EXPECT_EQ(sign_extend(0, 1), 0);
    EXPECT_EQ(sign_extend(0xFFFFFFFFu, 32), -1);
    EXPECT_EQ(sign_extend(0x80000000u, 32), std::numeric_limits<std::int32_t>::min());
    EXPECT_EQ(sign_extend(0x7FFFFFFFu, 32), std::numeric_limits<std::int32_t>::max());
}

TEST(SignExtend, IgnoresBitsAboveWidth) {
    EXPECT_EQ(sign_extend(0x1FFF, 12), -1);   // 0x1FFF & 0xFFF = 0xFFF
    EXPECT_EQ(sign_extend(0xABC123, 8), 0x23);
}

TEST(LessMixed, NegativeIsLessThanAnyUnsigned) {
    EXPECT_TRUE(less_mixed(-1, 1u));
    EXPECT_TRUE(less_mixed(-1, 0u));
    EXPECT_TRUE(less_mixed(3, 5u));
    EXPECT_FALSE(less_mixed(5, 3u));
    EXPECT_FALSE(less_mixed(0, 0u));
    EXPECT_FALSE(less_mixed(7, 7u));
    EXPECT_TRUE(less_mixed(std::numeric_limits<int>::max(), 0x80000000u));
}

TEST(NarrowU8, RangeChecked) {
    std::uint8_t out = 0x55;
    EXPECT_TRUE(narrow_u8(255, out));
    EXPECT_EQ(out, 255);
    out = 0x55;
    EXPECT_TRUE(narrow_u8(0, out));
    EXPECT_EQ(out, 0);
    out = 0x55;
    EXPECT_FALSE(narrow_u8(256, out));
    EXPECT_EQ(out, 0x55);
    EXPECT_FALSE(narrow_u8(-1, out));
    EXPECT_EQ(out, 0x55);
}
