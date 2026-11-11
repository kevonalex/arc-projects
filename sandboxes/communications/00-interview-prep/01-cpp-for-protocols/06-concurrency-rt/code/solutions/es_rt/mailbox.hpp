#pragma once
// SOLUTION - attempt Q21 first.
#include <atomic>

namespace es::rt {

template <class T>
class Mailbox {
public:
    bool try_publish(const T& v) {
        // acquire: pairs with the consumer's release-store of `false`; guarantees the consumer has FINISHED
        // reading slot_ before we overwrite it.
        if (full_.load(std::memory_order_acquire)) return false;
        slot_ = v;                                    // plain write
        full_.store(true, std::memory_order_release); // release: the write above becomes visible to whoever
        return true;                                  // acquire-loads `true`
    }

    bool try_take(T& out) {
        // acquire: pairs with the producer's release-store of `true`; makes slot_ contents visible.
        if (!full_.load(std::memory_order_acquire)) return false;
        out = slot_;                                   // plain read
        full_.store(false, std::memory_order_release); // release: our read of slot_ happens-before the next write
        return true;
    }

private:
    T slot_{};
    std::atomic<bool> full_{false};
};

}  // namespace es::rt
