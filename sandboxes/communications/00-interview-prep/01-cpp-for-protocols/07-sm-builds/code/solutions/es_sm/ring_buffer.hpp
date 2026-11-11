#pragma once
// SOLUTION - attempt Q22, Q23 first.
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

    std::size_t size() const noexcept {
        // For uint8_t, `head_ - tail_` is computed in int (promotion). E.g. head_=2 (wrapped), tail_=254:
        // 2 - 254 = -252 as int; casting to uint8_t gives 4 (= -252 mod 256): exactly the element count.
        return static_cast<Index>(head_ - tail_);
    }
    bool empty() const noexcept { return head_ == tail_; }
    bool full() const noexcept { return size() == N; }

    bool push(const T& v) {
        if (full()) return false;
        buf_[head_ & kMask] = v;  // N is a power of two, so & replaces % (one AND instead of a division)
        ++head_;                  // free-running: never reduced, wraps at 2^bits - which N divides
        return true;
    }

    std::optional<T> pop() {
        if (empty()) return std::nullopt;
        T v = std::move(buf_[tail_ & kMask]);
        ++tail_;
        return v;
    }

    void push_overwrite(const T& v) {
        if (full()) ++tail_;  // drop the oldest
        push(v);
    }

    const T& operator[](std::size_t i) const {
        return buf_[(static_cast<std::size_t>(tail_) + i) & kMask];
    }

    void clear() noexcept { tail_ = head_; }

private:
    std::array<T, N> buf_{};
    Index head_ = 0;
    Index tail_ = 0;
};

}  // namespace es::sm
