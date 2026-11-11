#pragma once
// SOLUTION - worksheet 06, Build B.
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "lmr/todo.hpp"
#include "p25p1/nid_given.hpp"
#include "p25p1/synth.hpp"

namespace lmrb {

inline std::uint64_t symbols_to_bits(const std::vector<std::uint8_t>& symbols, std::size_t start,
                                     std::size_t count) {
  std::uint64_t v = 0;
  for (std::size_t i = 0; i < count; ++i) v = (v << 2) | static_cast<std::uint64_t>(symbols[start + i] & 3u);
  return v;
}

inline std::vector<std::uint8_t> strip_status_symbols(const std::vector<std::uint8_t>& air,
                                                      std::size_t frame_start) {
  std::vector<std::uint8_t> out;
  if (frame_start >= air.size()) return out;
  out.reserve(air.size() - frame_start);
  for (std::size_t k = frame_start; k < air.size(); ++k) {
    const std::size_t rel = k - frame_start;
    if ((rel + 1) % p25p1::kStatusPeriod != 0) out.push_back(air[k]);
  }
  return out;
}

struct SyncHit {
  std::size_t pos = 0;
  int errors = 0;
};

inline std::optional<SyncHit> find_sync(const std::vector<std::uint8_t>& air, std::size_t from,
                                        int max_bit_errors) {
  constexpr std::uint64_t mask = (1ULL << 48) - 1;
  if (air.size() < p25p1::kSyncSymbols) return std::nullopt;
  for (std::size_t i = from; i + p25p1::kSyncSymbols <= air.size(); ++i) {
    const std::uint64_t window = symbols_to_bits(air, i, p25p1::kSyncSymbols);
    const int d = static_cast<int>(std::bitset<64>((window ^ p25p1::kFrameSync48) & mask).count());
    if (d <= max_bit_errors) return SyncHit{i, d};
  }
  return std::nullopt;
}

struct NidDecode {
  bool ok = false;
  std::uint16_t nac = 0;
  p25p1::Duid duid = p25p1::Duid::Hdu;
  int errors = 0;
};

inline NidDecode decode_nid(std::uint64_t nid64, const std::vector<std::uint16_t>& nacs,
                            int max_bit_errors) {
  NidDecode best;
  best.errors = 65;
  bool tie = false;
  auto consider = [&](std::uint16_t nac) {
    for (const p25p1::Duid d : p25p1::kAllDuids) {
      const int dist = static_cast<int>(std::bitset<64>(p25p1::nid_encode64(nac, d) ^ nid64).count());
      if (dist < best.errors) {
        best.errors = dist;
        best.nac = nac;
        best.duid = d;
        tie = false;
      } else if (dist == best.errors && (nac != best.nac || d != best.duid)) {
        tie = true;
      }
    }
  };
  if (nacs.empty()) {
    for (std::uint16_t nac = 0; nac <= 0xFFF; ++nac) consider(nac);
  } else {
    for (const std::uint16_t nac : nacs) consider(nac);
  }
  best.ok = (best.errors <= max_bit_errors) && !tie;
  return best;
}

enum class FrameClass { Header, Voice, Control, Data, Terminator };

inline FrameClass classify(p25p1::Duid duid) {
  switch (duid) {
    case p25p1::Duid::Hdu: return FrameClass::Header;
    case p25p1::Duid::Ldu1: return FrameClass::Voice;
    case p25p1::Duid::Ldu2: return FrameClass::Voice;
    case p25p1::Duid::Tsbk: return FrameClass::Control;
    case p25p1::Duid::Pdu: return FrameClass::Data;
    case p25p1::Duid::Tdu: return FrameClass::Terminator;
    case p25p1::Duid::TduLc: return FrameClass::Terminator;
  }
  return FrameClass::Data;  // unreachable for valid enumerators
}

inline std::optional<std::size_t> expected_payload_symbols(p25p1::Duid duid) {
  if (duid == p25p1::Duid::Ldu1 || duid == p25p1::Duid::Ldu2) return std::size_t{784};
  return std::nullopt;
}

struct ParsedFrame {
  std::size_t frame_start = 0;
  int sync_errors = 0;
  std::uint16_t nac = 0;
  p25p1::Duid duid = p25p1::Duid::Hdu;
  int nid_errors = 0;
  std::vector<std::uint8_t> payload;
};

inline std::optional<ParsedFrame> parse_frame(const std::vector<std::uint8_t>& air,
                                              const std::vector<std::uint16_t>& nacs, int sync_tol,
                                              std::size_t from = 0) {
  constexpr std::size_t kHeader = p25p1::kSyncSymbols + p25p1::kNidSymbols;  // 56
  std::size_t pos = from;
  for (;;) {
    const auto hit = find_sync(air, pos, sync_tol);
    if (!hit) return std::nullopt;
    const std::vector<std::uint8_t> data = strip_status_symbols(air, hit->pos);
    if (data.size() < kHeader) return std::nullopt;  // truncated: later sync hits can't be longer
    const NidDecode nid =
        decode_nid(symbols_to_bits(data, p25p1::kSyncSymbols, p25p1::kNidSymbols), nacs, 11);
    if (!nid.ok) {
      pos = hit->pos + 1;  // false sync (or unwanted NAC): keep looking
      continue;
    }
    ParsedFrame f;
    f.frame_start = hit->pos;
    f.sync_errors = hit->errors;
    f.nac = nid.nac;
    f.duid = nid.duid;
    f.nid_errors = nid.errors;
    f.payload.assign(data.begin() + static_cast<std::ptrdiff_t>(kHeader), data.end());
    if (const auto want = expected_payload_symbols(nid.duid)) {
      if (f.payload.size() < *want) return std::nullopt;
      f.payload.resize(*want);
    }
    return f;
  }
}

}  // namespace lmrb
