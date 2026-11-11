#pragma once
// SOLUTION - worksheet 05, Q7/Q11. See the stub for the rules.
#include <algorithm>
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

  void affiliate(std::uint32_t unit, std::uint32_t group) { aff_[unit] = group; }

  void deaffiliate(std::uint32_t unit) { aff_.erase(unit); }

  bool is_affiliated(std::uint32_t unit, std::uint32_t group) const {
    const auto it = aff_.find(unit);
    return it != aff_.end() && it->second == group;
  }

  std::size_t affiliated_count(std::uint32_t group) const {
    std::size_t n = 0;
    for (const auto& kv : aff_) {
      if (kv.second == group) ++n;
    }
    return n;
  }

  GrantResult request_call(std::uint32_t unit, std::uint32_t group, bool emergency) {
    GrantResult r;
    if (!is_affiliated(unit, group)) {
      r.kind = GrantKind::DeniedNotAffiliated;
      return r;
    }
    const auto it = active_.find(group);
    if (it != active_.end()) {
      if (emergency) it->second.emergency = true;
      r.kind = GrantKind::Joined;
      r.channel = it->second.channel;
      return r;
    }
    if (free_channels() > 0) {
      const int ch = lowest_free_channel();
      active_[group] = Active{ch, emergency};
      r.kind = GrantKind::Granted;
      r.channel = ch;
      return r;
    }
    if (emergency) {
      auto victim = active_.end();
      for (auto i = active_.begin(); i != active_.end(); ++i) {
        if (!i->second.emergency) {
          victim = i;
          break;
        }
      }
      if (victim != active_.end()) {
        const int ch = victim->second.channel;
        const std::uint32_t victim_group = victim->first;
        active_.erase(victim);
        active_[group] = Active{ch, true};
        r.kind = GrantKind::Preempted;
        r.channel = ch;
        r.preempted_group = victim_group;
        return r;
      }
    }
    bool found = false;
    for (auto& w : queue_) {
      if (w.group == group) {
        if (emergency) w.emergency = true;
        found = true;
        break;
      }
    }
    if (!found) queue_.push_back(Waiting{group, emergency});
    r.kind = GrantKind::Queued;
    return r;
  }

  std::optional<QueuedGrant> release_call(std::uint32_t group) {
    const auto it = active_.find(group);
    if (it == active_.end()) {
      queue_.erase(std::remove_if(queue_.begin(), queue_.end(),
                                  [group](const Waiting& w) { return w.group == group; }),
                   queue_.end());
      return std::nullopt;
    }
    const int ch = it->second.channel;
    active_.erase(it);
    if (queue_.empty()) return std::nullopt;
    auto pick = std::find_if(queue_.begin(), queue_.end(), [](const Waiting& w) { return w.emergency; });
    if (pick == queue_.end()) pick = queue_.begin();
    const QueuedGrant g{pick->group, ch};
    active_[g.group] = Active{ch, pick->emergency};
    queue_.erase(pick);
    return g;
  }

  int channel_of(std::uint32_t group) const {
    const auto it = active_.find(group);
    return it == active_.end() ? -1 : it->second.channel;
  }

  int free_channels() const { return n_ - static_cast<int>(active_.size()); }

  std::size_t queue_length() const { return queue_.size(); }

 private:
  struct Active {
    int channel = -1;
    bool emergency = false;
  };
  struct Waiting {
    std::uint32_t group = 0;
    bool emergency = false;
  };

  int lowest_free_channel() const {
    std::vector<bool> used(static_cast<std::size_t>(n_), false);
    for (const auto& kv : active_) used[static_cast<std::size_t>(kv.second.channel)] = true;
    for (int i = 0; i < n_; ++i) {
      if (!used[static_cast<std::size_t>(i)]) return i;
    }
    return -1;
  }

  int n_;
  std::unordered_map<std::uint32_t, std::uint32_t> aff_;
  std::map<std::uint32_t, Active> active_;
  std::deque<Waiting> queue_;
};

}  // namespace lmrsys
