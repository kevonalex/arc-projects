// Reference solution - 03-net/03-sockets-poll   (POSIX: Linux / macOS / WSL)
#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

namespace ether {

// ------------------------------------------------------------------ RAII file descriptor
// Why: a leaked fd in a gateway that runs for months is a slow death (EMFILE). One owner, move-only,
// closed exactly once in the destructor - including on early return and exceptions.
class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) : fd_(fd) {}
    ~Fd() { reset(); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& o) noexcept : fd_(o.release()) {}
    Fd& operator=(Fd&& o) noexcept {
        if (this != &o) reset(o.release());
        return *this;
    }
    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }
    int release() { int f = fd_; fd_ = -1; return f; }
    void reset(int fd = -1) {
        if (fd_ >= 0) ::close(fd_);     // do NOT retry on EINTR: the fd is already released on Linux
        fd_ = fd;
    }
private:
    int fd_ = -1;
};

inline sockaddr_in make_addr(const char* ip, uint16_t port) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    ::inet_pton(AF_INET, ip, &a.sin_addr);
    return a;
}
inline uint16_t local_port(int fd) {
    sockaddr_in a{}; socklen_t l = sizeof a;
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&a), &l) != 0) return 0;
    return ntohs(a.sin_port);
}
inline bool set_nonblocking(int fd) {
    int fl = ::fcntl(fd, F_GETFL, 0);
    return fl >= 0 && ::fcntl(fd, F_SETFL, fl | O_NONBLOCK) == 0;
}

// ------------------------------------------------------------------ UDP
inline Fd make_udp_socket(uint16_t port = 0, const char* ip = "127.0.0.1") {
    Fd fd(::socket(AF_INET, SOCK_DGRAM, 0));
    if (!fd.valid()) return Fd{};
    sockaddr_in a = make_addr(ip, port);
    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) return Fd{};
    return fd;
}
struct Datagram { std::vector<uint8_t> data; sockaddr_in from{}; };
// Nonblocking receive: nullopt when nothing is queued (EAGAIN/EWOULDBLOCK) or on EINTR.
inline std::optional<Datagram> recv_datagram(int fd) {
    std::vector<uint8_t> buf(65535);
    sockaddr_in from{}; socklen_t fl = sizeof from;
    ssize_t n = ::recvfrom(fd, buf.data(), buf.size(), MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&from), &fl);
    if (n < 0) return std::nullopt;
    buf.resize(static_cast<size_t>(n));
    return Datagram{std::move(buf), from};
}
inline bool send_datagram(int fd, const uint8_t* p, size_t n, const sockaddr_in& to) {
    ssize_t s = ::sendto(fd, p, n, 0, reinterpret_cast<const sockaddr*>(&to), sizeof to);
    return s == static_cast<ssize_t>(n);       // UDP is all-or-nothing
}
// Wait up to timeout_ms for ONE datagram and echo it to the sender. true if echoed.
inline bool udp_echo_once(int fd, int timeout_ms) {
    pollfd p{fd, POLLIN, 0};
    int r;
    do { r = ::poll(&p, 1, timeout_ms); } while (r < 0 && errno == EINTR);
    if (r <= 0) return false;
    auto d = recv_datagram(fd);
    return d && send_datagram(fd, d->data.data(), d->data.size(), d->from);
}

// ------------------------------------------------------------------ multicast (worksheet Part D)
inline bool join_multicast(int fd, const char* group, const char* iface_ip) {
    ip_mreq m{};
    if (::inet_pton(AF_INET, group, &m.imr_multiaddr) != 1) return false;
    if (::inet_pton(AF_INET, iface_ip, &m.imr_interface) != 1) return false;
    return ::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof m) == 0;
}
// Receiver: SO_REUSEADDR lets several processes share group:port; binding to the GROUP address (Linux)
// filters out other groups using the same port.
inline Fd make_multicast_receiver(const char* group, uint16_t port, const char* iface_ip = "127.0.0.1") {
    Fd fd(::socket(AF_INET, SOCK_DGRAM, 0));
    if (!fd.valid()) return Fd{};
    int one = 1;
    ::setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in a = make_addr(group, port);
    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) return Fd{};
    if (!join_multicast(fd.get(), group, iface_ip)) return Fd{};
    return fd;
}
inline Fd make_multicast_sender(const char* iface_ip = "127.0.0.1", int ttl = 1, bool loop = true) {
    Fd fd(::socket(AF_INET, SOCK_DGRAM, 0));
    if (!fd.valid()) return Fd{};
    in_addr ia{};
    if (::inet_pton(AF_INET, iface_ip, &ia) != 1) return Fd{};
    if (::setsockopt(fd.get(), IPPROTO_IP, IP_MULTICAST_IF, &ia, sizeof ia) != 0) return Fd{};
    unsigned char t = static_cast<unsigned char>(ttl), l = loop ? 1 : 0;
    ::setsockopt(fd.get(), IPPROTO_IP, IP_MULTICAST_TTL, &t, sizeof t);
    ::setsockopt(fd.get(), IPPROTO_IP, IP_MULTICAST_LOOP, &l, sizeof l);
    return fd;
}

// ------------------------------------------------------------------ TCP helpers
inline Fd tcp_listen(uint16_t port = 0, const char* ip = "127.0.0.1") {
    Fd fd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!fd.valid()) return Fd{};
    int one = 1;
    ::setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);   // survive TIME_WAIT on restart
    sockaddr_in a = make_addr(ip, port);
    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) return Fd{};
    if (::listen(fd.get(), 16) != 0) return Fd{};
    return fd;
}
inline Fd tcp_connect(const char* ip, uint16_t port) {
    Fd fd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!fd.valid()) return Fd{};
    sockaddr_in a = make_addr(ip, port);
    if (::connect(fd.get(), reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) return Fd{};
    int one = 1;
    ::setsockopt(fd.get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);   // latency-sensitive RPC (pack 02, Part E)
    return fd;
}
// TCP is a byte stream: send() may accept fewer bytes than asked. Loop until done (handles EINTR).
inline bool send_all(int fd, const uint8_t* p, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t s = ::send(fd, p + off, n - off, MSG_NOSIGNAL);
        if (s < 0) { if (errno == EINTR) continue; return false; }
        off += static_cast<size_t>(s);
    }
    return true;
}
// Wait up to timeout_ms for readable data. nullopt = timeout/error; empty vector = peer closed (EOF).
inline std::optional<std::vector<uint8_t>> recv_some(int fd, size_t max, int timeout_ms) {
    pollfd p{fd, POLLIN, 0};
    int r;
    do { r = ::poll(&p, 1, timeout_ms); } while (r < 0 && errno == EINTR);
    if (r <= 0) return std::nullopt;
    std::vector<uint8_t> buf(max);
    ssize_t n = ::recv(fd, buf.data(), buf.size(), 0);
    if (n < 0) return std::nullopt;
    buf.resize(static_cast<size_t>(n));
    return buf;
}

// ------------------------------------------------------------------ poll()-based single-threaded server
// One thread, N sockets: UDP echo sockets, one TCP listener, and the TCP connections it accepted.
// Why not a thread per client? 1000 talkgroup sockets x 1 MiB stack = 1 GiB; context switches; locking.
// Simplification (documented in the worksheet): TCP echo does one non-blocking send(); a production server
// keeps a per-connection output buffer and polls POLLOUT.
class PollServer {
public:
    bool add_udp_echo(Fd fd) {
        if (!fd.valid() || !set_nonblocking(fd.get())) return false;
        entries_.push_back({std::move(fd), Kind::UdpEcho});
        return true;
    }
    bool add_tcp_listener(Fd fd) {
        if (!fd.valid() || !set_nonblocking(fd.get())) return false;
        entries_.push_back({std::move(fd), Kind::Listener});
        return true;
    }
    size_t connection_count() const {
        size_t c = 0;
        for (const auto& e : entries_) c += (e.kind == Kind::Conn);
        return c;
    }
    // One iteration of the event loop. Returns the number of descriptors that had events handled.
    size_t run_once(int timeout_ms) {
        std::vector<pollfd> pfds;
        pfds.reserve(entries_.size());
        for (const auto& e : entries_) pfds.push_back({e.fd.get(), POLLIN, 0});
        int r;
        do { r = ::poll(pfds.data(), pfds.size(), timeout_ms); } while (r < 0 && errno == EINTR);
        if (r <= 0) return 0;

        size_t handled = 0;
        std::vector<Entry> added;
        std::vector<bool> dead(entries_.size(), false);
        for (size_t i = 0; i < pfds.size(); ++i) {
            if (!pfds[i].revents) continue;
            ++handled;
            Entry& e = entries_[i];
            switch (e.kind) {
            case Kind::UdpEcho:
                while (auto d = recv_datagram(e.fd.get())) send_datagram(e.fd.get(), d->data.data(), d->data.size(), d->from);
                break;
            case Kind::Listener:
                for (;;) {
                    int c = ::accept(e.fd.get(), nullptr, nullptr);
                    if (c < 0) break;                       // EAGAIN: backlog drained
                    Fd cf(c);
                    if (set_nonblocking(c)) added.push_back({std::move(cf), Kind::Conn});
                }
                break;
            case Kind::Conn: {
                uint8_t buf[2048];
                ssize_t n = ::recv(e.fd.get(), buf, sizeof buf, 0);
                if (n > 0) ::send(e.fd.get(), buf, static_cast<size_t>(n), MSG_NOSIGNAL);
                else if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) dead[i] = true;   // EOF or error
                break;
            }
            }
        }
        // compact: erase dead (their Fd destructors close them), then append new connections
        std::vector<Entry> kept;
        for (size_t i = 0; i < entries_.size(); ++i) if (!dead[i]) kept.push_back(std::move(entries_[i]));
        for (auto& a : added) kept.push_back(std::move(a));
        entries_ = std::move(kept);
        return handled;
    }
    void run(const std::atomic<bool>& stop, int tick_ms = 10) {
        while (!stop.load()) run_once(tick_ms);
    }
private:
    enum class Kind { UdpEcho, Listener, Conn };
    struct Entry { Fd fd; Kind kind; };
    std::vector<Entry> entries_;
};

}  // namespace ether
