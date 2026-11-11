#pragma once
// GIVEN (complete) - GF(2^6) arithmetic and the BCH(63,16) generator polynomial,
// DERIVED from first principles instead of copied from a table.
//
// Construction (worksheet 01 section 3.3):
//   * Field GF(64) built from the primitive polynomial x^6 + x + 1.
//   * BCH code of length 63 with designed roots alpha^1 .. alpha^22
//     (designed distance 23  ->  corrects t = 11 errors).
//   * g(x) = product of (x + alpha^r) over the roots plus all their conjugates
//     (the cyclotomic cosets of 1..22 under doubling mod 63) -> degree 47 -> k = 63-47 = 16.
//
// VERIFY: that TIA-102.BAAA uses exactly this generator (field polynomial choice).
//         Any primitive degree-6 polynomial gives a BCH(63,16,23) code, but a DIFFERENT one.
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

namespace p25p1 {

class Gf64 {
 public:
  Gf64() {
    unsigned x = 1;
    for (int i = 0; i < 63; ++i) {
      exp_[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(x);
      log_[x] = static_cast<std::uint8_t>(i);
      x <<= 1;
      if (x & 0x40u) x ^= 0x43u;  // reduce modulo x^6 + x + 1
    }
  }
  std::uint8_t mul(std::uint8_t a, std::uint8_t b) const {
    if (a == 0 || b == 0) return 0;
    return exp_[static_cast<std::size_t>((log_[a] + log_[b]) % 63)];
  }
  std::uint8_t alpha_pow(int i) const {
    return exp_[static_cast<std::size_t>(((i % 63) + 63) % 63)];
  }

 private:
  std::array<std::uint8_t, 63> exp_{};
  std::array<std::uint8_t, 64> log_{};
};

// Bit k of the result is the coefficient of x^k. Bit 47 is the leading term.
inline std::uint64_t bch63_16_generator() {
  static const std::uint64_t g = [] {
    Gf64 gf;
    std::set<int> roots;
    for (int i = 1; i <= 22; ++i) {
      int r = i;
      do {
        roots.insert(r);
        r = (r * 2) % 63;
      } while (r != i);
    }
    std::vector<std::uint8_t> poly{1};  // poly[k] = coefficient of x^k
    for (int r : roots) {
      std::vector<std::uint8_t> next(poly.size() + 1, 0);
      const std::uint8_t a = gf.alpha_pow(r);
      for (std::size_t k = 0; k < poly.size(); ++k) {
        next[k + 1] ^= poly[k];            // x * poly
        next[k] ^= gf.mul(poly[k], a);     // alpha^r * poly
      }
      poly.swap(next);
    }
    std::uint64_t bits = 0;
    for (std::size_t k = 0; k < poly.size(); ++k) {
      if (poly[k]) bits |= (1ULL << k);
    }
    return bits;
  }();
  return g;
}

}  // namespace p25p1
