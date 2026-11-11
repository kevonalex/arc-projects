#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <es_containers/text.hpp>

using namespace es::containers;

TEST(Split, Basic) {
    const std::string s = "a,b,c";
    auto parts = split(s, ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(Split, ViewsPointIntoTheOriginal) {
    const std::string s = "ab,cd";
    auto parts = split(s, ',');
    ASSERT_EQ(parts.size(), 2u);
    EXPECT_EQ(parts[0].data(), s.data());
    EXPECT_EQ(parts[1].data(), s.data() + 3) << "a view, not a copy";
}

TEST(Split, KeepsEmptyFields) {
    EXPECT_EQ(split("a,,b", ',').size(), 3u);
    EXPECT_EQ(split("a,,b", ',')[1], "");
    EXPECT_EQ(split("", ',').size(), 1u);
    EXPECT_EQ(split(",", ',').size(), 2u);
    auto t = split("a,", ',');
    ASSERT_EQ(t.size(), 2u);
    EXPECT_EQ(t[0], "a");
    EXPECT_EQ(t[1], "");
}

TEST(ParseU16, Valid) {
    EXPECT_EQ(parse_u16("0"), std::optional<std::uint16_t>(0));
    EXPECT_EQ(parse_u16("42"), std::optional<std::uint16_t>(42));
    EXPECT_EQ(parse_u16("65535"), std::optional<std::uint16_t>(65535));
    EXPECT_EQ(parse_u16("00042"), std::optional<std::uint16_t>(42));
    EXPECT_EQ(parse_u16("0000000000000065535"), std::optional<std::uint16_t>(65535));
}

TEST(ParseU16, Invalid) {
    EXPECT_FALSE(parse_u16("").has_value());
    EXPECT_FALSE(parse_u16("65536").has_value());
    EXPECT_FALSE(parse_u16("99999999999999999999").has_value());  // must not overflow while accumulating
    EXPECT_FALSE(parse_u16("12a").has_value());
    EXPECT_FALSE(parse_u16("-1").has_value());
    EXPECT_FALSE(parse_u16("+1").has_value());
    EXPECT_FALSE(parse_u16(" 1").has_value());
    EXPECT_FALSE(parse_u16("1 ").has_value());
    EXPECT_FALSE(parse_u16("0x10").has_value());
}

TEST(ParseField, Empty) {
    EXPECT_TRUE(std::holds_alternative<std::monostate>(parse_field("")));
}

TEST(ParseField, Integers) {
    auto a = parse_field("42");
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(a));
    EXPECT_EQ(std::get<std::int64_t>(a), 42);
    auto b = parse_field("-7");
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(b));
    EXPECT_EQ(std::get<std::int64_t>(b), -7);
    auto c = parse_field("123456789012345678");  // 18 digits
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(c));
    EXPECT_EQ(std::get<std::int64_t>(c), 123456789012345678LL);
}

TEST(ParseField, Text) {
    for (std::string_view txt : {"abc", "12a", "-", "9999999999999999999" /* 19 digits */, "4 2"}) {
        auto f = parse_field(txt);
        ASSERT_TRUE(std::holds_alternative<std::string_view>(f)) << txt;
        EXPECT_EQ(std::get<std::string_view>(f), txt);
    }
}

TEST(Describe, AllAlternatives) {
    EXPECT_EQ(describe(Field{std::monostate{}}), "empty");
    EXPECT_EQ(describe(Field{std::int64_t{42}}), "int:42");
    EXPECT_EQ(describe(Field{std::int64_t{-7}}), "int:-7");
    EXPECT_EQ(describe(Field{std::string_view{"abc"}}), "str:abc");
}
