#pragma once
// PROVIDED (complete, not an exercise). A type that COUNTS what happens to it, so that
// "what is actually moved" becomes a number you can read instead of a belief.
// Identical copy lives in solutions/.
namespace es::lifetime {

struct Counts {
    int ctor = 0, copy_ctor = 0, move_ctor = 0, copy_assign = 0, move_assign = 0, dtor = 0;
};

template <bool NothrowMove>
class BasicProbe {
public:
    static inline Counts counts{};
    static void reset() { counts = Counts{}; }

    BasicProbe() { ++counts.ctor; }
    explicit BasicProbe(int v) : value(v) { ++counts.ctor; }
    BasicProbe(const BasicProbe& o) : value(o.value) { ++counts.copy_ctor; }
    BasicProbe(BasicProbe&& o) noexcept(NothrowMove) : value(o.value) {
        o.value = -1;  // "moved-from" marker so tests can see the source was touched
        ++counts.move_ctor;
    }
    BasicProbe& operator=(const BasicProbe& o) {
        value = o.value;
        ++counts.copy_assign;
        return *this;
    }
    BasicProbe& operator=(BasicProbe&& o) noexcept(NothrowMove) {
        value = o.value;
        o.value = -1;
        ++counts.move_assign;
        return *this;
    }
    ~BasicProbe() { ++counts.dtor; }

    int value = 0;
};

using Probe = BasicProbe<true>;          // noexcept move: containers will MOVE on reallocation
using SlowMoveProbe = BasicProbe<false>; // potentially-throwing move: containers will COPY

}  // namespace es::lifetime
