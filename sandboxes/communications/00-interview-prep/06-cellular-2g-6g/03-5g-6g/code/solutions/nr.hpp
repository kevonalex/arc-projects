// nr.hpp  --  REFERENCE SOLUTION for Pack 06/03 (5G NR numerology, coding, link budget; 6G / "no 7G").
// Table values (PRB counts, base-graph rule, K_cb) are from memory of TS 38.101 / 38.211 / 38.212: VERIFY.
#pragma once
#include <cmath>
#include <cstdint>

namespace nr {

constexpr double kC = 299792458.0;
constexpr double kPi = 3.14159265358979323846;

// ------------------------------------------------------------------ numerology (W 3.1 - 3.2)
// mu = 0..4  ->  SCS = 15 * 2^mu kHz (15, 30, 60, 120, 240).  -1 outside that range.
inline int scs_khz(int mu) { return (mu >= 0 && mu <= 4) ? (15 << mu) : -1; }
inline int slots_per_subframe(int mu) { return (mu >= 0 && mu <= 4) ? (1 << mu) : -1; }
inline int slots_per_frame(int mu) { return (mu >= 0 && mu <= 4) ? (10 << mu) : -1; }
inline double slot_ms(int mu) { return (mu >= 0 && mu <= 4) ? 1.0 / (1 << mu) : -1.0; }
inline int symbols_per_slot() { return 14; }  // normal cyclic prefix
// Useful symbol length = 1 / SCS (CP excluded).
inline double symbol_useful_us(int mu) { return mu < 0 || mu > 4 ? -1.0 : 1000.0 / scs_khz(mu); }
// Average symbol incl. CP = slot / 14.
inline double symbol_avg_us(int mu) { return mu < 0 || mu > 4 ? -1.0 : slot_ms(mu) * 1000.0 / symbols_per_slot(); }

enum class FreqRange { FR1, FR2 };
// FR1 (sub-7 GHz) data SCS: 15/30/60 kHz = mu 0..2; FR2 (mmWave): 60/120 kHz = mu 2..3.
inline bool numerology_allowed(FreqRange fr, int mu) { return fr == FreqRange::FR1 ? (mu >= 0 && mu <= 2) : (mu >= 2 && mu <= 3); }

// Max transmission bandwidth in PRBs (TS 38.101-1/-2 tables; a few entries only, -1 otherwise).
inline int prb_count(double bw_mhz, int mu) {
  struct E { double bw; int mu; int n; };
  static const E t[] = {{5, 0, 25}, {10, 0, 52}, {20, 0, 106}, {50, 0, 270}, {10, 1, 24}, {20, 1, 51},
                        {40, 1, 106}, {100, 1, 273}, {100, 3, 66}, {400, 3, 264}};
  for (const E& e : t)
    if (e.bw == bw_mhz && e.mu == mu) return e.n;
  return -1;
}
// Number of slots in a TDD pattern period of period_ms at numerology mu (e.g. 2.5 ms, mu=1 -> 5 slots).
inline int slots_in_period(double period_ms, int mu) { return static_cast<int>(std::lround(period_ms * (1 << mu))); }

// ------------------------------------------------------------------ channel coding (W 3.4)
enum class NrChannel { PDSCH, PUSCH, PDCCH, PBCH };
enum class Code { LDPC, Polar };
inline Code channel_code_for(NrChannel c) { return (c == NrChannel::PDSCH || c == NrChannel::PUSCH) ? Code::LDPC : Code::Polar; }
// LDPC base graph for a transport block of A bits at code rate R (TS 38.212 7.2.2; VERIFY).
inline int ldpc_base_graph(int a_bits, double code_rate) {
  if (a_bits <= 292 || code_rate <= 0.25 || (a_bits <= 3824 && code_rate <= 0.67)) return 2;
  return 1;
}
// Code-block segmentation: K_cb = 8448 (BG1) or 3840 (BG2); each CB carries a 24-bit CRC when C > 1.
inline int num_code_blocks(int b_bits, int base_graph) {
  const int kcb = base_graph == 1 ? 8448 : 3840;
  if (b_bits <= kcb) return 1;
  return (b_bits + (kcb - 24) - 1) / (kcb - 24);
}

// ------------------------------------------------------------------ link physics (W 3.5 - 3.6)
// Free-space path loss in dB: 20 log10(4 pi d f / c).
inline double fspl_db(double freq_hz, double dist_m) { return 20.0 * std::log10(4.0 * kPi * dist_m * freq_hz / kC); }
inline double shannon_capacity_bps(double bw_hz, double snr_db) { return bw_hz * std::log2(1.0 + std::pow(10.0, snr_db / 10.0)); }
inline double one_way_delay_ms(double dist_km) { return dist_km * 1000.0 / kC * 1000.0; }
inline double max_doppler_hz(double speed_mps, double freq_hz) { return speed_mps * freq_hz / kC; }

// ------------------------------------------------------------------ generations (W 3.7)
enum class StandardStatus { Deployed, InStandardisation, NoStandard };
// There is NO 7G standard: nothing is being specified under that name at 3GPP or ITU.
inline StandardStatus generation_status(int g) {
  if (g >= 2 && g <= 5) return StandardStatus::Deployed;
  if (g == 6) return StandardStatus::InStandardisation;
  return StandardStatus::NoStandard;
}

}  // namespace nr
