#pragma once
// STUB - worksheet Q24. Lock-free single-producer / single-consumer queue (pack 06 §3.4 applied).
//
//   producer thread: try_push()          consumer thread: try_pop()          exactly ONE of each.
//
// Same free-running index idea as RingBuffer, but head_ and tail_ are std::atomic and each is WRITTEN by
// only one thread. Wait-free: no loops, no locks, no heap. N must be a power of two.
#include <array>
#include <atomic>
#include <cstddef>

namespace es::sm {

template <class T, std::size_t N>
class SpscQueue {
    static_assert(N > 0 && (N & (N - 1)) == 0, "N must be a power of two");

public:
    static constexpr std::size_t capacity() noexcept { return N; }

    // Producer only. false if full.
    // Hints: own index (head_) -> relaxed load. Other side's index (tail_) -> which order?
    //        write the element, THEN publish by storing head_ + 1 with which order?
    bool try_push(const T& v) {
        (void)v;
        return false;  // TODO
    }

    // Consumer only. false if empty. Mirror image of try_push.
    bool try_pop(T& out) {
        (void)out;
        return false;  // TODO
    }

    // Approximate number of elements (either thread, or a monitoring thread): never more than N.
    // Load tail_ BEFORE head_ (see worksheet 3.4) and clamp to N.
    std::size_t size_approx() const noexcept {
        return 0;  // TODO
    }

private:
    static constexpr std::size_t kCacheLine = 64;  // typical x86/ARM line size (VERIFY for your target)
    // Separate cache lines: the producer's writes to head_ must not invalidate the line holding tail_
    // (false sharing) - otherwise the two threads fight over one line on every operation.
    alignas(kCacheLine) std::atomic<std::size_t> head_{0};  // written by the producer only
    alignas(kCacheLine) std::atomic<std::size_t> tail_{0};  // written by the consumer only
    alignas(kCacheLine) std::array<T, N> buf_{};
};

}  // namespace es::sm
