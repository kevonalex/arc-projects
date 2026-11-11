#pragma once
// STUB - worksheet 01, Q6 (trace) - the 16 information bits of the Network ID.
// NID = 12-bit NAC | 4-bit DUID  (then BCH(63,16) + 1 parity -> 64 bits on air; see bch.hpp)
#include <cstdint>
#include <optional>
#include <stdexcept>

#include "lmr/todo.hpp"

namespace p25p1 {

// Data Unit IDs (TIA-102.BAAA). VERIFY values.
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
  std::uint16_t nac;   // 12 bits
  std::uint8_t duid;   // 4 bits, raw (may be an invalid code point)
};

// TODO: map a 4-bit value to a Duid, or std::nullopt for unassigned code points.
inline std::optional<Duid> duid_from_nibble(std::uint8_t /*nibble*/) {
  lmr::todo("duid_from_nibble");
}

// TODO: (nac << 4) | duid.  Throw std::out_of_range if nac > 0xFFF.
inline std::uint16_t pack_nid_data(std::uint16_t /*nac*/, Duid /*duid*/) {
  lmr::todo("pack_nid_data");
}

// TODO: inverse of pack_nid_data (do not validate the DUID here).
inline Nid unpack_nid_data(std::uint16_t /*data16*/) { lmr::todo("unpack_nid_data"); }

// GIVEN
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
