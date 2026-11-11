#pragma once
// STUBS - worksheet Q21 (function templates) and Q22 (StaticStack).
#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace es::containers {

// lo if v < lo; hi if v > hi; else v. Precondition lo <= hi. Works for any T with operator<.
template <class T>
constexpr T clamp_val(T v, T lo, T hi) {
    (void)lo; (void)hi;
    return v;  // TODO
}

// Number of elements of a built-in array, deduced from its TYPE (T(&)[N]); evaluated at compile time.
template <class T, std::size_t N>
constexpr std::size_t array_size(const T (&)[N]) {
    return 1;  // TODO (wrong on purpose; must be 1+ so that arrays sized by it still compile)
}

// ceil(a / b) for UNSIGNED integers, correct even for a close to the maximum value
// (hint: (a + b - 1) / b overflows). b != 0.
template <class T>
constexpr T ceil_div(T a, T b) {
    static_assert(std::is_unsigned_v<T>, "ceil_div is for unsigned integers");
    (void)a; (void)b;
    return 0;  // TODO
}

// Sum of any number of arguments using a C++17 fold expression, accumulated in long long.
// sum_all() == 0, sum_all(1,2,3) == 6, sum_all(uint8_t{200}, uint8_t{100}) == 300.
template <class... Ts>
constexpr long long sum_all(Ts... xs) {
    ((void)xs, ...);
    return -1;  // TODO
}

// First element in [first, last) satisfying pred, or `last`.
template <class It, class Pred>
It find_if_simple(It first, It last, Pred pred) {
    (void)first; (void)pred;
    return last;  // TODO (stub is wrong: it never searches)
}

// Fixed-capacity LIFO stack with NO heap use. T must be default constructible (std::array<T,N>).
template <class T, std::size_t N>
class StaticStack {
public:
    // false (and no change) if full
    bool push(const T& v) {
        (void)v;
        return false;  // TODO
    }
    // nullopt if empty; otherwise the most recently pushed element, removed
    std::optional<T> pop() {
        return std::nullopt;  // TODO
    }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }
    static constexpr std::size_t capacity() { return N; }

private:
    std::array<T, N> items_{};
    std::size_t size_ = 0;
};

}  // namespace es::containers
