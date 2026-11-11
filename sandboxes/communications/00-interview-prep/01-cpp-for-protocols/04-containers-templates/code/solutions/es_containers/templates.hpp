#pragma once
// SOLUTION - attempt Q21, Q22 first.
#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace es::containers {

template <class T>
constexpr T clamp_val(T v, T lo, T hi) {
    return v < lo ? lo : (hi < v ? hi : v);  // only operator< is required of T
}

template <class T, std::size_t N>
constexpr std::size_t array_size(const T (&)[N]) {
    return N;
}

template <class T>
constexpr T ceil_div(T a, T b) {
    static_assert(std::is_unsigned_v<T>, "ceil_div is for unsigned integers");
    return a / b + (a % b != 0 ? T{1} : T{0});  // never forms a + b - 1, so cannot overflow
}

template <class... Ts>
constexpr long long sum_all(Ts... xs) {
    return (0LL + ... + static_cast<long long>(xs));  // binary left fold with init 0
}

template <class It, class Pred>
It find_if_simple(It first, It last, Pred pred) {
    for (; first != last; ++first) {
        if (pred(*first)) return first;
    }
    return last;
}

template <class T, std::size_t N>
class StaticStack {
public:
    bool push(const T& v) {
        if (size_ == N) return false;
        items_[size_++] = v;
        return true;
    }
    std::optional<T> pop() {
        if (size_ == 0) return std::nullopt;
        return std::move(items_[--size_]);
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
