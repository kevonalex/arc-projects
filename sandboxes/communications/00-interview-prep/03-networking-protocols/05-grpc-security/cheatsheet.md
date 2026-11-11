# Cheatsheet — gRPC, Protobuf, HTTP/2, SRTP/DTLS

## Protobuf wire format
| Item | Rule |
|------|------|
| varint | 7 bits/byte, LSB group first, MSB=continuation; max 10 B for 64 bit; 300 = `ac 02` |
| tag | varint of `(field << 3) \| wire_type`; fields 1–15 = 1 byte |
| wire types | 0 varint · 1 fixed64 (LE, double) · 2 len-delimited (string/bytes/message/packed) · 5 fixed32 (LE, float) |
| negative `int32/64` | 10 bytes (sign-extended) → use `sint*` (ZigZag: `(n<<1) ^ (n>>63)`) |
| defaults | proto3 doesn't send 0/""/false; absent ≠ 0 unless `optional` |
| evolution | add fields freely; never reuse numbers (`reserved`); unknown fields skipped; last scalar wins |
| safety | check `len ≤ remaining` *before* allocating; cap message size (4 MiB default ⚠); cap nesting depth |

Example `CallStats{c1,300,3,1.5}` = `0a 02 63 31 10 ac 02 18 03 21 00 00 00 00 00 00 f8 3f` (18 B vs 55 B JSON).

## HTTP/2
* Frame header 9 B: `len(24) type(8) flags(8) R(1)+stream(31)`; types 0 DATA, 1 HEADERS, 3 RST_STREAM, 4 SETTINGS, 6 PING, 7 GOAWAY, 8 WINDOW_UPDATE, 9 CONTINUATION. Flags END_STREAM 0x1, END_HEADERS 0x4, ACK 0x1.
* Client preface `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n` (24 B). Client streams odd ids.
* Multiplexed streams, HPACK, flow control per stream + connection (initial 65 535 ⚠). HOL is *removed at HTTP layer only* — TCP loss stalls all streams.

## gRPC
* Message prefix: `[compressed 1B][length 4B BE][protobuf]`. One RPC = one HTTP/2 stream.
* Request headers: `:method POST`, `:path /pkg.Service/Method`, `content-type: application/grpc`, `te: trailers`, `grpc-timeout`.
* Response: HEADERS(200) → DATA… → trailers `grpc-status`, `grpc-message`.
* Types: **unary** · **server-stream** (`WatchStats`) · **client-stream** (upload) · **bidi** (floor events).
* `grpc-timeout`: `<≤8 digits><H M S m u n>` — *relative*; each hop recomputes `remaining = deadline − now − margin`; ≤0 → fail fast.
* Always set deadlines. Retry only idempotent + `UNAVAILABLE`, backoff + jitter; use client-generated ids for idempotency.
* Status: 0 OK · 1 CANCELLED · 3 INVALID_ARGUMENT · 4 DEADLINE_EXCEEDED · 5 NOT_FOUND · 6 ALREADY_EXISTS · 7 PERMISSION_DENIED · 8 RESOURCE_EXHAUSTED · 9 FAILED_PRECONDITION · 12 UNIMPLEMENTED · 13 INTERNAL · 14 UNAVAILABLE · 16 UNAUTHENTICATED.
* Don't call sinks/callbacks under a lock.

## SRTP / DTLS
* Payload encrypted (AES-CTR or GCM), header in clear, whole packet authenticated (HMAC-SHA1, 80 or 32-bit tag; GCM 16 B).
* Index `i = 2^16·ROC + SEQ`; ROC implicit; estimate per RFC 3711 App. A; desync after ≥ 2^15 lost packets.
* Order: check replay → verify tag → **then** update ROC/window → decrypt.
* Overhead G.711 20 ms: 200 → 210 B (84 kbit/s).
* Keys: SDES (SDP `a=crypto`, needs TLS signalling) · **DTLS-SRTP** (fingerprint in SDP; handshake on media path; keys exported) · ZRTP · MIKEY(-SAKKE).
* DTLS = TLS + record epoch/seq, retransmit timers, HelloVerify cookie, fragmentation.
* Misc: headers/timing still visible (traffic analysis); never reuse a keystream.

## Build
`cmake -S code -B build && cmake --build build && ctest --test-dir build`
Real gRPC: see `code/grpc/README.md` (`-DWITH_GRPC=ON`; untested skeleton).
