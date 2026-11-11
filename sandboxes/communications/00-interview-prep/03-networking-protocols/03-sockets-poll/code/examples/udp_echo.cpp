// Build 1: UDP echo server/client on loopback.  Capture with:  sudo tcpdump -i lo -n udp port 9000 -w echo.pcap
//   ./udp_echo server 9000
//   ./udp_echo client 9000 "hello"
// Wireshark filter: udp.port == 9000   -> verify UDP length = 8 + payload, checksum, and the echo's swapped ports.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "sockets.hpp"
using namespace ether;

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: %s server|client PORT [MSG]\n", argv[0]); return 2; }
    uint16_t port = static_cast<uint16_t>(std::atoi(argv[2]));
    if (std::strcmp(argv[1], "server") == 0) {
        Fd s = make_udp_socket(port);
        if (!s.valid()) { std::perror("bind"); return 1; }
        std::printf("echo server on 127.0.0.1:%u (Ctrl-C to stop)\n", port);
        for (;;) udp_echo_once(s.get(), 1000);
    }
    Fd c = make_udp_socket(0);
    std::string msg = argc > 3 ? argv[3] : "ping";
    send_datagram(c.get(), reinterpret_cast<const uint8_t*>(msg.data()), msg.size(), make_addr("127.0.0.1", port));
    pollfd p{c.get(), POLLIN, 0};
    if (::poll(&p, 1, 2000) != 1) { std::puts("timeout"); return 1; }
    auto d = recv_datagram(c.get());
    std::printf("echoed: %.*s\n", static_cast<int>(d->data.size()), reinterpret_cast<const char*>(d->data.data()));
}
