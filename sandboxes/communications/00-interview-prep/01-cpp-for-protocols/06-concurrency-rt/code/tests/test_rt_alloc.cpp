// Allocation-counting tests (worksheet 3.5, Q18, Q23).
// This translation unit REPLACES the global operator new/delete so we can count heap allocations.
// That is legal C++ ([replacement.functions]) but affects the whole executable - hence its own executable.
#include <es_rt/mailbox.hpp>
#include <es_rt/pool.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

namespace {
std::atomic<std::size_t> g_new_calls{0};

void* counted_alloc(std::size_t n) {
    g_new_calls.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
}  // namespace

void* operator new(std::size_t n) { return counted_alloc(n); }
void* operator new[](std::size_t n) { return counted_alloc(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {

// Counts allocations made by the current process between construction and count().
class AllocScope {
public:
    AllocScope() : start_(g_new_calls.load()) {}
    std::size_t count() const { return g_new_calls.load() - start_; }

private:
    std::size_t start_;
};

struct Frame {
    std::uint8_t data[160];  // e.g. 20 ms of 8 kHz 8-bit samples
    int len;
};

bool hook_active() {
    AllocScope s;
    // Call the allocation FUNCTION directly: a new-expression may legally be optimised away.
    void* p = ::operator new(sizeof(int));
    const bool counted = s.count() >= 1;
    ::operator delete(p);
    return counted;
}

}  // namespace

// Passes from the start: proves the measuring tool works (or skips if the sanitizer runtime
// does not let us replace operator new - then the other tests are meaningless, so they skip too).
TEST(AllocCounting, HookCountsNewAndVectorGrowth) {
    if (!hook_active()) GTEST_SKIP() << "operator new replacement not active under this runtime";
    AllocScope s;
    std::vector<int> v;
    for (int i = 0; i < 100; ++i) v.push_back(i);  // reallocates several times as it grows
    EXPECT_GE(s.count(), 5u) << "vector growth should have allocated repeatedly (1,2,4,8,...,128)";

    AllocScope s2;
    std::string big(200, 'x');  // longer than any small-string buffer -> heap
    EXPECT_GE(s2.count(), 1u);
}

TEST(AllocCounting, PoolHotLoopMakesNoHeapAllocations) {
    if (!hook_active()) GTEST_SKIP() << "operator new replacement not active under this runtime";
    es::rt::FixedPool<Frame, 8> pool;  // constructed (stack/static storage) BEFORE the measured region
    AllocScope s;
    for (int iter = 0; iter < 10000; ++iter) {
        Frame* a = pool.acquire();
        Frame* b = pool.acquire();
        ASSERT_NE(a, nullptr);
        ASSERT_NE(b, nullptr);
        a->len = iter;
        b->len = iter + 1;
        EXPECT_TRUE(pool.release(b));
        EXPECT_TRUE(pool.release(a));
    }
    EXPECT_EQ(s.count(), 0u) << "pool acquire/release must never touch the heap";
    EXPECT_EQ(pool.available(), 8u);
}

TEST(AllocCounting, MailboxHotLoopMakesNoHeapAllocations) {
    if (!hook_active()) GTEST_SKIP() << "operator new replacement not active under this runtime";
    es::rt::Mailbox<std::uint64_t> mb;
    AllocScope s;
    std::uint64_t out = 0;
    for (std::uint64_t i = 1; i <= 10000; ++i) {
        ASSERT_TRUE(mb.try_publish(i));
        ASSERT_TRUE(mb.try_take(out));
        ASSERT_EQ(out, i);
    }
    EXPECT_EQ(s.count(), 0u);
}
