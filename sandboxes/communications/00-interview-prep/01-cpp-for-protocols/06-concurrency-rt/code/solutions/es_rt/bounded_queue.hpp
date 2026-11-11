#pragma once
// SOLUTION - attempt Q20 first.
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

    bool push(T v) {
        std::unique_lock<std::mutex> lock(m_);
        // Predicate form: re-checks the condition after EVERY wake-up (spurious wake-ups happen, and
        // another producer may have taken the free slot between notify and our wake-up).
        not_full_.wait(lock, [this] { return closed_ || q_.size() < cap_; });
        if (closed_) return false;
        q_.push_back(std::move(v));
        lock.unlock();            // don't wake a consumer just to make it block on our mutex
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [this] { return closed_ || !q_.empty(); });
        if (q_.empty()) return std::nullopt;  // closed AND drained
        T v = std::move(q_.front());
        q_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return v;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;       // state change is made under the lock the waiters use
        }
        not_full_.notify_all();   // every waiter must re-evaluate
        not_empty_.notify_all();
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
