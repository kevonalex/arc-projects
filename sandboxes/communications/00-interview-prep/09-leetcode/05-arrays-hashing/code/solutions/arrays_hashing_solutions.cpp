#include "arrays_hashing.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace lc {

// 1 -- one pass; the table maps "value seen" -> index. Complement is looked up BEFORE inserting
// the current element so an element is never paired with itself.
std::vector<int> two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;
    seen.reserve(nums.size());
    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        const long long need = static_cast<long long>(target) - nums[static_cast<std::size_t>(i)];
        if (need >= INT_MIN && need <= INT_MAX) {
            const auto it = seen.find(static_cast<int>(need));
            if (it != seen.end()) return {it->second, i};
        }
        seen[nums[static_cast<std::size_t>(i)]] = i;
    }
    return {};
}

// 49 -- canonical form = per-letter counts (O(L)), serialised as the hash key.
std::vector<std::vector<std::string>> group_anagrams(const std::vector<std::string>& strs) {
    std::unordered_map<std::string, std::vector<std::string>> groups;
    for (const auto& s : strs) {
        std::array<int, 26> cnt{};
        for (char c : s) ++cnt[static_cast<std::size_t>(c - 'a')];
        std::string key;
        for (int c : cnt) {
            key += std::to_string(c);
            key.push_back(',');  // separator: counts 1,11 vs 11,1 must differ
        }
        groups[key].push_back(s);
    }
    std::vector<std::vector<std::string>> out;
    out.reserve(groups.size());
    for (auto& kv : groups) out.push_back(std::move(kv.second));
    return out;
}

// 238 -- out[i] = (product of everything left of i) * (product of everything right of i).
// Pass 1 writes prefix products, pass 2 multiplies in a running suffix. No division, so zeros are harmless.
std::vector<int> product_except_self(const std::vector<int>& nums) {
    const std::size_t n = nums.size();
    std::vector<int> out(n, 1);
    int prefix = 1;
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = prefix;
        prefix *= nums[i];
    }
    int suffix = 1;
    for (std::size_t i = n; i-- > 0;) {
        out[i] *= suffix;
        suffix *= nums[i];
    }
    return out;
}

// 128 -- only start counting at a sequence START (x-1 absent); each element is then
// visited O(1) times in total across all runs, so O(n) not O(n^2).
int longest_consecutive(const std::vector<int>& nums) {
    const std::unordered_set<int> s(nums.begin(), nums.end());
    int best = 0;
    for (int x : s) {
        if (x > INT_MIN && s.count(x - 1)) continue;  // not a run start
        int len = 1;
        long long cur = x;
        while (cur < INT_MAX && s.count(static_cast<int>(cur + 1))) {
            ++cur;
            ++len;
        }
        best = std::max(best, len);
    }
    return best;
}

// 41 -- the answer lies in [1, n+1]. Use the array itself as a presence table:
// put value v at index v-1. Then the first index i with nums[i] != i+1 gives answer i+1.
int first_missing_positive(std::vector<int>& nums) {
    const int n = static_cast<int>(nums.size());
    for (int i = 0; i < n; ++i) {
        // `while`, not `if`: the swapped-in value may itself belong elsewhere.
        // Each swap puts one value in its final slot, so total swaps <= n.
        while (nums[static_cast<std::size_t>(i)] >= 1 && nums[static_cast<std::size_t>(i)] <= n &&
               nums[static_cast<std::size_t>(nums[static_cast<std::size_t>(i)] - 1)] != nums[static_cast<std::size_t>(i)]) {
            std::swap(nums[static_cast<std::size_t>(i)],
                      nums[static_cast<std::size_t>(nums[static_cast<std::size_t>(i)] - 1)]);
        }
    }
    for (int i = 0; i < n; ++i) {
        if (nums[static_cast<std::size_t>(i)] != i + 1) return i + 1;
    }
    return n + 1;
}

}  // namespace lc
