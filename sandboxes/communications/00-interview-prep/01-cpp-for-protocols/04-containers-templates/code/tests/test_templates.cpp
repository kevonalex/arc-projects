#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include <es_containers/templates.hpp>

using namespace es::containers;

TEST(ClampVal, IntsAndDoubles) {
    EXPECT_EQ(clamp_val(5, 0, 3), 3);
    EXPECT_EQ(clamp_val(-5, 0, 3), 0);
    EXPECT_EQ(clamp_val(2, 0, 3), 2);
    EXPECT_DOUBLE_EQ(clamp_val(1.5, 0.0, 1.0), 1.0);
    EXPECT_EQ(clamp_val(std::string("m"), std::string("a"), std::string("f")), "f");
}

TEST(ClampVal, IsConstexprUsable) {
    using C = std::integral_constant<int, clamp_val(5, 0, 3)>;  // compile error unless constexpr-evaluable
    EXPECT_EQ(C::value, 3);
}

TEST(ArraySize, DeducedFromTheType) {
    int a[7] = {};
    const char b[] = "abcd";  // 5 with the terminator
    EXPECT_EQ(array_size(a), 7u);
    EXPECT_EQ(array_size(b), 5u);
    using S = std::integral_constant<std::size_t, array_size(a)>;  // usable at compile time
    EXPECT_EQ(S::value, 7u);
    std::uint8_t same_size[array_size(a)] = {};
    EXPECT_EQ(sizeof(same_size), 7u);
}

TEST(CeilDiv, Basic) {
    EXPECT_EQ(ceil_div(7u, 2u), 4u);
    EXPECT_EQ(ceil_div(8u, 2u), 4u);
    EXPECT_EQ(ceil_div(0u, 5u), 0u);
    EXPECT_EQ(ceil_div(1u, 1u), 1u);
    EXPECT_EQ(ceil_div(std::size_t{37}, std::size_t{8}), 5u);  // 37 bits -> 5 bytes
}

TEST(CeilDiv, NearMaxDoesNotOverflow) {
    const auto m = std::numeric_limits<std::uint32_t>::max();
    EXPECT_EQ(ceil_div<std::uint32_t>(m, 2u), 0x80000000u);
    EXPECT_EQ(ceil_div<std::uint32_t>(m, m), 1u);
}

TEST(SumAll, FoldExpression) {
    EXPECT_EQ(sum_all(), 0);
    EXPECT_EQ(sum_all(1, 2, 3), 6);
    EXPECT_EQ(sum_all(std::uint8_t{200}, std::uint8_t{100}), 300);  // accumulated wide: no uint8_t wrap
    EXPECT_EQ(sum_all(1, 2.0, 3u, std::int8_t{-4}), 2);              // heterogeneous arguments
}

TEST(FindIfSimple, OnVectorAndArray) {
    std::vector<int> v = {1, 3, 4, 6};
    auto it = find_if_simple(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    ASSERT_NE(it, v.end());
    EXPECT_EQ(*it, 4);
    auto none = find_if_simple(v.begin(), v.end(), [](int x) { return x > 100; });
    EXPECT_EQ(none, v.end());

    const char text[] = "ab;cd";
    const char* p = find_if_simple(text, text + 5, [](char c) { return c == ';'; });
    EXPECT_EQ(p, text + 2);
}

TEST(StaticStack, PushPopLifo) {
    StaticStack<int, 3> s;
    EXPECT_TRUE(s.empty());
    EXPECT_TRUE(s.push(1));
    EXPECT_TRUE(s.push(2));
    EXPECT_TRUE(s.push(3));
    EXPECT_TRUE(s.full());
    EXPECT_FALSE(s.push(4));  // full: rejected, nothing changes
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.pop(), std::optional<int>(3));
    EXPECT_EQ(s.pop(), std::optional<int>(2));
    EXPECT_EQ(s.pop(), std::optional<int>(1));
    EXPECT_EQ(s.pop(), std::nullopt);
    EXPECT_TRUE(s.empty());
}

TEST(StaticStack, WorksWithNonTrivialTypes) {
    StaticStack<std::string, 2> s;
    EXPECT_TRUE(s.push("alpha"));
    EXPECT_TRUE(s.push("beta"));
    EXPECT_EQ(s.pop().value(), "beta");
    EXPECT_TRUE(s.push("gamma"));
    EXPECT_EQ(s.pop().value(), "gamma");
    EXPECT_EQ(s.pop().value(), "alpha");
}

TEST(StaticStack, CapacityIsCompileTime) {
    using S = StaticStack<char, 5>;
    using C = std::integral_constant<std::size_t, S::capacity()>;
    EXPECT_EQ(C::value, 5u);
}
