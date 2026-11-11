# VERIFY checklist

Items flagged **VERIFY** in worksheets/answers. Check each against the cited standard (or datasheet) before treating it as interview-ready fact. Synthetic signals only — never live public-safety RX.

| Status | Pack | Item |
|--------|------|------|
| [ ] | $key | *Costs and limits (why not on the radio). Roughly 2x CPU and several x memory (VERIFY against your toolchain docs; order of magnitude is what matters). A sma... |
| [ ] | $key | *Q3. *Reasoning:* ASan keeps shadow memory for addresses and replaces `malloc`/`free`; TSan keeps different shadow state (per-memory-location access history ... |
| [ ] | $key | *Q6. *Reasoning:* `0x12 = 0001 0010`, `0x34 = 0011 0100` -> XOR `0010 0110 = 0x26`; `0x56 = 0101 0110` -> XOR `0111 0000 = 0x70`. `{01,02}` -> `0x03`; `{02,0... |
| [ ] | $key | *Concrete first. On the radio's 16-bit-`int` microcontroller `sizeof(int) == 2`; on the Linux gateway `sizeof(int) == 4`. The standard guarantees only `sizeo... |
| [ ] | $key | constexpr unsigned kSymbolRate = 4800;                         // DMR: symbols per second (VERIFY ETSI TS 102 361-1) |
| [ ] | $key | constexpr unsigned kSlotMs = 30;                               // DMR timeslot length (VERIFY) |
| [ ] | $key | static_assert(kSlotSymbols == 144, "30 ms * 4800 sym/s = 144 symbols (288 bits at 2 bits/symbol; VERIFY)"); |
| [ ] | $key | *Q7. 64-bit typical: `Buffer` 16; `UniqueBox<T>` 8; `shared_ptr<T>` 16 (object pointer + control block pointer); `vector<uint8_t>` 24 (begin, end, end-of-cap... |
| [ ] | $key | *Q28. Because the move constructor isn't `noexcept` (and the type is copyable): the vector uses `move_if_noexcept` to preserve the strong guarantee. Fix: dec... |
| [ ] | $key | *Derive. A function that may fail to produce a value needs a way to say so. Options: (a) sentinel (`-1`, `0xFFFF`) - collides with legal values (all 65,536 `... |
| [ ] | $key | *Derive. A C `union` overlaps its members' storage but does not remember which one is active - you need a separate tag, and non-trivial members (strings) are... |
| [ ] | $key | *Q6. `optional<uint8_t>` 2 (1 + 1 flag); `optional<uint16_t>` 4 (2 + flag, padded to alignment 2); `optional<uint32_t>` 8; `optional<uint64_t>` 16 (alignment... |
| [ ] | $key | *Concrete first. `labs/lab_shift.cpp` computes `1u << n` with `n == 32`. On x86-64 and AArch64 it prints `1` (VERIFY on your CPU); on 32-bit ARM register shi... |
| [ ] | $key | / CPU family (VERIFY) / `1u << 32` / `1u << 33` / |
| [ ] | $key | x86 and most ARM configurations are little-endian; network/telecom specs are overwhelmingly big-endian ("network byte order"; most-significant byte first mat... |
| [ ] | $key | *Q18. *Context: planning struct layouts for a shared-memory ring.* Implement `is_pow2`, `align_up`, `is_aligned`, `field_offset`, `struct_size` in `layout.hp... |
| [ ] | $key | *Q9. x86-64 (and AArch64): `1u << 32` -> count masked to 5 bits -> `1 << 0` = 1 (VERIFY on your CPU). 32-bit ARM register shift: 0. UBSan: `shift exponent 32... |
| [ ] | $key | A radio gateway is several things happening at once: a thread reading the radio/DSP interface, a protocol thread running the call state machine, a network th... |
| [ ] | $key | *Priority inversion (preview of Â§3.5). A high-priority thread H needs a mutex held by low-priority L. A medium-priority thread M becomes runnable and pre-em... |
| [ ] | $key | *Lock-free or not? `std::atomic<T>::is_lock_free()` tells you whether the CPU can do it with real atomic instructions or the library silently uses a hidden m... |
| [ ] | $key | Which hardware does which (VERIFY for your exact core): x86-64 (TSO) keeps stores in order, loads in order, and only lets a later *load* pass an earlier *sto... |
| [ ] | $key | *Rule 3 - no exceptions (`throw`). Why? The first throw has to find a handler by walking unwind tables, running destructors on the way, usually allocating th... |
| [ ] | $key | *Also decide your scheduling. On a real-time OS or Linux with `SCHED_FIFO`, priority decides who runs; a high-priority thread that busy-waits can starve the ... |
| [ ] | $key | [ ] I can say which hardware reorderings exist on x86 vs ARM (and flag what to VERIFY). |
| [ ] | $key | Decision rule: start with a mutex; it's simplest, correct, and cheap when uncontended. Use an atomic for a *single* independent word whose meaning is self-co... |
| [ ] | $key | A high-priority task H blocks on a lock held by a low-priority task L; a medium-priority task M pre-empts L; H effectively waits for M. Prevention: priority ... |
| [ ] | $key | > Safety: all call events, ids and payloads here are synthetic; never connect these exercises to live public-safety traffic. Timing figures for hang time and... |
| [ ] | $key | All 25 other (state, event) pairs are *ignored*: no state change, no action. Why does each row exist? *Hang* exists because after a talker releases PTT, some... |
| [ ] | $key | *False sharing - a performance rule. CPUs move memory between cores in cache lines (typically 64 bytes - VERIFY for your target). If `head_` and `tail_` live... |
| [ ] | $key | Each answer: reasoning first, then common wrong answers (why tempting / why wrong). Try the question yourself before reading. Section references are to `work... |
| [ ] | $key | `Msg`: the largest alternative is 4 bytes (`uint32_t`, alignment 4); the variant adds a 1-byte index after the storage: 4 + 1 = 5, padded to alignment 4 -> `... |
| [ ] | $key | *Model answer. Not only speed. (1) `x mod 2^k` = low k bits = `x & (2^k - 1)` (derive from binary: the remainder of division by a power of two is the discard... |
| [ ] | $key | *Model answer. Pure `step(state, event) -> {next, action, handled}`; `enum class` state and event, `switch` on state with no `default`; a thin `CallMachine` ... |
| [ ] | $key | Synthetic example. P25 frames carry (after the frame sync) a Network ID. Its first 16 data bits are `[ NAC : 12 bits ][ DUID : 4 bits ]`, NAC first (high bit... |
| [ ] | $key | (DUID value 5 means "Logical Link Data Unit 1" in the standard â€” VERIFY the table; our code does not depend on the meaning.)* |
| [ ] | $key | *Q8 â€” Calculate. *Context: checksum/CRC groundwork.* In 8 bits compute `âˆ’x` for x = `0x14` and x = `0x80` using `~x + 1`; verify `x + (âˆ’x) â‰¡ 0`. What... |
| [ ] | $key | *VERIFY items for this pack: P25 NID layout and DUID table (TIA-102.BAAA-A); that your target compiler is C++17/20 and defines out-of-range unsignedâ†’signed... |
| [ ] | $key | 2. Hardware set/clear registers (write-1-to-set / write-1-to-clear aliases; Cortex-M bit-banding where present): the write itself is the operation, no read n... |
| [ ] | $key | *VERIFY (this pack): P25 NID bit layout and DUID values (TIA-102.BAAA-A); `std::popcount` availability on your toolchain; MCU-specific set/clear registers. |
| [ ] | $key | Names: Danny Cohen's 1980 note *"On Holy Wars and a Plea for Peace"* (IEN 137; later in IEEE Computer, 1981) borrowed *Big-Endians* and *Little-Endians* from... |
| [ ] | $key | Internet protocol headers define multi-byte integers to be transmitted most-significant byte first, i.e. big-endian. This is called network byte order. It wa... |
| [ ] | $key | > 2. *"RTP audio samples are in network byte order."* RTP payload formats define their own (e.g., 16-bit linear audio L16 is big-endian per RFC 3551; Î¼-law ... |
| [ ] | $key | *This is RFC 1982 *Serial Number Arithmetic* (R. Elz & R. Bush, 1996, written for DNS zone serials). Its definition, for `SERIAL_BITS = N`: `s1 < s2` iff `(i... |
| [ ] | $key | *When it fails â€” know the assumption. The method assumes two packets being compared are less than half a turn apart (32,768 packets â‰ˆ 10.9 minutes at 50 ... |
| [ ] | $key | This is RFC 3550's *extended highest sequence number received* (Â§6.4.1 / A.1): `cycles + max_seq`. *(VERIFY section numbers.)* |
| [ ] | $key | *VERIFY list: IEN 137 citation details; RFC 791 Appendix B text; RFC 1700 "big-endian" sentence; RFC 1982 Â§3 definition and "undefined at half" clause; RFC ... |
| [ ] | $key | `37000 âˆ’ 1000 = 36000` ticks clockwise (â‰¥ 32768). `static_cast<int16_t>(36000)` = `36000 âˆ’ 65536 = âˆ’29536` â†’ `seq_newer16(37000, 1000)` is false. T... |
| [ ] | $key | *"Accept any packet as new if far away."* â€” Allows an attacker or a stale sender to hijack sequence state; hold and verify. |
| [ ] | $key | *VERIFY (this pack): see the list at the end of the worksheet (RFC 791 App. B, RFC 1700, RFC 1982 Â§3, RFC 3550 Â§5.1/Â§6.4.1/A.1, RFC 3551, IEN 137). |
| [ ] | $key | *Failure if ignored: the Heartbleed class of bug (a length field trusted without bounds-checking against the real buffer; TLS heartbeat, 2014 â€” VERIFY) is ... |
| [ ] | $key | / X extension / 1 / 0 / One header extension follows the CSRC list: 16-bit profile id, 16-bit length (in 32-bit words, excluding those 4 bytes), then data. (... |
| [ ] | $key | Synthetic-but-classic example (this exact header is used in the Wikipedia checksum example â€” VERIFY there): |
| [ ] | $key | *VERIFY list: RFC 3550 Â§5.1 (header, padding, CSRC, extension, SSRC Â§8), RFC 3551 (PT table, marker semantics Â§4.1), RFC 8285 (extension forms), RFC 791 Â... |
| [ ] | $key | *(b) The IPv4 header checksum (Pack 04 Â§3.8) is computed over the header with the checksum field zeroed; UDP's covers a pseudo-header + UDP header + data. A... |
| [ ] | $key | Order: (1) `data != nullptr && len >= 20`; (2) read version/IHL, reject `version != 4`, `ihl < 5`; (3) `hl = ihl*4`, require `len >= hl`; (4) read `total_len... |
| [ ] | $key | *VERIFY (this pack): see list at end of worksheet (RFC 3550/3551/8285/791/768/2474/3168/8200/1918/1624, Wikipedia IPv4 example, Heartbleed). |
| [ ] | $key | / LMR air interfaces / Data headers/blocks and packet data are protected with CRCs (P25 TIA-102.BAAA/BAAB and DMR ETSI TS 102 361 define which polynomial pro... |
| [ ] | $key | *Check values for `"123456789"` (all verified against an independent bit-level computation; VERIFY against the RevEng catalogue): |
| [ ] | $key | ### 3.6 Verify against `"123456789"` |
| [ ] | $key | `4500+0073 = 4573; +4000 = 8573; +4011 = c584; +c0a8 = 1862c â†’ 862d; +0001 = 862e; +c0a8 = 146d6 â†’ 46d7; +00c7 = 479e`. Checksum `~479e = b861` âœ“. Veri... |
| [ ] | $key | *Incremental update (rewriting gateway): when you change one 16-bit word `m â†’ m'`, `HC' = ~(~HC + ~m + m')` in one's-complement arithmetic (RFC 1624 â€” VE... |
| [ ] | $key | *Q2 â€” Calculate. *Context: toy link protecting 4-bit control words.* With `g = xÂ³ + x + 1` (`1011`), compute the 3-bit CRC and the 7-bit codeword for mess... |
| [ ] | $key | *Q9 â€” Calculate. *Context: IPv4 header in a gateway.* Compute the Internet checksum of `45 00 00 73 00 00 40 00 40 11 00 00 c0 a8 00 01 c0 a8 00 c7` by han... |
| [ ] | $key | *VERIFY list: which polynomial/init/xorout P25 (TIA-102.BAAA/BAAB) and DMR (ETSI TS 102 361-x) use per field; RevEng/Rocksoft catalogue values (https://reven... |
| [ ] | $key | Each answer: reasoning â†’ common wrong answers (*why tempting* / *why wrong*). `Â§` = [`worksheet.md`](worksheet.md) section. All data synthetic. Numeric CR... |
| [ ] | $key | `4500+0073 = 4573`; `+0000 = 4573`; `+4000 = 8573`; `+4011 = c584`; `+0000 = c584`; `+c0a8 = 1862c` â†’ fold `862c+1 = 862d`; `+0001 = 862e`; `+c0a8 = 146d6`... |
| [ ] | $key | *(c) TTL changes â‡’ the IPv4 header checksum must be updated. Recompute over the header, or use the RFC 1624 incremental formula `HC' = ~(~HC + ~m + m')` (V... |
| [ ] | $key | *VERIFY (this pack): see the list at the end of the worksheet (P25/DMR CRC fields, RevEng catalogue, RFC 1071/1624/768/1141, residue constants, SSE4.2 = CRC-... |
| [ ] | $key | / LMR (P25, DMR) / Control words (network ID, link control, headers) get strong block codes; voice gets weaker codes plus tolerance of residual errors. Speci... |
| [ ] | $key | *Extended Golay (24,12,8): add an overall parity bit: `d = 8` â‡’ correct 3 and detect 4. Shortened codes: to protect fewer data bits (e.g. 6), fix the missi... |
| [ ] | $key | Reading codes: BCH(n, k, d). P25's NID is BCH(63, 16, 23): `n = 63 = 2â¶ âˆ’ 1`, `k = 16` (NAC 12 + DUID 4, Pack 01 Â§3.3), `d = 23` â‡’ `t = âŒŠ22/2âŒ‹ = 1... |
| [ ] | $key | *Why RS for bursts: it corrects *symbols*; a burst of up to 8 consecutive bit errors inside an 8-bit symbol is one symbol error. Real RS uses symbols of `m` ... |
| [ ] | $key | *Q5 â€” Calculate. *Context: encoding a P25-style 4-bit field with Hamming(7,4).* Encode nibbles `0110` and `1001` with the position layout of Â§3.4; give co... |
| [ ] | $key | *VERIFY list: which codes P25 (TIA-102.BAAA-A, BAAB) and DMR (ETSI TS 102 361-1, -2) use for each field: BCH(63,16,23) NID, Golay(23,12,7)/(24,12,8)/(18,6,8)... |
| [ ] | $key | (Verify BCH(31,16,7) parameters, VERIFY.) |
| [ ] | $key | / Convolutional code + Viterbi / Every output bit depends on the last few input bits; the decoder finds the single most likely input sequence / Wi-Fi (IEEE 8... |
| [ ] | $key | / Interleaver / Reorders bits so a burst of channel errors turns into scattered single errors / Virtually every radio standard: P25, DMR, GSM, LTE (VERIFY ea... |
| [ ] | $key | / LFSR scrambler / XORs the stream with a pseudo-random sequence to remove long runs of 0s/1s (timing recovery) and flatten the spectrum / 802.11 (xâ·+xâ´+... |
| [ ] | $key | *Hard vs soft decisions. Our decoder sees only bits (0/1) â€” *hard decision*. A real receiver can pass the *analog confidence* of each bit (soft decision) a... |
| [ ] | $key | Other cousins (VERIFY numbers): LC 746 *Min Cost Climbing Stairs* (1 predecessor pair), LC 120 *Triangle*, LC 64 *Minimum Path Sum* (2D grid), LC 72 *Edit Di... |
| [ ] | $key | A linear-feedback shift register is a shift register whose new input bit is the XOR of selected earlier bits. Polynomial `xâ· + xâ´ + 1` (the 802.11 scramb... |
| [ ] | $key | With seed all-ones (`0x7F`) the output begins `0000 1110 1111 0010â€¦` (first byte `0x0E`) â€” the well-known 802.11 sequence (VERIFY against the standard's ... |
| [ ] | $key | *What it needs: the receiver's keystream must be aligned with the transmitter's: same polynomial, same seed, same bit position. If the seed is wrong, or one ... |
| [ ] | $key | > 3. *"Scramble after the CRC."* Apply it consistently on both ends; in 802.11 the scrambler runs on the data *before* convolutional coding (VERIFY), and the... |
| [ ] | $key | *Q2 â€” Calculate. *Context: hand-checking a test vector.* Encode `1101` with the (7,5) K=3 encoder including the 2-bit tail, in a table like Â§3.2. Then ver... |
| [ ] | $key | *Q12 â€” Connect. *Context: interview "have you seen DP in the wild?"* (a) Fill the DP table for LC 931 on `{{2,1,3},{6,5,4},{7,8,9}}`, recover the path, and... |
| [ ] | $key | *VERIFY list: IEEE 802.11 Â§17.3.5.5 scrambler `xâ·+xâ´+1` and the 0x0E start for the all-ones seed; 802.11 convolutional code K=7 (133, 171 octal) and pun... |
| [ ] | $key | *(c) The first 7 data bits are 0, so the first 7 transmitted bits equal the first 7 keystream bits. Because the state *is* the last 7 outputs (Â§3.7), the re... |
| [ ] | $key | *(b) LC 746 *Min Cost Climbing Stairs*, LC 120 *Triangle*, LC 64 *Minimum Path Sum*, LC 72 *Edit Distance* (Viterbi on an alignment lattice) (VERIFY numbers). |
| [ ] | $key | *(c) Typical TX chain: `payload â†’ CRC (Pack 04, detect) â†’ [header: strong block code, Pack 05; payload: convolutional + tail] â†’ interleave â†’ (scrambl... |
| [ ] | $key | "Decoding a convolutional code means finding the input path through a trellis whose output is closest to the received bits. The number of paths is 2â¿, but ... |
| [ ] | $key | Conventions: all multi-byte header fields are big-endian ("network order"). Hex is shown as bytes. `âš  VERIFY` = a figure I could not confirm from memory wi... |
| [ ] | $key | EtherType (16 bits): demultiplex key to the layer above: `0x0800` IPv4, `0x86DD` IPv6, `0x0806` ARP, `0x8100` VLAN tag. Original 802.3 used this field as a *... |
| [ ] | $key | On the wire only: 7-byte preamble + 1-byte SFD + 12-byte inter-frame gap = 20 bytes of "invisible" overhead per frame (needed when computing *line-rate* util... |
| [ ] | $key | *Q6. Part A header: compute the IPv4 header checksum by hand (show the 16-bit additions and the fold). Then verify the receiver-side sum equals `0xFFFF`. *Co... |
| [ ] | $key | / Header checksum / Link CRC + L4 checksum already exist; removing it saves per-hop work (TTL change needs no recompute). Consequence: UDP checksum becomes m... |
| [ ] | $key | Ports: demultiplex to a process. *Source port is optional in spirit* (may be 0 if no reply is expected); RTP conventionally uses an even port with RTCP on po... |
| [ ] | $key | / CRC32c / Stronger than Adler-32 (original) and has hardware support (SSE4.2); âš  VERIFY RFC 3309â†’4960 history. / |
| [ ] | $key | *Q21. (Integration table) For each Etherstack interface give the transport and well-known port, or write VERIFY: RTP voice, RTCP, SIP, SIP-TLS, gRPC, GTP-U (... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | *Q21. (âš  VERIFY all) |
| [ ] | $key | *UDP: nothing is retransmitted. TCP: the sender notices after a retransmission timeout of RTO = 200 ms (generous; real stacks are â‰¥ 200 ms, RFC 6298 recomm... |
| [ ] | $key | Human conversation tolerates â‰ˆ 150 ms one-way mouth-to-ear delay before it feels broken (ITU-T G.114 âš  VERIFY: 150 ms recommended, up to 400 ms tolerable... |
| [ ] | $key | > 3. *"Retransmitting voice always hurts."* Selective retransmit within the jitter-buffer window (RTCP NACK, RFC 4585 âš  VERIFY) or FEC can help; TCP's blin... |
| [ ] | $key | *Why not two messages? Suppose a SYN from an *old, dead* connection wanders the network and arrives late (delayed duplicate). With a 2-way handshake the serv... |
| [ ] | $key | *Teardown: FIN in each direction (a half-close is legal). The side that sent the last ACK sits in TIME_WAIT for 2Ã—MSL (MSL = 60 s in the RFC â†’ 120 s; Linu... |
| [ ] | $key | *Q6. A gateway opens a new TCP connection per call-control request to one core node (same dst IP:port). Ephemeral range 32768â€“60999 (28 232 ports) and TIME... |
| [ ] | $key | Slow start: start at IW (initial window, 10 segments in modern stacks, RFC 6928 âš  VERIFY); each ACK adds 1 MSS â†’ `cwnd` doubles every RTT (exponential) u... |
| [ ] | $key | Mathis formula (steady state, random loss p): `throughput â‰ˆ (MSS / RTT) Ã— 1.22 / âˆšp`. Derivation sketch: the sawtooth between W/2 and W loses one packet... |
| [ ] | $key | *Design rule: size voice packets so `IP + UDP + RTP + payload â‰¤ path MTU` (budget tunnel headers too: GTP-U adds outer IP 20 + UDP 8 + GTP 8 = 36 B (IPv4) ... |
| [ ] | $key | *Q13. (a) SIP INVITE with SDP of 1 800 bytes over UDP on a 1 500-MTU path: list fragments. (b) RFC 3261 says to switch to TCP when a request is within 200 B ... |
| [ ] | $key | *Q15. Compute the maximum RTP payload for IPv6 over a GTP-U tunnel with outer IPv6 on a 1500-MTU path (outer IPv6 40 + UDP 8 + GTP-U 8 = 56; inner: IPv6 40 +... |
| [ ] | $key | *Addressing: IPv4 multicast = 224.0.0.0/4 (first nibble `1110`). Useful ranges: 224.0.0.0/24 link-local control (not forwarded; e.g. 224.0.0.1 all-hosts, 224... |
| [ ] | $key | *Q16. Compute the 8-byte IGMPv2 membership report for 239.1.1.1 including checksum (type 0x16, max-resp 0). Verify the receiver sum is 0. *(Test `Multicast.I... |
| [ ] | $key | / (VOICE-ADMIT) / 44 / 101100 / admitted voice, RFC 5865 âš  VERIFY / |
| [ ] | $key | TOS byte for EF = `101110 00` = 0xB8. 802.1Q PCP 5 conventionally carries voice at L2 âš  VERIFY mapping per vendor. |
| [ ] | $key | *Q21. MCPTT floor-control (3GPP TS 24.380) runs over UDP with its own retransmission timers instead of TCP âš  VERIFY. Give the engineering argument. *Contex... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | *Q6. Port budget / TIME_WAIT: 28 232 / 60 s â‰ˆ 470 connections/s to the same dst IP:port (the 4-tuple space is exhausted). Fixes: persistent connections / m... |
| [ ] | $key | *Q20. (1) The app/stack never set it: wrong option (IPv6 needs `IPV6_TCLASS`), `setsockopt` error ignored, or the OS overwrote it â†’ check `getsockopt`/capt... |
| [ ] | $key | *Q22. Outline: RTP needs *late = lost* semantics: TCP/QUIC streams retransmit and deliver in order (HOL), QUIC removes HOL between streams but each stream is... |
| [ ] | $key | 2. Thread per socket â€” works, but costs: each thread has a stack (default 8 MiB virtual on Linux âš  VERIFY; the pages are committed lazily, but kernel sta... |
| [ ] | $key | > 4. *"One thread can't handle 100 streams."* 100 Ã— 50 pps = 5 000 pps; at roughly 2â€“10 Âµs per `recvfrom` + handling that is 1â€“5 % of one core âš  VERI... |
| [ ] | $key | *Q4. Memory/scheduling estimate: 2 000 talkgroup sockets, thread-per-socket. Give virtual stack reservation at 8 MiB, the number of wake-ups per second at 50... |
| [ ] | $key | Receive buffer (`SO_RCVBUF`): datagrams queue here until you read them. If you stall (GC pause, debugger, scheduling), the kernel drops new datagrams â€” vis... |
| [ ] | $key | *Q11. One UDP socket receives RTP for 100 talkgroups at 50 pps. The audio thread stalls for 50 ms. How many packets queue, what receive-buffer size do you ne... |
| [ ] | $key | *What to verify per build: |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | *Q8. States: `NeedHeader(have k of 4)` â†’ `NeedBody(len, have m)` â†’ emit â†’ `NeedHeader`. (a) 2 bytes: append to the buffer, still `NeedHeader` with 2/4.... |
| [ ] | $key | *Q11. 100 Ã— 50 = 5 000 pps; 50 ms â†’ 250 packets queued; Ã— 2 KiB â‰ˆ 500 KiB > default ~208 KiB â†’ roughly the last 60 % of the burst is dropped. Need `S... |
| [ ] | $key | / V / 2 / Version = 2. Also a demultiplexing guard: on a shared port, STUN starts with bits `00`, DTLS 20â€“63, RTP/RTCP 128â€“191 as the first byte, so V=2 ... |
| [ ] | $key | / X / 1 / Header extension follows the CSRC list: `profile(16) length(16, in words)` + data. Used for per-packet metadata (audio level, MID) without changing... |
| [ ] | $key | / PT / 7 / Payload type: which codec/format. 0â€“95 static (RFC 3551: 0 = PCMU, 8 = PCMA, 9 = G.722, 18 = G.729, 13 = CN); 96â€“127 dynamic, bound by SDP (`a... |
| [ ] | $key | RTCP is limited to 5 % of the session bandwidth (senders get 25 % of that if they are â‰¤ 25 % of members), with a minimum report interval of 5 s (halved bef... |
| [ ] | $key | SDES (CNAME, NAMEâ€¦), BYE (leaving; also changes SSRC when collision), APP (application-defined â€” 3GPP MCPTT floor control uses an RTCP-based MBCP profile... |
| [ ] | $key | *Q12. Session: G.711 (IP rate 80 kbit/s). Compute the RTCP budget, and the report interval for a two-party call with 120-byte compound RTCP packets. What doe... |
| [ ] | $key | Timers over UDP: T1 = 500 ms RTT estimate, retransmit intervals double (T2 = 4 s cap), transaction timeout 64 Ã— T1 = 32 s âš  VERIFY RFC 3261 Â§17 / Table 4. |
| [ ] | $key | *Q22. (MCPTT/PTT mapping) When a user presses PTT on a gateway-connected radio, which actions go over SIP (INVITE/re-INVITE/BYE), which over RTP, and which o... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | *Q2. (a) 160 (20 ms Ã— 8 000). (b) 960 (20 ms Ã— 48 000, regardless of the actual Opus sample rate). (c) 160: G.722 samples at 16 kHz but the RTP spec regist... |
| [ ] | $key | *Q12. Session bandwidth = 80 kbit/s â†’ RTCP = 5 % = 4 kbit/s. Two members, 120-byte (960-bit) compound packets: `n Ã— size / bw = 2 Ã— 960 / 4000 = 0.48 s` ... |
| [ ] | $key | *Q16. Copy Via (all, in order), From (unchanged), To, Call-ID, CSeq. Add: To tag (`;tag=â€¦`) for a dialog-creating response, `Content-Length: 0` (or the bod... |
| [ ] | $key | *Q22. *Sure:* SIP/SDP sets up the session (INVITE + 200 + ACK, BYE to end; re-INVITE to modify); RTP carries the voice once the talker has the floor; RTCP ca... |
| [ ] | $key | Compatibility rules: never reuse a field number; add new fields freely; `int32/uint32/int64/uint64/bool` are wire-compatible with each other (âš  VERIFY); ch... |
| [ ] | $key | Every length is attacker-controlled: a `Len` field claiming 4 GiB must be rejected *before* allocating; `pointer + length` overflow must be checked against t... |
| [ ] | $key | Per-stream and per-connection flow control (credit-based; initial window 65 535 B per stream and per connection âš  VERIFY, raised with WINDOW_UPDATE / SETTI... |
| [ ] | $key | Authentication: HMAC-SHA1 over header+payload (+ROC), tag truncated to 80 bits (10 B) by default (or 32 bits, 4 B) âš  VERIFY. Check the tag before decryptin... |
| [ ] | $key | Master key/salt: 128-bit key + 112-bit salt typically âš  VERIFY; session encryption/auth/salt keys derived from them with a KDF, so one master key never enc... |
| [ ] | $key | / MIKEY (3GPP MCPTT uses MIKEY-SAKKE) âš  VERIFY / key transport with the signalling / operator-managed keys / |
| [ ] | $key | *DTLS = TLS adapted to datagrams: explicit record sequence numbers + epoch (records can be lost/reordered), retransmission timers for the handshake flight me... |
| [ ] | $key | > 6. *"Encrypting the LMR air interface secures the IP leg."* Different hops, different keys (AES-256 over-the-air on P25/DMR; SRTP/TLS on IP): the gateway i... |
| [ ] | $key | *Q21. Draw the DTLS-SRTP call setup: SIP INVITE/200 with `a=fingerprint`, DTLS handshake (ClientHello, HelloVerifyRequest + cookie, â€¦), key export, first S... |
| [ ] | $key | *Q22. (Design) The gateway receives AES-256 encrypted LMR voice from a radio network and must deliver it to an LTE MCPTT client. Describe three options (end-... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | Try each question first. âš  = verify against the cited spec. |
| [ ] | $key | *Q11. Throughput â‰¤ window/RTT = 65 535 B / 0.1 s = 655 350 B/s â‰ˆ 5.24 Mbit/s, regardless of link speed (pack 02 BDP: window must be â‰¥ bandwidth Ã— RTT;... |
| [ ] | $key | *Q19. Case 1: `s_l = 0x0010 < 0x8000`, `seq âˆ’ s_l = 0xFFE0 > 0x8000` â†’ v = ROC âˆ’ 1 = 2 â†’ index = 2Â·65 536 + 0xFFF0 = 0x2FFF0 (a *late* packet from b... |
| [ ] | $key | Alice â†’ Bob  HelloVerifyRequest (cookie)        (stateless anti-spoof) |
| [ ] | $key | Bob â†’ Alice  Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Finished |
| [ ] | $key | The certificates are self-signed; the fingerprint in SDP, carried over authenticated signalling (SIPS/TLS + identity), binds the DTLS peer â€” otherwise a ma... |
| [ ] | $key | *Q22. Options: (1) Opaque end-to-end transport â€” carry the encrypted vocoder frames as an opaque payload (a custom payload type, or `application/octet-stre... |
| [ ] | $key | (âš  = verify spec/version.) Etherstack touches the *application* above SGi/N6 (SIP/RTP/MCX), but must reason about everything underneath. |
| [ ] | $key | *Extension headers (the "E" mechanism): `length(8, in 4-byte units, including length and next-type bytes) / content / next type`. 5G uses PDU Session Contain... |
| [ ] | $key | 3. SEQUENCE preamble: one bit per `OPTIONAL`/`DEFAULT` component (1 = present) placed *first*, so a decoder knows what follows without tags. Extensible types... |
| [ ] | $key | 5. Nothing is padded in UNALIGNED PER: values are concatenated bit-after-bit; pad zeros appear only at the very end to fill the last byte. ALIGNED PER (used ... |
| [ ] | $key | Example call (SM context creation): `POST /nsmf-pdusession/v1/sm-contexts` with a JSON body containing the SUPI, DNN, S-NSSAI â€¦ returns `201 Created` + `Lo... |
| [ ] | $key | *Q21. An MCPTT group call needs a dedicated, guaranteed-bit-rate bearer. Describe the LTE path (MCPTT server â†’ Rx AAR â†’ PCRF â†’ Gx RAR â†’ PGW â†’ S5 â†... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | Attempt first. âš  = verify against the 3GPP/IETF/ITU text for your release. |
| [ ] | $key | Wider bands follow the same logic: wideband 50â€“7000 Hz â†’ 16 kHz (1 kHz transition); fullband 20 Hzâ€“20 kHz â†’ 44.1 or 48 kHz (CD at 44.1 kHz inherited ... |
| [ ] | $key | Voices vary by 30â€“40 dB between a whisper and a shout (and callers move away from microphones). The error `Î”Â²/12` is *fixed*, so a talker 40 dB below ful... |
| [ ] | $key | 8-bit WAV PCM is unsigned (silence = 0x80) whereas 16-bit is signed (silence = 0) âš  VERIFY; Î¼-law/A-law bytes are neither (Pack 02). |
| [ ] | $key | Delay: a packet cannot leave before its *last* sample was captured â†’ at least `ptime` of algorithmic delay (plus codec look-ahead, jitter buffer, network).... |
| [ ] | $key | *Q10. A remote radio site is connected over a 1 Mbit/s microwave/LTE link (budget 800 kbit/s for voice). How many G.711/20 ms calls (IP-level)? How many G.72... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | Attempt each question first. âš  = verify against the standard named. |
| [ ] | $key | *What you gain: error â‰¤ half a step: â‰¤ 4 at small levels, â‰¤ 512 at the loudest; relative error stays â‰ˆ 3â€“6 % across ~48 dB; dynamic range of â‰ˆ 13... |
| [ ] | $key | > 6. *"G.711 sounds worse than G.729 because it has more bits."* No: G.711 has *better* quality (MOS â‰ˆ 4.1 vs â‰ˆ 3.9 âš  VERIFY) at 8Ã— the bit rate and n... |
| [ ] | $key | (âš  = verify in RFC 3551 / the codec's recommendation.) |
| [ ] | $key | Licensing: G.711 and Opus free; G.729 patents expired (2017, âš  VERIFY); AMR/AMR-WB via patent pools; IMBE/AMBE+2 proprietary (DVSI hardware/software licence). |
| [ ] | $key | ## Part D â€” Radio vocoders (frame arithmetic only; VERIFY each figure) |
| [ ] | $key | *Q16. Legal/IP: which of G.711, G.729, AMR-WB, Opus, IMBE, AMBE+2, TETRA ACELP can you ship in a product without a licence fee (âš  VERIFY per codec, per jur... |
| [ ] | $key | ## âš  VERIFY list (this pack) |
| [ ] | $key | Attempt first. âš  = verify against the standard named. Numbers marked "(run)" were produced by running the solution code. |
| [ ] | $key | *Q12. IMBE: 144 bits / 0.020 s = 7 200 bit/s; voice 88 bits / 0.020 = 4 400; FEC 56 / 0.020 = 2 800; code rate = 88/144 = 0.611 (56 parity bits protect the m... |
| [ ] | $key | *Q16. Free to use (âš  VERIFY): G.711 (no royalties); Opus (royalty-free, BSD-licensed reference); G.729 (patents expired by 2017; implementations historical... |
| [ ] | $key | > Legal / safety: all audio is synthetic (sines, silence, a deterministic noise generator, simulated arrival times). No live public-safety receive. Intercept... |
| [ ] | $key | Why 1/16?* A smoother with gain `g = 1/16` averages over ~16 packets (~320 ms) and is a shift in integer arithmetic. It follows slow changes but ignores sing... |
| [ ] | $key | Extra delay = depth Ã— ptime. Depth 3 at 20 ms = 60 ms added one-way latency â€” to be compared with the end-to-end budget (ITU-T G.114: â‰¤ 150 ms one-way c... |
| [ ] | $key | A simple, defensible rule: `target = clamp( ceil( k Â· J / ptime ), min_depth, max_depth )` with `k = 4` ("cover four mean deviations"). (Ramjee et al. 1994 ... |
| [ ] | $key | *Concealment: `out[i] = gain Â· history[end âˆ’ P + (i mod P)]` with `gain = max(0, 1 âˆ’ 0.2Â·(loss_index+1))`: 0.8, 0.6, 0.4, 0.2, 0 â€” silent from the fi... |
| [ ] | $key | *Q10. Implement `estimate_pitch_period` and `conceal_pitch`. Show that with `loss_index = 0` and a 200 Hz sine the concealed 160 samples equal `0.8 Ã— the tr... |
| [ ] | $key | In a conversation each party speaks roughly 35â€“40 % of the time (Brady 1969 âš  VERIFY; push-to-talk radio is more extreme: one talker, many listeners, lon... |
| [ ] | $key | 3. Comfort noise (RFC 3389): payload type 13 (8 kHz) â€” a 1-byte noise level in âˆ’dBov (0 = loudest, 127 = quietest), optionally followed by up to 8 spectr... |
| [ ] | $key | ## VERIFY list for this pack |
| [ ] | $key | Etherstack:* IMBE/AMBE are parametric: you cannot repeat PCM-domain pitch; P25 decoders repeat the previous frame's parameters or mute based on error status ... |
| [ ] | $key | *Bandwidth check (worksheet D.2): SID packet `12 + 8 + 20 + 1 = 41 B`; every 160 ms â‡’ `41Â·8/0.16 = 2.05 kbit/s`; average `0.4Â·80 + 0.6Â·2.05 = 33.2` kbit... |
| [ ] | $key | / WebRTC equivalent? / NetEq: adaptive jitter buffer + time-scale + PLC in one (âš  VERIFY details). / |
| [ ] | $key | > Legal / safety: synthetic audio only (sines, silence). No live public-safety receive; do not use recordings of real emergency-services traffic as test mate... |
| [ ] | $key | ## Build 2 â€” Exhaustive Î¼-law verification (and verifying the verifier) |
| [ ] | $key | *Q4. Implement `mulaw_verify_exhaustive` (function pointers, so it can test *any* codec). Make `MulawExhaustive.*` pass. Report the `max_error` and explain w... |
| [ ] | $key | ## VERIFY list for this pack |
| [ ] | $key | Ethernet overhead (14 + 4 FCS) and wire overhead (preamble 8 + IFG 12) as used in drills 15 âš  VERIFY if you quote "87.2 kbit/s". |
| [ ] | $key | Everything marked "(test)" is asserted by `code/tests/test_builds.cpp` and was run against `code/solutions/builds.hpp` (22 tests, g++ `-Wall -Wextra -fsaniti... |
| [ ] | $key | Etherstack:* for a table-driven optimisation, run `mulaw_verify_exhaustive` on both (table-driven and reference) and compare `table_encode(x) == ref_encode(x... |
| [ ] | $key | "## 6. VERIFY list for this pack\n", |
| [ ] | $key | The SDR front end delivers I/Q at some sample rate. A P25-style 4FSK/C4FM demodulator is an FM discriminator on I/Q: arg(z[n+1]z*[n]) â†’ average per symbol ... |
| [ ] | $key | ## 7. VERIFY list |
| [ ] | $key | 4FSK/C4FM symbol rate and deviations quoted in Â§5 are cross-referenced to Pack 05; VERIFY against TIA-102.BAAA / ETS(I) documents before quoting in an inter... |
| [ ] | $key | "## 7. VERIFY list\n", |
| [ ] | $key | ### 2.3 Design rules (textbook; Oppenheim & Schafer Table 7.1 â€” VERIFY if quoting) |
| [ ] | $key | Channel selection: from MHz-rate I/Q down to a 12.5 kHz (or 25 kHz) channel needs a decimation chain; the channel filter's stopband must reject the adjacent ... |
| [ ] | $key | Group delay: a 101-tap FIR at 48 kS/s delays by 50 samples = 1.04 ms â€” relevant when a stack measures slot timing against a TDMA frame (e.g. DMR 30 ms slot... |
| [ ] | $key | ## 8. VERIFY list |
| [ ] | $key | 5. 50 samples at 96 kS/s = 0.52 ms. A sync correlator after the filter will peak 0.52 ms late relative to the antenna signal; slot/frame timing derived from ... |
| [ ] | $key | > Rule for this pack: a number marked VERIFY is quoted from memory of the standards and must be checked against the document before you rely on it in an inte... |
| [ ] | $key | Example:* narrowband land-mobile FM, deviation Î”f = 2.5 kHz, audio up to 3 kHz â‡’ BW â‰ˆ 2(2.5 + 3) = 11 kHz which just fits a 12.5 kHz channel. (Typical L... |
| [ ] | $key | *P25 Phase 1 C4FM (as I recall from TIA-102.BAAA; VERIFY every number): |
| [ ] | $key | *DMR 4FSK (ETSI TS 102 361-1; VERIFY): same 4800 symbols/s and 9600 bit/s, but D = 648 Hz â‡’ Â±648 Hz and Â±1944 Hz. |
| [ ] | $key | ### 3.2 Why Ï€/4-DQPSK (TETRA; VERIFY: 18 ksym/s, 36 kbit/s, 25 kHz channel, RRC Î± = 0.35) |
| [ ] | $key | Ï€/4-DQPSK uses two interleaved QPSK constellations (0, Ï€/2, Ï€, 3Ï€/2 and Ï€/4, 3Ï€/4, â€¦) and *alternates* between them: every symbol is a phase incremen... |
| [ ] | $key | TETRA dibit â†’ phase increment (VERIFY against ETSI EN 300 392-2): 00 â†’ +Ï€/4; 01 â†’ +3Ï€/4; 11 â†’ âˆ’3Ï€/4; 10 â†’ âˆ’Ï€/4. |
| [ ] | $key | 7. 16-QAM Gray mapping: write all 16 points' normalised coordinates for I-bits 00,01,11,10; verify average energy 1. |
| [ ] | $key | ## 9. VERIFY list (check against the standards) |
| [ ] | $key | Hand-computed; nothing executed. Standard-derived numbers are marked VERIFY (see the worksheet's VERIFY table). |
| [ ] | $key | *A2. (P25 convention, VERIFY) 01 â†’ +1800 Hz, 00 â†’ +600 Hz, 10 â†’ âˆ’600 Hz, 11 â†’ âˆ’1800 Hz. Each held for 208.3 Âµs: staircase +1800, +600, âˆ’600, â... |
| [ ] | $key | 1. 2 bits/symbol, 4800 sym/s â‡’ 9600 bit/s in a 12.5 kHz channel; deviations Â±600 Hz (inner) and Â±1800 Hz (outer) for the 4 levels (VERIFY TIA-102.BAAA). |
| [ ] | $key | Hand-computed with a calculator; nothing executed. Roll-off values of real standards are VERIFY items. |
| [ ] | $key | "/ Etherstack relevance / P25 / DMR / TETRA transmitters shape their symbols with (root-)raised-cosine-type filters so the occupied spectrum fits a 12.5 kHz ... |
| [ ] | $key | "Smaller $\\beta$ â‡’ narrower spectrum but longer, slower-decaying tails (higher sensitivity to timing error). Compare $\\beta = 0.2, 0.35, 0.5$ (P25 / DMR ... |
| [ ] | $key | "* Transmit mask: the RRC roll-off and span choose how much energy leaks into the adjacent 12.5 kHz channel. With $R_s$ = 4800 sym/s, $\\beta$ = 0.2 gives a ... |
| [ ] | $key | "3. Verify the RC time-domain formula at $t = T$ for $\\beta=0.35$ gives 0, and compute $h_{RC}(0.5T)$ (use the formula). Then compute the limit value at $t ... |
| [ ] | $key | "## 7. VERIFY list\n", |
| [ ] | $key | "* The tail decay claims ($1/t$ for sinc, $1/t^3$ for RC with $\\beta\u003e0$) are textbook; verify numerically if required.\n", |
| [ ] | $key | 48-bit frame sync in P25/DMR-style framing â€” correlate on symbols (4 levels) or dibits; compute your own false-alarm and miss rate for the threshold chosen... |
| [ ] | $key | ## 9. VERIFY list |
| [ ] | $key | P25 frame-sync word 0x5575F5FF77FF and DMR sync 0x755FD7DF75F7 are quoted from memory â€” VERIFY (TIA-102.BAAA; ETSI TS 102 361-1). Not used by the code or t... |
| [ ] | $key | Hand-computed; nothing executed. VERIFY marks values that must be checked against a standard. |
| [ ] | $key | Hand-computed; nothing executed. VERIFY marks items to check against standards/datasheets. |
| [ ] | $key | "*Worked example (P25-like numbers; VERIFY the standard):* $R_b$ = 9600 bit/s, noise bandwidth $B$ = 12.5 kHz: $10\\log_{10}(12500/9600)$ = +1.15 dB, so $E_b... |
| [ ] | $key | "* Delay spread $\\sigma_\\tau$ (rms): the time-spread of the channel. Coherence bandwidth $B_c\\approx 1/(5\\sigma_\\tau)$ (rule of thumb, 0.5 correlation; ... |
| [ ] | $key | "* *Radio context:* a 12.5 kHz channel needs $B_c \\gg 12.5$ kHz â‡’ $\\sigma_\\tau \\ll 16\\,\\mu$s. Typical urban rms delay spreads are of order a microsec... |
| [ ] | $key | "Its autocorrelation is $R(\\tau)=J_0(2\\pi f_d\\tau)$ (a Bessel function; first zero at $f_d\\tau = 0.383$). Coherence time $T_c\\approx 0.423/f_d$ (Rappapo... |
| [ ] | $key | "* Sensitivity maths: noise floor $-174+10\\log_{10}B$ dBm; + noise figure + required SNR â†’ sensitivity. For 12.5 kHz: $-133.0$ dBm floor; NF 6 dB â‡’ âˆ’1... |
| [ ] | $key | "5. How would you verify a BER simulator?\n", |
| [ ] | $key | "## 10. VERIFY list\n", |
| [ ] | $key | "* P25 9600 bit/s in 12.5 kHz is quoted from Pack 05 (VERIFY there).\n", |
| [ ] | $key | > Honesty note. All numbers below were derived by hand. Device figures (NF, IIP3, ENOB, phase-noise masks, standard test conditions) are *illustrative*; ever... |
| [ ] | $key | Illustrative:* 12-bit ADC at fs = 61.44 MS/s (VERIFY your actual part). A full-scale sine has SQNR = 6.02Â·12 + 1.76 = 74.0 dB over fs/2 = 30.72 MHz. The wan... |
| [ ] | $key | Phase accumulator of W = 32 bits: Ï†[n+1] = Ï†[n] + M (mod 2Â³Â²), frequency f = MÂ·fs/2Â³Â². Resolution fs/2Â³Â² = 61.44Ã—10â¶/4.295Ã—10â¹ = 0.0143 Hz. To... |
| [ ] | $key | An analogue front-end typically achieves 25â€“40 dB (VERIFY per device). A strong adjacent-channel signal 60â€“70 dB above the wanted one then leaves its ima... |
| [ ] | $key | Example: adjacent-channel blocker âˆ’48 dBm (â‰ˆ 70 dB above a âˆ’118 dBm sensitivity; VERIFY the ACS test condition in TIA-102.CAAA / ETSI TS 102 361-2), ch... |
| [ ] | $key | Truncation (no +2Â¹â´) biases every result by âˆ’Â½ LSB on average â‡’ DC offset in a recursive filter. Rounding here is "ties toward +âˆž"; convergent (rou... |
| [ ] | $key | Stated requirements (sensitivity, adjacent-channel selectivity, frequency stability) come from the conformance documents (TIA-102.CAAA for P25, ETSI TS 102 3... |
| [ ] | $key | 6. How do you verify that your fixed-point C++ matches the reference model? (Pack 10.) |
| [ ] | $key | ## 11. VERIFY list |
| [ ] | $key | Hand-computed; nothing executed. VERIFY marks items to check against datasheets/standards. |
| [ ] | $key | Spec (Pack 05; VERIFY all P25 C4FM numbers against TIA-102.BAAA): 4800 sym/s, 2 bits/symbol = 9600 bit/s; dibit 01 â†’ +3, 00 â†’ +1, 10 â†’ âˆ’1, 11 â†’ âˆ’... |
| [ ] | $key | Word: P25 frame sync 0x5575F5FF77FF (VERIFY against TIA-102.BAAA). The generator embeds it with 2 bit errors at lag 100. |
| [ ] | $key | Conformance numbers (sensitivity, 5 % BER vs 1 % BER definitions, sync words) must come from the standards: VERIFY. |
| [ ] | $key | 1. How do you verify a C++ DSP block against a Matlab/Octave model? |
| [ ] | $key | ## 8. VERIFY list |
| [ ] | $key | Hand-derived; nothing compiled or run. VERIFY marks items to check against standards. |
| [ ] | $key | > Act 2016 â€” VERIFY for your jurisdiction). GSM/UMTS here is arithmetic and a code-tree exercise, not a sniffer. |
| [ ] | $key | Standards (numbers from memory â€” VERIFY; I never invent clause numbers): 3GPP TS 45.001/45.002 (GSM physical layer, |
| [ ] | $key | Different phones in different slots *and* carriers = the TDMA/FDMA hybrid. (E-GSM / DCS 1800 plans differ â€” VERIFY.) |
| [ ] | $key | (the famous ~35 km GSM range limit; extended-range cells use a trick â€” VERIFY). |
| [ ] | $key | (`gsm_tch_coded_rate_bps`; the split of class I/II bits is VERIFY). The 456 bits are what one user's 20 ms of speech occupies on the air (spread over several... |
| [ ] | $key | uses frequency deviation `Â±Rb/4` (`msk_deviation_hz`: 67.7 kHz at 270.833 kbit/s); the Gaussian pre-filter (GSM uses BT = 0.3, VERIFY) smooths the |
| [ ] | $key | BTS = *the radio*, BSC = *the radio manager*, MSC = *the switch*. (Packet data on 2G: GPRS adds SGSN/GGSN â€” VERIFY details; out of scope here.) |
| [ ] | $key | orthogonality is lost. UMTS therefore scrambles each link with a long pseudo-random sequence; users become separable *statistically* (low cross-correlation),... |
| [ ] | $key | > (the nearâ€“far problem: a close phone swamps a far one unless it lowers its power ~1500 times per second â€” VERIFY) is mandatory. |
| [ ] | $key | *(a) Slot budget. A normal burst carries 148 bits in 0.5769 ms. A user gets one burst per 4.615 ms frame, so the raw rate is `148 bits / 4.615 ms = 32.1 kbit... |
| [ ] | $key | *VERIFY list (look these up before quoting in an interview): GMSK BT = 0.3; 260â†’456 bit class split; P-GSM vs E-GSM ARFCN ranges; extended-range TA trick; ... |
| [ ] | $key | Synthetic data only; never live traffic. VERIFY = check 3GPP before quoting. |
| [ ] | $key | *Reasoning. Speech needs â‰ˆ13 kbit/s, the carrier moves 270 kbit/s. Time-slicing converts spare *speed* into *users*; the phone transmits 1/8 of the time (b... |
| [ ] | $key | *Reasoning. Duplex spacing 45 MHz keeps TX and RX apart for a duplex filter. E-GSM extends the range (VERIFY). |
| [ ] | $key | *Reasoning. Without TA a phone at distance d arrives `2d/c` late; if that exceeds the guard (~30 Âµs â‰ˆ 8.25 bits, VERIFY) it corrupts the next slot. The BT... |
| [ ] | $key | *Reasoning. Radio channels are error-prone; the receiver needs redundancy. Unequal protection: the most important speech bits (class 1a) get CRC + convolutio... |
| [ ] | $key | *Reasoning. D is the distance between co-channel cells. Larger N pushes interferers farther away (better C/I) but divides the channels among more cells (lowe... |
| [ ] | $key | Etherstack angle:* gateway vendors terminate interfaces between these boxes (A-bis, A â€” VERIFY). |
| [ ] | $key | *Answer. Carry over: slot grid derived from a symbol clock, bursts with training/sync, guard time, a base-station-controlled timing offset for outbound/inbou... |
| [ ] | $key | > Investigatory Powers Act 2016 â€” VERIFY for yours). Studying srsRAN's architecture is fine; do not copy its code (pack 04). |
| [ ] | $key | Standards (from memory â€” VERIFY; I do not invent clause numbers): 3GPP TS 36.211 (physical channels and modulation), |
| [ ] | $key | Slot = `160 + 6Â·144 + 7Â·2048 = 15 360 samples = 0.5 ms` (`slot_samples`). *Cost:* CP is â‰ˆ 7 % overhead â€” time the receiver cannot use for data. Extende... |
| [ ] | $key | signals, control channels (PDCCH), coding rate, overhead (â‰ˆ 25â€“30 %), and Cat-limits (VERIFY). Never quote this figure as "LTE speed". |
| [ ] | $key | / PAPR / high (â‰ˆ 8â€“12 dB) / lower (â‰ˆ 4â€“7 dB for QPSK/16QAM; VERIFY) / |
| [ ] | $key | *QCI = QoS Class Identifier: a label for a bearer's treatment. Packet delay budgets (`qci_delay_budget_ms`, TS 23.203 â€” VERIFY): QCI 1 (conversational voic... |
| [ ] | $key | *ROHC (in PDCP) compresses RTP/UDP/IP down to ~3 bytes on the air: `34 Ã— 8 / 20 = 13.6 kbit/s` (`ip_voice_bitrate_bps`). Also: SPS (semi-persistent scheduli... |
| [ ] | $key | 13. Q13 (Explain) â€” What is a QCI and why does MCPTT voice (QCI 65) have a tighter budget than default data? (W 3.5; VERIFY table) |
| [ ] | $key | [ ] Draw eNB/MME/HSS/S-GW/P-GW with S1-MME, S1-U, S5/S8, SGi labelled (VERIFY interface names). |
| [ ] | $key | *VERIFY list: channel-bandwidth/RB table (TS 36.101); CP lengths at each FFT; typical OFDM/SC-FDMA PAPR figures; QCI 1/5/9/65/69 delay budgets (TS 23.203); G... |
| [ ] | $key | Synthetic data only. VERIFY = check 3GPP before quoting. |
| [ ] | $key | *Answer. MME: NAS signalling, attach, authentication orchestration, paging, bearer setup â€” control plane only. HSS: subscriber data/keys. S-GW: user-plane ... |
| [ ] | $key | *Answer. A QCI selects a standardised treatment (resource type GBR/non-GBR, priority, packet delay budget, loss rate). MCPTT voice (QCI 65, 75 ms) and signal... |
| [ ] | $key | > spectrum. Test with your own isolated 5G test network (test SIM/PLMN, shielded) only. UK: Wireless Telegraphy Act 2006 â€” VERIFY for your jurisdiction. |
| [ ] | $key | Standards (from memory â€” VERIFY; I never invent clause numbers): 3GPP TS 38.211 (physical channels, numerology), 38.212 (multiplexing and channel coding), ... |
| [ ] | $key | *TR 38.811 / 38.821 (non-terrestrial networks). Timeline source: 3GPP Release 20/21 work plan (VERIFY â€” dates below are from the brief and may move). |
| [ ] | $key | FR1 (sub-7 GHz, nominally 410 MHzâ€“7.125 GHz â€” VERIFY): data SCS 15/30/60 kHz. FR2 (24.25â€“52.6 GHz, "mmWave" â€” VERIFY): 60/120 kHz (`numerology_allowe... |
| [ ] | $key | A PRB = 12 subcarriers. `prb_count(bw, Î¼)` table (TS 38.101 â€” VERIFY): 20 MHz @15 kHz â†’ 106; 20 MHz @30 kHz â†’ 51; 100 MHz @30 kHz â†’ 273; 100 MHz @12... |
| [ ] | $key | *RAN: gNB split. The gNB is decomposed into CU (Central Unit: RRC + PDCP, possibly split CU-CP / CU-UP), DU (Distributed Unit: RLC + MAC + high-PHY), and RU ... |
| [ ] | $key | User plane stays on GTP-U over N3 (gNBâ€“UPF); N2 = NGAP gNBâ€“AMF; N4 = PFCP SMFâ€“UPF (VERIFY). |
| [ ] | $key | *Network slicing: a slice = a logical end-to-end network (specific NFs + resources) selected by S-NSSAI = SST + SD (SST 1 eMBB, 2 URLLC, 3 MIoT â€” VERIFY). ... |
| [ ] | $key | *Two base graphs. BG1 (large, up to 8448-bit blocks, rate â‰¥ 1/3) and BG2 (smaller, 3840 bits, rate â‰¥ 1/5): selection rule (`ldpc_base_graph`, TS 38.212 Â... |
| [ ] | $key | Etherstack angle:* public-safety coverage extension (remote areas, disaster relief) is the NTN story; hand-held *direct-to-device* is limited by exactly thes... |
| [ ] | $key | Brief's timeline â€” VERIFY against the current 3GPP work plan before quoting: Rel-21 timeline freeze â‰ˆ March 2027, specifications complete â‰ˆ March 2029,... |
| [ ] | $key | Themes: AI-native air interface/network (AI/ML for PHY, scheduling, CSI), integrated sensing and communication (ISAC) â€” the network as radar, non-terrestri... |
| [ ] | $key | > Misconception box â€” "each G is a clean break". 5G NSA ran on an LTE core and anchor; 6G will coexist with 5G for decades; releases evolve the "same G" (5... |
| [ ] | $key | [ ] Say the 6G timeline with a "verify" caveat and say "no 7G". |
| [ ] | $key | *VERIFY list: 6G dates (Rel-21 freeze Mar 2027, specs Mar 2029, commercial ~2030) against the 3GPP work plan; FR1/FR2 boundaries; PRB table (TS 38.101-1/-2);... |
| [ ] | $key | Synthetic data only. VERIFY = check 3GPP before quoting. |
| [ ] | $key | / "Î¼=4 is used for data in FR2" / Table has it. / 240 kHz is for SSB only (VERIFY). / |
| [ ] | $key | *Answer. CU: RRC + PDCP (non-real-time, per-UE; may split CU-CP/CU-UP). DU: RLC, MAC, high PHY (hard real-time scheduling). RU: low PHY + RF/antenna. F1 = CU... |
| [ ] | $key | *Answer (one design). Dedicated S-NSSAI (e.g. SST custom/URLLC-like, VERIFY); subscribers via UDM; dedicated SMF/UPF at the edge (local breakout to the MCPTT... |
| [ ] | $key | *Answer. `20e6 Ã— log2(1+100) = 133.2 Mbit/s`; `100e6 Ã— log2(1+10) = 345.9 Mbit/s`. Capacity is linear in bandwidth, logarithmic in SNR: 5Ã— more bandwidth ... |
| [ ] | $key | *Answer. LEO 550 km: 1.83 ms one way, FSPL 153.3 dB (2 GHz), Doppler up to â‰ˆ 50 kHz. GEO: 119.4 ms one way, 189.5 dB; Doppler â‰ˆ 0 (nearly static). NR nee... |
| [ ] | $key | *Answer (6G). "6G (IMT-2030) is in 3GPP study in Rel-20, with normative work in Rel-21; the plan I've seen is timeline freeze around March 2027, specs about ... |
| [ ] | $key | > Parse only bytes you build in tests or capture on your own isolated test network. UK: Wireless Telegraphy Act 2006 / Investigatory Powers Act 2016 â€” VERI... |
| [ ] | $key | > Reading srsRAN (Â§3.6) is allowed; its licence (AGPL-3.0 â€” VERIFY) restricts copying code â€” do not copy source into your repo; read it, close it, write... |
| [ ] | $key | Standards (from memory â€” VERIFY; I never invent clause numbers): 3GPP TS 22.179 (MCPTT requirements), TS 23.179 (MCPTT architecture), TS 23.280 (common MC ... |
| [ ] | $key | *TS 23.283 (MC interworking with LMR â€” Rel-15, VERIFY number and scope), TS 29.281 (GTP-U). |
| [ ] | $key | Then, if the next-ext type â‰  0, a chain of extension headers: each is `[length in 4-byte units][contentâ€¦][next type]`; a next type of 0 ends the chain. 5... |
| [ ] | $key | `numerology_row(Î¼, bw_MHz)` returns `{Î¼, SCS, slots/subframe, slot ms, useful symbol Âµs, PRBs, occupied MHz}`; PRBs come from the given `nr_prb_count` tab... |
| [ ] | $key | *Releases (VERIFY): Rel-13 â€” MCPTT (group/private calls, emergency, floor control, off-network ProSe); Rel-14 â€” MCData (SDS, file transfer, IP data) and ... |
| [ ] | $key | Bearers: MCPTT voice rides a dedicated bearer, QCI 65 (75 ms budget), signalling QCI 69 (60 ms) â€” see pack 02 Q13 (VERIFY table). eMBMS/MBMS can broadcast ... |
| [ ] | $key | Latency targets (TS 22.179 defines an "MCPTT access time" and an end-to-end access time â€” on the order of a few hundred ms; VERIFY the actual values) shape... |
| [ ] | $key | *Identity: model URI `sip:tg<ID>@<domain>` (`make_group_uri(1234, "mcptt.example.org") = sip:tg1234@mcptt.example.org`; real MC group identities are specifie... |
| [ ] | $key | 1. Obtain the upstream source yourself (`github.com/srsran/srsRAN_Project` or `github.com/srsran/srsRAN_4G` â€” VERIFY current layout and licence). Do not pa... |
| [ ] | $key | 5. Re-implement a *different* header (e.g. the PDCP data PDU header for 12-bit SN, TS 36.323/38.323 â€” VERIFY layout) from the spec, test-first, in `scratch... |
| [ ] | $key | 2. Q2 (Explain) â€” MCPTT (Rel-13), MCData/MCVideo (Rel-14), LMR interworking (Rel-15): what did each release add? What would you VERIFY? (W 3.4) |
| [ ] | $key | *VERIFY list: TS 23.283 number, scope and release (Rel-15); MCData/MCVideo spec numbers (TS 23.282/23.281); MBCP in TS 24.380; QCI 65/69 values; GTP-U ext ty... |
| [ ] | $key | Synthetic data only. VERIFY = check 3GPP before quoting. |
| [ ] | $key | *Answer. MCX client (UE) â€”LTE/5G radioâ†’ eNB/gNB â†’ EPC/5GC (P-GW/UPF) â†’ IMS core (SIP: TS 24.379) â†’ MC server(s) (MCPTT AS: call control, floor cont... |
| [ ] | $key | *Answer. Rel-13: MCPTT (group/private calls, emergency, floor control, off-network). Rel-14: MCData (short data, file, IP data) and MCVideo. Rel-15: interwor... |
| [ ] | $key | *Reasoning. version = bits 7â€“5, PT = bit 4, bit 3 spare, E bit 2, S bit 1, PN bit 0 (VERIFY TS 29.281). |
| [ ] | $key | *Answer. Floor, identity, voice, security. Hardest in practice: security (key management across two worlds; transcoding forces decrypt/re-encrypt at the gate... |
| [ ] | $key | A good table answers: bit order (MSB-first? packed with shifts or bitfield struct?), endianness handling, length validation, error propagation (bool/`expecte... |
| [ ] | $key | *Answer. Behind the P-GW/UPF (SGi/N6) on the IP side and as an IMS/MC peer; on the LMR side via ISSI/CSSI-style or radio-site interfaces (VERIFY per technolo... |
| [ ] | $key | are from memory â€” mark VERIFY wherever you see it and check the current TIA catalogue before quoting |
| [ ] | $key | in an interview. I deliberately do not invent section numbers: "Â§ VERIFY" means "go and look". |
| [ ] | $key | radio protocols (P25, DMR, TETRA, dPMR, NXDN, analogue/MPT1327 families â€” VERIFY the current product |
| [ ] | $key | / dibit / symbol / deviation (VERIFY) / |
| [ ] | $key | fixed 48-bit frame sync = 24 symbols: `0x5575F5FF77FF` (VERIFY [TIA-102.BAAA]). |
| [ ] | $key | Special values: `0xF7E` = "receive any NAC" and `0xF7F` = "any NAC / repeater-ing" (VERIFY [TIA-102.BAAC]). |
| [ ] | $key | DUID â€” Data Unit ID, 4 bits (VERIFY values [TIA-102.BAAA]): |
| [ ] | $key | way. (VERIFY: that the standard picks the same field polynomial xâ¶+x+1 and hence the identical g(x).) |
| [ ] | $key | which lets the decoder correct 11 and detect 12. (VERIFY the parity convention [TIA-102.BAAA].) |
| [ ] | $key | (VERIFY the exact counting origin). On a conventional channel these carry busy/idle information so |
| [ ] | $key | Total on air â‰ˆ 792 bits â‰ˆ 82.5 ms (VERIFY the exact accounting of trailing bits). |
| [ ] | $key | 240. (VERIFY codes [TIA-102.BAAA].) Reed-Solomon works on *symbols* (here 6-bit), so a burst that wipes out a |
| [ ] | $key | *LDU1 â€” Link Control (LC), 72 bits. Fields (group voice channel user format â€” VERIFY |
| [ ] | $key | *ALGID (8 bits: `0x80` = unencrypted; `0x81` DES-OFB, `0x84` AES-256, `0xAA` ADP/RC4 â€” VERIFY), KID |
| [ ] | $key | > worst-case join time â‰ˆ 540 ms (Q7). MI also *advances* during the call (an LFSR step per LDU2 â€” VERIFY), |
| [ ] | $key | (VERIFY names/opcodes [TIA-102.AABC]): unit registration, group affiliation, group voice channel grant, |
| [ ] | $key | / ISSI â€” Inter-RF-Subsystem Interface / RF subsystem â†” RF subsystem (system-to-system roaming, group calls across sites/systems) / SIP (call control, wit... |
| [ ] | $key | / CSSI â€” Console Subsystem Interface / dispatch console â†” RF subsystem / SIP + RTP (ISSI-derived) / TIA-102.BACE? VERIFY / |
| [ ] | $key | / DFSI â€” Digital Fixed Station Interface / fixed station (repeater) â†” controller / serial / IP, carries voice and control (VERIFY) / TIA-102.BAHA / |
| [ ] | $key | [ ] I can state three things in this pack I would VERIFY before quoting. |
| [ ] | $key | ### VERIFY list for this pack |
| [ ] | $key | Anything marked VERIFY must be checked against the TIA-102 documents before you quote it. |
| [ ] | $key | word gets Hamming(10,6) â†’ 240 bits. For ES: 96 info = 16 hex â†’ RS(24,16) = 144 â†’ 240 (VERIFY codes). |
| [ ] | $key | / "The 64th bit is protected by the BCH." / It's in the NID. / It's outside the 63-bit code; it's the overall-parity bit (VERIFY convention). / |
| [ ] | $key | *VERIFY), and DUID 0x7 = TSBK. |
| [ ] | $key | AMBE+2 vocoder (proprietary, DVSI; referenced by TIA-102.BABG? â€” VERIFY). Document numbers are from |
| [ ] | $key | memory: VERIFY before quoting. I do not invent section numbers; "Â§ VERIFY" = look it up. |
| [ ] | $key | Etherstack's core business is LMR protocol stacks (P25, DMR, TETRA, dPMRâ€¦ VERIFY current list). P25 |
| [ ] | $key | Imagine two radios, A and B, one frequency. The time axis is chopped into 30 ms slots (VERIFY): |
| [ ] | $key | *12 slots = 360 ms = a superframe (VERIFY). Logical channel 0 (even slots) and 1 (odd slots) each own 6 |
| [ ] | $key | AMBE+2 is a *half-rate* codec at similar perceived quality (VERIFY subjective claims) â€” *and* because |
| [ ] | $key | absolute phase. Teaching table (exact Phase 2 table: VERIFY TIA-102.BBAB): |
| [ ] | $key | The "H" stands for *harmonised* (VERIFY the exact meaning), i.e. chosen to coexist with Phase 1 receivers/ |
| [ ] | $key | (4 Ã— 72 = 288 bits of the 360 bits; the other 72 bits are sync/ID/signalling overhead â€” VERIFY the |
| [ ] | $key | Teaching layout (real order VERIFY TIA-102.BBAC), per channel: `4V 4V 4V 4V 2V SACCH` â‡’ 4+4+4+4+2 = 18 voice |
| [ ] | $key | Logical channels (VERIFY names): VCH voice; SACCH slow associated control (always there, low rate); |
| [ ] | $key | radio that joins mid-call can pick it up within a superframe (VERIFY ESS-A/ESS-B split) â€” the same *repeat for |
| [ ] | $key | A system can use a Phase 1 (FDMA) control channel and mixed FDMA/TDMA voice channels (VERIFY dynamic |
| [ ] | $key | *transcoder at gateways (pack 05, Q-IWF). IMBE and AMBE+2 are licensed proprietary vocoders (VERIFY |
| [ ] | $key | ### VERIFY list for this pack |
| [ ] | $key | Synthetic data only. VERIFY = check against TIA-102.BBAB / BBAC before quoting. |
| [ ] | $key | / "AMBE+2 is just better quality." / Marketing. / It's *lower rate*; quality is comparable (VERIFY), not better. / |
| [ ] | $key | / "H = high speed." / Letter H. / "Harmonised" (VERIFY). / |
| [ ] | $key | / 4V overhead / 360 âˆ’ 4 Ã— 72 / 72 bits (VERIFY meaning) / |
| [ ] | $key | (Pattern is a teaching layout â€” VERIFY real order.) |
| [ ] | $key | loss; VERIFY alternatives such as parametric transcoding). |
| [ ] | $key | one agreed encoding per call (negotiated in SIP/SDP â€” VERIFY), so a mixed call forces a transcoder at the border. |
| [ ] | $key | tagged VERIFY (and I never invent clause numbers â€” "clause VERIFY" means "go and look"). |
| [ ] | $key | stacks (P25, DMR, TETRA, dPMRâ€¦ VERIFY the current product list): a DMR stack must parse the 264-bit burst |
| [ ] | $key | Dibitâ†’symbol mapping (VERIFY, TS 102 361-1): `01 â†’ +3`, `00 â†’ +1`, `10 â†’ âˆ’1`, `11 â†’ âˆ’3` (a Gray map again: neighbours |
| [ ] | $key | announcement channel: access-type/busy-idle info and short link control â€” VERIFY), on an uplink they act as a |
| [ ] | $key | *Sync words (48 bits each; VERIFY every digit [TS 102 361-1]): |
| [ ] | $key | AMBE+2 frames are 72 bits per 20 ms (3600 bit/s: 49 speech + 23 FEC bits; VERIFY split). With 2-slot TDMA, a |
| [ ] | $key | A carries the sync, and F carries other embedded signalling (null/reverse channel; VERIFY). 1 + 4 + 1 = 6. |
| [ ] | $key | info bits). (VERIFY.) |
| [ ] | $key | Data types (VERIFY): 0 PI header, 1 voice LC header, 2 terminator with LC, 3 CSBK, 4 MBC header, 5 MBC |
| [ ] | $key | matrix is VERIFY TS 102 361-1 Annex B): `word20 = (word19 << 1) / parity`. The result has minimum distance 8: |
| [ ] | $key | Full LC (72 bits) + 24 bits of Reed-Solomon(12,9) parity = 96 bits; CSBK = 80 bits + 16-bit CRC = 96 bits (VERIFY). |
| [ ] | $key | two d = 3 codes has d = 3 Ã— 3 = 9 (VERIFY decoder capability), and decoding is just "run the small Hamming decoders |
| [ ] | $key | *Interleaving. Matrix index *a* is transmitted at position (a Ã— 181) mod 196 (VERIFY direction [Annex B]). 181 is prime and |
| [ ] | $key | LC (72 bits, VERIFY layout): PF(1) R(1) FLCO(6) FID(8) + 56 bits: service options(8) + destination ID(24) + source ID(24). |
| [ ] | $key | FLCO values (VERIFY): 0 = group voice channel user (destination is a talkgroup), 3 = unit-to-unit voice channel |
| [ ] | $key | ### VERIFY list for this pack |
| [ ] | $key | Synthetic data only. VERIFY = check ETSI TS 102 361-1/-2/-3/-4 before quoting. |
| [ ] | $key | less on overhead but has a lower gross rate; both carry two calls per 12.5 kHz. (VERIFY: AMBE+2 frame composition.) |
| [ ] | $key | implementation parameter; VERIFY standard guidance). The cost: one missed re-sync is harmless; many misses â‡’ drift. |
| [ ] | $key | / "Store the Golay table in a file." / Speed. / Build from the polynomial once (static init), verify perfectness in tests. / |
| [ ] | $key | / Vocoder / DMR AMBE+2 vs P25 Phase 1 IMBE â‡’ transcode (lossy tandem). To P25 Phase 2 (AMBE+2) a re-framing might suffice (VERIFY layout equality). / |
| [ ] | $key | / Emergency/priority / Map service-options bits; semantics differ (VERIFY). / |
| [ ] | $key | / Encryption / DMR privacy (basic/enhanced, mostly vendor-specific â€” VERIFY) â†” P25 AES-256/DES-OFB: no mapping; E2E-encrypted calls don't bridge without ... |
| [ ] | $key | ignores foreign bursts (like P25's 12-bit NAC; like TETRA's colour code â€” VERIFY details of TETRA CC in pack 04). Two |
| [ ] | $key | Standards (numbers from memory â€” VERIFY; I never invent clause numbers, "clause VERIFY" = look it up): |
| [ ] | $key | Etherstack's core business is LMR protocol stacks (P25, DMR, TETRA, dPMR, analogue/MPT-familyâ€¦ VERIFY the current |
| [ ] | $key | by slicing time; dPMR by slicing frequency. (VERIFY: exact regulatory targets by region.) |
| [ ] | $key | Numbering is 1-based (timeslot 1â€“4, frame 1â€“18, multiframe 1â€“60 â€” VERIFY, EN 300 392-2 clause on time structure). |
| [ ] | $key | (VERIFY detail of how mobiles use it.) |
| [ ] | $key | training sequence, tail, phase adjustment and broadcast/signalling bits (VERIFY breakdown). |
| [ ] | $key | roll-off 0.35 occupies â‰ˆ 18 000 Ã— 1.35 = 24.3 kHz (VERIFY roll-off), nearly filling the 25 kHz carrier, whereas P25/DMR |
| [ ] | $key | (VERIFY bit-pair order). Differential detection makes the receiver robust to unknown carrier phase; the Ï€/4 family never |
| [ ] | $key | ### 3.5 Logical structure (names to know â€” VERIFY each against EN 300 392-2) |
| [ ] | $key | CC 6 bits = 30 bits â€” VERIFY) initialises the scrambler; a receiver with the wrong code descrambles garbage and every CRC |
| [ ] | $key | / Layer / Question it answers / Example PDUs (VERIFY) / |
| [ ] | $key | *Identities: ITSI = MCC (10 bits) + MNC (14 bits) + SSI (24 bits) = 48 bits (VERIFY); within a network only the 24-bit SSI is |
| [ ] | $key | *Security note (VERIFY): TETRA air-interface encryption uses TEA1â€“TEA4; in 2023 independent researchers published weaknesses in |
| [ ] | $key | frequency-division twin of a single DMR slot. Vocoder is AMBE+2 class (â‰ˆ 3600 bit/s on air, 75 % of 4800 â€” VERIFY). |
| [ ] | $key | Modes (VERIFY): Mode 1 peer-to-peer, Mode 2 repeater/conventional, Mode 3 trunked/multi-site. Standards TS 102 490 / TS 102 658. |
| [ ] | $key | `1800 Hz` tones (VERIFY mapping of tone to bit value). |
| [ ] | $key | g(x) = xÂ¹âµ + xÂ¹â´ + xÂ¹Â³ + xÂ¹Â¹ + xâ´ + xÂ² + 1 = `0xE815` (VERIFY; the real standard also complements the check bits â€” this pack doesn't). |
| [ ] | $key | traffic channel (VERIFY). Because SYNCT = ~SYNC the two can't be mistaken for each other (16-bit distance). |
| [ ] | $key | Addressing: a 20-bit identity = 7-bit prefix (fleet) + 13-bit ident (VERIFY). |
| [ ] | $key | Etherstack context:* debugging frame-number-dependent features (e.g. encryption sync uses the hyperframe/multiframe/frame numbers â€” VERIFY). |
| [ ] | $key | ### VERIFY list for this pack |
| [ ] | $key | Synthetic data only. VERIFY = check ETSI EN 300 392 / TS 102 490 / TS 102 658 / MPT 1327 before quoting. |
| [ ] | $key | / normal-burst overhead / 510 âˆ’ 432 / 78 bits (VERIFY breakdown) / |
| [ ] | $key | Why is TETRA's gross efficiency higher? 18 ksym/s with roll-off 0.35 occupies â‰ˆ 24.3 kHz (VERIFY), nearly filling 25 kHz; P25/DMR use 4800â€“6000 sym/s wit... |
| [ ] | $key | dPMR radios ship with â‰¤ 1 ppm TCXOs (VERIFY spec limits). |
| [ ] | $key | Standards (numbers from memory â€” VERIFY; "clause VERIFY" = look it up, I never invent clause numbers): |
| [ ] | $key | LMR â€” Rel-15, VERIFY number/scope), TS 24.379 (MCPTT call control), TS 24.380 (MCPTT floor control), |
| [ ] | $key | and small, testable models of the pieces. (Etherstack's gateway/IWF product line â€” VERIFY current product list.) |
| [ ] | $key | > failsoft/site-trunking mode for that; VERIFY terms). |
| [ ] | $key | carrier (pack 04 Â§3.5). Message names (P25: `GRP_V_CH_GRANT`, `GRP_AFF_REQ`, `U_REG_REQ`, `QUE_RSP`, `DENY_RSP` â€¦) VERIFY |
| [ ] | $key | lowest group id (a *deterministic* tie-break chosen for testability; real systems use configured priority levels â€” VERIFY), |
| [ ] | $key | interference*). Design rule (VERIFY, rule of thumb): keep Î”t below â‰ˆ Â¼ symbol â‡’ 52.08 Âµs â‡’ Î”path â‰¤ 15.6 km at 4800 sym/s. Faster symbols shrink t... |
| [ ] | $key | simulcast-friendly modulation (CQPSK/LSM, pack 01 â€” VERIFY) for the downlink. |
| [ ] | $key | *keystream: `K_i = E_key(K_{iâˆ’1})`, starting from an initialisation vector (in P25 the message indicator, MI â€” 72 bits, VERIFY), |
| [ ] | $key | / Algorithm (P25 ALGID â€” VERIFY values) / Key size / Status / |
| [ ] | $key | eavesdropper learns the XOR of two plaintexts (the two-time pad). So the MI advances each frame (an LFSR in P25 â€” VERIFY) and a new call uses a fresh MI. |
| [ ] | $key | air; the radio decrypts and installs them (TIA-102.AACA family â€” VERIFY). Costs: the KEK is now the crown jewel; OTAR messages need |
| [ ] | $key | voice in RTP with AMR-WB or EVS (TS 26.179 â€” VERIFY), E2E-capable security (TS 33.180). An IWF (interworking function) sits between the MC system and an LMR |
| [ ] | $key | system (the Rel-15 "MC interworking with LMR" work, TS 23.283 â€” VERIFY exact title and scope) so that a handheld on LTE can talk to a P25 talkgroup. A gate... |
| [ ] | $key | Revoke / Queue Position Info* (names VERIFY, TS 24.380). Mostly a one-to-one state mapping (Q10) â€” the hard part is *timing*: LMR grants take hundreds of m... |
| [ ] | $key | ### VERIFY list for this pack |
| [ ] | $key | Synthetic data only; never live traffic. VERIFY = check TIA-102 / ETSI / 3GPP before quoting. |
| [ ] | $key | / "The CC carries voice too." / It's a channel. / Dedicated CC carries signalling only (some systems use a composite CC; VERIFY). / |
| [ ] | $key | / "A unit can affiliate to many groups." / Scanning radios do hear many. / Scan lists are a radio feature; the system-visible affiliation is one group (VERIF... |
| [ ] | $key | deployments â€” VERIFY). Test: `max_delay_spread_km(4800, 0.25) â‰ˆ 15.61`. |
| [ ] | $key | *No single equivalent (VERIFY each against TS 24.380 / TIA-102): |
| [ ] | $key | Late entry â€” LMR repeats parameters; MCPTT late joiners get state via SIP/floor signalling (e.g. the floor arbitrator sends *Taken* with the current talker... |
| [ ] | $key | lcm: TETRA 30 ms â†” P25/DMR 20 ms: lcm = 60 ms (2 TETRA = 3 P25 frames). P25 â†” MCPTT (AMR-WB 20 ms â€” VERIFY): lcm = 20 ms (frames align one-to-one). |
| [ ] | $key | Samples at 8 kHz: 20 ms = 160; 30 ms = 240. (Real transcoders may resample to 16 kHz for wideband codecs; VERIFY.) |
| [ ] | $key | *Boxes. LMR system (P25 RFSS/CSSI or DFSI endpoint â€” VERIFY interface) â†” IWF â†” MCPTT server (SIP/RTP) â†” LTE/5G UEs. IWF internals: *identity mapper* ... |
| [ ] | $key | > copyright-protected and under licences with obligations â€” VERIFY each licence); write your own from the derivations below. This pack implements no cipher. |
| [ ] | $key | Standards (numbers from memory â€” VERIFY; "clause VERIFY" = look it up): TIA-102.BAAA (P25 FDMA common air interface: sync, NID, status symbols, LDU, |
| [ ] | $key | 5. Dispatch on DUID: HDU â†’ header, LDU1/LDU2 â†’ voice, TSBK â†’ control, PDU â†’ data, TDU/TDULC â†’ terminator; fixed-size frames have a known payload (L... |
| [ ] | $key | Concrete layout (this pack's stream model â€” VERIFY against TS 102 361-1): slot = 24-bit gap/CACH + 264-bit burst = 288 bits = 30 ms at 9600 bit/s; burst = ... |
| [ ] | $key | ### VERIFY list for this pack |
| [ ] | $key | 6. Licences of OP25 / dsd-fme / SDRTrunk (all believed GPL-family; VERIFY) and the rule "study, don't copy". |
| [ ] | $key | Synthetic data only; never live traffic; no ciphers. VERIFY = check TIA-102.BAAA / ETSI TS 102 361-1 / TIA-102.AABC before quoting. |
| [ ] | $key | Etherstack angle:* P25 LC and DMR slot-type/embedded words use Golay-family codes (VERIFY which); knowing the failure mode (silent miscorrection) is why desi... |
| [ ] | $key | *(c) A sync burst appears once per superframe (6 bursts of a logical channel = 360 ms, pack 03), so worst-case acquisition â‰ˆ 360 ms + 30 ms burst time â‰ˆ ... |
| [ ] | $key | Because it's perfect it can never detect failure: 4 errors miscorrect. Adding an overall parity bit gives Golay(24,12,8): correct 3, detect 4 â€” decode the ... |
| [ ] | $key | *Failure modes to volunteer: (1) stuck PTT â‡’ talk timeout *and* lock-out until release (Q12); (2) lost release (radio leaves coverage) â‡’ talk timeout/los... |
| [ ] | $key | 1. Hand-computed golden vectors (0xC75, 0x149F, 0xBEE0000100ABCD style) and vectors from the standards where published (VERIFY). |
| [ ] | $key | > Real-target addresses/bit positions below are from memory (STM32/Cortex-M style) â€” VERIFY against the reference manual and the ARM Architecture Reference... |
| [ ] | $key | At power-on the CPU has no instructions in its pipeline and no stack. It does exactly two memory reads from the vector table at the start of flash (address 0... |
| [ ] | $key | 1. (Optionally set up clocks, FPU, VTOR.) 2. Copy `.data` (initialised globals) from flash to RAM. 3. Zero `.bss` (zero-initialised globals). 4. Run static c... |
| [ ] | $key | (`const` globals stay in flash only: no RAM cost â€” important on a 20 KiB part; but a `const` object with a constructor may land in RAM â€” VERIFY.) |
| [ ] | $key | *Fix: BSRR (bit set/reset register, STM32-style): the lower 16 bits *set* pins, the upper 16 *reset* them, in a single write with no read: pins you don't men... |
| [ ] | $key | 2. No memory barrier for the CPU/bus. It doesn't order writes against the CPU's own write buffer or other cores/DMA â€” use `DSB`/`DMB` (or the platform's ba... |
| [ ] | $key | 3. Q3 (Calculate) â€” Where is the handler address for device IRQ 5? For the SysTick exception (VERIFY its index)? (W 3.1) |
| [ ] | $key | *VERIFY list: vector-table layout and Thumb requirement (ARMv6-M/v7-M ARM); flash alias at 0x0 and VTOR relocation; `.init_array` symbol names and startup or... |
| [ ] | $key | Host-simulated registers; no hardware. VERIFY = check the ARM ARM / reference manual. |
| [ ] | $key | *Answer. Device IRQ 5 â†’ `vt[16+5] = vt[21]`, byte offset `21 Ã— 4 = 0x54`; strip bit 0. SysTick is exception 15 â†’ `vt[15]`, offset `0x3C` (VERIFY: the sy... |
| [ ] | $key | 1. Thumb bit / vector table wrong (linker/VTOR after bootloader). 2. Stack top (linker script RAM size) or stack overflow due to the optimised build's inlini... |
| [ ] | $key | > Register conventions (STM32-style USART_BRR, "top bits implemented" NVIC priorities, ISR entry cycles) are from memory â€” VERIFY against your part's refer... |
| [ ] | $key | *Priority = a number, lower is more urgent. Cortex-M implements only the top N bits of the 8-bit priority field (N = 3 on many parts, 4 on STM32 â€” VERIFY):... |
| [ ] | $key | *ISR hygiene: keep it short; clear the peripheral's flag (otherwise it re-fires immediately); no blocking, no `malloc`, no `printf`; communicate through `vol... |
| [ ] | $key | The baud generator divides the peripheral clock: `BRR = round(fclk / baud)` (16Ã— oversampling; `uart_brr`). The actual baud is `fclk / BRR` and the error ac... |
| [ ] | $key | > Misconception box â€” "I2C and SPI are interchangeable". SPI: fast, more wires, no addressing, full duplex. I2C: fewer wires, slower, addressed, half-duple... |
| [ ] | $key | *Cache coherence caveat on Cortex-M7/A cores: DMA bypasses the CPU cache â€” invalidate/clean the buffer region around the transfers (VERIFY). |
| [ ] | $key | *VERIFY list: number of implemented priority bits on your part; ISR entry/tail-chain cycle counts; USART oversampling and BRR format (STM32 RM); baud toleran... |
| [ ] | $key | Host-simulated; no hardware. VERIFY = check your part's reference manual. |
| [ ] | $key | / "BRR = fclk / (16Â·baud)" / Mix with oversampling formula. / In this model BRR already includes the Ã—16 (`fclk/baud`); the 16Ã— is inside the hardware div... |
| [ ] | $key | > VERIFY against the FreeRTOS documentation for your version before relying on a detail. Mars Pathfinder facts are from the public post-mortems (VERIFY detai... |
| [ ] | $key | higher-priority task that becomes ready preempts the running one immediately. Tasks of equal priority share the CPU round-robin on each tick (`configUSE_TIME... |
| [ ] | $key | (FreeRTOS also offers task notifications, a lighter binary-semaphore replacement â€” VERIFY.) |
| [ ] | $key | / Priority inheritance / no / yes (FreeRTOS mutexes implement it â€” VERIFY; binary semaphores don't) / |
| [ ] | $key | *History (VERIFY): July 1997, Mars Pathfinder (VxWorks RTOS). A low-priority meteorological task held the information-bus mutex; the high-priority bus-manage... |
| [ ] | $key | *Alternatives: priority ceiling (the mutex has a ceiling = highest priority of any task that uses it; the owner runs at the ceiling immediately â€” prevents ... |
| [ ] | $key | *VERIFY list: FreeRTOS time-slicing default; mutex priority inheritance behaviour (single vs chained inheritance) in your kernel version; recursive mutex API... |
| [ ] | $key | Tick-based simulation, not a real RTOS. VERIFY FreeRTOS API details for your version. |
| [ ] | $key | *Answer. Each task has a fixed priority; the scheduler always runs the highest-priority ready task; when a higher-priority task becomes ready (timer expiry, ... |
| [ ] | $key | *Answer. "July 1997, Mars Pathfinder, VxWorks. A low-priority meteorological task took the information-bus mutex; the high-priority bus manager blocked on it... |
| [ ] | $key | / "Use a mutex with zero timeout in the ISR" / Non-blocking. / Still no owner semantics; unsupported/UB in FreeRTOS (VERIFY). / |
| [ ] | $key | > The register layout is invented for this exercise (not any real chip's). Board/RTOS/Linux facts are from memory â€” VERIFY for your platform. |
| [ ] | $key | Tests: FIFO order and full/empty; wraps 1000Ã—; and `TwoThreadsPreserveOrderAndLoseNothing`: producer thread pushes 200 000 integers, consumer pops them â€” ... |
| [ ] | $key | *Mapping to real FreeRTOS (exercise, not built here â€” VERIFY the port layout): run the three tasks on the FreeRTOS POSIX/Linux simulator port (kernel's `po... |
| [ ] | $key | Boot chain: ROM â†’ bootloader (U-Boot, VERIFY) â†’ kernel + device tree (describes hardware: addresses, IRQs, clocks) â†’ root filesystem â†’ init (systemd/... |
| [ ] | $key | Real-time: stock Linux is not hard real-time; `PREEMPT_RT`, CPU isolation, `SCHED_FIFO`, `mlockall`, and moving the time-critical bit to an MCU/FPGA are the ... |
| [ ] | $key | Linker flags: `-ffunction-sections -fdata-sections -Wl,--gc-sections` drop unused code; `-Os` vs `-O2`; map file for size accounting (VERIFY toolchain flags). |
| [ ] | $key | *SWD (Arm Serial Wire Debug): 2 pins (SWCLK, SWDIO), same capabilities for Arm cores, fewer pins on small packages; optional SWO pin for trace/printf output.... |
| [ ] | $key | Decide by: sample rate Ã— operations per sample vs CPU capacity; deterministic latency (fabric: cycles; CPU: scheduling jitter); power/area/cost; development... |
| [ ] | $key | *VERIFY list: register layout is invented (not a real chip); `-fsanitize=thread` incompatibility with ASan; FreeRTOS POSIX port path and trace-hook names; UI... |
| [ ] | $key | Emulated hardware, synthetic signals only. VERIFY = check your platform. |
| [ ] | $key | User-space peripheral access: kernel driver (`/dev`, ioctl/sysfs), UIO (mmap registers, IRQ via read), or `/dev/mem` mmap for prototyping only. A gateway typ... |
| [ ] | $key | *Answer. JTAG: 4â€“5 wires, scan chain (multiple devices, boundary scan), universal. SWD: 2 wires (+SWO trace) on Arm cores, same debug features. "Won't conn... |
| [ ] | $key | Honesty note: the 160 B payload is 20 ms of 8 kHz Î¼-law (64 kbit/s), a *stand-in* for a vocoder frame. Real narrowband systems compress speech to a few kbit... |
| [ ] | $key | Guarantees (properties of the polynomial; VERIFY the exact message-length limits in a CRC catalogue): detects all burst errors â‰¤ 16 bits, all odd-weight er... |
| [ ] | $key | *Q11 â€” Story (design). *Why Golay and Hamming here, and what would you use in a production radio?* Cover rate (12/23 vs 4/7), burst errors and interleaving... |
| [ ] | $key | Context: interview framing; VERIFY any standard-specific claims.* |
| [ ] | $key | *VERIFY list (not tested here; confirm before quoting in an interview): |
| [ ] | $key | Toy format, synthetic data only. VERIFY = check against the standard/catalogue before quoting. |
| [ ] | $key | / "Detects up to 16 flipped bits anywhere" / 16-bit CRC. / The guarantee is for a *burst* of length â‰¤ 16; scattered multi-bit patterns have only the HD-bas... |
| [ ] | $key | *Model answer. (1) *Target:* residual undetected error â‰ˆ 2^-16 Ã— (fraction of frames with uncorrected errors â‰ˆ 0.7 %) â‰ˆ 10^-7 per frame; at 50 frames/... |
| [ ] | $key | *Model answer. Rate: Golay 12/23 = 0.52, Hamming 4/7 = 0.57 â€” both costly for their power. Hamming(7,4) corrects 1 in 7 bits; real radio errors come in bur... |
| [ ] | $key | *Model answer. *Sync word:* the receiver needs a pattern whose correlation with every shifted copy of itself and with random data is small, so the peak is un... |
| [ ] | $key | *Answer (examples; VERIFY per standard). Vocoder (speech at a few kbit/s, not 64); interleaving (burst â†’ scattered errors); scrambling/whitening (avoid lon... |
| [ ] | $key | Speech has a wide dynamic range (whispers to shouts) but human hearing judges *relative* error. Uniform 8-bit PCM would give loud sounds a great SNR and quie... |
| [ ] | $key | > Misconception box. *"Î¼-law is compression like MP3."* It is a fixed 2:1 reduction (16 â†’ 8 bits) by non-uniform quantisation, no model of speech. A vocod... |
| [ ] | $key | Why PLC "repeat previous frame": it's the cheapest concealment, with an acceptable result for a single 20 ms loss, and it degrades to a buzz when several fra... |
| [ ] | $key | Talkgroup mapping used here: TG N â†’ `239.192.(N>>8).(N&255)` (the `239.192.0.0/14` organisation-local scope, RFC 2365; VERIFY scope ranges). Because the se... |
| [ ] | $key | *Q17 â€” Story (design). *Why multicast, and how do you map talkgroups to groups?* Cover: replication in the network, `239.192/14` scope, TTL 1, the 32:1 ali... |
| [ ] | $key | *VERIFY list (not tested here): |
| [ ] | $key | Synthetic signals only. VERIFY = check the RFC/standard before quoting. |
| [ ] | $key | *Answer. `80 80 00 10 00 00 0A 00 12 34 56 78 / FF FF`. Byte0 `0x80` â†’ V=2, P=0, X=0, CC=0. Byte1 `0x80` â†’ M=1, PT=0 (PCMU). Seq `0x0010` = 16. Timestamp... |
| [ ] | $key | *Answer. A constant delay means `Rj âˆ’ Ri = Sj âˆ’ Si` for every pair â†’ D = 0 â†’ J = 0: a fixed 777-tick offset needs no buffer, and the sender/receiver ... |
| [ ] | $key | *Model answer â€” hits: (1) Deadline: voice has a ~150 ms mouth-to-ear budget (VERIFY ITU-T G.114); a retransmit costs â‰¥ 1 RTT and arrives after its slot. ... |
| [ ] | $key | *Model answer. Speech is quasi-stationary over ~20 ms, so vocoders frame at 20 ms and RTP follows. Trade-off table: 10 ms = 96 kbit/s (+20 %), 20 ms = 80, 40... |
| [ ] | $key | *Model answer. G.711 is the lowest-common-denominator codec: universal interoperability with consoles/telephony, no licence, trivial CPU. Cost: 64 kbit/s vs ... |
| [ ] | $key | *Model answer. One send, N receivers: sender and gateway uplink are independent of the number of listeners; dispatch consoles join the group they need. Mappi... |
| [ ] | $key | Non-coherent detection: the receiver does not know the carrier phase, so it uses the *magnitude* of each correlation, `/Î£ w[n] e^{âˆ’j2Ï€f_k n/8}/Â²`, and p... |
| [ ] | $key | Honesty: this toy has `1/T`-spaced tones (modulation index 1), roughly 4â€“5 Ã— the symbol rate of bandwidth. Narrowband land-mobile systems use a much small... |
| [ ] | $key | *Q13 â€” Story (design). *Why non-coherent 4FSK for this link rather than QPSK/QAM?* Cover: constant envelope (saturated PA, battery), no carrier-phase recov... |
| [ ] | $key | *Q15 â€” Story (design). *How much CFO can this modem tolerate and how did you design for it?* Cover: the half-spacing limit, the sinc losses, oscillator ppm... |
| [ ] | $key | *VERIFY list (not tested here): |
| [ ] | $key | Synthetic signals only. VERIFY = check a reference before quoting. |
| [ ] | $key | *Model answer. *Constant envelope* lets the PA run saturated (efficient; handheld battery life, low heat) without spectral regrowth that would hurt adjacent ... |
| [ ] | $key | *Model answer. Hard limit: half the tone spacing (Î´ = 0.5 cycles/symbol = half the symbol rate); practical limit well below, because `sincÂ²` loss (âˆ’0.58 ... |
| [ ] | $key | Build: `cmake -S code -B build -DCMAKE_CXX_FLAGS="-Wall -Wextra -fsanitize=address,undefined"`, `cmake --build build`, `ctest --test-dir build`. `student.*` ... |
| [ ] | $key | *Status codes (numbers equal gRPC's, so the adapter is a `static_cast`; VERIFY against `grpc/status_code_enum.h`): `Ok 0`, `InvalidArgument 3`, `DeadlineExce... |
| [ ] | $key | *Q13 â€” Story (design). *Why floor control, and what happens without it?* Cover: half-duplex PTT semantics, collision of simultaneous talkers at the gateway... |
| [ ] | $key | *VERIFY list (not tested here): |
| [ ] | $key | Synthetic traffic only. VERIFY = check against the standard/library before quoting. The gRPC adapter is an untested skeleton. |
| [ ] | $key | *Answer. `Unavailable`: true/true. `DeadlineExceeded`: false (non-idempotent) / true (idempotent). `Ok`, `InvalidArgument`, `NotFound`, `AlreadyExists`, `Res... |
| [ ] | $key | *Model answer â€” hits: (1) PTT is half-duplex: one talker per talkgroup; with two simultaneous talkers a gateway produces overlapping RTP streams that recei... |
| [ ] | $key | *Model answer. *Deadlines:* every call has one (otherwise a hung server hangs the caller); deadlines propagate so servers abandon work for callers that have ... |
| [ ] | $key | Parse a *language* `L` of valid strings. A DFA decides `L` in one pass if `L` is regular. The state encodes exactly the information about the prefix that mat... |
| [ ] | $key | *Ctx: verify a sorted index/frequency table wasn't corrupted in storage. |

Generated from worksheet/answers VERIFY markers. Re-run extraction after editing packs.
