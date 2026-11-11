#pragma once
// STUB - worksheet Q23. A fixed-capacity object pool: NO heap, O(1), noexcept - fit for a real-time thread.
// All N objects live inside the pool object itself (std::array); a stack of free indices tracks which are free.
// Single-threaded by design: it is owned by ONE thread (hand objects to other threads through a queue).
#include <array>
#include <cstddef>

namespace es::rt {

template <class T, std::size_t N>
class FixedPool {
public:
    FixedPool() {
        // TODO: initialise the free-index stack so that the first acquire() returns &items_[0],
        //       the second &items_[1], ... (free_[k] = N - 1 - k, free_count_ = N)
    }

    // Returns a pointer to an unused element, or nullptr if all N are in use. Never allocates.
    T* acquire() noexcept {
        return nullptr;  // TODO
    }

    // Returns the element to the pool. Returns false (and changes nothing) if p does not point to the
    // START of an element of THIS pool, or the element is not currently acquired (double release).
    bool release(T* p) noexcept {
        (void)p;
        return false;  // TODO
    }

    std::size_t available() const noexcept { return free_count_; }
    static constexpr std::size_t capacity() noexcept { return N; }

private:
    std::array<T, N> items_{};
    std::array<std::size_t, N> free_{};  // stack of free indices
    std::array<bool, N> in_use_{};
    std::size_t free_count_ = 0;
};

}  // namespace es::rt
