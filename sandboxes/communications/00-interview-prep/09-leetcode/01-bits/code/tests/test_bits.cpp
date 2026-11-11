#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <vector>

#include "bits.hpp"

using namespace lc;

// ---- 191 -------------------------------------------------------------------
TEST(Bits191, Normal) {
    EXPECT_EQ(hamming_weight(0b1011u), 3);
    EXPECT_EQ(hamming_weight(0x80000001u), 2);
}
TEST(Bits191, Edges) {
    EXPECT_EQ(hamming_weight(0u), 0);
    EXPECT_EQ(hamming_weight(0xFFFFFFFFu), 32);
    EXPECT_EQ(hamming_weight(0x80000000u), 1);  // top bit: no signed shift trouble
}

// ---- 190 -------------------------------------------------------------------
TEST(Bits190, Normal) {
    EXPECT_EQ(reverse_bits(0b00000010100101000001111010011100u), 0b00111001011110000010100101000000u);
    EXPECT_EQ(reverse_bits(1u), 0x80000000u);
}
TEST(Bits190, EdgesAndInvolution) {
    EXPECT_EQ(reverse_bits(0u), 0u);
    EXPECT_EQ(reverse_bits(0xFFFFFFFFu), 0xFFFFFFFFu);
    for (std::uint32_t x : {0x12345678u, 0xDEADBEEFu, 0x80000001u}) {
        EXPECT_EQ(reverse_bits(reverse_bits(x)), x);
    }
}

// ---- 338 -------------------------------------------------------------------
TEST(Bits338, Normal) {
    EXPECT_EQ(count_bits(5), (std::vector<int>{0, 1, 1, 2, 1, 2}));
}
TEST(Bits338, EdgesAndAgreesWith191) {
    EXPECT_EQ(count_bits(0), (std::vector<int>{0}));
    const auto v = count_bits(1000);
    for (int i = 0; i <= 1000; ++i) {
        ASSERT_EQ(v[static_cast<std::size_t>(i)], hamming_weight(static_cast<std::uint32_t>(i))) << i;
    }
}

// ---- 136 -------------------------------------------------------------------
TEST(Bits136, Normal) {
    EXPECT_EQ(single_number({2, 2, 1}), 1);
    EXPECT_EQ(single_number({4, 1, 2, 1, 2}), 4);
}
TEST(Bits136, Edges) {
    EXPECT_EQ(single_number({7}), 7);
    EXPECT_EQ(single_number({-1, 5, -1}), 5);
    EXPECT_EQ(single_number({0, 9, 0}), 9);
}

// ---- 137 -------------------------------------------------------------------
TEST(Bits137, Normal) {
    EXPECT_EQ(single_number_ii({2, 2, 3, 2}), 3);
    EXPECT_EQ(single_number_ii({0, 1, 0, 1, 0, 1, 99}), 99);
}
TEST(Bits137, Edges) {
    EXPECT_EQ(single_number_ii({-2, -2, 1, 1, -3, 1, -3, -3, -4, -2}), -4);
    EXPECT_EQ(single_number_ii({5}), 5);
    EXPECT_EQ(single_number_ii({INT_MIN, 7, 7, 7}), INT_MIN);
}

// ---- 268 -------------------------------------------------------------------
TEST(Bits268, Normal) {
    EXPECT_EQ(missing_number({3, 0, 1}), 2);
    EXPECT_EQ(missing_number({9, 6, 4, 2, 3, 5, 7, 0, 1}), 8);
}
TEST(Bits268, Edges) {
    EXPECT_EQ(missing_number({0}), 1);  // missing is n itself
    EXPECT_EQ(missing_number({1}), 0);  // missing is 0
    EXPECT_EQ(missing_number({0, 1}), 2);
}

// ---- 461 / 477 ------------------------------------------------------------
TEST(Bits461, Basic) {
    EXPECT_EQ(hamming_distance(1, 4), 2);
    EXPECT_EQ(hamming_distance(3, 1), 1);
    EXPECT_EQ(hamming_distance(0, 0), 0);
    EXPECT_EQ(hamming_distance(0, INT_MAX), 31);
}
TEST(Bits477, Basic) {
    EXPECT_EQ(total_hamming_distance({4, 14, 2}), 6);
    EXPECT_EQ(total_hamming_distance({4, 14, 4}), 4);
    EXPECT_EQ(total_hamming_distance({5}), 0);
}
TEST(Bits477, MatchesBruteForce) {
    const std::vector<int> v{1, 2, 3, 7, 8, 255, 1024, 77};
    int brute = 0;
    for (std::size_t i = 0; i < v.size(); ++i)
        for (std::size_t j = i + 1; j < v.size(); ++j) brute += hamming_distance(v[i], v[j]);
    EXPECT_EQ(total_hamming_distance(v), brute);
}

// ---- 371 -------------------------------------------------------------------
TEST(Bits371, Normal) {
    EXPECT_EQ(get_sum(1, 2), 3);
    EXPECT_EQ(get_sum(2, 3), 5);
    EXPECT_EQ(get_sum(13, 7), 20);
}
TEST(Bits371, NegativesAndEdges) {
    EXPECT_EQ(get_sum(-1, 1), 0);
    EXPECT_EQ(get_sum(-5, -7), -12);
    EXPECT_EQ(get_sum(0, 0), 0);
    EXPECT_EQ(get_sum(1000, -1), 999);
    EXPECT_EQ(get_sum(INT_MIN, INT_MAX), -1);
}

// ---- 201 -------------------------------------------------------------------
TEST(Bits201, Basic) {
    EXPECT_EQ(range_bitwise_and(5, 7), 4);
    EXPECT_EQ(range_bitwise_and(0, 0), 0);
    EXPECT_EQ(range_bitwise_and(1, INT_MAX), 0);
    EXPECT_EQ(range_bitwise_and(12, 12), 12);
    EXPECT_EQ(range_bitwise_and(26, 30), 24);
}

// ---- 89 --------------------------------------------------------------------
TEST(Bits89, Normal) {
    EXPECT_EQ(gray_code(2), (std::vector<int>{0, 1, 3, 2}));
    EXPECT_EQ(gray_code(1), (std::vector<int>{0, 1}));
}
TEST(Bits89, PropertiesForN5) {
    const auto g = gray_code(5);
    ASSERT_EQ(g.size(), 32u);
    auto sorted = g;
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < 32; ++i) EXPECT_EQ(sorted[static_cast<std::size_t>(i)], i);  // permutation
    for (std::size_t i = 0; i < g.size(); ++i) {
        const int d = g[i] ^ g[(i + 1) % g.size()];  // cyclic: last vs first too
        EXPECT_EQ(hamming_weight(static_cast<std::uint32_t>(d)), 1) << i;
    }
}

// ---- 393 -------------------------------------------------------------------
TEST(Bits393, Normal) {
    EXPECT_TRUE(valid_utf8({197, 130, 1}));   // 2-byte char then ASCII
    EXPECT_FALSE(valid_utf8({235, 140, 4}));  // 3-byte header, third byte is not 10xxxxxx
}
TEST(Bits393, Edges) {
    EXPECT_TRUE(valid_utf8({0}));
    EXPECT_TRUE(valid_utf8({240, 162, 138, 147}));  // 4-byte
    EXPECT_FALSE(valid_utf8({145}));                // orphan continuation byte
    EXPECT_FALSE(valid_utf8({197}));                // truncated
    EXPECT_FALSE(valid_utf8({248, 130, 130, 130, 130}));  // 11111xxx never valid
    EXPECT_TRUE(valid_utf8({256 + 65}));            // only low 8 bits matter
}
