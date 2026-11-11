#pragma once
// STUB - worksheet Q21. One-slot, single-producer/single-consumer hand-off using ONLY an atomic flag.
// The payload itself is a plain (non-atomic) object: the acquire/release pair on `full_` is what makes
// the producer's writes to `slot_` visible to the consumer, and the consumer's read finished before the
// producer overwrites the slot.
//
//   producer thread : try_publish()        consumer thread : try_take()
//   Exactly ONE producer and ONE consumer. No locks, no heap, never blocks (wait-free).
#include <atomic>

namespace es::rt {

template <class T>
class Mailbox {
public:
    // false if the previous value has not been taken yet. Otherwise copies v into the slot and publishes it.
    bool try_publish(const T& v) {
        (void)v;
        return false;  // TODO: which memory orders on full_?
    }

    // false if nothing is published. Otherwise copies the slot into `out`, frees the slot, returns true.
    bool try_take(T& out) {
        (void)out;
        return false;  // TODO
    }

private:
    T slot_{};
    std::atomic<bool> full_{false};
};

}  // namespace es::rt
