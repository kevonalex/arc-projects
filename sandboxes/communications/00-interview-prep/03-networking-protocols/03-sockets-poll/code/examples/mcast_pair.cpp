// Build 2: multicast pair. Run two receivers in two terminals, then one sender:
//   ./mcast_pair rx 239.255.42.99 5004      (x2)
//   ./mcast_pair tx 239.255.42.99 5004 "talkgroup 7 voice frame"
// Interface defaults to 127.0.0.1; pass a LAN IP as argv[5] to leave the host.
// Wireshark: filter  igmp || ip.dst == 239.255.42.99   -> see the IGMP membership report when rx starts,
// and check the destination MAC 01:00:5e:7f:2a:63 (low 23 bits of the group).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "sockets.hpp"
using namespace ether;

int main(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "usage: %s rx|tx GROUP PORT [MSG] [IFACE_IP]\n", argv[0]); return 2; }
    const char* group = argv[2];
    uint16_t port = static_cast<uint16_t>(std::atoi(argv[3]));
    const char* iface = argc > 5 ? argv[5] : "127.0.0.1";
    if (std::strcmp(argv[1], "rx") == 0) {
        Fd r = make_multicast_receiver(group, port, iface);
        if (!r.valid()) { std::perror("multicast rx"); return 1; }
        std::printf("joined %s:%u on %s\n", group, port, iface);
        for (;;) {
            pollfd p{r.get(), POLLIN, 0};
            if (::poll(&p, 1, 1000) == 1)
                if (auto d = recv_datagram(r.get()))
                    std::printf("rx %zu B: %.*s\n", d->data.size(), static_cast<int>(d->data.size()), reinterpret_cast<const char*>(d->data.data()));
        }
    }
    Fd t = make_multicast_sender(iface, 1, true);
    if (!t.valid()) { std::perror("multicast tx"); return 1; }
    std::string msg = argc > 4 ? argv[4] : "hello group";
    send_datagram(t.get(), reinterpret_cast<const uint8_t*>(msg.data()), msg.size(), make_addr(group, port));
}
