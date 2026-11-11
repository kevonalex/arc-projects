#pragma once
// STUB - worksheet 05, Q7/Q11 : a minimal trunking controller: affiliation, channel grants, queueing,
// emergency pre-emption.  Teaching model of what a P25/TETRA/DMR Tier III control channel does
// (P25: TIA-102.AABC GRP_AFF_REQ, GRP_V_CH_GRANT, QUE_RSP, DENY_RSP - VERIFY names/opcodes).
//
// RULES (the tests encode exactly these):
//  1. A unit is affiliated with AT MOST ONE group. affiliate(u, g) replaces any earlier affiliation.
//  2. request_call(unit, group, emergency):
//       a. unit not affiliated to `group`            -> DeniedNotAffiliated
//       b. group already has an active call          -> Joined (same channel; emergency upgrades the call)
//       c. a channel is free                         -> Granted (lowest free channel index)
//       d. none free and emergency: the active call with the LOWEST group id that is NOT an emergency
//          call is dropped and its channel reused   -> Preempted (channel, preempted_group)
//       e. otherwise                                 -> Queued (a group is queued at most once; a later
//          emergency request upgrades its queue entry)
//  3. release_call(group): if the group is active, free its channel and, if the queue is not empty,
//     promote the FIRST queued emergency entry, else the first entry, onto that channel and return it.
//     If the group is merely queued, remove it from the queue (returns nullopt).
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "lmr/todo.hpp"

namespace lmrsys {

enum class GrantKind { Granted, Joined, Queued, Preempted, DeniedNotAffiliated };

struct GrantResult {
  GrantKind kind = GrantKind::DeniedNotAffiliated;
  int channel = -1;
  std::uint32_t preempted_group = 0;
};

struct QueuedGrant {
  std::uint32_t group = 0;
  int channel = -1;
};

class TrunkController {
 public:
  explicit TrunkController(int n_channels) : n_(n_channels) {}

  // TODO (rule 1)
  void affiliate(std::uint32_t /*unit*/, std::uint32_t /*group*/) { lmr::todo("affiliate"); }
  void deaffiliate(std::uint32_t /*unit*/) { lmr::todo("deaffiliate"); }
  bool is_affiliated(std::uint32_t /*unit*/, std::uint32_t /*group*/) const {
    lmr::todo("is_affiliated");
  }
  std::size_t affiliated_count(std::uint32_t /*group*/) const { lmr::todo("affiliated_count"); }

  // TODO (rule 2)
  GrantResult request_call(std::uint32_t /*unit*/, std::uint32_t /*group*/, bool /*emergency*/) {
    lmr::todo("request_call");
  }

  // TODO (rule 3)
  std::optional<QueuedGrant> release_call(std::uint32_t /*group*/) { lmr::todo("release_call"); }

  // TODO: channel index of the group's active call, or -1
  int channel_of(std::uint32_t /*group*/) const { lmr::todo("channel_of"); }
  // TODO: n_channels - active calls
  int free_channels() const { lmr::todo("free_channels"); }
  // TODO
  std::size_t queue_length() const { lmr::todo("queue_length"); }

 private:
  struct Active {
    int channel = -1;
    bool emergency = false;
  };
  struct Waiting {
    std::uint32_t group = 0;
    bool emergency = false;
  };
  int n_;
  std::unordered_map<std::uint32_t, std::uint32_t> aff_;  // unit -> group
  std::map<std::uint32_t, Active> active_;                // group -> call (ordered by group id)
  std::deque<Waiting> queue_;
};

}  // namespace lmrsys
