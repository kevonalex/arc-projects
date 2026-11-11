// Build 3: poll() server. One thread: N UDP echo ports + one TCP listener.
//   ./poll_server 9100 8        -> UDP echo on 9100..9107, TCP echo listener on 9100
// Test:  echo hi | nc -u -w1 127.0.0.1 9103      and      nc 127.0.0.1 9100
// Observe with:  ss -tunlp | grep poll_server     and     strace -f -e trace=poll,accept,recvfrom ./poll_server 9100 8
#include <cstdio>
#include <cstdlib>
#include <csignal>

#include "sockets.hpp"
using namespace ether;

static std::atomic<bool> g_stop{false};
static void on_sigint(int) { g_stop = true; }

int main(int argc, char** argv) {
    uint16_t base = argc > 1 ? static_cast<uint16_t>(std::atoi(argv[1])) : 9100;
    int n = argc > 2 ? std::atoi(argv[2]) : 4;
    std::signal(SIGINT, on_sigint);
    PollServer s;
    for (int i = 0; i < n; ++i)
        if (!s.add_udp_echo(make_udp_socket(static_cast<uint16_t>(base + i)))) { std::perror("udp"); return 1; }
    if (!s.add_tcp_listener(tcp_listen(base))) { std::perror("tcp"); return 1; }
    std::printf("poll server: UDP %u..%u, TCP %u. Ctrl-C to stop.\n", base, base + n - 1, base);
    s.run(g_stop);
    std::puts("bye (all fds closed by RAII)");
}
