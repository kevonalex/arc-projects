#pragma once
// STUB - worksheet 04, Q3/Q4/Q6/Q9 : TETRA TDMA structure and rates (ETSI EN 300 392-2).
// SYNTHETIC ONLY.  VERIFY every constant against EN 300 392-2 (air interface, physical layer).
#include <cstdint>

#include "lmr/todo.hpp"

namespace tetra {

inline constexpr int kSymbolRate = 18000;  // pi/4-DQPSK symbols per second
inline constexpr int kBitsPerSymbol = 2;
inline constexpr int kBitsPerSlot = 510;   // 255 symbols
inline constexpr int kSlotsPerFrame = 4;
inline constexpr int kFramesPerMultiframe = 18;       // frame 18 = control frame
inline constexpr int kMultiframesPerHyperframe = 60;
inline constexpr int kTchBlockBits = 432;             // 2 x 216 type-5 bits per normal-burst slot
inline constexpr int kSpeechFrameBits = 137;          // ACELP frame, 30 ms
inline constexpr int kSpeechFrameMs = 30;

// TODO: 18000 * 2 = 36000
inline int gross_bit_rate() { lmr::todo("gross_bit_rate"); }

// TODO: slot duration in ms = bits per slot / gross rate  (14.1667)
inline double slot_ms() { lmr::todo("slot_ms"); }

// TODO: 4 slots (56.6667 ms)
inline double frame_ms() { lmr::todo("frame_ms"); }

// TODO: 18 frames (1020 ms)
inline double multiframe_ms() { lmr::todo("multiframe_ms"); }

// TODO: 60 multiframes, in SECONDS (61.2)
inline double hyperframe_s() { lmr::todo("hyperframe_s"); }

// TODO: frames 1..17 carry traffic, frame 18 is the control frame -> 17
inline int traffic_frames_per_multiframe() { lmr::todo("traffic_frames_per_multiframe"); }

// TODO: bits of a slot that are NOT the 432 coded/user bits (training, tails, phase adjust, broadcast block...)
inline int overhead_bits_in_normal_burst() { lmr::todo("overhead_bits_in_normal_burst"); }

// TODO: net user rate of ONE timeslot carrying unprotected data (TCH/7.2):
//       432 bits in each of 17 traffic frames per multiframe, divided by the multiframe duration.
inline double tch72_net_rate() { lmr::todo("tch72_net_rate"); }

// TODO: all four timeslots (28800)
inline double net_rate_per_carrier() { lmr::todo("net_rate_per_carrier"); }

// TODO: net/gross as a fraction (0.8)
inline double net_over_gross() { lmr::todo("net_over_gross"); }

// TODO: bit rate of the ACELP speech coder before channel coding: 137 bits per 30 ms (4566.67)
inline double speech_rate_bps() { lmr::todo("speech_rate_bps"); }

// TODO: speech frames carried by one traffic slot (2) and the ms of speech they hold (60)
inline int speech_frames_per_slot() { lmr::todo("speech_frames_per_slot"); }
inline int speech_ms_per_slot() { lmr::todo("speech_ms_per_slot"); }

struct TetraTime {
  int timeslot = 1;    // 1..4
  int frame = 1;       // 1..18
  int multiframe = 1;  // 1..60
};

// TODO: from an absolute slot count n (0 = timeslot 1 of frame 1 of multiframe 1):
//       timeslot = n % 4 + 1; frame = (n / 4) % 18 + 1; multiframe = (n / 72) % 60 + 1.
inline TetraTime time_of_slot(std::uint64_t /*n*/) { lmr::todo("time_of_slot"); }

// TODO: frame 18 of every multiframe is the control frame.
inline bool is_control_frame(int /*frame*/) { lmr::todo("is_control_frame"); }

}  // namespace tetra
