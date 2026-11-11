#pragma once
// SOLUTION - attempt Q24 first.
#include <array>
#include <atomic>
#include <cstddef>

namespace es::sm {

template <class T, std::size_t N>
class SpscQueue {
    static_assert(N > 0 && (N & (N - 1)) == 0, "N must be a power of two");

public:
    static constexpr std::size_t capacity() noexcept { return N; }

    bool try_push(const T& v) {
        const std::size_t h = head_.load(std::memory_order_relaxed);   // only we write head_: always current
        const std::size_t t = tail_.load(std::memory_order_acquire);   // pairs with consumer's release: it has
                                                                       // FINISHED reading the slot we may reuse
        if (h - t == N) return false;                                  // full (stale t only makes us pessimistic)
        buf_[h & (N - 1)] = v;                                         // plain write into the free slot
        head_.store(h + 1, std::memory_order_release);                 // publish: element write happens-before
        return true;                                                   // a consumer that acquire-loads h + 1
    }

    bool try_pop(T& out) {
        const std::size_t t = tail_.load(std::memory_order_relaxed);   // only we write tail_
        const std::size_t h = head_.load(std::memory_order_acquire);   // pairs with producer's release
        if (h == t) return false;                                      // empty
        out = buf_[t & (N - 1)];                                       // plain read of a published element
        tail_.store(t + 1, std::memory_order_release);                 // hand the slot back
        return true;
    }

    std::size_t size_approx() const noexcept {
        // tail first, then head: head can only have grown since, so head >= tail_loaded and the unsigned
        // difference cannot underflow. It may overshoot N slightly if the consumer advanced meanwhile: clamp.
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t d = h - t;
        return d > N ? N : d;
    }

private:
    static constexpr std::size_t kCacheLine = 64;
    alignas(kCacheLine) std::atomic<std::size_t> head_{0};
    alignas(kCacheLine) std::atomic<std::size_t> tail_{0};
    alignas(kCacheLine) std::array<T, N> buf_{};
};

}  // namespace es::sm
