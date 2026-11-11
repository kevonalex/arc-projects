// rtos_sim.hpp  --  REFERENCE SOLUTION for Pack 08/03 (scheduler policy, priority inheritance, RTOS primitives).
#pragma once
#include <cstddef>
#include <deque>

#include "../common/rtos_types.hpp"

namespace rtos {

// ------------------------------------------------------------------ priority inheritance (W 3.5 - 3.6)
// Effective priority = base priority, raised (only if sc.inheritance) to the highest effective priority of any task
// blocked on a mutex this task owns.  Recursion makes the inheritance transitive; depth-limited against deadlock cycles.
inline int effective_priority(const Sched& sc, int task, int depth = 0) {
  int eff = sc.t[static_cast<size_t>(task)].prio;
  if (!sc.inheritance || depth > 16) return eff;
  for (size_t m = 0; m < sc.owner.size(); ++m) {
    if (sc.owner[m] != task) continue;
    for (size_t j = 0; j < sc.t.size(); ++j) {
      if (sc.t[j].blocked_on == static_cast<int>(m) && !sc.t[j].done) {
        const int e = effective_priority(sc, static_cast<int>(j), depth + 1);
        if (e > eff) eff = e;
      }
    }
  }
  return eff;
}
// Index of the ready task with the highest effective priority (ties: lowest index); -1 if none (idle).
inline int pick_next(const Sched& sc, int now) {
  int best = -1, best_p = -1;
  for (size_t i = 0; i < sc.t.size(); ++i) {
    if (!ready(sc.t[i], now)) continue;
    const int p = effective_priority(sc, static_cast<int>(i));
    if (best < 0 || p > best_p) {
      best = static_cast<int>(i);
      best_p = p;
    }
  }
  return best;
}

}  // namespace rtos

#include "../common/rtos_engine.hpp"

namespace rtos {

// ------------------------------------------------------------------ primitives (W 3.2 - 3.4)
// Counting semaphore: no ownership; ANY task (or ISR) may give. give() fails when already at max (binary: max = 1).
class Semaphore {
 public:
  Semaphore(int initial, int max) : count_(initial), max_(max) {}
  bool give() {
    if (count_ >= max_) return false;
    ++count_;
    return true;
  }
  bool take() {  // non-blocking (timeout 0)
    if (count_ <= 0) return false;
    --count_;
    return true;
  }
  int count() const { return count_; }

 private:
  int count_, max_;
};
// Mutex: has an OWNER; only the owner may unlock; non-recursive (a second lock by the owner fails).
class Mutex {
 public:
  bool lock(int task) {
    if (owner_ >= 0) return false;
    owner_ = task;
    return true;
  }
  bool unlock(int task) {
    if (owner_ != task) return false;
    owner_ = -1;
    return true;
  }
  int owner() const { return owner_; }

 private:
  int owner_ = -1;
};
// Bounded FIFO queue, copy-in/copy-out like xQueueSend/xQueueReceive; non-blocking.
template <class T>
class BoundedQueue {
 public:
  explicit BoundedQueue(size_t cap) : cap_(cap) {}
  bool send(const T& v) {
    if (q_.size() >= cap_) return false;
    q_.push_back(v);
    return true;
  }
  bool receive(T& out) {
    if (q_.empty()) return false;
    out = q_.front();
    q_.pop_front();
    return true;
  }
  size_t size() const { return q_.size(); }

 private:
  size_t cap_;
  std::deque<T> q_;
};

}  // namespace rtos
