#include "sliding_window.hpp"

#include <algorithm>
#include <array>

namespace lc {

// 3 -- window [l, r] holds distinct chars. last[c] = index after the last occurrence of c.
// On a repeat, jump l past the previous occurrence (never move l backwards).
int length_of_longest_substring(const std::string& s) {
    std::array<int, 256> next_start{};  // 0-initialised: "window may start at 0"
    int best = 0, l = 0;
    for (int r = 0; r < static_cast<int>(s.size()); ++r) {
        const auto c = static_cast<unsigned char>(s[static_cast<std::size_t>(r)]);
        l = std::max(l, next_start[c]);
        best = std::max(best, r - l + 1);
        next_start[c] = r + 1;
    }
    return best;
}

// 424 -- window is valid iff (len - maxFreq) <= k. maxFreq is never decreased when the window
// slides: a stale (too big) maxFreq can only keep the window the same size, and the answer
// only grows when a TRUE higher frequency appears, so the result is still correct.
int character_replacement(const std::string& s, int k) {
    std::array<int, 26> cnt{};
    int l = 0, max_freq = 0, best = 0;
    for (int r = 0; r < static_cast<int>(s.size()); ++r) {
        max_freq = std::max(max_freq, ++cnt[static_cast<std::size_t>(s[static_cast<std::size_t>(r)] - 'A')]);
        if ((r - l + 1) - max_freq > k) {
            --cnt[static_cast<std::size_t>(s[static_cast<std::size_t>(l)] - 'A')];
            ++l;
        }
        best = std::max(best, r - l + 1);
    }
    return best;
}

// 76 -- expand r until the window covers t, then shrink l while it still does; record the best.
// `missing` counts how many required characters (with multiplicity) are still lacking.
std::string min_window(const std::string& s, const std::string& t) {
    if (t.empty() || s.size() < t.size()) return "";
    std::array<int, 256> need{};
    for (char c : t) ++need[static_cast<unsigned char>(c)];
    int missing = static_cast<int>(t.size());
    int best_l = 0, best_len = static_cast<int>(s.size()) + 1, l = 0;
    for (int r = 0; r < static_cast<int>(s.size()); ++r) {
        const auto rc = static_cast<unsigned char>(s[static_cast<std::size_t>(r)]);
        if (need[rc]-- > 0) --missing;  // only counts while it was still needed
        while (missing == 0) {
            if (r - l + 1 < best_len) {
                best_len = r - l + 1;
                best_l = l;
            }
            const auto lc_ = static_cast<unsigned char>(s[static_cast<std::size_t>(l)]);
            if (++need[lc_] > 0) ++missing;  // dropping a needed char breaks coverage
            ++l;
        }
    }
    return best_len > static_cast<int>(s.size()) ? "" : s.substr(static_cast<std::size_t>(best_l), static_cast<std::size_t>(best_len));
}

// 11 -- move the SHORTER wall inward: with the shorter wall fixed, every narrower pair that keeps it
// is no better, so that wall can be discarded safely.
int max_area(const std::vector<int>& height) {
    if (height.size() < 2) return 0;
    std::size_t l = 0, r = height.size() - 1;
    int best = 0;
    while (l < r) {
        const int h = std::min(height[l], height[r]);
        best = std::max(best, h * static_cast<int>(r - l));
        if (height[l] < height[r]) ++l; else --r;
    }
    return best;
}

}  // namespace lc
