#include <gtest/gtest.h>

#include <climits>
#include <string>
#include <vector>

#include "parsing.hpp"

using namespace lc;

// ---- 8 ---------------------------------------------------------------------
TEST(Parsing8, Normal) {
    EXPECT_EQ(my_atoi("42"), 42);
    EXPECT_EQ(my_atoi("   -42"), -42);
    EXPECT_EQ(my_atoi("4193 with words"), 4193);
    EXPECT_EQ(my_atoi("+7"), 7);
}
TEST(Parsing8, Edges) {
    EXPECT_EQ(my_atoi(""), 0);
    EXPECT_EQ(my_atoi("   "), 0);
    EXPECT_EQ(my_atoi("words and 987"), 0);
    EXPECT_EQ(my_atoi("+-12"), 0);
    EXPECT_EQ(my_atoi("-"), 0);
    EXPECT_EQ(my_atoi("0032"), 32);
    EXPECT_EQ(my_atoi("-91283472332"), INT_MIN);
    EXPECT_EQ(my_atoi("91283472332"), INT_MAX);
    EXPECT_EQ(my_atoi("2147483647"), INT_MAX);
    EXPECT_EQ(my_atoi("2147483648"), INT_MAX);
    EXPECT_EQ(my_atoi("-2147483648"), INT_MIN);
    EXPECT_EQ(my_atoi("-2147483649"), INT_MIN);
}

// ---- 468 -------------------------------------------------------------------
TEST(Parsing468, Normal) {
    EXPECT_EQ(valid_ip_address("172.16.254.1"), "IPv4");
    EXPECT_EQ(valid_ip_address("2001:0db8:85a3:0000:0000:8a2e:0370:7334"), "IPv6");
    EXPECT_EQ(valid_ip_address("256.256.256.256"), "Neither");
}
TEST(Parsing468, V4Edges) {
    EXPECT_EQ(valid_ip_address("0.0.0.0"), "IPv4");
    EXPECT_EQ(valid_ip_address("255.255.255.255"), "IPv4");
    EXPECT_EQ(valid_ip_address("192.168.01.1"), "Neither");  // leading zero
    EXPECT_EQ(valid_ip_address("1.1.1.1."), "Neither");      // trailing empty field
    EXPECT_EQ(valid_ip_address("1.1.1"), "Neither");
    EXPECT_EQ(valid_ip_address("1.1.1.1.1"), "Neither");
    EXPECT_EQ(valid_ip_address("1..1.1"), "Neither");
    EXPECT_EQ(valid_ip_address("1.2.3.a"), "Neither");
    EXPECT_EQ(valid_ip_address("1.2.3.-4"), "Neither");
    EXPECT_EQ(valid_ip_address("1.2.3.0004"), "Neither");
}
TEST(Parsing468, V6Edges) {
    EXPECT_EQ(valid_ip_address("2001:db8:85a3:0:0:8A2E:0370:7334"), "IPv6");
    EXPECT_EQ(valid_ip_address("2001:0db8:85a3::8A2E:037j:7334"), "Neither");
    EXPECT_EQ(valid_ip_address("02001:0db8:85a3:0000:0000:8a2e:0370:7334"), "Neither");  // 5 hex digits
    EXPECT_EQ(valid_ip_address("1:2:3:4:5:6:7:8:"), "Neither");
    EXPECT_EQ(valid_ip_address("1:2:3:4:5:6:7"), "Neither");
    EXPECT_EQ(valid_ip_address("::1"), "Neither");  // compression not accepted by this problem
    EXPECT_EQ(valid_ip_address(""), "Neither");
    EXPECT_EQ(valid_ip_address("hello"), "Neither");
}

// ---- 271 -------------------------------------------------------------------
TEST(Parsing271, RoundTripNormal) {
    const std::vector<std::string> in{"hello", "world"};
    EXPECT_EQ(decode_strings(encode_strings(in)), in);
}
TEST(Parsing271, RoundTripEdges) {
    for (const std::vector<std::string>& in : std::vector<std::vector<std::string>>{
             {},
             {""},
             {"", ""},
             {"#"},
             {"3#abc", "12#"},
             {"a#b#c", "", "99"},
             {std::string("nul\0byte", 8)},
             {std::string(1000, 'x')}}) {
        EXPECT_EQ(decode_strings(encode_strings(in)), in);
    }
}
TEST(Parsing271, WireFormatIsLengthPrefixed) {
    EXPECT_EQ(encode_strings({"ab", ""}), "2#ab0#");
}

// ---- 443 -------------------------------------------------------------------
static std::string run(std::vector<char> v) {
    const int n = compress(v);
    return std::string(v.begin(), v.begin() + n);
}
TEST(Parsing443, Normal) {
    EXPECT_EQ(run({'a', 'a', 'b', 'b', 'c', 'c', 'c'}), "a2b2c3");
    EXPECT_EQ(run({'a'}), "a");
    EXPECT_EQ(run({'a', 'b', 'c'}), "abc");
}
TEST(Parsing443, Edges) {
    EXPECT_EQ(run({'a', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b'}), "ab12");
    EXPECT_EQ(run(std::vector<char>(100, 'z')), "z100");
    EXPECT_EQ(run({}), "");
    std::vector<char> v{'a', 'a', 'b', 'b', 'c', 'c', 'c'};
    EXPECT_EQ(compress(v), 6);
}

// ---- 65 --------------------------------------------------------------------
TEST(Parsing65, Valid) {
    for (const char* s : {"2", "0089", "-0.1", "+3.14", "4.", "-.9", "2e10", "-90E3", "3e+7", "+6e-1",
                          "53.5e93", "-123.456e789", ".5", "0", "9."}) {
        EXPECT_TRUE(is_number(s)) << s;
    }
}
TEST(Parsing65, Invalid) {
    for (const char* s : {"abc", "1a", "1e", "e3", "99e2.5", "--6", "-+3", "95a54e53", "", ".", "+", "-.",
                          "e", "1e+", "1 ", " 1", "1.2.3", "1e1e1", "+.e1", "4e5.", "0x1A", "Infinity", "NaN"}) {
        EXPECT_FALSE(is_number(s)) << '"' << s << '"';
    }
}
