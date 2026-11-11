// 03-queues-buffers  --  stubs for ring buffers, deques and monotonic queues.
//
// The classes keep LeetCode's method names so you can paste them into the
// judge. Private members are declared here so the *.cpp stays the only place
// you edit; feel free to change them while practising.
#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace lc {

// 622  Design Circular Queue.  Context: fixed-capacity RX ring buffer between a DMA ISR and a decoder thread.
class MyCircularQueue {
public:
    explicit MyCircularQueue(int k);
    bool enQueue(int value);   // false if full
    bool deQueue();            // false if empty
    int Front() const;         // -1 if empty
    int Rear() const;          // -1 if empty
    bool isEmpty() const;
    bool isFull() const;

private:
    std::vector<int> buf_;
    std::size_t head_ = 0;  // index of the oldest element
    std::size_t size_ = 0;  // number of stored elements (disambiguates full vs empty)
};

// 641  Design Circular Deque.  Context: retransmission window that can push at both ends (priority re-insert at front).
class MyCircularDeque {
public:
    explicit MyCircularDeque(int k);
    bool insertFront(int value);
    bool insertLast(int value);
    bool deleteFront();
    bool deleteLast();
    int getFront() const;  // -1 if empty
    int getRear() const;   // -1 if empty
    bool isEmpty() const;
    bool isFull() const;

private:
    std::vector<int> buf_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

// 232  Implement Queue using Stacks.  Context: FIFO built from two LIFO stacks (e.g. only a stack allocator available).
class MyQueue {
public:
    void push(int x);
    int pop();         // precondition: not empty
    int peek();        // precondition: not empty
    bool empty() const;

private:
    void refill();
    std::vector<int> in_, out_;
};

// 239  Sliding Window Maximum.  Context: peak RSSI over the last k samples, O(1) amortised per sample.
std::vector<int> max_sliding_window(const std::vector<int>& nums, int k);

// 346  Moving Average from Data Stream.  Context: smoothed link-quality / SNR estimate over the last `size` samples.
class MovingAverage {
public:
    explicit MovingAverage(int size);
    double next(int val);

private:
    std::vector<int> ring_;
    std::size_t head_ = 0;   // next slot to overwrite
    std::size_t count_ = 0;
    long long sum_ = 0;
};

}  // namespace lc
