#include "queues_buffers.hpp"

namespace lc {

// ---- 622 -----------------------------------------------------------------
// Invariant: elements occupy slots head_, head_+1, ... head_+size_-1 (mod cap).
MyCircularQueue::MyCircularQueue(int k) : buf_(static_cast<std::size_t>(k)) {}

bool MyCircularQueue::enQueue(int value) {
    if (isFull()) return false;
    buf_[(head_ + size_) % buf_.size()] = value;
    ++size_;
    return true;
}
bool MyCircularQueue::deQueue() {
    if (isEmpty()) return false;
    head_ = (head_ + 1) % buf_.size();
    --size_;
    return true;
}
int MyCircularQueue::Front() const { return isEmpty() ? -1 : buf_[head_]; }
int MyCircularQueue::Rear() const {
    return isEmpty() ? -1 : buf_[(head_ + size_ - 1) % buf_.size()];
}
bool MyCircularQueue::isEmpty() const { return size_ == 0; }
bool MyCircularQueue::isFull() const { return size_ == buf_.size(); }

// ---- 641 -----------------------------------------------------------------
MyCircularDeque::MyCircularDeque(int k) : buf_(static_cast<std::size_t>(k)) {}

bool MyCircularDeque::insertFront(int value) {
    if (isFull()) return false;
    head_ = (head_ + buf_.size() - 1) % buf_.size();  // step back without going negative
    buf_[head_] = value;
    ++size_;
    return true;
}
bool MyCircularDeque::insertLast(int value) {
    if (isFull()) return false;
    buf_[(head_ + size_) % buf_.size()] = value;
    ++size_;
    return true;
}
bool MyCircularDeque::deleteFront() {
    if (isEmpty()) return false;
    head_ = (head_ + 1) % buf_.size();
    --size_;
    return true;
}
bool MyCircularDeque::deleteLast() {
    if (isEmpty()) return false;
    --size_;
    return true;
}
int MyCircularDeque::getFront() const { return isEmpty() ? -1 : buf_[head_]; }
int MyCircularDeque::getRear() const {
    return isEmpty() ? -1 : buf_[(head_ + size_ - 1) % buf_.size()];
}
bool MyCircularDeque::isEmpty() const { return size_ == 0; }
bool MyCircularDeque::isFull() const { return size_ == buf_.size(); }

// ---- 232 -----------------------------------------------------------------
// Amortised O(1): every element is moved from in_ to out_ at most once.
void MyQueue::push(int x) { in_.push_back(x); }
void MyQueue::refill() {
    if (!out_.empty()) return;  // only refill when out_ is EMPTY, or order breaks
    while (!in_.empty()) {
        out_.push_back(in_.back());
        in_.pop_back();
    }
}
int MyQueue::pop() {
    refill();
    const int v = out_.back();
    out_.pop_back();
    return v;
}
int MyQueue::peek() {
    refill();
    return out_.back();
}
bool MyQueue::empty() const { return in_.empty() && out_.empty(); }

// ---- 239 -----------------------------------------------------------------
// Deque of indices; values at those indices are strictly decreasing front->back.
// Front is always the window maximum.
std::vector<int> max_sliding_window(const std::vector<int>& nums, int k) {
    std::vector<int> out;
    if (k <= 0 || nums.empty()) return out;
    std::deque<int> dq;
    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        // 1. expire: the front index fell out of [i-k+1, i]
        if (!dq.empty() && dq.front() <= i - k) dq.pop_front();
        // 2. dominate: anything not larger than the new element can never be a max again
        while (!dq.empty() && nums[static_cast<std::size_t>(dq.back())] <= nums[static_cast<std::size_t>(i)]) {
            dq.pop_back();
        }
        dq.push_back(i);
        // 3. report once the first window is full
        if (i >= k - 1) out.push_back(nums[static_cast<std::size_t>(dq.front())]);
    }
    return out;
}

// ---- 346 -----------------------------------------------------------------
MovingAverage::MovingAverage(int size) : ring_(static_cast<std::size_t>(size), 0) {}

double MovingAverage::next(int val) {
    sum_ -= ring_[head_];  // evict oldest (0 while still filling)
    ring_[head_] = val;
    sum_ += val;
    head_ = (head_ + 1) % ring_.size();
    if (count_ < ring_.size()) ++count_;
    return static_cast<double>(sum_) / static_cast<double>(count_);
}

}  // namespace lc
