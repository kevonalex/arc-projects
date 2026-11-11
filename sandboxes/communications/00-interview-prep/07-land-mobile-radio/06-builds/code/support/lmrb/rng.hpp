#pragma once
// GIVEN: a tiny deterministic generator so every synthetic stream is reproducible (no flaky tests).
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lmrb {

class XorShift64 {
 public:
  explicit XorShift64(std::uint64_t seed) : s_(seed != 0 ? seed : 0x9E3779B97F4A7C15ULL) {}
  std::uint64_t next() {
    s_ ^= s_ << 13;
    s_ ^= s_ >> 7;
    s_ ^= s_ << 17;
    return s_;
  }
  std::uint8_t bit() { return static_cast<std::uint8_t>(next() >> 63); }
  std::uint8_t symbol() { return static_cast<std::uint8_t>(next() >> 62); }  // 0..3 (a dibit)

 private:
  std::uint64_t s_;
};

inline std::vector<std::uint8_t> random_bits(XorShift64& r, std::size_t n) {
  std::vector<std::uint8_t> v(n);
  for (auto& b : v) b = r.bit();
  return v;
}

inline std::vector<std::uint8_t> random_symbols(XorShift64& r, std::size_t n) {
  std::vector<std::uint8_t> v(n);
  for (auto& s : v) s = r.symbol();
  return v;
}

}  // namespace lmrb
