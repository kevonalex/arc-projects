# Pack 05 — System architecture & interworking: trunking, floor control, simulcast/voting, encryption, LMR ↔ MCPTT

> **SAFETY / LEGAL.** Everything here uses **synthetic** frames, identities and arithmetic you produce in unit tests.
> **Never** receive, record or decode live public-safety / commercial radio traffic with anything from this pack
> (or an SDR). It may be unlawful, and encrypted traffic is off-limits by design. Studying the structure of
> open-source decoders (OP25, dsd-fme, SDRTrunk) is optional; **do not copy their copyrighted code**.
> This pack implements **no cipher**: never write your own cryptography; use a vetted library in real work.

Standards (numbers from memory — **VERIFY**; "clause VERIFY" = look it up, I never invent clause numbers):
TIA-102 P25 suite — trunking control-channel messages and procedures (TIA-102.AABC / AABD family), OTAR and key
management (TIA-102.AACA family), block-encryption protocol (TIA-102.AAAD family), ISSI/CSSI (TIA-102.BACA family);
ETSI TS 102 361-4 (DMR Tier III trunking); ETSI EN 300 392 (TETRA); NIST FIPS 197 (AES), FIPS 46-3 (DES, withdrawn);
3GPP **TS 23.179** (MCPTT architecture), **TS 23.280** (common MC architecture), **TS 23.283** (MC interworking with
LMR — Rel-15, **VERIFY** number/scope), **TS 24.379** (MCPTT call control), **TS 24.380** (MCPTT floor control),
**TS 26.179** (MCPTT codecs), **TS 33.180** (MC security).

---

## 1. Why this matters (Etherstack context)

Etherstack's **core business is LMR protocol stacks** — but a stack is never alone. Radios sit on **systems**: trunked
networks with control channels, multi-site simulcast cells, voting receivers, key-management facilities, and increasingly
**gateways to LTE/5G mission-critical push-to-talk (MCPTT/MCX)**. A candidate who knows only frame layouts (packs 01–04)
can't answer "why does a late-joining radio take half a second to unmute?", "what happens to a call when the grant queue is
full?" or "how would you bridge a P25 talkgroup into an MCPTT group?". This pack gives you the system-level mental model
and small, testable models of the pieces. (Etherstack's gateway/IWF product line — **VERIFY** current product list.)

## 2. Prerequisites

- Pack 01 §3 (symbols, FEC chains, LDU/LC/ES, late entry 540 ms worst / 360 ms mean) and Pack 02–04 (slot arithmetic).
- C++17, GoogleTest, Linux/macOS/WSL with `-Wall -Wextra -fsanitize=address,undefined`.
- Probability at the level "independent events multiply".

```bash
cd code
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure  # stubs: FAIL
cmake -S . -B build_sol -DLMR_USE_SOLUTIONS=ON && cmake --build build_sol -j && ctest --test-dir build_sol
```

## 3. Concept build-up

### 3.1 Conventional vs trunked — derive the benefit

**Conventional:** each user group is *assigned a fixed channel*. Users listen on it and, in a busy system, manually check
whether it is free before keying up. **Trunked:** a pool of channels is shared; a controller assigns a channel **per call**
(a *grant*) and the radios retune to it.

Why would pooling help? Concrete example. A fleet generates **2 Erlangs** of traffic (on average 2 calls in progress).
- *Conventional:* four channels, each group has its own: each channel sees **0.5 Erlang**.
- *Trunked:* the same traffic shares all four channels.

Blocking (a call finds no free channel) is given by **Erlang B** with offered load E and m channels
(blocked calls cleared; recursion `B(E,0)=1`, `B(E,m) = E·B(E,m−1) / (m + E·B(E,m−1))`):

| Layout | Calculation | Blocking |
|---|---|---|
| 4 × (1 channel, 0.5 E) | `B(0.5,1) = 0.5/(1+0.5)` | **33.3 %** |
| 1 × (4 channels, 2 E) pooled | B1 = 0.667, B2 = 0.4, B3 = 0.2105, B4 | **9.5 %** |
| pooled but 1 of 4 is the control channel: (3 channels, 2 E) | `B(2,3)` | **21.1 %** |

Pooling wins because **statistical multiplexing**: a busy group borrows the channel an idle group isn't using. The third row
is the **cost**: the control channel carries no speech. Trunking pays off when the pool is large enough that one control
channel is a small tax (row 3 still beats row 1 but by far less than row 2 suggested).

> **Misconception box — "trunking gives you more channels."** It gives you **no extra spectrum**. It gives you better *use* of the
> same channels, at the price of a control channel, grant latency, and a single point of failure (trunked systems keep a
> **failsoft/site-trunking** mode for that; VERIFY terms).

### 3.2 The control channel

Idle trunked radios can't scan 20 channels to find out where their group is talking. So the system keeps **one channel
transmitting continuously** — the **control channel** (CC). All idle radios *park* on it and read broadcast messages;
when a call starts the CC announces "group G → channel N" and every member jumps there.

Two directions, two roles:
- **Outbound** (system → radios): broadcast grants, channel identifiers, system/site info, queue/deny responses.
- **Inbound** (radios → system): registration, affiliation, call requests (a *slotted random-access* channel, so requests can
  collide and be retried — the reason request latency isn't constant).

Standards realisations: **P25 Phase 1/2** — a dedicated CC carries **TSBKs** (pack 01: 96 bits → trellis 196);
**DMR Tier III** — a logical control channel in a timeslot (TS 102 361-4); **TETRA** — **MCCH** in timeslot 1 of the main
carrier (pack 04 §3.5). Message names (P25: `GRP_V_CH_GRANT`, `GRP_AFF_REQ`, `U_REG_REQ`, `QUE_RSP`, `DENY_RSP` …) **VERIFY**
against TIA-102.AABC.

### 3.3 Registration, affiliation, grants — three different verbs

| Verb | Who → whom | Meaning | Why it exists |
|---|---|---|---|
| **Registration** | unit → system | "I am unit U; authenticate me / record me as present at this site." | Identity, billing, authorisation, location. |
| **Affiliation** | unit → system | "I am listening to talkgroup G now (my knob position)." | The system must know **where members are** so a call is broadcast **only on sites with members** and no channel is wasted. |
| **Grant** | system → group | "Group G: go to channel N (talker is unit T)." | The actual channel assignment per call. |

*Derivation:* without affiliation the controller would have to occupy a channel **at every site** for every call, because it
couldn't know where members are. With affiliation a site-local call uses one channel at one site. **A unit is affiliated with one
talkgroup at a time** (its selector position), so `affiliate(u, g)` *replaces* an earlier affiliation — the code model's rule 1.

> **Misconception box — "registration and affiliation are the same."** Registration says *who you are and that you're here*;
> affiliation says *what you want to hear*. A registered unit can be affiliated to nothing; an affiliation without registration
> should be rejected.

### 3.4 Floor control, hang time, late entry, emergency, preemption

A talkgroup call is **half-duplex**: exactly one **talker** holds the **floor** at a time.

- **Request → grant/queue/deny.** PTT sends a request. The controller answers: *granted* (channel free), *queued* (all busy —
  "wait for your turn"), *denied* (not affiliated, not authorised, system busy policy). **Joined**: if the group's call is already
  up, the new talker simply uses the same channel (floor passes within the call).
- **Hang time.** After the talker releases, the controller **keeps the channel assigned to the group for a few seconds**. *Derivation:*
  conversation has turn-taking; replies come within a couple of seconds. Without hang time every reply needs a new request and
  grant (latency ≈ hundreds of ms to seconds, and risk of **queueing**, so the reply might be delayed behind other groups).
  Cost: the channel is idle but unavailable during hang time (§4(c), Q8). Longer hang = better conversations, worse capacity.
- **Late entry.** A radio that turns up mid-call (power-on, coming back into coverage, scanning) needs the call parameters
  (group, source, encryption) **while the call is running** — see §3.5.
- **Emergency.** An emergency request (a flagged button press) is treated with priority: if no channel is free, the controller
  **preempts** (drops) a lower-priority call and reuses its channel. In the code model (rules 2d/3): victim = non-emergency call with the
  lowest group id (a *deterministic* tie-break chosen for testability; real systems use configured **priority levels** — VERIFY),
  and emergency calls cannot preempt each other; they **queue ahead** of normal calls.
- **Talk timeout.** A stuck PTT can hold a channel forever, so controllers enforce a maximum talk time (pack 06 implements this).

> **Misconception box — "hang time is wasted airtime."** It is *deliberately* spent airtime that buys lower reply latency and
> fewer new grants (and fewer control-channel messages). But it can **starve queued groups** if replies keep resetting it (Q8).

### 3.5 Why headers get stronger FEC than voice — and why late entry needs a repeated LC

**Header vs voice FEC (derive it, don't recite it).** Compare what a single bit error *costs*:

| Where | One bad bit means | Recovery |
|---|---|---|
| IMBE voice frame (pack 01 §3) | a **20 ms** glitch, often inaudible | none needed; next frame is independent |
| NID / header / TSBK / link control | **wrong NAC/DUID ⇒ the frame is discarded**; wrong talkgroup or encryption fields ⇒ the **whole call** is dropped, misrouted or mis-decrypted | **none** — broadcast radio has no ARQ; retransmission isn't possible for a listener who can't talk back |

Also: voice has **psychoacoustic redundancy** (the ear tolerates errors; the vocoder conceals them), signalling has none; and
the **bit cost** of strong FEC is small for short control words (a 96-bit TSBK → 196 bits at about rate 1/2, once per message),
but applying rate-1/2 FEC to a **continuous** voice stream would halve the bits available to the vocoder. So the standard
spends FEC **unequally by consequence per bit**: strong codes (BCH, Golay, trellis) on signalling, graded codes (Golay on the
most significant voice bits, none on the least significant: pack 01) on voice. This is **unequal error protection**.

**Repeated LC / ES (late entry).** A receiver may join at **any instant** and cannot ask for a repeat. If the call parameters were sent only
at the call start, everyone who missed that moment would hear nothing, or ciphertext, for the whole call. So the link control (who/which group)
and the **encryption sync** (algorithm, key id, message indicator) are **repeated throughout the call**: in P25 Phase 1, LC rides in
every LDU1 and ES in every LDU2 (pack 01), i.e. each repeats every 360 ms. **Trade-off:** repetition period *T* costs overhead ∝ 1/T and
gives mean wait ≈ T/2 (+ more if a repetition is lost): `acquire_time_ms(T, p_loss) = T/2 + p/(1−p)·T` (Q5).

> **Misconception box — "late entry is a nice-to-have."** It is the reason **a handheld powered on during an ongoing group call** can
> join it; it also decides **how fast encrypted calls unmute** for roamers and scanners. Remove repetition and the protocol breaks
> for every receiver that wasn't already listening at PTT.

### 3.6 Simulcast vs voting (two opposite problems, two opposite solutions)

*The asymmetry:* a tower transmits at tens of watts from a tall mast; a handheld transmits at ~1–5 W at head height. So **coverage is
limited by the uplink** (handheld → tower), not the downlink.

| Direction | Problem | Solution | Mechanism |
|---|---|---|---|
| **Downlink** (system → radios) | One tower can't cover the area | **Simulcast**: several transmitters send **the same bits on the same frequency at the same time** | Needs GPS-locked timing and frequency; every transmitter's audio must be aligned within a small fraction of a symbol |
| **Uplink** (radio → system) | Weak handheld heard by different towers at different quality | **Voting**: several receivers decode the same transmission; a **comparator picks the best copy** frame by frame | Needs the copies aligned, and a quality metric (e.g. corrected-error count, CRC) |

**Simulcast delay budget — derive it.** Radio waves travel at c = 299 792.458 km/s ⇒ **3.3356 µs per km**. A receiver between two
transmitters hears copy 1 and copy 2 with arrival-time difference Δt = Δpath / c. If Δt is a significant part of a **symbol period**
(P25 Phase 1: 1/4800 s = **208.33 µs**), the two copies **interfere destructively at the symbol decision point** (delay spread → *inter-symbol
interference*). Design rule (VERIFY, rule of thumb): keep Δt below ≈ **¼ symbol** ⇒ 52.08 µs ⇒ **Δpath ≤ 15.6 km** at 4800 sym/s. Faster symbols shrink the budget
(Q3). Engineers *add deliberate delay* to the nearer transmitter so the copies align where signals overlap, and P25 specifies a
simulcast-friendly modulation (**CQPSK/LSM**, pack 01 — VERIFY) for the downlink.

**Voting arithmetic.** If each of *n* receivers independently loses a given frame with probability *p*, **all** lose it with probability
*pⁿ*: p = 0.1 → 1 receiver 0.1, 2 receivers 0.01, 3 receivers **0.001** (Q4). **Caveat:** independence is optimistic — receivers close to each other share
the same fade or interferer; the gain is smaller than *pⁿ* when errors are correlated.

> **Misconception box — "simulcast and voting are the same thing."** Simulcast = *several transmitters*, same signal, **downlink**. Voting =
> *several receivers*, pick best, **uplink**. Many wide-area sites use both ("simulcast downlink with voted uplink").

> **Misconception box — "more overlapping transmitters always improve the signal."** Only if their copies are aligned in time.
> Misaligned overlap **creates** a dead zone (a "simulcast null") exactly where two equally strong signals meet.

### 3.7 Encryption: DES-OFB, AES-256, OTAR

**Why stream-style (OFB)?** Voice is a continuous stream with error-tolerant payload. In **OFB** mode a block cipher generates a
**keystream**: `K_i = E_key(K_{i−1})`, starting from an **initialisation vector** (in P25 the **message indicator, MI** — 72 bits, VERIFY),
and ciphertext = plaintext ⊕ keystream. One flipped ciphertext bit flips exactly **one** plaintext bit (no error propagation) —
which the vocoder shrugs off. A chaining mode like CBC would smear one bit error across a block: bad for radio.

| Algorithm (P25 ALGID — VERIFY values) | Key size | Status |
|---|---|---|
| **DES-OFB** | 56 bits | legacy; brute-forceable (2⁵⁶ ≈ 7.2·10¹⁶ trials: ≈ 20 hours at an illustrative 10¹² trials/s — Q14) |
| **AES-256** (OFB in P25) | 256 bits | current recommendation; infeasible to brute-force |

**What the receiver must know:** algorithm id, **key id** (which key), and **MI** (the IV). They travel **in clear**
in the **encryption sync** words (pack 01, `parse_encryption_sync`) and — see §3.5 — repeat for late entry. *The header's metadata is not secret; the voice is.*

**IV discipline.** Keystream must **never repeat** under one key: with the same keystream K, `C1 ⊕ C2 = P1 ⊕ P2` — the key cancels and an
eavesdropper learns the XOR of two plaintexts (the **two-time pad**). So the MI advances each frame (an LFSR in P25 — VERIFY) and a new call uses a fresh MI.

**OTAR — over-the-air rekeying.** Thousands of radios can't all be reloaded with a **key variable loader** each time a key is retired or a
radio is lost. A **Key Management Facility (KMF)** sends new traffic keys **encrypted under a per-radio key-encryption key (KEK)** over the
air; the radio decrypts and installs them (TIA-102.AACA family — VERIFY). Costs: the KEK is now the crown jewel; OTAR messages need
reliable delivery (acks, retries) on a lossy channel.

> **Misconception box — "encrypted calls hide who is talking."** Only the **voice** is encrypted. Talkgroup, source ID, ALGID, KID, MI are sent in
> clear (they must be, so the network can route and the receiver can pick the key). Traffic analysis remains possible.

> **Misconception box — "AES makes any radio secure."** Weak key management (shared static keys, no rotation, KEK reuse, MI reuse) defeats
> any cipher. The 2023 TETRA TEA1 findings (pack 04 §3.6) illustrate the point: the protocol's *crypto design and key handling* matter more than the framing.

### 3.8 Interworking: LMR ↔ MCPTT (3GPP mission-critical push-to-talk)

**MCPTT** brings group push-to-talk to LTE/5G: SIP signalling, **floor control** over RTCP-like messages (TS 24.380),
voice in RTP with AMR-WB or EVS (TS 26.179 — VERIFY), E2E-capable security (TS 33.180). An **IWF** (interworking function) sits between the MC system and an LMR
system (the Rel-15 "MC interworking with LMR" work, TS 23.283 — **VERIFY** exact title and scope) so that a handheld on LTE can talk to a P25 talkgroup. A gateway has to map **four**
things:

1. **Identities.** P25 unit identity is **56 bits**: WACN (20) + System ID (12) + Unit ID (24); group: WACN + SysID + 16-bit talkgroup. MCPTT
   addresses by **MCPTT ID / SIP URI**. The mapping must be **injective** (two different LMR identities never map to the same URI) ⇒ it
   must include **all three** fields — the 24-bit unit alone collides between systems (Q9, Q13). *(The URI format in the code is a teaching convention, not 3GPP's.)*
2. **Floor control.** LMR: PTT → grant/queue/deny → talker change → release → idle. MCPTT: *Floor Request / Granted / Deny / Taken / Release / Idle /
   Revoke / Queue Position Info* (names VERIFY, TS 24.380). Mostly a **one-to-one** state mapping (Q10) — the hard part is *timing*: LMR grants take hundreds of ms on a
   control channel.
3. **Voice transcoding.** IMBE/AMBE+2 (P25, DMR: 20 ms frames), ACELP (TETRA: 30 ms), AMR-WB/EVS (MCPTT: 20 ms): decode to PCM and re-encode.
   Consequences: **latency** (re-framing buffer + codec look-ahead; Q12), **tandem-coding quality loss** (vocoder → vocoder), and **DSP cost per concurrent call**.
4. **Security.** LMR encryption is keyed by the LMR key-management system; MCPTT end-to-end security by the MC key management server. To **transcode**,
   the IWF must see the **clear** audio ⇒ it must **decrypt and re-encrypt** ⇒ the IWF becomes a trusted party and **end-to-end security is broken
   at the gateway**. Options: trusted IWF (policy-controlled, hardened, audited), pass-through *without* transcoding (only if both sides use the same codec **and** a
   shared key — rare), or no encrypted interworking (Q15, Q16).

> **Misconception box — "an IWF is just a protocol converter."** It is a **media transcoder + identity mapper + key-handling endpoint + timing
> adapter**. The control-plane mapping is the easy 20 %.

> **Misconception box — "end-to-end encryption survives transcoding."** E2E means *only the endpoints hold the keys*. A transcoder in the middle needs the plaintext, so it
> is by definition an endpoint.

## 4. Fully worked example

**(a) A trunked system, step by step** (the code model; 2 channels, units 1/2/3 affiliated to groups 100/200/300).
1. `request_call(1,100)`: affiliated; free channel 0 ⇒ **Granted ch 0**.
2. `request_call(2,200)`: free channel 1 ⇒ **Granted ch 1**.
3. `request_call(3,300)`: no free channel, not emergency ⇒ **Queued** (queue length 1).
4. `release_call(100)`: channel 0 freed; queue front is group 300 ⇒ **promoted**: `QueuedGrant{300, ch 0}`.
5. Unit 5 (group 500, emergency) when both busy with normal calls: victim = lowest group id among non-emergency active calls = 200 ⇒ **Preempted(ch 1, victim 200)**.

**(b) Simulcast budget** for P25 Phase 1 (4800 sym/s): symbol = 1/4800 = 208.33 µs; ¼ symbol = 52.08 µs; 52.08 / 3.3356 = **15.6 km**.
Two transmitters 40 km apart, receiver at the midpoint: Δpath = 0 ⇒ Δt = 0 ✔; receiver 10 km from A, 30 km from B: Δpath = 20 km ⇒ Δt = 66.7 µs = 0.32 symbol — beyond
the rule of thumb, though B is weaker there (capture effect helps), so engineers equalise delays and place overlaps carefully.

**(c) Hang time economics.** 3 s hang; groups reply with probability 0.6 within hang. Per transmission, expected idle-hold ≈ (if reply) the gap until reply, (if not) the full 3 s: with
uniform reply times in the window, E[idle hold] = 0.6·1.5 + 0.4·3.0 = **2.1 s**; the benefit is the 0.6 of replies that skip a new grant. (Toy numbers; the point is the shape of the trade-off.)

**(d) Two-time pad, toy.** Plaintext bytes `P1 = 0x41`, `P2 = 0x42`, same keystream byte `K = 0x5A` (a *mistake*): `C1 = 0x1B`, `C2 = 0x18`. An eavesdropper sees `C1 ⊕ C2 = 0x03`. And
`P1 ⊕ P2 = 0x41 ⊕ 0x42 = 0x03` — identical; the key vanished. Guessing one plaintext yields the other.

**(e) Identity packing.** `GlobalUnitId{wacn = 0xBEE00, sysid = 0x001, unit = 0x00ABCD}` → `(0xBEE00 << 36) | (0x001 << 24) | 0x00ABCD`. Field widths 20 + 12 + 24 = 56 bits = **14 hex digits**:
`BEE00 | 001 | 00ABCD` ⇒ `0xBEE0000100ABCD`. URI: `sip:p25-BEE00-001-00ABCD@iwf.example`.

**(f) Re-framing.** TETRA 30 ms ↔ P25 20 ms: lcm = **60 ms** (2 TETRA frames = 3 P25 frames); 30 ms = 240 PCM samples at 8 kHz; 20 ms = 160.
Worst-case framing delay 30 → 20: `30 − gcd(30,20) = 20 ms`.

## 5. Questions

All questions carry an **Etherstack context** line. Answers: `answers.md`. Code: `code/include/lmrsys/…`.

### Q1 — Explain + Calculate — **code: `rf_calcs.hpp` (`erlang_b`)**
(a) Why does a trunked system serve the same traffic with less blocking than conventional channels? (b) Compute blocking for 2 Erlangs on 4 pooled channels, for four separate 0.5-Erlang channels, and for 3 pooled channels (one is the CC).
*Etherstack context:* a customer's RFP asks "why is trunking worth a control channel?"

### Q2 — Explain
What are the control channel, registration, affiliation and grant, and why does a trunked system need *affiliation* separately from registration?
*Etherstack context:* the stack's mobility-management state machine must keep both tables.

### Q3 — Calculate — **code: `rf_calcs.hpp`**
Derive propagation delay per km, symbol periods for P25 Phase 1, Phase 2 and TETRA, and the largest path-length difference at ¼ symbol for each. What happens to a receiver 10 km from transmitter A and 30 km from B in a P25 Phase 1 simulcast?
*Etherstack context:* simulcast controller delay-equalisation settings.

### Q4 — Calculate — **code: `rf_calcs.hpp` (`vote`, `vote_failure_probability`)**
(a) p = 0.1 per receiver, n = 1…4: probability all lose the frame. (b) Four voting samples: `(rx1, 3 corrected errors, CRC ok)`, `(rx2, 0, ok)`, `(rx3, 0, CRC fail)`, `(rx4, 0, ok)`: which wins and why? (c) Why isn't *pⁿ* achievable in practice?
*Etherstack context:* voter comparator design.

### Q5 — Calculate — **code: `rf_calcs.hpp` (`acquire_time_ms`)**
LC repeats every 360 ms. Mean time until the *next repetition begins* for p_loss = 0, 0.1, 0.5, 0.9? Add the 180 ms LDU duration to get the mean time to a *complete* LC. What if LC were sent every 720 ms instead?
*Etherstack context:* "why is there a half-second delay before an encrypted call unmutes on my scanning radio?"

### Q6 — Explain
(a) Why do headers/NID/TSBK/LC get stronger FEC than voice? (b) Why does late entry require the LC and ES to be repeated, and what is the cost of repeating them more or less often?
*Etherstack context:* the exact "interview classic" for the FEC-chain code in packs 01 and 03.

### Q7 — Trace — **code: `trunking.hpp`**
A 2-channel controller; units 1→g10, 2→g20, 3→g30, 4→g10 (all affiliated). Sequence: `req(1,10)`, `req(4,10)`, `req(2,20)`, `req(3,30)`, `release(10)`, `req(3,30,emergency)`. Give the result of every call.
*Etherstack context:* the trunk-controller log you read when debugging a "why was my call queued?" ticket.

### Q8 — Predict
Hang time 3 s. Group G talks 0–5 s; a member replies at 6.5 s, talks 6.5–9 s, releases at 9 s. Group Y requests at t = 10 s while the only other channel is permanently busy. (a) When does Y get a channel if hang time always runs to its end? (b) What if G replies again at 11.5 s (talks 11.5–12 s)? (c) What if G's reply at 10.5 s lasts to 14 s? (d) Design two policies that prevent starvation. (e) What does an *emergency* request at t = 10 s get?
*Etherstack context:* a customer complains "my queued call never goes through".

### Q9 — Implement — **code: `iwf_identity.hpp` (identity part)**
Implement `pack_unit`, `unpack_unit`, `to_unit_uri`, `parse_unit_uri`, `to_group_uri`. Why must the URI carry all three fields?
*Etherstack context:* IWF identity-mapping table.

### Q10 — Implement — **code: `iwf_identity.hpp` (floor part)**
Implement `to_mcptt` / `to_lmr` and prove (by test) that the mapping is a bijection. Which LMR concepts have *no* single MCPTT equivalent?
*Etherstack context:* floor-control translator between LMR and MCPTT.

### Q11 — Implement — **code: `trunking.hpp`**
Make `test_trunking` pass. Which data structure holds each rule and why?
*Etherstack context:* a trunk-site controller's call-processing core.

### Q12 — Calculate — **code: `rf_calcs.hpp`**
Frame alignment (lcm) for TETRA ↔ P25 and P25 ↔ MCPTT (20 ms ↔ 20 ms), samples per frame at 8 kHz, and worst-case re-framing delay in both directions (formula + derivation).
*Etherstack context:* the transcoder's jitter-buffer sizing.

### Q13 — Find the bug — **code: `iwf_identity.hpp`**
```cpp
uint64_t pack(uint32_t wacn, uint16_t sysid, uint32_t unit) { return (wacn << 36) | (sysid << 24) | unit; } // (1)
std::string uri(uint32_t w, uint16_t s, uint32_t u) { char b[16]; sprintf(b, "sip:p25-%d-%d-%d", w, s, u); return b; } // (2)
bool same_user(const Id& a, const Id& b) { return a.unit == b.unit; }                                          // (3)
McpttFloorMsg map(LmrFloorEvent e) {                                                                            // (4)
  switch (e) { case LmrFloorEvent::PttPress: return McpttFloorMsg::FloorRequest;
               case LmrFloorEvent::GrantToRequester: return McpttFloorMsg::FloorGranted; }
  return McpttFloorMsg::FloorGranted; }
std::optional<Id> parse(std::string_view s) { auto i = s.find("p25-"); return Id{stoul(string(s.substr(i+4)))}; } // (5)
```
Find all five bugs, state each symptom, fix them.
*Etherstack context:* the first-draft IWF code review.

### Q14 — Predict
(a) A bit error in an OFB ciphertext: how many plaintext bits are affected? (b) A late-entry radio never receives an ES: what does it output? (c) Two calls reuse the same (key, MI): what can an eavesdropper compute? (d) Brute-force time for DES vs AES-256 at 10¹² keys/s. (e) What does OTAR protect, and what is the single most valuable key?
*Etherstack context:* what a key-management interface in a stack must and must not do.

### Q15 — Connect
Design an LMR ↔ MCPTT gateway for one P25 talkgroup ↔ one MCPTT group: boxes, what is mapped (identity, floor, voice, security, emergency, affiliation) and where latency is added. Give an end-to-end latency budget.
*Etherstack context:* the IWF product architecture review.

### Q16 — Interview
"A customer demands *end-to-end encryption* **and** interworking between their P25 system and an LTE MCPTT service. What do you tell them?"
*Etherstack context:* the security-vs-interoperability conversation.

### Q17 — Interview
"Simulcast or voting — and when do you need both?"
*Etherstack context:* wide-area coverage design question.

### Q18 — Interview
"A 6-channel county fleet is on conventional channels and complains about busy signals. Do you recommend trunking?"
*Etherstack context:* pre-sales technology advice.

## 6. Self-assessment

- [ ] I can derive the Erlang-B pooling gain and the control-channel tax (33.3 % → 9.5 % → 21.1 %).
- [ ] I can explain registration vs affiliation vs grant.
- [ ] I can explain hang time, late entry, emergency preemption and how hang time can starve a queue.
- [ ] I can explain *why* headers get stronger FEC than voice and *why* LC/ES repeat.
- [ ] I can derive 3.3356 µs/km and the simulcast delay budget (15.6 km at ¼ symbol, P25 Phase 1).
- [ ] I can explain simulcast (downlink) vs voting (uplink) and the independence caveat.
- [ ] I can explain OFB, MI/IV discipline, two-time pad and OTAR.
- [ ] I can list the four IWF mappings and why E2E encryption breaks at a transcoder.
- [ ] Tests pass.

| Score | Meaning |
|---|---|
| 0–3 | Re-read §3.1–3.5 |
| 4–6 | Redo Q1, Q6, Q8, Q16 |
| 7–9 | Move to pack 06 |

### VERIFY list for this pack
1. TIA-102 document numbers and control-channel message names (`GRP_V_CH_GRANT`, `GRP_AFF_REQ`, `U_REG_REQ`, `QUE_RSP`, `DENY_RSP`); inbound slotted-access behaviour.
2. DMR Tier III (ETSI TS 102 361-4) control-channel arrangement; TETRA MCCH location (EN 300 392-2).
3. P25 encryption: ALGID values for DES-OFB and AES-256, MI length (72 bits), MI update rule, ES field layout (TIA-102.AAAD family).
4. OTAR (TIA-102.AACA family), KEK/KMF roles, KVL.
5. P25 simulcast modulation (CQPSK/LSM) and the ¼-symbol rule of thumb; real equaliser practice.
6. 3GPP: TS 23.179 / 23.280 / **23.283** (title, release, scope of MC–LMR interworking), TS 24.379/24.380 floor-message names, TS 26.179 codecs (AMR-WB/EVS), TS 33.180 security.
7. Real emergency/priority semantics vs this pack's lowest-group-id tie-break; failsoft/site-trunking terminology.
8. Group-ID widths (16-bit talkgroup) and the 56-bit unit identity layout.
9. Etherstack's current product list (gateway/IWF products).
