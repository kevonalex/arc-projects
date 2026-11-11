# 03-net / 06 — Cellular Encodings: GTP-U, Diameter, 5G HTTP/2 SBI, ASN.1 PER  *(Tier 2)*

**Time:** ~3–4 h · **Prereqs:** `01-headers-layering`, `02-udp-tcp-qos`, `05-grpc-security` (HTTP/2) · **Code:** `code/include/cellenc.hpp` (stub), `code/solutions/cellenc.hpp`, `code/tests/test_cellenc.cpp`
**Etherstack lens:** LMR↔LTE/5G interworking (MCPTT/MCX) means the gateway sits *next to* a 3GPP network: its media rides inside **GTP-U** tunnels on the user plane, its QoS is requested over **Diameter (Rx)** or the **5G SBI (N5)**, and the radio-side control messages are **ASN.1 PER** bit-packs it will never parse but must understand. The interview question is "what does each of these carry, and why is it shaped that way?"

> **Legal:** use only synthetic bytes and published test vectors. Do not capture, decode or replay live public-safety or operator traffic (UK Wireless Telegraphy Act 2006; operator licence terms).

---

## Part A — The map: what carries what

```text
            control plane                                   user plane
UE ─RRC(UPER, over the air)─ eNB/gNB ─S1AP / NGAP(APER over SCTP)─ MME / AMF
                                │                                      │ S11 GTP-C / N11 HTTP2-SBI
                                │ GTP-U (UDP 2152)               SGW-C/PGW-C / SMF ─N4 PFCP (UDP 8805)─ UPF
                                └──────────────S1-U / N3─────────────────────────────── SGW-U/PGW-U / UPF ─ SGi/N6 ─ IP (MCX server, Etherstack gateway)
core ↔ core:   LTE: Diameter (S6a HSS, Gx/Rx PCRF, Gy/Ro charging)   5G: HTTP/2+JSON "SBI" between network functions (AMF, SMF, UDM, AUSF, PCF, NRF…)
```
| Protocol | Plane | Transport | Encoding | Carries |
|----------|-------|-----------|----------|---------|
| **GTP-U** | user | UDP 2152 | fixed binary header | each UE's IP packets, tunnelled eNB/gNB ↔ gateway ↔ UPF |
| GTP-C v2 | control (LTE) | UDP 2123 | TLV | bearer/session create/modify (S11, S5/S8) ⚠ |
| **Diameter** | control (LTE core) | TCP/SCTP 3868 (+TLS/DTLS) | header + AVPs | subscriber data (S6a), policy (Gx/Rx), charging (Gy) |
| **5G SBI** | control (5GC) | HTTP/2 over TLS | JSON (OpenAPI) | NF service calls: session mgmt, subscriber data, discovery, policy |
| PFCP | control (5GC N4) | UDP 8805 | TLV | SMF → UPF rules (PDR/FAR/QER) ⚠ |
| **S1AP/NGAP** | control (RAN↔core) | SCTP | **ASN.1 aligned PER** ⚠ | UE context setup, handover, paging |
| **RRC** | control (UE↔RAN) | radio (PDCP/RLC) | **ASN.1 unaligned PER** ⚠ | radio configuration, measurement reports |
(⚠ = verify spec/version.) Etherstack touches the *application* above SGi/N6 (SIP/RTP/MCX), but must reason about everything underneath.

**Q1.** Draw the map from memory for an LTE MCPTT voice packet: UE → eNB → SGW → PGW → MCX server/Etherstack gateway. At each hop name the encapsulation (what header wraps the user's IP packet?). *(Context: where would Wireshark on the SGi side see the original UE address; where would it see tunnels?)*

---

## Part B — GTP-U (3GPP TS 29.281)

### B.1 Why tunnel at all? (derive)
A phone moves between cells but must keep **one IP address** (an RTP flow must survive handover). The IP routers between the base station and the core know *nothing* about UE addresses and must not need to. Solution: wrap the UE's packet as the **payload of a UDP packet between two fixed infrastructure addresses**; routing only ever sees the outer header. Handover = one end of the tunnel changes address; the UE's own packets are untouched. UDP (not TCP) because the inner packet is already an end-to-end IP packet whose own transport (RTP/UDP or TCP) handles reliability — a tunnel that retransmitted would cause TCP-over-TCP meltdown and add voice latency (pack 02).

### B.2 Header, field by field
```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-----+-+-+-+-+-+-+---------------+-------------------------------+
| Ver |P|*|E|S|N|  Message Type   |            Length             |
+-----+-+-+-+-+-+-+---------------+-------------------------------+
|                 Tunnel Endpoint Identifier (TEID)               |
+-----------------------------------------------------------------+
|   Sequence Number (opt, if E|S|PN)   | N-PDU No. | Next Ext Type |
+-----------------------------------------------------------------+
```
| Field | Bits | Why it exists |
|-------|------|---------------|
| Version | 3 | = 1; room to evolve (GTPv2 is a different protocol on the control plane) |
| PT | 1 | 1 = GTP, 0 = GTP′ (charging): same port family, different protocol |
| E / S / PN | 1 each | flags that *announce the 4 optional bytes* — most packets omit them, saving 4 B at every packet |
| Message type | 8 | 0xFF **G-PDU** (a user packet), 1/2 Echo Req/Resp (path liveness — GTP-U has no connection), 0x1A Error Indication (sent when a TEID is unknown), 0xFE End Marker (last packet on the old path after handover so the receiver can reorder) |
| Length | 16 | bytes **after the first 8** (so it *includes* the optional 4 bytes and extensions) |
| **TEID** | 32 | demultiplexing key: which tunnel / bearer / session this packet belongs to. It is **allocated by the receiver** and told to the sender over the control plane, so each direction of a bearer has a *different* TEID. |
| Sequence | 16 | optional ordering (rarely used on the user plane) |
| N-PDU | 8 | legacy (inter-system handover of 2G/3G) |
| Next ext type | 8 | 0 = none; else the first extension header's type |

**Extension headers** (the "E" mechanism): `length(8, in 4-byte units, including length and next-type bytes) | content | next type`. 5G uses **PDU Session Container (type 0x85)** on N3 to carry the **QFI** (QoS Flow Identifier, 6 bits), RQI and PDU type, because in 5G one tunnel (PDU session) carries *several QoS flows* — the TEID alone no longer says which QoS to apply. The upper two bits of an extension type tell nodes whether they must understand it (comprehension rules ⚠ VERIFY).

**Overhead:** outer IPv4 20 + UDP 8 + GTP-U 8 = **36 B** (+4 with S, +extension bytes; +20 more if the outer is IPv6). A 1500 B transport MTU → **1464 B** inner MTU. UEs are told a smaller MTU (via PCO/RA ⚠ operator-dependent) so tunnelled packets are never fragmented.

**Security:** GTP-U has **no authentication or encryption**; anybody who can reach UDP 2152 and guess a TEID can inject packets. Protection = network isolation, ACL/firewall on 2152, IPsec on the backhaul, GTP-aware firewalls (drop unknown TEIDs, check source addresses).

> **Common misconceptions**
> 1. *"The TEID identifies the user."* It identifies a tunnel endpoint (a bearer/PDU session) and is a **local index chosen by the receiver**.
> 2. *"GTP-U is reliable."* It's UDP with optional sequence numbers; Echo only tests the path.
> 3. *"The DSCP on the inner packet is what the transport network honours."* The transport routers only see the **outer** header; the node that builds the outer header decides its DSCP (from QCI/5QI), copy or map it deliberately.
> 4. *"GTP-U carries signalling."* It carries user packets (plus the few path-management messages); signalling is GTP-C/S1AP/NGAP/SBI.
> 5. *"Length is the packet length."* It excludes the first 8 bytes.

**Q2.** Decode by hand: `34 ff 00 2c 0a 0b 0c 0d 00 00 00 85 01 00 09 00 | <inner>`. Give version, flags, message type, what 0x2c counts, TEID, header length, the extension's type and its QFI (low 6 bits of the second content byte), and how many bytes the inner IP packet has. *(Tests `Gtpu.ExtensionHeaderPduSessionContainer`.)*

**Q3.** Why do the two directions of one bearer use *different* TEIDs? Walk through who tells whom which TEID during bearer setup. What does a gNB do on receiving a G-PDU with an unknown TEID, and how does the sender learn? *(Context: a GTP firewall in front of the core.)*

**Q4.** Overhead drill. (a) Inner MTU for transport MTU 1500, 1500 with S flag + one 4-byte extension, and 9000 (jumbo). (b) A 20 ms G.711 RTP packet is 200 B at IP level; how large is the tunnelled packet and the IP-level bandwidth per call on the S1-U/N3 link? How much extra per 1 000 calls? *(Context: Etherstack's MCPTT media on a backhaul budget.)*

**Q5.** Why does an Echo Request carry TEID 0 and a sequence number with the S flag set, while G-PDUs usually have neither? Compute the `length` field of an Echo Request with S set and no payload. *(Test `Gtpu.EchoRequestWithSequence`.)*

**Q6.** Hardening: list the checks `parse_gtpu` must perform before trusting anything (version, PT, length vs datagram, flags vs optional bytes, extension length 0, chain bound), and what attack each prevents (infinite loop, out-of-bounds read, amplification…). *(Test `Gtpu.RejectsMalformed`.)*

**Q7.** (Design) Your test tool replays *synthetic* GTP-U traffic into the Etherstack gateway lab. Which fields do you set per packet (TEID, outer DSCP, inner IP/UDP/RTP), how do you generate sequence/timestamps, and what do you validate on the gateway (TEID→call mapping, MTU, reordering via End Marker, QFI→DSCP mapping)? Why is *replaying captured live traffic* off the table?

---

## Part C — Diameter (RFC 6733)

### C.1 Why Diameter?
Core nodes need a **reliable, extensible request/answer protocol** between *peers* (not client/server): the HSS asks "who is this subscriber and what may they do?" (S6a), the PCRF says "give this flow a guaranteed bearer" (Gx/Rx), the charging system counts usage (Gy). Diameter descends from RADIUS (UDP, 8-bit attributes, no reliability, no peer model); fixes: TCP/SCTP transport, 32-bit codes, **application IDs**, vendor-specific attributes, capability exchange, failover, relaying/routing by realm.

### C.2 Header, derived
```text
 0       8                 31
+--------+------------------+   Version = 1
| Ver(8) | Message Length(24)|   whole message incl. header, multiple of 4
+--------+------------------+
| Flags(8) | Command Code(24)|   R=request  P=proxiable  E=error  T=potentially re-transmitted
+----------+----------------+
|        Application-ID(32)  |   which Diameter application (0 = base, 4 = credit control, 16777251 = S6a, 16777238 = Gx, 16777236 = Rx ⚠)
+----------------------------+
|        Hop-by-Hop ID(32)   |   matches answer to request on ONE hop; every relay rewrites it
+----------------------------+
|        End-to-End ID(32)   |   duplicate detection between the original endpoints; never rewritten
+----------------------------+
```
* The **command** is the verb (257 CER/CEA, 280 DWR/DWA, 272 CCR/CCA, 316 ULR/ULA ⚠); the **R flag** distinguishes request from answer (same code).
* **Hop-by-Hop vs End-to-End:** a request may cross relays/proxies. Each peer connection needs its own matching key (hop-by-hop), but the *originator and final server* need an id that survives (end-to-end, plus Origin-Host, for duplicate suppression after a retransmission — the **T** flag).
* Message length multiple of 4 → word-aligned parsing on 32-bit CPUs.

### C.3 AVPs: the extensibility trick
```text
| AVP Code (32) | V M P r r r r r | AVP Length (24) |  [Vendor-ID (32) only if V]  | Data … | pad to 4 B |
```
* **V**: vendor-specific code (the code is only unique *within the vendor* — 3GPP = 10415 ⚠); **M**: *must understand* — a receiver that doesn't know an M-AVP must reject the message (a way to make new semantics mandatory without a new application); **P**: protected (end-to-end security, rare).
* **Length excludes padding.** So `Origin-Host = "abc"` has length 11, but 12 bytes occupy the wire. A parser must advance by the *rounded-up* length, but give the application only `length − header` bytes (strip the pad).
* **Grouped AVP:** its data is itself a run of AVPs (e.g. `Vendor-Specific-Application-Id { Vendor-Id, Auth-Application-Id }`) → recursion with a depth limit.
* Typed data: Unsigned32, UTF8String, Address, Enumerated, Grouped, Time…

### C.4 Session of a Diameter peer
1. TCP/SCTP connect to port 3868 (TLS/DTLS optional but expected).
2. **CER/CEA** — exchange Origin-Host/Realm, supported applications (Vendor-Specific-Application-Id), Host-IP-Address, Product-Name. No common app → error (`DIAMETER_NO_COMMON_APPLICATION` 5010 ⚠).
3. Application requests/answers; **DWR/DWA** watchdogs detect dead peers (so failover happens in seconds, not TCP-timeout minutes).
4. **DPR/DPA** graceful disconnect.
Answers carry `Result-Code` (2001 SUCCESS; 3xxx protocol, 4xxx transient, 5xxx permanent errors ⚠) and optionally `Experimental-Result` for 3GPP-specific codes.

**Etherstack relevance:** an MCPTT server asks the PCRF over **Rx** (AA-Request, code 265 ⚠) to set up a **dedicated bearer** (QCI 65 *mission-critical PTT voice* ⚠) for a group call; in 5G the same intent is an HTTP/2 call to the PCF (N5).

> **Common misconceptions**
> 1. *"Diameter is RADIUS version 2."* Shares the AAA idea only; different transport, framing, and peer model.
> 2. *"AVP length includes padding."* No — and that is the #1 hand-parser bug.
> 3. *"The hop-by-hop ID is end-to-end."* It's rewritten at every relay; End-to-End is the stable one.
> 4. *"Diameter is secure because it's internal."* Historically inter-operator Diameter was abused (location tracking, fraud) — filtering/firewalls and TLS/IPsec are required ⚠.
> 5. *"One TCP connection = one session."* A connection carries many sessions/transactions multiplexed by IDs.

**Q8.** Decode the header `01 00 00 6c c0 00 01 10 00 00 00 04 4a 6b 2c 01 91 d3 a5 02`: version, total length, flags, command and name, application, both IDs, and how many bytes of AVPs follow. *(Context: a Gy charging request.)*

**Q9.** Decode `00 00 01 08 40 00 00 12 67 77 2e 65 78 61 6d 70 6c 65 00 00`: code, flags, length, data string, number of padding bytes, offset of the next AVP. Then encode `Vendor-Id` (266) = 10415 with the M bit; give the 12 bytes. *(Tests `Diameter.AvpBytesAndPadding`.)*

**Q10.** A hand-written parser advances by `length` (not rounded up) and reads a following AVP out of the padding. Describe the failure. Another parser reads `len = 0` and loops `pos += len`. Describe the DoS and the checks that prevent both. *(Test `Diameter.AvpRejects`.)*

**Q11.** Explain Hop-by-Hop vs End-to-End with a picture: console/PCEF → Diameter relay → PCRF and back. What does the relay change in the request and in the answer? What does the T flag + End-to-End ID let the server do after a link failure?

**Q12.** CER/CEA: write the AVPs of a CER from the Etherstack gateway acting as an MCPTT/Rx client (Origin-Host, Origin-Realm, Host-IP-Address, Vendor-Id, Product-Name, Auth-Application-Id or Vendor-Specific-Application-Id). Which would be M? What if the peer answers 5010? *(Test `Diameter.BuildAndParseCer`.)*

**Q13.** Compare Diameter and the 5G SBI as designs: transport, peer model, discovery/routing, extensibility mechanism, error model, security. Which is easier to debug with Wireshark? Why did 3GPP move the core to HTTP/2 + JSON?

---

## Part D — ASN.1 PER: why cellular is bit-packed

### D.1 The problem (concrete first)
RRC messages travel on radio resources shared by many users: a few hundred bits per message on a control channel matter. Compare one tiny message:
```text
Msg ::= SEQUENCE { a INTEGER (0..7), b BOOLEAN, c INTEGER (1000..1999) OPTIONAL, d ENUMERATED { x, y, z } }
value:  a = 5, b = TRUE, c = 1234, d = z
```
| Encoding | Bytes | Why |
|----------|-------|-----|
| JSON `{"a":5,"b":true,"c":1234,"d":"z"}` | 33 | names + punctuation + decimal text |
| **BER** (TLV: tag, length, value for every field) | **15** | `30 0D | 02 01 05 | 01 01 FF | 02 02 04 D2 | 0A 01 02` (BER bool TRUE = 0xFF; real wire may carry context tags) |
| **UPER** | **3** (17 bits) | no tags, no lengths, no names: **the schema is the format** |
JSON/BER are *self-describing* — the data says what it is. PER is **schema-driven**: sender and receiver both compiled the same ASN.1, so *types, field order and ranges are already known* and the wire carries only the information that is *not* in the schema. The cost: you cannot decode without the exact schema version, and errors are catastrophic (one bit wrong shifts everything).

### D.2 Derive the rules (UNALIGNED PER)
1. **Constrained integer** `INTEGER (lb..ub)`: the receiver knows the range, so only `value − lb` needs sending, using the minimum number of bits that can represent `range = ub − lb + 1` values: **`ceil(log2(range))`** bits. (0..7) → 3 bits; (1000..1999) → range 1000 → 10 bits; a single-value range → **0 bits**; (0..255) → 8; (0..256) → 9 bits (not 8!).
2. **BOOLEAN** = 1 bit. **ENUMERATED** with n root values = constrained integer (0..n−1). **CHOICE** = index (0..k−1) then the chosen alternative.
3. **SEQUENCE preamble:** one bit per `OPTIONAL`/`DEFAULT` component (1 = present) placed *first*, so a decoder knows what follows without tags. Extensible types (`...`) add one **extension bit** at the start (VERIFY detail) — that is how new fields are added later without breaking old decoders.
4. **Length determinant** for unbounded things: `< 128` → `0nnnnnnn` (8 bits); `< 16384` → `10nnnnnn nnnnnnnn`; `≥ 16384` → fragmentation. When the size is *constrained*, e.g. `OCTET STRING (SIZE(1..8))`, the length is `len − 1` in `ceil(log2 8) = 3` bits.
5. **Nothing is padded** in UNALIGNED PER: values are concatenated bit-after-bit; pad zeros appear only at the very end to fill the last byte. **ALIGNED PER** (used by S1AP/NGAP ⚠) inserts padding so that some fields start on octet boundaries — larger, but faster to parse. Ranges ≥ 256 are handled differently in aligned PER (whole-octet fields), unaligned always uses minimum bits (X.691 §11.5 ⚠ VERIFY).

Worked example: preamble `1` (c present) · a=5 → `101` · b → `1` · c = 1234 − 1000 = 234 → `0011101010` · d = z (index 2) → `10` = `1 101 1 0011101010 10` = 17 bits → pad → `D9 D5 00` (`1101 1001  1101 0101  0000 0000`). With c absent and d = y: `0 101 1 01` = 7 bits → `0101 1010` = `5A`.

> **Common misconceptions**
> 1. *"PER is compression."* It isn't entropy coding; it simply omits what the schema already implies.
> 2. *"UPER and APER are the same."* Same type system, different padding rules; RRC and NGAP/S1AP differ ⚠.
> 3. *"You can skip an unknown field."* Not without length information: PER has none — hence extension markers + length-wrapped extension additions.
> 4. *"A smaller range always means fewer bytes."* It means fewer *bits*; bytes only after concatenation and end padding.
> 5. *"A decoded value is always in range."* With 10 bits for (1000..1999) the patterns 1000–1023 are **illegal**: a decoder must reject them (test `Uper.DecodeRejectsOutOfRangeCodepoint`).

**Q14.** Bit-width drill (UPER): `INTEGER(0..7)`, `(0..255)`, `(0..256)`, `(1..100)`, `(-128..127)`, `(0..65535)`, `(0..4294967295)`, `(5..5)`, `BOOLEAN`, `ENUMERATED{a,b,c,d,e}`, `OCTET STRING(SIZE(4))`, `OCTET STRING(SIZE(1..8))` of length 3 (give total bits). *(Test `Uper.ConstrainedIntegerWidths`.)*

**Q15.** Encode by hand with the `Msg` schema: (a) `a=3,b=false,c absent,d=x`; (b) `a=7,b=true,c=1999,d=z`. Decode `B4 80 80`. Show the bit strings. *(Tests `Uper.DemoSequenceOptionalAbsent/Present`.)*

**Q16.** Derive from first principles why a decoder must **range-check** after reading `ceil(log2 range)` bits (range 1000 → 10 bits → 24 illegal patterns). List five ways a malicious or corrupted PER stream can hurt a decoder (illegal enum/choice index, length determinant vs remaining bits, recursion depth through nested CHOICE/SEQUENCE OF, huge `SEQUENCE OF` counts, truncated input) and how `BitReader::get` returning `nullopt` helps. *(Context: fuzzing a diagnostics decoder with synthetic input.)*

**Q17.** The length determinant of 200 octets is `80 C8`; of 127 is `7F`; of 16384 is not expressible here. Derive each from the two forms. Why `10` + 14 bits and not simply 16 bits? *(Test `Uper.LengthDeterminant`.)*

**Q18.** Why did 3GPP choose ASN.1 PER for RRC/NGAP rather than protobuf or JSON? Argue from: air-interface capacity, formal schema + backward-compatible extension, multi-vendor interoperability, tooling/long product life, parsing cost on a UE. What is the downside you'd flag in a code review?

**Q19.** (Compare with pack 05) Encode the same `CallStats` (packets=300, lost=3) in protobuf (bytes) and in a hypothetical UPER schema with `packets INTEGER (0..65535)`, `lost INTEGER (0..255)`. Sizes in bits/bytes? When is each the better engineering choice (schema evolution vs density)?

---

## Part E — The 5G Service-Based Interface (SBI)

### E.1 What and why
The 5G core (5GC) replaced fixed point-to-point interfaces (S6a, Gx, S11…) with **services** exposed by *network functions* (NFs) over **HTTP/2 + JSON** (resources and OpenAPI-defined schemas, `application/json`, `application/problem+json` for errors). Any NF can call any other service it is authorised for; an **NRF** (Network Repository Function) lets NFs register and be *discovered*; **OAuth 2.0** access tokens (issued by the NRF) authorise calls; TLS (mTLS) protects the hop.
| NF | Job | LTE ancestor |
|----|-----|--------------|
| AMF | registration, mobility, connection (N1 NAS, N2 NGAP) | MME (part) |
| SMF | PDU session, IP allocation, picks/instructs UPF over N4 | MME session part + SGW-C/PGW-C |
| UPF | user plane forwarding (GTP-U on N3, N6 to data network) | SGW-U/PGW-U |
| UDM / UDR | subscriber data (and storage) | HSS |
| AUSF | authentication | HSS auth |
| PCF | policy and QoS decisions | PCRF |
| NRF | registration, discovery, tokens | (new) |
| NEF | exposure to third parties (e.g. an MCX server asking for QoS) | SCEF |
Example call (SM context creation): `POST /nsmf-pdusession/v1/sm-contexts` with a JSON body containing the SUPI, DNN, S-NSSAI … returns `201 Created` + `Location`. Problems return `4xx/5xx` with `ProblemDetails` (`cause`). ⚠ VERIFY exact resource paths per release.

### E.2 Why HTTP/2 and not Diameter again?
Reuse the web ecosystem (load balancers, service meshes, TLS, tracing, OpenAPI codegen), natural fit for cloud-native/containerised NFs, multiplexing many transactions on one connection (pack 05), richer, versioned REST resources instead of bespoke AVP dictionaries. Costs: JSON verbosity (hence HPACK + local network), TCP head-of-line blocking (all streams share a connection), and *less* formal "must-understand" semantics than M-bit AVPs.

**What stayed binary:** N2 (NGAP, ASN.1), N3 (GTP-U), N4 (PFCP), and the whole radio interface — i.e. everything that is *per-packet*, *latency-critical* or *air-bound*; SBI is for *control-plane transactions between servers*.

> **Common misconceptions**
> 1. *"5G replaced GTP with HTTP."* GTP-U still carries user data on N3/N9; HTTP/2 only replaced core *control* interfaces (Diameter/GTP-C).
> 2. *"SBI is REST so it's stateless."* Procedures are stateful (NF state, subscriptions, callbacks via notification URIs).
> 3. *"JSON means the radio is JSON."* The radio protocols stay ASN.1 PER.
> 4. *"HTTP/2 = no HOL."* TCP HOL remains (pack 05, Q12 there).

**Q20.** For each interface give transport + encoding + one thing it carries: N1, N2, N3, N4, N5, N6, N11, N8/N10 (UDM), Rx, S6a, S1-U, S11. *(Use the Part A table.)* Which of these would an MCX server / Etherstack gateway talk to directly, and which only indirectly?

**Q21.** An MCPTT group call needs a dedicated, guaranteed-bit-rate bearer. Describe the LTE path (MCPTT server → Rx AAR → PCRF → Gx RAR → PGW → S5 → S11 → eNB bearer setup, QCI 65) and the 5G path (MCX AS → NEF/PCF over HTTP/2 → SMF → N4 → UPF/N2 → gNB QoS flow, 5QI 65). Mark where GTP-U TEIDs/QFIs are assigned and where DSCP for the transport is chosen. ⚠ flag values to verify.

**Q22.** Classify each failure and say where you'd look: (a) UE has IP but no uplink voice — GTP-U TEID mismatch? (b) Diameter `DIAMETER_UNABLE_TO_COMPLY` on Rx; (c) HTTP 403 from an NF; (d) NGAP decode error after a software upgrade; (e) peer drops after 30 s with no traffic. Which tool for each (Wireshark filter, logs)?

---

## Part F — Build tasks (`code/`)

| Step | Functions in `cellenc.hpp` | Tests |
|------|----------------------------|-------|
| 1 | `parse_gtpu`, `build_gtpu_gpdu`, `gtpu_overhead`, `gtpu_inner_mtu` | `Gtpu.*` |
| 2 | `parse_diameter_header`, `parse_avps`, `build_avp`, `build_diameter` | `Diameter.*` |
| 3 | `BitWriter::put`, `BitReader::get` | `Bits.*` |
| 4 | `ceil_log2`, `uper_put/get_constrained`, `uper_put/get_length`, `uper_put/get_octets` | `Uper.*` |
| 5 | `encode_demo`, `decode_demo` | `Uper.Demo*` (includes an exhaustive round trip) |

```bash
cmake -S code -B build && cmake --build build
./build/cellenc_tests_student     # red until include/cellenc.hpp is implemented
./build/cellenc_tests_solution
```
Optional Wireshark exercise: open a **published** sample capture (e.g. from the Wireshark SampleCaptures wiki ⚠ pick a GTP/Diameter one), apply filters `gtp`, `diameter`, `diameter.flags.request == 1`, `ngap`, `http2`, and match fields to the diagrams. Never capture live operator traffic.

---

## ⚠ VERIFY list (this pack)
1. GTP-U extension-header comprehension bits; type 0x85 = PDU Session Container; QFI = low 6 bits of the container's second byte (TS 29.281, TS 38.415).
2. Application IDs: S6a 16777251, Gx 16777238, Rx 16777236, Credit-Control 4; command codes 316/318/265/272; Result-Code families and 5010; 3GPP vendor id 10415.
3. QCI/5QI 65 (mission-critical PTT voice), 66, 69, 70; MTU values operators advertise.
4. Which interfaces use UPER vs APER (RRC UPER; S1AP/NGAP/X2AP APER); UPER minimum-bit rule for ranges ≥ 65 536; extension-bit placement.
5. PFCP (8805) and GTPv2-C (2123) ports; SBI resource paths and OAuth details per 3GPP release.
6. BER byte count (15) assumes universal tags; real RRC/NGAP uses PER, not BER — the BER row is illustrative.
