# Cheatsheet — 03-net/03 Sockets, RAII, poll, Wireshark
*Each line cites the worksheet part that derived it. ⚠ = VERIFY.*

## Lifecycle (B.1)
- UDP: `socket → bind → recvfrom/sendto`. TCP server: `socket → bind → listen → accept (NEW fd) → recv/send`. Client: `socket → connect`.
- Port/addr are network order: `htons`, `inet_pton`. Zero-init `sockaddr_in`. `recvfrom` len in/out must start at `sizeof`.
- Port 0 + `getsockname` = free port (tests).

## RAII Fd (B.2)
- Move-only; dtor closes once; move-assign: `reset(o.release())` with self-check; moved-from = −1; no `EINTR` retry on `close`.
- Raw int bugs: leak (EMFILE), double close, recycled-fd cross-talk, exception path leaks.

## Event loop (C)
- Thread-per-socket: 8 MiB virtual stack ⚠ each, context switches, locks. Single poll loop: no locks, predictable.
- `poll(fds,n,timeout)`: 0 timeout, −1 error (EINTR), else #ready. Level-triggered → drain to `EAGAIN`.
- select (1024 limit, O(max fd)) · poll (O(n)) · epoll (O(ready), edge needs drain) · kqueue/IOCP.
- Non-blocking: `EAGAIN`/`EWOULDBLOCK`. TCP: byte stream → framing + partial `send` buffer + `POLLOUT`. `recv==0`: TCP EOF, UDP empty datagram.
- `MSG_NOSIGNAL` (or ignore SIGPIPE). Retry `EINTR`.
- Timer: poll timeout = (next absolute deadline − now); advance `tick += 20ms`, never sleep-after-work.
- Don't mutate the fd vector mid-iteration: collect dead/added, compact after.

## UDP/multicast (D)
- `SO_RCVBUF` full → silent drops (`RcvbufErrors`) ⚠. Default ~208 KiB ⚠.
- Sender: `IP_MULTICAST_IF` (pick NIC!), `IP_MULTICAST_TTL` (1 = link), `IP_MULTICAST_LOOP` (0 in prod).
- Receiver: `SO_REUSEADDR`, bind(group:port), `IP_ADD_MEMBERSHIP{group,iface}` → IGMP report.
- Echo servers on `INADDR_ANY` are reflection gadgets.

## Wireshark / tcpdump (E)
- Capture: `tcpdump -i lo -n -w x.pcap 'udp port 9000'` (BPF ≠ display filter).
- Display: `udp.port==`, `tcp.flags.syn==1`, `tcp.analysis.retransmission`, `igmp`, `ip.dsfield.dscp==46`, `rtp.ssrc==0x…`, `sip.Method=="INVITE"`, `ip.flags.mf==1 || ip.frag_offset>0`.
- RTP: Decode As → RTP; Telephony → RTP Streams → Analyze (loss, jitter).
- False alarms: checksum offload, TSO big frames, lo MTU 65 536.
- Multicast MAC check: 239.255.42.99 → `01:00:5e:7f:2a:63`.

## Builds (F)
1. UDP echo (`udp_echo_once`) → verify swapped ports, UDP length = 8 + payload.
2. Multicast pair → IGMP report; both receivers get one datagram.
3. poll server → many UDP sockets + TCP accept/echo in one thread; `strace -f -e trace=poll,accept,recvfrom`.
