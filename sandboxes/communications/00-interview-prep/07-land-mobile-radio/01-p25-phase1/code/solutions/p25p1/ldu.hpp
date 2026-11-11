#pragma once
// SOLUTION - worksheet 01, Q11.
#include <array>
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p1 {

struct GroupVoiceLc {
  std::uint8_t lcf = 0;
  std::uint8_t mfid = 0;
  std::uint8_t service_options = 0;
  std::uint16_t tgid = 0;
  std::uint32_t source = 0;
};

inline GroupVoiceLc parse_group_voice_lc(const std::array<std::uint8_t, 9>& lc) {
  GroupVoiceLc r;
  r.lcf = lc[0];
  r.mfid = lc[1];
  r.service_options = lc[2];
  // lc[3] is reserved
  r.tgid = static_cast<std::uint16_t>((lc[4] << 8) | lc[5]);
  r.source = (static_cast<std::uint32_t>(lc[6]) << 16) | (static_cast<std::uint32_t>(lc[7]) << 8) |
             static_cast<std::uint32_t>(lc[8]);
  return r;
}

struct EncryptionSync {
  std::array<std::uint8_t, 9> mi{};
  std::uint8_t algid = 0x80;
  std::uint16_t kid = 0;
};

inline EncryptionSync parse_encryption_sync(const std::array<std::uint8_t, 12>& es) {
  EncryptionSync r;
  for (std::size_t i = 0; i < 9; ++i) r.mi[i] = es[i];
  r.algid = es[9];
  r.kid = static_cast<std::uint16_t>((es[10] << 8) | es[11]);
  return r;
}

inline bool is_clear(std::uint8_t algid) { return algid == 0x80; }

inline bool service_options_emergency(std::uint8_t so) { return (so & 0x80) != 0; }

inline const char* algid_name(std::uint8_t algid) {
  switch (algid) {
    case 0x80: return "clear";
    case 0x81: return "DES-OFB";
    case 0x84: return "AES-256";
    case 0xAA: return "ADP/RC4";
    default: return "other/vendor";
  }
}

}  // namespace p25p1
