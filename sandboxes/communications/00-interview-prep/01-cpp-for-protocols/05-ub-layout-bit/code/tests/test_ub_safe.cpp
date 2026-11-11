#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <es_ubbit/ub_safe.hpp>

using namespace es::ubbit;

namespace {
constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();
constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();
}  // namespace

TEST(CheckedAdd, NormalCases) {
    EXPECT_EQ(checked_add(1, 2), std::optional<std::int32_t>(3));
    EXPECT_EQ(checked_add(-5, -6), std::optional<std::int32_t>(-11));
    EXPECT_EQ(checked_add(kMax, 0), std::optional<std::int32_t>(kMax));
    EXPECT_EQ(checked_add(kMax, kMin), std::optional<std::int32_t>(-1));
    EXPECT_EQ(checked_add(kMin, kMax), std::optional<std::int32_t>(-1));
}

TEST(CheckedAdd, DetectsOverflowBothDirections) {
    EXPECT_FALSE(checked_add(kMax, 1).has_value());
    EXPECT_FALSE(checked_add(1, kMax).has_value());
    EXPECT_FALSE(checked_add(kMax, kMax).has_value());
    EXPECT_FALSE(checked_add(kMin, -1).has_value());
    EXPECT_FALSE(checked_add(-1, kMin).has_value());
    EXPECT_FALSE(checked_add(kMin, kMin).has_value());
}

TEST(CheckedAdd, ExactBoundariesAreFine) {
    EXPECT_EQ(checked_add(kMax - 1, 1), std::optional<std::int32_t>(kMax));
    EXPECT_EQ(checked_add(kMin + 1, -1), std::optional<std::int32_t>(kMin));
}

TEST(SatAdd, Saturates) {
    EXPECT_EQ(sat_add(kMax, 1), kMax);
    EXPECT_EQ(sat_add(kMax, kMax), kMax);
    EXPECT_EQ(sat_add(kMin, -1), kMin);
    EXPECT_EQ(sat_add(kMin, kMin), kMin);
    EXPECT_EQ(sat_add(100, 23), 123);
    EXPECT_EQ(sat_add(kMax, -1), kMax - 1);
}

TEST(WrappingAdd, MatchesHardware) {
    EXPECT_EQ(wrapping_add(kMax, 1), kMin);
    EXPECT_EQ(wrapping_add(kMin, -1), kMax);
    EXPECT_EQ(wrapping_add(7, 8), 15);
    EXPECT_EQ(wrapping_add(kMax, kMax), -2);  // 0xFFFFFFFE
}

TEST(CheckedAbs, HandlesIntMin) {
    EXPECT_EQ(checked_abs(-5), std::optional<std::int32_t>(5));
    EXPECT_EQ(checked_abs(5), std::optional<std::int32_t>(5));
    EXPECT_EQ(checked_abs(0), std::optional<std::int32_t>(0));
    EXPECT_EQ(checked_abs(kMax), std::optional<std::int32_t>(kMax));
    EXPECT_EQ(checked_abs(kMin + 1), std::optional<std::int32_t>(kMax));
    EXPECT_FALSE(checked_abs(kMin).has_value());  // |-2^31| = 2^31 does not fit
}

TEST(Shifts, InRange) {
    EXPECT_EQ(shl(1u, 0), 1u);
    EXPECT_EQ(shl(1u, 31), 0x80000000u);
    EXPECT_EQ(shl(0xFFu, 4), 0xFF0u);
    EXPECT_EQ(shl(0xFFFFFFFFu, 31), 0x80000000u);
    EXPECT_EQ(shr(0x80000000u, 31), 1u);
    EXPECT_EQ(shr(0xFFu, 0), 0xFFu);
}

TEST(Shifts, CountAtOrAboveWidthGivesZero) {
    EXPECT_EQ(shl(1u, 32), 0u);
    EXPECT_EQ(shl(0xFFFFFFFFu, 33), 0u);
    EXPECT_EQ(shl(1u, 1000), 0u);
    EXPECT_EQ(shr(0x80000000u, 32), 0u);
    EXPECT_EQ(shr(0xFFFFFFFFu, 64), 0u);
}

TEST(Rotl, KnownValues) {
    EXPECT_EQ(rotl32_manual(0x80000001u, 1), 0x00000003u);
    EXPECT_EQ(rotl32_manual(0x12345678u, 8), 0x34567812u);
    EXPECT_EQ(rotl32_manual(0x12345678u, 4), 0x23456781u);
    EXPECT_EQ(rotl32_manual(0x00000001u, 31), 0x80000000u);
}

TEST(Rotl, ZeroAndFullTurnAreIdentity) {
    EXPECT_EQ(rotl32_manual(0xDEADBEEFu, 0), 0xDEADBEEFu);   // (v << 0) | (v >> 32) would be UB
    EXPECT_EQ(rotl32_manual(0xDEADBEEFu, 32), 0xDEADBEEFu);
    EXPECT_EQ(rotl32_manual(0xDEADBEEFu, 64), 0xDEADBEEFu);
}

TEST(Rotl, CountIsModulo32) {
    for (unsigned n = 0; n < 32; ++n) {
        EXPECT_EQ(rotl32_manual(0x0F1E2D3Cu, n + 32), rotl32_manual(0x0F1E2D3Cu, n)) << "n=" << n;
        // rotating left by n then by (32-n) mod 32 restores the value
        EXPECT_EQ(rotl32_manual(rotl32_manual(0x0F1E2D3Cu, n), (32 - n) % 32), 0x0F1E2D3Cu) << "n=" << n;
    }
}
