#pragma once
// STUBS - worksheet Q17 (be_bytes, erase_value, dedupe_sorted) and Q18 (frame_payload, sub_span).
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace es::containers {

using ByteView = std::span<const std::uint8_t>;

// Toy frame from pack 01:  0x7E | N | payload[N] | xor(payload)     (total N + 3 bytes)
// Return a VIEW of the payload INSIDE `frame` (no copy; payload.data() == frame.data() + 2),
// or std::nullopt if: shorter than 3 bytes, first byte != 0x7E, frame.size() != N + 3,
// or the xor checksum is wrong.
inline std::optional<ByteView> frame_payload(ByteView frame) {
    (void)frame;
    return std::nullopt;  // TODO
}

// Checked sub-view: s[offset, offset+count). nullopt if offset > s.size() or count > s.size() - offset.
// (Written so that offset + count can never overflow size_t.)
inline std::optional<ByteView> sub_span(ByteView s, std::size_t offset, std::size_t count) {
    (void)s; (void)offset; (void)count;
    return std::nullopt;  // TODO
}

// Big-endian bytes of v: 0x01020304 -> {1,2,3,4}. std::array: size known at compile time, no heap.
inline std::array<std::uint8_t, 4> be_bytes(std::uint32_t v) {
    (void)v;
    return {0xFF, 0xFF, 0xFF, 0xFF};  // TODO
}

// Remove every element equal to `value`; return how many were removed. (erase-remove idiom)
inline std::size_t erase_value(std::vector<std::uint8_t>& v, std::uint8_t value) {
    (void)v; (void)value;
    return 999;  // TODO
}

// Remove consecutive duplicates from a SORTED vector: {1,1,2,2,2,3} -> {1,2,3}.
inline void dedupe_sorted(std::vector<int>& v) {
    (void)v;  // TODO
}

}  // namespace es::containers
