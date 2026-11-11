# Pack 06 — Answers (builds)

Format: **Answer → Reasoning → Wrong answers (wrong | why tempting | why wrong) → Etherstack angle.**
Synthetic data only; never live traffic; no ciphers. **VERIFY** = check TIA-102.BAAA / ETSI TS 102 361-1 / TIA-102.AABC before quoting.

---

## Q1 — Explain: why Golay(23,12) is perfect

**Answer.** The Hamming spheres of radius 3 around the 2¹² = 4096 codewords exactly tile the space of 2²³ words: each sphere holds
`1 + 23 + C(23,2) + C(23,3) = 1 + 23 + 253 + 1771 = 2048 = 2¹¹` words and `4096 × 2048 = 2²³`. Equivalently, the 2048 possible syndromes are in one-to-one correspondence with the 2048 error patterns of weight ≤ 3.

**Reasoning.** (1) Two different patterns of weight ≤ 3 with the same syndrome would differ by a *nonzero codeword of weight ≤ 6*; the minimum weight is 7, so impossible ⇒ all 2048 syndromes are distinct. (2) There are only 2048 syndromes, so each
is hit exactly once ⇒ every received word is within 3 of exactly one codeword. **Decoder consequence:** syndrome → pattern → XOR; it can **never** report failure. With 4+ errors it *silently miscorrects* to a different codeword (test: all 8855 four-error patterns miscorrect).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Perfect = corrects any number of errors." | The word 'perfect'. | Perfect = no wasted syndromes. Beyond radius 3 it miscorrects. |
| "It's perfect because d = 7." | d = 7 gives t = 3. | Many d = 7 codes exist; perfection is the *counting identity* 2048 = 2¹¹. |
| "BCH(63,16) is perfect too." | Also a cyclic code. | Its radius-11 balls cover a vanishing fraction of 2⁶³. |

*Etherstack angle:* P25 LC and DMR slot-type/embedded words use Golay-family codes (VERIFY which); knowing the failure mode (silent miscorrection) is why designers add CRCs or extra parity.

---

## Q2 — Calculate: Golay by hand

**Answer.** `encode(2) = 0x149F`, weight **8**. Powers of *x* mod *g* (shift left; if bit 11 set XOR 0xC75): x¹¹ = `0x475`, x¹² = `0x49F`, x¹³ = `0x54B`, x¹⁴ = `0x6E3`, x¹⁵ = `0x1B3`.
Syndromes of single errors: bit *i* < 11 → `1<<i`; bit 11 → `0x475`; 12 → `0x49F`; 13 → `0x54B`; 14 → `0x6E3`; 15 → `0x1B3`.
Errors at 15 and 0: syndrome `0x1B3 ^ 0x001 = 0x1B2` → table gives pattern `{15, 0}` → corrected.

**Reasoning (encode 2).** info 2 → `2·x¹¹` = `0x1000` (bit 12). XOR `g<<1 = 0x18EA` → `0x08EA`; bit 11 set → XOR `0xC75` → `0x049F` (the remainder). Codeword = `0x1000 | 0x049F = 0x149F`.
Cross-check by linearity: `encode(3) = x·g = 0x18EA` (info part `0x18EA >> 11 = 3`), `encode(1) = g = 0xC75`; `encode(2) = encode(3) ^ encode(1) = 0x18EA ^ 0x0C75 = 0x149F` ✔. Weight of `0x149F`: `1+1+2+4 = 8`; `g = 0xC75` has weight `2+3+2 = 7` = d_min ✔.
Decode a word `0x149F ^ 0x8001 = 0x949E`: syndrome = remainder(0x8001) = `0x1B3 ^ 0x001 = 0x1B2`; table[0x1B2] = `0x8001`; corrected `0x149F`; data = `0x149F >> 11 = 2` ✔.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "encode(2) = encode(1) << 1 = 0x18EA." | Doubling the info doubles the codeword. | Shifting the codeword left *shifts the info too*: `0x18EA` carries info 3 (x·g), not 2. Systematic encoding puts info in the top 12 bits. |
| "Syndrome of bit 15 = 1<<15." | Bit positions equal values for low bits. | Only bits below 11 are their own remainder; high bits reduce mod g. |
| "Weight of 0x149F is 7." | d_min is 7. | It's 8; weight 7 words are among the 253 of weight 7 but this is not one. |

*Etherstack angle:* being able to run the algorithm without a lookup table is the whiteboard check.

---

## Q3 — Prove: Golay(24,12) corrects 3 and detects 4

**Setup.** Word = (23-bit part r, parity bit p). Decode: `d = golay23_decode(r)` with `e = d.errors ≤ 3`, corrected 23-bit word `c'`; expected parity `q = parity(c')`; `m = [q ≠ p]`; `total = e + m`; accept iff total ≤ 3.

**Claim 1 (≤ 3 errors corrected).** Let the transmitted word be (c, parity(c)) and *j* errors land in the 23-bit part and *k ∈ {0,1}* in the parity bit, `j + k ≤ 3`. Then `j ≤ 3`, so the perfect-code decoder removes them exactly: `c' = c`, `e = j`; `q = parity(c)`, and `p = q ⊕ k`, so `m = k`. Total = `j + k` ≤ 3, accepted and correct ✔.

**Claim 2 (4 errors always detected).** Four errors split as (4 in the 23 part, 0 in parity) or (3, 1).
- *(3, 1):* decoder returns `c' = c` with `e = 3`; the parity bit is wrong ⇒ `m = 1` ⇒ total = 4 ⇒ rejected ✔.
- *(4, 0):* the received 23-bit word is at distance 4 from `c`; the perfect decoder finds a codeword `c'` at distance ≤ 3, necessarily `c' ≠ c` (otherwise the pattern would have weight ≤ 3). Then `7 ≤ d(c,c') ≤ d(c,r) + d(r,c') ≤ 4 + 3 = 7`, so `d(c,c') = 7` and `e = 3`.
  Two codewords at *odd* distance have **different parities** (parity(c) ⊕ parity(c') = parity of weight(c ⊕ c') = 1), while the received parity bit equals parity(c). So `q ≠ p`, `m = 1`, total = 4 ⇒ rejected ✔.
Both cases give total ≥ 4 ⇒ **uncorrectable**. (Test: all 10 626 four-error patterns are detected; all 2325 patterns of weight ≤ 3 are corrected.)
**Not claimed:** 5+ errors may miscorrect (e.g. 5 errors can look like 3).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "d = 8 means it detects 7 errors." | The detection-only rule `d − 1`. | With correction enabled the radius is spent: correcting *t* and detecting *s* more needs `d ≥ t + s + 1`, so t = 3 gives s = 4 — exactly what the proof shows. |
| "Threshold `total > 4`." | Matches '4 errors'. | Total 4 must be rejected: `total ≥ 4`. |
| "Compute the parity of the *received* 23 bits." | They're the bits you have. | Parity must be taken on the **corrected** word; otherwise m is off whenever e is odd (Q13). |
| "5 errors are detected too." | Monotone thinking. | 5 errors can be at distance 3 from another codeword in the 24-bit code; not guaranteed. |

*Etherstack angle:* if a stack must decide "use or drop", detection of the first uncorrectable case is what prevents garbage reaching call control.

---

## Q4 — Implement: Golay

**Answer.** See `solutions/lmrb/golay.hpp`.
- `golay_remainder`: mask to 23 bits; for *i* = 22 … 11, if bit *i* set, XOR `g << (i − 11)`.
- `golay23_encode`: mask info to 12 bits, shift left 11, OR the remainder (systematic).
- `golay_pattern_table`: enumerate weights 0–3 (the zero pattern is the default entry); store each at `table[remainder(pattern)]`.
- `golay23_decode`: XOR the table pattern for `remainder(received)`.
- `golay24_encode`: `(cw23 << 1) | (popcount(cw23) & 1)`; `golay24_decode`: Q3 algorithm with the corrected word's parity; `total ≥ 4` ⇒ Uncorrectable.

**Which tests catch what.** *Wrong generator:* `GeneratorDividesItself`, `AllCodewordsAreDivisible…`, **weight distribution** (1, 253, 506, 1288, 1288, 506, 253, 1) and **min weight 7**. *Missing mask:* `RemainderIgnoresBitsAbove23`, `KnownEncodings` (0x1000 case). *Off-by-one threshold:* `CorrectsThreeAndDetectsEveryFour` (4-error cases).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Build the table lazily on demand." | Saves startup. | The 2048-entry table is tiny; lazy init adds thread-safety questions. (A function-local `static` is thread-safe in C++11 and later.) |
| "Brute-force the nearest codeword." | Always correct. | 4096 distance computations per word — fine for a test oracle, wrong for a decoder in a receive path. |
| "Skip the zero pattern." | It's 'no error'. | Syndrome 0 must map to pattern 0, and the array default of zero provides it — an explicit test checks it. |

---

## Q5 — Explain + Calculate: sync first, then strip

**(a) Answer.** Status symbols are positioned by a counter that starts at the **frame start**, which is exactly what the sync tells you. Before sync you don't know the phase of the status symbols, so you can't strip them.
Searching **raw** symbols works because the first status symbol of a frame arrives after **35** symbols, but the sync is only **24** symbols: it is never interrupted.
**(b)** `air = k + k/35` (integer division): *k* = 0 → 0; 34 → 34; 35 → 36 (air 35 is status); 70 → 72 (air 71 is status); 839 → 862 (air 863 is the 24th status symbol).
LDU: 24 + 32 + 784 = 840 data symbols; 840/35 = 24 status; air = 864 symbols = 1728 bits = 180 ms ✔.
**(c)** Stripping with the phase off by one removes a **data** symbol and keeps a **status** symbol: everything after the first misplaced removal is shifted by one symbol, so the NID decodes as garbage (bits shifted by 2) and the frame is lost. A shift of one symbol turns a codeword into a word far from any codeword.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Strip status symbols from the start of the stream." | Simple. | The counter starts at the frame start, not the stream start. |
| "Status symbols are every 36 data symbols." | '36' is in the standard. | Every 36th **air** symbol ⇒ after every 35 **data** symbols. |
| "Search sync after stripping because status symbols could corrupt it." | Sounds careful. | They can't: sync lies within the first 24 < 35 symbols. |

*Etherstack angle:* the first line of any P25 receiver; off-by-one here is a classic interview trap.

---

## Q6 — Calculate: NID candidate matching

**(a) Answer.** The 64-bit NID code (BCH(63,16) + overall parity) has minimum distance ≥ 24. If a received word is within 11 bits of some codeword *c*, then for any other codeword *c′*: `d(r,c′) ≥ d(c,c′) − d(r,c) ≥ 24 − 11 = 13 > 11`. So the nearest valid candidate within 11 is **unique**, and picking it is exactly bounded-distance decoding. Restricting the candidates to *known NACs × 7 DUIDs* is a sub-codebook: the same argument holds and extra prior knowledge reduces false accepts.
**(b) Cost.** One NAC: 7 encodes (each a short division). All NACs: `7 × 4096 = 28 672` encodes.
**(c) False accept** for a random 64-bit word, one NAC (7 candidates): at tolerance 11: `7 × Σ_{k≤11} C(64,k) / 2⁶⁴ = 7 × 9.28·10¹¹ / 1.845·10¹⁹ ≈ 3.5·10⁻⁷`; at tolerance 5: `7 × 8 303 633 / 1.845·10¹⁹ ≈ 3.2·10⁻¹²`.
**With "all NACs"** the candidates multiply by 4096: ≈ 1.4·10⁻³ at tolerance 11 (0.14 % of random NIDs would be accepted) and ≈ 1.3·10⁻⁸ at tolerance 5. Hence a receiver accepting *any* NAC should use a tighter tolerance.
**(d) Ambiguity** can only occur if the best distance is ≥ 12 (two candidates at equal distance *a*: `2a ≥ 24`); we report **not ok** on ties (comparing `(nac, duid)`, so a NAC listed twice isn't a false tie).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Matching is slower, so implement full BCH decoding." | Textbook. | 7 encodes is trivial; BCH decoding (syndromes, Berlekamp–Massey, Chien) adds code and bugs with no benefit when the candidate set is tiny. |
| "Accept the first candidate under the tolerance." | Early exit. | Not necessarily the closest; ambiguity undetected. |
| "Tolerance 11 is always right." | It is t. | It maximises sensitivity and false accepts; choose by context (single NAC vs any NAC). |
| "Distance to the code is 23." | Designed distance of BCH. | The extended (parity) code is 24; the 64-bit NID includes the parity bit. |

---

## Q7 — Find the bug (parser)

| # | Bug | Symptom | Fix |
|---|---|---|---|
| 1 | Strip status symbols from index 0 before searching sync | status phase wrong ⇒ frames at arbitrary offsets are corrupted; works only when the frame happens to start at stream index 0 | find sync on raw symbols, strip relative to the hit |
| 2 | `rel % 36 == 0` | removes the first data symbol and keeps the real status symbol at index 35: everything shifted | status iff `(rel + 1) % 36 == 0` |
| 3 | `v = (v >> 2) or s` in `symbols_to_bits` | LSB-first packing; NID bits reversed ⇒ no candidate matches | `v = (v << 2) or (s and 3)`, first symbol most significant |
| 4 | `decode_nid` returns the first candidate within tolerance | wrong NAC/DUID chosen when two candidates are inside tolerance; no ambiguity reporting | scan all candidates, keep the best, require uniqueness |
| 5 | `payload.resize(784)` on a short frame | zero-padded truncated frames are accepted as valid LDUs; downstream decoders see zeros | if `payload.size() < expected` return nullopt, otherwise truncate |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Bug 5 is harmless; zeros are just silence." | Zeros look benign. | A truncated frame must be a *loss event* (counted, concealed), not 'audio'. |
| "Bug 2 is a style difference (0- vs 1-based)." | Off-by-one feels minor. | It drops data and keeps status — total loss. |
| "Bug 1: strip then re-find sync." | Self-correcting? | Re-finding sync on a shifted stream finds the sync only if it wasn't hit by the removal; phase still unknown. |

---

## Q8 — Implement: parser

**Answer.** See `solutions/lmrb/p25_parser.hpp`. `parse_frame` loops: `find_sync` (tolerant, raw) → `strip_status_symbols(hit.pos)` → length check (≥ 56) → `decode_nid(symbols_to_bits(data, 24, 32), nacs, 11)`;
on failure `pos = hit.pos + 1` (false sync); on success take `data[56…]` as payload, truncate to the fixed size for LDUs or return nullopt if shorter.
Design notes: **truncation returns nullopt immediately** (a later sync hit can only have less data); the tests pin down false sync, noisy sync (2 errors), noisy NID (4 errors), wrong NAC, missing sync, tiny and truncated frames, trailing junk, and a second frame via `from`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "On NID failure, return nullopt." | Simplest. | A false sync would hide the real frame right behind it. |
| "Retry from `hit.pos + 24`." | Skip the sync. | A real frame could start inside the false sync region (overlap). `+1` is safe. |
| "Throw on malformed input." | Clear error. | Input arrives from noisy channels; failure is normal, return values not exceptions. |

---

## Q9 — Trace: DMR splitter

**(a)** Slot 6 starts at `100 + 6·288 = 1828`; its middle starts at `+24 + 108 = +132` ⇒ bit **1960** (to 2007). Sync at 1960 ⇒ phase `(1960 − 132) mod 288 = 1828 mod 288 = 100` (1828 = 6·288 + 100).
**(b)** Phase 100, slot 1 starts at `388`; burst at `412`. Voice frame 1 = burst bits `72…107` ⇒ stream `484…519` and burst bits `156…191` ⇒ stream `568…603` (36 + 36 = 72 bits). Frame 0 = stream `412…483`; frame 2 = burst `192…263` ⇒ stream `604…675`.
**(c)** See `solutions/lmrb/dmr_splitter.hpp`: `std::copy` ranges cut at 108 and 156; join inverts; `split_stream` loops `k` while `phase + (k+1)·288 ≤ size` (partial tail ignored) and classifies `pack_bits48(mid)`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Middle starts at bit 108 of the slot." | 108 is the half. | The slot begins with a 24-bit gap/CACH: middle at 24 + 108 = 132. |
| "Frame 1 = bits 72–143 of the burst." | Contiguous 72 bits. | The middle (108–155) intervenes: frame 1 straddles it. |
| "Phase = sync index." | Where the sync is. | Subtract the sync's offset inside the slot (132) and reduce mod 288. |

---

## Q10 — Calculate: slot alignment

**(a)** `P(hit) = 4 × Σ_{k≤4} C(48,k) / 2⁴⁸ = 4 × 213 053 / 2.815·10¹⁴ = 852 212 / 2.815·10¹⁴ ≈ 3.0·10⁻⁹` (Σ = 1 + 48 + 1128 + 17 296 + 194 580).
**(b)** 288 phases × 12 slots = 3456 trials ⇒ expected false hits ≈ 1.0·10⁻⁵ — and the true phase gets 2 hits in the test.
**(c)** A sync burst appears once per superframe (6 bursts of a logical channel = **360 ms**, pack 03), so worst-case acquisition ≈ 360 ms + 30 ms burst time ≈ 390 ms for voice; data bursts carry sync more often (VERIFY).
**(d)** "Most hits" uses all evidence; "ties → smallest phase" is just determinism. Weakness: with a **single** sync in the buffer, a false phase with one random hit *ties* the true phase (probability ~10⁻⁵) and the smaller phase wins, possibly the wrong one. Mitigations: require ≥ 2 hits (spaced by multiples of 6 slots), or validate the candidate phase with slot-type Golay/CC checks.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Check only phases that contain a sync at the first slot." | Cheaper. | The sync may be in slot 3; you'd miss it. Scan all 288 phases. |
| "Use tolerance 10 for robustness." | Noisy channel. | False-hit probability grows ≈ ×10⁵ per 3 bits of extra tolerance; and the four words are only 12–24 bits apart, so ties appear. |
| "Alignment is needed only once." | It's a one-off. | Re-align after loss of signal or timing drift. |

---

## Q11 — Trace: floor FSM

Script (§4(d)) results: Granted (talker 1, Active) → Denied → Released (prev 1, Hang) → none → Granted (talker 2) → Preempted (prev 2, talker 3, emergency) → Released (Hang) → HangExpired (Idle).
Extra: after step 5 (`press(2)` from Hang → Active, talk timer reset to 0) a `tick(60 000)` reaches the limit exactly ⇒ **TalkTimeout**, previous 2, state Hang.
**Implementation.** See `solutions/lmrb/floor_control.hpp`: a `switch` on state inside `press`; `release` acts only for the talker in Active; `tick` increments the matching timer and performs at most one transition; `enter_hang()` zeroes talker, emergency flag and both timers.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Step 6 is Denied: someone is talking." | Active ⇒ busy. | Emergency over a non-emergency call preempts. |
| "Step 5 needs a new channel grant." | New talker. | Reply inside hang time reuses the held channel — that's the purpose of hang time. |
| "Timeout needs `>` the limit." | 'exceeds'. | Reaching the limit (`≥`) is the contract: tests tick exactly 10 × 100 ms. |

---

## Q12 — Predict: timer/event seams

**(a)** *Timer first:* tick expires hang → Idle, then `press` is a fresh **Granted from Idle** — the system must request a new channel (possibly queued; pack 05). *Press first:* the reply arrives in hang → Granted **reusing the channel**. Both return `Granted` to the FSM caller, but the *system consequence* differs. Rule of thumb: process **input events before timers** at the same instant (favour the user, avoid needless grants); document it.
**(b)** `tick(10 000)` in Active with limit 1000: one transition ⇒ **TalkTimeout**, state Hang, remaining 9000 ms is **dropped**. The next `tick(10 000)` ⇒ **HangExpired**, Idle. So big ticks lose time; either tick in small steps or loop over the remainder.
**(c)** The timed-out talker presses again: state Hang ⇒ `Granted` immediately — the **timeout is defeated** by a stuck PTT. Harden: remember `locked_out = unit` and deny presses from it until a `release` from that unit is seen (key-up edge).
**(d)** A late `release` after TalkTimeout hits Hang ⇒ `None`; with the hardening it also clears the lock-out.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "The tick order doesn't matter, both give Granted." | Same action. | System behaviour (new grant vs reused channel) differs. |
| "tick(10 000) drives Active all the way to Idle." | Time passed. | One transition per tick by contract; pending time dropped. |
| "A stuck PTT is the radio's problem." | Hardware fault. | The system must contain it (timeouts + lock-out). |

---

## Q13 — Find the bug (Golay)

| # | Bug | Symptom | Fix |
|---|---|---|---|
| 1 | loop `i > 11` stops before bit 11 | remainders can have bit 11 set (12-bit values): codewords not divisible; syndromes ≥ 2048 ⇒ out-of-bounds table index (ASan error) | loop `i >= 11` |
| 2 | `info << 11` without masking to 12 bits (and `uint16_t` promoted to `int`) | stray high bits ⇒ codeword wider than 23 bits; garbage remainders | `(uint32_t(info) and 0xFFF) << 11` |
| 3 | table build writes only weight-3 patterns | weights 0–2 map to the zero pattern ⇒ single/double errors not corrected; `SyndromeTableIsPerfect` fails | write patterns for weights 1, 2, 3 (0 by default) |
| 4 | reject only if `total > 4` | 4-error patterns are accepted as "Corrected" ⇒ silent miscorrection; `CorrectsThreeAndDetectsEveryFour` fails | reject if `total >= 4` |
| 5 | parity computed on the **received** 23 bits | parity mismatch counted wrongly when the 23-bit part had an odd number of errors: 3 correctable errors look like 4 and are rejected | parity of the **corrected** codeword |

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Bug 1 is cosmetic." | Loop bound. | It breaks the algorithm's invariant (remainder < 2¹¹). |
| "Bug 5: use the received parity, it's the same thing." | Parity of the same bits. | Errors change the parity; the check compares to the *corrected* word. |
| "Fix bug 4 by lowering the correction capability." | Safer. | The code corrects 3; the threshold must match the proof (Q3). |

---

## Q14 — Connect: synthetic test bench

**Data flow (all synthetic):**
`generator (seeded RNG + frame/slot builders)` → `noise injector (bit flips by BER or fixed patterns)` → `parser/splitter (find sync, strip, cut)` → `FEC (BCH/Golay decode of NID, LC, slot type)` → `field extraction (NAC, DUID, talker, group)` → `call control (floor FSM from this pack, trunk controller from pack 05)` → `event log`.
**Interfaces:** `parse_frame(symbols, nacs, tol) → ParsedFrame`; `golay24_decode(word) → status/data`; `FloorControl::press/release/tick`.
**End-to-end assertions:** (1) *No-noise*: a script of N calls produces exactly the expected log of floor actions. (2) *Noise within correction*: same log. (3) *Noise beyond correction*: frames are dropped with **counters** incremented, never mis-decoded as valid; the FSM's timers eventually return it to Idle (talk timeout/hang). (4) *Invariants*: at most one talker; talker ≠ 0 iff Active; no panic under ASan/UBSan across seeds.
**Seam to design:** the parser delivers **events at frame granularity**, the FSM is driven by **ticks** — an adapter converts 180 ms LDUs into `press`/`tick(180)` events and a missing frame into a loss counter.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Test with a captured recording." | More realistic. | Not allowed here (legal/safety); synthetic frames let you control and repeat every scenario. |
| "One big test." | Simple. | Failures are undiagnosable; layered tests + counters localise. |
| "Skip the noise injector." | Clean frames are enough. | Noise handling is most of the work. |

---

## Q15 — Interview: Golay

**Answer.** "Golay(23,12) is a binary cyclic code with generator g(x) = x¹¹ + x¹⁰ + x⁶ + x⁵ + x⁴ + x² + 1. It's **perfect**: the 2048 error patterns of weight ≤ 3 match the 2048 syndromes, so a table decodes every word and corrects up to 3 errors.
Because it's perfect it can **never detect** failure: 4 errors miscorrect. Adding an overall parity bit gives Golay(24,12,8): correct 3, **detect 4** — decode the 23-bit part, compare the parity with the corrected word, and reject if `e + m ≥ 4`. In P25/DMR such codes protect small, critical fields (LC, slot type — VERIFY)."
Offer the 0xC75 example (encode(1) = g) to show you can compute.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "It corrects 3 and detects 4." (for the 23-bit code) | Memorised line. | That's the **extended** code; the 23-bit perfect code detects nothing beyond 3. |
| "It's a Reed–Solomon code." | Famous FEC. | Binary BCH-type cyclic code; not Reed–Solomon. |

---

## Q16 — Interview: 30 % frame loss on a noisy bench

**Answer.** "Instrument each stage with counters: *sync missed*, *NID failed*, *wrong NAC*, *truncated*, *fixed-size violated*, then compare with the **binomial prediction** for the injected BER."
**Worked prediction.** Per-bit error rate p = 7 %: errors in the 48-bit sync ~ Binomial(48, 0.07): P(0…4) = 0.031, 0.111, 0.196, 0.226, 0.192 ⇒ P(≤ 4) ≈ **0.756** ⇒ ≈ **24 %** of frames lost *at the sync stage alone*, with tolerance 4. NID (64 bits, mean 4.5 errors, tolerance 11) loses ≈ 0.4 %. So the bottleneck is the **sync tolerance**, not the FEC.
**Fix:** widen sync tolerance — at 8 errors false syncs on random data are `465 174 935 / 2.8·10¹⁴ ≈ 1.7·10⁻⁶` per position (one per ~2 minutes at 4800 symbols/s), but each is filtered by the NID check (3.5·10⁻⁷ false accept), so system-level false frames become astronomically rare. Re-measure.
**Other checks:** wrong status phase (a systematic 100 % loss, not 30 %), symbol-timing slips in the injector, truncation by the test harness, endianness of packing.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Add more FEC." | FEC fixes errors. | The loss is before FEC: sync detection. |
| "Raise the NID tolerance." | NID is the visible failure. | NID rarely fails at 7 % BER; the sync does. |
| "Noise is random, nothing to do." | Fatalism. | The loss rate is *predictable*; mismatch with the prediction localises bugs. |

---

## Q17 — Interview: floor-control FSM and failure modes

**Answer.** Present the table (§3.5) in thirty seconds: three states, three events; one talker; grants, duplicates, denials; hang for replies; emergency preemption with `emergency` as a per-call flag; time as an input.
**Failure modes to volunteer:** (1) **stuck PTT** ⇒ talk timeout *and* lock-out until release (Q12); (2) **lost release** (radio leaves coverage) ⇒ talk timeout/loss-of-signal detection; (3) **tick granularity** and ordering of timers vs events (Q12); (4) **hang-time starvation** of queued groups (pack 05 Q8); (5) **emergency semantics**: who can preempt, can emergency be preempted, does emergency ignore talk limit? (policy; VERIFY); (6) **identity 0** reserved as 'nobody'; (7) **duplicate/late messages** over lossy links.
**Testing:** every table cell has a test; model-based random walks check invariants (one talker; talker ≠ 0 iff Active).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "It's a switch statement." | Implementation detail. | Design = the table; code follows. |
| "Preemption is always fair." | Emergency = good. | Preemption drops someone's call: needs authorisation, logging, and a policy for emergency vs emergency. |
| "Timers are wall-clock." | Natural. | Make time an input for deterministic tests. |

---

## Q18 — Interview: testing and hardening without live traffic

**Answer.** "First, I *don't* need live traffic and shouldn't use it: receiving public-safety traffic may be unlawful, and encrypted traffic is off-limits. The goal is correctness against the **spec**, not against recordings." Then:
1. **Hand-computed golden vectors** (0xC75, 0x149F, 0xBEE0000100ABCD style) and vectors from the standards where published (VERIFY).
2. **Exhaustive tests** where the space is small (4096 Golay words; every ≤ 4-bit error pattern).
3. **Property/round-trip tests**: `decode(encode(x) ^ e) == x` for `weight(e) ≤ t`; `join(split(x)) == x`.
4. **Synthetic generators with seeded RNGs**, injectable noise and a *BER sweep* compared with binomial predictions (Q16).
5. **Sanitizers** (ASan/UBSan) in CI, plus **fuzzing** the parser and splitter with random symbol/bit streams (invariant: never crashes, never returns an oversized payload).
6. **Differential testing**: table decoder vs brute-force oracle.
7. **Model-based tests** for the FSM.
8. **Vendor/customer test vectors** supplied under agreement for conformance, not scraped from the air.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Capture some local traffic with an SDR." | Easiest 'real' data. | Possibly illegal, and often encrypted/personal; not reproducible. |
| "Unit tests are enough." | They pass. | Parsers need fuzzing and noise sweeps; bit-twiddling needs sanitizers. |
| "Copy a decoder's code to compare outputs." | Quick oracle. | Copyright/licence obligations; study structure, write your own and test against the spec. |
