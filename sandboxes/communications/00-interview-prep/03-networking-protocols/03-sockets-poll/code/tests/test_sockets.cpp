#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>

#include "sockets.hpp"

using namespace ether;

static std::vector<uint8_t> bytes(const std::string& s) { return {s.begin(), s.end()}; }
static bool fd_is_open(int fd) { return ::fcntl(fd, F_GETFD) != -1; }

// ------------------------------------------------------------------ RAII
TEST(Fd, ClosesInDestructorExactlyOnce) {
    int raw;
    {
        Fd f(::socket(AF_INET, SOCK_DGRAM, 0));
        ASSERT_TRUE(f.valid());
        raw = f.get();
        EXPECT_TRUE(fd_is_open(raw));
    }
    EXPECT_FALSE(fd_is_open(raw));
}
TEST(Fd, MoveTransfersOwnership) {
    Fd a(::socket(AF_INET, SOCK_DGRAM, 0));
    int raw = a.get();
    Fd b(std::move(a));
    EXPECT_FALSE(a.valid());                       // moved-from owns nothing
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(b.get(), raw);
    Fd c;
    c = std::move(b);
    EXPECT_FALSE(b.valid());
    EXPECT_EQ(c.get(), raw);
    EXPECT_TRUE(fd_is_open(raw));
    c.reset();
    EXPECT_FALSE(fd_is_open(raw));
}
TEST(Fd, MoveAssignClosesPreviousFd) {
    Fd a(::socket(AF_INET, SOCK_DGRAM, 0));
    Fd b(::socket(AF_INET, SOCK_DGRAM, 0));
    int old_a = a.get();
    a = std::move(b);                              // old_a must be closed, not leaked
    EXPECT_FALSE(fd_is_open(old_a));
}
TEST(Fd, ReleaseGivesUpOwnership) {
    Fd a(::socket(AF_INET, SOCK_DGRAM, 0));
    int raw = a.release();
    EXPECT_FALSE(a.valid());
    EXPECT_TRUE(fd_is_open(raw));
    ::close(raw);
}

// ------------------------------------------------------------------ UDP basics
TEST(Udp, BindEphemeralPortAndBoundaries) {
    Fd server = make_udp_socket(0);
    Fd client = make_udp_socket(0);
    ASSERT_TRUE(server.valid()); ASSERT_TRUE(client.valid());
    uint16_t sp = local_port(server.get());
    EXPECT_NE(sp, 0);
    sockaddr_in to = make_addr("127.0.0.1", sp);
    // two sends -> two receives: datagram boundaries are preserved (unlike TCP)
    auto m1 = bytes("AAAA"), m2 = bytes("BB");
    ASSERT_TRUE(send_datagram(client.get(), m1.data(), m1.size(), to));
    ASSERT_TRUE(send_datagram(client.get(), m2.data(), m2.size(), to));
    pollfd p{server.get(), POLLIN, 0};
    ASSERT_EQ(::poll(&p, 1, 1000), 1);
    auto d1 = recv_datagram(server.get());
    auto d2 = recv_datagram(server.get());
    ASSERT_TRUE(d1); ASSERT_TRUE(d2);
    EXPECT_EQ(d1->data, m1);
    EXPECT_EQ(d2->data, m2);
    EXPECT_EQ(ntohs(d1->from.sin_port), local_port(client.get()));
}
TEST(Udp, NonblockingRecvReturnsNulloptWhenEmpty) {
    Fd s = make_udp_socket(0);
    ASSERT_TRUE(s.valid());
    ASSERT_TRUE(set_nonblocking(s.get()));
    EXPECT_FALSE(recv_datagram(s.get()));           // EAGAIN, must not block
}
TEST(Udp, EchoOnce) {
    Fd server = make_udp_socket(0), client = make_udp_socket(0);
    ASSERT_TRUE(server.valid()); ASSERT_TRUE(client.valid());
    auto msg = bytes("hello gateway");
    ASSERT_TRUE(send_datagram(client.get(), msg.data(), msg.size(), make_addr("127.0.0.1", local_port(server.get()))));
    EXPECT_TRUE(udp_echo_once(server.get(), 1000));
    pollfd p{client.get(), POLLIN, 0};
    ASSERT_EQ(::poll(&p, 1, 1000), 1);
    auto r = recv_datagram(client.get());
    ASSERT_TRUE(r);
    EXPECT_EQ(r->data, msg);
}
TEST(Udp, EchoOnceTimesOutWithoutTraffic) {
    Fd server = make_udp_socket(0);
    ASSERT_TRUE(server.valid());
    EXPECT_FALSE(udp_echo_once(server.get(), 30));
}
TEST(Udp, ZeroLengthDatagramIsLegal) {
    Fd server = make_udp_socket(0), client = make_udp_socket(0);
    ASSERT_TRUE(server.valid()); ASSERT_TRUE(client.valid());
    ASSERT_TRUE(send_datagram(client.get(), nullptr, 0, make_addr("127.0.0.1", local_port(server.get()))));
    pollfd p{server.get(), POLLIN, 0};
    ASSERT_EQ(::poll(&p, 1, 1000), 1);
    auto d = recv_datagram(server.get());
    ASSERT_TRUE(d);
    EXPECT_TRUE(d->data.empty());                   // 0 bytes received is NOT "closed" on UDP
}

// ------------------------------------------------------------------ multicast pair
TEST(Multicast, SenderReachesTwoReceivers) {
    Fd probe = make_udp_socket(0);
    ASSERT_TRUE(probe.valid());
    uint16_t port = local_port(probe.get());
    probe.reset();
    const char* group = "239.255.42.99";            // admin-scoped (239/8)
    Fd r1 = make_multicast_receiver(group, port, "127.0.0.1");
    Fd r2 = make_multicast_receiver(group, port, "127.0.0.1");
    Fd tx = make_multicast_sender("127.0.0.1", 1, true);
    if (!r1.valid() || !r2.valid() || !tx.valid())
        GTEST_SKIP() << "multicast sockets unavailable here (container/WSL without multicast on lo?)";
    auto msg = bytes("talkgroup-7");
    ASSERT_TRUE(send_datagram(tx.get(), msg.data(), msg.size(), make_addr(group, port)));
    int got = 0;
    for (Fd* r : {&r1, &r2}) {
        pollfd p{r->get(), POLLIN, 0};
        if (::poll(&p, 1, 500) == 1) {
            auto d = recv_datagram(r->get());
            if (d && d->data == msg) ++got;
        }
    }
    if (got == 0) GTEST_SKIP() << "no multicast loopback delivery in this environment";
    EXPECT_EQ(got, 2);                              // both members of the group got the ONE sent datagram
}

// ------------------------------------------------------------------ TCP helpers
TEST(Tcp, ConnectSendRecv) {
    Fd lst = tcp_listen(0);
    ASSERT_TRUE(lst.valid());
    uint16_t port = local_port(lst.get());
    Fd cli = tcp_connect("127.0.0.1", port);
    ASSERT_TRUE(cli.valid());
    Fd srv(::accept(lst.get(), nullptr, nullptr));
    ASSERT_TRUE(srv.valid());
    auto big = std::vector<uint8_t>(300000, 0x5A);    // larger than the socket buffer: forces partial sends
    std::thread t([&] { EXPECT_TRUE(send_all(cli.get(), big.data(), big.size())); cli.reset(); });
    size_t total = 0;
    for (;;) {
        auto r = recv_some(srv.get(), 65536, 2000);
        ASSERT_TRUE(r) << "timeout";
        if (r->empty()) break;                         // EOF after the client closed
        total += r->size();
    }
    t.join();
    EXPECT_EQ(total, big.size());                      // bytes, not messages: any chunking is fine
}

// ------------------------------------------------------------------ poll server
namespace {
struct ServerThread {
    PollServer srv; std::atomic<bool> stop{false}; std::thread th;
    void start() { th = std::thread([this] { srv.run(stop, 5); }); }
    ~ServerThread() { stop = true; if (th.joinable()) th.join(); }
};
}  // namespace

TEST(PollServer, RunOnceTimesOutWithZeroEvents) {
    PollServer s;
    Fd u = make_udp_socket(0);
    ASSERT_TRUE(s.add_udp_echo(std::move(u)));
    EXPECT_EQ(s.run_once(20), 0u);
}
TEST(PollServer, OneThreadServesManyUdpSockets) {
    ServerThread st;
    std::vector<uint16_t> ports;
    for (int i = 0; i < 8; ++i) {                     // 8 "talkgroup" sockets on ONE thread
        Fd f = make_udp_socket(0);
        ASSERT_TRUE(f.valid());
        ports.push_back(local_port(f.get()));
        ASSERT_TRUE(st.srv.add_udp_echo(std::move(f)));
    }
    st.start();
    Fd cli = make_udp_socket(0);
    ASSERT_TRUE(cli.valid());
    for (size_t i = 0; i < ports.size(); ++i) {
        auto msg = bytes("tg" + std::to_string(i));
        ASSERT_TRUE(send_datagram(cli.get(), msg.data(), msg.size(), make_addr("127.0.0.1", ports[i])));
        pollfd p{cli.get(), POLLIN, 0};
        ASSERT_EQ(::poll(&p, 1, 1000), 1) << "no echo from socket " << i;
        auto r = recv_datagram(cli.get());
        ASSERT_TRUE(r);
        EXPECT_EQ(r->data, msg);
        EXPECT_EQ(ntohs(r->from.sin_port), ports[i]);   // the echo comes from the socket that was addressed
    }
}
TEST(PollServer, AcceptsAndEchoesTcpAlongsideUdpAndSurvivesClientClose) {
    ServerThread st;
    Fd lst = tcp_listen(0);
    ASSERT_TRUE(lst.valid());
    uint16_t port = local_port(lst.get());
    ASSERT_TRUE(st.srv.add_tcp_listener(std::move(lst)));
    Fd udp = make_udp_socket(0);                       // mix UDP + TCP in one loop
    uint16_t uport = local_port(udp.get());
    ASSERT_TRUE(st.srv.add_udp_echo(std::move(udp)));
    st.start();

    Fd c1 = tcp_connect("127.0.0.1", port), c2 = tcp_connect("127.0.0.1", port);
    ASSERT_TRUE(c1.valid()); ASSERT_TRUE(c2.valid());
    auto m1 = bytes("INVITE"), m2 = bytes("BYE");
    ASSERT_TRUE(send_all(c1.get(), m1.data(), m1.size()));
    ASSERT_TRUE(send_all(c2.get(), m2.data(), m2.size()));
    auto r1 = recv_some(c1.get(), 100, 1000), r2 = recv_some(c2.get(), 100, 1000);
    ASSERT_TRUE(r1); ASSERT_TRUE(r2);
    EXPECT_EQ(*r1, m1); EXPECT_EQ(*r2, m2);

    Fd cu = make_udp_socket(0);
    auto um = bytes("rtp");
    ASSERT_TRUE(send_datagram(cu.get(), um.data(), um.size(), make_addr("127.0.0.1", uport)));
    pollfd p{cu.get(), POLLIN, 0};
    ASSERT_EQ(::poll(&p, 1, 1000), 1);                // UDP still served while TCP is connected
    c1.reset();                                        // c1 closes; the server must close its side but keep serving c2
    auto m3 = bytes("STILL-THERE");
    ASSERT_TRUE(send_all(c2.get(), m3.data(), m3.size()));
    auto r3 = recv_some(c2.get(), 100, 1000);
    ASSERT_TRUE(r3);
    EXPECT_EQ(*r3, m3);
}
