// nr.hpp  --  STUBS for Pack 06/03 (student version).  Reference: ../solutions/nr.hpp
// Table values (PRB counts, base-graph rule, K_cb) come from TS 38.101 / 38.211 / 38.212: VERIFY each.
#pragma once
#include <cmath>
#include <cstdint>

namespace nr {

constexpr double kC = 299792458.0;
constexpr double kPi = 3.14159265358979323846;

// ---- numerology (W 3.1 - 3.2).  Valid mu is 0..4; return -1 (or -1.0) outside that range. ----
inline int scs_khz([[maybe_unused]] int mu) { return 0; }             // TODO W 3.1: 15 * 2^mu
inline int slots_per_subframe([[maybe_unused]] int mu) { return 0; }  // TODO W 3.1: 2^mu
inline int slots_per_frame([[maybe_unused]] int mu) { return 0; }     // TODO W 3.1: 10 * 2^mu
inline double slot_ms([[maybe_unused]] int mu) { return 0.0; }        // TODO W 3.1: 1 / 2^mu
inline int symbols_per_slot() { return 0; }                           // TODO W 3.1: normal CP
inline double symbol_useful_us([[maybe_unused]] int mu) { return 0.0; }  // TODO W 3.1: 1 / SCS
inline double symbol_avg_us([[maybe_unused]] int mu) { return 0.0; }     // TODO W 3.1: slot / 14 (includes CP)

enum class FreqRange { FR1, FR2 };
// FR1 data SCS 15/30/60 kHz; FR2 data SCS 60/120 kHz.  (VERIFY TS 38.211 Table 4.2-1)
inline bool numerology_allowed([[maybe_unused]] FreqRange fr, [[maybe_unused]] int mu) { return false; }  // TODO W 3.2
// (bw MHz, mu) -> PRBs: (5,0)25 (10,0)52 (20,0)106 (50,0)270 (10,1)24 (20,1)51 (40,1)106 (100,1)273
// (100,3)66 (400,3)264; -1 otherwise.  (VERIFY TS 38.101-1/-2)
inline int prb_count([[maybe_unused]] double bw_mhz, [[maybe_unused]] int mu) { return 0; }  // TODO W 3.2
inline int slots_in_period([[maybe_unused]] double period_ms, [[maybe_unused]] int mu) { return 0; }  // TODO W 3.2

// ---- channel coding (W 3.4) ----
enum class NrChannel { PDSCH, PUSCH, PDCCH, PBCH };
enum class Code { LDPC, Polar };
inline Code channel_code_for([[maybe_unused]] NrChannel c) { return Code::Polar; }  // TODO W 3.4: data vs control
// BG2 if A <= 292, or R <= 0.25, or (A <= 3824 and R <= 0.67); else BG1.  (VERIFY TS 38.212 7.2.2)
inline int ldpc_base_graph([[maybe_unused]] int a_bits, [[maybe_unused]] double code_rate) { return 0; }  // TODO W 3.4
// K_cb = 8448 (BG1) / 3840 (BG2). One block if B <= K_cb, else ceil(B / (K_cb - 24)).
inline int num_code_blocks([[maybe_unused]] int b_bits, [[maybe_unused]] int base_graph) { return 0; }  // TODO W 3.4

// ---- link physics (W 3.5 - 3.6) ----
inline double fspl_db([[maybe_unused]] double freq_hz, [[maybe_unused]] double dist_m) { return 0.0; }  // TODO: 20log10(4 pi d f / c)
inline double shannon_capacity_bps([[maybe_unused]] double bw_hz, [[maybe_unused]] double snr_db) { return 0.0; }  // TODO
inline double one_way_delay_ms([[maybe_unused]] double dist_km) { return 0.0; }  // TODO: distance / c
inline double max_doppler_hz([[maybe_unused]] double speed_mps, [[maybe_unused]] double freq_hz) { return 0.0; }  // TODO v*f/c

// ---- generations (W 3.7) ----
enum class StandardStatus { Deployed, InStandardisation, NoStandard };
// 2G..5G deployed; 6G in standardisation (Rel-20 study -> Rel-21 normative work); 7G and above: NO standard.
inline StandardStatus generation_status([[maybe_unused]] int g) { return StandardStatus::Deployed; }  // TODO W 3.7

}  // namespace nr
