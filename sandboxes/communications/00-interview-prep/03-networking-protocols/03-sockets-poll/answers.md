# 03-net / 03 — Answers

**Q1.** TG2's audio is **stalled for up to 10 s**: the program is blocked inside `recvfrom(sock1)` and cannot look at `sock2`. Meanwhile the kernel keeps accepting TG2 datagrams into socket 2's receive queue (`SO_RCVBUF`, ≈ 208 KiB default ⚠). At 50 pps × ~2 KiB truesize ≈ 100 KiB/s the queue fills in ~2 s; after that the kernel **silently drops** new datagrams (UDP has no back-pressure). When TG1 finally speaks, TG2 is served *stale* audio from the buffer, then a gap. Principle: **head-of-line blocking inside the program** — one blocking call serialises independent event sources. Fix: wait on *all* sources at once (`poll`/`epoll`), and never block on one.

**Q2.** Skeleton (see `solutions/sockets.hpp`):
```cpp
class Fd {
  int fd_ = -1;
public:
  Fd() = default;  explicit Fd(int f) : fd_(f) {}
  ~Fd() { reset(); }                                   // close exactly once
  Fd(const Fd&) = delete;  Fd& operator=(const Fd&) = delete;   // one owner
  Fd(Fd&& o) noexcept : fd_(o.release()) {}            // steal; o becomes -1
  Fd& operator=(Fd&& o) noexcept { if (this != &o) reset(o.release()); return *this; }
  int get() const; bool valid() const;
  int release() { int f = fd_; fd_ = -1; return f; }
  void reset(int f = -1) { if (fd_ >= 0) ::close(fd_); fd_ = f; }
};
```
`Fd.MoveAssignClosesPreviousFd` guards the *leak on assignment*: if move-assign just copied the int, the target's old descriptor would never be closed. (The `this != &o` guard in the solution protects against `a = std::move(a)`, which would otherwise close the fd it is about to keep.)

**Q3.** Copy would have to mean either "two owners of one int" (double close, recycled-fd cross-talk) or `dup()` (a *new descriptor* that shares the same open-file description: closing one does not close the socket; `shutdown`/non-blocking flags are shared; hidden table usage and failure modes `EMFILE` in a copy constructor that cannot report errors). Deleting the copy forces ownership to be explicit. `dup` is right when two parties genuinely need independent lifetimes of the same socket (hand a connection to a worker while the acceptor keeps its handle; redirect stdout; fork/exec plumbing) — spelled out as `Fd dup() const`, which can return an invalid `Fd` on failure.

**Q4.** Virtual stack: 2 000 × 8 MiB = 16 000 MiB ≈ **15.6 GiB** of address space (committed lazily, but kernel stacks and thread structs are real and each thread that touches its stack commits pages). Wake-ups if all active: 2 000 × 50 = **100 000/s**, each a context switch. Hazard absent in the single thread: **data races / lock contention** on shared state (floor control, jitter buffers, stats), with lock-ordering deadlocks and priority inversion (`01-cpp/06`).

**Q5.** n = 8, k = 3: `poll` copies 8 entries in and scans 8 out (64 B each way; trivial). `epoll_wait` returns 3 events. Equal in practice → pick **poll** (portable, simple). n = 10 000: `poll` copies/scans **10 000** entries (8 B each = 80 KB per direction) every call, i.e. ~20 000 entry-touches to find 3 ready ones; `epoll_wait` does ~3 units of work (+ O(1) `epoll_ctl` when sockets are added/removed) → pick **epoll**.

**Q6.** Edge-triggered: the event fires only on the empty→non-empty transition. The handler reads one datagram and returns; four stay queued and **no new edge occurs** until another datagram arrives, so audio frames sit in the kernel and appear late/bursty (or stall at the end of a talk burst — the last frames are never read). Fix: read until `EAGAIN`. Level-triggered `poll`: `POLLIN` is reported again immediately, so it still works — you just pay five extra `poll` syscalls instead of a drain loop.

**Q7.**
```text
tick = now + 20ms;  setup_deadline = now + 3s (one-shot, armed)
loop:
  next = min(tick, setup_deadline if armed)
  timeout_ms = clamp(ceil((next - now)), 0, INT_MAX)
  n = poll(fds, timeout_ms)               // retry on EINTR, recompute timeout
  for ready fds: drain & handle
  now = steady_clock::now()
  while now >= tick:  audio_tick(); tick += 20ms     // may run twice if we were late; never skip
  if armed && now >= setup_deadline: on_setup_timeout(); armed = false
```
Use absolute deadlines on `steady_clock`. If the audio path must never block on I/O: the I/O thread only reads datagrams, timestamps them and pushes into a **lock-free SPSC ring**; the audio thread (own timer, e.g. `timerfd`/`clock_nanosleep` to absolute time) pops, runs jitter buffer/decoder and pushes outbound packets into a second ring that the I/O thread sends. No allocation, locks or syscalls on the audio path other than the clock/timer.

**Q8.** States: `NeedHeader(have k of 4)` → `NeedBody(len, have m)` → emit → `NeedHeader`. (a) 2 bytes: append to the buffer, still `NeedHeader` with 2/4. (b) 1 000 bytes with 2.5 messages: append; loop `while complete frame in buffer { emit; erase }`; the remaining half-message stays buffered with state `NeedBody`. Attack: a declared length of 4 GiB → enforce `max_frame` (gRPC's default receive limit is 4 MiB ⚠ VERIFY) and close the connection with a protocol error before allocating anything. Also guard `4 + len` overflow in 32-bit arithmetic.

**Q9.** TCP: `recv() == 0` = the peer sent FIN (orderly shutdown; you can still `send` — half-close — until you close). UDP: `0` = a received **empty datagram** (legal), not a close. `EAGAIN` and `EWOULDBLOCK` are the same value on Linux but may differ elsewhere — test both. `POLLHUP`: the connection is hung up (peer closed/reset); unread data may remain, so keep reading until `recv` returns 0/error.

**Q10.** With no `IP_MULTICAST_IF` the kernel picks the interface by routing: there is normally no route for 224.0.0.0/4, so it takes the **default route → eth1 (core side)** — talkgroup audio would never reach the LMR-side repeaters. Production sender: `IP_MULTICAST_IF` = address of eth0 (10.1.x.x); `IP_MULTICAST_TTL` = 1 on a single LAN, or the minimum hop count of the routed path (e.g. 4–16 ⚠ policy) so it cannot leak far; `IP_MULTICAST_LOOP` = 0 (do not hear your own stream). Also set DSCP (`IP_TOS` 0xB8).

**Q11.** 100 × 50 = 5 000 pps; 50 ms → **250 packets** queued; × 2 KiB ≈ **500 KiB** > default ~208 KiB → roughly the last 60 % of the burst is dropped. Need `SO_RCVBUF` ≥ ~512 KiB (kernel doubles the requested value; capped by `net.core.rmem_max`, raise it). Detect drops: `Udp: ... RcvbufErrors` in `/proc/net/snmp`, `netstat -su` "receive buffer errors", `ss -uamp` skmem `d` (drops), and RTP sequence gaps / RTCP loss fraction. ⚠ VERIFY truesize.

**Q12.**
(a) `rtp.ssrc == 0x11223344`
(b) `tcp.analysis.retransmission` (fast retransmits: `tcp.analysis.fast_retransmission`)
(c) `sip.Method == "INVITE"`
(d) `igmp.maddr == 239.1.1.1` (or just `igmp`)
(e) `ip.dsfield.dscp == 46`
(f) `udp.length > 1008` (UDP length includes its 8 B header) or `frame.len > 1000`
(g) `ip.flags.mf == 1 || ip.frag_offset > 0`
(h) `tcp.flags.syn == 1 && tcp.flags.ack == 0`

**Q13.** **Checksum offload**: the OS hands the NIC a packet with the checksum field unfilled and the NIC computes it on transmit — after the capture point, so Wireshark on the sending host sees an incorrect/zero value; the receiver's capture sees the real one. Confirm: `ethtool -k eth0 | grep -i tx-checksumming` (shows `on`); disable temporarily with `ethtool -K eth0 tx off` to see correct checksums locally; or capture on the receiver. (Loopback also never computes real checksums.)

**Q14.** Bugs: (1) `sockaddr_in a;` not zero-initialised (`= {}`); (2) `sin_port = 9000` without **`htons`**; (3) `peer`, `len` undeclared, and **`len` must be initialised to `sizeof peer`** before `recvfrom`; (4) `socket`/`bind` return values unchecked; (5) `recvfrom` may return **−1** (EINTR, error) → `sendto(..., n = -1, ...)` converts to a huge `size_t`; use `ssize_t` and check; (6) 100-byte buffer **silently truncates** larger datagrams (use ≥ 1500 or 65 535, detect `MSG_TRUNC`); (7) the socket is never closed — no RAII; (8) blocking forever, can't serve other sockets/timers, no shutdown path; (9) **binds `INADDR_ANY`** and echoes any source → a public **UDP reflection/amplification** gadget; bind loopback/an internal address and validate peers; (10) C-style casts and no `EINTR` handling; `sendto` result unchecked.

**Q15.** *select*: portable, `FD_SETSIZE` 1024 limit, O(max fd), rebuild sets each call — legacy. *poll*: array-based, no fd limit, O(n) copy+scan per call, simple. *epoll*: Linux, kernel-held interest list, O(1) add/remove, O(ready) per wait, level or edge trigger (edge requires non-blocking + draining). 8 sockets → **poll** (simplicity, portability, cost identical). 8 000 → **epoll** (or `io_uring`/kqueue on other platforms) — and consider multiple I/O threads with `SO_REUSEPORT`. Say what you'd *measure*: syscalls per packet, wake-ups, `RcvbufErrors`.

**Q16.** *SIGPIPE*: `send` on a socket the peer closed → default action kills the process; fix `MSG_NOSIGNAL` or `signal(SIGPIPE, SIG_IGN)` and handle `EPIPE`. *EINTR*: a signal interrupts a slow syscall → `-1/EINTR` is not an error; retry (recompute timeouts); no retry for `close`. *Partial send*: `send` returns fewer bytes than requested (socket buffer full, or interrupted) → loop with an offset (`send_all`) or, non-blocking, queue the remainder and wait for `POLLOUT`. A naive client truncates messages, dies randomly, or fails under load.

**Q17.** Threads: **I/O thread** (epoll; owns all sockets; reads datagrams, stamps arrival time, pushes `Packet` handles into per-stream SPSC rings; sends from an outbound ring); **audio-tick thread** (absolute 20 ms timer, SCHED_FIFO if allowed; pops packets, runs jitter buffer, decoder, mixer, encoder; pushes outbound RTP); **control thread** (gRPC/SIP; may allocate and lock; talks to the others through a command queue and atomic config snapshots). Crossing boundaries: SPSC rings of preallocated fixed-size buffers (pool), atomics with acquire/release; logging via a lock-free ring drained by another thread. **Never** allocate, lock, log synchronously, or touch the filesystem on the audio thread.
