#pragma once
// STUB - worksheet Q25, Q26. FixedVector<T, N>: a vector with inline storage and capacity N. No heap.
//
// The elements live in RAW BYTES inside the object. Nothing is constructed until you push; only the
// first size_ slots hold live objects. So:
//   - T need not be default constructible,
//   - you construct with PLACEMENT NEW (::new (address) T(args...)),
//   - you destroy by calling the destructor yourself (p->~T()),
//   - the destructor of FixedVector must destroy exactly the live elements.
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

    ~FixedVector() {
        // TODO: destroy all live elements
    }

    // Copy: copy-construct each element into the new storage (placement new). If a copy throws, destroy
    // what was already built, then rethrow - the destructor of a half-constructed object does NOT run.
    FixedVector(const FixedVector& other) {
        (void)other;  // TODO
    }

    // Move: move-construct each element, then leave `other` EMPTY (destroy its moved-from elements).
    FixedVector(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        (void)other;  // TODO
    }

    FixedVector& operator=(const FixedVector&) = delete;
    FixedVector& operator=(FixedVector&&) = delete;

    // Construct a T in place at the end from args. Returns a pointer to it, or nullptr if full
    // (nothing constructed). Increment size_ only AFTER the construction succeeded.
    template <class... Args>
    T* emplace_back(Args&&... args) {
        (void)sizeof...(args);
        return nullptr;  // TODO
    }

    bool push_back(const T& v) { return emplace_back(v) != nullptr; }
    bool push_back(T&& v) { return emplace_back(std::move(v)) != nullptr; }

    // Destroy the last element. No-op if empty.
    void pop_back() noexcept {
        // TODO
    }

    // Destroy all elements (last to first).
    void clear() noexcept {
        // TODO
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
    // Pointer to the live element i (precondition i < size_). `std::launder` tells the compiler "an object
    // of type T really lives at this address" - needed because the bytes were an unsigned-char/std::byte
    // array and the T was created there by placement new (C++17 [ptr.launder]).
    T* elem(std::size_t i) noexcept {
        return std::launder(reinterpret_cast<T*>(storage_ + i * sizeof(T)));
    }
    const T* elem(std::size_t i) const noexcept {
        return std::launder(reinterpret_cast<const T*>(storage_ + i * sizeof(T)));
    }

    alignas(T) std::byte storage_[sizeof(T) * N];  // raw, uninitialised
    std::size_t size_ = 0;
};

}  // namespace es::sm
