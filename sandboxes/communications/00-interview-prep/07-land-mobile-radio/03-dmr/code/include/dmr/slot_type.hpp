#pragma once
// STUB - worksheet 03, Q6/Q10 : the DMR slot type field = 4-bit colour code + 4-bit data type,
// protected by a Golay code into 20 bits (10 bits on each side of the 48-bit middle).
//
// TEACHING construction of the (20,8) code (the exact ETSI matrix is VERIFY TS 102 361-1 Annex B):
//   word19  = Golay(23,12) codeword of the 8 info bits with the 4 leading (always zero) info bits dropped
//   word20  = (word19 << 1) | overall even parity of word19
// This is a *shortened + extended* Golay code: corrects 3 errors, detects 4.
#include <cstdint>
#include <stdexcept>

#include "dmr/golay23.hpp"
#include "lmr/todo.hpp"

namespace dmr {

// ETSI TS 102 361-1 data types (VERIFY).
enum class DataType : std::uint8_t {
  PiHeader = 0,
  VoiceLcHeader = 1,
  TerminatorWithLc = 2,
  Csbk = 3,
  MbcHeader = 4,
  MbcContinuation = 5,
  DataHeader = 6,
  Rate12Data = 7,
  Rate34Data = 8,
  Idle = 9,
  Rate1Data = 10,
};

struct SlotType {
  std::uint8_t colour_code = 0;  // 0..15
  std::uint8_t data_type = 0;    // 0..15 (may be an unassigned code point)
};

// TODO: (colour_code << 4) | data_type.  Throw std::out_of_range if either exceeds 15.
inline std::uint8_t pack_slot_type_byte(std::uint8_t /*cc*/, std::uint8_t /*dt*/) {
  lmr::todo("pack_slot_type_byte");
}

// TODO: inverse.
inline SlotType unpack_slot_type_byte(std::uint8_t /*byte*/) {
  lmr::todo("unpack_slot_type_byte");
}

// TODO: the 20-bit teaching codeword described above (use dmr::golay23_encode).
inline std::uint32_t slot_type_encode(std::uint8_t /*cc*/, std::uint8_t /*dt*/) {
  lmr::todo("slot_type_encode");
}

struct SlotTypeDecode {
  bool ok = false;
  SlotType slot_type{};
  int errors = 0;  // corrected bit errors (including the parity bit)
};

// TODO: decode.  Steps: split off parity bit; golay23_decode(word19); reject if the decoded 12-bit data
//       does not fit in 8 bits (error pattern outside the shortened code); count a parity mismatch as
//       one more error; ok only if total errors <= 3.
inline SlotTypeDecode slot_type_decode(std::uint32_t /*word20*/) {
  lmr::todo("slot_type_decode");
}

}  // namespace dmr
