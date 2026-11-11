// Reference solution - 03-net/02-udp-tcp-qos
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

// ---------------------------------------------------------------- HOL blocking timeline
struct Delivery {
    int idx = 0;
    double sent_ms = 0;
    double delivered_ms = 0;   // time the APPLICATION can read it; meaningless if dropped
    bool dropped = false;      // UDP: lost for good
    double delay() const { return delivered_ms - sent_ms; }
};

// n packets sent every interval_ms; one-way delay owd_ms. Packets in 'lost' vanish on first transmission.
inline std::vector<Delivery> simulate_udp(int n, double interval_ms, double owd_ms, const std::set<int>& lost) {
    std::vector<Delivery> v;
    for (int i = 0; i < n; ++i) {
        Delivery d; d.idx = i; d.sent_ms = i * interval_ms;
        d.dropped = lost.count(i) > 0;
        d.delivered_ms = d.sent_ms + owd_ms;
        v.push_back(d);
    }
    return v;
}
// TCP: a lost packet is retransmitted and arrives at sent + rto + owd. The kernel hands data to the app IN ORDER,
// so every later packet waits: delivered[i] = max(arrival[i], delivered[i-1]).
inline std::vector<Delivery> simulate_tcp(int n, double interval_ms, double owd_ms, const std::set<int>& lost, double rto_ms) {
    std::vector<Delivery> v;
    double prev = 0;
    for (int i = 0; i < n; ++i) {
        Delivery d; d.idx = i; d.sent_ms = i * interval_ms;
        double arrival = d.sent_ms + owd_ms + (lost.count(i) ? rto_ms : 0.0);
        d.delivered_ms = std::max(arrival, prev);
        prev = d.delivered_ms;
        v.push_back(d);
    }
    return v;
}
inline double worst_delay_ms(const std::vector<Delivery>& v) {
    double w = 0;
    for (const auto& d : v) if (!d.dropped) w = std::max(w, d.delay());
    return w;
}
// Packets that a jitter buffer with a fixed playout delay (from SEND time) would have to discard or conceal:
// dropped ones plus those arriving later than playout_delay_ms after sending.
inline int unusable_count(const std::vector<Delivery>& v, double playout_delay_ms) {
    int c = 0;
    for (const auto& d : v) if (d.dropped || d.delay() > playout_delay_ms) ++c;
    return c;
}

// ---------------------------------------------------------------- TCP handshake as a state machine
enum class TcpState { Closed, Listen, SynSent, SynReceived, Established };
enum class TcpEvent { ActiveOpen, PassiveOpen, RecvSyn, RecvSynAck, RecvAck, RecvRst };
inline TcpState tcp_next_state(TcpState s, TcpEvent e, bool came_from_listen = true) {
    switch (s) {
        case TcpState::Closed:
            if (e == TcpEvent::ActiveOpen)  return TcpState::SynSent;   // send SYN
            if (e == TcpEvent::PassiveOpen) return TcpState::Listen;
            break;
        case TcpState::Listen:
            if (e == TcpEvent::RecvSyn) return TcpState::SynReceived;   // send SYN+ACK
            break;
        case TcpState::SynSent:
            if (e == TcpEvent::RecvSynAck) return TcpState::Established; // send ACK
            if (e == TcpEvent::RecvSyn)    return TcpState::SynReceived; // simultaneous open
            if (e == TcpEvent::RecvRst)    return TcpState::Closed;
            break;
        case TcpState::SynReceived:
            if (e == TcpEvent::RecvAck) return TcpState::Established;
            if (e == TcpEvent::RecvRst) return came_from_listen ? TcpState::Listen : TcpState::Closed;
            break;
        case TcpState::Established:
            if (e == TcpEvent::RecvRst) return TcpState::Closed;
            break;
    }
    return s;   // event not meaningful in this state: ignore
}

// ---------------------------------------------------------------- windows, throughput, congestion
inline double bdp_bytes(double rate_bps, double rtt_ms) { return rate_bps * (rtt_ms / 1000.0) / 8.0; }
inline double max_tcp_throughput_bps(double window_bytes, double rtt_ms) { return window_bytes * 8.0 / (rtt_ms / 1000.0); }
// smallest window-scale shift s (0..14) such that 65535<<s >= needed_bytes; -1 if impossible
inline int window_scale_for(double needed_bytes) {
    for (int s = 0; s <= 14; ++s) if (65535.0 * double(1u << s) >= needed_bytes) return s;
    return -1;
}
// Mathis et al.: throughput ~ (MSS/RTT) * 1.22 / sqrt(p)    [bits per second]   VERIFY constant sqrt(3/2)=1.2247
inline double mathis_throughput_bps(double mss_bytes, double rtt_ms, double loss_prob) {
    return (mss_bytes * 8.0 / (rtt_ms / 1000.0)) * 1.22 / std::sqrt(loss_prob);
}
inline double slow_start_cwnd(double init_cwnd_mss, int rtts) { return init_cwnd_mss * std::pow(2.0, rtts); }
inline double reno_ssthresh_after_loss(double cwnd_mss) { return std::max(cwnd_mss / 2.0, 2.0); }

// RFC 6298 retransmission-timeout estimator. alpha=1/8, beta=1/4, K=4.
struct RtoEstimator {
    bool have = false;
    double srtt = 0, rttvar = 0, min_rto_ms = 0;
    double update(double sample_ms) {
        if (!have) { srtt = sample_ms; rttvar = sample_ms / 2.0; have = true; }
        else {
            rttvar = 0.75 * rttvar + 0.25 * std::fabs(srtt - sample_ms);   // uses OLD srtt
            srtt = 0.875 * srtt + 0.125 * sample_ms;
        }
        return rto();
    }
    double rto() const { return std::max(srtt + 4.0 * rttvar, min_rto_ms); }
};

// ---------------------------------------------------------------- Nagle
// Nagle's rule: send now if a full segment is ready OR nothing is in flight; otherwise wait for the ACK.
inline bool nagle_should_send(size_t pending_bytes, size_t unacked_bytes, size_t mss) {
    return pending_bytes >= mss || unacked_bytes == 0;
}
struct NagleResult { int segments = 0; double max_write_to_send_ms = 0; };
// App writes frame_bytes every interval_ms (n times). Every segment is acknowledged rtt_ms after it is sent.
inline NagleResult nagle_simulate(int n, size_t frame_bytes, double interval_ms, double rtt_ms, bool nagle_on, size_t mss = 1460) {
    NagleResult r;
    size_t pending = 0; double first_write = 0, ack_at = -1.0;   // ack_at < 0: nothing in flight
    auto send = [&](double t) {
        ++r.segments; r.max_write_to_send_ms = std::max(r.max_write_to_send_ms, t - first_write);
        pending = 0; ack_at = t + rtt_ms;
    };
    for (int i = 0; i < n; ++i) {
        double t = i * interval_ms;
        if (pending > 0 && ack_at >= 0 && ack_at <= t) send(ack_at);       // ACK arrived earlier: release held data
        if (pending == 0) first_write = t;
        pending += frame_bytes;
        bool in_flight = ack_at >= 0 && ack_at > t;
        if (!nagle_on || nagle_should_send(pending, in_flight ? 1 : 0, mss)) send(t);
    }
    if (pending > 0) send(std::max(ack_at, (n - 1) * interval_ms));
    return r;
}

// ---------------------------------------------------------------- MTU / fragmentation
struct Fragment { size_t offset_units = 0; size_t data_len = 0; bool mf = false; };
// Fragment an IP payload of 'payload' bytes over a link with 'mtu' (IP header 'ip_hdr' bytes per fragment).
// Every fragment except the last must carry a multiple of 8 data bytes. Returns {} if no data fits.
inline std::vector<Fragment> fragment_ipv4(size_t payload, size_t mtu, size_t ip_hdr = 20) {
    std::vector<Fragment> out;
    if (mtu < ip_hdr + 8) return out;
    if (payload + ip_hdr <= mtu) { out.push_back({0, payload, false}); return out; }
    size_t per = ((mtu - ip_hdr) / 8) * 8;
    size_t off = 0;
    while (off < payload) {
        size_t len = std::min(per, payload - off);
        bool more = off + len < payload;
        out.push_back({off / 8, len, more});
        off += len;
    }
    return out;
}
inline size_t max_udp_payload(size_t mtu, size_t ip_hdr = 20) { return mtu - ip_hdr - 8; }
inline size_t total_wire_bytes_with_fragments(size_t payload, size_t mtu, size_t ip_hdr = 20) {
    size_t t = 0; for (const auto& f : fragment_ipv4(payload, mtu, ip_hdr)) t += ip_hdr + f.data_len; return t;
}

// ---------------------------------------------------------------- multicast / IGMP
inline bool is_ipv4_multicast(uint32_t a) { return (a >> 28) == 0xE; }                   // 224.0.0.0/4
inline std::array<uint8_t, 6> ipv4_mcast_mac(uint32_t group) {                             // 01:00:5e + low 23 bits
    return {0x01, 0x00, 0x5E, static_cast<uint8_t>((group >> 16) & 0x7F), static_cast<uint8_t>(group >> 8), static_cast<uint8_t>(group)};
}
inline int mcast_groups_per_mac() { return 1 << (28 - 23); }   // 28 significant group bits, 23 copied -> 2^5 = 32
inline uint16_t checksum16(const uint8_t* d, size_t n) {
    uint32_t s = 0; size_t i = 0;
    for (; i + 1 < n; i += 2) s += (d[i] << 8) | d[i + 1];
    if (i < n) s += d[i] << 8;
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return static_cast<uint16_t>(~s);
}
// IGMPv2 (RFC 2236) 8-byte message: type, max-resp(1/10 s), checksum, group.  0x16 = v2 membership report.
inline std::array<uint8_t, 8> build_igmpv2(uint8_t type, uint8_t max_resp, uint32_t group) {
    std::array<uint8_t, 8> m{type, max_resp, 0, 0, uint8_t(group >> 24), uint8_t(group >> 16), uint8_t(group >> 8), uint8_t(group)};
    uint16_t c = checksum16(m.data(), 8);
    m[2] = uint8_t(c >> 8); m[3] = uint8_t(c);
    return m;
}

// ---------------------------------------------------------------- DSCP / TOS
inline uint8_t dscp_to_tos(uint8_t dscp, uint8_t ecn = 0) { return static_cast<uint8_t>((dscp << 2) | (ecn & 3)); }
inline uint8_t tos_to_dscp(uint8_t tos) { return tos >> 2; }
inline std::string dscp_name(uint8_t d) {
    if (d == 0) return "BE";
    if (d == 46) return "EF";
    if (d == 44) return "VOICE-ADMIT";                    // RFC 5865, VERIFY
    if (d % 8 == 0 && d <= 56) return "CS" + std::to_string(d / 8);
    int cls = d >> 3, drop = (d >> 1) & 3;                // AFxy = 8x + 2y
    if ((d & 1) == 0 && cls >= 1 && cls <= 4 && drop >= 1 && drop <= 3) return "AF" + std::to_string(cls) + std::to_string(drop);
    return "?";
}

}  // namespace ether
