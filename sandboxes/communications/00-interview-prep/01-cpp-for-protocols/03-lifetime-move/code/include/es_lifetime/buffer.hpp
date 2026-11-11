#pragma once
// STUB - worksheet Q17 (rule of five). Tests fail until you implement the special members.
//
// Buffer owns a heap block of bytes. It must behave like a VALUE:
//   copy  -> independent deep copy
//   move  -> steals the block, leaves the source EMPTY (data()==nullptr, size()==0)
// ALWAYS allocate with detail::alloc_bytes and release with detail::free_bytes - they keep a
// live-block counter that the tests read through Buffer::live_blocks().
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace es::lifetime {

namespace detail {
inline long g_live_blocks = 0;

// Zero-initialised block of n bytes; nullptr if n == 0.
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

    // ---- TODO: the five special members --------------------------------------------
    ~Buffer() {
        // TODO: release the block
    }

    Buffer(const Buffer& other) : data_(other.data_), size_(other.size_) {
        // TODO: BUG ON PURPOSE - this shares the block. Make an independent copy.
    }

    Buffer(Buffer&& other) noexcept : data_(other.data_), size_(other.size_) {
        // TODO: steal AND leave `other` empty.
    }

    Buffer& operator=(const Buffer& other) {
        // TODO: BUG ON PURPOSE (shallow, leaks the old block, breaks on self-assignment)
        data_ = other.data_;
        size_ = other.size_;
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        // TODO: BUG ON PURPOSE. Free current block, steal, empty `other`, survive self-move.
        data_ = other.data_;
        size_ = other.size_;
        return *this;
    }
    // ---------------------------------------------------------------------------------

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
