#pragma once
// GIVEN (complete; you built these in pack 01): DUIDs, NID packing and the NID encoder
// (BCH(63,16) systematic encode + one overall parity bit = 64 bits).
//   NID data word (16 bits) = NAC (12 bits) << 4 | DUID (4 bits)
//   NID codeword (64 bits)  = (bch63_16_encode(data16) << 1) | even-parity bit
// VERIFY: bit order, DUID values and the exact BCH generator against TIA-102.BAAA.
#include <bitset>
#include <cstdint>
#include <stdexcept>

#include "p25p1/gf64.hpp"

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

inline constexpr Duid kAllDuids[7] = {Duid::Hdu,  Duid::Tdu, Duid::Ldu1, Duid::Tsbk,
                                      Duid::Ldu2, Duid::Pdu, Duid::TduLc};

inline std::uint16_t pack_nid_data(std::uint16_t nac, Duid duid) {
  if (nac > 0xFFF) throw std::out_of_range("NAC is 12 bits");
  return static_cast<std::uint16_t>((nac << 4) | static_cast<std::uint8_t>(duid));
}

inline std::uint64_t poly_mod(std::uint64_t value, std::uint64_t gen, int gen_degree) {
  for (int i = 63; i >= gen_degree; --i) {
    if ((value >> i) & 1ULL) value ^= gen << (i - gen_degree);
  }
  return value;
}

inline std::uint64_t bch63_16_encode(std::uint16_t data) {
  constexpr int kParity = 47;
  const std::uint64_t shifted = static_cast<std::uint64_t>(data) << kParity;
  return shifted | poly_mod(shifted, bch63_16_generator(), kParity);
}

inline std::uint64_t nid_encode64(std::uint16_t nac, Duid duid) {
  const std::uint64_t cw = bch63_16_encode(pack_nid_data(nac, duid));
  const std::uint64_t parity = std::bitset<64>(cw).count() & 1ULL;
  return (cw << 1) | parity;
}

}  // namespace p25p1
