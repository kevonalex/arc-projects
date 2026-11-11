#pragma once
// STUB - worksheet 03, Q7/Q8 : the voice superframe, bursts A..F, embedded signalling, late entry.
// SYNTHETIC ONLY. VERIFY against ETSI TS 102 361-1 (burst and frame structure).
#include <cstdint>

#include "lmr/todo.hpp"

namespace dmr {

// One voice superframe = six consecutive voice bursts of ONE logical channel (timeslot): A B C D E F.
enum class VoiceBurst : std::uint8_t { A = 0, B, C, D, E, F };

inline constexpr int kBurstsPerSuperframe = 6;
inline constexpr int kBurstPeriodMs = 60;  // same slot repeats every 2 x 30 ms (2-slot TDMA)

// TODO: letter of the n-th voice burst of a call (n = 0 is the first burst, which is A).
inline VoiceBurst burst_letter(std::uint64_t /*n*/) { lmr::todo("burst_letter"); }

// TODO: superframe duration in ms = 6 bursts * 60 ms (360).
inline int superframe_ms() { lmr::todo("superframe_ms"); }

// TODO: only burst A carries the 48-bit voice SYNC in its middle.
inline bool has_voice_sync(VoiceBurst /*b*/) { lmr::todo("has_voice_sync"); }

// TODO: bursts B, C, D, E each carry a 32-bit fragment of the embedded link control
//       (4 x 32 = 128 bits); F carries other embedded signalling (null / reverse channel, VERIFY).
inline bool carries_embedded_lc(VoiceBurst /*b*/) { lmr::todo("carries_embedded_lc"); }

// TODO: bits available for embedded link control per superframe =
//       (number of LC-carrying bursts) * 32  -> 128.
inline int embedded_lc_capacity_bits() { lmr::todo("embedded_lc_capacity_bits"); }

// TODO: worst-case time (ms) for a receiver that joins mid-call to collect ALL fragments of one
//       embedded LC, if it must see the first fragment and then the others in order:
//         it has just missed the first fragment:  wait `cycle_bursts * burst_ms`
//         then collect the remaining `fragments - 1`:  (fragments - 1) * burst_ms
//       (measure between the centres of the bursts; defaults: 4 fragments, 60 ms, cycle of 6.)
inline int late_entry_worst_case_ms(int /*fragments*/ = 4, int /*burst_ms*/ = kBurstPeriodMs,
                                    int /*cycle_bursts*/ = kBurstsPerSuperframe) {
  lmr::todo("late_entry_worst_case_ms");
}

// TODO: AMBE+2 frames in one superframe (6 bursts * 3 frames) -> 18; times 20 ms = 360 ms.
inline int voice_frames_per_superframe() { lmr::todo("voice_frames_per_superframe"); }

}  // namespace dmr
