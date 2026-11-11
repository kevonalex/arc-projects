#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include <es_types/bytes_enums.hpp>

using namespace es::types;

// enum class is NOT implicitly convertible (provided facts; they pass with stubs).
static_assert(!std::is_convertible_v<Protocol, int>, "enum class does not convert to int");
static_assert(std::is_same_v<std::underlying_type_t<Protocol>, std::uint8_t>);
static_assert(sizeof(Protocol) == 1, "fixed underlying type -> predictable size");

TEST(StdByte, HighNibble) {
    EXPECT_EQ(std::to_integer<int>(high_nibble(std::byte{0xAB})), 0x0A);
    EXPECT_EQ(std::to_integer<int>(high_nibble(std::byte{0x0F})), 0x00);
    EXPECT_EQ(std::to_integer<int>(high_nibble(std::byte{0xF0})), 0x0F);
}

TEST(StdByte, NibbleSwap) {
    EXPECT_EQ(std::to_integer<int>(nibble_swap(std::byte{0xAB})), 0xBA);
    EXPECT_EQ(std::to_integer<int>(nibble_swap(std::byte{0x01})), 0x10);
    EXPECT_EQ(std::to_integer<int>(nibble_swap(std::byte{0xFF})), 0xFF);
}

TEST(StdByte, ToHex) {
    const std::byte b[] = {std::byte{0x00}, std::byte{0xAB}, std::byte{0x7E}};
    EXPECT_EQ(to_hex(b, 3), "00AB7E");
    EXPECT_EQ(to_hex(b, 0), "");
    const std::byte c[] = {std::byte{0x0F}};
    EXPECT_EQ(to_hex(c, 1), "0F");  // leading zero kept
}

TEST(EnumClass, Names) {
    EXPECT_STREQ(name(Protocol::P25), "P25");
    EXPECT_STREQ(name(Protocol::DMR), "DMR");
    EXPECT_STREQ(name(Protocol::TETRA), "TETRA");
    EXPECT_STREQ(name(static_cast<Protocol>(77)), "UNKNOWN");
}

TEST(EnumClass, ValidateBeforeConverting) {
    Protocol p = Protocol::P25;
    EXPECT_TRUE(protocol_from_u8(2, p));
    EXPECT_EQ(p, Protocol::TETRA);
    EXPECT_TRUE(protocol_from_u8(1, p));
    EXPECT_EQ(p, Protocol::DMR);
    p = Protocol::DMR;
    EXPECT_FALSE(protocol_from_u8(3, p));
    EXPECT_EQ(p, Protocol::DMR);  // untouched on failure
    EXPECT_FALSE(protocol_from_u8(255, p));
}

TEST(EnumClass, ToUnderlying) {
    EXPECT_EQ(to_underlying(Protocol::TETRA), 2);
    EXPECT_EQ(to_underlying(Flags::Data), 4);
}

TEST(FlagsEnum, OrCombines) {
    const Flags f = Flags::Encrypted | Flags::Data;
    EXPECT_EQ(to_underlying(f), 5);
    EXPECT_EQ(to_underlying(Flags::None | Flags::None), 0);
}

TEST(FlagsEnum, AndMasks) {
    const Flags f = Flags::Encrypted | Flags::Data;
    EXPECT_EQ(f & Flags::Encrypted, Flags::Encrypted);
    EXPECT_EQ(f & Flags::Emergency, Flags::None);
}

TEST(FlagsEnum, HasRequiresAllBits) {
    const Flags f = Flags::Encrypted | Flags::Data;
    EXPECT_TRUE(has(f, Flags::Data));
    EXPECT_TRUE(has(f, Flags::Encrypted | Flags::Data));
    EXPECT_FALSE(has(f, Flags::Emergency));
    EXPECT_FALSE(has(f, Flags::Emergency | Flags::Data));  // one of two is not enough
    EXPECT_TRUE(has(f, Flags::None));                      // vacuous: no bits required
}
