#include <gtest/gtest.h>
#include "net_qos.hpp"

using namespace ether;

// ---- HOL blocking (worksheet Part B): 10 voice frames, 20 ms apart, 30 ms one-way, packet 3 lost ----
TEST(Hol, UdpOnlyTheLostPacketIsMissing) {
    auto v = simulate_udp(10, 20, 30, {3});
    ASSERT_EQ(v.size(), 10u);
    EXPECT_TRUE(v[3].dropped);
    EXPECT_DOUBLE_EQ(v[4].delivered_ms, 110);
    EXPECT_DOUBLE_EQ(worst_delay_ms(v), 30);
    EXPECT_EQ(unusable_count(v, 60), 1);          // only the lost one
}
TEST(Hol, TcpBlocksEveryLaterPacketUntilRetransmit) {
    auto v = simulate_tcp(10, 20, 30, {3}, 200);  // retransmit arrives 60+30+200 = 290
    ASSERT_EQ(v.size(), 10u);
    EXPECT_DOUBLE_EQ(v[2].delivered_ms, 70);      // before the loss: normal
    EXPECT_DOUBLE_EQ(v[3].delivered_ms, 290);
    EXPECT_DOUBLE_EQ(v[4].delivered_ms, 290);     // arrived at 110 but stuck behind packet 3
    EXPECT_DOUBLE_EQ(v[9].delivered_ms, 290);     // arrived at 210
    EXPECT_DOUBLE_EQ(v[4].delay(), 210);
    EXPECT_DOUBLE_EQ(worst_delay_ms(v), 230);     // packet 3: 290 - 60
    EXPECT_EQ(unusable_count(v, 60), 7);          // packets 3..9 miss a 60 ms playout deadline
}

// ---- TCP handshake ----
TEST(Handshake, ThreeWay) {
    TcpState c = TcpState::Closed, s = TcpState::Closed;
    s = tcp_next_state(s, TcpEvent::PassiveOpen); EXPECT_EQ(s, TcpState::Listen);
    c = tcp_next_state(c, TcpEvent::ActiveOpen);  EXPECT_EQ(c, TcpState::SynSent);          // client sends SYN
    s = tcp_next_state(s, TcpEvent::RecvSyn);     EXPECT_EQ(s, TcpState::SynReceived);      // server sends SYN+ACK
    c = tcp_next_state(c, TcpEvent::RecvSynAck);  EXPECT_EQ(c, TcpState::Established);      // client sends ACK
    s = tcp_next_state(s, TcpEvent::RecvAck);     EXPECT_EQ(s, TcpState::Established);
}
TEST(Handshake, SimultaneousOpenAndReset) {
    EXPECT_EQ(tcp_next_state(TcpState::SynSent, TcpEvent::RecvSyn), TcpState::SynReceived);
    EXPECT_EQ(tcp_next_state(TcpState::SynSent, TcpEvent::RecvRst), TcpState::Closed);
    EXPECT_EQ(tcp_next_state(TcpState::SynReceived, TcpEvent::RecvRst, true), TcpState::Listen);
    EXPECT_EQ(tcp_next_state(TcpState::SynReceived, TcpEvent::RecvRst, false), TcpState::Closed);
    EXPECT_EQ(tcp_next_state(TcpState::Listen, TcpEvent::RecvAck), TcpState::Listen);     // stray ACK ignored
    EXPECT_EQ(tcp_next_state(TcpState::Established, TcpEvent::RecvRst), TcpState::Closed);
}

// ---- windows & congestion ----
TEST(Window, BdpAndThroughput) {
    EXPECT_DOUBLE_EQ(bdp_bytes(20e6, 200), 500000.0);
    EXPECT_NEAR(max_tcp_throughput_bps(65535, 200), 2621400.0, 1.0);
    EXPECT_EQ(window_scale_for(500000), 3);
    EXPECT_EQ(window_scale_for(65535), 0);
    EXPECT_EQ(window_scale_for(65536), 1);
    EXPECT_EQ(window_scale_for(1e12), -1);
}
TEST(Congestion, SlowStartAndLoss) {
    EXPECT_DOUBLE_EQ(slow_start_cwnd(10, 3), 80);       // IW10 -> 20 -> 40 -> 80 MSS
    EXPECT_DOUBLE_EQ(reno_ssthresh_after_loss(40), 20);
    EXPECT_DOUBLE_EQ(reno_ssthresh_after_loss(3), 2);
}
TEST(Congestion, MathisFormula) {
    // MSS 1460 B, RTT 100 ms, p = 1%:  (1460*8/0.1) * 1.22 / 0.1 = 1,424,960 bit/s
    EXPECT_NEAR(mathis_throughput_bps(1460, 100, 0.01), 1424960.0, 1.0);
}
TEST(Rto, Rfc6298) {
    RtoEstimator e; e.min_rto_ms = 0;
    EXPECT_NEAR(e.update(100), 300.0, 1e-9);     // srtt 100, rttvar 50 -> 100 + 4*50
    EXPECT_NEAR(e.update(120), 272.5, 1e-9);     // rttvar 42.5, srtt 102.5
    EXPECT_NEAR(e.srtt, 102.5, 1e-9);
    RtoEstimator f; f.min_rto_ms = 1000;
    EXPECT_NEAR(f.update(100), 1000.0, 1e-9);    // clamped to the minimum
}

// ---- Nagle ----
TEST(Nagle, Rule) {
    EXPECT_TRUE(nagle_should_send(100, 0, 1460));       // nothing in flight
    EXPECT_FALSE(nagle_should_send(100, 50, 1460));     // small data + unacked data -> wait
    EXPECT_TRUE(nagle_should_send(1460, 50, 1460));     // full segment
}
TEST(Nagle, VoiceFramesGetBatchedAndDelayed) {
    NagleResult off = nagle_simulate(10, 160, 20, 100, false);
    EXPECT_EQ(off.segments, 10);
    EXPECT_DOUBLE_EQ(off.max_write_to_send_ms, 0);
    NagleResult on = nagle_simulate(10, 160, 20, 100, true);
    EXPECT_EQ(on.segments, 3);                          // t=0, t=100 (frames 1-4), t=200 (frames 5-9)
    EXPECT_DOUBLE_EQ(on.max_write_to_send_ms, 100);
}

// ---- MTU / fragmentation ----
TEST(Frag, ThreeThousandBytesOverMtu1500) {
    auto f = fragment_ipv4(3000, 1500);
    ASSERT_EQ(f.size(), 3u);
    EXPECT_EQ(f[0].data_len, 1480u); EXPECT_EQ(f[0].offset_units, 0u);   EXPECT_TRUE(f[0].mf);
    EXPECT_EQ(f[1].data_len, 1480u); EXPECT_EQ(f[1].offset_units, 185u); EXPECT_TRUE(f[1].mf);
    EXPECT_EQ(f[2].data_len, 40u);   EXPECT_EQ(f[2].offset_units, 370u); EXPECT_FALSE(f[2].mf);
    EXPECT_EQ(total_wire_bytes_with_fragments(3000, 1500), 3060u);   // 3000 + 3*20
}
TEST(Frag, FitsAndEdgeCases) {
    auto one = fragment_ipv4(1480, 1500);
    ASSERT_EQ(one.size(), 1u); EXPECT_FALSE(one[0].mf);
    auto two = fragment_ipv4(1481, 1500);
    ASSERT_EQ(two.size(), 2u); EXPECT_EQ(two[1].data_len, 1u);
    EXPECT_TRUE(fragment_ipv4(100, 27).empty());                 // < hdr + 8
    auto odd = fragment_ipv4(100, 50);                           // (50-20)=30 -> rounded down to 24
    ASSERT_EQ(odd.size(), 5u); EXPECT_EQ(odd[0].data_len, 24u); EXPECT_EQ(odd[4].data_len, 4u);
    EXPECT_EQ(max_udp_payload(1500), 1472u);
}

// ---- multicast ----
TEST(Multicast, MacMapping) {
    EXPECT_TRUE(is_ipv4_multicast(0xE0000001));    // 224.0.0.1
    EXPECT_TRUE(is_ipv4_multicast(0xEFFFFFFF));
    EXPECT_FALSE(is_ipv4_multicast(0xF0000000));
    EXPECT_FALSE(is_ipv4_multicast(0x0A000001));
    auto m = ipv4_mcast_mac(0xEF010101);           // 239.1.1.1
    std::array<uint8_t, 6> expect{0x01, 0x00, 0x5E, 0x01, 0x01, 0x01};
    EXPECT_EQ(m, expect);
    // 224.129.1.1 and 225.1.1.1 share a MAC (bit 23 of the group is dropped)
    EXPECT_EQ(ipv4_mcast_mac(0xE0810101), ipv4_mcast_mac(0xE1010101));
    EXPECT_EQ(mcast_groups_per_mac(), 32);
}
TEST(Multicast, Igmpv2Report) {
    auto m = build_igmpv2(0x16, 0, 0xEF010101);    // report for 239.1.1.1
    EXPECT_EQ(m[0], 0x16); EXPECT_EQ(m[4], 0xEF);
    EXPECT_EQ(checksum16(m.data(), 8), 0);         // valid message sums to zero
    EXPECT_EQ((m[2] << 8) | m[3], 0xF9FC);         // hand-computed in answers.md
}

// ---- DSCP ----
TEST(Dscp, TosAndNames) {
    EXPECT_EQ(dscp_to_tos(46), 0xB8);
    EXPECT_EQ(dscp_to_tos(46, 3), 0xBB);
    EXPECT_EQ(tos_to_dscp(0xB8), 46);
    EXPECT_EQ(dscp_name(46), "EF");
    EXPECT_EQ(dscp_name(0), "BE");
    EXPECT_EQ(dscp_name(48), "CS6");
    EXPECT_EQ(dscp_name(34), "AF41");   // 8*4 + 2*1
    EXPECT_EQ(dscp_name(26), "AF31");
    EXPECT_EQ(dscp_name(38), "AF43");
    EXPECT_EQ(dscp_name(44), "VOICE-ADMIT");
    EXPECT_EQ(dscp_name(1), "?");
}
