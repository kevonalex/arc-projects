#pragma once
// STUB - worksheet 05, Q9-Q10 : identity and floor-control mapping for an LMR <-> MCPTT gateway.
//
// P25 global unit identity   = WACN (20 bits) | System ID (12 bits) | Unit ID (24 bits)  = 56 bits
// P25 global group identity  = WACN (20 bits) | System ID (12 bits) | Talkgroup (16 bits) = 48 bits
// (field widths: TIA-102.AABC family - VERIFY.)
//
// URI forms are a TEACHING CONVENTION invented for this worksheet (NOT from 3GPP/TIA):
//   unit  : "sip:p25-"   + %05X wacn + "-" + %03X sysid + "-" + %06X unit + "@" + domain
//   group : "sip:p25tg-" + %05X wacn + "-" + %03X sysid + "-" + %04X tgid + "@" + domain
//   e.g.  sip:p25-BEE00-001-00ABCD@iwf.example
// How a real IWF maps identities (MCPTT ID, SIP URIs) is defined in 3GPP TS 23.283 / TS 24.379 - VERIFY.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "lmr/todo.hpp"

namespace lmrsys {

struct GlobalUnitId {
  std::uint32_t wacn = 0;   // 20 bits
  std::uint16_t sysid = 0;  // 12 bits
  std::uint32_t unit = 0;   // 24 bits
};

struct GlobalGroupId {
  std::uint32_t wacn = 0;   // 20 bits
  std::uint16_t sysid = 0;  // 12 bits
  std::uint16_t tgid = 0;   // 16 bits
};

// Q9. TODO: (wacn << 36) | (sysid << 24) | unit.  Throw std::out_of_range if any field exceeds its width.
inline std::uint64_t pack_unit(const GlobalUnitId&) { lmr::todo("pack_unit"); }

// Q9. TODO: inverse of pack_unit (bits above 56 must be zero; otherwise throw std::out_of_range).
inline GlobalUnitId unpack_unit(std::uint64_t) { lmr::todo("unpack_unit"); }

// Q9. TODO: format as described above (uppercase hex, zero padded).
inline std::string to_unit_uri(const GlobalUnitId&, std::string_view /*domain*/) { lmr::todo("to_unit_uri"); }

// Q9. TODO: exact inverse of to_unit_uri. Return nullopt on ANY deviation: wrong prefix, wrong
// separators, wrong length, non-hex digit, or a domain different from `domain`. Accept lowercase hex.
inline std::optional<GlobalUnitId> parse_unit_uri(std::string_view /*uri*/, std::string_view /*domain*/) {
  lmr::todo("parse_unit_uri");
}

// Q9. TODO: group URI as described above.
inline std::string to_group_uri(const GlobalGroupId&, std::string_view /*domain*/) {
  lmr::todo("to_group_uri");
}

// Q10. Floor-control vocabulary on each side of the gateway.
enum class LmrFloorEvent {
  PttPress,                // a unit asks to talk
  PttRelease,              // the talker lets go
  GrantToRequester,        // the system tells the requester "you have the floor"
  TalkerChangedForOthers,  // the system tells listeners "someone else now talks"
  Deny,                    // request refused
  Queued,                  // request waits in a queue
  Revoked,                 // the talker is pre-empted
  ChannelIdle              // nobody holds the floor
};

// Floor message names follow MCPTT floor control (3GPP TS 24.380 - VERIFY exact names and the full list).
enum class McpttFloorMsg {
  FloorRequest,
  FloorRelease,
  FloorGranted,
  FloorTaken,
  FloorDeny,
  FloorQueuePositionInfo,
  FloorRevoke,
  FloorIdle
};

// Q10. TODO: one-to-one mapping
//   PttPress->FloorRequest, PttRelease->FloorRelease, GrantToRequester->FloorGranted,
//   TalkerChangedForOthers->FloorTaken, Deny->FloorDeny, Queued->FloorQueuePositionInfo,
//   Revoked->FloorRevoke, ChannelIdle->FloorIdle
inline McpttFloorMsg to_mcptt(LmrFloorEvent) { lmr::todo("to_mcptt"); }

// Q10. TODO: the inverse
inline LmrFloorEvent to_lmr(McpttFloorMsg) { lmr::todo("to_lmr"); }

}  // namespace lmrsys
