# 03-net / 03 — POSIX Sockets, RAII, poll/epoll Event Loops, Wireshark

**Time:** ~4 h · **Prereqs:** `01-cpp/03-lifetime-move` (RAII), `02-udp-tcp-qos` · **Platform:** Linux / macOS / **WSL** (POSIX sockets; the CMake refuses to configure on native Windows)
**Code:** `code/include/sockets.hpp` (your stub) · `code/solutions/sockets.hpp` · `code/tests/test_sockets.cpp` · `code/examples/*.cpp` (three builds)
**Etherstack lens:** a gateway terminates hundreds of talkgroup sockets, a SIP/gRPC channel and a 20 ms audio tick — typically from **one I/O thread**. Interviewers ask: "why not a thread per socket?", "what does `poll` return?", "how do you avoid leaking file descriptors?", and "show me in Wireshark that it works".

> **Legal / safety:** run everything on loopback or your own lab LAN with *synthetic* data. Never capture or receive live public-safety traffic (UK Wireless Telegraphy Act 2006). Packet capture needs privileges (`sudo`/`CAP_NET_RAW`); capture only your own traffic.

---

## Part A — The problem, concretely (before any API)

Your gateway serves two talkgroups. TG1 is on UDP port 9001, TG2 on 9002. A junior writes:

```cpp
while (true) {
    n = recvfrom(sock1, buf, sizeof buf, 0, ...);   // blocks until TG1 speaks
    forward(buf, n);
    n = recvfrom(sock2, buf, sizeof buf, 0, ...);   // blocks until TG2 speaks
    forward(buf, n);
}
```

**Q1.** TG1 is silent for 10 s while TG2 sends 50 packets per second. What happens to TG2's audio? What does the kernel do with TG2's datagrams while the program is blocked on `sock1`? (Use `SO_RCVBUF` in your answer.) What is the general principle violated? *Context: PTT talkgroups are bursty and independent.*

Keep this in mind: the whole pack is the answer to Q1.

---

## Part B — Sockets from first principles

### B.1 A socket is a file descriptor
The kernel keeps, per process, a table: **integer → open kernel object**. `socket()` returns the smallest free integer (3, 4, 5… after stdin/out/err). `read`, `write`, `close`, `poll` all take that integer. "Everything is a file" is why `poll` can wait on sockets, pipes and timers together.

UDP lifecycle: `socket(AF_INET, SOCK_DGRAM, 0)` → `bind(addr, port)` → `recvfrom` / `sendto`.
TCP server: `socket(SOCK_STREAM)` → `bind` → `listen(backlog)` → `accept` (returns a **new** fd per client) → `recv`/`send` → `close`.
TCP client: `socket` → `connect` (the 3-way handshake happens here) → `send`/`recv` → `close`.

**Byte order:** `sin_port` and `sin_addr` are **network order**: `htons(port)`, `inet_pton`. A classic bug is `a.sin_port = 9000` (binds port 0x2823 = 10 275).

### B.2 Why RAII for the fd
The fd is a *resource with an owner and a lifetime*. Raw `int` fails in five ways — each is a real incident class:

| Bug with raw `int` | RAII fix |
|---------------------|----------|
| Early `return`/`throw` skips `close` → **fd leak** → after ~1 000 leaks `socket()` fails with `EMFILE` | destructor closes |
| `close(fd)` twice | `reset()` sets `-1` |
| Copy of the int: two owners, one closes, the other uses a **recycled fd number that now belongs to another socket** (silent cross-talk!) | **copy deleted** |
| Return by value then forget to close | move transfers ownership |
| Exception thrown mid-setup | stack unwinding |

Rule-of-five specifics (Part `01-cpp/03`): copy ctor/assign `= delete`; move ctor steals via `release()`; move assign must **close its own fd first** (`reset(o.release())`) and guard self-move; moved-from holds `-1`; destructor `noexcept`. Do **not** loop on `EINTR` after `close()` on Linux — the descriptor is already freed; retrying can close an fd another thread just got.

> **Common misconceptions**
> 1. *"`close` flushes unsent data."* For TCP, `close` queues a FIN after pending data (unless `SO_LINGER` says otherwise); for UDP nothing is queued — packets are gone.
> 2. *"A smart pointer is the same thing."* `unique_ptr<int, Closer>` works but the fd is not a pointer; a dedicated `Fd` type documents ownership and gives `get()/release()/reset()`.
> 3. *"`accept` returns the same fd as the listener."* A **new** fd per connection; the listener keeps listening.

**Q2.** Write `Fd` from memory (move-only). List what each of the five special members does and why. Then explain what the test `Fd.MoveAssignClosesPreviousFd` guards against.

**Q3.** Why is `Fd(const Fd&)` deleted instead of implemented with `dup()`? When *is* `dup` the right tool? *Context: handing a socket to a worker thread vs sharing it.*

---

## Part C — Blocking, non-blocking, multiplexed: derive the event loop

### C.1 Three ways to serve N sockets
1. **Block on each in turn** (Part A) — wrong: idle socket starves busy ones.
2. **Thread per socket** — works, but costs: each thread has a stack (default **8 MiB virtual** on Linux ⚠ VERIFY; the pages are committed lazily, but kernel stack + bookkeeping are real), a context switch at every wake-up, and *locks around all shared state* (jitter buffers, floor state). 2 000 talkgroup sockets → 2 000 threads: scheduler thrash, 16 GiB of virtual address space, cache misses. This is the **C10K problem**.
3. **One thread, one `poll` loop** — ask the kernel *"which of these N fds is ready?"*, then touch only those. No locks (single thread), predictable latency, fits a 20 ms audio tick.

### C.2 What `poll` does
`poll(struct pollfd* fds, nfds_t n, int timeout_ms)`: you pass an **array** of `{fd, events, revents}`; the kernel blocks until at least one fd has an event in `events` (or timeout), fills `revents`, returns the number of ready fds (0 = timeout, −1 = error, check `EINTR`).
* `POLLIN` readable (data, or a pending `accept`, or EOF), `POLLOUT` writable, `POLLERR/POLLHUP/POLLNVAL` always reported.
* **Level-triggered:** it keeps reporting `POLLIN` while data remains unread. That is why the UDP echo handler **drains until `EAGAIN`**.

### C.3 select vs poll vs epoll — complexity
| API | fd limit | Per-call cost | Notes |
|-----|----------|---------------|-------|
| `select` | `FD_SETSIZE` = 1024 | O(max fd), rebuild bitsets each call | portable but dated; fd ≥ 1024 is UB |
| `poll` | none | O(n): copy `n` entries in, scan `n` out | simple; fine for tens–hundreds of fds |
| `epoll` (Linux) | none | O(1) to add/remove (`epoll_ctl`), **O(ready)** per `epoll_wait` | kernel keeps the interest list; level- or **edge-triggered** (`EPOLLET`, must read to `EAGAIN`) |
| `kqueue` / IOCP | — | BSD/macOS / Windows equivalents | |

Why O(n) hurts: 10 000 fds × 50 packets/s each = 500 000 events/s. With `poll`, *every wake-up* re-copies and rescans 10 000 entries even if one is ready. With `epoll`, a wake-up costs ~ the ready count. For **8** talkgroup sockets, `poll` is simpler and just as fast — choose by scale, and know both.

### C.4 Non-blocking I/O and its rules
* `fcntl(fd, F_SETFL, O_NONBLOCK)`: `recv` with no data returns `-1/EAGAIN` instead of sleeping.
* After `poll` says readable, still use non-blocking reads: a `POLLIN` can be spurious (UDP checksum failure drops the datagram after the wake-up).
* **TCP is a byte stream:** `recv(…, 2048)` may return half a message or 3 messages. You need **framing** (length prefix, as in gRPC's 5-byte header, or delimiters) and per-connection **input and output buffers**. `send` may accept fewer bytes than asked (full socket buffer) → keep the remainder, register `POLLOUT`, resume later (**back-pressure**). The reference server's TCP echo is deliberately simplified (single `send`).
* `recv() == 0`: **TCP = peer closed (EOF)**. **UDP = a legal empty datagram** (test `Udp.ZeroLengthDatagramIsLegal`) — never treat it as a close.
* `send()` to a socket the peer has closed raises **SIGPIPE** (kills the process by default): use `MSG_NOSIGNAL` or ignore the signal.
* `EINTR`: a signal interrupted the call — retry (except `close`).

### C.5 The poll timeout is your timer
The audio engine must tick every 20 ms. Loop:

```text
next_tick = steady_clock::now() + 20ms
loop:
   timeout = max(0, next_tick - now)        // time until the NEXT deadline
   n = poll(fds, timeout)
   handle ready fds
   if now >= next_tick: audio_tick(); next_tick += 20ms     // ABSOLUTE deadlines: no cumulative drift
```
Using `sleep(20ms)` after each iteration instead accumulates processing time as **clock drift**; adding the interval to the *previous deadline* does not.

> **Common misconceptions**
> 1. *"Non-blocking means asynchronous."* It means *the call returns immediately*; you still need `poll`/`epoll` to know when to try again.
> 2. *"epoll is always faster."* Slower to set up, Linux-only; for a few fds `poll` wins on simplicity.
> 3. *"Edge-triggered is just better."* It requires draining to `EAGAIN`; one missed read stalls the fd forever.
> 4. *"One thread can't handle 100 streams."* 100 × 50 pps = 5 000 pps; at roughly 2–10 µs per `recvfrom` + handling that is 1–5 % of one core ⚠ VERIFY by measuring with `perf stat`/`strace -c` on your machine.

**Q4.** Memory/scheduling estimate: 2 000 talkgroup sockets, thread-per-socket. Give virtual stack reservation at 8 MiB, the number of wake-ups per second at 50 pps/socket if all are active, and one correctness hazard that does not exist in the single-thread design. ⚠ VERIFY the stack default on your system with `ulimit -s`.

**Q5.** `poll` vs `epoll` cost comparison: n = 8 and n = 10 000 sockets, k = 3 ready per wake-up. Give the per-wake-up work (entries copied/scanned) for each API and state which you'd pick for each n.

**Q6.** An edge-triggered `epoll` UDP handler reads **one** datagram per event. Five datagrams arrive back-to-back. What happens? And with a level-triggered `poll` loop doing the same?

**Q7.** Write the event-loop pseudocode for "poll sockets + 20 ms audio tick + one-shot 3 s call-setup timer". How do you compute the poll timeout from two deadlines? What changes if the audio thread must never block on I/O (hand off via SPSC queue)? *(Link: `01-cpp/06-concurrency-rt`, `capstone/M2`.)*

**Q8.** A length-prefixed TCP message protocol has a 4-byte big-endian length then payload. Describe the incremental parser states for a `recv` that returns (a) 2 bytes, (b) 1 000 bytes containing 2.5 messages. What must you do about a length of 4 GiB from an attacker? *Context: gRPC's 5-byte message header.*

**Q9.** Explain `recv() == 0` for TCP vs UDP, and `EAGAIN` vs `EWOULDBLOCK`. What does `POLLHUP` mean?

---

## Part D — UDP and multicast socket details

* **Binding:** `bind(0.0.0.0:p)` accepts datagrams for any local address; `bind(127.0.0.1:p)` only loopback. Port 0 = kernel picks an ephemeral port; read it back with `getsockname` (the tests do this to avoid port clashes).
* **`SO_REUSEADDR`:** lets you re-bind a TCP listener while old connections sit in TIME_WAIT, and (for multicast) lets several sockets share `group:port` — so two local receivers can both join. `SO_REUSEPORT` (Linux ≥ 3.9) additionally load-balances unicast datagrams/connections across sockets — a multi-thread scaling tool.
* **Receive buffer (`SO_RCVBUF`):** datagrams queue here until you read them. If you stall (GC pause, debugger, scheduling), **the kernel drops new datagrams** — visible as `RcvbufErrors` in `/proc/net/snmp` / `netstat -su` ⚠ VERIFY. Default `rmem_default` is ~208 KiB on many distros ⚠ VERIFY. Each small packet costs far more than its payload in kernel memory ("truesize", ~ 0.75–2.3 KiB ⚠ VERIFY).
* **Multicast sender:** `IP_MULTICAST_IF` (choose the NIC — critical on a gateway with an LMR-side and a core-side network, otherwise the routing table picks one), `IP_MULTICAST_TTL` (default 1 = stays on the link), `IP_MULTICAST_LOOP` (hear your own packets on the same host — needed for single-host tests, usually off on a production sender).
* **Multicast receiver:** `bind` to the group address (Linux) or `INADDR_ANY`, then `IP_ADD_MEMBERSHIP` with `{group, interface}`. Membership triggers the IGMP report (pack 02, Part G). Binding to `INADDR_ANY:port` would also receive *other* groups using the same port.
* **Why `sendto` is atomic for UDP:** returns either `n` or an error — no partial datagrams.

> **Common misconceptions**
> 1. *"If `join` succeeds I receive the stream."* The network path (snooping/PIM) and the right interface must also be right.
> 2. *"The receiving app is slow, so the sender is blocked."* UDP has no back-pressure — the kernel silently drops.
> 3. *"Multicast to myself works on any interface."* Loopback may not carry multicast (the test skips if so).

**Q10.** The gateway has `eth0` (LMR-side LAN, 10.1.0.0/16) and `eth1` (core-side, 10.2.0.0/16) and the default route is on `eth1`. Where do multicast packets go if you do not set `IP_MULTICAST_IF`? Which three socket options does a production talkgroup sender set, and what are sensible values? *Context: talkgroup audio must reach LMR-side repeaters, not the core.*

**Q11.** One UDP socket receives RTP for 100 talkgroups at 50 pps. The audio thread stalls for 50 ms. How many packets queue, what receive-buffer size do you need if each costs ~2 KiB of kernel memory, and how do you detect that drops occurred? ⚠ VERIFY truesize on your kernel.

---

## Part E — Wireshark / tcpdump workflow

**Capture** (needs privilege; own traffic only):
```text
tcpdump -i lo -n -w echo.pcap 'udp port 9000'          # BPF *capture* filter (syntax differs from display filters)
tcpdump -i any -n -vv 'igmp or host 239.255.42.99'
tshark -r echo.pcap -Y 'udp.port==9000' -T fields -e frame.number -e udp.length -e udp.checksum.status
```
**Display filters** (Wireshark bar): `udp.port == 9000`, `ip.addr == 10.0.0.1`, `tcp.flags.syn == 1`, `tcp.analysis.retransmission`, `igmp`, `ip.dsfield.dscp == 46`, `rtp.ssrc == 0x11223344`, `sip.Method == "INVITE"`, `ip.flags.mf == 1 || ip.frag_offset > 0`.
**RTP:** if UDP port is not auto-decoded: *Analyze → Decode As… → UDP port → RTP*; *Telephony → RTP → RTP Streams → Analyze* gives loss, sequence errors, jitter (computed by the RFC 3550 estimator in `04-rtp-sip`).

**Artefacts that look like bugs but are not:**
* *"UDP checksum incorrect [maybe caused by checksum offload]"* on **outgoing** packets captured on the sender — the NIC fills the checksum after the capture point.
* Zero checksum on IPv4 UDP: legal (pack 01).
* **TCP segmentation offload** shows frames larger than the MTU on the sending host.
* On `lo`, MTU is 65 536, so you will not see fragmentation.

**What to verify per build:**
| Build | In Wireshark confirm |
|-------|----------------------|
| UDP echo | UDP length = 8 + payload; reply has swapped ports; both packets IPv4 TTL 64, DF set, IP ID |
| Multicast pair | IGMP membership report on `rx` start; destination MAC `01:00:5e:7f:2a:63` for 239.255.42.99; TTL 1 |
| Poll server (TCP) | 3-way handshake (SYN, SYN-ACK, ACK); FIN exchange on close; echo payloads |

> **Common misconceptions**
> 1. *"Wireshark shows what is on the wire."* It shows what the capture point saw — offloads, VLAN tag stripping and tunnels change that.
> 2. *"A capture filter and a display filter are the same language."* BPF (capture) vs Wireshark display — different syntax.

**Q12.** Write display filters for: (a) RTP packets from SSRC `0x11223344`; (b) every TCP retransmission; (c) SIP INVITEs; (d) IGMP traffic for 239.1.1.1; (e) packets marked EF; (f) UDP datagrams > 1 000 B; (g) any fragmented IPv4 packet; (h) TCP SYN without ACK (new connection attempts).

**Q13.** Wireshark flags every outgoing packet with a bad UDP checksum on your Linux laptop but not on the receiver's capture. Explain, and give one command to confirm. (`ethtool -k <if>` shows `tx-checksumming`.)

---

## Part F — The three builds (run them!)

### Build 1 — UDP echo (`examples/udp_echo.cpp`)
Implement in `include/sockets.hpp`: `Fd`, `make_addr`, `make_udp_socket`, `local_port`, `recv_datagram`, `send_datagram`, `udp_echo_once`. Tests: `Fd.*`, `Udp.*`.
Run: `./udp_echo server 9000` / `./udp_echo client 9000 hello`; capture on `lo`.

### Build 2 — Multicast pair (`examples/mcast_pair.cpp`)
Implement `join_multicast`, `make_multicast_receiver`, `make_multicast_sender`. Test `Multicast.SenderReachesTwoReceivers` (skips automatically if your environment cannot do loopback multicast). Run two receivers + one sender; capture IGMP.

### Build 3 — poll() server (`examples/poll_server.cpp`)
Implement `set_nonblocking`, `tcp_listen`, `tcp_connect`, `send_all`, `recv_some`, `PollServer` (UDP echo for many sockets + TCP accept/echo, all in `run_once`). Tests: `Tcp.*`, `PollServer.*`. Run with `strace -f -e trace=poll,accept,recvfrom ./poll_server 9100 8` to *see* the multiplexing.

```bash
cmake -S code -B build && cmake --build build
./build/sockets_tests_student        # red until you implement code/include/sockets.hpp
./build/sockets_tests_solution       # green reference
```
Design checklist for `PollServer::run_once` — (1) build `pollfd` array from entries; (2) `poll`; (3) for each `revents`: UDP → read until EAGAIN and echo; listener → `accept` until EAGAIN, set non-blocking; connection → `recv`, echo, close on EOF/error; (4) **do not mutate the entries vector while iterating** — collect `dead`/`added` and compact afterwards (iterator invalidation is the bug everyone hits).

**Stretch:** replace `poll` with `epoll` (`epoll_create1`, `epoll_ctl`, `epoll_wait`), add per-connection output buffers + `POLLOUT`, and add `timerfd_create` for the 20 ms tick.

---

## Part G — Review exercises

**Q14.** Find the bugs (at least 8):

```cpp
int s = socket(AF_INET, SOCK_DGRAM, 0);
sockaddr_in a;
a.sin_family = AF_INET;  a.sin_port = 9000;
a.sin_addr.s_addr = INADDR_ANY;
bind(s, (sockaddr*)&a, sizeof(a));
char buf[100];
while (1) {
    int n = recvfrom(s, buf, 100, 0, (sockaddr*)&peer, &len);
    sendto(s, buf, n, 0, (sockaddr*)&peer, len);
}
```
*Context: a first-draft gateway echo/keepalive responder.*

**Q15.** (Oral, 2 min) "Compare select, poll and epoll and tell me which you'd use for a radio gateway with 8 sockets, and for 8 000."

**Q16.** Explain `SIGPIPE`, `EINTR` and partial `send` — what each breaks in a naive TCP client, and the fix.

**Q17.** Architecture: sketch the threads of a gateway: (I/O thread with epoll, audio-tick thread, control thread). What crosses thread boundaries, and how (SPSC ring, atomics)? Where do you *not* allocate or lock? *Context: real-time habits, `01-cpp/06`.*

---

## ⚠ VERIFY list (this pack)
1. Default thread stack 8 MiB (`ulimit -s`); default `ulimit -n` = 1024 (soft).
2. `rmem_default` ≈ 208 KiB; per-skb truesize for small packets.
3. `RcvbufErrors` location in `/proc/net/snmp` / `netstat -su`.
4. Linux multicast bind-to-group semantics; `SO_REUSEPORT` ≥ Linux 3.9.
5. Loopback multicast support in WSL2/containers (test auto-skips).
6. `close()` + `EINTR` behaviour on Linux (do not retry) — man 2 close, "Dealing with error returns from close()".
7. Wireshark checksum-offload message wording.
8. Your numerical estimates in Q4, Q5, Q11 (measure with `perf`/`strace -c`).
