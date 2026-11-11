#pragma once
// SOLUTION - attempt Q20 first.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace es::containers {

inline std::vector<std::string_view> split(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    std::size_t start = 0;
    for (;;) {
        const std::size_t pos = s.find(sep, start);
        if (pos == std::string_view::npos) {
            out.push_back(s.substr(start));   // last field (possibly empty)
            return out;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

inline std::optional<std::uint16_t> parse_u16(std::string_view s) {
    if (s.empty()) return std::nullopt;
    std::uint32_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        v = v * 10u + static_cast<std::uint32_t>(c - '0');  // v <= 65535 before this: max 655359, no overflow
        if (v > 65535u) return std::nullopt;                // check EVERY step
    }
    return static_cast<std::uint16_t>(v);
}

using Field = std::variant<std::monostate, std::int64_t, std::string_view>;

inline Field parse_field(std::string_view s) {
    if (s.empty()) return std::monostate{};
    std::size_t i = 0;
    bool neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    }
    const std::size_t digits = s.size() - i;
    if (digits == 0 || digits > 18) return s;  // "-" or too long for a safe int64 accumulate
    std::int64_t v = 0;
    for (; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return s;
        v = v * 10 + (s[i] - '0');
    }
    return neg ? -v : v;
}

inline std::string describe(const Field& f) {
    if (std::holds_alternative<std::monostate>(f)) return "empty";
    if (const auto* i = std::get_if<std::int64_t>(&f)) return "int:" + std::to_string(*i);
    if (const auto* sv = std::get_if<std::string_view>(&f)) return "str:" + std::string(*sv);
    return "?";  // unreachable while Field has exactly these three alternatives
}

}  // namespace es::containers
