#pragma once
// STUB - worksheet Q18. RAII for "run this when I leave the scope, however I leave it".
#include <utility>

namespace es::lifetime {

template <class F>
class ScopeExit {
public:
    explicit ScopeExit(F f) : f_(std::move(f)) {}
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

    ~ScopeExit() {
        // TODO: run f_ unless release() was called
    }

    // Cancel the guard (the "commit" step: everything succeeded, nothing to undo).
    void release() noexcept {
        // TODO
    }

private:
    F f_;
    bool active_ = true;
};

template <class F>
ScopeExit(F) -> ScopeExit<F>;  // C++17 deduction guide: `ScopeExit g([&]{ ... });`

}  // namespace es::lifetime
