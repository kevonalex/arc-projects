// Lab A (worksheet 3.1): lost updates. Deliberately contains a DATA RACE (undefined behaviour) - built
// WITHOUT sanitizers on purpose. Never copy the "racy" function into real code.
//
// `volatile` here only forces the compiler to emit a real load and a real store on every iteration
// (otherwise at -O2 it may legally turn the whole loop into "counter += 1000000" in a register).
// volatile adds NO atomicity and NO ordering between threads - that is the point of the demo.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 4;
constexpr int kIters = 1'000'000;

volatile int g_racy = 0;
std::atomic<int> g_atomic{0};

template <class F>
double run_threads(F body) {
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> ts;
    for (int t = 0; t < kThreads; ++t) ts.emplace_back(body);
    for (auto& t : ts) t.join();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

}  // namespace

int main() {
    const int expected = kThreads * kIters;

    const double ms_racy = run_threads([] {
        for (int i = 0; i < kIters; ++i) g_racy = g_racy + 1;  // load, add, store: three steps
    });
    const double ms_atomic = run_threads([] {
        for (int i = 0; i < kIters; ++i) g_atomic.fetch_add(1, std::memory_order_relaxed);  // one indivisible step
    });

    const int racy = g_racy;
    std::printf("expected      : %d\n", expected);
    std::printf("racy (volatile): %d   (lost %d updates, %.1f ms)\n", racy, expected - racy, ms_racy);
    std::printf("atomic        : %d   (lost %d updates, %.1f ms)\n", g_atomic.load(), expected - g_atomic.load(), ms_atomic);
    std::printf("Run it several times: the racy total differs from run to run.\n");
    return 0;
}
