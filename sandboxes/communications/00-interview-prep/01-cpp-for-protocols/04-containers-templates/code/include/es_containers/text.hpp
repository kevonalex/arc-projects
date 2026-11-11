#pragma once
// STUBS - worksheet Q19 (split, parse_u16) and Q20 (parse_field, describe).
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace es::containers {

// Split on `sep`, KEEPING empty fields. The number of fields is always (count of sep) + 1:
//   "a,b,c" -> {"a","b","c"};  "a,,b" -> {"a","","b"};  "" -> {""};  "," -> {"",""};  "a," -> {"a",""}
// The returned views point INTO `s` - no copies. They are valid only while the characters of `s` live.
inline std::vector<std::string_view> split(std::string_view s, char sep) {
    (void)s; (void)sep;
    return {};  // TODO
}

// Parse an unsigned decimal in [0, 65535]. Digits only (no sign, no spaces, no "0x"), at least one
// digit. Leading zeros are allowed. nullopt if empty, any non-digit, or the value exceeds 65535.
// Must not overflow while accumulating (check as you go).
inline std::optional<std::uint16_t> parse_u16(std::string_view s) {
    (void)s;
    return std::nullopt;  // TODO
}

// A config/CLI-like field that may be empty, an integer, or arbitrary text.
using Field = std::variant<std::monostate, std::int64_t, std::string_view>;

// ""            -> monostate
// "42" / "-7"   -> int64   (optional leading '-', then 1..18 digits, nothing else)
// anything else -> string_view of the input (e.g. "abc", "12a", "-", and 19+ digit numbers)
inline Field parse_field(std::string_view s) {
    (void)s;
    return std::string_view{"TODO"};  // TODO
}

// monostate -> "empty";  int -> "int:" + decimal;  string_view -> "str:" + text
// Use std::holds_alternative / std::get_if here (std::visit is pack 07).
inline std::string describe(const Field& f) {
    (void)f;
    return "TODO";  // TODO
}

}  // namespace es::containers
