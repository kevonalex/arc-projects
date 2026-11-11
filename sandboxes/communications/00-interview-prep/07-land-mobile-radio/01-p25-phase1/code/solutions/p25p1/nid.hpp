#pragma once
// SOLUTION - worksheet 01, Q6.
#include <cstdint>
#include <optional>
#include <stdexcept>

#include "lmr/todo.hpp"

namespace p25p1 {

enum class Duid : std::uint8_t {
  Hdu = 0x0,
  Tdu = 0x3,
  Ldu1 = 0x5,
  Tsbk = 0x7,
  Ldu2 = 0xA,
  Pdu = 0xC,
  TduLc = 0xF,
};

struct Nid {
  std::uint16_t nac;
  std::uint8_t duid;
};

inline std::optional<Duid> duid_from_nibble(std::uint8_t nibble) {
  switch (nibble) {
    case 0x0: return Duid::Hdu;
    case 0x3: return Duid::Tdu;
    case 0x5: return Duid::Ldu1;
    case 0x7: return Duid::Tsbk;
    case 0xA: return Duid::Ldu2;
    case 0xC: return Duid::Pdu;
    case 0xF: return Duid::TduLc;
    default: return std::nullopt;
  }
}

inline std::uint16_t pack_nid_data(std::uint16_t nac, Duid duid) {
  if (nac > 0xFFF) throw std::out_of_range("NAC is 12 bits");
  return static_cast<std::uint16_t>((nac << 4) | static_cast<std::uint8_t>(duid));
}

inline Nid unpack_nid_data(std::uint16_t data16) {
  return Nid{static_cast<std::uint16_t>(data16 >> 4), static_cast<std::uint8_t>(data16 & 0xF)};
}

inline const char* duid_name(Duid d) {
  switch (d) {
    case Duid::Hdu: return "HDU";
    case Duid::Tdu: return "TDU";
    case Duid::Ldu1: return "LDU1";
    case Duid::Tsbk: return "TSBK";
    case Duid::Ldu2: return "LDU2";
    case Duid::Pdu: return "PDU";
    case Duid::TduLc: return "TDULC";
  }
  return "?";
}

}  // namespace p25p1
