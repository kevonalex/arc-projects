#include "intervals.hpp"

#include <algorithm>

namespace lc {

// 56 -- sort by start; then each interval either extends the last output interval (start <= its end)
// or begins a new one. Sorting makes "overlaps with something earlier" equivalent to "overlaps with the LAST output".
Intervals merge_intervals(Intervals intervals) {
    std::sort(intervals.begin(), intervals.end());
    Intervals out;
    for (const auto& iv : intervals) {
        if (!out.empty() && iv[0] <= out.back()[1]) {
            out.back()[1] = std::max(out.back()[1], iv[1]);  // max: the new one may be nested inside
        } else {
            out.push_back(iv);
        }
    }
    return out;
}

// 57 -- three phases over the already-sorted input: strictly-before, overlapping (absorb), strictly-after.
Intervals insert_interval(const Intervals& sorted_disjoint, std::vector<int> nv) {
    Intervals out;
    std::size_t i = 0;
    const std::size_t n = sorted_disjoint.size();
    while (i < n && sorted_disjoint[i][1] < nv[0]) out.push_back(sorted_disjoint[i++]);
    while (i < n && sorted_disjoint[i][0] <= nv[1]) {
        nv[0] = std::min(nv[0], sorted_disjoint[i][0]);
        nv[1] = std::max(nv[1], sorted_disjoint[i][1]);
        ++i;
    }
    out.push_back(nv);
    while (i < n) out.push_back(sorted_disjoint[i++]);
    return out;
}

// 435 -- greedy "earliest finish": sort by END, keep an interval iff it starts at/after the last kept end.
// Everything else is removed. (Equivalently: n - maximum set of mutually compatible intervals.)
int erase_overlap_intervals(Intervals intervals) {
    if (intervals.empty()) return 0;
    std::sort(intervals.begin(), intervals.end(),
              [](const std::vector<int>& a, const std::vector<int>& b) { return a[1] < b[1]; });
    int kept = 1;
    int last_end = intervals[0][1];
    for (std::size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i][0] >= last_end) {  // touching is allowed
            ++kept;
            last_end = intervals[i][1];
        }
    }
    return static_cast<int>(intervals.size()) - kept;
}

}  // namespace lc
