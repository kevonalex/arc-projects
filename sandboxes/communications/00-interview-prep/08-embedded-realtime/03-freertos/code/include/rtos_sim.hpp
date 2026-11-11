// rtos_sim.hpp  --  STUBS for Pack 08/03 (student version).  Reference: ../solutions/rtos_sim.hpp
// You implement the scheduler POLICY and the primitives; the tick loop lives in ../common/rtos_engine.hpp (given).
#pragma once
#include <cstddef>
#include <deque>

#include "../common/rtos_types.hpp"

namespace rtos {

// ---- priority inheritance (W 3.5 - 3.6) ----
// base priority; if sc.inheritance: raised to the highest effective priority of any task BLOCKED on a mutex this task
// owns (sc.owner[m] == task, other.blocked_on == m, other not done).  Make it transitive (recursion, depth limit 16).
inline int effective_priority([[maybe_unused]] const Sched& sc, [[maybe_unused]] int task, [[maybe_unused]] int depth = 0) {
  return -1;  // TODO W 3.5
}
// Index of the ready (see ready()) task with the highest effective priority; ties -> lowest index; -1 if none.
inline int pick_next([[maybe_unused]] const Sched& sc, [[maybe_unused]] int now) {
  return -2;  // TODO W 3.5
}

}  // namespace rtos

#include "../common/rtos_engine.hpp"

namespace rtos {

// ---- primitives (W 3.2 - 3.4) ----
// Counting semaphore: NO owner; any task may give. give() fails (false) at max, take() fails at 0. Non-blocking.
class Semaphore {
 public:
  Semaphore(int initial, int max) : count_(initial), max_(max) {}
  bool give() { return true; }  // TODO W 3.3
  bool take() { return true; }  // TODO W 3.3
  int count() const { return -1; }  // TODO W 3.3
 private:
  [[maybe_unused]] int count_, max_;
};
// Mutex: has an OWNER (task id >= 0); only the owner may unlock; non-recursive (owner locking again fails).
class Mutex {
 public:
  bool lock([[maybe_unused]] int task) { return true; }    // TODO W 3.4
  bool unlock([[maybe_unused]] int task) { return true; }  // TODO W 3.4
  int owner() const { return -5; }                          // TODO W 3.4 (-1 when free)
};
// Bounded FIFO, copy-in/copy-out like xQueueSend/xQueueReceive, non-blocking: send fails when full, receive when empty.
template <class T>
class BoundedQueue {
 public:
  explicit BoundedQueue(size_t cap) : cap_(cap) {}
  bool send([[maybe_unused]] const T& v) { return true; }   // TODO W 3.2
  bool receive([[maybe_unused]] T& out) { return true; }    // TODO W 3.2
  size_t size() const { return 999; }                        // TODO W 3.2
 private:
  [[maybe_unused]] size_t cap_;
  [[maybe_unused]] std::deque<T> q_;
};

}  // namespace rtos
