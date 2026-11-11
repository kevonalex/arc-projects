#pragma once
// Given helper: a stub body calls lmr::todo("name") so the matching test FAILS
// loudly (with a message) instead of silently returning garbage.
#include <stdexcept>
#include <string>

namespace lmr {
[[noreturn]] inline void todo(const std::string& what) {
  throw std::logic_error("TODO (not implemented yet): " + what);
}
}  // namespace lmr
