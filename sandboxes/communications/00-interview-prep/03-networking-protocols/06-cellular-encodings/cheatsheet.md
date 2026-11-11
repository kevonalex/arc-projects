# Cheatsheet — Cellular encodings

## Who carries what
| | Transport | Encoding | Carries |
|-|-----------|----------|---------|
| GTP-U | UDP 2152 | 8 B header (+4) | user IP packets, eNB/gNB ↔ SGW/UPF |
| GTPv2-C | UDP 2123 | TLV | LTE session/bearer control ⚠ |
| Diameter | TCP/SCTP 3868 | 20 B header + AVPs | S6a (HSS), Gx/Rx (PCRF), Gy (charging) |
| 5G SBI | HTTP/2 + TLS | JSON | NF ↔ NF services (SMF, AMF, UDM, PCF, NRF…) |
| PFCP (N4) | UDP 8805 | TLV | SMF → UPF rules ⚠ |
| S1AP / NGAP | SCTP | ASN.1 **APER** ⚠ | RAN ↔ core control |
| RRC | radio | ASN.1 **UPER** ⚠ | UE ↔ RAN control |

## GTP-U
`[Ver 001|PT 1|·|E|S|PN] [type] [length: bytes after first 8] [TEID 32] ([seq 16][N-PDU 8][next ext 8])`
* Type 0xFF G-PDU · 01/02 Echo · 1A Error Ind. · FE End Marker. TEID chosen by the **receiver**. Echo uses TEID 0.
* Extension: `[len in 4B units][content][next]`; 0x85 = PDU Session Container (QFI) ⚠.
* Overhead 36 B (20+8+8) → inner MTU 1464 @1500. No auth/encryption: isolate UDP 2152, IPsec, GTP firewall.
* Validate: n ≥ 8, ver/PT, `8+len ≤ n`, optional bytes present, ext len ≥ 1, chain bound.

## Diameter
`[ver 1][len 24][flags R P E T][cmd 24][app-id 32][hop-by-hop 32][end-to-end 32]`
* AVP: `[code 32][V M P ·····][len 24 (no padding)][vendor 32 if V][data][pad to 4]`.
* Advance by `(len+3)&~3`; require `len ≥ 8/12`, `len ≤ remaining` (no zero-length loops).
* Hop-by-hop rewritten per relay; end-to-end stable (+Origin-Host → duplicate detect, T flag).
* Peer: CER/CEA → traffic → DWR/DWA → DPR/DPA. Codes: 257 CER, 280 DWR, 272 CCR, 316 ULR ⚠; app 4 CC, 16777251 S6a, 16777238 Gx, 16777236 Rx ⚠; 3GPP vendor 10415 ⚠.

## ASN.1 PER (unaligned)
| Type | Bits |
|------|------|
| `INTEGER (lb..ub)` | `ceil(log2(ub−lb+1))`, value−lb; single value = 0 bits |
| BOOLEAN | 1 |
| ENUMERATED n | `ceil(log2 n)` |
| SEQUENCE | 1 preamble bit per OPTIONAL/DEFAULT (+ extension bit if `...`) |
| CHOICE k | `ceil(log2 k)` index + alternative |
| Length (unbounded) | `0nnnnnnn` <128; `10nnnnnn nnnnnnnn` <16384; else fragments |
| `OCTET STRING(SIZE(lb..ub))` | (len−lb) in `ceil(log2(ub−lb+1))` bits, then bytes, **no padding** |
* Not self-describing: needs exact schema; range-check after reading bits; bound lengths/recursion.
* Example `Msg{a:0..7,b,c:1000..1999 OPT,d:enum3}`: (5,T,absent,y)=`5A`; (5,T,1234,z)=`D9 D5 00`.
* APER pads to octet boundaries (bigger, faster to parse) — S1AP/NGAP ⚠.

## 5G SBI
HTTP/2 + JSON + OpenAPI; NRF discovery; OAuth2 tokens; ProblemDetails errors. AMF (mobility), SMF (sessions), UPF (data plane), UDM/UDR (subscriber), AUSF (auth), PCF (policy), NEF (exposure). N2 NGAP, N3 GTP-U, N4 PFCP stay binary. Still TCP HOL.

## Numbers
G.711 20 ms packet 200 B → +36 B tunnel = 236 B → 94.4 kbit/s per call.

## Safety
Synthetic bytes / published vectors only — no live operator or public-safety capture.

## Build
`cmake -S code -B build && cmake --build build && ctest --test-dir build`
