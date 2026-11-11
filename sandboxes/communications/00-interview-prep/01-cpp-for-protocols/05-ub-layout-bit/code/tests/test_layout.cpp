#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <es_ubbit/layout.hpp>

using namespace es::ubbit;

// Real structs whose layout the compiler decided. Your arithmetic must reproduce it.
struct A { std::uint8_t a; std::uint32_t b; std::uint8_t c; };   // classic padding trap
struct B { std::uint32_t b; std::uint8_t a; std::uint8_t c; };   // same fields, reordered
struct C { std::uint8_t a; std::uint16_t b; std::uint8_t c; };
struct D { std::uint64_t a; std::uint8_t b; };                   // tail padding
struct E { std::uint8_t a; std::uint64_t b; std::uint16_t c; std::uint8_t d; };

TEST(AlignmentMath, IsPow2) {
    EXPECT_TRUE(is_pow2(1));
    EXPECT_TRUE(is_pow2(2));
    EXPECT_TRUE(is_pow2(64));
    EXPECT_FALSE(is_pow2(0));
    EXPECT_FALSE(is_pow2(3));
    EXPECT_FALSE(is_pow2(12));
    EXPECT_FALSE(is_pow2(255));
}

TEST(AlignmentMath, AlignUp) {
    EXPECT_EQ(align_up(0, 8), 0u);
    EXPECT_EQ(align_up(1, 8), 8u);
    EXPECT_EQ(align_up(7, 8), 8u);
    EXPECT_EQ(align_up(8, 8), 8u);
    EXPECT_EQ(align_up(9, 8), 16u);
    EXPECT_EQ(align_up(13, 4), 16u);
    EXPECT_EQ(align_up(5, 1), 5u);
    EXPECT_EQ(align_up(1000, 64), 1024u);
}

TEST(AlignmentMath, IsAligned) {
    alignas(16) std::uint8_t buf[32] = {};
    EXPECT_TRUE(is_aligned(buf, 16));
    EXPECT_TRUE(is_aligned(buf, 1));
    EXPECT_TRUE(is_aligned(buf + 4, 4));
    EXPECT_FALSE(is_aligned(buf + 1, 2));
    EXPECT_FALSE(is_aligned(buf + 4, 8));
}

TEST(LayoutByHand, MatchesCompilerForStructA) {
    const FieldSpec f[] = {{1, 1}, {4, 4}, {1, 1}};
    EXPECT_EQ(field_offset(f, 0), offsetof(A, a));
    EXPECT_EQ(field_offset(f, 1), offsetof(A, b));
    EXPECT_EQ(field_offset(f, 2), offsetof(A, c));
    EXPECT_EQ(struct_size(f, 3), sizeof(A));
}

TEST(LayoutByHand, MatchesCompilerForStructB) {
    const FieldSpec f[] = {{4, 4}, {1, 1}, {1, 1}};
    EXPECT_EQ(field_offset(f, 1), offsetof(B, a));
    EXPECT_EQ(field_offset(f, 2), offsetof(B, c));
    EXPECT_EQ(struct_size(f, 3), sizeof(B));
}

TEST(LayoutByHand, MatchesCompilerForStructC) {
    const FieldSpec f[] = {{1, 1}, {2, 2}, {1, 1}};
    EXPECT_EQ(field_offset(f, 1), offsetof(C, b));
    EXPECT_EQ(struct_size(f, 3), sizeof(C));
}

TEST(LayoutByHand, MatchesCompilerForStructD) {
    const FieldSpec f[] = {{8, alignof(std::uint64_t)}, {1, 1}};
    EXPECT_EQ(field_offset(f, 1), offsetof(D, b));
    EXPECT_EQ(struct_size(f, 2), sizeof(D));  // tail padding so arrays of D stay aligned
}

TEST(LayoutByHand, MatchesCompilerForStructE) {
    const FieldSpec f[] = {{1, 1}, {8, alignof(std::uint64_t)}, {2, 2}, {1, 1}};
    EXPECT_EQ(field_offset(f, 1), offsetof(E, b));
    EXPECT_EQ(field_offset(f, 2), offsetof(E, c));
    EXPECT_EQ(field_offset(f, 3), offsetof(E, d));
    EXPECT_EQ(struct_size(f, 4), sizeof(E));
}

TEST(LayoutByHand, EmptyIsZeroAndSingleField) {
    EXPECT_EQ(struct_size(nullptr, 0), 0u);
    const FieldSpec one[] = {{4, 4}};
    EXPECT_EQ(struct_size(one, 1), 4u);
}

TEST(LayoutByHand, ReorderingLargestFirstNeverIncreasesSize) {
    const FieldSpec bad[] = {{1, 1}, {4, 4}, {1, 1}};
    const FieldSpec good[] = {{4, 4}, {1, 1}, {1, 1}};
    EXPECT_LE(struct_size(good, 3), struct_size(bad, 3));
    EXPECT_LT(struct_size(good, 3), struct_size(bad, 3));  // here strictly smaller (8 < 12 on mainstream ABIs)
}

TEST(HeaderWire, WritesBigEndianBytes) {
    const Header h{0xA5, 0x1234, 0xDEADBEEF};
    std::vector<std::uint8_t> out(kHeaderWireSize);  // exact size: ASan guards the edge
    ASSERT_EQ(write_header(h, out.data(), out.size()), kHeaderWireSize);
    EXPECT_EQ(out, (std::vector<std::uint8_t>{0xA5, 0x12, 0x34, 0xDE, 0xAD, 0xBE, 0xEF}));
}

TEST(HeaderWire, RejectsSmallBufferWritingNothing) {
    const Header h{1, 2, 3};
    std::vector<std::uint8_t> out(6, 0xCC);
    EXPECT_EQ(write_header(h, out.data(), out.size()), 0u);
    for (auto b : out) EXPECT_EQ(b, 0xCC);
}

TEST(HeaderWire, RoundTrip) {
    const Header h{0x7F, 0xFFFF, 0x80000001u};
    std::uint8_t buf[kHeaderWireSize];
    ASSERT_EQ(write_header(h, buf, sizeof buf), kHeaderWireSize);
    Header back{0, 0, 0};
    ASSERT_TRUE(read_header(buf, sizeof buf, back));
    EXPECT_EQ(back.type, 0x7F);
    EXPECT_EQ(back.length, 0xFFFF);
    EXPECT_EQ(back.id, 0x80000001u);
}

TEST(HeaderWire, ShortInputRejectedAndOutputUntouched) {
    const std::uint8_t buf[6] = {1, 2, 3, 4, 5, 6};
    Header h{9, 9, 9};
    EXPECT_FALSE(read_header(buf, 6, h));
    EXPECT_EQ(h.type, 9);
    EXPECT_EQ(h.length, 9);
    EXPECT_EQ(h.id, 9u);
}

TEST(HeaderWire, InMemoryStructIsNotTheWireFormat) {
    // Why we serialise field by field: the struct has padding (and host endianness).
    EXPECT_GE(sizeof(Header), kHeaderWireSize);
}
