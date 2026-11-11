// lte.hpp  --  REFERENCE SOLUTION for Pack 06/02 (4G LTE numerology, OFDMA vs SC-FDMA, stack, EPC, VoLTE sizing).
#pragma once
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace lte {

using cd = std::complex<double>;
constexpr double kPi = 3.14159265358979323846;
constexpr double kScsHz = 15000.0;  // LTE subcarrier spacing

// ------------------------------------------------------------------ numerology (W 3.1 - 3.2)
// Channel bandwidth (MHz) -> number of 180 kHz resource blocks (TS 36.101 Table 5.6-1; VERIFY).
inline int rb_for_bandwidth_mhz(double bw) {
  if (bw == 1.4) return 6;
  if (bw == 3.0) return 15;
  if (bw == 5.0) return 25;
  if (bw == 10.0) return 50;
  if (bw == 15.0) return 75;
  if (bw == 20.0) return 100;
  return -1;
}
// Smallest convenient FFT size per RB count (sample rate = FFT * 15 kHz; 2048 -> 30.72 Msps).
inline int fft_size_for_rb(int nrb) {
  switch (nrb) {
    case 6: return 128;
    case 15: return 256;
    case 25: return 512;
    case 50: return 1024;
    case 75: return 1536;
    case 100: return 2048;
    default: return -1;
  }
}
inline double sample_rate_hz(int nrb) {
  const int n = fft_size_for_rb(nrb);
  return n < 0 ? -1.0 : n * kScsHz;
}
// Useful (post-CP) symbol length is exactly 1 / subcarrier spacing.
inline double useful_symbol_us() { return 1.0e6 / kScsHz; }
// Normal CP: first symbol of a slot has 160/2048 of the FFT size as CP, symbols 1..6 have 144/2048.
inline int cp_samples(int symbol_in_slot, int fft) { return (symbol_in_slot == 0 ? 160 : 144) * fft / 2048; }
inline int slot_samples(int fft) {
  int total = 0;
  for (int s = 0; s < 7; ++s) total += cp_samples(s, fft) + fft;
  return total;
}
inline double cp_duration_us(int symbol_in_slot, int fft) {
  return 1.0e6 * cp_samples(symbol_in_slot, fft) / (fft * kScsHz);
}
// 12 subcarriers x 14 symbols per 1 ms subframe per RB.
inline int resource_elements_per_subframe(int nrb) { return nrb * 12 * 14; }
// Raw RE ceiling in Mbit/s (no reference signals, control, coding): REs/ms * bits/symbol * layers.
inline double raw_peak_mbps(int nrb, int bits_per_symbol, int layers) {
  return resource_elements_per_subframe(nrb) * static_cast<double>(bits_per_symbol) * layers / 1000.0;
}

// ------------------------------------------------------------------ OFDMA vs SC-FDMA (W 3.3)
inline std::vector<cd> dft(const std::vector<cd>& x) {
  const size_t n = x.size();
  std::vector<cd> out(n);
  for (size_t k = 0; k < n; ++k) {
    cd acc = 0;
    for (size_t i = 0; i < n; ++i) acc += x[i] * std::polar(1.0, -2.0 * kPi * static_cast<double>(k * i % n) / static_cast<double>(n));
    out[k] = acc;
  }
  return out;
}
// x[n] = (1/N) sum_k X[k] e^{+j 2 pi k n / N}
inline std::vector<cd> idft(const std::vector<cd>& X) {
  const size_t n = X.size();
  std::vector<cd> out(n);
  for (size_t i = 0; i < n; ++i) {
    cd acc = 0;
    for (size_t k = 0; k < n; ++k) acc += X[k] * std::polar(1.0, 2.0 * kPi * static_cast<double>(k * i % n) / static_cast<double>(n));
    out[i] = acc / static_cast<double>(n);
  }
  return out;
}
// Peak-to-average power ratio in dB: 10 log10( max|x|^2 / mean|x|^2 ).
inline double papr_db(const std::vector<cd>& x) {
  double peak = 0, sum = 0;
  for (const cd& v : x) {
    const double p = std::norm(v);
    peak = p > peak ? p : peak;
    sum += p;
  }
  if (sum == 0) return 0.0;
  return 10.0 * std::log10(peak / (sum / static_cast<double>(x.size())));
}
// OFDMA symbol: place M data symbols on bins first_bin .. first_bin+M-1 of an N-point spectrum, IDFT.
inline std::vector<cd> ofdm_modulate(const std::vector<cd>& data, int n_fft, int first_bin) {
  std::vector<cd> spec(static_cast<size_t>(n_fft), cd(0, 0));
  for (size_t i = 0; i < data.size(); ++i) spec[static_cast<size_t>(first_bin) + i] = data[i];
  return idft(spec);
}
// SC-FDMA symbol: M-point DFT precoding of the data first, then the same mapping + IDFT as OFDMA.
inline std::vector<cd> scfdma_modulate(const std::vector<cd>& data, int n_fft, int first_bin) {
  return ofdm_modulate(dft(data), n_fft, first_bin);
}
// Deterministic QPSK source (unit average power) so tests are reproducible. Given, not a stub.
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

// ------------------------------------------------------------------ protocol stack and EPC (W 3.4 - 3.5)
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
inline Layer layer_for(Function f) {
  switch (f) {
    case Function::Modulation: return Layer::PHY;
    case Function::HarqRetransmission:
    case Function::Scheduling: return Layer::MAC;
    case Function::ArqAndSegmentation: return Layer::RLC;
    case Function::CipheringAndRohc: return Layer::PDCP;
    case Function::RadioBearerReconfiguration: return Layer::RRC;
    case Function::AuthenticationAndAttach: return Layer::NAS;
  }
  return Layer::PHY;
}
enum class Node { UE, ENB, MME, SGW, PGW, HSS };
// Where does this layer's peer entity live?  (NAS is the odd one out: it passes THROUGH the eNB.)
inline Node peer_node(Layer l) { return l == Layer::NAS ? Node::MME : Node::ENB; }
enum class Role { NasSignallingAndPaging, SubscriberDatabase, HandoverUserPlaneAnchor, UeIpAddressAndInternetGateway };
inline Node epc_node_for(Role r) {
  switch (r) {
    case Role::NasSignallingAndPaging: return Node::MME;
    case Role::SubscriberDatabase: return Node::HSS;
    case Role::HandoverUserPlaneAnchor: return Node::SGW;
    case Role::UeIpAddressAndInternetGateway: return Node::PGW;
  }
  return Node::MME;
}
// QCI -> packet delay budget in ms (TS 23.203 Table 6.1.7; VERIFY). -1 for unknown.
inline int qci_delay_budget_ms(int qci) {
  switch (qci) {
    case 1: return 100;   // conversational voice (GBR)
    case 5: return 100;   // IMS signalling (non-GBR)
    case 9: return 300;   // default best-effort bearer
    case 65: return 75;   // mission-critical PTT voice (GBR)
    case 69: return 60;   // mission-critical delay-sensitive signalling
    default: return -1;
  }
}

// ------------------------------------------------------------------ GTP-U + VoLTE sizing (W 3.5 - 3.6)
// Outer IPv4 (20) + UDP (8) + GTP-U base header (8) wrap every user IP packet between eNB and S-GW.
inline int gtpu_overhead_bytes(bool with_optional_fields) { return 20 + 8 + 8 + (with_optional_fields ? 4 : 0); }
inline int gtpu_inner_mtu(int link_mtu, bool with_optional_fields) { return link_mtu - gtpu_overhead_bytes(with_optional_fields); }
// IP-layer bitrate of a voice stream: (payload + headers) bytes per packet, one packet per ptime.
inline double ip_voice_bitrate_bps(int payload_bytes, int header_bytes, double ptime_ms) {
  return (payload_bytes + header_bytes) * 8.0 / (ptime_ms / 1000.0);
}
inline int rtp_udp_ip_header_bytes(bool ipv6) { return 12 + 8 + (ipv6 ? 40 : 20); }

}  // namespace lte
