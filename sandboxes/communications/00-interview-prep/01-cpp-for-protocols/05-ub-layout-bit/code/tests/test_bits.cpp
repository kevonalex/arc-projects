#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

#if __has_include(<version>)
#include <version>
#endif
#if defined(__cpp_lib_bitops) || defined(__cpp_lib_bit_cast) || defined(__cpp_lib_endian)
#include <bit>
#endif

#include <es_ubbit/bits.hpp>

using namespace es::ubbit;

namespace {
// Deterministic pseudo-random sample (xorshift32) - no <random> needed.
std::vector<std::uint32_t> samples() {
    std::vector<std::uint32_t> v = {0u, 1u, 2u, 0x7FFFFFFFu, 0x80000000u, 0x80000001u, 0xFFFFFFFFu, 0xF0F0F0F0u};
    std::uint32_t x = 2463534242u;
    for (int i = 0; i < 500; ++i) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        v.push_back(x);
    }
    return v;
}
}  // namespace

TEST(Popcount, KnownValues) {
    EXPECT_EQ(popcount32(0), 0u);
    EXPECT_EQ(popcount32(1), 1u);
    EXPECT_EQ(popcount32(0xFFFFFFFFu), 32u);
    EXPECT_EQ(popcount32(0x80000001u), 2u);
    EXPECT_EQ(popcount32(0xF0F0F0F0u), 16u);
}

TEST(Popcount, Parity) {
    EXPECT_EQ(parity32(0), 0u);
    EXPECT_EQ(parity32(1), 1u);
    EXPECT_EQ(parity32(3), 0u);
    EXPECT_EQ(parity32(7), 1u);
    EXPECT_EQ(parity32(0xFFFFFFFFu), 0u);
    EXPECT_EQ(parity32(0x80000000u), 1u);
}

TEST(Clz, KnownValues) {
    EXPECT_EQ(clz32(0), 32u);
    EXPECT_EQ(clz32(1), 31u);
    EXPECT_EQ(clz32(0x80000000u), 0u);
    EXPECT_EQ(clz32(0x00010000u), 15u);
    EXPECT_EQ(clz32(0xFFFFFFFFu), 0u);
}

TEST(Byteswap, KnownValues) {
    EXPECT_EQ(byteswap16(0x1234), 0x3412);
    EXPECT_EQ(byteswap32(0x11223344u), 0x44332211u);
    EXPECT_EQ(byteswap32(0x000000FFu), 0xFF000000u);
}

TEST(Byteswap, IsAnInvolution) {
    for (auto v : samples()) EXPECT_EQ(byteswap32(byteswap32(v)), v);
}

TEST(Endian, DetectedFromTheBytes) {
    const std::uint32_t one = 1;
    unsigned char first;
    std::memcpy(&first, &one, 1);
    EXPECT_EQ(is_little_endian(), first == 1);
}

TEST(BitFields, Extract) {
    EXPECT_EQ(extract_bits(0xABCD1234u, 8, 8), 0x12u);
    EXPECT_EQ(extract_bits(0xABCD1234u, 0, 4), 0x4u);
    EXPECT_EQ(extract_bits(0xABCD1234u, 28, 4), 0xAu);
    EXPECT_EQ(extract_bits(0xABCD1234u, 0, 32), 0xABCD1234u);  // width 32 must not do 1u << 32
    EXPECT_EQ(extract_bits(0xABCD1234u, 5, 0), 0u);
    EXPECT_EQ(extract_bits(0xFFFFFFFFu, 31, 1), 1u);
}

TEST(BitFields, Insert) {
    EXPECT_EQ(insert_bits(0u, 4, 4, 0xF), 0xF0u);
    EXPECT_EQ(insert_bits(0xFFFFFFFFu, 8, 8, 0u), 0xFFFF00FFu);
    EXPECT_EQ(insert_bits(0u, 0, 4, 0x1F), 0xFu);               // extra high bits of field ignored
    EXPECT_EQ(insert_bits(0x12345678u, 0, 32, 0xAAAAAAAAu), 0xAAAAAAAAu);
    EXPECT_EQ(insert_bits(0x12345678u, 12, 0, 0xFFu), 0x12345678u);
}

TEST(BitFields, InsertThenExtractRoundTrip) {
    for (unsigned lo = 0; lo < 32; lo += 3) {
        for (unsigned width = 1; lo + width <= 32; width += 5) {
            const std::uint32_t field = 0xA5A5A5A5u;
            const std::uint32_t v = insert_bits(0x5A5A5A5Au, lo, width, field);
            const std::uint32_t mask = (width == 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
            EXPECT_EQ(extract_bits(v, lo, width), field & mask) << "lo=" << lo << " width=" << width;
        }
    }
}

TEST(TypePunning, FloatBits) {
    EXPECT_EQ(float_bits(1.0f), 0x3F800000u);
    EXPECT_EQ(float_bits(-2.0f), 0xC0000000u);
    EXPECT_EQ(float_bits(0.0f), 0u);
    EXPECT_EQ(bits_float(0x3F800000u), 1.0f);
    EXPECT_FLOAT_EQ(bits_float(0x40490FDBu), 3.14159265f);
}

TEST(TypePunning, FloatRoundTripsExactly) {
    for (float f : {0.5f, -123.25f, 1e-30f, 3.0e38f}) EXPECT_EQ(bits_float(float_bits(f)), f);
}

TEST(Loads, UnalignedNativeEndianLoad) {
    const std::uint32_t x = 0x01020304u;
    unsigned char buf[9] = {};
    std::memcpy(buf + 1, &x, 4);  // deliberately misaligned for uint32_t
    EXPECT_EQ(load_u32_ne(buf + 1), x);
}

TEST(Loads, BigEndianLoadIsHostIndependent) {
    const unsigned char b[] = {0x12, 0x34, 0x56, 0x78};
    EXPECT_EQ(load_be32(b), 0x12345678u);
    const unsigned char c[] = {0xFF, 0x00, 0x00, 0x01};
    EXPECT_EQ(load_be32(c), 0xFF000001u);
}

// Cross-checks against the standard library (only when the library provides them).
#if defined(__cpp_lib_bitops)
TEST(StdCrossCheck, PopcountClzMatchStd) {
    for (auto v : samples()) {
        EXPECT_EQ(popcount32(v), static_cast<unsigned>(std::popcount(v))) << std::hex << v;
        EXPECT_EQ(parity32(v), static_cast<unsigned>(std::popcount(v) & 1)) << std::hex << v;
        EXPECT_EQ(clz32(v), static_cast<unsigned>(std::countl_zero(v))) << std::hex << v;
    }
}
#endif

#if defined(__cpp_lib_bit_cast)
TEST(StdCrossCheck, FloatBitsMatchBitCast) {
    for (float f : {0.0f, 1.0f, -1.5f, 6.02e23f, 1e-40f}) {
        EXPECT_EQ(float_bits(f), std::bit_cast<std::uint32_t>(f));
    }
}
#endif

#if defined(__cpp_lib_endian)
TEST(StdCrossCheck, EndianMatchesStd) {
    EXPECT_EQ(is_little_endian(), std::endian::native == std::endian::little);
}
#endif
