#pragma once
// SOLUTION - attempt Q19 first.
#include <utility>

namespace es::lifetime {

template <class T>
class UniqueBox {
public:
    UniqueBox() noexcept = default;
    explicit UniqueBox(T* p) noexcept : p_(p) {}

    ~UniqueBox() { delete p_; }  // delete nullptr is a no-op

    UniqueBox(UniqueBox&& other) noexcept : p_(std::exchange(other.p_, nullptr)) {}

    UniqueBox& operator=(UniqueBox&& other) noexcept {
        if (this != &other) reset(other.release());
        return *this;
    }

    UniqueBox(const UniqueBox&) = delete;
    UniqueBox& operator=(const UniqueBox&) = delete;

    T* get() const noexcept { return p_; }

    T* release() noexcept { return std::exchange(p_, nullptr); }

    void reset(T* p = nullptr) noexcept {
        T* old = std::exchange(p_, p);  // install the new pointer BEFORE deleting the old one
        delete old;
    }

    T& operator*() const { return *p_; }
    T* operator->() const noexcept { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }

private:
    T* p_ = nullptr;
};

template <class T, class... Args>
UniqueBox<T> make_box(Args&&... args) {
    return UniqueBox<T>(new T(std::forward<Args>(args)...));
}

}  // namespace es::lifetime
