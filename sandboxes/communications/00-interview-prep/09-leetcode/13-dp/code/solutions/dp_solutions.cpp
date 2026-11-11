#include "dp.hpp"

#include <algorithm>
#include <climits>

namespace lc {

// 70 -- ways(n) = ways(n-1) + ways(n-2): the last step was a 1 or a 2, disjoint cases. O(n) time, O(1) space.
int climb_stairs(int n) {
    if (n <= 2) return n;
    int a = 1, b = 2;  // ways(1), ways(2)
    for (int i = 3; i <= n; ++i) {
        const int c = a + b;
        a = b;
        b = c;
    }
    return b;
}

// 198 -- best(i) = max(best(i-1) [skip i], best(i-2) + nums[i] [take i]). Two rolling variables.
int rob(const std::vector<int>& nums) {
    int skip = 0, take = 0;  // best up to i-2, best up to i-1
    for (int x : nums) {
        const int cur = std::max(take, skip + x);
        skip = take;
        take = cur;
    }
    return take;
}

// 322 -- dp[a] = 1 + min over coins c <= a of dp[a - c]; dp[0] = 0; "infinity" = amount + 1.
int coin_change(const std::vector<int>& coins, int amount) {
    if (amount < 0) return -1;
    const int inf = amount + 1;
    std::vector<int> dp(static_cast<std::size_t>(amount) + 1, inf);
    dp[0] = 0;
    for (int a = 1; a <= amount; ++a) {
        for (int c : coins) {
            if (c > 0 && c <= a) dp[static_cast<std::size_t>(a)] = std::min(dp[static_cast<std::size_t>(a)], dp[static_cast<std::size_t>(a - c)] + 1);
        }
    }
    return dp[static_cast<std::size_t>(amount)] >= inf ? -1 : dp[static_cast<std::size_t>(amount)];
}

// 300 -- patience sorting: tails[k] = smallest possible tail of an increasing subsequence of length k+1.
// tails is always sorted, so lower_bound (strict increase) finds where x belongs. O(n log n).
int length_of_lis(const std::vector<int>& nums) {
    std::vector<int> tails;
    for (int x : nums) {
        const auto it = std::lower_bound(tails.begin(), tails.end(), x);
        if (it == tails.end()) tails.push_back(x); else *it = x;
    }
    return static_cast<int>(tails.size());
}

// 1143 -- dp[i][j] = LCS of a[0..i), b[0..j). Match: diagonal + 1; mismatch: best of dropping one char.
// Two rolling rows -> O(min(m, n)) space is possible; one row of the shorter string used here.
int longest_common_subsequence(const std::string& a, const std::string& b) {
    const std::string& s = (a.size() >= b.size()) ? a : b;  // outer
    const std::string& t = (a.size() >= b.size()) ? b : a;  // inner (shorter): row length
    std::vector<int> prev(t.size() + 1, 0), cur(t.size() + 1, 0);
    for (std::size_t i = 1; i <= s.size(); ++i) {
        for (std::size_t j = 1; j <= t.size(); ++j) {
            if (s[i - 1] == t[j - 1]) cur[j] = prev[j - 1] + 1;
            else cur[j] = std::max(prev[j], cur[j - 1]);
        }
        std::swap(prev, cur);
    }
    return prev[t.size()];
}

}  // namespace lc
