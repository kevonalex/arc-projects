#pragma once
// STUB - worksheet 06, Build B (Q5-Q8): a parser for SYNTHETIC P25-style frames.
// Input is a vector of dibits (0..3) produced by p25p1::make_frame() in the tests - never a radio.
//
// Pipeline you will build and DERIVE (worksheet 06 section 3.2):
//   air symbols --find_sync (tolerant)--> frame_start
//               --strip_status_symbols(frame_start)--> data symbols (sync | NID | payload)
//               --symbols_to_bits(NID)--> decode_nid by CANDIDATE MATCHING --> NAC, DUID
//               --classify / expected_payload_symbols--> dispatch
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "lmr/todo.hpp"
#include "p25p1/nid_given.hpp"
#include "p25p1/synth.hpp"

namespace lmrb {

// Pack `count` dibits starting at `start` into an integer, FIRST symbol in the most significant position.
// {1,2,3,0} -> 0b01'10'11'00 = 0x6C.   Precondition: start + count <= symbols.size() and count <= 32.
// TODO
inline std::uint64_t symbols_to_bits(const std::vector<std::uint8_t>& /*symbols*/, std::size_t /*start*/,
                                     std::size_t /*count*/) {
  lmr::todo("symbols_to_bits");
}

// Return the symbols from `frame_start` to the end with every status symbol removed. Status symbols are
// those whose index RELATIVE TO frame_start satisfies (rel + 1) % 36 == 0.   (Symbols before frame_start are dropped.)
// TODO
inline std::vector<std::uint8_t> strip_status_symbols(const std::vector<std::uint8_t>& /*air*/,
                                                      std::size_t /*frame_start*/) {
  lmr::todo("strip_status_symbols");
}

struct SyncHit {
  std::size_t pos = 0;  // index of the first sync symbol in the air stream
  int errors = 0;       // bit errors against the 48-bit sync
};

// First position >= `from` whose 24 symbols (48 bits) are within `max_bit_errors` bit errors of
// p25p1::kFrameSync48. Operates on RAW air symbols: the first status symbol of a frame comes only after
// 35 symbols, so the 24-symbol sync is never interrupted. nullopt if none (also if fewer than 24 symbols remain).
// TODO
inline std::optional<SyncHit> find_sync(const std::vector<std::uint8_t>& /*air*/, std::size_t /*from*/,
                                        int /*max_bit_errors*/) {
  lmr::todo("find_sync");
}

struct NidDecode {
  bool ok = false;
  std::uint16_t nac = 0;
  p25p1::Duid duid = p25p1::Duid::Hdu;
  int errors = 0;  // bit distance to the winning candidate
};

// Candidate matching: for every (nac, duid) candidate compute p25p1::nid_encode64(nac, duid) and its Hamming
// distance to `nid64`. `nacs` is the list of NACs the receiver is configured for; if it is EMPTY try all 4096.
// Duids = the 7 valid values p25p1::kAllDuids. Pick the closest candidate. ok = (distance <= max_bit_errors)
// AND the best is UNIQUE (a different (nac,duid) at the same distance means ambiguous -> not ok).
// Why this works without a BCH decoder: worksheet 06 Q6 (d_min of the extended code is 24).
// TODO
inline NidDecode decode_nid(std::uint64_t /*nid64*/, const std::vector<std::uint16_t>& /*nacs*/,
                            int /*max_bit_errors*/) {
  lmr::todo("decode_nid");
}

enum class FrameClass { Header, Voice, Control, Data, Terminator };

// HDU -> Header; LDU1, LDU2 -> Voice; TSBK -> Control; PDU -> Data; TDU, TDULC -> Terminator.   TODO
inline FrameClass classify(p25p1::Duid /*duid*/) { lmr::todo("classify"); }

// Payload length in SYMBOLS after sync+NID, where the frame has a fixed size: LDU1 and LDU2 = 784
// (1568 bits: 9 x 144 voice + 240 LC/ES + 32 LSD - VERIFY). Other DUIDs are variable -> nullopt.   TODO
inline std::optional<std::size_t> expected_payload_symbols(p25p1::Duid /*duid*/) {
  lmr::todo("expected_payload_symbols");
}

struct ParsedFrame {
  std::size_t frame_start = 0;  // air index of the first sync symbol
  int sync_errors = 0;
  std::uint16_t nac = 0;
  p25p1::Duid duid = p25p1::Duid::Hdu;
  int nid_errors = 0;
  std::vector<std::uint8_t> payload;  // data symbols after the NID, status symbols removed
};

// Find and decode the first valid frame at or after `from`.
//  loop: hit = find_sync(air, pos, sync_tol); none -> nullopt.
//        data = strip_status_symbols(air, hit.pos); need >= 24 + 32 symbols else nullopt (truncated).
//        nid  = decode_nid(symbols_to_bits(data, 24, 32), nacs, 11); if !ok -> a FALSE SYNC: pos = hit.pos + 1, retry.
//        payload = data[56 ..]; if expected_payload_symbols(duid) is known: fewer than that -> nullopt (truncated);
//        more -> keep only the first `expected` symbols.
// TODO
inline std::optional<ParsedFrame> parse_frame(const std::vector<std::uint8_t>& /*air*/,
                                              const std::vector<std::uint16_t>& /*nacs*/, int /*sync_tol*/,
                                              std::size_t /*from*/ = 0) {
  lmr::todo("parse_frame");
}

}  // namespace lmrb
