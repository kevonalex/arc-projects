#pragma once
// SOLUTION - attempt Q18, Q19 first.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace es::containers {

using ByteView = std::span<const std::uint8_t>;

inline std::optional<ByteView> frame_payload(ByteView frame) {
    if (frame.size() < 3) return std::nullopt;           // look at bytes only after proving they exist
    if (frame[0] != 0x7E) return std::nullopt;
    const std::size_t n = frame[1];
    if (frame.size() != n + 3) return std::nullopt;      // exact length: no truncation, no trailing bytes
    ByteView payload = frame.subspan(2, n);              // a VIEW: no copy, no allocation
    std::uint8_t x = 0;
    for (std::uint8_t b : payload) x = static_cast<std::uint8_t>(x ^ b);
    if (x != frame[2 + n]) return std::nullopt;
    return payload;
}

inline std::optional<ByteView> sub_span(ByteView s, std::size_t offset, std::size_t count) {
    if (offset > s.size()) return std::nullopt;
    if (count > s.size() - offset) return std::nullopt;  // s.size() - offset cannot underflow here
    return s.subspan(offset, count);
}

inline std::array<std::uint8_t, 4> be_bytes(std::uint32_t v) {
    return {static_cast<std::uint8_t>(v >> 24), static_cast<std::uint8_t>(v >> 16),
            static_cast<std::uint8_t>(v >> 8), static_cast<std::uint8_t>(v)};
}

inline std::size_t erase_value(std::vector<std::uint8_t>& v, std::uint8_t value) {
    // remove() shuffles the keepers to the front and returns the new logical end;
    // erase() then actually shrinks the vector (remove alone never changes size()).
    auto new_end = std::remove(v.begin(), v.end(), value);
    const std::size_t removed = static_cast<std::size_t>(v.end() - new_end);
    v.erase(new_end, v.end());
    return removed;
}

inline void dedupe_sorted(std::vector<int>& v) {
    v.erase(std::unique(v.begin(), v.end()), v.end());
}

}  // namespace es::containers
