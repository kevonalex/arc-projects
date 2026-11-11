#pragma once
// SOLUTION - attempt Q18 first.
#include <utility>

namespace es::lifetime {

template <class F>
class ScopeExit {
public:
    explicit ScopeExit(F f) : f_(std::move(f)) {}
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

    // Destructors are noexcept by default: if f_ throws while another exception is
    // unwinding, std::terminate is called. Cleanup callbacks must not throw.
    ~ScopeExit() {
        if (active_) f_();
    }

    void release() noexcept { active_ = false; }

private:
    F f_;
    bool active_ = true;
};

template <class F>
ScopeExit(F) -> ScopeExit<F>;

}  // namespace es::lifetime
