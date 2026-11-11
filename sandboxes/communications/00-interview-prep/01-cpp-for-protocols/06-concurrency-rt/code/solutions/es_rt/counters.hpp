#pragma once
// SOLUTION - attempt Q22 first.
#include <atomic>
#include <cstdint>

namespace es::rt {

class EventCounter {
public:
    // fetch_add is ONE indivisible read-modify-write (unlike `++x` on a plain int: load, add, store).
    void add(std::uint64_t n = 1) noexcept { v_.fetch_add(n, std::memory_order_relaxed); }
    std::uint64_t load() const noexcept { return v_.load(std::memory_order_relaxed); }

private:
    std::atomic<std::uint64_t> v_{0};
};

class StopFlag {
public:
    void request() noexcept { flag_.store(true, std::memory_order_release); }
    bool requested() const noexcept { return flag_.load(std::memory_order_acquire); }

private:
    std::atomic<bool> flag_{false};
};

}  // namespace es::rt
