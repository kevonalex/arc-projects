// STUB - 03-net/02-udp-tcp-qos. Replace every TODO body. Tests: tests/test_net_qos.cpp
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace ether {

struct Delivery {
    int idx = 0;
    double sent_ms = 0;
    double delivered_ms = 0;
    bool dropped = false;
    double delay() const { return delivered_ms - sent_ms; }
};
// TODO UDP: packet i is sent at i*interval, arrives at sent+owd; packets in 'lost' have dropped=true.
inline std::vector<Delivery> simulate_udp(int n, double interval_ms, double owd_ms, const std::set<int>& lost) {
    (void)n; (void)interval_ms; (void)owd_ms; (void)lost; return {};
}
// TODO TCP: lost packet arrives at sent+owd+rto (retransmission); the app sees data IN ORDER:
//      delivered[i] = max(arrival[i], delivered[i-1]).
inline std::vector<Delivery> simulate_tcp(int n, double interval_ms, double owd_ms, const std::set<int>& lost, double rto_ms) {
    (void)n; (void)interval_ms; (void)owd_ms; (void)lost; (void)rto_ms; return {};
}
inline double worst_delay_ms(const std::vector<Delivery>& v) { (void)v; return -1; }          // TODO: ignore dropped
inline int unusable_count(const std::vector<Delivery>& v, double playout_delay_ms) { (void)v; (void)playout_delay_ms; return -1; }  // TODO: dropped or delay>playout

enum class TcpState { Closed, Listen, SynSent, SynReceived, Established };
enum class TcpEvent { ActiveOpen, PassiveOpen, RecvSyn, RecvSynAck, RecvAck, RecvRst };
// TODO: RFC 9293 handshake subset incl. simultaneous open (SynSent+RecvSyn -> SynReceived).
// Unknown event in a state: stay. RST in SynReceived: back to Listen if came_from_listen else Closed.
inline TcpState tcp_next_state(TcpState s, TcpEvent e, bool came_from_listen = true) { (void)e; (void)came_from_listen; return s; }

inline double bdp_bytes(double rate_bps, double rtt_ms) { (void)rate_bps; (void)rtt_ms; return -1; }              // TODO
inline double max_tcp_throughput_bps(double window_bytes, double rtt_ms) { (void)window_bytes; (void)rtt_ms; return -1; }  // TODO
inline int window_scale_for(double needed_bytes) { (void)needed_bytes; return -2; }       // TODO smallest s in 0..14 with 65535<<s >= needed, else -1
inline double mathis_throughput_bps(double mss_bytes, double rtt_ms, double loss_prob) { (void)mss_bytes; (void)rtt_ms; (void)loss_prob; return -1; }  // TODO MSS/RTT*1.22/sqrt(p)
inline double slow_start_cwnd(double init_cwnd_mss, int rtts) { (void)init_cwnd_mss; (void)rtts; return -1; }     // TODO doubles per RTT
inline double reno_ssthresh_after_loss(double cwnd_mss) { (void)cwnd_mss; return -1; }                            // TODO max(cwnd/2, 2)

// TODO RFC 6298: first sample: srtt=R, rttvar=R/2; later: rttvar=3/4*rttvar+1/4*|srtt-R| (old srtt!), srtt=7/8*srtt+1/8*R;
//      rto = max(srtt + 4*rttvar, min_rto_ms)
struct RtoEstimator {
    bool have = false;
    double srtt = 0, rttvar = 0, min_rto_ms = 0;
    double update(double sample_ms) { (void)sample_ms; return -1; }
    double rto() const { return -1; }
};

inline bool nagle_should_send(size_t pending_bytes, size_t unacked_bytes, size_t mss) { (void)pending_bytes; (void)unacked_bytes; (void)mss; return false; }  // TODO
struct NagleResult { int segments = 0; double max_write_to_send_ms = 0; };
// TODO: app writes frame_bytes every interval_ms (n times); each segment is ACKed rtt_ms after sending.
// With Nagle: hold small data while a segment is unacknowledged; release when the ACK arrives.
inline NagleResult nagle_simulate(int n, size_t frame_bytes, double interval_ms, double rtt_ms, bool nagle_on, size_t mss = 1460) {
    (void)n; (void)frame_bytes; (void)interval_ms; (void)rtt_ms; (void)nagle_on; (void)mss; return {-1, -1};
}

struct Fragment { size_t offset_units = 0; size_t data_len = 0; bool mf = false; };
// TODO: all fragments but the last carry a multiple of 8 bytes; offset in 8-byte units; {} if mtu < ip_hdr+8
inline std::vector<Fragment> fragment_ipv4(size_t payload, size_t mtu, size_t ip_hdr = 20) { (void)payload; (void)mtu; (void)ip_hdr; return {}; }
inline size_t max_udp_payload(size_t mtu, size_t ip_hdr = 20) { (void)mtu; (void)ip_hdr; return 0; }   // TODO
inline size_t total_wire_bytes_with_fragments(size_t payload, size_t mtu, size_t ip_hdr = 20) { (void)payload; (void)mtu; (void)ip_hdr; return 0; }  // TODO sum of ip_hdr+data

inline bool is_ipv4_multicast(uint32_t a) { (void)a; return false; }                    // TODO 224.0.0.0/4
inline std::array<uint8_t, 6> ipv4_mcast_mac(uint32_t group) { (void)group; return {}; }  // TODO 01:00:5e + low 23 bits
inline int mcast_groups_per_mac() { return 0; }                                           // TODO 2^(28-23)
inline uint16_t checksum16(const uint8_t* d, size_t n) { (void)d; (void)n; return 0; }    // TODO RFC 1071
inline std::array<uint8_t, 8> build_igmpv2(uint8_t type, uint8_t max_resp, uint32_t group) { (void)type; (void)max_resp; (void)group; return {}; }  // TODO

inline uint8_t dscp_to_tos(uint8_t dscp, uint8_t ecn = 0) { (void)dscp; (void)ecn; return 0; }  // TODO
inline uint8_t tos_to_dscp(uint8_t tos) { (void)tos; return 0; }                                // TODO
inline std::string dscp_name(uint8_t d) { (void)d; return ""; }   // TODO BE, CSn (8n), AFxy (8x+2y), EF=46, 44=VOICE-ADMIT, else "?"

}  // namespace ether
