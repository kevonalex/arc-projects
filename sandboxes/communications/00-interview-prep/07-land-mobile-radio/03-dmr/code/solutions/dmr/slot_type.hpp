#pragma once
// SOLUTION - worksheet 03, Q6/Q10.
#include <cstdint>
#include <stdexcept>

#include "dmr/golay23.hpp"
#include "lmr/todo.hpp"

namespace dmr {

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
  std::uint8_t colour_code = 0;
  std::uint8_t data_type = 0;
};

inline std::uint8_t pack_slot_type_byte(std::uint8_t cc, std::uint8_t dt) {
  if (cc > 15 || dt > 15) throw std::out_of_range("colour code and data type are 4 bits each");
  return static_cast<std::uint8_t>((cc << 4) | dt);
}

inline SlotType unpack_slot_type_byte(std::uint8_t byte) {
  SlotType s;
  s.colour_code = static_cast<std::uint8_t>(byte >> 4);
  s.data_type = static_cast<std::uint8_t>(byte & 0xF);
  return s;
}

inline std::uint32_t slot_type_encode(std::uint8_t cc, std::uint8_t dt) {
  const std::uint8_t info = pack_slot_type_byte(cc, dt);
  const std::uint32_t cw23 = golay23_encode(info);   // info < 256: the top 4 info bits are zero
  const std::uint32_t word19 = cw23 & 0x7FFFFu;      // drop those 4 zero bits (shortening)
  const std::uint32_t parity = static_cast<std::uint32_t>(popcount32(word19)) & 1u;
  return (word19 << 1) | parity;
}

struct SlotTypeDecode {
  bool ok = false;
  SlotType slot_type{};
  int errors = 0;
};

inline SlotTypeDecode slot_type_decode(std::uint32_t word20) {
  word20 &= 0xFFFFFu;
  const std::uint32_t word19 = word20 >> 1;
  const std::uint32_t received_parity = word20 & 1u;
  const Golay23Result g = golay23_decode(word19);
  SlotTypeDecode r;
  if (g.data > 0xFF) return r;  // a codeword outside the shortened code: more than 3 errors
  const std::uint32_t corrected19 = g.codeword & 0x7FFFFu;
  const std::uint32_t expected_parity = static_cast<std::uint32_t>(popcount32(corrected19)) & 1u;
  r.errors = g.errors + (expected_parity != received_parity ? 1 : 0);
  if (r.errors > 3) return r;
  r.ok = true;
  r.slot_type = unpack_slot_type_byte(static_cast<std::uint8_t>(g.data));
  return r;
}

}  // namespace dmr
