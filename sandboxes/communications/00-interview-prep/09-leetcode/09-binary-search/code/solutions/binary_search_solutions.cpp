#include "binary_search.hpp"

namespace lc {

// 704 -- closed interval [lo, hi]; invariant: if target is present it is inside [lo, hi].
// lo + (hi - lo) / 2 avoids overflow of lo + hi.
int search(const std::vector<int>& nums, int target) {
    int lo = 0, hi = static_cast<int>(nums.size()) - 1;
    while (lo <= hi) {
        const int mid = lo + (hi - lo) / 2;
        const int v = nums[static_cast<std::size_t>(mid)];
        if (v == target) return mid;
        if (v < target) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

// 33 -- at least ONE half of [lo, hi] is always plainly sorted. Decide which, then test whether
// target lies inside that sorted half to choose the side to keep.
int search_rotated(const std::vector<int>& nums, int target) {
    int lo = 0, hi = static_cast<int>(nums.size()) - 1;
    auto at = [&nums](int i) { return nums[static_cast<std::size_t>(i)]; };
    while (lo <= hi) {
        const int mid = lo + (hi - lo) / 2;
        if (at(mid) == target) return mid;
        if (at(lo) <= at(mid)) {  // left half [lo, mid] sorted (<= handles lo == mid)
            if (at(lo) <= target && target < at(mid)) hi = mid - 1; else lo = mid + 1;
        } else {                  // right half [mid, hi] sorted
            if (at(mid) < target && target <= at(hi)) lo = mid + 1; else hi = mid - 1;
        }
    }
    return -1;
}

// 153 -- half-open style: compare mid with hi (NOT lo). If nums[mid] > nums[hi] the drop is right of mid.
// Otherwise the minimum is at mid or to its left. Loop ends with lo == hi == answer.
int find_min(const std::vector<int>& nums) {
    int lo = 0, hi = static_cast<int>(nums.size()) - 1;
    while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        if (nums[static_cast<std::size_t>(mid)] > nums[static_cast<std::size_t>(hi)]) lo = mid + 1; else hi = mid;
    }
    return nums[static_cast<std::size_t>(lo)];
}

}  // namespace lc
