#pragma once
// STUBS - worksheet Q16 (overflow) and Q17 (shifts, rotate). Tests fail until you implement them.
// None of your implementations may EXECUTE undefined behaviour - UBSan is watching.
#include <cstdint>
#include <limits>
#include <optional>

namespace es::ubbit {

// a + b, or nullopt if the true result does not fit in int32_t.
// You must DETECT overflow BEFORE adding (testing after the fact is too late: the add is the UB).
inline std::optional<std::int32_t> checked_add(std::int32_t a, std::int32_t b) {
    (void)a; (void)b;
    return std::nullopt;  // TODO
}

// Saturating add: clamps to INT32_MAX / INT32_MIN instead of overflowing.
inline std::int32_t sat_add(std::int32_t a, std::int32_t b) {
    (void)a; (void)b;
    return 0;  // TODO
}

// Two's-complement wrap-around add (what the hardware does), with NO undefined behaviour.
// wrapping_add(INT32_MAX, 1) == INT32_MIN. Hint: do the arithmetic in uint32_t.
inline std::int32_t wrapping_add(std::int32_t a, std::int32_t b) {
    (void)a; (void)b;
    return 0;  // TODO
}

// |a| or nullopt for INT32_MIN (its absolute value is not representable).
inline std::optional<std::int32_t> checked_abs(std::int32_t a) {
    (void)a;
    return std::nullopt;  // TODO
}

// Shifts with a DEFINED policy: shifting by >= 32 yields 0 (never UB).
inline std::uint32_t shl(std::uint32_t v, unsigned n) {
    (void)v; (void)n;
    return 1;  // TODO
}
inline std::uint32_t shr(std::uint32_t v, unsigned n) {
    (void)v; (void)n;
    return 1;  // TODO
}

// Rotate left. Must be correct for every n (including 0 and >= 32: rotation is modulo 32).
// The textbook (v << n) | (v >> (32 - n)) is UB at n == 0.
inline std::uint32_t rotl32_manual(std::uint32_t v, unsigned n) {
    (void)n;
    return v + 1;  // TODO
}

}  // namespace es::ubbit
