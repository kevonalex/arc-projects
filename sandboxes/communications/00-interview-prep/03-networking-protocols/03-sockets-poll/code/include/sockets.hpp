// STUB - 03-net/03-sockets-poll (POSIX: Linux / macOS / WSL). Implement every TODO.
// Tests: tests/test_sockets.cpp (loopback only, no external network).
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

// TODO RAII: move-only owner of a file descriptor. Destructor closes exactly once; moved-from object owns nothing.
// release() gives up ownership; reset(fd) closes the current fd then adopts fd.
class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) : fd_(fd) {}
    // TODO: destructor, deleted copy, move ctor/assign
    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }
    int release() { return -1; }          // TODO
    void reset(int fd = -1) { (void)fd; } // TODO
private:
    int fd_ = -1;
};

inline sockaddr_in make_addr(const char* ip, uint16_t port) { (void)ip; (void)port; return {}; }   // TODO inet_pton + htons
inline uint16_t local_port(int fd) { (void)fd; return 0; }                                          // TODO getsockname
inline bool set_nonblocking(int fd) { (void)fd; return false; }                                     // TODO fcntl O_NONBLOCK

inline Fd make_udp_socket(uint16_t port = 0, const char* ip = "127.0.0.1") { (void)port; (void)ip; return Fd{}; }  // TODO socket+bind
struct Datagram { std::vector<uint8_t> data; sockaddr_in from{}; };
inline std::optional<Datagram> recv_datagram(int fd) { (void)fd; return std::nullopt; }   // TODO nonblocking recvfrom, nullopt on EAGAIN
inline bool send_datagram(int fd, const uint8_t* p, size_t n, const sockaddr_in& to) { (void)fd; (void)p; (void)n; (void)to; return false; } // TODO
inline bool udp_echo_once(int fd, int timeout_ms) { (void)fd; (void)timeout_ms; return false; }   // TODO poll(POLLIN) then echo ONE datagram

inline bool join_multicast(int fd, const char* group, const char* iface_ip) { (void)fd; (void)group; (void)iface_ip; return false; }  // TODO IP_ADD_MEMBERSHIP
inline Fd make_multicast_receiver(const char* group, uint16_t port, const char* iface_ip = "127.0.0.1") { (void)group; (void)port; (void)iface_ip; return Fd{}; }  // TODO reuseaddr, bind(group:port), join
inline Fd make_multicast_sender(const char* iface_ip = "127.0.0.1", int ttl = 1, bool loop = true) { (void)iface_ip; (void)ttl; (void)loop; return Fd{}; }  // TODO IP_MULTICAST_IF/TTL/LOOP

inline Fd tcp_listen(uint16_t port = 0, const char* ip = "127.0.0.1") { (void)port; (void)ip; return Fd{}; }   // TODO SO_REUSEADDR, bind, listen
inline Fd tcp_connect(const char* ip, uint16_t port) { (void)ip; (void)port; return Fd{}; }                    // TODO connect + TCP_NODELAY
inline bool send_all(int fd, const uint8_t* p, size_t n) { (void)fd; (void)p; (void)n; return false; }       // TODO loop on partial send, EINTR, MSG_NOSIGNAL
inline std::optional<std::vector<uint8_t>> recv_some(int fd, size_t max, int timeout_ms) { (void)fd; (void)max; (void)timeout_ms; return std::nullopt; }  // TODO poll then recv; empty vector = EOF

// TODO: single-threaded poll() loop over UDP echo sockets, one TCP listener and the accepted TCP connections.
class PollServer {
public:
    bool add_udp_echo(Fd fd) { (void)fd; return false; }
    bool add_tcp_listener(Fd fd) { (void)fd; return false; }
    size_t connection_count() const { return 0; }
    size_t run_once(int timeout_ms) { (void)timeout_ms; return 0; }   // returns number of fds with events handled
    void run(const std::atomic<bool>& stop, int tick_ms = 10) { (void)stop; (void)tick_ms; }
};

}  // namespace ether
