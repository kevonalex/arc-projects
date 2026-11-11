# Answers — 03-net / 06 Cellular Encodings

Attempt first. ⚠ = verify against the 3GPP/IETF/ITU text for your release.

---

**Q1.** LTE MCPTT voice, uplink:
```text
UE:   IP(UE addr → MCX/gateway addr) / UDP / RTP / AMR-WB or Opus payload      ← the user packet
 │ radio: PDCP/RLC/MAC/PHY (header-compressed with ROHC ⚠, encrypted); RRC configures it (UPER)
eNB:  strips radio layers, wraps the user packet:  outer IP(eNB → SGW) / UDP 2152 / GTP-U(TEID_sgw)
SGW:  re-tunnels on S5/S8: outer IP(SGW → PGW) / UDP 2152 / GTP-U(TEID_pgw)       (may be collapsed if SGW/PGW are co-located)
PGW:  removes the tunnel → plain IP(UE addr → gateway) on SGi
Gateway (Etherstack): sees the ORIGINAL UE address, RTP etc. — no tunnel.
```
Wireshark on S1-U/S5 sees *two* IP layers (outer infrastructure, inner UE). On SGi only the inner one remains (the UE's address, usually NATed or private). On the radio side the packet is not IP at all.

**Q2.** `34` = `001 1 0 1 0 0` → version 1, PT 1, spare 0, **E=1**, S=0, PN=0. Type `ff` = G-PDU. `0x2c` = 44 = bytes after the first 8: 4 (optional part) + 4 (one extension) + **36 inner**. TEID `0x0A0B0C0D`. Optional part `00 00 00 85`: seq 0, N-PDU 0, next-ext = 0x85. Extension `01 00 09 00`: length 1 (×4 = 4 bytes), content `00 09`, next = 0 (end). QFI = `0x09 & 0x3F` = **9**. Header length 8 + 4 + 4 = **16**. Inner IP packet 36 bytes. (Verified by running the solution: `hl=16 payload=36 qfi=9`.)

**Q3.** TEID is chosen by the *receiving* end ("use this label when you send to me") so the receiver can allocate it from its own table with no coordination. Each direction therefore has an independent label: uplink TEID allocated by the SGW/UPF, downlink TEID allocated by the eNB/gNB. Setup: the core's *Create Session Response* (GTP-C) / SMF's N2 message (5G) tells the RAN the core-side TEID and address; the RAN's *Initial Context Setup Response* / N2 response returns its own TEID and address; the core then (Modify Bearer) tells the SGW/UPF the downlink TEID. Unknown TEID: the receiver drops it and (per TS 29.281) returns **Error Indication** (type 0x1A) to the source with the TEID/address in it, which tells the sender the tunnel is gone (e.g. after a restart). GTP firewalls use a table of valid TEIDs to drop injected packets; Error Indication floods are themselves rate-limited ⚠.

**Q4.** (a) 1500 − 36 = **1464**; with S flag (+4) and one 4-byte extension: 1500 − 20 − 8 − 8 − 4 − 4 = **1456**; 9000 − 36 = **8964**. (b) Tunnelled packet = 200 + 36 = **236 B**; 236 × 8 × 50 = **94.4 kbit/s** per call per direction (vs 80 kbit/s inner IP: +18 %). 1 000 calls: 94.4 Mbit/s vs 80 → 14.4 Mbit/s of pure tunnel overhead (add Ethernet 18 B + preamble/IFG 20 B → 236 + 38 = 274 B → 109.6 Mbit/s on the wire). Small packets make tunnel overhead proportionally larger — a reason voice stays at 20 ms packets, not 10 ms.

**Q5.** Echo is *path management* between two nodes, not about a user flow, so there's no tunnel: TEID = 0. The sequence number matches a response to its request (Echo Response copies it), hence the S flag. A G-PDU belongs to a flow where order is the IP layer's business; adding 4 bytes per packet would waste bandwidth. Echo Request with S set, no payload: length counts everything after the first 8 bytes = the 4 optional bytes = **4** (test `Gtpu.EchoRequestWithSequence`; packet `32 01 00 04 00000000 002A 00 00`).

**Q6.** Checks: ≥ 8 bytes before reading any field (OOB read); version == 1 and PT == 1 (don't parse GTP′ as GTP); `8 + length ≤ datagram` (never trust length; reading beyond → leak/crash); if any E/S/PN flag then ≥ 4 optional bytes exist inside `length`; every extension length ≥ 1 (0 would loop forever: `pos += 0`) and `pos + len×4 ≤ end`; a bound on chained extensions (an attacker can make a long chain; each is individually legal); ignore trailing bytes beyond `length` (not part of the packet); copy only `length` bytes. Also at policy level: TEID known? source address allowed? rate-limit Error Indications. Tests: `Gtpu.RejectsMalformed`, `Gtpu.TrailingBytesBeyondLengthAreIgnored`.

**Q7.** Per packet: **TEID** (matches what the lab's fake control plane told the gateway), outer IP/UDP 2152 (src/dst ports often both 2152), outer **DSCP** per the QCI/5QI→DSCP policy under test, optional S flag + sequence, optional PDU session container with QFI; inner packet: IPv4/UDP/RTP built from your own `RtpPacketiser` with synthetic audio (sine/test tones, `04-audio/04`), RTP timestamps +160 per 20 ms, sequence +1, pacing at exactly 20 ms. Validate: TEID→call mapping, QFI→DSCP mapping on egress, inner-MTU handling (send 1464/1465 B inner), reordering around End Marker, Error Indication on unknown TEID, no crash on malformed fuzz packets (Q6). **Why no captured live traffic:** legal (interception of public-safety/operator communications is an offence under the Wireless Telegraphy Act 2006 / Investigatory Powers Act — ⚠ legal advice), data protection (voice content, subscriber identifiers), and repeatability (synthetic data is reproducible and shareable).

**Q8.** `01` version 1; `00 00 6c` length **108** (multiple of 4 ✓); `c0` = R|P (request, proxiable); `00 01 10` = 272 = **Credit-Control-Request**; app `00 00 00 04` = **Credit-Control** (Gy/Ro); hop-by-hop `0x4a6b2c01`; end-to-end `0x91d3a502`; AVP bytes = 108 − 20 = **88**. (Verified with the parser.)

**Q9.** Code `00 00 01 08` = 264 **Origin-Host**; flags `40` = M only (no V, P); length `00 00 12` = **18** = 8 header + 10 data; data `67 77 2e 65 78 61 6d 70 6c 65` = **"gw.example"**; padding = 2 bytes (`00 00`) so the AVP occupies 20 bytes; next AVP starts at offset **20**. Vendor-Id (266) with M: `00 00 01 0A | 40 | 00 00 0C | 00 00 28 AF` (10415 = 0x28AF; length 12 = 8 + 4; no padding needed).

**Q10.** Parser A advances by the unpadded length: with `length = 18` it lands 2 bytes before the next AVP → interprets the padding zeros + first two bytes of the next AVP as a header → garbage code/length, misparsed rest of message (or accepts attacker-chosen bytes inside "padding"). Parser B with `len = 0`: `pos += 0` → **infinite loop** (CPU DoS from a single 8-byte AVP); also `len < 8` underflow when computing `len − 8` (huge unsigned size → heap overread). Checks: `n − pos ≥ 8`; `len ≥ header size (8 or 12)`; `len ≤ n − pos`; advance by `(len + 3) & ~3` and require it ≤ `n − pos`; limit grouped-AVP nesting and total AVP count. Test `Diameter.AvpRejects`.

**Q11.**
```text
PCEF ──req(hbh=A, e2e=E)──► Relay ──req(hbh=B, e2e=E)──► PCRF
PCEF ◄─ans(hbh=A, e2e=E)──  Relay ◄─ans(hbh=B, e2e=E)──  PCRF
```
The relay rewrites **Hop-by-Hop** on the way out (B, fresh per its link to the PCRF), remembers A, and restores it on the answer. **End-to-End** is generated by the originator and is *never* changed; the pair (Origin-Host, End-to-End ID) uniquely identifies a transaction. After a link failure the PCEF resends the request with the **T** (potentially retransmitted) flag on (same e2e) — the server compares (Origin-Host, e2e) against its recent-request cache to avoid performing e.g. a charging deduction twice.

**Q12.** CER (all in the base application, code 257, R|P flags, app-id 0): `Origin-Host` (264, M, UTF8String `"gw.example.org"`), `Origin-Realm` (296, M), `Host-IP-Address` (257, M, Address), `Vendor-Id` (266, M, 10415 or the vendor's IANA enterprise number), `Product-Name` (269, no M, `"etherstack-gw"`), `Vendor-Specific-Application-Id` (260, M, grouped: `Vendor-Id` + `Auth-Application-Id` 16777236 for Rx), optionally `Firmware-Revision`. M for everything mandated by the base protocol; Product-Name is informational (M clear). If the peer answers **5010 DIAMETER_NO_COMMON_APPLICATION**, no application both support: close the transport, log the peer's advertised applications, don't retry in a tight loop (check your Vendor-Specific-Application-Id vs the peer's). Test `Diameter.BuildAndParseCer`.

**Q13.**
| | Diameter | 5G SBI (HTTP/2) |
|---|----------|-----------------|
| Transport | TCP/SCTP 3868 (+TLS/DTLS) | HTTP/2 over TLS (mTLS) |
| Model | persistent **peer** connections; request/answer with app-ids | client/server resources + subscriptions/notifications (callbacks) |
| Routing/discovery | realm-based, static peers/relays/DRAs | **NRF discovery**, SCP/service mesh, load balancers |
| Extensibility | vendor AVPs, M-bit "must understand", new apps | OpenAPI schemas, optional JSON fields, versioned URIs; "critical" features by negotiation ⚠ |
| Errors | `Result-Code`/`Experimental-Result` | HTTP status + `ProblemDetails` |
| Security | TLS/IPsec, filtering at borders | TLS + OAuth2 access tokens (NRF-issued) |
| Debug | Wireshark `diameter` easy | needs TLS keys (SSLKEYLOGFILE) in a lab; JSON readable once decrypted |
3GPP moved to HTTP/2 + JSON to ride web/cloud tooling (service meshes, load balancers, telemetry, language-neutral codegen), to model the core as *services* that can be scaled independently, and to avoid maintaining a bespoke transport/AVP dictionary ecosystem. Cost: bigger messages, TCP HOL, and less compact semantics. **Debug:** Diameter is easier on a plain capture (unencrypted TCP/SCTP in a lab); SBI needs session keys.

**Q14.** (0..7) **3**; (0..255) **8**; (0..256) range 257 → **9**; (1..100) range 100 → **7**; (−128..127) range 256 → **8**; (0..65535) **16**; (0..4294967295) **32**; (5..5) **0**; BOOLEAN **1**; ENUMERATED{a..e} 5 values → **3**; OCTET STRING(SIZE(4)) fixed → no length, **32** bits; OCTET STRING(SIZE(1..8)) with 3 octets → length−1 = 2 in 3 bits + 24 data = **27** bits (not byte-aligned in UPER). Test `Uper.ConstrainedIntegerWidths`.

**Q15.** (a) `a=3,b=false,c absent,d=x`: preamble `0` · `011` · `0` · d=0 → `00` ⇒ `0 011 0 00` = 7 bits → pad → `0011 0000` = **`30`**. (b) `a=7,b=true,c=1999,d=z`: preamble `1` · `111` · `1` · c−1000 = 999 = `1111100111` · d=2 → `10` ⇒ `1 111 1 1111100111 10` (17 bits) = `1111 1111  1100 1111  0` → **`FF CF 00`**. Decode `B4 80 80` = `1011 0100  1000 0000  1000 0000`: preamble `1` (c present); a = `011` = 3; b = `0` = FALSE; c = next 10 bits `1001000000` = 576 → 1576; d = `01` = 1 (y); remaining 7 bits are padding. (All verified by running the solution.)

**Q16.** `ceil(log2 1000)` = 10 bits can hold 1024 values, only 1000 legal: patterns for 1000–1023 are *unused code points* — a corrupted bit flip or hostile sender can produce them; accepting leads to values outside the declared constraint, array indices out of range, switch default fall-through, etc. So decode = read bits, **then** check `offset < range`. Five attack/corruption patterns: (1) illegal enum/choice index (acts as array index); (2) length determinant larger than remaining bits → pre-allocation DoS / OOB read; (3) deep recursion through nested CHOICE/SEQUENCE OF (stack exhaustion) → depth limit; (4) enormous `SEQUENCE OF` element counts (memory) → cap by remaining bits (each element needs ≥ 1 bit or the type’s min size); (5) truncated input — every `get(n)` must fail cleanly. `BitReader::get` returning `nullopt` *without consuming* makes (2) and (5) one check each. Fuzz with synthetic/corrupted self-generated messages (and coverage-guided tools), not live captures.

**Q17.** Short form: `n < 128` → `0` + 7 bits: 127 = `0 1111111` = `7F`. 200 ≥ 128 → long form `10` + 14 bits: `10 000000 11001000` = `80 C8`. 16384 would need the fragmentation form `11` + 6-bit multiplier of 16K blocks, which is not implemented. The leading `0` / `10` / `11` bits make the length *prefix-free* (self-delimiting): the first bits say how many more bits to read. Using a fixed 16 bits for everything would cost 8 bits more on the (overwhelmingly common) short lengths. Test `Uper.LengthDeterminant`.

**Q18.** Arguments for PER: (1) **air-interface capacity**: control channels carry a few hundred bits; schema-driven bit-packing is ~3–10× denser than TLV/JSON. (2) **Formal, machine-checked schema** with version/extension markers (`...`) → backward compatibility across 3GPP releases; the spec *is* the ASN.1. (3) **Multi-vendor interoperability**: eNB/UE/core from different vendors need a precise unambiguous bit-level spec — compilers (asn1c, vendor tools) generate matching codecs. (4) **Longevity**: ASN.1 predates, and was already the telecom tool (BER in 2G/3G, PER on air since 3G/LTE). (5) **UE cost**: bit-fiddling is cheap in silicon; no string parsing. Downsides: tool dependency; non-self-describing (cannot decode or even skip without the exact schema version → logs useless without schema); unforgiving to bit errors; historically a rich source of decoder memory-safety bugs (generated C decoders) → fuzzing, bounds limits, and memory-safe decoders are essential.

**Q19.** Protobuf (`packets=300` → `10 ac 02`, `lost=3` → `18 03`) = **5 bytes (40 bits)**. UPER with (0..65535) + (0..255) = 16 + 8 = **24 bits = 3 bytes** (no preamble since nothing is optional and the type is not extensible). So PER is ≈ 40 % smaller here. But PER bakes the range into the format: if `lost` grows beyond 255 you must change the schema and *both* sides' code in lock step (or use `...` extension), whereas protobuf's varint absorbs it with no schema change. Choose PER where bits are the scarcest resource and the schema is centrally governed (the air interface); protobuf where services evolve independently and bytes are cheap (gateway control APIs).

**Q20.**
| Interface | Transport | Encoding | Carries |
|-----------|-----------|----------|---------|
| N1 | via N2/radio | NAS (binary TLV) | UE ↔ AMF mobility/session NAS ⚠ |
| N2 | SCTP | NGAP, APER | gNB ↔ AMF: UE contexts, handover |
| N3 | UDP 2152 | GTP-U | user data gNB ↔ UPF |
| N4 | UDP 8805 | PFCP | SMF → UPF forwarding rules |
| N5 | HTTP/2 | JSON (SBI) | AF/MCX server ↔ PCF: policy/QoS request |
| N6 | IP | plain IP | UPF ↔ data network (Etherstack side) |
| N11 | HTTP/2 | JSON | AMF ↔ SMF session management |
| N8/N10 | HTTP/2 | JSON | AMF/SMF ↔ UDM subscriber data |
| Rx | Diameter | AVPs | AF ↔ PCRF policy (LTE) |
| S6a | Diameter | AVPs | MME ↔ HSS subscriber data |
| S1-U | UDP 2152 | GTP-U | eNB ↔ SGW user data |
| S11 | UDP 2123 | GTPv2-C | MME ↔ SGW bearers ⚠ |
An MCX server / the gateway directly speaks **N5/Rx** (policy), **N6/SGi** (media: RTP/SIP) and signalling toward the MCX application. N1–N4, N8/N10/N11, S6a, S1-U, S11 are inside the operator network — the gateway only experiences their *effects* (bearer/QoS, latency, MTU, IP address changes).

**Q21.** LTE: MCPTT server needs QoS → **Rx AA-Request** (AF → PCRF) with the media description (bandwidth, flow 5-tuple) → PCRF decides a policy rule → **Gx RAR** to the PGW (PCEF) → PGW sends *Create Bearer Request* (GTP-C) over S5 → SGW over S11 → MME → S1AP *E-RAB Setup Request* to the eNB (QCI 65 ⚠) → RRC reconfiguration to the UE; **GTP-U TEIDs** for the new bearer are allocated at the eNB/SGW/PGW during these exchanges; the **outer DSCP** for transport is chosen by the node that builds the outer header (eNB/SGW/PGW) from the QCI→DSCP policy. 5G: MCX AS → **NEF/PCF Npcf_PolicyAuthorization** (HTTP/2, N5) → PCF generates PCC rules → **SMF** (Npcf_SMPolicyControl notify) → **N4 (PFCP)** to the UPF and **N2 (NGAP) via AMF** to the gNB with a new **QoS flow (QFI, 5QI 65 ⚠)** → gNB radio bearer; **GTP-U N3** keeps the *same tunnel* (PDU session) for several QFIs, with QFI in the PDU session container extension header. DSCP on N3's outer header comes from the 5QI/QFI mapping at the gNB/UPF. ⚠ All flows summarised; check TS 23.401/23.501/29.214/29.514.

**Q22.** (a) UE has IP but no uplink voice: GTP-U TEID/address mismatch (uplink TEID wrong, sent to stale tunnel) or a firewall dropping UDP 2152 → look for **Error Indication** (GTP type 0x1A) in the capture (`gtp.message == 0x1a`), check the control-plane messages that allocated TEIDs; also MTU/fragmentation. (b) `DIAMETER_UNABLE_TO_COMPLY` (5012 ⚠) on Rx: request understood but server can't act — check the AVPs (media components, flow description, QoS values), PCRF logs, `diameter.Result-Code`/`Experimental-Result-Code`. (c) HTTP 403 from an NF: access token missing/expired/wrong scope or the NF isn't authorised for that service → check OAuth token from the NRF, mTLS identity; JSON `ProblemDetails.cause`. (d) NGAP decode error after an upgrade: schema/release mismatch between vendors (extension IEs, criticality), or a broken ASN.1 build → decode with the *matching* spec version in Wireshark (`ngap`), check criticality handling. (e) Peer drops after 30 s idle: watchdog (DWR/DWA) not answered, or an intermediate firewall/NAT idle-timeout → capture shows TCP FIN/RST; configure Tw timer and keepalives.

---

## Self-check of the code
* `34ff002c0a0b0c0d000000850100…` → header_len 16, payload 36, QFI 9 (run).
* UPER examples: `5A`, `D9 D5 00`, `30`, `FF CF 00`, `B4 80 80` → (3, false, 1576, 1) (run).
* `ceil_log2(256)=8`, `(257)=9`; `INT64_MIN..INT64_MAX` = 64 bits.
* 26 tests pass on `solutions/` under ASan/UBSan (g++, WSL); the stubs compile and fail by design (24 failures).
