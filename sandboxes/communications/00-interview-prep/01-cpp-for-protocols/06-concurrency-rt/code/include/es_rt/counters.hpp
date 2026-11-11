#pragma once
// STUBS - worksheet Q22. Two tiny atomic building blocks.
#include <atomic>
#include <cstdint>

namespace es::rt {

// Statistics counter (e.g. "frames received"), incremented by many threads, read occasionally.
// Nothing else is published through it, so it needs atomicity but NO ordering: use memory_order_relaxed.
class EventCounter {
public:
    void add(std::uint64_t n = 1) noexcept {
        (void)n;  // TODO
    }
    std::uint64_t load() const noexcept {
        return 0;  // TODO
    }

private:
    std::atomic<std::uint64_t> v_{0};
};

// "Please stop" flag, set by a controller thread, polled by a worker loop. A worker that sees the flag may
// also read data the controller wrote BEFORE setting it (e.g. a final configuration), so request() must
// release and requested() must acquire.
class StopFlag {
public:
    void request() noexcept {
        // TODO
    }
    bool requested() const noexcept {
        return false;  // TODO
    }

private:
    std::atomic<bool> flag_{false};
};

}  // namespace es::rt
