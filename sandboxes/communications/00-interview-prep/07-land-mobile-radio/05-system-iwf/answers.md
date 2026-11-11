# Pack 05 — Answers (system architecture & interworking)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic data only; never live traffic. **VERIFY** = check TIA-102 / ETSI / 3GPP before quoting.

---

## Q1 — Explain + Calculate: trunking gain

**Answer.** Pooling channels lets a busy group use a channel an idle group isn't using (statistical multiplexing), so the *same traffic* is
blocked less often. Erlang-B recursion `B(E,0)=1`, `B(E,m)=E·B(E,m−1)/(m+E·B(E,m−1))`:

| Layout | Steps | Blocking |
|---|---|---|
| 4 × (m=1, E=0.5) | B(0.5,1) = 0.5/(1+0.5) | **0.3333** |
| m=4, E=2 | B1 = 2/3 = 0.6667; B2 = 1.3333/3.3333 = 0.4; B3 = 0.8/3.8 = 0.2105; B4 = 0.4211/4.4211 | **0.0952** |
| m=3, E=2 (one channel is the CC) | B3 above | **0.2105** |

**Reasoning.** Blocking happens when *all* channels are busy at the instant of a request. With separate channels, a request is blocked whenever **its**
channel is busy, even when three others are idle. Pooled, it's blocked only when **everything** is busy — a rarer event.
Control-channel tax: the CC removes one channel of voice capacity; pooled blocking rises from 9.5 % to 21.1 %, still below 33.3 %.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Trunking adds channels." | More calls get through. | Same spectrum; better sharing. |
| "Blocking = E/m = 0.5." | Looks like utilisation. | Utilisation is not blocking; Erlang-B accounts for random arrivals. |
| "Ignore the CC; the pooled result is 9.5 %." | The textbook table. | The CC carries no speech: 3 voice channels, 21.1 %. |
| "Trunking is always better." | Trunking sounds modern. | With very few channels the CC tax dominates (Q18). |

*Etherstack angle:* the capacity model sizes a site; code `erlang_b` in `rf_calcs.hpp` (tests: 0.2, 0.4, 0.2105, 0.0952).

---

## Q2 — Explain: control channel, registration, affiliation, grant

**Answer.** The **control channel** is a continuously transmitting channel idle radios park on. **Registration** = "I'm unit U, present here, authenticate me".
**Affiliation** = "I'm listening to talkgroup G". **Grant** = "group G → channel N". Affiliation is separate because the system must know **which sites have
members of G** (and how many calls to set up per site), a fact registration doesn't carry: a registered unit may listen to nothing, and the same unit
changes group by turning a knob without re-registering.

**Reasoning.** Without affiliation the controller would have to assume members are everywhere: it would occupy a channel at **every** site for every call
(wasting capacity) or never reach someone. A unit is affiliated with **one** group at a time (selector position), hence a map `unit → group`; re-affiliation overwrites.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Registration and affiliation are the same message." | Both are 'attach' messages. | Different keys: unit-level (identity) vs group-level (interest). |
| "The CC carries voice too." | It's a channel. | Dedicated CC carries signalling only (some systems use a composite CC; VERIFY). |
| "Affiliation grants a channel." | The word 'join'. | A grant is a *per-call* assignment issued after a request. |
| "A unit can affiliate to many groups." | Scanning radios do hear many. | Scan lists are a radio feature; the system-visible affiliation is one group (VERIFY for vendor extensions). |

*Etherstack angle:* the mobility-management tables in any trunking stack: `unit→group`, `group→sites`.

---

## Q3 — Calculate: simulcast delay budget

| Quantity | Derivation | Value |
|---|---|---|
| delay per km | 1 km / 299 792.458 km/s | **3.3356 µs** |
| P25 Phase 1 symbol | 1/4800 | **208.33 µs** |
| P25 Phase 2 symbol | 1/6000 | **166.67 µs** |
| TETRA symbol | 1/18 000 | **55.56 µs** |
| ¼ symbol: P1 / P2 / TETRA | 52.08 / 41.67 / 13.89 µs | |
| max Δpath: P1 / P2 / TETRA | ÷ 3.3356 | **15.6 / 12.5 / 4.2 km** |

Receiver 10 km from A and 30 km from B: Δpath = 20 km ⇒ Δt = 66.7 µs = **0.32 symbol** (P25 Phase 1) — beyond ¼ symbol. In practice B is much weaker there
(free-space: (30/10)² = 9× ≈ 9.5 dB; in real terrain far more), so **capture** lets A dominate; the *dangerous* places are where the two signals are *equally
strong but misaligned* — which is why designers insert delay in one transmitter to centre the overlap region.

**Reasoning.** The faster the symbol rate, the less time-misalignment you can tolerate; hence TETRA's 4.2 km vs P25's 15.6 km (TETRA simulcast is for DMO/special
deployments — VERIFY). Test: `max_delay_spread_km(4800, 0.25) ≈ 15.61`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Delay = 3.3 ms/km." | Unit slip. | µs, not ms: 3.3 **µs**/km. |
| "Use the bit period 104 µs." | Bits are the data rate. | The decision interval for a 4-level symbol is the symbol period (dibit per symbol). |
| "Simulcast works at any spacing." | Same bits on same frequency. | The overlap region needs time alignment; otherwise ISI/null. |
| "Use 100 % of the symbol period." | The limit looks like 1 symbol. | At 1 symbol the copies are a full symbol apart: you decode the *previous* symbol; ¼ is a conservative margin. |

*Etherstack angle:* equaliser and delay-compensation parameters live in the simulcast controller.

---

## Q4 — Calculate: voting

(a) pⁿ with p = 0.1: **0.1, 0.01, 0.001, 0.0001**. (b) **rx2 wins**: valid samples are rx1 (3 errors), rx2 (0), rx4 (0); rx2 and rx4 tie, lowest id wins ⇒ 2. rx3 is
**excluded**: its 0 means nothing, because a failed decode has no meaningful corrected-error count. (c) Independence is optimistic: nearby receivers see the same fade, co-channel
interferer, or a deep local null in the handheld's pattern; a handheld in a basement is lost to all of them. Comparators also have **latency** and **switching artefacts** between sources.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "rx3 wins: 0 errors and the lowest unused id." | Lowest error count. | CRC failed ⇒ decode invalid ⇒ ignore its count. |
| "Average the receivers' bits." | Averaging reduces noise. | Digital frames are not analogue; pick one *whole* copy (or soft-combine with soft bits, which is a different, more complex scheme). |
| "n receivers = n× range." | More receivers. | Gain is diversity (log), not range multiplication. |
| "Vote per call." | Simple. | Per **frame** voting is what yields the gain; per-call choice wastes it. |

*Etherstack angle:* the comparator logic is `vote()`; tests cover tie, invalid, empty.

---

## Q5 — Calculate: late-entry acquire time

| p_loss | p/(1−p) | mean wait for next LC start (ms) | + 180 ms LDU (ms) |
|---|---|---|---|
| 0 | 0 | 180 | **360** |
| 0.1 | 0.111 | 180 + 40 = 220 | **400** |
| 0.5 | 1 | 180 + 360 = 540 | **720** |
| 0.9 | 9 | 180 + 3240 = 3420 | **3600** |

Derivation: the first wait is uniform on (0, T] ⇒ mean T/2 = 180 ms; each lost repetition adds a full period T; the number of losses before success is geometric with mean p/(1−p).
Pack 01's "mean 360 ms, worst 540 ms" is the p=0 row: worst = 360 (just missed the start) + 180 (receive it). With T = 720 ms: mean wait 360 (+180 = 540 at p=0), half the overhead.
Caveat: independent losses are a model; real fades are bursty, so a deep fade can wipe several repetitions in a row.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "180 ms mean total." | The formula gives T/2. | That's the wait for the *start* of the next repetition; you still need to receive the 180 ms LDU. |
| "Mean time = T." | Round number. | Uniform arrival ⇒ T/2 average wait. |
| "p = 0.9 adds 0.9 T." | Linear thinking. | Geometric: p/(1−p) = 9 periods. |

*Etherstack angle:* explaining "why an encrypted call takes about half a second to unmute for a scanning radio".

---

## Q6 — Explain: FEC strength, repeated LC

**(a) Answer.** Because an error in signalling is **catastrophic and unrecoverable**, while an error in voice is **local and tolerable**, and because strong coding is **cheap on short, rare
words** but **expensive on a continuous stream**.

**Reasoning.** (1) *Consequence per bit:* a corrupted NID drops the whole frame; a corrupted talkgroup/ALGID/MI misroutes or mis-decrypts the entire call; a corrupted IMBE bit is a 20 ms
glitch the vocoder can conceal. (2) *No ARQ:* one-to-many broadcast, listeners can't ask for a repeat. (3) *Redundancy:* the ear tolerates error; control fields have no redundancy. (4) *Cost:* a 96-bit TSBK
→ 196 bits happens at message rate; rate-½ coding of voice would halve the vocoder's bits. So FEC is allocated **by consequence per bit**: BCH/trellis on NID/TSBK, Golay on the important voice bits,
none on the least significant ones (**unequal error protection**).

**(b) Answer.** Late joiners can't ask; if parameters were sent once, anyone not listening at PTT would never learn group/source/encryption fields. So LC and ES **repeat** every LDU1/LDU2
(360 ms each). Cost vs benefit: overhead ∝ 1/T, mean acquisition ≈ T/2 (+ losses, Q5). Doubling T halves overhead and doubles the wait. Because the **MI** also changes during the call, ES must be *current*, not just
repeated.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Voice is less important." | Sounds right. | Not importance; *consequence per bit* and *redundancy*. A voice call is the whole purpose. |
| "FEC is expensive so headers get it only because they're short." | Half-right. | Length explains the *cost*, not the *need*: the need is consequence + no ARQ. |
| "Late entry is for monitoring only." | Scanners. | Any roamer, power-cycle or fade recovery needs it. |
| "Repeat the LC every 20 ms for perfect late entry." | Lower latency. | Overhead explodes; there's an optimum (and capacity belongs to voice). |

*Etherstack angle:* reflect the answer in the FEC-chain code of packs 01 and 03 (BCH(64,16) NID, Golay LC, trellis TSBK, Hamming/Golay IMBE).

---

## Q7 — Trace: trunk controller

2 channels; units 1→g10, 2→g20, 3→g30, 4→g10.

| Step | Call | Result |
|---|---|---|
| 1 | `request_call(1,10,false)` | **Granted**, channel 0 |
| 2 | `request_call(4,10,false)` | **Joined**, channel 0 (group already active) |
| 3 | `request_call(2,20,false)` | **Granted**, channel 1 |
| 4 | `request_call(3,30,false)` | **Queued** (both busy) |
| 5 | `release_call(10)` | frees ch 0; promotes **g30 on ch 0** |
| 6 | `request_call(3,30,true)` | **Joined** on ch 0 — group 30 is *already active* (promoted in step 5); the emergency flag **upgrades** the active call |

**Reasoning / trap.** Step 6 looks like "emergency, all channels busy ⇒ preempt", but the controller first asks "does the group already have a call?" (rule 2b). That's the correct
order: otherwise the group could preempt *itself*.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Step 6: Preempted." | Emergency + all busy. | Rule 2b precedes 2d: group is already active ⇒ Joined. |
| "Step 2: Granted ch 1." | New unit = new grant. | One call per group; same channel. |
| "Step 5 returns nullopt." | Release returns nothing. | A non-empty queue promotes its first entry. |
| "Step 5 promotes channel 1." | Both free? | Only ch 0 is freed. |

---

## Q8 — Predict: hang time and starvation

**(a)** G holds the channel through hang 9→12 s. Y was queued at 10 s and is promoted at **12 s** (waited 2 s).
**(b)** A reply at 11.5 s resets hang: talks until 12 s, hang to 15 s ⇒ Y waits until **15 s** (5 s).
**(c)** A reply at 10.5 s talking until 14 s, hang to 17 s ⇒ Y waits until **17 s** (7 s). With conversational group chatter this can repeat indefinitely: **starvation**.
**(d)** Policies: (1) *cap total call duration* per group call (max call time even with replies); (2) *shorten/cancel hang when someone is queued* (queue-aware hang), or when a queue entry's age exceeds a bound; (3)
priority levels / fairness (a recently served group yields). **(e)** Emergency at t = 10 s: no free channel ⇒ preempt. In *this pack's controller* a group in hang is still "active" (until `release_call`), so the victim is the lowest-id non-emergency group — which may be the
talking group on the other channel rather than G in hang. A better real policy drops **hang-time-only** calls first (no one is talking).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Y gets the channel at 9 s." | Talker released. | The controller holds the channel during hang. |
| "Hang time should be 0." | Maximises capacity. | Every reply needs a new grant/queue risk; conversations suffer. |
| "Queue is FIFO so Y can't starve." | Fairness. | FIFO among *queued* entries, but active-group replies keep re-holding the channel. |
| "Emergency waits for hang to end." | Hang is sacred. | Emergency preempts; a hang-only call is the cheapest victim. |

*Etherstack angle:* policy knobs (hang time, max call time, queue-aware hang) are exposed to the operator.

---

## Q9 — Implement: identities and URIs

**Answer (key points).**
- `pack_unit`: validate widths (20/12/24), then **cast to `uint64_t` before shifting**; `unpack_unit` masks and rejects bits above 56.
- URIs use **fixed-width uppercase hex** so the textual form is **canonical** (one string per identity).
- `parse_unit_uri` is strict: exact prefix, exact length (`17 + domain.size()` after the prefix), separators at fixed positions, hex digits only, exact domain; any deviation → `nullopt`.
  Exact domain comparison blocks `iwf.example.evil`; exact length blocks trailing junk.
- Must carry **all three fields**: unit IDs repeat across systems. If two systems both have unit `0x00ABCD`, a URI with only the unit would collide: calls would be
  attributed to, and floor control granted to, the wrong user (an impersonation vector).

**Reference.** See `solutions/lmrsys/iwf_identity.hpp`; tests include 14-hex-digit packing `0xBEE0000100ABCD`, the lowercase-hex acceptance, and eight rejection cases.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "`uint32_t wacn << 36` is fine." | Looks like 64-bit maths. | Shift ≥ type width is UB; cast first. |
| "Parse with `sscanf` and ignore the rest." | Short. | Accepts trailing junk and overflow; untrusted input. |
| "Use decimal and no padding." | Human friendly. | Not canonical; pad/case variants create duplicate identities. |
| "The unit ID alone is unique." | In one system it is. | Not across WACN/System boundaries. |

---

## Q10 — Implement: floor mapping

**Answer.** `PttPress↔FloorRequest`, `PttRelease↔FloorRelease`, `GrantToRequester↔FloorGranted`, `TalkerChangedForOthers↔FloorTaken`, `Deny↔FloorDeny`, `Queued↔FloorQueuePositionInfo`,
`Revoked↔FloorRevoke`, `ChannelIdle↔FloorIdle`. A **bijection** over the eight events in this pack; the test maps all eight, checks 8 distinct targets and the round trip.
Switch statements list **every** enumerator and have **no `default`** so that adding an enumerator triggers `-Wswitch`.

**No single equivalent (VERIFY each against TS 24.380 / TIA-102):**
- **Emergency** — in LMR a flag on the call; in MCPTT an *imminent-peril/emergency state* plus floor *priority*, different semantics.
- **Hang time** — an LMR controller timer; MCPTT has *idle* and inactivity timers; mapping is a timing policy, not a message.
- **Late entry** — LMR repeats parameters; MCPTT late joiners get state via SIP/floor signalling (e.g. the floor arbitrator sends *Taken* with the current talker; VERIFY).
- **Queue position** — MCPTT carries a number; LMR queue response typically doesn't.
- **Talker identity** — LC source ID vs MCPTT floor-granted-party identity: needs the identity mapping of Q9.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Add a `default: return FloorIdle`." | Silences the compiler. | Hides new enumerators; wrong behaviour silently. |
| "`Taken` and `Granted` are the same." | Both mean 'someone is talking'. | `Granted` goes to the *requester*; `Taken` goes to *listeners*. Confusing them tells listeners they can talk. |
| "Mapping is stateless." | Enum to enum. | Per-call state (who holds the floor, queue) is needed to *generate* these events; only the vocabulary mapping is stateless. |

---

## Q11 — Implement: trunk controller

**Answer.** See `solutions/lmrsys/trunking.hpp`. Data structures:

| Rule | Structure | Why |
|---|---|---|
| 1 one affiliation per unit | `unordered_map<unit, group>` | key uniqueness *is* the rule; `affiliate` overwrites |
| 2b/2c/2d active calls | `std::map<group, Active>` | lookup by group; **ordered by group id** ⇒ deterministic victim (first non-emergency) |
| 2e/3 queue | `std::deque<Waiting>` | FIFO, but promotion may take the first *emergency* entry (erase from the middle: O(n), n tiny) |
| free channel | `vector<bool>` built from `active_` | lowest free index; derived, so it can't drift from the call table |

`free_channels() = n − active_.size()` is derived, not stored — storing a counter would be a second source of truth that can desynchronise after preemption.
Pitfalls: erase an iterator **after** choosing it (don't mutate while scanning); preempted channel is *reused immediately* for the emergency call; dedupe the queue; an emergency re-request upgrades a queued entry.
`affiliated_count` is O(#units): acceptable for a model; a production stack keeps a reverse index `group → set<unit>`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use a `vector<bool> busy` and a counter." | Simple. | Two sources of truth; preemption and promotion must update both. |
| "Queue is `std::priority_queue`." | Emergency first. | Loses FIFO among equals unless you add a sequence number; the model is simpler. |
| "Preempt a random call." | Any victim works. | Non-deterministic tests, unfair, unexplainable to operators. |
| "Release should also free the group's affiliations." | Tidy. | Affiliation is unit state; ending a call doesn't change who listens. |

---

## Q12 — Calculate: transcoding and re-framing

- **lcm:** TETRA 30 ms ↔ P25/DMR 20 ms: lcm = 60 ms (2 TETRA = 3 P25 frames). P25 ↔ MCPTT (AMR-WB 20 ms — VERIFY): lcm = 20 ms (frames align one-to-one).
- **Samples at 8 kHz:** 20 ms = 160; 30 ms = 240. (Real transcoders may resample to 16 kHz for wideband codecs; VERIFY.)
- **Re-framing delay (framing only, not codec look-ahead).** An output frame can be emitted only after the input frame containing its last sample has fully arrived.
  In = 30, out = 20: output frames end at 20, 40, 60 ms; the covering input frames end at 30, 60, 60 ⇒ extra delay 10, 20, 0; maximum **20 ms**.
  In = 20, out = 30: outputs end at 30, 60; covering inputs end at 40, 60 ⇒ 10, 0; maximum **10 ms**. General: `in − gcd(in, out)` (30−10 = 20, 20−10 = 10, equal frames ⇒ 0).
  Why: the end-of-output sits at multiples of `out`, the end-of-input at multiples of `in`; the gap `ceil(t/in)·in − t` over multiples of out takes values at multiples of gcd, max `in − gcd`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "No extra delay: just copy frames." | Compressed frames are small. | Different codecs: must decode to PCM, re-frame, re-encode. |
| "Delay = lcm = 60 ms." | The alignment period. | lcm is the *pattern period*; the delay within it is bounded by `in − gcd`. |
| "20 ms ↔ 30 ms the same in both directions." | Symmetry. | `in − gcd` isn't symmetric (20 vs 10 ms). |
| "Transcoding is lossless." | Digital = exact. | Lossy codecs: tandem coding degrades quality. |

*Etherstack angle:* jitter-buffer sizing and "mouth-to-ear" budget.

---

## Q13 — Find the bug

| # | Bug | Symptom | Fix |
|---|---|---|---|
| 1 | `wacn << 36` with 32-bit `wacn`; `sysid << 24` is an `int` shift | UB/garbage high bits; `-fsanitize=undefined` flags shift exponent too large; also no width checks | cast each field to `uint64_t` first; validate 20/12/24-bit ranges |
| 2 | `char b[16]` too small; `%d` with unsigned types; decimal, unpadded; no domain; no `snprintf` | ASan stack-buffer-overflow; non-canonical, duplicated URIs | `snprintf` into an adequate buffer, `%05X-%03X-%06X`, append `@domain` |
| 3 | `same_user` compares only `unit` | two systems' users collapse into one; wrong floor-holder mapping | compare all three fields (or the packed 56-bit value) |
| 4 | switch missing enumerators with a catch-all `return FloorGranted` | `Deny`, `Taken`, `Idle` all become `Granted` ⇒ listeners are told they have the floor | list every enumerator, no `default`, throw after the switch |
| 5 | `find("p25-")` + `stoul` of the rest | accepts garbage, parses decimal, throws `std::invalid_argument`/`out_of_range` on attacker input, ignores domain, builds `Id` with one field | strict fixed-position parse returning `nullopt` |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Bug 4 is just style." | Compiles. | It is a **safety** bug: wrong floor state to radios. |
| "Bug 2: just enlarge the buffer." | Quick. | Format still non-canonical and `%d` on unsigned is wrong. |
| "Bug 5: wrap in try/catch." | Handles exceptions. | Still accepts malformed prefixes; validation, not exception handling, is the fix. |

---

## Q14 — Predict: encryption

(a) **One** plaintext bit (OFB: keystream is independent of the ciphertext). (b) Garbage (or silence/mute): without ALGID/KID/MI the receiver can't generate the keystream; it should **not** output noise as audio, so stacks mute until the ES arrives (the late-entry delay of Q5).
(c) With the same (key, MI): `C1 ⊕ C2 = P1 ⊕ P2` (§4(d): `0x03`); known/guessable plaintext (call-signs, silence frames, headers) reveals the other call. (d) DES: 2⁵⁶ = 7.2·10¹⁶; ÷10¹² = 72 058 s ≈ **20 h** (illustrative rate; real attacks are cheaper with dedicated hardware).
AES-256: 2²⁵⁶ ≈ 1.2·10⁷⁷ — not feasible. (e) OTAR protects **traffic keys in transit** by encrypting them under a per-radio **KEK**; the KEK (and the KMF that holds it) is the most valuable secret: compromise it and all future rekeyed traffic keys fall.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "A bit error garbles the whole frame." | Block ciphers diffuse. | OFB keystream does not depend on ciphertext; error stays 1 bit. |
| "MI is secret." | It's an IV. | IVs are public; they must be **unique**. |
| "DES-OFB with a good MI is fine." | OFB is sound. | 56-bit key is the weakness. |
| "OTAR sends keys in clear to speed up." | Fast. | Defeats the purpose; keys under KEK. |

*Etherstack angle:* a stack's key-management interface (key store, MI generator, OTAR client) is a high-stakes surface; use vetted crypto libraries.

---

## Q15 — Connect: designing the gateway

**Boxes.** LMR system (P25 RFSS/CSSI or DFSI endpoint — VERIFY interface) ↔ **IWF** ↔ MCPTT server (SIP/RTP) ↔ LTE/5G UEs. IWF internals: *identity mapper* (Q9), *floor translator* (Q10),
*transcoder pool* (Q12), *key-handling module/HSM*, *call-state/timer service*, *OAM/logging*.

| Mapping | LMR side | MCPTT side | Notes |
|---|---|---|---|
| Identity | WACN+SysID+unit / talkgroup | MCPTT ID, SIP URI | injective; table kept by the IWF |
| Affiliation | group affiliation | group membership/affiliation | IWF acts as one 'pseudo-unit' per bridged group, or as a CSSI/ISSI subscriber group — VERIFY |
| Floor | request/grant/queue/deny/release | Floor Request/Granted/Taken/Deny/Release/Idle | timing differs; mapping in Q10 |
| Voice | IMBE/AMBE+2/ACELP | AMR-WB/EVS | transcode; PCM pivot |
| Emergency | flag + priority | emergency state + floor priority | semantic mapping policy |
| Security | ALGID/KID/MI, OTAR | MC security / group master key | IWF decrypts/re-encrypts or refuses |

**Illustrative latency budget (VERIFY all; 3GPP sets MCPTT access and mouth-to-ear targets, TS 22.179):**

| Stage | ms (illustrative) |
|---|---|
| LMR air + CC grant (first voice) | 200–500 |
| Re-framing (Q12) | 10–20 |
| Codec look-ahead + transcode compute | 10–30 |
| IWF jitter buffer | 40–60 |
| IP backhaul + LTE access | 50–100 |
| **Sum (voice path after grant)** | ≈ 110–210 |

The floor-grant path is the long pole; the voice path must fit the user-perceived budget (≈ 300 ms mouth-to-ear for MCPTT — VERIFY).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Map only identities and voice." | Visible things. | Floor semantics and security decide whether it works. |
| "The IWF can just forward RTP." | Both use RTP. | Different codecs, framing, and keys. |
| "The IWF can be stateless." | Stateless is scalable. | Floor/queue/call state must live somewhere. |

---

## Q16 — Interview: E2E encryption plus interworking

**Answer.** "They're in tension: **E2E means only the two endpoints hold the keys; a transcoder needs plaintext, so the gateway becomes an endpoint.**" Offer options:
(1) **Trusted IWF** — decrypt on one side, re-encrypt on the other; the IWF holds both key sets, so it must be hardened, HSM-backed, audited, and the customer must accept it as a trust boundary.
(2) **No transcoding** — if both sides can use the **same codec payload and a shared key** (rare), the gateway can relay ciphertext only re-wrapping signalling.
(3) **Clear bridging for non-sensitive groups** only, by policy. (4) **Segregate**: sensitive groups stay LMR-only. Recommend (1) or (4) after a **threat model**: the gateway is the highest-value
target in the system. Also mention key hierarchy (KEK/OTAR on LMR, MC security on 3GPP — TS 33.180, VERIFY), and logging.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "No problem; encrypt the RTP and carry it through." | Sounds simple. | Needs identical codec, framing and key; and then there is no transcoding. |
| "Refuse: E2E and interworking can't coexist." | Purist. | They can, with an explicit trust decision at the gateway. |
| "Put the keys on a normal server." | Cheap. | The IWF key store is the crown jewel; HSM/least-privilege. |

---

## Q17 — Interview: simulcast vs voting

**Answer.** "Simulcast is **downlink**: multiple transmitters on one frequency sending the same bits at the same time, so one frequency covers a wide area; it needs GPS-disciplined timing, equal audio delay, and
delay-equalisation (Q3). Voting is **uplink**: several receivers decode the same weak handheld transmission and a comparator picks the best frame (Q4). You need **both** when the uplink is the limiting link —
which it usually is, because the handheld is 100× weaker than the tower — and you want one frequency across the area: *simulcast downlink, voted uplink*." Mention the limits: ¼-symbol delay budget,
independence caveat, back-haul synchronisation, and the null where equal signals meet.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "They're alternatives." | Both multi-site. | Opposite directions; complementary. |
| "Voting needs GPS timing." | Simulcast does. | Voting needs frame alignment of received copies, not transmit-time sync. |
| "Simulcast gives frequency reuse." | Wide area, one frequency. | It uses *one* frequency across the cell cluster; it's the opposite of reuse. |

---

## Q18 — Interview: trunking for a 6-channel fleet

**Answer.** Quantify first. Assume six groups of 0.5 Erlang (3 E total, the same traffic). Conventional: each group on its own channel: **33.3 %** blocking. Trunked with one channel as CC (5 voice): `B(3,5) = 0.1101` ⇒ **11 %**.
Trunked with CC integrated (6 voice, e.g. a composite or time-slotted CC — VERIFY): `B(3,6) = 0.0522` ⇒ **5.2 %**.
(Recursion: B1=0.75, B2=0.5294, B3=0.3462, B4=0.2061, B5=0.1101, B6=0.0522.) Trunking cuts blocking 3–6×. Then the **non-numeric** side: all radios must be trunking-capable (replacement cost), controller/site equipment, training, failsoft behaviour, and the possibility that adding two more **conventional** channels or scan lists
costs less if traffic is bursty and the fleet is small. Also note Erlang-B assumes blocked calls are cleared; a trunked system with **queueing** (Erlang-C) turns blocking into waiting.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Always recommend trunking." | It's the vendor's product. | CC tax and radio replacement cost may outweigh the benefit for small fleets. |
| "Trunking only helps big networks." | Common claim. | 6 channels already show a 3× gain; but the break-even depends on traffic. |
| "Use the average load per channel." | 0.5 E per channel looks fine. | Averages hide burstiness — the whole point of Erlang-B. |
