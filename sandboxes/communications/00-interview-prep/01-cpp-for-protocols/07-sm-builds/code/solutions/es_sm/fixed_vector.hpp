#pragma once
// SOLUTION - attempt Q25, Q26 first.
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace es::sm {

template <class T, std::size_t N>
class FixedVector {
    static_assert(N > 0, "N must be > 0");

public:
    using value_type = T;

    FixedVector() = default;

    ~FixedVector() { clear(); }

    FixedVector(const FixedVector& other) {
        try {
            for (std::size_t i = 0; i < other.size_; ++i) emplace_back(other[i]);
        } catch (...) {
            clear();  // the destructor will not run for a constructor that throws: clean up ourselves
            throw;
        }
    }

    FixedVector(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        try {
            for (std::size_t i = 0; i < other.size_; ++i) emplace_back(std::move(other[i]));
        } catch (...) {
            clear();
            throw;
        }
        other.clear();  // moved-from FixedVector is empty (elements are inline, so they were moved, not stolen)
    }

    FixedVector& operator=(const FixedVector&) = delete;
    FixedVector& operator=(FixedVector&&) = delete;

    template <class... Args>
    T* emplace_back(Args&&... args) {
        if (size_ == N) return nullptr;
        // Placement new: construct a T at this exact address, forwarding the arguments (no copy, no temp).
        T* p = ::new (static_cast<void*>(storage_ + size_ * sizeof(T))) T(std::forward<Args>(args)...);
        ++size_;  // only after success: if T's constructor throws, size_ still counts live objects only
        return p;
    }

    bool push_back(const T& v) { return emplace_back(v) != nullptr; }
    bool push_back(T&& v) { return emplace_back(std::move(v)) != nullptr; }

    void pop_back() noexcept {
        if (size_ == 0) return;
        --size_;
        elem(size_)->~T();  // explicit destructor call: the memory stays, the object ends
    }

    void clear() noexcept {
        while (size_ != 0) pop_back();  // reverse order of construction, like a normal container
    }

    std::size_t size() const noexcept { return size_; }
    static constexpr std::size_t capacity() noexcept { return N; }
    bool empty() const noexcept { return size_ == 0; }
    bool full() const noexcept { return size_ == N; }

    T& operator[](std::size_t i) noexcept { return *elem(i); }
    const T& operator[](std::size_t i) const noexcept { return *elem(i); }

    T* data() noexcept { return size_ ? elem(0) : nullptr; }
    const T* data() const noexcept { return size_ ? elem(0) : nullptr; }
    T* begin() noexcept { return data(); }
    T* end() noexcept { return data() + size_; }
    const T* begin() const noexcept { return data(); }
    const T* end() const noexcept { return data() + size_; }

private:
    T* elem(std::size_t i) noexcept {
        return std::launder(reinterpret_cast<T*>(storage_ + i * sizeof(T)));
    }
    const T* elem(std::size_t i) const noexcept {
        return std::launder(reinterpret_cast<const T*>(storage_ + i * sizeof(T)));
    }

    alignas(T) std::byte storage_[sizeof(T) * N];
    std::size_t size_ = 0;
};

}  // namespace es::sm
