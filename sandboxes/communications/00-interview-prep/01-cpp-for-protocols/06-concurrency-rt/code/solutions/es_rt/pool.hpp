#pragma once
// SOLUTION - attempt Q23 first.
#include <array>
#include <cstddef>
#include <cstdint>

namespace es::rt {

template <class T, std::size_t N>
class FixedPool {
public:
    FixedPool() {
        for (std::size_t k = 0; k < N; ++k) free_[k] = N - 1 - k;  // top of stack = index 0
        free_count_ = N;
    }

    T* acquire() noexcept {
        if (free_count_ == 0) return nullptr;   // exhaustion is a normal, reportable condition - no throw, no alloc
        const std::size_t idx = free_[--free_count_];
        in_use_[idx] = true;
        return &items_[idx];
    }

    bool release(T* p) noexcept {
        // Pointer -> index, validated. Compare as integers to avoid comparing unrelated pointers.
        const auto base = reinterpret_cast<std::uintptr_t>(items_.data());
        const auto addr = reinterpret_cast<std::uintptr_t>(p);
        if (addr < base) return false;
        const std::size_t byte_off = static_cast<std::size_t>(addr - base);
        if (byte_off % sizeof(T) != 0) return false;   // points into the middle of an element
        const std::size_t idx = byte_off / sizeof(T);
        if (idx >= N) return false;                    // outside this pool
        if (!in_use_[idx]) return false;               // double release
        in_use_[idx] = false;
        free_[free_count_++] = idx;                    // LIFO: the most recently freed is reused first (cache-warm)
        return true;
    }

    std::size_t available() const noexcept { return free_count_; }
    static constexpr std::size_t capacity() noexcept { return N; }

private:
    std::array<T, N> items_{};
    std::array<std::size_t, N> free_{};
    std::array<bool, N> in_use_{};
    std::size_t free_count_ = 0;
};

}  // namespace es::rt
