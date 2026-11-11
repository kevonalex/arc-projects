#pragma once
// SOLUTION - worksheet 05, Q9-Q10.
#include <cstdint>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "lmr/todo.hpp"

namespace lmrsys {

struct GlobalUnitId {
  std::uint32_t wacn = 0;
  std::uint16_t sysid = 0;
  std::uint32_t unit = 0;
};

struct GlobalGroupId {
  std::uint32_t wacn = 0;
  std::uint16_t sysid = 0;
  std::uint16_t tgid = 0;
};

inline std::uint64_t pack_unit(const GlobalUnitId& id) {
  if (id.wacn > 0xFFFFFu || id.sysid > 0xFFFu || id.unit > 0xFFFFFFu) {
    throw std::out_of_range("GlobalUnitId field too wide");
  }
  return (static_cast<std::uint64_t>(id.wacn) << 36) | (static_cast<std::uint64_t>(id.sysid) << 24) |
         static_cast<std::uint64_t>(id.unit);
}

inline GlobalUnitId unpack_unit(std::uint64_t v) {
  if ((v >> 56) != 0) throw std::out_of_range("more than 56 bits");
  GlobalUnitId id;
  id.wacn = static_cast<std::uint32_t>((v >> 36) & 0xFFFFFu);
  id.sysid = static_cast<std::uint16_t>((v >> 24) & 0xFFFu);
  id.unit = static_cast<std::uint32_t>(v & 0xFFFFFFu);
  return id;
}

inline std::string to_unit_uri(const GlobalUnitId& id, std::string_view domain) {
  char buf[40];
  std::snprintf(buf, sizeof buf, "sip:p25-%05X-%03X-%06X@", static_cast<unsigned>(id.wacn),
                static_cast<unsigned>(id.sysid), static_cast<unsigned>(id.unit));
  return std::string(buf) + std::string(domain);
}

inline std::string to_group_uri(const GlobalGroupId& id, std::string_view domain) {
  char buf[40];
  std::snprintf(buf, sizeof buf, "sip:p25tg-%05X-%03X-%04X@", static_cast<unsigned>(id.wacn),
                static_cast<unsigned>(id.sysid), static_cast<unsigned>(id.tgid));
  return std::string(buf) + std::string(domain);
}

inline std::optional<GlobalUnitId> parse_unit_uri(std::string_view uri, std::string_view domain) {
  constexpr std::string_view prefix = "sip:p25-";
  if (uri.size() < prefix.size() || uri.substr(0, prefix.size()) != prefix) return std::nullopt;
  uri.remove_prefix(prefix.size());
  // WWWWW-SSS-UUUUUU@domain  -> separators at 5, 9 and '@' at 16
  if (uri.size() != 17 + domain.size()) return std::nullopt;
  if (uri[5] != '-' || uri[9] != '-' || uri[16] != '@') return std::nullopt;
  if (uri.substr(17) != domain) return std::nullopt;
  auto hex = [](std::string_view s, std::uint32_t& out) {
    out = 0;
    for (const char c : s) {
      std::uint32_t d;
      if (c >= '0' && c <= '9') {
        d = static_cast<std::uint32_t>(c - '0');
      } else if (c >= 'A' && c <= 'F') {
        d = static_cast<std::uint32_t>(c - 'A' + 10);
      } else if (c >= 'a' && c <= 'f') {
        d = static_cast<std::uint32_t>(c - 'a' + 10);
      } else {
        return false;
      }
      out = (out << 4) | d;
    }
    return true;
  };
  std::uint32_t w = 0, s = 0, u = 0;
  if (!hex(uri.substr(0, 5), w) || !hex(uri.substr(6, 3), s) || !hex(uri.substr(10, 6), u)) {
    return std::nullopt;
  }
  GlobalUnitId id;
  id.wacn = w;
  id.sysid = static_cast<std::uint16_t>(s);
  id.unit = u;
  return id;
}

enum class LmrFloorEvent {
  PttPress,
  PttRelease,
  GrantToRequester,
  TalkerChangedForOthers,
  Deny,
  Queued,
  Revoked,
  ChannelIdle
};

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

inline McpttFloorMsg to_mcptt(LmrFloorEvent e) {
  switch (e) {
    case LmrFloorEvent::PttPress: return McpttFloorMsg::FloorRequest;
    case LmrFloorEvent::PttRelease: return McpttFloorMsg::FloorRelease;
    case LmrFloorEvent::GrantToRequester: return McpttFloorMsg::FloorGranted;
    case LmrFloorEvent::TalkerChangedForOthers: return McpttFloorMsg::FloorTaken;
    case LmrFloorEvent::Deny: return McpttFloorMsg::FloorDeny;
    case LmrFloorEvent::Queued: return McpttFloorMsg::FloorQueuePositionInfo;
    case LmrFloorEvent::Revoked: return McpttFloorMsg::FloorRevoke;
    case LmrFloorEvent::ChannelIdle: return McpttFloorMsg::FloorIdle;
  }
  throw std::out_of_range("bad LmrFloorEvent");
}

inline LmrFloorEvent to_lmr(McpttFloorMsg m) {
  switch (m) {
    case McpttFloorMsg::FloorRequest: return LmrFloorEvent::PttPress;
    case McpttFloorMsg::FloorRelease: return LmrFloorEvent::PttRelease;
    case McpttFloorMsg::FloorGranted: return LmrFloorEvent::GrantToRequester;
    case McpttFloorMsg::FloorTaken: return LmrFloorEvent::TalkerChangedForOthers;
    case McpttFloorMsg::FloorDeny: return LmrFloorEvent::Deny;
    case McpttFloorMsg::FloorQueuePositionInfo: return LmrFloorEvent::Queued;
    case McpttFloorMsg::FloorRevoke: return LmrFloorEvent::Revoked;
    case McpttFloorMsg::FloorIdle: return LmrFloorEvent::ChannelIdle;
  }
  throw std::out_of_range("bad McpttFloorMsg");
}

}  // namespace lmrsys
