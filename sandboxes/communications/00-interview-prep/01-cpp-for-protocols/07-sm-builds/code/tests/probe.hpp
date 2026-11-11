#pragma once
// Test helper: counts every construction, copy, move and destruction, and is NOT default constructible -
// so a container that secretly default-constructs unused slots is caught immediately.
#include <string>

namespace probe {

struct Counters {
    int ctors = 0;   // Probe(int)
    int copies = 0;  // Probe(const Probe&)
    int moves = 0;   // Probe(Probe&&)
    int dtors = 0;
    int live() const { return ctors + copies + moves - dtors; }
};

inline Counters& counters() {
    static Counters c;
    return c;
}
inline void reset() { counters() = Counters{}; }

struct Probe {
    int v;
    explicit Probe(int x) : v(x) { ++counters().ctors; }
    Probe(const Probe& o) : v(o.v) { ++counters().copies; }
    Probe(Probe&& o) noexcept : v(o.v) {
        o.v = -1;  // visibly moved-from
        ++counters().moves;
    }
    Probe& operator=(const Probe&) = delete;
    Probe& operator=(Probe&&) = delete;
    ~Probe() { ++counters().dtors; }
};

}  // namespace probe
