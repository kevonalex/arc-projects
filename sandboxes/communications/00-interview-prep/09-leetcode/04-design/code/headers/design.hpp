// 04-design  --  stubs for "combine two structures to get O(1)" designs.
//
// Premium problems (359, 362) are included because they map 1:1 onto
// protocol rate limiting / sliding-window counters.
#pragma once

#include <cstddef>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lc {

// 146  LRU Cache.  Context: TEID / RNTI -> context cache with bounded memory; evict the coldest UE context.
class LRUCache {
public:
    explicit LRUCache(int capacity);
    int get(int key);               // -1 if absent; a hit makes key most-recently-used
    void put(int key, int value);   // insert/update; evict LRU when over capacity

private:
    using Entry = std::pair<int, int>;  // key, value
    std::size_t cap_;
    std::list<Entry> order_;  // front = most recently used
    std::unordered_map<int, std::list<Entry>::iterator> index_;
};

// 359  Logger Rate Limiter.  Context: suppress the same alarm text more than once per 10 s.
class Logger {
public:
    bool shouldPrintMessage(int timestamp, const std::string& message);

private:
    std::unordered_map<std::string, int> next_allowed_;
};

// 362  Design Hit Counter.  Context: packets-per-5-minutes counter with O(1) memory (300 one-second buckets).
class HitCounter {
public:
    HitCounter();
    void hit(int timestamp);
    int getHits(int timestamp);  // hits in (timestamp-300, timestamp]

private:
    static constexpr int kWindow = 300;
    std::vector<int> stamp_;
    std::vector<int> count_;
};

// 705  Design HashSet.  Context: the dedup table for received sequence numbers.
class MyHashSet {
public:
    MyHashSet();
    void add(int key);
    void remove(int key);
    bool contains(int key) const;

private:
    static constexpr std::size_t kBuckets = 769;  // prime
    std::vector<std::list<int>> table_;
};

// 706  Design HashMap.  Context: session-id -> session state with separate chaining.
class MyHashMap {
public:
    MyHashMap();
    void put(int key, int value);
    int get(int key) const;  // -1 if absent
    void remove(int key);

private:
    static constexpr std::size_t kBuckets = 769;
    std::vector<std::list<std::pair<int, int>>> table_;
};

// 155  Min Stack.  Context: running minimum RTT over an undo-able stack of measurements.
class MinStack {
public:
    void push(int val);
    void pop();
    int top() const;
    int getMin() const;

private:
    std::vector<std::pair<int, int>> st_;  // value, min-so-far
};

}  // namespace lc
