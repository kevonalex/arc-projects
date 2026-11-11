// lte.hpp  --  STUBS for Pack 06/02 (student version).  Reference: ../solutions/lte.hpp
#pragma once
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace lte {

using cd = std::complex<double>;
constexpr double kPi = 3.14159265358979323846;
constexpr double kScsHz = 15000.0;  // LTE subcarrier spacing

// ---- numerology (W 3.1 - 3.2) ----
// 1.4->6, 3->15, 5->25, 10->50, 15->75, 20->100 resource blocks; -1 otherwise.  (VERIFY in TS 36.101)
inline int rb_for_bandwidth_mhz([[maybe_unused]] double bw) { return 0; }  // TODO W 3.1
// RB count -> FFT size (6:128 15:256 25:512 50:1024 75:1536 100:2048); -1 otherwise.
inline int fft_size_for_rb([[maybe_unused]] int nrb) { return 0; }  // TODO W 3.1
inline double sample_rate_hz([[maybe_unused]] int nrb) { return 0.0; }  // TODO W 3.1: FFT * 15 kHz, -1 if unknown
inline double useful_symbol_us() { return 0.0; }                          // TODO W 3.1: 1 / SCS
// Normal CP: symbol 0 of a slot has 160/2048 * FFT samples of CP, symbols 1..6 have 144/2048 * FFT.
inline int cp_samples([[maybe_unused]] int symbol_in_slot, [[maybe_unused]] int fft) { return 0; }  // TODO W 3.2
inline int slot_samples([[maybe_unused]] int fft) { return 0; }  // TODO W 3.2: 7 symbols incl. CPs (2048 -> 15360)
inline double cp_duration_us([[maybe_unused]] int symbol_in_slot, [[maybe_unused]] int fft) { return 0.0; }  // TODO W 3.2
inline int resource_elements_per_subframe([[maybe_unused]] int nrb) { return 0; }  // TODO W 3.2: nrb*12*14
// Raw RE ceiling in Mbit/s: REs per ms * bits per symbol * layers / 1000.
inline double raw_peak_mbps([[maybe_unused]] int nrb, [[maybe_unused]] int bits_per_symbol, [[maybe_unused]] int layers) {
  return 0.0;  // TODO W 3.2
}

// ---- OFDMA vs SC-FDMA (W 3.3) ----
inline std::vector<cd> dft([[maybe_unused]] const std::vector<cd>& x) { return {}; }   // TODO: X[k] = sum x[n] e^{-j2pi kn/N}
inline std::vector<cd> idft([[maybe_unused]] const std::vector<cd>& X) { return {}; }  // TODO: x[n] = (1/N) sum X[k] e^{+j2pi kn/N}
inline double papr_db([[maybe_unused]] const std::vector<cd>& x) { return -1.0; }       // TODO: 10 log10(max|x|^2 / mean|x|^2)
// Put the M data symbols on bins first_bin .. first_bin+M-1 of an N-point spectrum (rest zero), then IDFT.
inline std::vector<cd> ofdm_modulate([[maybe_unused]] const std::vector<cd>& data, [[maybe_unused]] int n_fft,
                                     [[maybe_unused]] int first_bin) {
  return {};  // TODO W 3.3
}
// Same, but DFT-precode the M data symbols first.
inline std::vector<cd> scfdma_modulate([[maybe_unused]] const std::vector<cd>& data, [[maybe_unused]] int n_fft,
                                       [[maybe_unused]] int first_bin) {
  return {};  // TODO W 3.3
}
// Deterministic QPSK source (unit average power). Given, not a stub.
inline std::vector<cd> random_qpsk(int n, uint64_t seed) {
  std::vector<cd> v;
  uint64_t s = seed;
  for (int i = 0; i < n; ++i) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    const unsigned b = static_cast<unsigned>(s >> 33) & 3u;
    v.emplace_back(((b & 1u) ? -1.0 : 1.0) / std::sqrt(2.0), ((b & 2u) ? -1.0 : 1.0) / std::sqrt(2.0));
  }
  return v;
}

// ---- protocol stack and EPC (W 3.4 - 3.5) ----
enum class Layer { PHY, MAC, RLC, PDCP, RRC, NAS };
enum class Function {
  Modulation,
  HarqRetransmission,
  Scheduling,
  ArqAndSegmentation,
  CipheringAndRohc,
  RadioBearerReconfiguration,
  AuthenticationAndAttach
};
inline Layer layer_for([[maybe_unused]] Function f) { return Layer::NAS; }  // TODO W 3.4: which layer owns it?
enum class Node { UE, ENB, MME, SGW, PGW, HSS };
inline Node peer_node([[maybe_unused]] Layer l) { return Node::UE; }  // TODO W 3.4: where does the PEER of this layer live?
enum class Role { NasSignallingAndPaging, SubscriberDatabase, HandoverUserPlaneAnchor, UeIpAddressAndInternetGateway };
inline Node epc_node_for([[maybe_unused]] Role r) { return Node::UE; }  // TODO W 3.5
// QCI 1, 5, 9, 65, 69 -> packet delay budget ms (TS 23.203 Table 6.1.7; VERIFY); -1 otherwise.
inline int qci_delay_budget_ms([[maybe_unused]] int qci) { return 0; }  // TODO W 3.5

// ---- GTP-U + VoLTE sizing (W 3.5 - 3.6) ----
// Outer IPv4 + UDP + GTP-U base header, plus 4 optional bytes when seq/N-PDU/extension present.
inline int gtpu_overhead_bytes([[maybe_unused]] bool with_optional_fields) { return 0; }  // TODO W 3.5
inline int gtpu_inner_mtu([[maybe_unused]] int link_mtu, [[maybe_unused]] bool with_optional_fields) { return 0; }  // TODO
// IP-layer bitrate: (payload + header bytes) * 8 per packet, one packet every ptime_ms.
inline double ip_voice_bitrate_bps([[maybe_unused]] int payload_bytes, [[maybe_unused]] int header_bytes,
                                   [[maybe_unused]] double ptime_ms) {
  return 0.0;  // TODO W 3.6
}
inline int rtp_udp_ip_header_bytes([[maybe_unused]] bool ipv6) { return 0; }  // TODO W 3.6: RTP 12 + UDP 8 + IP 20/40

}  // namespace lte
