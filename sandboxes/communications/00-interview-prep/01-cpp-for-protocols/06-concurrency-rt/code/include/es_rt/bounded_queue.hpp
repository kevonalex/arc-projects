#pragma once
// STUB - worksheet Q20. A blocking multi-producer/multi-consumer queue: mutex + condition_variable.
// Gateway use: hand decoded packets from the network thread to worker threads (NOT for the real-time path:
// it takes a lock and may allocate; see worksheet 3.5).
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace es::rt {

template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : cap_(capacity) {}

    // Blocks while the queue is full. Returns true if the value was enqueued; returns false (value
    // dropped) if the queue is/gets closed while waiting or was already closed.
    bool push(T v) {
        (void)v;
        return false;  // TODO
    }

    // Blocks while the queue is empty and not closed. Returns the oldest value, or nullopt once the queue
    // is closed AND drained (values enqueued before close() are still delivered).
    std::optional<T> pop() {
        return std::nullopt;  // TODO
    }

    // Wake every waiter. Afterwards push() fails; pop() drains what is left, then returns nullopt.
    void close() {
        // TODO
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
    }

private:
    mutable std::mutex m_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::deque<T> q_;
    std::size_t cap_;
    bool closed_ = false;
};

}  // namespace es::rt
