#pragma once
// SOLUTION - attempt Q17 first.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace es::lifetime {

namespace detail {
inline long g_live_blocks = 0;

inline std::uint8_t* alloc_bytes(std::size_t n) {
    if (n == 0) return nullptr;
    ++g_live_blocks;
    return new std::uint8_t[n]();
}
inline void free_bytes(std::uint8_t* p) noexcept {
    if (p == nullptr) return;
    --g_live_blocks;
    delete[] p;
}
}  // namespace detail

class Buffer {
public:
    Buffer() = default;
    explicit Buffer(std::size_t n) : data_(detail::alloc_bytes(n)), size_(n) {}

    ~Buffer() { detail::free_bytes(data_); }

    // Deep copy: allocate our own block, copy the bytes.
    Buffer(const Buffer& other) : data_(detail::alloc_bytes(other.size_)), size_(other.size_) {
        if (size_ != 0) std::memcpy(data_, other.data_, size_);  // guard: memcpy(nullptr, ...) is UB even for 0
    }

    // Move: steal the pointer, leave the source in the valid "empty" state. noexcept is
    // what lets std::vector move (not copy) elements when it reallocates.
    Buffer(Buffer&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)) {}

    // Copy-and-swap: strong exception safety (if allocation throws, *this is untouched),
    // and self-assignment is safe without a special case.
    Buffer& operator=(const Buffer& other) {
        Buffer tmp(other);
        swap(tmp);
        return *this;  // tmp now holds our old block and frees it
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {  // self-move must not free our own block
            detail::free_bytes(data_);
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    std::uint8_t* data() noexcept { return data_; }
    const std::uint8_t* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    std::uint8_t& operator[](std::size_t i) noexcept { return data_[i]; }
    const std::uint8_t& operator[](std::size_t i) const noexcept { return data_[i]; }

    void swap(Buffer& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    static long live_blocks() noexcept { return detail::g_live_blocks; }

private:
    std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
};

}  // namespace es::lifetime
