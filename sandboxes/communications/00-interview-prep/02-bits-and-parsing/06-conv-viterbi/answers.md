# Pack 06 — Answers (Convolutional codes, Viterbi, interleaving, LFSR scramblers)

Format: **Answer → full reasoning → common wrong answers (why tempting / why wrong)**. Section refs (§) point into [`worksheet.md`](worksheet.md). All data is synthetic. Numeric examples (trellis tables, keystreams, burst counts) were cross-checked with an independent script; the C++ itself was not compiled in the authoring environment.

---

## Q1 — Memory and state

The encoder holds the last two input bits (`u[n−1], u[n−2]`) in a shift register; that pair **is the state** → `2² = 4` states. Each output bit is a XOR of the current input and some of the remembered bits, so one input bit appears in the outputs at times `n, n+1, n+2` (as `u[n]`, `u[n−1]`, `u[n−2]`): it is *smeared* over three output pairs and the decoder can use all of them to vote on it. A block code (Pack 05) encodes each chunk independently with no memory between chunks; a convolutional code is a streaming finite-state machine (§3.1–3.3).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "State = current input bit." | The input drives the output. | The input is the *branch choice*; state is the *remembered* previous two bits. |
| "Rate 1/2 means every second bit is a check bit." | Block-code intuition (data + parity). | Every coded bit is a mixture; there are no separate check bits. |
| "4 states because K = 4." | Misremembering the K=3 definition. | K = memory + 1 = 3; states = 2^(K−1) = 4. |

---

## Q2 — Encode `1101`

| n | u | u[n−1] | u[n−2] | out0 = u⊕u1⊕u2 | out1 = u⊕u2 | pair |
|---|---|---|---|---|---|---|
| 0 | 1 | 0 | 0 | 1 | 1 | 11 |
| 1 | 1 | 1 | 0 | 1⊕1⊕0 = 0 | 1⊕0 = 1 | 01 |
| 2 | 0 | 1 | 1 | 0⊕1⊕1 = 0 | 0⊕1 = 1 | 01 |
| 3 | 1 | 0 | 1 | 1⊕0⊕1 = 0 | 1⊕1 = 0 | 00 |
| 4 (tail) | 0 | 1 | 0 | 0⊕1⊕0 = 1 | 0⊕0 = 0 | 10 |
| 5 (tail) | 0 | 0 | 1 | 0⊕0⊕1 = 1 | 0⊕1 = 1 | 11 |

Result `11 01 01 00 10 11` = `110101001011`.
**Polynomial check:** `u = 1 + D + D³`. `out0 = u·(1+D+D²) = (1+D+D³) + (D+D²+D⁴) + (D²+D³+D⁵)`; pairs cancel mod 2 → `1 + D⁴ + D⁵` → time 0..5: `1,0,0,0,1,1` ✓ (first bit of each pair). `out1 = u·(1+D²) = (1+D+D³) + (D²+D³+D⁵) = 1 + D + D² + D⁵` → `1,1,1,0,0,1` ✓ (second bits).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Forgetting the tail → `11 01 01 00`. | The data is only 4 bits. | The tail flushes the register; without it the stream doesn't end in S0 and the last bits are weakly protected. |
| Using `u[n−1]` where `u[n−2]` is needed in out1 (g1 = 101 skips the middle tap). | 5 = 101 is easy to misread. | Octal 5 = binary 101 → taps at u[n] and u[n−2] only. |
| Using the *new* state's bits in the same step. | Shift-register picture confusion. | Outputs depend on the *old* memory plus the new input; the shift happens after output. |

---

## Q3 — Linearity and distance

**(a)** `encode(0110)`: `u = 0,1,1,0,0,0` → pairs `00 11 01 01 11 00` = `001101011100`. And
`111000010111 ⊕ 110101001011 = 001101011100` ✓ — equal, because the encoder is a linear map over GF(2) (§3.2).
**(b)** `d_free = 5` → `t = ⌊(5−1)/2⌋ = 2` errors are always corrected.
**(c)** Linearity: the difference of two codewords is the codeword of the XOR of their inputs, so the minimum distance equals the minimum weight of a nonzero codeword, i.e. of a path that leaves S0 and later returns. From the §3.3 table: leaving costs `S0 →(u=1) S2` = pair 11 (weight 2); returning must go through `S1 → S0` = pair 11 (weight 2); to reach S1 you must come from S2 (pair 10, weight 1) or from S3 (pair 01, weight 1). Shortest detour: `S0→S2→S1→S0` = 2 + 1 + 2 = **5** — the encoding of `1 0 0` (`11 10 11`). Longer detours only add weight. Direct check of alternatives: input `11` → weight 6; `101` → 6; `111` → 7.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "d_free = 2 (the weight of the first output pair)." | Looking only at the first step. | The path must return to S0, adding weight. |
| "Rate 1/2 ⇒ corrects 50% errors." | Rate ≠ correction power. | Correction is `⌊(d_free − 1)/2⌋ = 2` *per error event window*. |

---

## Q4 — Viterbi on `11 01 00 00 10 11`

Metrics after each step (∞ = unreachable):

| after step (rx) | S0 | S1 | S2 | S3 |
|---|---|---|---|---|
| start | 0 | ∞ | ∞ | ∞ |
| t0 (11) | 2 | ∞ | 0 | ∞ |
| t1 (01) | 3 | 2 | 3 | 0 |
| t2 (00) | 3 | 1 | 2 | 1 |
| t3 (00) | 3 | 2 | 1 | 2 |
| t4 (10) | 3 | 1 | 3 | 2 |
| t5 (11) | **1** | 3 | 3 | 3 |

Survivors (predecessor, `u`):

| t | S0 ← | S1 ← | S2 ← | S3 ← |
|---|---|---|---|---|
| 0 | (S0,0) | – | (S0,1) | – |
| 1 | (S0,0) | (S2,0) | (S0,1) | (S2,1) |
| 2 | (S0,0) | (S3,0) | (S1,1) | (S3,1) |
| 3 | (S0,0) | (S3,0) | (S1,1) | (S3,1) |
| 4 | (S1,0) | (S2,0) | (S1,1) | (S3,1) |
| 5 | (S1,0) | (S3,0) | (S0,1) | (S3,1) |

Sample cell (t1, S3): from S2 (metric 0) with `u=1`, expected pair `01`, received `01` → bm 0 → 0; the other arrival into S3 is from S3 itself, not yet reachable at t0 → candidate absent.
Traceback from S0 at t5: `(S1,0)` → t4 S1: `(S2,0)` → t3 S2: `(S1,1)` → t2 S1: `(S3,0)` → t1 S3: `(S2,1)` → t0 S2: `(S0,1)`. `u` forward: `1,1,0,1,0,0` → drop tail → **`1101`**, metric **1** (the single flipped bit: pair 2 `01 → 00`).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Starting the traceback from the smallest metric in the last column. | "Always pick the smallest." | For a *terminated* stream the path must end in S0 (the tail forces it); here S0 = 1 happens to be the smallest, but in general another state may be smaller and belong to a path that ignores the tail constraint. |
| Greedily picking the best branch at each step. | It's simple. | A branch that looks worse now can win later (e.g. at t1 S3 has metric 0 while S0 has 3, but all four states must stay alive because the received bits at t2..t5 decide). Viterbi prunes only *within* a state, never across states. |
| Forgetting `∞` for unreachable states. | Start metrics "should be 0". | If S1..S3 start at 0 the decoder may assume any starting state (Q8-a). |

---

## Q5 — Limits of the code

**(a)** The terminated block code has minimum distance ≥ `d_free = 5`. Two errors put `rx` within distance 2 of the sent codeword and, by the triangle inequality, at least `5 − 2 = 3` from every other codeword, so the closest (ML) codeword is the right one. (Exhaustively checked: all 12 single and all 66 double errors on `encode(1011)` decode correctly with metric 1 and 2.)
**(b)** Errors at coded positions 0, 1, 2 turn `11 10 00 01 01 11` into `00 00 00 01 01 11`. The codeword for `0011` is `00 00 11 01 01 11`: distance **2** from `rx`, while the true codeword `111000010111` is at distance 3. Viterbi picks the closer one → decoded `0011` (wrong), metric 2 — smaller than the 3 real errors. (Statistics: 108 of the 220 three-error patterns on this word decode wrongly.) So the decoder never "knows" it failed: add an outer CRC (Pack 04).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Metric 2 means 2 errors happened." | The metric is described as "the number of errors". | It is the distance to the *chosen* codeword; chosen codeword ≠ sent one. |
| "3 errors can be corrected since rate 1/2." | Over-generous. | `t = 2`; 3 errors can land closer to another codeword (Pack 05 §3.3 lies at the heart of it). |

---

## Q6 — Complexity

(a) `2¹⁰⁰ ≈ 1.27 × 10³⁰` input sequences. (b) 102 steps (100 + 2 tail) × (4 states × 2 branches) = **816** add-compare operations. (c) K=7: 64 states × 2 = 128 per step → 102 × 128 = **13,056**. (d) DP: stage = time step, state = 2^(K−1) memory states, recurrence `M[t+1][s'] = min over (s,u: next(s,u) = s') of M[t][s] + bm(s,u,r_t)`; answer `M[T][S0]`; back-pointers give the path. Time `O(T · 2^(K−1))`, memory `O(T · 2^(K−1))` for survivors (real decoders truncate traceback to ~5K steps).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Viterbi is O(n) regardless of K." | The step count is n. | Work per step grows with the number of states `2^(K−1)`; K=7 → 64. |
| "Still exponential: each state has two branches." | Branching of the tree. | Paths *merge* into states; we keep one survivor per state, so the count doesn't double each step. |

---

## Q7 — Interleaver mapping

Transmit slot `j = c·rows + r`, coded index `i = r·cols + c` (rows = 4, cols = 7). Slots 10..13:
`10`: c=2, r=2 → i = 2·7 + 2 = **16**; `11`: c=2, r=3 → 23; `12`: c=3, r=0 → 3; `13`: c=3, r=1 → 10. Corrupted coded positions: **3, 10, 16, 23** (spacings 7, 6, 7): four isolated single errors, each corrected → the original data, metric 4.
A 5-long burst (slots 10..14) adds slot 14: c=3, r=2 → i = 17, adjacent to 16 — two errors close together plus others. For our test message this decodes **wrongly** (output `110110011010`, a tie at distance 5). In general, 5-long bursts beat a 4-row interleaver at 16 of 24 start positions (6-long: 22 of 23). **Rule:** interleaver depth (rows) must be at least the burst length, and the neighbouring-column wraparound must still leave enough spacing.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Positions 10, 11, 12, 13 (the burst stays contiguous)." | Forgetting to deinterleave. | The receiver reorders *before* decoding; the burst is spread out. |
| "Interleaving fixes any burst." | It fixed the length-4 one. | Needs rows ≥ burst length; longer bursts hit the same row/neighbours. |
| "Transposed formula: i = c·rows + r." | Mixing the two index spaces. | `c·rows + r` is the *transmit* index; the coded (write-order) index is `r·cols + c`. |

---

## Q8 — Find the bug

1. **(a)** `metric{0,0,0,0}`: all start states are equally good, so the decoder may begin in any state. Must be `metric[0] = 0` and the others large (∞).
2. **(b)** Bit packing is reversed: `expected_pair` returns `(out0 << 1) | out1`, so the **first** received bit is the high bit: `r = (rx[2t] << 1) | rx[2t+1]`.
3. **(c)** `next{0,0,0,0}` — all costs are ≥ 0 so `cost < next[ns]` is never true: survivors are never recorded (garbage/uninitialised traceback). `next` must start at ∞.
4. **(d)** `expected ^ r` is an integer 0–3, not a Hamming distance: `10 → 2`, `01 → 1`, `11 → 3`. Use `popcount(expected ^ r)` (0, 1 or 2).
5. **(e)** Traceback emits bits **last-to-first** (needs `std::reverse`) and the 2 **tail** bits must be dropped (`resize(steps − 2)`).
6. Hygiene: no check that `rx.size()` is even and ≥ 4; when you replace "∞" by `UINT_MAX` then `metric[ps] + bm` **overflows** — skip unreachable states or use a sentinel such as `0x3FFFFFFF`. Also `ns = (u << 1) | (ps >> 1)` is correct for the state layout `bit1 = u[n−1], bit0 = u[n−2]`.

Reference fix: see `code/solutions/viterbi.h`.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Only the initialisation is wrong." | It is the most visible conceptual bug. | Five independent defects, any of which breaks the output. |
| "`cost < next[ns]` should be `<=`." | Equal costs feel like they should update. | `<` vs `<=` is only the *tie-break rule*; both give valid (ML) answers. |

---

## Q9 — LFSR by hand + scrambling

With `ks[−1] = 1`, `ks[−2..−7] = 0`: `ks[n] = ks[n−4] ⊕ ks[n−7]`:
`ks[0] = ks[−4]⊕ks[−7] = 0`, `ks[1] = ks[−3]⊕ks[−6] = 0`, `ks[2] = ks[−2]⊕ks[−5] = 0`, `ks[3] = ks[−1]⊕ks[−4] = 1`, `ks[4] = ks[0]⊕ks[−3] = 0`, `ks[5] = ks[1]⊕ks[−2] = 0`, `ks[6] = ks[2]⊕ks[−1] = 1`, `ks[7] = ks[3]⊕ks[0] = 1`, `ks[8] = ks[4]⊕ks[1] = 0`, `ks[9] = ks[5]⊕ks[2] = 0`, `ks[10] = ks[6]⊕ks[3] = 0`, `ks[11] = ks[7]⊕ks[4] = 1` → **`000100110001`**.
Scramble `1111 0000 1111`: `111100001111 ⊕ 000100110001 = 111000111110`. Descramble: `111000111110 ⊕ 000100110001 = 111100001111` ✓ (same operation).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| Treating the seed `0x01` as "ks[−7] = 1" (bit 0 = oldest). | Seed bit 0 looks like the first element. | In our convention bit 0 = produced **1** step ago (bit k−1 = k steps ago), so ks[−1] = 1. |
| Starting at `ks[0]` with `ks[−1] = 0`. | Treating unspecified history as 0. | All-zero history keeps the output zero forever (the seed-0 trap, §3.7). |

---

## Q10 — Why descrambling works

**(a)** `(d ⊕ k) ⊕ k = d ⊕ (k ⊕ k) = d ⊕ 0 = d`: XOR is associative and every bit XOR itself is 0. So the receiver applies the same function with the same keystream.
**(b)** Wrong seed or a slipped bit → the receiver's keystream `k'` ≠ `k`; output = `d ⊕ k ⊕ k'`. For two different phases of a maximal-length sequence, `k ⊕ k'` is *another phase of the same sequence* (the shift-and-add property), i.e. pseudo-random: roughly **half** of the bits are wrong for the rest of the frame, until re-seeded.
**(c)** The first 7 data bits are 0, so the first 7 transmitted bits equal the first 7 keystream bits. Because the state *is* the last 7 outputs (§3.7), the receiver loads those 7 bits as its state and continues with `ks[n] = ks[n−4] ⊕ ks[n−7]` (**VERIFY**, IEEE 802.11 §17.3.5.5 — the receiver estimating the scrambler state from SERVICE bits is a common implementation approach).
**(d)** One flipped channel bit flips only the same bit of the data: one error stays one error (tested).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Descramble = inverse LFSR run backwards." | "Inverse" suggests running backwards. | XOR with the same keystream is its own inverse; no backwards run. |
| "A slipped bit costs one error." | Additive: errors don't multiply. | Alignment loss corrupts every following bit. |
| "Scrambling hides the data from eavesdroppers." | Output looks random. | The polynomial is public and the LFSR is linear: 14 output bits reveal the state. |

---

## Q11 — Self-synchronising scrambler

**(a)** Channel bit `k = 20` appears as `rx[n]` (n=20), as `rx[n−4]` (n = 24) and as `rx[n−7]` (n = 27): output errors at **20, 24, 27** (tested).
**(b)** `dec[n]` depends only on `rx[n], rx[n−4], rx[n−7]` — the last 7 received bits; a wrongly initialised descrambler is correct from output index **7** on (only the first 7 outputs can be wrong; in the test, mismatches at 1, 2, 4, 6).
**(c)** Additive: no error multiplication (1 → 1), but no self-sync: a slip or wrong seed ruins the rest of the frame. Self-sync: automatic resync after 7 bits, errors multiplied by the number of taps (3).

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Errors at 20 only (like additive)." | Mixing the two types. | The descrambler's feed-forward taps replicate each input bit three times. |
| "Errors at 20, 21, 22 (burst)." | Thinking of a burst of 3. | Offsets are the polynomial exponents 0, 4, 7 (bits `n`, `n−4`, `n−7`). |
| "Locks after 4 bits." | Thinking only of the first tap. | The longest tap is 7. |

---

## Q12 — LC 931 as Viterbi

**(a)** DP rows:
`row0: 2 1 3` → `row1: 7 6 5` (6+min(2,1), 5+min(2,1,3), 4+min(1,3)) → `row2: 13 13 14` (7+min(7,6), 8+min(7,6,5), 9+min(6,5)). Min = **13**, tie between columns 0 and 1. Back-pointers: `r2c0 ← r1c1 ← r0c1` = path `1 → 5 → 7 = 13`; `r2c1 ← r1c2 ← r0c1` = path `1 → 4 → 8 = 13`.
Mapping: stage = row ↔ time step; state = column ↔ encoder state; `dp` ↔ path metric; the 3 (or 2) allowed predecessor cells ↔ the 2 incoming trellis branches; back-pointer ↔ survivor; reading back-pointers from the best last cell ↔ traceback (from S0 for a terminated code). The tie is exactly Viterbi's tie-break issue.
**(b)** LC 746 *Min Cost Climbing Stairs*, LC 120 *Triangle*, LC 64 *Minimum Path Sum*, LC 72 *Edit Distance* (Viterbi on an alignment lattice) (**VERIFY** numbers).
**(c)** Edge weights: `dp[r][c] = min over p (dp[r−1][p] + w(p→c))`, which is precisely Viterbi's `M + bm`; LC 931 puts the weight on the destination cell instead.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Greedy: start at the smallest top cell and always step to the smallest neighbour below." | It happens to give 13 here. | It fails in general: on `{{2,1,3},{10,5,4},{1,9,9}}` greedy goes `1 → 4 → 9 = 14`, but the optimum is `1 → 5 → 1 = 7`. Use `dp`. |
| "dp[r][c] = m[r][c] + dp[r−1][c]." | Only the straight-up move. | The recurrence must range over all allowed predecessors. |

---

## Q13 — Implement

Reference: `code/solutions/viterbi.h`. Suggested order, each guarded by tests: `conv_encode` (pair table; remember the tail) → `viterbi_decode` (`kInf` for unreachable, metric add, strict `<`, survivor arrays, traceback from S0, reverse, `resize(steps − 2)`) → `interleave/deinterleave` (indices `c·rows + r` ↔ `r·cols + c`; empty on size mismatch) → `lfsr_keystream` (`fb = ((s>>6)^(s>>3))&1`) → `scramble/descramble` → `ss_scramble/ss_descramble` (history from *out* vs from *rx*) → `min_falling_path_sum`.
Pitfalls: bit order of the pair; `kInf + bm` overflow; descrambler history must be the **received** bit; seed 0; `[[maybe_unused]]` removal when implementing; `-Wsign-compare` with `size_t`.
Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`. Expect `solution.*` green; `student.*` turn green as you implement.

---

## Q14 — Connections

**(a) CRC ↔ LFSR.** Fibonacci (this pack): `fb = s6 ⊕ s3`, shift, feed `fb` into bit 0. Galois (the CRC's `if (msb) crc ^= poly`, here in right-shift form): `out = r & 1; r >>= 1; if (out) r ^= 0x48;` — the feedback is XORed into *two internal taps* (bits 6 and 3) instead of one end. Same polynomial → same recurrence; the Galois output with seed `0x01` begins `1000100110001011`, which is the Fibonacci(`0x7F`) stream shifted by 30 positions (the two machines have different internal states but trace the same 127-cycle). Verified with a script.
**(b)** Self-sync scrambler = *divide the stream by* `1 + D⁴ + D⁷`, output = **quotient** (infinite impulse response); CRC = *divide the message by g(x)*, keep the **remainder**. The descrambler is the matching multiplication (FIR). Same shift-and-XOR hardware.
**(c) Typical TX chain:** `payload → CRC (Pack 04, detect) → [header: strong block code, Pack 05; payload: convolutional + tail] → interleave → (scramble, standard-dependent) → modulate`; RX in reverse. In 802.11 the scrambler runs first on the data and the convolutional code afterwards (**VERIFY**), other systems scramble after FEC; mirror the order at both ends.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "CRC and LFSR are different circuits." | Different names. | A CRC register is a Galois LFSR with the input XORed in. |
| "Galois produces a different sequence." | Different taps location. | Same recurrence/cycle; a different state mapping/phase. |

---

## Q15 — Interview: Viterbi

"Decoding a convolutional code means finding the input path through a trellis whose output is closest to the received bits. The number of paths is 2ⁿ, but the encoder has only 2^(K−1) states and the future depends only on the current state, so I keep one best path per state per step — the survivor. At each step, for each state I compare the two incoming candidates (predecessor metric + branch Hamming distance), keep the smaller, and store a back-pointer. At the end I trace back from the known final state (the tail forces state 0). Correctness: optimal substructure — if a path reaches state s via a non-best prefix, replacing the prefix with the best one can only lower the total. Complexity: O(T · 2^(K−1)). Hard decision uses Hamming distance; soft decision uses analog distance (about 2 dB better, VERIFY); same algorithm."
Follow-ups: ties, truncated traceback, puncturing, why K matters (more memory = more distance = more states), relation to HMM decoding and Dijkstra on a DAG.

---

## Q16 — Interview: pipeline design

**TX:** `header (16 bit) + payload → CRC over all (detects miscorrections) →` header: strong **block code** (e.g. Golay/BCH from Pack 05, or a lower-rate code) so that if the header is lost we don't know the length — **unequal error protection**; payload: **convolutional (rate 1/2) with tail** → **block interleaver** with depth ≥ expected burst length → **additive scrambler** (frame-synchronous: reseed at the sync word) for clock recovery and spectrum (or scramble before FEC as in 802.11 — consistency matters more than the exact position) → modulator.
**RX:** sync word (resets scrambler and interleaver alignment) → descramble → deinterleave → Viterbi (payload) / block decode (header) → CRC. **Failure modes:** short burst without interleaver → decoder returns plausible wrong bits (CRC catches); slipped bit with additive scrambler → rest of frame garbage until the next sync word (acceptable if frames are short); self-sync scrambler would auto-resync but multiplies each error by 3, hurting the Viterbi decoder; wrong interleaver alignment → whole frame unreadable; no tail → last bits poorly protected.
**Resync mid-stream:** periodic sync words/frame boundaries; a self-sync scrambler resyncs itself after 7 bits.
Mention synthetic test traffic only; never live public-safety captures.

| Wrong answer | Why tempting | Why wrong |
|---|---|---|
| "Use a scrambler for security." | Looks random. | Whitening only. |
| "Skip the CRC: FEC already corrects errors." | FEC handles errors. | FEC can miscorrect silently (Q5). |
| "One strong code for everything." | Simplicity. | The header and payload have different cost/benefit; unequal error protection is the norm. |

---

## Rapid fire
1. 100 data bits + 2 tail → 204 coded bits → rate ≈ 100/204 ≈ 0.49 (→ 1/2 for long messages).
2. K=7 → 2⁶ = **64** states.
3. Period of a maximal 7-bit LFSR: 2⁷ − 1 = **127**.
4. One channel error through a 3-tap self-sync descrambler → **3** errors.
5. TX: FEC → interleave → modulate; the scrambler position is standard-dependent (802.11: scramble *before* FEC), but both ends must mirror.
