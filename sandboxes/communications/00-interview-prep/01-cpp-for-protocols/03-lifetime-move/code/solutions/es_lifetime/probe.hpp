#pragma once
// PROVIDED (complete, not an exercise). Identical to include/es_lifetime/probe.hpp.
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
        o.value = -1;
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

using Probe = BasicProbe<true>;
using SlowMoveProbe = BasicProbe<false>;

}  // namespace es::lifetime
