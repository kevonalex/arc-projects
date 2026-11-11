#include "design.hpp"

#include <algorithm>

namespace lc {

// ---- 146 -----------------------------------------------------------------
// Invariant: index_ and order_ describe the same set of keys; order_ is sorted by recency.
LRUCache::LRUCache(int capacity) : cap_(static_cast<std::size_t>(capacity)) {}

int LRUCache::get(int key) {
    const auto it = index_.find(key);
    if (it == index_.end()) return -1;
    order_.splice(order_.begin(), order_, it->second);  // O(1), iterators stay valid
    return it->second->second;
}

void LRUCache::put(int key, int value) {
    const auto it = index_.find(key);
    if (it != index_.end()) {
        it->second->second = value;
        order_.splice(order_.begin(), order_, it->second);
        return;
    }
    if (cap_ == 0) return;
    if (order_.size() == cap_) {
        index_.erase(order_.back().first);
        order_.pop_back();
    }
    order_.emplace_front(key, value);
    index_[key] = order_.begin();
}

// ---- 359 -----------------------------------------------------------------
bool Logger::shouldPrintMessage(int timestamp, const std::string& message) {
    auto it = next_allowed_.find(message);
    if (it != next_allowed_.end() && timestamp < it->second) return false;
    next_allowed_[message] = timestamp + 10;
    return true;
}

// ---- 362 -----------------------------------------------------------------
// bucket i holds hits for the second `stamp_[i]` with stamp_[i] % 300 == i.
// A stale stamp means the bucket belongs to an expired second.
HitCounter::HitCounter() : stamp_(kWindow, -1), count_(kWindow, 0) {}

void HitCounter::hit(int timestamp) {
    const auto i = static_cast<std::size_t>(timestamp % kWindow);
    if (stamp_[i] != timestamp) {
        stamp_[i] = timestamp;
        count_[i] = 0;
    }
    ++count_[i];
}

int HitCounter::getHits(int timestamp) {
    int total = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(kWindow); ++i) {
        if (stamp_[i] >= 0 && timestamp - stamp_[i] < kWindow) total += count_[i];
    }
    return total;
}

// ---- 705 -----------------------------------------------------------------
MyHashSet::MyHashSet() : table_(kBuckets) {}

void MyHashSet::add(int key) {
    auto& b = table_[static_cast<std::size_t>(key) % kBuckets];
    if (std::find(b.begin(), b.end(), key) == b.end()) b.push_back(key);
}
void MyHashSet::remove(int key) { table_[static_cast<std::size_t>(key) % kBuckets].remove(key); }
bool MyHashSet::contains(int key) const {
    const auto& b = table_[static_cast<std::size_t>(key) % kBuckets];
    return std::find(b.begin(), b.end(), key) != b.end();
}

// ---- 706 -----------------------------------------------------------------
MyHashMap::MyHashMap() : table_(kBuckets) {}

void MyHashMap::put(int key, int value) {
    auto& b = table_[static_cast<std::size_t>(key) % kBuckets];
    for (auto& kv : b) {
        if (kv.first == key) {
            kv.second = value;
            return;
        }
    }
    b.emplace_back(key, value);
}
int MyHashMap::get(int key) const {
    const auto& b = table_[static_cast<std::size_t>(key) % kBuckets];
    for (const auto& kv : b)
        if (kv.first == key) return kv.second;
    return -1;
}
void MyHashMap::remove(int key) {
    auto& b = table_[static_cast<std::size_t>(key) % kBuckets];
    b.remove_if([key](const std::pair<int, int>& kv) { return kv.first == key; });
}

// ---- 155 -----------------------------------------------------------------
// Each frame stores the minimum of everything at or below it, so pop restores the old min for free.
void MinStack::push(int val) {
    const int m = st_.empty() ? val : std::min(val, st_.back().second);
    st_.emplace_back(val, m);
}
void MinStack::pop() { st_.pop_back(); }
int MinStack::top() const { return st_.back().first; }
int MinStack::getMin() const { return st_.back().second; }

}  // namespace lc
