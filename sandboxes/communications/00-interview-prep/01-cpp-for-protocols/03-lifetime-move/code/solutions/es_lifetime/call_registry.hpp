#pragma once
// SOLUTION - attempt Q20 first.
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace es::lifetime {

struct Listener {
    virtual ~Listener() = default;
    virtual void on_event(int code) = 0;
};

class CallRegistry {
public:
    void add(const std::shared_ptr<Listener>& l) { entries_.push_back(l); }  // weak_ptr from shared_ptr

    std::size_t notify(int code) {
        // 1. prune expired entries
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                      [](const std::weak_ptr<Listener>& w) { return w.expired(); }),
                       entries_.end());
        // 2. SNAPSHOT strong references. This (a) keeps each listener alive for the duration of
        //    its own callback even if another callback drops the last external owner, and
        //    (b) lets callbacks call add() without invalidating the iteration (entries_ may reallocate).
        std::vector<std::shared_ptr<Listener>> live;
        live.reserve(entries_.size());
        for (const auto& w : entries_) {
            if (auto s = w.lock()) live.push_back(std::move(s));
        }
        // 3. call outside any iteration over entries_
        for (const auto& s : live) s->on_event(code);
        return live.size();
    }

    std::size_t size() const { return entries_.size(); }

private:
    std::vector<std::weak_ptr<Listener>> entries_;
};

}  // namespace es::lifetime
