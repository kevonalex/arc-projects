#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <es_toolchain/checksum.hpp>

using es::toolchain::xor_checksum;

TEST(XorChecksum, EmptyInputIsZero) {
    EXPECT_EQ(xor_checksum(nullptr, 0), 0);
}

TEST(XorChecksum, SingleByteIsItself) {
    const std::uint8_t b[] = {0xA5};
    EXPECT_EQ(xor_checksum(b, 1), 0xA5);
}

TEST(XorChecksum, HandComputedVector) {
    // 0x12 ^ 0x34 = 0x26 ; 0x26 ^ 0x56 = 0x70   (worksheet Q6)
    const std::uint8_t b[] = {0x12, 0x34, 0x56};
    EXPECT_EQ(xor_checksum(b, 3), 0x70);
}

TEST(XorChecksum, OrderDoesNotMatterWhichIsTheWeakness) {
    const std::uint8_t a[] = {0x01, 0x02};
    const std::uint8_t b[] = {0x02, 0x01};
    EXPECT_EQ(xor_checksum(a, 2), xor_checksum(b, 2));  // swapped bytes undetected
}

TEST(XorChecksum, AppendingChecksumGivesZero) {
    std::vector<std::uint8_t> v = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
    v.push_back(xor_checksum(v.data(), v.size()));
    EXPECT_EQ(xor_checksum(v.data(), v.size()), 0);
}
