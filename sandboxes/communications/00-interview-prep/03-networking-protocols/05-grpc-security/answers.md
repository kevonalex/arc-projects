# Answers — 03-net / 05 gRPC, Protobuf, HTTP/2, SRTP/DTLS

Try each question first. ⚠ = verify against the cited spec.

---

**Q1.** `0a 02 63 31 | 10 ac 02 | 18 03 | 21 00 00 00 00 00 00 f8 3f`
| Bytes | Tag | Field # | Wire type | Value |
|-------|-----|---------|-----------|-------|
| `0a 02 63 31` | `0a` = 0b00001010 → (1<<3)\|2 | 1 | 2 (length-delimited) | len 2, "c1" |
| `10 ac 02` | `10` → (2<<3)\|0 | 2 | 0 varint | `ac 02` = 0x2C + (2<<7) = 44 + 256 = **300** |
| `18 03` | `18` → (3<<3)\|0 | 3 | 0 | **3** |
| `21 …` | `21` → (4<<3)\|1 | 4 | 1 (fixed64 LE) | bytes reversed `3F F8 00…` = 0x3FF8000000000000 = **1.5** |
Total 4 + 3 + 2 + 9 = **18 bytes**. JSON `{"call_id":"c1","packets":300,"lost":3,"jitter_ms":1.5}` = 55 bytes (field names repeated every message; decimal text). 100 calls × 5/s = 500 msgs/s: 9 kB/s vs 27.5 kB/s payload before framing — and no text parsing.

**Q2.**
| n | bytes |
|---|-------|
| 1 | `01` |
| 127 | `7f` |
| 128 | `80 01` |
| 300 | `ac 02` |
| 16 384 | `80 80 01` |
| 2³² | `80 80 80 80 10` |
16 384 = 2¹⁴ = bit 14 set. 7-bit groups from the right: `[000 0001][000 0000][000 0000]` (bits 14–20, 7–13, 0–6). Emit low first: `0000000|1 → 0x80`, `0x80`, last group `0x01` (no MSB). `e8 07`: 0x68 = 104; 7 << 7 = 896 → **1000**. 2³²: bit 32 lies in group 4 (bits 28–34) as value 2⁴ = 0x10.

**Q3.** 64 bits / 7 = 9.14 → 10 bytes; the 10th carries only bit 63, so it must be 0 or 1 (`0x01` terminal). With an 11-byte varint the shift passes 63 → undefined shift / wrap, or an infinite loop if the parser never stops on `0x80…` input. A 10th byte of `0x7F` sets bits 64–69 that do not exist: overflow is silently dropped → two different encodings decode to the same value (canonicalisation/ambiguity bug, useful for smuggling past validators). A continuation bit on the last byte of the buffer = *truncated* → return error / need more data (never read past the end). Test: `Varint.DecodeRejectsMalformed`.

**Q4.** zz(−1)=1, zz(−2)=3, zz(63)=126, zz(−64)=127, zz(2147483647)=4294967294 (0xFFFFFFFE), zz(−97)=193. As `int32`, −97 is sign-extended to 64 bits → **10 bytes**; as `sint32` → `c1 01` = 2 bytes. Note 63 and −64 are the extreme 1-byte values (zz ≤ 127). `sint` for RSSI (dBm), timing offset (ppm), frequency error (Hz), power-control steps; plain `uint32` for counters, ids.

**Q5.**
| Field | tag value | bytes |
|-------|-----------|-------|
| 1 varint | 1<<3\|0 = 8 | `08` |
| 2 string | 2<<3\|2 = 18 | `12` |
| 4 double | 4<<3\|1 = 33 | `21` |
| 16 string | 16<<3\|2 = 130 | `82 01` |
| 2047 varint | 2047<<3 = 16 376 | `f8 7f` |
Fields ≥ 16 pay an extra byte *per occurrence*. Give the 15 most frequent/hot fields (call_id, counters, timestamps) numbers 1–15; rare/optional ones 16+. Never renumber later.

**Q6.** (a) Old server doesn't know field 4 but knows its wire type (varint) → skips it (proto3 ≥ 3.5 keeps it as an *unknown field* and re-serialises it). (b) Absent → default **0**; indistinguishable from "explicitly 0" unless declared `optional`/has-bit — so choose a default that is safe ("0 = normal priority"). (c) An old client still sends field 4 as a *varint*; the new server expects wire type 2 → type mismatch: treated as unknown or parse error, silently losing/corrupting data. `reserved 4; reserved "priority";` makes `protoc` refuse to reuse it. Test: `Messages.DecodeSkipsUnknownFieldsAndDefaultsMissing`.

**Q7.** Protobuf: 18 + 5 + 9 = **32 B** × 500/s = 16 000 B/s = **128 kbit/s**. JSON: 55 + 5 + 9 = 69 B × 500 = 34 500 B/s = **276 kbit/s** (2.2×). (HPACK makes headers negligible after the first call; TCP/IP overhead adds ≈ 40–52 B per segment, equal for both unless messages share segments.) Practical lesson: both are small on a LAN, but the encode/parse CPU and allocations matter at scale, and a console should coalesce (stream one `WatchStats` rather than 100 polls).

**Q8.** `12` = field 2, wire type 2. Length varint `ff ff ff ff 0f` = 0xFFFFFFFF (≈ 4 GiB). Only **2** payload bytes remain (`41 42`). Naive: `resize(len)` → 4 GiB allocation (DoS) or `memcpy` past the buffer (memory disclosure). Correct: `len > remaining` → fail *before* allocating; also check `pos + len` for overflow on 32-bit size_t; plus global message limit. Test: `Proto.ReaderRejectsMalformed`.

**Q9.** Server-streaming `WatchStats` (stream 1):
```text
C→S HEADERS(1, END_HEADERS)               :path /…/WatchStats, grpc-timeout …
C→S DATA(1, END_STREAM)                   00 | len | WatchRequest        (client half-closed)
S→C HEADERS(1, END_HEADERS)               :status 200, content-type application/grpc
S→C DATA(1) ×3                            00 | len | CallStats   (each may be a separate frame)
S→C HEADERS(1, END_HEADERS|END_STREAM)    grpc-status: 0   (trailers)
```
END_STREAM appears on the client's DATA (end of request) and on the server's trailers; `grpc-status` only in the trailers. Cancel after message 1: client sends `RST_STREAM(1, CANCEL=0x8)`; no trailers follow; the server's write fails / `IsCancelled()` becomes true → it must remove the subscription (test `CallControl.ClientCancelRemovesSubscription`); the client app sees `CANCELLED`.

**Q10.** First: length 10, type 1 = HEADERS, flags 0x04 = END_HEADERS, R=0, stream 1. Second: header `00 00 04 | 08 | 00 | 00 00 00 00` = length 4, WINDOW_UPDATE, no flags, **stream 0 = connection-level**; payload `00 00 ff ff` = increment 65 535 → connection window grows by 65 535 (e.g. 65 535 → 131 070).

**Q11.** Throughput ≤ window/RTT = 65 535 B / 0.1 s = 655 350 B/s ≈ **5.24 Mbit/s**, regardless of link speed (pack 02 BDP: window must be ≥ bandwidth × RTT; 100 Mbit/s × 100 ms = 1.25 MB). Connection-level window caps the *sum* of all streams; so does the stream window per stream. Fix: larger SETTINGS_INITIAL_WINDOW_SIZE / eager WINDOW_UPDATEs (gRPC does BDP-estimation dynamic windows ⚠ VERIFY), and receivers must consume data promptly.

**Q12.** All streams share one TCP connection: a lost segment stalls the *entire* connection until retransmission (RTO or fast retransmit) → `StartCall` waits behind the bulk stream's hole: HOL at the transport layer. Mitigations: (1) separate channel (TCP connection) for bulk vs control; (2) rate-limit/chunk bulk and use HTTP/2 flow control; (3) DSCP-mark control traffic, shape bulk; (4) deadlines+hedging for control calls; (5) HTTP/3.

**Q13.**
| | Type | Proto | Retry? |
|-|------|-------|--------|
| a | unary | `rpc ListCalls(ListCallsRequest) returns (CallTable);` | yes (read-only) |
| b | server streaming | `rpc WatchQuality(QualityFilter) returns (stream QualityEvent);` | yes — re-subscribe, resume token/state snapshot first |
| c | client streaming | `rpc UploadClip(stream ClipChunk) returns (UploadSummary);` | only if chunks carry offset/hash (idempotent), else restart |
| d | bidi | `rpc FloorChannel(stream FloorEvent) returns (stream FloorEvent);` | reconnect and resync state; events not blindly replayed |
| e | unary | `rpc Check(HealthRequest) returns (HealthResponse);` | yes |

**Q14.** Console → gateway: `grpc-timeout: 2S` (2 000 ms). Gateway: 2000 − 300 (own work) − 100 (margin) = **1600 ms** → format: 1.6e9 ns is not a whole number of seconds; exact in ms with 4 digits → `1600m`. Core spends 200 ms: 1400 left; DB gets half = **700 ms** → `700m`. (Also each hop may subtract its own margin; you should say so.) `parse("1500m")` = 1 500 000 000 ns = 1.5 s; `parse("10S")` = 10 s. If remaining reaches 0 → don't call; return `DEADLINE_EXCEEDED` immediately.

**Q15.**
| Case | Status | Retry |
|------|--------|-------|
| empty `call_id` | `INVALID_ARGUMENT` | no (fix request) |
| duplicate `call_id` | `ALREADY_EXISTS` | no — means "already done" (idempotency) |
| unknown call on `EndCall` | `NOT_FOUND` (or OK if you define EndCall idempotent) | no |
| talkgroup table full | `RESOURCE_EXHAUSTED` | later, with backoff |
| deadline passed | `DEADLINE_EXCEEDED` | only if idempotent |
| authenticated, no rights | `PERMISSION_DENIED` | no |
| missing/invalid credentials | `UNAUTHENTICATED` | after re-auth |
| method not implemented | `UNIMPLEMENTED` | no |
| core restarting | `UNAVAILABLE` | **yes**, backoff+jitter (idempotent calls) |
(`PERMISSION_DENIED` ≠ `UNAUTHENTICATED`; `FAILED_PRECONDITION` = system state wrong, e.g. ending a call that isn't active.)

**Q16.** A sink is arbitrary client/adapter code. If invoked while holding the service mutex: (1) a sink that calls `unsubscribe()`/`end_call()`/`start_call()` (e.g. on seeing the final message it cancels itself) re-locks a non-recursive mutex → **self-deadlock**; (2) a slow sink (network write blocked by flow control) stalls *every* other call and the media path; (3) two services with sinks calling each other → lock-order inversion. Fix: copy the subscriber list under the lock, release, then call sinks (trade-off: a sink might get one more callback after unsubscribing — handle idempotently). Test: `CallControl.SinkMayCallBackIntoService`.

**Q17.** The header must stay readable for: (1) **receivers/jitter buffers/RTCP** — sequence number and timestamp to reorder/play out and compute loss/jitter; (2) **middleboxes** — SBC/media gateways/translators demux by SSRC and look at PT, and cellular **header compression (RoHC)** needs plain fields; QoS probes and monitoring (loss/jitter MOS) too; (3) **SRTP itself** needs `SEQ` and SSRC in the clear to build the IV and ROC estimate. Authenticating the header (HMAC covers it) still stops tampering; confidentiality of header fields isn't needed for the call content. Cost: traffic analysis (who, when, codec).

**Q18.** Base 200 B IP packet (12 RTP + 160 + 8 UDP + 20 IP).
| Mode | Packet | Rate (×50×8) |
|------|--------|--------------|
| plain RTP | 200 B | 80.0 kbit/s |
| SRTP, 80-bit tag (10 B) | 210 B | **84.0** kbit/s (+5 %) |
| SRTP, 32-bit tag (4 B) | 204 B | **81.6** kbit/s (+2 %) — weaker forgery resistance (2⁻³² per guess) |
| AES-GCM, 16 B tag | 216 B | **86.4** kbit/s (+8 %); no separate HMAC, single pass ⚠ RFC 7714 (an 8 B tag profile exists) |
For 200 calls: 200 × 4 kbit/s = 0.8 Mbit/s extra — negligible on a LAN, noticeable on a 64 kbit/s-class radio backhaul.

**Q19.** Case 1: `s_l = 0x0010 < 0x8000`, `seq − s_l = 0xFFE0 > 0x8000` → v = ROC − 1 = 2 → index = 2·65 536 + 0xFFF0 = **0x2FFF0** (a *late* packet from before the wrap). Case 2: `s_l = 0xFFF0 ≥ 0x8000`, `s_l − 0x8000 = 0x7FF0 > 5` → v = ROC + 1 = 4 → index = 4·65 536 + 5 = **0x40005**. `update()` before authentication: an attacker sends forged packets with chosen `seq` and the receiver would move `s_l`/ROC — future *genuine* packets are then mis-indexed → auth failures forever (DoS), and replay window poisoned. 15 min × 60 × 50 = 45 000 packets > 32 768 → the estimate is off by one ROC: IV mismatch → every packet fails authentication; recovery needs re-keying/resync (RFC 3711 §3.3.1 says implementations must trigger/accept resync; ⚠ VERIFY). Tests: `Srtp.IndexRolloverEstimation`, `Srtp.IndexHighSeqCases`.

**Q20.** Window of 64, anchored on the highest index:
| Arrive | Result | Why |
|--------|--------|-----|
| 100 | accept | first |
| 101 | accept | new highest |
| 99 | accept | 2 behind, unseen |
| 101 | **reject** | duplicate |
| 37 | **reject** | 64 behind → outside |
| 200 | accept | new highest; window slides (mask shifted by 99 ≥ 64 → cleared) |
| 150 | accept | 50 behind, unseen |
| 136 | **reject** | 64 behind 200 → too old (137 would be OK) |
Test: `Srtp.ReplayWindow`.

**Q21.**
```text
Alice              SIP/SDP via proxy (TLS)                Bob
INVITE  SDP: m=audio … UDP/TLS/RTP/SAVP, a=fingerprint:sha-256 AB:CD…, a=setup:actpass  →
        ← 200 OK  SDP: a=fingerprint:sha-256 12:34…, a=setup:active
ACK
--- media path (UDP) ---
Bob → Alice  DTLS ClientHello
Alice → Bob  HelloVerifyRequest (cookie)        (stateless anti-spoof)
Bob → Alice  ClientHello+cookie
Alice → Bob  ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
Bob → Alice  Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Finished
Alice → Bob  ChangeCipherSpec, Finished
        both: export keying material (RFC 5705, label "EXTRACTOR-dtls_srtp") → SRTP master key/salt per direction
SRTP media flows
```
The certificates are self-signed; **the fingerprint in SDP, carried over authenticated signalling (SIPS/TLS + identity), binds the DTLS peer** — otherwise a man-in-the-middle on the media path could run its own DTLS to both sides. Clipping: media can't be protected until the handshake completes (≥ 1 RTT, 2 with HelloVerify, more with a retransmission timer ≈ 1 s); audio sent early is dropped → the first syllable of the first talk-burst is lost. ⚠ VERIFY message flight details per DTLS version (1.2 vs 1.3).

**Q22.** Options: (1) **Opaque end-to-end transport** — carry the encrypted vocoder frames as an opaque payload (a custom payload type, or `application/octet-stream`-like); the gateway never has the key; only endpoints with the same vocoder + key can decode. Strong trust, but the MCPTT client can't play it unless it has the LMR vocoder and key, and the SRTP layer on top protects only the IP leg. (2) **Decrypt and re-encrypt** — gateway holds LMR keys (via an over-the-air-rekeying/KMF interface), decrypts, optionally transcodes IMBE→AMR-WB/Opus, then SRTP-protects towards LTE. Interoperable but the gateway becomes a *plaintext point* (a high-value target: HSM, key custody, audit, accreditation). (3) **No interworking** for encrypted talkgroups (fail closed) — safest, least capable. Verify with customer: legal basis/accreditation, who controls keys (KMF), algorithm and key IDs per system, whether policy allows plaintext at the gateway, key-update timing, logging/recording requirements. This is design discussion — not a standards statement.

---

## Self-check of the code
* Varint 300 → `ac 02`; negative int32 costs 10 bytes; zigzag(−97)=193.
* `StartCallRequest{c1,7,gw}` → `0a 02 63 31 10 07 1a 02 67 77`.
* `grpc_frame("abc")` → `00 00 00 00 03 61 62 63`.
* `format_grpc_timeout(1.5 s)` → `1500m`; `123 456 789 ns` → `123457u`.
* All 29 tests pass for `solutions/` under ASan/UBSan (g++, Linux/WSL); the student stubs fail by design.
