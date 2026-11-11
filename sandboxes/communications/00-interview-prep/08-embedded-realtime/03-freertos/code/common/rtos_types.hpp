// rtos_types.hpp  --  data types shared by the stub and the solution of Pack 08/03 (GIVEN, not a stub).
// A tiny deterministic model of a preemptive fixed-priority scheduler (FreeRTOS-like: higher number = higher priority).
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace rtos {

enum class Op { Run, Lock, Unlock, Delay };
// Run n ticks of CPU | Lock mutex id | Unlock mutex id | sleep for n ticks.
struct Step {
  Op op;
  int arg;
};
struct TaskSpec {
  std::string name;
  int prio;   // base priority; HIGHER number = more urgent (FreeRTOS convention)
  int start;  // tick at which the task is released
  std::vector<Step> steps;
};
struct TaskState {
  int prio = 0;
  int start = 0;
  size_t pc = 0;          // index of the current step
  int remaining = -1;     // ticks left in the current Run step (-1 = not started)
  int blocked_on = -1;    // mutex id this task is waiting for, or -1
  int wake = 0;           // tick at which a Delay ends
  bool done = false;
};
struct Sched {
  std::vector<TaskState> t;
  std::vector<int> owner;  // owner[m] = task index holding mutex m, or -1
  bool inheritance = false;
};
inline bool ready(const TaskState& s, int now) { return !s.done && now >= s.start && now >= s.wake && s.blocked_on < 0; }

struct Result {
  std::vector<int> trace;   // task index that used the CPU in each tick, -1 = idle
  std::vector<int> finish;  // tick at which each task completed, -1 if it never did
};
inline int response_ticks(const Result& r, const std::vector<TaskSpec>& specs, size_t i) {
  return r.finish[i] < 0 ? -1 : r.finish[i] - specs[i].start;
}

}  // namespace rtos
