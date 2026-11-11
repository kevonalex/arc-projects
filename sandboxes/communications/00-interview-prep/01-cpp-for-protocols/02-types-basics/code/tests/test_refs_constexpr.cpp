#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <es_types/refs_constexpr.hpp>

using namespace es::types;

TEST(Refs, SwapU8) {
    std::uint8_t a = 1, b = 2;
    swap_u8(a, b);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);
}

TEST(Refs, SwapSelfIsHarmless) {
    std::uint8_t a = 9;
    swap_u8(a, a);
    EXPECT_EQ(a, 9);
}

TEST(Pointers, FindByteFound) {
    const std::uint8_t d[] = {0x10, 0x7E, 0x20, 0x7E};
    const std::uint8_t* r = find_byte(d, d + 4, 0x7E);
    EXPECT_EQ(r, d + 1);  // FIRST match
}

TEST(Pointers, FindByteNotFoundReturnsEnd) {
    const std::uint8_t d[] = {1, 2, 3};
    EXPECT_EQ(find_byte(d, d + 3, 9), d + 3);
}

TEST(Pointers, FindByteEmptyRange) {
    const std::uint8_t d[] = {1};
    EXPECT_EQ(find_byte(d, d, 1), d);  // empty range: end == begin
}

TEST(Pointers, CountNonzero) {
    const std::uint8_t d[] = {0, 1, 0, 255, 7};
    EXPECT_EQ(count_nonzero(d, 5), 3u);
    EXPECT_EQ(count_nonzero(nullptr, 0), 0u);
}

TEST(Refs, ClampInPlace) {
    int v = 15;
    clamp_inplace(v, 0, 10);
    EXPECT_EQ(v, 10);
    v = -3;
    clamp_inplace(v, 0, 10);
    EXPECT_EQ(v, 0);
    v = 5;
    clamp_inplace(v, 0, 10);
    EXPECT_EQ(v, 5);
}

TEST(Pointers, FillPatternWrapsAt256) {
    std::vector<std::uint8_t> v(4, 0xEE);  // exact size: ASan guards the end
    fill_pattern(v.data(), v.size(), 254);
    EXPECT_EQ(v, (std::vector<std::uint8_t>{254, 255, 0, 1}));
}

TEST(Constexpr, MaskValues) {
    EXPECT_EQ(mask(0), 0u);
    EXPECT_EQ(mask(1), 1u);
    EXPECT_EQ(mask(8), 0xFFu);
    EXPECT_EQ(mask(12), 0xFFFu);
    EXPECT_EQ(mask(31), 0x7FFFFFFFu);
    EXPECT_EQ(mask(32), 0xFFFFFFFFu);  // UBSan fails you here if you wrote (1u << width) - 1
}

TEST(Constexpr, BytesForBits) {
    EXPECT_EQ(bytes_for_bits(0), 0u);
    EXPECT_EQ(bytes_for_bits(1), 1u);
    EXPECT_EQ(bytes_for_bits(8), 1u);
    EXPECT_EQ(bytes_for_bits(9), 2u);
    EXPECT_EQ(bytes_for_bits(24), 3u);
    EXPECT_EQ(bytes_for_bits(25), 4u);
}

TEST(Constexpr, ReverseBitsAtRuntime) {
    EXPECT_EQ(reverse_bits8(0x01), 0x80);
    EXPECT_EQ(reverse_bits8(0x80), 0x01);
    EXPECT_EQ(reverse_bits8(0x2C), 0x34);
    EXPECT_EQ(reverse_bits8(0xFF), 0xFF);
    EXPECT_EQ(reverse_bits8(0x00), 0x00);
}

// This compiles ONLY if the functions are genuinely constexpr; the VALUE is checked at run time
// so that stubs still compile.
TEST(Constexpr, UsableInConstantExpressions) {
    using R = std::integral_constant<std::uint8_t, reverse_bits8(0x2C)>;
    using M = std::integral_constant<std::uint32_t, mask(12)>;
    using B = std::integral_constant<std::size_t, bytes_for_bits(17)>;
    EXPECT_EQ(R::value, 0x34);
    EXPECT_EQ(M::value, 0xFFFu);
    EXPECT_EQ(B::value, 3u);
    std::uint8_t arr[bytes_for_bits(17)] = {};  // array bound must be a constant expression
    EXPECT_EQ(sizeof(arr), 3u);
}

TEST(Constexpr, CompileTimeTable) {
    EXPECT_EQ(kReverseTable[0x01], 0x80);
    EXPECT_EQ(kReverseTable[0x2C], 0x34);
    EXPECT_EQ(kReverseTable[0xFF], 0xFF);
    for (unsigned i = 0; i < 256; ++i) {
        EXPECT_EQ(kReverseTable[kReverseTable[i]], i) << "involution at " << i;  // reverse twice = identity
    }
}
