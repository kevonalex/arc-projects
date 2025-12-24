# C++ Guide for Quantitative Developers

## Table of Contents
1. [Move Semantics & Performance](#move-semantics--performance)
2. [Concurrency & Threading](#concurrency--threading)
3. [STL Containers Deep Dive](#stl-containers-deep-dive)
4. [STL Algorithms](#stl-algorithms)
5. [Template Metaprogramming](#template-metaprogramming)
6. [Modern C++20 Features](#modern-c20-features)
7. [Performance Optimization](#performance-optimization)
8. [Numerical Computing](#numerical-computing)
9. [Low-Latency Programming](#low-latency-programming)
10. [Best Practices for Quant Code](#best-practices-for-quant-code)

---

## Move Semantics & Performance

### What Are Move Semantics?
**Move semantics** (C++11) allow transferring resources instead of copying them:
- **Performance**: Avoid expensive copies
- **Efficiency**: Transfer ownership of large objects
- **Zero-cost abstractions**: Move is often as cheap as copying a pointer

### Lvalues and Rvalues
```cpp
int x = 10;           // x is an lvalue (has a name, can take address)
int& ref = x;         // ref is an lvalue reference

int&& rref = 10;      // rref is an rvalue reference (temporary, about to be destroyed)
int&& rref2 = std::move(x);  // Convert lvalue to rvalue
```

### Move Constructor and Move Assignment
```cpp
class Vector {
private:
    double* data;
    size_t size;
    
public:
    // Move constructor - steals resources from other
    Vector(Vector&& other) noexcept 
        : data(other.data), size(other.size) {
        other.data = nullptr;  // Leave other in valid but empty state
        other.size = 0;
    }
    
    // Move assignment operator
    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            delete[] data;        // Free current resources
            data = other.data;     // Steal from other
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
    
    // Copy constructor (for comparison)
    Vector(const Vector& other) : size(other.size) {
        data = new double[size];
        std::copy(other.data, other.data + size, data);  // Expensive copy!
    }
};

// Usage
Vector v1(1000);           // Create large vector
Vector v2 = std::move(v1); // Move: fast! (just pointer swap)
Vector v3 = v1;            // Copy: slow! (copies all 1000 elements)
```

### When Move Happens Automatically
```cpp
// Return values are automatically moved
Vector create_vector() {
    Vector v(1000);
    return v;  // Automatically moved (not copied) if compiler can't elide
}

// Temporary objects are moved
Vector v = create_vector();  // Move constructor called

// std::move explicitly requests move
Vector v1(1000);
Vector v2 = std::move(v1);  // Explicitly move v1
```

### Perfect Forwarding
**Forward arguments without losing their value category (lvalue/rvalue).**

```cpp
template<typename T>
void wrapper(T&& arg) {  // Universal reference
    process(std::forward<T>(arg));  // Forward as lvalue or rvalue
}

// Usage
int x = 42;
wrapper(x);              // Forwarded as lvalue
wrapper(42);             // Forwarded as rvalue
wrapper(std::move(x));   // Forwarded as rvalue
```

**Why it matters for quant code:**
- Avoid unnecessary copies in template functions
- Efficient parameter passing in generic code
- Critical for high-performance libraries

---

## Concurrency & Threading

### Thread Basics (C++11)
```cpp
#include <thread>
#include <iostream>

void compute(int id) {
    std::cout << "Thread " << id << " working\n";
}

int main() {
    std::thread t1(compute, 1);
    std::thread t2(compute, 2);
    
    t1.join();  // Wait for thread to finish
    t2.join();
    return 0;
}
```

### Data Races and Mutexes
```cpp
#include <thread>
#include <mutex>
#include <vector>

std::mutex mtx;
int shared_data = 0;

void increment() {
    for (int i = 0; i < 1000000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // RAII lock
        ++shared_data;  // Protected by mutex
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(increment);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << shared_data << std::endl;  // Should be 4000000
}
```

### Atomic Operations (Lock-Free)
```cpp
#include <atomic>
#include <thread>

std::atomic<int> counter{0};  // Thread-safe without locks

void increment() {
    for (int i = 0; i < 1000000; ++i) {
        ++counter;  // Atomic operation - no lock needed!
    }
}

// Atomic operations are much faster than mutexes for simple operations
```

### Condition Variables
```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

std::mutex mtx;
std::condition_variable cv;
std::queue<double> data_queue;
bool finished = false;

void producer() {
    for (int i = 0; i < 100; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        data_queue.push(i * 1.5);
        cv.notify_one();  // Wake up consumer
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
    }
    cv.notify_all();
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return !data_queue.empty() || finished; });
        
        if (finished && data_queue.empty()) break;
        
        double value = data_queue.front();
        data_queue.pop();
        lock.unlock();
        
        // Process value...
    }
}
```

### Thread Pools (C++11)
```cpp
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    
public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]{ return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    
    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }
};
```

### Parallel Algorithms (C++17)
```cpp
#include <algorithm>
#include <execution>
#include <vector>

std::vector<double> data(1000000);

// Parallel execution
std::sort(std::execution::par, data.begin(), data.end());
std::transform(std::execution::par_unseq, 
               data.begin(), data.end(), data.begin(),
               [](double x) { return x * 2.0; });
```

### Futures and Async
```cpp
#include <future>
#include <vector>

// Async execution
auto future1 = std::async(std::launch::async, compute_price, params1);
auto future2 = std::async(std::launch::async, compute_price, params2);

double result1 = future1.get();  // Wait and get result
double result2 = future2.get();

// Parallel Monte Carlo
std::vector<std::future<double>> futures;
for (int i = 0; i < num_simulations; ++i) {
    futures.push_back(std::async(std::launch::async, monte_carlo_simulation));
}

double sum = 0.0;
for (auto& f : futures) {
    sum += f.get();
}
double average = sum / num_simulations;
```

---

## STL Containers Deep Dive

### Container Complexity

| Container | Insert | Access | Search | Memory |
|-----------|--------|--------|--------|--------|
| `vector` | O(1) amortized | O(1) | O(n) | Contiguous |
| `deque` | O(1) | O(1) | O(n) | Chunks |
| `list` | O(1) | O(n) | O(n) | Linked |
| `map` | O(log n) | O(log n) | O(log n) | Tree |
| `unordered_map` | O(1) avg | O(1) avg | O(1) avg | Hash table |
| `set` | O(log n) | O(log n) | O(log n) | Tree |
| `unordered_set` | O(1) avg | O(1) avg | O(1) avg | Hash table |

### Vector - The Workhorse
```cpp
#include <vector>

std::vector<double> prices;

// Reserve space (avoid reallocations)
prices.reserve(10000);  // Pre-allocate capacity

// Emplace (construct in place - more efficient)
prices.emplace_back(100.5);  // No copy/move

// Access patterns
for (size_t i = 0; i < prices.size(); ++i) {
    prices[i] *= 1.1;  // Fast: O(1) access
}

// Range-based for (C++11)
for (auto& price : prices) {
    price *= 1.1;
}

// Iterator access
for (auto it = prices.begin(); it != prices.end(); ++it) {
    *it *= 1.1;
}
```

### Map vs Unordered Map
```cpp
#include <map>
#include <unordered_map>

// std::map - Ordered, O(log n) operations
std::map<std::string, double> ordered_prices;
ordered_prices["AAPL"] = 150.0;
ordered_prices["GOOGL"] = 2800.0;
// Iteration is in sorted order

// std::unordered_map - Hash table, O(1) average
std::unordered_map<std::string, double> hash_prices;
hash_prices["AAPL"] = 150.0;
hash_prices["GOOGL"] = 2800.0;
// Faster lookups, but no order

// For quant: Use unordered_map for fast lookups
// Use map if you need sorted iteration
```

### Custom Hash Functions
```cpp
#include <unordered_map>

struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// Custom hash function
struct PointHash {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

std::unordered_map<Point, double, PointHash> point_values;
```

### Set Operations
```cpp
#include <set>
#include <unordered_set>

// std::set - Ordered, unique elements
std::set<int> unique_values;
unique_values.insert(5);
unique_values.insert(3);
unique_values.insert(5);  // Ignored (already exists)

// Fast membership test
if (unique_values.find(5) != unique_values.end()) {
    // Found
}

// std::unordered_set - Hash set, faster
std::unordered_set<int> hash_set;
hash_set.insert(5);
// O(1) average lookup
```

### Deque - Double-Ended Queue
```cpp
#include <deque>

std::deque<double> prices;

prices.push_front(100.0);  // O(1) - vector can't do this efficiently
prices.push_back(200.0);   // O(1)
prices.pop_front();         // O(1)
prices.pop_back();          // O(1)

// Good for sliding windows, queues
```

---

## STL Algorithms

### Common Algorithms
```cpp
#include <algorithm>
#include <numeric>
#include <vector>

std::vector<double> prices = {100.0, 150.0, 120.0, 200.0, 180.0};

// Sorting
std::sort(prices.begin(), prices.end());
std::sort(prices.begin(), prices.end(), std::greater<double>());  // Descending

// Finding
auto it = std::find(prices.begin(), prices.end(), 150.0);
auto it2 = std::find_if(prices.begin(), prices.end(), 
                        [](double p) { return p > 150.0; });

// Counting
int count = std::count_if(prices.begin(), prices.end(),
                          [](double p) { return p > 150.0; });

// Transform
std::vector<double> returns;
std::transform(prices.begin(), prices.end(), 
               std::back_inserter(returns),
               [](double p) { return p * 0.1; });

// Accumulate (sum, product, etc.)
double total = std::accumulate(prices.begin(), prices.end(), 0.0);
double product = std::accumulate(prices.begin(), prices.end(), 1.0,
                                 std::multiplies<double>());

// Min/Max
auto minmax = std::minmax_element(prices.begin(), prices.end());
double min_price = *minmax.first;
double max_price = *minmax.second;

// Binary search (requires sorted range)
bool found = std::binary_search(prices.begin(), prices.end(), 150.0);
auto lower = std::lower_bound(prices.begin(), prices.end(), 150.0);
auto upper = std::upper_bound(prices.begin(), prices.end(), 150.0);
```

### Numerical Algorithms
```cpp
#include <numeric>

std::vector<double> returns = {0.01, 0.02, -0.01, 0.03, 0.01};

// Sum
double total_return = std::accumulate(returns.begin(), returns.end(), 0.0);

// Inner product (dot product)
std::vector<double> weights = {0.2, 0.3, 0.1, 0.2, 0.2};
double portfolio_return = std::inner_product(returns.begin(), returns.end(),
                                              weights.begin(), 0.0);

// Partial sum (cumulative)
std::vector<double> cumulative;
std::partial_sum(returns.begin(), returns.end(),
                 std::back_inserter(cumulative));

// Adjacent difference
std::vector<double> differences;
std::adjacent_difference(prices.begin(), prices.end(),
                         std::back_inserter(differences));
```

### Custom Comparators
```cpp
// Sort by absolute value
std::sort(prices.begin(), prices.end(),
          [](double a, double b) { return std::abs(a) < std::abs(b); });

// Sort structs
struct Trade {
    std::string symbol;
    double price;
    int volume;
};

std::vector<Trade> trades;
std::sort(trades.begin(), trades.end(),
          [](const Trade& a, const Trade& b) {
              return a.price * a.volume > b.price * b.volume;  // Sort by value
          });
```

---

## Template Metaprogramming

### SFINAE (Substitution Failure Is Not An Error)
```cpp
#include <type_traits>

// Enable function only if type is arithmetic
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type
compute_mean(const std::vector<T>& data) {
    return std::accumulate(data.begin(), data.end(), T{}) / data.size();
}

// C++20: Concepts (better than SFINAE)
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<Arithmetic T>
T compute_mean_v2(const std::vector<T>& data) {
    return std::accumulate(data.begin(), data.end(), T{}) / data.size();
}
```

### Type Traits
```cpp
#include <type_traits>

// Check types at compile time
static_assert(std::is_integral_v<int>);
static_assert(std::is_floating_point_v<double>);
static_assert(std::is_same_v<int, int>);

// Remove references, const, etc.
using T1 = std::remove_reference_t<int&>;        // int
using T2 = std::remove_const_t<const int>;       // int
using T3 = std::decay_t<int[10]>;                // int*

// Enable/disable based on type
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value>::type
process(T value) {
    // Only works for arithmetic types
}
```

### Variadic Templates
```cpp
// Variable number of template arguments
template<typename... Args>
void log(Args... args) {
    (std::cout << ... << args) << std::endl;  // C++17 fold expression
}

// Perfect forwarding variadic
template<typename... Args>
auto make_vector(Args&&... args) {
    return std::vector{std::forward<Args>(args)...};
}

// Usage
auto vec = make_vector(1, 2, 3, 4, 5);
```

### Expression Templates (Advanced)
```cpp
// Lazy evaluation for numerical computations
template<typename E>
class VectorExpression {
public:
    double operator[](size_t i) const {
        return static_cast<const E&>(*this)[i];
    }
    size_t size() const {
        return static_cast<const E&>(*this).size();
    }
};

template<typename E1, typename E2>
class VectorSum : public VectorExpression<VectorSum<E1, E2>> {
    const E1& e1;
    const E2& e2;
public:
    VectorSum(const E1& a, const E2& b) : e1(a), e2(b) {}
    double operator[](size_t i) const { return e1[i] + e2[i]; }
    size_t size() const { return e1.size(); }
};

// Usage: No temporary vectors created!
// auto result = vec1 + vec2 + vec3;  // Computed element-by-element
```

---

## Modern C++20 Features

### Concepts
```cpp
#include <concepts>

// Define concept
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// Use concept
template<Numeric T>
T add(T a, T b) {
    return a + b;
}

// Requires clause
template<typename T>
requires std::totally_ordered<T>
T max(T a, T b) {
    return a > b ? a : b;
}
```

### Ranges (C++20)
```cpp
#include <ranges>
#include <vector>
#include <algorithm>

std::vector<double> prices = {100, 150, 120, 200, 180, 90, 250};

// Filter and transform in one pipeline
auto result = prices
    | std::views::filter([](double p) { return p > 150; })
    | std::views::transform([](double p) { return p * 1.1; });

// Convert to vector
std::vector<double> filtered(std::begin(result), std::end(result));

// Range algorithms
std::ranges::sort(prices);
auto it = std::ranges::find(prices, 150.0);
```

### Coroutines (C++20)
```cpp
#include <coroutine>
#include <generator>  // C++23, but concept shown

// Generator for lazy sequences
template<typename T>
generator<T> fibonacci() {
    T a = 0, b = 1;
    while (true) {
        co_yield a;
        auto temp = a + b;
        a = b;
        b = temp;
    }
}

// Usage
for (auto val : fibonacci<int>()) {
    if (val > 1000) break;
    std::cout << val << " ";
}
```

### Modules (C++20)
```cpp
// math.cppm (module)
export module math;

export double add(double a, double b) {
    return a + b;
}

// main.cpp
import math;

int main() {
    double result = add(1.5, 2.5);
    return 0;
}
```

---

## Performance Optimization

### Cache-Friendly Code
```cpp
// ❌ BAD: Cache-unfriendly (column-major access)
for (int j = 0; j < cols; ++j) {
    for (int i = 0; i < rows; ++i) {
        matrix[i][j] *= 2.0;  // Jumping around memory
    }
}

// ✅ GOOD: Cache-friendly (row-major access)
for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
        matrix[i][j] *= 2.0;  // Sequential memory access
    }
}
```

### Memory Alignment
```cpp
#include <cstddef>

// Align data structures for cache lines (64 bytes)
struct alignas(64) CacheLineAligned {
    double data[8];  // 8 * 8 = 64 bytes
};

// Avoid false sharing in multi-threaded code
struct alignas(64) ThreadLocalData {
    double local_sum;  // Each thread gets its own cache line
    char padding[64 - sizeof(double)];
};
```

### Prefetching
```cpp
#include <xmmintrin.h>  // SSE prefetch

// Prefetch next cache line
for (size_t i = 0; i < size; ++i) {
    _mm_prefetch((char*)(data + i + 8), _MM_HINT_T0);  // Prefetch 8 elements ahead
    process(data[i]);
}
```

### Compiler Optimizations
```cpp
// Mark functions for inlining
inline double fast_compute(double x) {
    return x * x + 2 * x + 1;
}

// Mark as likely/unlikely branches
if (likely(condition)) {  // GCC/Clang extension
    // Most common path
}

// Restrict pointer (no aliasing)
void compute(double* __restrict__ a, double* __restrict__ b, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] = b[i] * 2.0;  // Compiler can optimize better
    }
}
```

### SIMD (Single Instruction Multiple Data)
```cpp
#include <immintrin.h>  // AVX

// Process 4 doubles at once (AVX)
void vectorized_add(const double* a, const double* b, double* c, int n) {
    for (int i = 0; i < n; i += 4) {
        __m256d va = _mm256_load_pd(&a[i]);
        __m256d vb = _mm256_load_pd(&b[i]);
        __m256d vc = _mm256_add_pd(va, vb);
        _mm256_store_pd(&c[i], vc);
    }
}
```

---

## Numerical Computing

### Floating Point Precision
```cpp
#include <limits>
#include <cmath>

// Machine epsilon
double eps = std::numeric_limits<double>::epsilon();

// Compare floating point numbers
bool approximately_equal(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

// Relative error
double relative_error(double computed, double exact) {
    return std::abs(computed - exact) / std::abs(exact);
}
```

### Numerical Stability
```cpp
// ❌ UNSTABLE: Subtracting similar numbers
double unstable = std::sqrt(x + 1.0) - std::sqrt(x);

// ✅ STABLE: Rationalize
double stable = 1.0 / (std::sqrt(x + 1.0) + std::sqrt(x));

// Kahan summation (compensated summation)
double kahan_sum(const std::vector<double>& values) {
    double sum = 0.0;
    double c = 0.0;  // Compensation
    for (double val : values) {
        double y = val - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    return sum;
}
```

### Matrix Operations
```cpp
// Use specialized libraries (Eigen, Armadillo) for production
// But here's a simple example:

class Matrix {
    std::vector<double> data;
    size_t rows, cols;
    
public:
    Matrix(size_t r, size_t c) : rows(r), cols(c), data(r * c) {}
    
    double& operator()(size_t i, size_t j) { return data[i * cols + j]; }
    const double& operator()(size_t i, size_t j) const { return data[i * cols + j]; }
    
    // Matrix multiplication (cache-friendly)
    Matrix operator*(const Matrix& other) const {
        Matrix result(rows, other.cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t k = 0; k < cols; ++k) {
                double temp = (*this)(i, k);
                for (size_t j = 0; j < other.cols; ++j) {
                    result(i, j) += temp * other(k, j);
                }
            }
        }
        return result;
    }
};
```

---

## Low-Latency Programming

### Avoid Dynamic Allocation
```cpp
// ❌ BAD: Heap allocation in hot path
void process_trade() {
    std::vector<double> prices(1000);  // Heap allocation
    // ...
}

// ✅ GOOD: Stack allocation or pre-allocated
void process_trade() {
    double prices[1000];  // Stack allocation
    // or use pre-allocated buffer from pool
}
```

### Memory Pools
```cpp
template<typename T, size_t PoolSize>
class MemoryPool {
    alignas(T) char pool[PoolSize * sizeof(T)];
    bool used[PoolSize];
    
public:
    T* allocate() {
        for (size_t i = 0; i < PoolSize; ++i) {
            if (!used[i]) {
                used[i] = true;
                return reinterpret_cast<T*>(pool + i * sizeof(T));
            }
        }
        return nullptr;
    }
    
    void deallocate(T* ptr) {
        size_t index = (reinterpret_cast<char*>(ptr) - pool) / sizeof(T);
        used[index] = false;
    }
};
```

### Lock-Free Data Structures
```cpp
#include <atomic>

// Lock-free queue (simplified)
template<typename T>
class LockFreeQueue {
    struct Node {
        std::atomic<T*> data;
        std::atomic<Node*> next;
    };
    
    std::atomic<Node*> head;
    std::atomic<Node*> tail;
    
public:
    void enqueue(T item) {
        Node* node = new Node;
        node->data.store(new T(item));
        node->next.store(nullptr);
        
        Node* prev_tail = tail.exchange(node);
        prev_tail->next.store(node);
    }
    
    bool dequeue(T& result) {
        Node* head_node = head.load();
        Node* next = head_node->next.load();
        if (next == nullptr) return false;
        
        T* data = next->data.load();
        head.store(next);
        result = *data;
        delete data;
        delete head_node;
        return true;
    }
};
```

### Branch Prediction
```cpp
// Mark likely branches
if (likely(price > threshold)) {  // GCC/Clang
    // Most common case
}

// Use lookup tables for expensive functions
constexpr size_t TABLE_SIZE = 1000;
double sin_table[TABLE_SIZE];

void init_table() {
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        sin_table[i] = std::sin(2.0 * M_PI * i / TABLE_SIZE);
    }
}

double fast_sin(double x) {
    size_t index = static_cast<size_t>(x * TABLE_SIZE / (2.0 * M_PI)) % TABLE_SIZE;
    return sin_table[index];
}
```

---

## Best Practices for Quant Code

### 1. Use Fixed-Point or Decimal for Money
```cpp
// ❌ BAD: Floating point for money
double price = 100.50;  // Precision issues!

// ✅ GOOD: Use integer cents or decimal library
int price_cents = 10050;  // Represent as cents
// or use boost::multiprecision::cpp_dec_float
```

### 2. Prefer `constexpr` for Constants
```cpp
constexpr double PI = 3.141592653589793238;
constexpr int MAX_POSITIONS = 10000;

// Computed at compile time
constexpr double area = PI * 10.0 * 10.0;
```

### 3. Use `noexcept` for Performance
```cpp
// Mark functions that don't throw
double compute_price(double spot, double strike) noexcept {
    return std::max(spot - strike, 0.0);
}
```

### 4. Reserve Vector Capacity
```cpp
std::vector<double> prices;
prices.reserve(10000);  // Avoid reallocations
for (int i = 0; i < 10000; ++i) {
    prices.push_back(get_price(i));
}
```

### 5. Use `emplace_back` Instead of `push_back`
```cpp
// ✅ GOOD: Constructs in place
prices.emplace_back(100.5);

// ❌ LESS EFFICIENT: Creates temporary
prices.push_back(100.5);
```

### 6. Profile Before Optimizing
```cpp
// Use tools like:
// - perf (Linux)
// - Intel VTune
// - Valgrind
// - Google Benchmark library
```

### 7. Use Specialized Libraries
```cpp
// For production quant code, use:
// - Eigen: Linear algebra
// - Boost: Various utilities
// - Intel MKL: Math kernel library
// - QuantLib: Financial mathematics
```

---

## Summary

For quantitative C++ development, focus on:

1. **Performance**: Move semantics, cache-friendly code, SIMD
2. **Concurrency**: Threading, atomics, lock-free structures
3. **STL Mastery**: Know container complexities, use right tool
4. **Modern C++**: Concepts, ranges, coroutines (C++20)
5. **Numerical Stability**: Understand floating point, use stable algorithms
6. **Low Latency**: Avoid allocations, use memory pools, lock-free code
7. **Profiling**: Measure before optimizing

**Key Libraries for Quant:**
- **Eigen**: Matrix operations
- **Boost**: Utilities, math, threading
- **QuantLib**: Financial instruments
- **Intel TBB**: Parallel algorithms
- **Google Benchmark**: Performance testing

---

**Happy Quant Coding! 📈💻**

