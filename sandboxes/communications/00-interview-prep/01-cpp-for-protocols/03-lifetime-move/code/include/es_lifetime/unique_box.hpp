#pragma once
// STUB - worksheet Q19. A minimal std::unique_ptr: exclusive ownership of one heap object.
#include <utility>

namespace es::lifetime {

template <class T>
class UniqueBox {
public:
    UniqueBox() noexcept = default;
    explicit UniqueBox(T* p) noexcept : p_(p) {}

    // TODO(Q19): the four lifetime functions. They compile but are WRONG on purpose.
    ~UniqueBox() {
        // TODO: delete the owned object
    }
    UniqueBox(UniqueBox&& other) noexcept {
        (void)other;  // TODO: take other's pointer, leave other null
    }
    UniqueBox& operator=(UniqueBox&& other) noexcept {
        (void)other;  // TODO: delete our object, take other's, survive self-move-assignment
        return *this;
    }

    UniqueBox(const UniqueBox&) = delete;             // exclusive ownership: copying is meaningless
    UniqueBox& operator=(const UniqueBox&) = delete;

    T* get() const noexcept { return p_; }

    // Give up ownership WITHOUT deleting; the caller becomes responsible.
    T* release() noexcept {
        return nullptr;  // TODO
    }

    // Delete the current object (if any), then own p.
    void reset(T* p = nullptr) noexcept {
        (void)p;  // TODO
    }

    T& operator*() const { return *p_; }
    T* operator->() const noexcept { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }

private:
    T* p_ = nullptr;
};

// Provided: construct and wrap in one step (so the raw `new` never appears in user code).
template <class T, class... Args>
UniqueBox<T> make_box(Args&&... args) {
    return UniqueBox<T>(new T(std::forward<Args>(args)...));
}

}  // namespace es::lifetime
