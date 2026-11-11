// Tests for Pack 06/02. Same file is compiled against the stubs and the solutions.
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "lte.hpp"

using namespace lte;

// ---------------------------------------------------------------- numerology (W 3.1 - 3.2)
TEST(Numerology, BandwidthToResourceBlocks) {
  EXPECT_EQ(rb_for_bandwidth_mhz(1.4), 6);
  EXPECT_EQ(rb_for_bandwidth_mhz(3.0), 15);
  EXPECT_EQ(rb_for_bandwidth_mhz(5.0), 25);
  EXPECT_EQ(rb_for_bandwidth_mhz(10.0), 50);
  EXPECT_EQ(rb_for_bandwidth_mhz(15.0), 75);
  EXPECT_EQ(rb_for_bandwidth_mhz(20.0), 100);
  EXPECT_EQ(rb_for_bandwidth_mhz(7.0), -1);
}
TEST(Numerology, FftAndSampleRate) {
  EXPECT_EQ(fft_size_for_rb(6), 128);
  EXPECT_EQ(fft_size_for_rb(100), 2048);
  EXPECT_EQ(fft_size_for_rb(75), 1536);
  EXPECT_EQ(fft_size_for_rb(7), -1);
  EXPECT_DOUBLE_EQ(sample_rate_hz(100), 30.72e6);
  EXPECT_DOUBLE_EQ(sample_rate_hz(25), 7.68e6);
  EXPECT_DOUBLE_EQ(sample_rate_hz(6), 1.92e6);
  EXPECT_DOUBLE_EQ(sample_rate_hz(8), -1.0);
}
TEST(Numerology, SymbolAndCyclicPrefix) {
  EXPECT_NEAR(useful_symbol_us(), 66.6667, 1e-3);
  EXPECT_EQ(cp_samples(0, 2048), 160);
  EXPECT_EQ(cp_samples(1, 2048), 144);
  EXPECT_EQ(cp_samples(6, 2048), 144);
  EXPECT_EQ(cp_samples(0, 128), 10);
  EXPECT_EQ(cp_samples(3, 128), 9);
  EXPECT_NEAR(cp_duration_us(0, 2048), 5.2083, 1e-3);
  EXPECT_NEAR(cp_duration_us(1, 2048), 4.6875, 1e-4);
  EXPECT_EQ(slot_samples(2048), 15360);  // 0.5 ms at 30.72 Msps
  EXPECT_EQ(slot_samples(128), 960);     // 0.5 ms at 1.92 Msps
}
TEST(Numerology, ResourceElementsAndPeak) {
  EXPECT_EQ(resource_elements_per_subframe(100), 16800);
  EXPECT_NEAR(raw_peak_mbps(100, 6, 1), 100.8, 1e-9);
  EXPECT_NEAR(raw_peak_mbps(100, 6, 4), 403.2, 1e-9);
  EXPECT_NEAR(raw_peak_mbps(25, 2, 1), 8.4, 1e-9);
}

// ---------------------------------------------------------------- DFT, PAPR, OFDMA vs SC-FDMA (W 3.3)
TEST(Dft, RoundTripAndKnownTransform) {
  std::vector<cd> x = {cd(1, 0), cd(0, 0), cd(0, 0), cd(0, 0)};
  auto X = dft(x);
  ASSERT_EQ(X.size(), 4u);
  for (auto& v : X) EXPECT_NEAR(std::abs(v - cd(1, 0)), 0.0, 1e-12);  // impulse -> flat spectrum
  auto q = random_qpsk(16, 7);
  auto back = idft(dft(q));
  ASSERT_EQ(back.size(), q.size());
  for (size_t i = 0; i < q.size(); ++i) EXPECT_NEAR(std::abs(back[i] - q[i]), 0.0, 1e-9);
}
TEST(Papr, KnownValues) {
  EXPECT_NEAR(papr_db({cd(1, 0), cd(0, 1), cd(-1, 0), cd(0, -1)}), 0.0, 1e-12);  // constant envelope
  EXPECT_NEAR(papr_db({cd(2, 0), cd(0, 0), cd(0, 0), cd(0, 0)}), 6.0206, 1e-3);  // one spike in 4: 10log10(4)
}
TEST(ScFdma, NoSpreadingWhenMEqualsN) {
  auto q = random_qpsk(32, 11);
  auto t = scfdma_modulate(q, 32, 0);  // DFT then IDFT of the same size is the identity
  ASSERT_EQ(t.size(), q.size());
  for (size_t i = 0; i < q.size(); ++i) EXPECT_NEAR(std::abs(t[i] - q[i]), 0.0, 1e-9);
}
TEST(ScFdma, OfdmMapsOntoRequestedBins) {
  auto q = random_qpsk(8, 3);
  auto t = ofdm_modulate(q, 32, 5);
  ASSERT_EQ(t.size(), 32u);
  auto spec = dft(t);
  for (int k = 0; k < 32; ++k) {
    cd want = (k >= 5 && k < 13) ? q[static_cast<size_t>(k - 5)] : cd(0, 0);
    EXPECT_NEAR(std::abs(spec[static_cast<size_t>(k)] - want), 0.0, 1e-9) << "bin " << k;
  }
}
TEST(ScFdma, LowerPaprThanOfdmOnAverage) {
  // 64 QPSK symbols on contiguous bins of a 256-point IFFT, 150 random symbols each.
  double sum_ofdm = 0, sum_sc = 0;
  const int trials = 150;
  for (int t = 0; t < trials; ++t) {
    auto q = random_qpsk(64, 1000 + static_cast<uint64_t>(t));
    sum_ofdm += papr_db(ofdm_modulate(q, 256, 40));
    sum_sc += papr_db(scfdma_modulate(q, 256, 40));
  }
  const double mean_ofdm = sum_ofdm / trials, mean_sc = sum_sc / trials;
  EXPECT_GT(mean_ofdm, 7.0);               // OFDM: Gaussian-like envelope, peaks well above average
  EXPECT_LT(mean_sc, mean_ofdm - 1.5);     // SC-FDMA: markedly lower PAPR -> cheaper, more efficient PA
}

// ---------------------------------------------------------------- stack and EPC (W 3.4 - 3.5)
TEST(Stack, FunctionToLayer) {
  EXPECT_EQ(layer_for(Function::Modulation), Layer::PHY);
  EXPECT_EQ(layer_for(Function::HarqRetransmission), Layer::MAC);
  EXPECT_EQ(layer_for(Function::Scheduling), Layer::MAC);
  EXPECT_EQ(layer_for(Function::ArqAndSegmentation), Layer::RLC);
  EXPECT_EQ(layer_for(Function::CipheringAndRohc), Layer::PDCP);
  EXPECT_EQ(layer_for(Function::RadioBearerReconfiguration), Layer::RRC);
  EXPECT_EQ(layer_for(Function::AuthenticationAndAttach), Layer::NAS);
}
TEST(Stack, PeerNodes) {
  EXPECT_EQ(peer_node(Layer::PHY), Node::ENB);
  EXPECT_EQ(peer_node(Layer::PDCP), Node::ENB);
  EXPECT_EQ(peer_node(Layer::RRC), Node::ENB);
  EXPECT_EQ(peer_node(Layer::NAS), Node::MME);  // transparent through the eNB
}
TEST(Epc, NodeRoles) {
  EXPECT_EQ(epc_node_for(Role::NasSignallingAndPaging), Node::MME);
  EXPECT_EQ(epc_node_for(Role::SubscriberDatabase), Node::HSS);
  EXPECT_EQ(epc_node_for(Role::HandoverUserPlaneAnchor), Node::SGW);
  EXPECT_EQ(epc_node_for(Role::UeIpAddressAndInternetGateway), Node::PGW);
}
TEST(Epc, QciDelayBudgets) {
  EXPECT_EQ(qci_delay_budget_ms(1), 100);
  EXPECT_EQ(qci_delay_budget_ms(5), 100);
  EXPECT_EQ(qci_delay_budget_ms(9), 300);
  EXPECT_EQ(qci_delay_budget_ms(65), 75);
  EXPECT_EQ(qci_delay_budget_ms(69), 60);
  EXPECT_EQ(qci_delay_budget_ms(42), -1);
}

// ---------------------------------------------------------------- GTP-U + VoLTE sizing (W 3.5 - 3.6)
TEST(Gtpu, OverheadAndMtu) {
  EXPECT_EQ(gtpu_overhead_bytes(false), 36);
  EXPECT_EQ(gtpu_overhead_bytes(true), 40);
  EXPECT_EQ(gtpu_inner_mtu(1500, false), 1464);
  EXPECT_EQ(gtpu_inner_mtu(1500, true), 1460);
}
TEST(Volte, HeaderOverheadMotivatesRohc) {
  EXPECT_EQ(rtp_udp_ip_header_bytes(false), 40);
  EXPECT_EQ(rtp_udp_ip_header_bytes(true), 60);
  // AMR-NB 12.2 kbit/s: 31-byte payload every 20 ms.
  EXPECT_NEAR(ip_voice_bitrate_bps(31, 0, 20.0), 12400.0, 1e-9);
  EXPECT_NEAR(ip_voice_bitrate_bps(31, 40, 20.0), 28400.0, 1e-9);  // headers more than double the rate
  EXPECT_NEAR(ip_voice_bitrate_bps(31, 3, 20.0), 13600.0, 1e-9);   // ROHC-compressed (~3 bytes)
}
