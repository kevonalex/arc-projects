#pragma once
// STUB - worksheet 01, Q11: parse already-FEC-decoded Link Control (LDU1) and Encryption Sync (LDU2).
// Layouts below are the TEACHING layouts used in the worksheet. VERIFY against TIA-102.BAAA-A
// (link control formats: TIA-102.AABF).
#include <array>
#include <cstdint>

#include "lmr/todo.hpp"

namespace p25p1 {

// Group Voice Channel User LC: 9 bytes = 72 bits
//   byte0 LCF (P|SF|LCO6)   byte1 MFID   byte2 service options   byte3 reserved
//   byte4-5 TGID (16 bit)   byte6-8 source unit id (24 bit)
struct GroupVoiceLc {
  std::uint8_t lcf = 0;
  std::uint8_t mfid = 0;
  std::uint8_t service_options = 0;
  std::uint16_t tgid = 0;
  std::uint32_t source = 0;
};

// TODO: big-endian field extraction as above.
inline GroupVoiceLc parse_group_voice_lc(const std::array<std::uint8_t, 9>& /*lc*/) {
  lmr::todo("parse_group_voice_lc");
}

// Encryption Sync: 12 bytes = 96 bits: MI = bytes 0-8 (72 bit), ALGID = byte 9, KID = bytes 10-11.
struct EncryptionSync {
  std::array<std::uint8_t, 9> mi{};
  std::uint8_t algid = 0x80;
  std::uint16_t kid = 0;
};

// TODO: extract the three fields (KID big-endian).
inline EncryptionSync parse_encryption_sync(const std::array<std::uint8_t, 12>& /*es*/) {
  lmr::todo("parse_encryption_sync");
}

// TODO: ALGID 0x80 means "unencrypted". Everything else means a cipher is in use.
inline bool is_clear(std::uint8_t /*algid*/) { lmr::todo("is_clear"); }

// TODO: emergency flag = MSB of service options (VERIFY bit position in TIA-102.AABC/AABF).
inline bool service_options_emergency(std::uint8_t /*so*/) {
  lmr::todo("service_options_emergency");
}

// GIVEN: names for logs. VERIFY ALGID assignments.
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
