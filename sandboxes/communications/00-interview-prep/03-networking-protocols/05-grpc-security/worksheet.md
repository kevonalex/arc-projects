# 03-net / 05 — gRPC, Protobuf (bit by bit), HTTP/2, Deadlines, SRTP/DTLS Concepts  *(Tier 2)*

**Time:** ~3–4 h · **Prereqs:** `03-sockets-poll`, `04-rtp-sip` · **Code:** `code/include/grpc_security.hpp` (stub), `code/solutions/grpc_security.hpp`, `code/tests/test_grpc_security.cpp`, optional real gRPC build in `code/grpc/`
**Etherstack lens:** the gateway's *control plane* — start/stop a call, stream statistics to an operator console — is a gRPC API (capstone M4). The *media plane* stays RTP, secured with SRTP. The interview asks: "what is on the wire for a gRPC call?", "how do you set deadlines?", "how is SRTP keyed?"

> **Legal:** all data here is synthetic (call IDs, counters). Do not capture or decrypt live public-safety traffic (UK Wireless Telegraphy Act 2006). Use your own loopback gRPC and test keys only.

---

## Part A — Bytes first

This is a `CallStats` message from the gateway (`call_id = "c1"`, `packets = 300`, `lost = 3`, `jitter_ms = 1.5`):

```text
0a 02 63 31   10 ac 02   18 03   21 00 00 00 00 00 00 f8 3f
```

**Q1.** Decode it by hand: split into fields; for each give *field number*, *wire type*, and value. Explain `ac 02` and the last 9 bytes. (IEEE-754: `1.5 = 0x3FF8000000000000`.) How many bytes is this vs the same data as JSON? *Context: a console polling 100 active calls five times a second.*

---

## Part B — Protobuf encoding, derived

### B.1 Varint (base-128)
**Problem:** integers are usually small; sending 8 bytes for the number 3 wastes space. **Idea:** use as many bytes as needed. Each byte carries **7 payload bits**; its top bit (MSB) is a **continuation flag** (1 = another byte follows). Groups are emitted **least-significant first** (little-endian) because the receiver can then start accumulating immediately and shifts only grow.

Encode **300** step by step:
```text
300 = 0b1_0010_1100          (9 bits)
split into 7-bit groups from the right:  [000 0010] [010 1100]
emit lowest group first, set MSB on all but the last:
   010 1100 -> 1010 1100 = 0xAC      (MSB=1: more follows)
   000 0010 -> 0000 0010 = 0x02      (MSB=0: last)
=> AC 02
decode: (0x02 << 7) | 0x2C = 256 + 44 = 300 ✓
```
Sizes: 0–127 → 1 B; 128–16 383 → 2 B; … up to 2⁶³ needs `ceil(64/7)` = **10 bytes**. The 10th byte can hold only bit 63, so a valid 10th byte is 0 or 1 — a decoder must reject larger values or silently overflow (test `Varint.DecodeRejectsMalformed`).
Implementation detail: `while (v >= 0x80) { out(v | 0x80); v >>= 7; } out(v);`

### B.2 Negative numbers and ZigZag
Plain `int32/int64` fields encode negatives as **64-bit two's complement** → `-1` becomes `FF FF FF FF FF FF FF FF FF 01` = **10 bytes**. Telemetry is full of small negatives (RSSI −97 dBm, clock drift −3 ppm). **ZigZag** maps signed → unsigned so small magnitudes stay small:
`zz(n) = (n << 1) ^ (n >> 63)` (arithmetic shift) : `0→0, −1→1, 1→2, −2→3, 2→4`. Inverse `(u >> 1) ^ -(u & 1)`. Use `sint32/sint64` for fields that are often negative; −97 → 193 → 2 bytes instead of 10.

### B.3 Fields, tags, wire types
A message is a sequence of `(tag, value)`. **tag = varint of `(field_number << 3) | wire_type`.**
| Wire type | Value | Used for |
|-----------|-------|----------|
| 0 | varint | int32/64, uint32/64, sint, bool, enum |
| 1 | 8 bytes little-endian | fixed64, sfixed64, **double** |
| 2 | varint length + bytes | string, bytes, nested message, packed repeated |
| 5 | 4 bytes little-endian | fixed32, float |
(3/4 = deprecated groups.) Field numbers 1–15 fit in a 1-byte tag (4 bits of number + 3 bits of type = 7 payload bits, MSB clear); 16–2047 take 2 bytes → give frequent fields low numbers. Max field number 2²⁹ − 1.

Design reasons: **self-describing enough to skip** (a reader that doesn't know field 7 still knows how long it is from the wire type) → **forward/backward compatibility**; **proto3 omits default values** (0, "", false) → smaller, but "absent" ≠ "set to 0" unless you use `optional`.
Compatibility rules: never reuse a field number; add new fields freely; `int32/uint32/int64/uint64/bool` are wire-compatible with each other (⚠ VERIFY); changing `string`↔`bytes` is compatible only for valid UTF-8; `sint` is **not** compatible with `int`.
Why not JSON? 55 bytes vs 18 for `CallStats` (and parsing costs).

### B.4 Robust decoding (security)
Every length is attacker-controlled: a `Len` field claiming 4 GiB must be rejected *before* allocating; `pointer + length` overflow must be checked against the remaining buffer; a decoder must stop on wire types it does not know; limit recursion depth for nested messages; apply a maximum message size (gRPC default receive limit 4 MiB ⚠ VERIFY).

> **Common misconceptions**
> 1. *"Protobuf is encrypted/obfuscated."* It is a plain, compact encoding; secrecy needs TLS.
> 2. *"A field with value 0 is sent."* Not in proto3 (default values are omitted).
> 3. *"Changing a field's type is harmless."* Only within the compatible wire-type families.
> 4. *"Varints are big-endian."* Little-endian groups.
> 5. *"Field order on the wire is fixed."* Parsers must accept any order; the last occurrence of a scalar wins.

**Q2.** Encode: 1, 127, 128, 300, 16 384, 2³². Draw the bit grouping for 16 384. Decode `e8 07`. *(Tests `Varint.*`.)*

**Q3.** Why at most 10 bytes? What bug results if a decoder accepts an 11-byte "varint" or a 10th byte of `0x7F`? What does a parser do on a continuation bit at the end of the buffer?

**Q4.** ZigZag: encode −1, −2, 63, −64, 2 147 483 647, −97. How many bytes does −97 take as `int32` vs `sint32`? Which fields of a radio telemetry message should be `sint`? *(Context: RSSI dBm, timing offset ppm, power-control step.)*

**Q5.** Compute the tag bytes for: field 1 varint, field 2 string, field 4 `double`, field 16 string, field 2047 varint. What does it imply for how you number fields in a high-rate telemetry message?

**Q6.** Schema evolution: the gateway adds `uint32 priority = 4;` to `StartCallRequest`. (a) What does an old server do with it? (b) A new server reading an old client's message — what is `priority`? (c) Someone later reuses `4` as `string region`. What goes wrong and what does protobuf's `reserved` do? *Context: rolling upgrades of a fleet of gateways.*

**Q7.** (Quantify) 100 active calls × 5 `CallStats` updates/s. Bandwidth with protobuf (18 B + 5 B gRPC prefix + 9 B HTTP/2 DATA header) vs JSON (55 B payload + same HTTP/2 overhead). *(Ignore HPACK headers and TCP/IP.)*

**Q8.** A parser receives `12 ff ff ff ff 0f 41 42`. Walk through what a naive and a correct parser do.

---

## Part C — HTTP/2 and gRPC on the wire

### C.1 Why HTTP/2 (derive from HTTP/1.1's problems)
HTTP/1.1: one outstanding request per connection (or fragile pipelining) → either many TCP connections (each with its own handshake and slow start) or **application-level HOL**; verbose text headers repeated on every request. HTTP/2 keeps TCP but changes the framing:
* **Binary frames**, **multiplexed streams** on one connection (`stream_id` identifies the logical request/response).
* **HPACK** header compression (static + dynamic table; repeated headers cost a few bits).
* **Per-stream and per-connection flow control** (credit-based; initial window 65 535 B per stream and per connection ⚠ VERIFY, raised with WINDOW_UPDATE / SETTINGS).
* Server push (rarely used), PING, GOAWAY (graceful shutdown), RST_STREAM (cancel one stream).

Frame header — 9 bytes: `length(24) | type(8) | flags(8) | R(1) stream_id(31)`. Types: 0 DATA, 1 HEADERS, 2 PRIORITY, 3 RST_STREAM, 4 SETTINGS, 5 PUSH_PROMISE, 6 PING, 7 GOAWAY, 8 WINDOW_UPDATE, 9 CONTINUATION. Flags: `END_STREAM` (0x1), `END_HEADERS` (0x4), `ACK` (0x1 on SETTINGS/PING). The client first sends the 24-byte **preface** `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n` (it deliberately looks like garbage to HTTP/1 servers, so they reject cleanly).

Example header: `00 00 0a 01 04 00 00 00 01` = length 10, type HEADERS, flags END_HEADERS, stream 1 (test `Http2.FrameHeaderRoundTrip`). Stream ids: client-initiated are **odd**, server-initiated even.

### C.2 gRPC = protobuf messages inside HTTP/2 streams
* One RPC = one HTTP/2 stream. Request: `POST /etherstack.callcontrol.v1.CallControl/StartCall`, headers `content-type: application/grpc`, `te: trailers`, optional `grpc-timeout: 2S`, `:authority`, `:scheme`.
* Each message is **length-prefixed**: `[1 byte compressed flag][4 bytes big-endian length][protobuf bytes]` — the *same* framing is required because HTTP/2 DATA frames don't align with messages (a message can span DATA frames; a DATA frame can hold several messages). (`grpc_frame` / `grpc_unframe`; incremental `NeedMore` is the TCP-stream lesson from pack 03.)
* Response: `HEADERS (:status 200, content-type application/grpc)` → `DATA` (messages) → **trailers** `HEADERS` with `grpc-status` and `grpc-message`, END_STREAM. Status goes in trailers because it is only known *after* a streaming response finished. (A failed call without messages may use "trailers-only".)

Unary `StartCall` on the wire:
```text
C→S  [preface] SETTINGS
S→C  SETTINGS, SETTINGS(ACK)       C→S SETTINGS(ACK)
C→S  HEADERS(stream 1, END_HEADERS)  :method POST, :path /…/StartCall, content-type, te, grpc-timeout
C→S  DATA(stream 1, END_STREAM)      00 | 00 00 00 0e | <protobuf StartCallRequest>
S→C  HEADERS(stream 1)               :status 200, content-type application/grpc
S→C  DATA(stream 1)                  00 | len | <protobuf StartCallResponse>
S→C  HEADERS(stream 1, END_STREAM)   grpc-status: 0
```

### C.3 The four call types
| Type | Shape | Gateway example | Ends when |
|------|-------|-----------------|-----------|
| **Unary** | 1 request → 1 response | `StartCall`, `EndCall` | response + trailers |
| **Server streaming** | 1 request → N responses | `WatchStats` | server finishes or client cancels |
| **Client streaming** | N requests → 1 response | `ReportAudio` (upload chunks, one summary) | client half-closes (END_STREAM), then response |
| **Bidirectional** | N ↔ N, independent | `FloorChannel` (floor events both ways) | either/both half-close |
Within a stream, **message order is preserved**; across streams there is no ordering. Streams give back-pressure through HTTP/2 flow control.

> **Common misconceptions**
> 1. *"gRPC is faster than HTTP."* It *is* HTTP/2 + protobuf; the gains are compact encoding, connection reuse and streaming.
> 2. *"HTTP/2 removed head-of-line blocking."* Only at the HTTP layer. All streams share **one TCP connection**: one lost segment stalls them all (pack 02 Part A). HTTP/3/QUIC moves streams over UDP to fix that.
> 3. *"A gRPC status is an HTTP status."* Always HTTP 200 with the real result in `grpc-status` trailers (non-gRPC errors map: 503→UNAVAILABLE, `grpc_status_from_http`).
> 4. *"Streaming RPCs are WebSockets."* Different protocol, but same idea.
> 5. *"gRPC needs TLS."* No, but you do (mTLS between gateway components).

**Q9.** Draw the HTTP/2 frame sequence for a **server-streaming** `WatchStats` that returns three messages and then the call ends; mark which frames carry END_STREAM and where `grpc-status` appears. What changes when the client cancels after the first message?

**Q10.** Parse by hand: `00 00 0a 01 04 00 00 00 01`, then `00 00 04 08 00 00 00 00 00 00 00 ff ff`. For the second, what does it do and what's the new window size? *(Type 8 = WINDOW_UPDATE: payload is a 31-bit increment.)*

**Q11.** A client-streaming audio upload with a 65 535 B stream window, RTT 100 ms: maximum throughput without WINDOW_UPDATEs? What is the connection-level analogue and which formula from pack 02 applies? *(BDP.)*

**Q12.** One gRPC channel carries both a latency-critical `StartCall` and a bulk `ReportAudio`. Packet loss hits the bulk transfer. What happens to `StartCall`? List two mitigations. *Context: operator console UI freezes while logs upload.*

**Q13.** Classify and write the proto signature for: (a) request the current call table; (b) subscribe to RTCP-derived quality changes; (c) upload a 30-second recorded test clip in chunks; (d) a floor-control chat between two gateways; (e) health check. Which can be retried safely?

---

## Part D — Deadlines, cancellation, status

**Deadline vs timeout.** gRPC propagates a **relative** `grpc-timeout` header (`<≤8 digits><H|M|S|m|u|n>`, e.g. `100m`, `2S`) — relative because the two machines' clocks are not synchronised (the same trick as RTCP's LSR/DLSR in `04-rtp-sip`). Each hop **converts to its own absolute deadline** on receipt and **sends the remaining time** on to the next hop.

Why *always* set a deadline: the default is "wait forever". A hung dependency then pins a thread/stream/memory per request until the gateway runs out of resources — a classic cascading failure. Deadline-exceeded is *normal control flow* in a distributed system.

**Propagation arithmetic** (code: `remaining_ms`, `format_grpc_timeout`): console sets 2 000 ms; the gateway spends 300 ms, keeps 100 ms for its own response: sends `2000 − 300 − 100 = 1600 ms` → `1600m` to the core. If the remaining budget is ≤ 0, **fail fast** with `DEADLINE_EXCEEDED` instead of calling.

**Cancellation:** `TryCancel` / dropping the RPC → `RST_STREAM(CANCEL)`; the server handler checks `ctx->IsCancelled()` and stops work.

**Status codes you must know:** `OK 0`, `CANCELLED 1`, `UNKNOWN 2`, `INVALID_ARGUMENT 3`, `DEADLINE_EXCEEDED 4`, `NOT_FOUND 5`, `ALREADY_EXISTS 6`, `PERMISSION_DENIED 7`, `RESOURCE_EXHAUSTED 8`, `FAILED_PRECONDITION 9`, `ABORTED 10`, `OUT_OF_RANGE 11`, `UNIMPLEMENTED 12`, `INTERNAL 13`, `UNAVAILABLE 14`, `DATA_LOSS 15`, `UNAUTHENTICATED 16`.
**Retries:** only retry **idempotent** operations and only on `UNAVAILABLE` (and sometimes `DEADLINE_EXCEEDED`), with exponential backoff + jitter and a retry budget — never retry `StartCall` blindly (it may have succeeded: use a client-generated `call_id` as an idempotency key → `ALREADY_EXISTS` means "already done").

**Q14.** The console calls `StartCall` with a 2 s deadline; the gateway needs 300 ms of its own work, a 100 ms margin, then calls the core which must call a database. Give the `grpc-timeout` strings sent at each hop if the DB hop gets half of what's left after another 200 ms of core work. Format them with the rules of `format_grpc_timeout`; parse `"1500m"` and `"10S"`.

**Q15.** Choose the gRPC status for: empty `call_id`; duplicate `call_id`; unknown call on `EndCall`; talkgroup table full; deadline passed; caller lacks rights (authenticated); no/invalid credentials; method not implemented; the core process is restarting. Which are safe to retry?

**Q16.** Why does `CallControlService::end_call` invoke subscriber sinks *outside* its mutex? Construct the deadlock if it did not. *(Test `CallControl.SinkMayCallBackIntoService`.)*

---

## Part E — Securing the media plane: SRTP and DTLS (concepts)

### E.1 Why a separate mechanism from TLS-over-TCP?
Media needs UDP's low latency and loss tolerance; TLS needs a reliable ordered stream (HOL again). Also routers, QoS classifiers, jitter buffers and RTCP need the **RTP header** (seq, timestamp, SSRC, PT) *in the clear* — so we **encrypt the payload and authenticate the whole packet**, leaving the header readable. That is **SRTP** (RFC 3711). RTCP gets **SRTCP**.

### E.2 SRTP essentials
* **Cipher:** AES in counter mode (keystream XOR payload) by default (or AES-GCM). Counter mode is a *stream cipher*: encrypting a lost/late packet doesn't affect others — no chaining across packets. **Keystream must never be reused** (two-time pad: XOR of two ciphertexts = XOR of two plaintexts).
* **IV derivation:** from the session salt, the **SSRC** and the **48-bit packet index** `i = 2¹⁶ × ROC + SEQ`. Only SEQ is in the packet; the receiver reconstructs the **rollover counter (ROC)** by estimating the closest index to the last one (RFC 3711 App. A) — code `SrtpIndex::estimate`: if the last seq `s_l < 2¹⁵`, a packet with seq `> s_l + 2¹⁵` belongs to the *previous* cycle; if `s_l ≥ 2¹⁵` a very small seq belongs to the *next* cycle. This saves 4 bytes per packet but means the two ends **desynchronise** after an outage of ≥ 2¹⁵ packets (655 s at 50 pps) — a design trade-off.
* **Authentication:** HMAC-SHA1 over header+payload (+ROC), tag truncated to **80 bits (10 B)** by default (or 32 bits, 4 B) ⚠ VERIFY. Check the tag **before** decrypting and **before** updating any state (otherwise an attacker spoofing packets can wreck your ROC estimate and replay window).
* **Replay protection:** sliding window of 64 indices (`ReplayWindow`): reject indices already seen or older than `highest − 63`. `check()` first, `update()` only after authentication.
* **Overhead:** G.711 20 ms packet 200 B → 210 B (10 B tag): 84 kbit/s IP rate vs 80 (+5 %).
* **Master key/salt:** 128-bit key + 112-bit salt typically ⚠ VERIFY; session encryption/auth/salt keys derived from them with a KDF, so one master key never encrypts directly.

### E.3 Key exchange (the hard part)
| Method | How | Trust |
|--------|-----|-------|
| **SDES** (RFC 4568) | key sent in SDP `a=crypto:` | only as secure as SIP signalling (needs TLS/SIPS); key visible to proxies |
| **DTLS-SRTP** (RFC 5764) | a **DTLS handshake runs on the media path**; keys exported from the DTLS session (RFC 5705 exporter); SDP carries `a=fingerprint:` of the certificate | end-to-end, forward secrecy; identity bound through authenticated signalling |
| **ZRTP** | Diffie-Hellman in-band, users compare a short string | no PKI |
| **MIKEY** (3GPP MCPTT uses MIKEY-SAKKE) ⚠ VERIFY | key transport with the signalling | operator-managed keys |

**DTLS** = TLS adapted to datagrams: explicit record sequence numbers + epoch (records can be lost/reordered), **retransmission timers** for the handshake flight messages, **HelloVerifyRequest/cookie** so a server doesn't allocate state for spoofed sources (same idea as SYN cookies), handshake message fragmentation (a certificate chain > MTU), replay window. After the handshake only the *exporter keys* are used for SRTP; DTLS then idles.
**Latency cost:** DTLS adds 1–2 RTT (plus retransmission) before the first audio can be protected → for PTT this is *first-word clipping*. Mitigate: keep sessions/keys established across talk bursts (floor control without re-keying), pre-share keys (MIKEY), or accept for call setup only.

> **Common misconceptions**
> 1. *"SRTP hides everything."* Headers, SSRC, sizes and timing (so *who talks when*, and codec by packet size) remain visible — traffic analysis.
> 2. *"Authenticated = encrypted."* SRTP can be run with NULL cipher and auth only; they are independent.
> 3. *"AES makes packet-length leaks impossible."* CTR mode preserves length; VBR codecs leak content patterns.
> 4. *"The ROC is transmitted."* It is implicit (derived).
> 5. *"DTLS-SRTP means DTLS carries the audio."* DTLS only does the handshake; SRTP carries the audio.
> 6. *"Encrypting the LMR air interface secures the IP leg."* Different hops, different keys (AES-256 over-the-air on P25/DMR; SRTP/TLS on IP): the gateway is a **trust boundary** where keys/formats change (`07-lmr/05`) ⚠ VERIFY per system.

**Q17.** Why encrypt only the payload and not the header? List three network functions that break if the RTP header were encrypted.

**Q18.** Overhead: compute the IP bandwidth of G.711 (20 ms) with SRTP tag 80 bit and 32 bit, and with AES-GCM (16 B tag, no HMAC): 200 B base packet. *Context: Etherstack's backhaul budget.*

**Q19.** SRTP index: `ROC = 3`, last seq `s_l = 0x0010`; a packet with seq `0xFFF0` arrives. Index? Now `s_l = 0xFFF0`, `ROC = 3`; seq `0x0005` arrives. Index? Why must `update()` not run before authentication? What happens after a 15-minute outage at 50 pps? *(Tests `Srtp.*`.)*

**Q20.** Replay window of 64. Indices arrive: `100, 101, 99, 101, 37, 200, 150, 136`. Mark accepted/rejected. *(Test `Srtp.ReplayWindow`.)*

**Q21.** Draw the DTLS-SRTP call setup: SIP INVITE/200 with `a=fingerprint`, DTLS handshake (ClientHello, HelloVerifyRequest + cookie, …), key export, first SRTP packet. Why is the fingerprint in SDP needed (what attack does it prevent)? Where does first-word clipping come from?

**Q22.** (Design) The gateway receives AES-256 **encrypted** LMR voice from a radio network and must deliver it to an LTE MCPTT client. Describe three options (end-to-end opaque transport with the vocoder payload encrypted; decrypt-and-re-encrypt at the gateway with a key-management interface; no interworking) with trust/key-management trade-offs. ⚠ Flag what you'd verify with the customer.

---

## Part F — Build tasks (`code/`)

| Step | Functions in `grpc_security.hpp` | Tests |
|------|----------------------------------|-------|
| 1 | `encode_varint`, `decode_varint`, `zigzag_*` | `Varint.*` |
| 2 | `make_tag`, `ProtoWriter`, `ProtoReader` | `Proto.*` |
| 3 | `encode/decode` of `StartCallRequest` and `CallStats` | `Messages.*` |
| 4 | `grpc_frame`, `grpc_unframe` | `GrpcFrame.*` |
| 5 | `parse/build_h2_frame_header`, `h2_type_name` | `Http2.*` |
| 6 | `parse/format_grpc_timeout`, `remaining_ms`, `grpc_status_from_http` | `Deadline.*` |
| 7 | `SrtpIndex`, `ReplayWindow` | `Srtp.*` |
| 8 | `CallControlService` (start/end/stats/subscribe/publish) | `CallControl.*` |

```bash
cmake -S code -B build && cmake --build build
./build/grpcsec_tests_student     # red until include/grpc_security.hpp is implemented
./build/grpcsec_tests_solution
```

**Real gRPC (optional):** `code/grpc/callcontrol.proto` defines `CallControl` (`StartCall`, `EndCall`, `WatchStats`) plus the two other call shapes in `CallExtras`. `code/grpc/README.md` explains **how to generate the C++ code** (`protoc --cpp_out --grpc_out --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin)`) and build the server/client with `-DWITH_GRPC=ON`. The adapters (`grpc_server.cpp`, `grpc_client.cpp`) are thin: they translate protobuf ↔ the structs you tested and map errors to gRPC status codes. **They were not compiled when this pack was authored — treat as a skeleton.**
Then capture on loopback and find the frames from Part C.2 in Wireshark (*Decode As → HTTP2*).

---

## ⚠ VERIFY list (this pack)
1. gRPC default max receive message size (4 MiB); HTTP/2 initial window 65 535.
2. Protobuf wire-compatible scalar families (int32/uint32/int64/uint64/bool; string/bytes).
3. HTTP→gRPC status mapping table (`grpc_status_from_http`) against the gRPC HTTP/2 spec.
4. SRTP default tag lengths (80/32 bit), master key/salt sizes, AES-GCM profiles (RFC 7714).
5. SRTP index estimation (RFC 3711 App. A) and the 2¹⁵-packet outage limit.
6. DTLS-SRTP (RFC 5764), exporter (RFC 5705), SDES (RFC 4568), MIKEY-SAKKE in MCPTT.
7. The gRPC skeleton (`code/grpc/`): never compiled here — package names/targets vary per distro.
8. Radio-side encryption interworking claims (Q22) are design discussion, not standards text.
