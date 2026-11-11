#pragma once
// STUB - worksheet Q20. Gateway scenario: a call-event bus tells interested components
// (logger, recorder, billing) about call events. The registry must NOT keep a component
// alive (the component's owner decides its lifetime) and must NOT call a destroyed one.
#include <cstddef>
#include <memory>
#include <vector>

namespace es::lifetime {

struct Listener {
    virtual ~Listener() = default;
    virtual void on_event(int code) = 0;
};

class CallRegistry {
public:
    // Remember the listener WITHOUT extending its lifetime (hint: weak_ptr).
    void add(const std::shared_ptr<Listener>& l) {
        (void)l;  // TODO
    }

    // Call on_event(code) on every listener that is still alive; drop the expired ones.
    // Returns the number of listeners called.
    // Must be safe if a listener calls add() from inside on_event (that new listener is NOT
    // called in this round) or releases the last owner of another listener.
    std::size_t notify(int code) {
        (void)code;
        return 0;  // TODO
    }

    // Entries currently stored (alive or not yet pruned).
    std::size_t size() const { return entries_.size(); }

private:
    std::vector<std::weak_ptr<Listener>> entries_;
};

}  // namespace es::lifetime
