#pragma once
// STUB - worksheet Q22, Q23. Fixed-capacity FIFO ring buffer, single-threaded, no heap.
//
// Design: two FREE-RUNNING counters.
//   head_ = total number of elements ever pushed   (next write goes to slot head_ & mask)
//   tail_ = total number of elements ever popped   (next read  comes from slot tail_ & mask)
//   size  = head_ - tail_   (unsigned subtraction: still right after the counters wrap around)
// N must be a power of two so that  x & (N-1)  == x % N  and so that the counters' own wrap-around
// (at 2^bits) lines up with the end of the buffer. The `Index` type parameter exists so the tests can use
// uint8_t counters and force a wrap-around after only 256 pushes.
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace es::sm {

template <class T, std::size_t N, class Index = std::size_t>
class RingBuffer {
    static_assert(N > 0 && (N & (N - 1)) == 0, "N must be a power of two");
    static_assert(std::is_unsigned_v<Index>, "Index must be an unsigned integer type");
    static_assert(N <= std::numeric_limits<Index>::max(), "Index too small to count to N");
    static constexpr Index kMask = static_cast<Index>(N - 1);

public:
    static constexpr std::size_t capacity() noexcept { return N; }

    // Number of stored elements: head_ - tail_ computed in Index arithmetic. Beware integer promotion
    // (pack 02): for Index = uint8_t the subtraction happens in int; cast the result back to Index.
    std::size_t size() const noexcept {
        return 0;  // TODO
    }
    bool empty() const noexcept { return size() == 0; }
    bool full() const noexcept { return size() == N; }

    // false (and no change) if full.
    bool push(const T& v) {
        (void)v;
        return false;  // TODO
    }

    // Oldest element, or nullopt if empty.
    std::optional<T> pop() {
        return std::nullopt;  // TODO
    }

    // Always stores v. If the buffer is full the OLDEST element is dropped (diagnostic-log behaviour).
    void push_overwrite(const T& v) {
        (void)v;  // TODO
    }

    // i-th oldest element (0 = next to be popped). Precondition: i < size().
    const T& operator[](std::size_t i) const {
        return buf_[i & kMask];  // TODO: logical index -> (tail_ + i) & mask
    }

    void clear() noexcept {
        // TODO
    }

private:
    std::array<T, N> buf_{};
    Index head_ = 0;
    Index tail_ = 0;
};

}  // namespace es::sm
