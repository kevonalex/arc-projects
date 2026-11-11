// rtos_policy.hpp  --  the finished scheduler policy from Pack 08/03 (GIVEN here: reuse it, don't re-derive it).
// Pulls in the types, the policy (priority inheritance, pick_next) and the tick engine.
#pragma once
#include "rtos_types.hpp"

namespace rtos {

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

#include "rtos_engine.hpp"
