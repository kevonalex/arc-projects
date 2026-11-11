#pragma once
// GIVEN (complete): builders for SYNTHETIC P25-style frames. Nothing here touches a radio.
//
// Unit of work = one SYMBOL = one dibit (2 bits) stored as a uint8_t in 0..3, MSB of the dibit first.
// A frame on the "air" is:
//     48-bit sync (24 symbols) | 64-bit NID (32 symbols) | payload symbols
// with one STATUS symbol inserted so that, counting from the first symbol of the frame (index 0),
// the symbol at index k is a status symbol exactly when (k + 1) % 36 == 0   (VERIFY: TIA-102.BAAA).
// Equivalently: after every 35 data symbols one status symbol follows.
#include <cstddef>
#include <cstdint>
#include <vector>

#include "p25p1/nid_given.hpp"

namespace p25p1 {

inline constexpr std::uint64_t kFrameSync48 = 0x5575F5FF77FFULL;  // VERIFY
inline constexpr std::size_t kSyncSymbols = 24;
inline constexpr std::size_t kNidSymbols = 32;
inline constexpr std::size_t kStatusPeriod = 36;

// Most-significant dibit first.
inline std::vector<std::uint8_t> bits_to_symbols(std::uint64_t value, std::size_t n_symbols) {
  std::vector<std::uint8_t> out(n_symbols);
  for (std::size_t i = 0; i < n_symbols; ++i) {
    const std::size_t shift = 2 * (n_symbols - 1 - i);
    out[i] = static_cast<std::uint8_t>((value >> shift) & 3ULL);
  }
  return out;
}

inline std::vector<std::uint8_t> insert_status_symbols(const std::vector<std::uint8_t>& data,
                                                       std::uint8_t status = 0) {
  std::vector<std::uint8_t> out;
  out.reserve(data.size() + data.size() / 35 + 1);
  for (const std::uint8_t s : data) {
    out.push_back(s);
    if ((out.size() + 1) % kStatusPeriod == 0) out.push_back(status);
  }
  return out;
}

// Index in the AIR stream of the k-th DATA symbol of a frame (data symbols exclude status symbols).
inline std::size_t air_index_of_data(std::size_t k) { return k + k / 35; }

inline std::vector<std::uint8_t> make_frame(std::uint16_t nac, Duid duid,
                                            const std::vector<std::uint8_t>& payload_symbols) {
  std::vector<std::uint8_t> data = bits_to_symbols(kFrameSync48, kSyncSymbols);
  const std::vector<std::uint8_t> nid = bits_to_symbols(nid_encode64(nac, duid), kNidSymbols);
  data.insert(data.end(), nid.begin(), nid.end());
  data.insert(data.end(), payload_symbols.begin(), payload_symbols.end());
  return insert_status_symbols(data, 0);
}

}  // namespace p25p1
