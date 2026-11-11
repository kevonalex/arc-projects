// pipeline_trace -- the audio-pipeline scenario of build 4, printed as a tick-by-tick timeline, WITHOUT and WITH priority
// inheritance.  Built against the reference solution.   ./pipeline_trace
// (This is the deterministic stand-in for running the same three tasks on the FreeRTOS POSIX/Linux simulator port.)
#include <cstdio>
#include <string>

#include "builds.hpp"

static void show(const builds::Scenario& s, bool inheritance) {
  const rtos::Result r = rtos::simulate(s.tasks, s.nmutex, inheritance, 200);
  std::printf("\n--- priority inheritance %s ---\n", inheritance ? "ON" : "OFF");
  std::printf("tick :");
  for (size_t t = 0; t < r.trace.size(); ++t) std::printf(" %2zu", t);
  std::printf("\nruns :");
  for (int who : r.trace) std::printf(" %2c", who < 0 ? '.' : s.tasks[static_cast<size_t>(who)].name[0]);
  std::printf("\n");
  for (size_t i = 0; i < s.tasks.size(); ++i)
    std::printf("  %-13s prio %d  start %d  finish %2d  response %2d%s\n", s.tasks[i].name.c_str(), s.tasks[i].prio, s.tasks[i].start, r.finish[i],
                rtos::response_ticks(r, s.tasks, i), i == s.critical ? "   <== deadline-critical" : "");
  std::printf("  deadline %d ticks -> %s\n", s.deadline_ticks, builds::deadline_misses(s, inheritance) ? "MISSED (audio glitch / watchdog reset)" : "met");
}

int main() {
  const builds::Scenario s = builds::audio_pipeline_scenario();
  std::printf("codec_out (high) and logger (low) share mutex 0; dsp_resample (medium) is CPU-bound and takes no lock.\n");
  show(s, false);
  show(s, true);
  return 0;
}
