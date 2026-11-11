#include "parsing.hpp"

#include <climits>
#include <cstddef>

namespace lc {

namespace {
bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_hex(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Split on `delim`, KEEPING empty tokens ("1.1." -> {"1","1",""}), because an
// empty field is exactly the malformed case we must reject.
std::vector<std::string> split_keep_empty(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == delim) {
            parts.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    parts.push_back(cur);
    return parts;
}

bool valid_v4(const std::string& ip) {
    const auto parts = split_keep_empty(ip, '.');
    if (parts.size() != 4) return false;
    for (const auto& p : parts) {
        if (p.empty() || p.size() > 3) return false;
        if (p.size() > 1 && p[0] == '0') return false;  // no leading zeros
        int v = 0;
        for (char c : p) {
            if (!is_digit(c)) return false;
            v = v * 10 + (c - '0');
        }
        if (v > 255) return false;
    }
    return true;
}

bool valid_v6(const std::string& ip) {
    const auto parts = split_keep_empty(ip, ':');
    if (parts.size() != 8) return false;
    for (const auto& p : parts) {
        if (p.empty() || p.size() > 4) return false;
        for (char c : p)
            if (!is_hex(c)) return false;
    }
    return true;
}
}  // namespace

// 8 -- state order: whitespace -> sign -> digits -> stop. Saturate BEFORE overflow happens.
int my_atoi(const std::string& s) {
    std::size_t i = 0;
    const std::size_t n = s.size();
    while (i < n && s[i] == ' ') ++i;
    int sign = 1;
    if (i < n && (s[i] == '+' || s[i] == '-')) {
        if (s[i] == '-') sign = -1;
        ++i;
    }
    int acc = 0;  // magnitude, kept <= INT_MAX
    while (i < n && is_digit(s[i])) {
        const int d = s[i] - '0';
        if (acc > (INT_MAX - d) / 10) return sign == 1 ? INT_MAX : INT_MIN;
        acc = acc * 10 + d;
        ++i;
    }
    return sign * acc;
}

// 468
std::string valid_ip_address(const std::string& ip) {
    if (ip.find('.') != std::string::npos) return valid_v4(ip) ? "IPv4" : "Neither";
    if (ip.find(':') != std::string::npos) return valid_v6(ip) ? "IPv6" : "Neither";
    return "Neither";
}

// 271 -- "<len>#<payload>" per string. The length, not a delimiter, ends the payload,
// so payloads may contain '#', digits, NULs, anything.
std::string encode_strings(const std::vector<std::string>& strs) {
    std::string out;
    for (const auto& s : strs) {
        out += std::to_string(s.size());
        out.push_back('#');
        out += s;
    }
    return out;
}

std::vector<std::string> decode_strings(const std::string& s) {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t len = 0;
        while (i < s.size() && s[i] != '#') {
            len = len * 10 + static_cast<std::size_t>(s[i] - '0');
            ++i;
        }
        ++i;  // skip '#'
        out.push_back(s.substr(i, len));
        i += len;
    }
    return out;
}

// 443 -- read pointer scans runs, write pointer never overtakes it.
int compress(std::vector<char>& chars) {
    const std::size_t n = chars.size();
    std::size_t read = 0, write = 0;
    while (read < n) {
        const char c = chars[read];
        std::size_t run = 0;
        while (read < n && chars[read] == c) {
            ++read;
            ++run;
        }
        chars[write++] = c;
        if (run > 1) {
            for (char d : std::to_string(run)) chars[write++] = d;
        }
    }
    return static_cast<int>(write);
}

// 65 -- DFA. Classes: 0 digit, 1 sign, 2 dot, 3 e/E, 4 other.
//  S0 start | S1 sign | S2 int digits | S3 "d." | S4 "." (needs digit)
//  S5 frac digits | S6 e | S7 e-sign | S8 exp digits.   Accepting: 2,3,5,8
bool is_number(const std::string& s) {
    static constexpr int kDead = -1;
    static constexpr int kTable[9][5] = {
        /*S0*/ {2, 1, 4, kDead, kDead},
        /*S1*/ {2, kDead, 4, kDead, kDead},
        /*S2*/ {2, kDead, 3, 6, kDead},
        /*S3*/ {5, kDead, kDead, 6, kDead},
        /*S4*/ {5, kDead, kDead, kDead, kDead},
        /*S5*/ {5, kDead, kDead, 6, kDead},
        /*S6*/ {8, 7, kDead, kDead, kDead},
        /*S7*/ {8, kDead, kDead, kDead, kDead},
        /*S8*/ {8, kDead, kDead, kDead, kDead},
    };
    int state = 0;
    for (char c : s) {
        int cls = 4;
        if (is_digit(c)) cls = 0;
        else if (c == '+' || c == '-') cls = 1;
        else if (c == '.') cls = 2;
        else if (c == 'e' || c == 'E') cls = 3;
        state = kTable[state][cls];
        if (state == kDead) return false;
    }
    return state == 2 || state == 3 || state == 5 || state == 8;
}

}  // namespace lc
