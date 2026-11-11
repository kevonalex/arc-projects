// rtos_engine.hpp  --  the tick loop (GIVEN). Include AFTER effective_priority() and pick_next() are declared.
// Each tick: ask pick_next() who runs. Zero-time steps (Lock, Unlock, Delay) are executed immediately and the
// scheduler is asked again; a Run step consumes exactly one tick.
#pragma once
#include "rtos_types.hpp"

namespace rtos {

inline Result simulate(const std::vector<TaskSpec>& specs, int nmutex, bool inheritance, int max_ticks) {
  Sched sc;
  sc.inheritance = inheritance;
  sc.owner.assign(static_cast<size_t>(nmutex), -1);
  for (const TaskSpec& sp : specs) {
    TaskState s;
    s.prio = sp.prio;
    s.start = sp.start;
    sc.t.push_back(s);
  }
  Result r;
  r.finish.assign(specs.size(), -1);
  for (int now = 0; now < max_ticks; ++now) {
    bool all_done = true;
    for (const TaskState& s : sc.t) all_done = all_done && s.done;
    if (all_done) break;
    bool ran = false;
    for (int guard = 0; guard < 256 && !ran; ++guard) {
      const int i = pick_next(sc, now);
      if (i < 0) break;
      TaskState& s = sc.t[static_cast<size_t>(i)];
      const std::vector<Step>& steps = specs[static_cast<size_t>(i)].steps;
      if (s.pc >= steps.size()) {
        s.done = true;
        r.finish[static_cast<size_t>(i)] = now;
        continue;
      }
      const Step& st = steps[s.pc];
      switch (st.op) {
        case Op::Run:
          if (s.remaining < 0) s.remaining = st.arg;
          --s.remaining;
          if (s.remaining <= 0) {
            s.remaining = -1;
            ++s.pc;
            if (s.pc >= steps.size()) {
              s.done = true;
              r.finish[static_cast<size_t>(i)] = now + 1;
            }
          }
          r.trace.push_back(i);
          ran = true;
          break;
        case Op::Lock:
          if (sc.owner[static_cast<size_t>(st.arg)] < 0) {
            sc.owner[static_cast<size_t>(st.arg)] = i;
            ++s.pc;
          } else {
            s.blocked_on = st.arg;  // wait; the scheduler is asked again
          }
          break;
        case Op::Unlock: {
          const size_t m = static_cast<size_t>(st.arg);
          if (sc.owner[m] == i) {
            sc.owner[m] = -1;
            int w = -1;  // hand the mutex to the highest-priority waiter (ties: lowest index)
            for (size_t j = 0; j < sc.t.size(); ++j)
              if (sc.t[j].blocked_on == st.arg && (w < 0 || sc.t[j].prio > sc.t[static_cast<size_t>(w)].prio)) w = static_cast<int>(j);
            if (w >= 0) {
              sc.owner[m] = w;
              sc.t[static_cast<size_t>(w)].blocked_on = -1;
              ++sc.t[static_cast<size_t>(w)].pc;  // its Lock step is now complete
            }
          }
          ++s.pc;
          if (s.pc >= steps.size()) {
            s.done = true;
            r.finish[static_cast<size_t>(i)] = now;
          }
          break;
        }
        case Op::Delay:
          s.wake = now + st.arg;
          ++s.pc;
          break;
      }
    }
    if (!ran) {
      bool finished_now = true;  // the last task completed through a zero-time step: this tick was not idle
      for (const TaskState& s : sc.t) finished_now = finished_now && s.done;
      if (finished_now) break;
      r.trace.push_back(-1);
    }
  }
  return r;
}

}  // namespace rtos
