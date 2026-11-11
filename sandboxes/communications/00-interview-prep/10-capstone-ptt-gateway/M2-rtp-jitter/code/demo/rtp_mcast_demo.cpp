// rtp_mcast_demo -- real UDP multicast loopback: a "radio side" thread sends 20 ms mu-law RTP packets (with artificial
// network jitter and loss) to talkgroup 7's group address; a "gateway side" receiver joins the group, feeds the
// jitter buffer and plays out on a 20 ms clock.  POSIX only; built against the reference solution; NOT part of ctest
// (multicast loopback depends on the host's network configuration - e.g. it needs a default route).
//
//   ./rtp_mcast_demo [target_frames=3]
//
// Synthetic tone only.  Never point this at real radio traffic.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include "rtp_jitter.hpp"

#if defined(__unix__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using Clock = std::chrono::steady_clock;
static const int kPort = 5004;
static const uint16_t kTalkgroup = 7;

static void sender(int packets) {
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  unsigned char ttl = 1, loop = 1;  // TTL 1: never leaves the local network; loop: deliver to ourselves
  ::setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof ttl);
  ::setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof loop);
  sockaddr_in to{};
  to.sin_family = AF_INET;
  to.sin_port = htons(kPort);
  to.sin_addr.s_addr = htonl(gw::talkgroup_group_addr(kTalkgroup));
  const auto t0 = Clock::now();
  uint32_t rng = 1;
  struct Pending { int due_ms; std::vector<uint8_t> pkt; };
  std::vector<Pending> pend;
  for (int i = 0; i < packets; ++i) {
    std::vector<uint8_t> pl(160);
    for (int k = 0; k < 160; ++k) pl[static_cast<size_t>(k)] = gw::mulaw_encode(static_cast<int16_t>(8000 * std::sin(2 * M_PI * 440.0 * (i * 160 + k) / 8000.0)));
    gw::RtpHeader h;
    h.seq = static_cast<uint16_t>(i);
    h.timestamp = static_cast<uint32_t>(i) * 160;
    h.ssrc = 0x1234;
    rng = rng * 1664525u + 1013904223u;
    if (i % 17 == 9) continue;  // dropped by the "network"
    const int jitter_ms = static_cast<int>((rng >> 16) % 45);
    pend.push_back({i * 20 + jitter_ms, gw::rtp_pack(h, pl.data(), pl.size())});
  }
  size_t sent = 0;
  while (sent < pend.size()) {
    const int now_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count());
    for (auto& p : pend)
      if (!p.pkt.empty() && p.due_ms <= now_ms) {
        ::sendto(fd, p.pkt.data(), p.pkt.size(), 0, reinterpret_cast<sockaddr*>(&to), sizeof to);
        p.pkt.clear();
        ++sent;
      }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  ::close(fd);
}

int main(int argc, char** argv) {
  const int target = argc > 1 ? std::atoi(argv[1]) : 3;
  const int packets = 100;
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  int one = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(kPort);
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  if (::bind(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) { std::perror("bind"); return 1; }
  ip_mreq mreq{};
  mreq.imr_multiaddr.s_addr = htonl(gw::talkgroup_group_addr(kTalkgroup));
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof mreq) != 0) {
    std::perror("IP_ADD_MEMBERSHIP (no multicast-capable interface?)");
    return 1;
  }
  std::printf("group %s:%d, jitter-buffer target %d frames (%d ms)\n", gw::ipv4_to_string(gw::talkgroup_group_addr(kTalkgroup)).c_str(), kPort,
              target, target * 20);
  std::thread tx(sender, packets);
  gw::JitterBuffer jb(target, 32);
  gw::JitterEstimator jit;
  gw::LossCounter loss;
  const auto t0 = Clock::now();
  int slots = 0, present = 0;
  auto next_tick = t0;
  while (slots < packets) {
    timeval tv{0, 2000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    uint8_t buf[2048];
    ssize_t n = ::recv(fd, buf, sizeof buf, 0);
    if (n > 0) {
      gw::RtpHeader h;
      size_t off = 0, len = 0;
      if (gw::rtp_parse(buf, static_cast<size_t>(n), h, off, len) == gw::RtpStatus::Ok) {
        const uint32_t arrival = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t0).count() * 8 / 1000);
        jit.update(h.timestamp, arrival);
        loss.on_packet(h.seq);
        jb.push(h.seq, std::vector<uint8_t>(buf + off, buf + off + len));
      }
    }
    if (Clock::now() >= next_tick) {
      next_tick += std::chrono::milliseconds(20);
      auto f = jb.pop();
      if (f.present || f.concealed) ++slots;
      if (f.present) ++present;
    }
  }
  tx.join();
  std::printf("received %u, lost-in-network %lld, played %d/%d, concealed %u, late %u, jitter %.2f ms\n", loss.received(),
              static_cast<long long>(loss.lost()), present, packets, jb.concealed(), jb.late(), jit.jitter_ms());
  ::close(fd);
  return 0;
}
#else
int main() {
  std::puts("POSIX sockets required for this demo.");
  return 0;
}
#endif
