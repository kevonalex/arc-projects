# Pack 06 — Convolutional Codes, Viterbi, Interleaving, and LFSR Scramblers (Etherstack interview worksheet)

> **Audience:** from zero; C++17/20. **Rule:** concrete bits first, then derive, then name it. Every trellis number below is computed bit by bit.
> **Data rule:** all bit streams, bursts and "received" words are **synthetic**. Never run this on live public-safety RX or captured traffic.
> **Companions:** [`answers.md`](answers.md) · [`cheatsheet.md`](cheatsheet.md) · `code/` (stubs `code/include/viterbi.h`, tests, reference in `code/solutions/`).
> **Cross-links:** **Pack 04** (CRC register = LFSR; Galois vs Fibonacci; scrambler = polynomial division) · **Pack 05** (block codes; why bursts break them) · **Pack 01** (XOR, masks, shifts) · **DP / LeetCode** (Viterbi *is* dynamic programming — §3.5).

Sections (the cheatsheet cites these): **§1** Why · **§2** Prerequisites · **§3** Concept build-up (3.1–3.9) · **§4** Worked example · **§5** Questions Q1–Q16 · **§6** Self-assessment.

---

## §1 Why this matters

Block codes (Pack 05) protect *chunks*. Real links also need to protect *continuous streams*, survive *bursts*, and keep the signal *statistically well-behaved*:

| Tool | What it does | Where you meet it |
|---|---|---|
| **Convolutional code + Viterbi** | Every output bit depends on the last few input bits; the decoder finds the single most likely input sequence | Wi-Fi (IEEE 802.11 uses K=7, rate 1/2, generators 133/171 octal), satellite (CCSDS/NASA), GSM (K=5); P25 and DMR data use trellis-coded 1/2 and 3/4 rates (**VERIFY** per standard) |
| **Interleaver** | Reorders bits so a burst of channel errors turns into scattered single errors | Virtually every radio standard: P25, DMR, GSM, LTE (**VERIFY** each) |
| **LFSR scrambler** | XORs the stream with a pseudo-random sequence to remove long runs of 0s/1s (timing recovery) and flatten the spectrum | 802.11 (x⁷+x⁴+1), Ethernet 64b/66b (self-sync), SONET, many LMR air interfaces (**VERIFY**) |

**You will be able to:** hand-encode a convolutional code; run Viterbi on a 4-state trellis with a table; explain why survivors suffice (DP optimal substructure); compare to LeetCode DP; interleave/deinterleave a block and say what a burst becomes; generate an LFSR keystream by hand; explain why descrambling works and what a channel error does to each type of scrambler.

**Failure if ignored:** a decoder that picks the best *final* state vs the *all-zero* state misreads the last bits; a scrambler with seed `0` outputs all zeros forever (no whitening at all); a bursty channel without an interleaver destroys a convolutional code that corrects 2 random errors per word (§3.6).

---

## §2 Prerequisites

- Pack 01: XOR, shifts, masks. Pack 04 §3.2: GF(2) arithmetic (add = subtract = XOR), polynomial multiplication/division by hand.
- Basic dynamic programming: `dp[i] = best over previous states`. If shaky, do LeetCode 746 first (§3.5).
- Notation: `u[n]` = input bit at time n (`u[−1], u[−2] = 0` at the start). `⊕` = XOR. `D` = a one-step delay.
- Bits are stored one per element (`std::vector<uint8_t>` of 0/1) so every step is visible; efficiency is not the point here.

---

## §3 Concept build-up

### 3.1 Concrete first: codes with memory

A block code (Pack 05) encodes each chunk **independently**. A **convolutional** code keeps a small **memory** (a shift register of recent input bits) and emits *several* output bits per input bit, each a XOR of the current and some previous inputs. So **one input bit influences several consecutive output symbols**, and the receiver can use *all* of them to vote on it.

Picture: input stream `1 0 1 1`. At each tick one new input bit enters a 2-bit memory; two output bits leave. Redundancy is smeared across time instead of appended at the end of a block.

Rate = inputs/outputs = 1/2 here (two coded bits per data bit). The price of the memory is that decoding must consider *sequences*, not isolated words — that is what Viterbi does (§3.4).

> **Misconceptions box 3.1**
> 1. *"Convolutional = block code with a long block."* The encoder is a *finite state machine*, with a few states, streaming forever.
> 2. *"Rate 1/2 means half the bits are checks, each protecting a particular data bit."* Every coded bit mixes several data bits; there is no one-to-one protector.

### 3.2 The (7,5) K=3 encoder, derived bit by bit

**Memory** = the last two inputs `u[n−1], u[n−2]`; **constraint length K = 3** (current bit + 2 memory bits). Two **generators** say which taps are XORed:

| generator | taps (u[n], u[n−1], u[n−2]) | binary | octal | output |
|---|---|---|---|---|
| g0 | 1 1 1 | 111 | **7** | `out0 = u[n] ⊕ u[n−1] ⊕ u[n−2]` |
| g1 | 1 0 1 | 101 | **5** | `out1 = u[n]  ⊕ u[n−2]` |

**Encode `1 0 1 1` (+ two zero tail bits).** Start with memory `00`.

| n | u[n] | u[n−1] | u[n−2] | out0 | out1 | pair |
|---|---|---|---|---|---|---|
| 0 | 1 | 0 | 0 | 1⊕0⊕0 = 1 | 1⊕0 = 1 | **11** |
| 1 | 0 | 1 | 0 | 0⊕1⊕0 = 1 | 0⊕0 = 0 | **10** |
| 2 | 1 | 0 | 1 | 1⊕0⊕1 = 0 | 1⊕1 = 0 | **00** |
| 3 | 1 | 1 | 0 | 1⊕1⊕0 = 0 | 1⊕0 = 1 | **01** |
| 4 (tail) | 0 | 1 | 1 | 0⊕1⊕1 = 0 | 0⊕1 = 1 | **01** |
| 5 (tail) | 0 | 0 | 1 | 0⊕0⊕1 = 1 | 0⊕1 = 1 | **11** |

Coded stream: `11 10 00 01 01 11` = `111000010111` (12 bits for 4 data bits: the 2 tail bits cost rate; for long messages it is negligible).

**Why the tail?** The two zeros flush the register back to the all-zero state, so the decoder *knows where the path ends* (state 0) and can run the traceback from there. Without it, the last few bits are decoded with less protection (no later symbols vote on them) and the decoder must guess the final state.

**Why it is called "convolution".** Treat bits as polynomials in `D` (delay): `u = 1 + D² + D³` (inputs at times 0, 2, 3). Then
`out0 = u·(1 + D + D²) = 1 + D + D⁵` and `out1 = u·(1 + D²) = 1 + D³ + D⁴ + D⁵` (working mod 2: terms cancel in pairs). Reading time 0..5: `out0 = 1,1,0,0,0,1`, `out1 = 1,0,0,1,1,1` — interleaved pair by pair that is exactly `11 10 00 01 01 11` ✓. The encoder multiplies the input by each generator polynomial (a convolution over GF(2)) — compare Pack 04's polynomial division: here *multiplication*, there *division*.

**Linearity.** XOR of two inputs encodes to the XOR of their encodings. Consequently the **free distance** `d_free` = the minimum weight of a nonzero codeword = the encoding of `1 0 0 …`: `11 10 11` = weight **5**. So any 2 channel errors are always correctable (`⌊(5−1)/2⌋ = 2`) — same distance logic as Pack 05 §3.3.

> **Misconceptions box 3.2**
> 1. *"Generators 7 and 5 are arbitrary numbers."* They are tap masks written in octal; 111 and 101.
> 2. *"The tail bits are data."* They are always 0, known to both ends, and discarded.
> 3. *"State = the input bit."* State = the *previous two* inputs (4 states); the current input chooses the branch.

### 3.3 State machine and trellis

**State** `(u[n−1] u[n−2])`, written as two bits: `00, 01, 10, 11` (call them S0..S3; our code stores `state = u[n−1]·2 + u[n−2]`). From a state, input `u` moves to the next state `(u, u[n−1])` — the memory shifts — and emits a pair:

| state | u | next | output | state | u | next | output |
|---|---|---|---|---|---|---|---|
| S0 (00) | 0 | S0 (00) | 00 | S2 (10) | 0 | S1 (01) | 10 |
| S0 (00) | 1 | S2 (10) | 11 | S2 (10) | 1 | S3 (11) | 01 |
| S1 (01) | 0 | S0 (00) | 11 | S3 (11) | 0 | S1 (01) | 01 |
| S1 (01) | 1 | S2 (10) | 00 | S3 (11) | 1 | S3 (11) | 10 |

(Check against §3.2: n=1, state `10` (u[0]=1, u[−1]=0), `u=0` → out `10`, next `01` ✓.)

**Trellis** = this diagram unrolled in time: columns for each time step, four dots per column, every dot with two arrows to the next column. Any input sequence is *one path* through the trellis; the encoder's output is the labels along it. **Decoding means finding the most likely path.**

There are `2^n` input sequences (paths) for `n` data bits, but only **4 states per column**: the trellis compresses `2^n` paths into `4n` dots. That is the opening for dynamic programming.

> **Misconceptions box 3.3**
> 1. *"The trellis has 2^n nodes."* It has `4·(n+2)` nodes; paths are exponential, nodes are not.
> 2. *"Each state has 4 outgoing arrows."* One arrow per input bit value: 2 in, 2 out.
> 3. *"The labels on arrows are the data."* The data is the *choice* of arrow (u); the label is the coded pair.

### 3.4 Viterbi decoding, step by step

You receive a possibly corrupted coded stream `rx` and want the input whose encoding is **closest** (smallest Hamming distance) to `rx`: **maximum-likelihood** decoding when bit flips are independent and rare.

**Branch metric:** at step `t`, for the branch with expected pair `e` and received pair `r`: `bm = popcount(e ⊕ r)` (0, 1 or 2).
**Path metric:** sum of branch metrics along the path.
**Algorithm (the "add–compare–select" loop):** keep for each of the 4 states the *best (smallest) metric of any path arriving there* and which predecessor/input produced it (the **survivor**). For each state at time `t+1` look at its two incoming arrows, **add** `bm` to the predecessor's metric, **compare** the two candidates, **select** the smaller. At the end, **trace back** from state S0 (we terminated) following survivors, collect the `u` bits, **drop the 2 tail bits**.

**Why discarding the losing path is safe (derive).** Suppose the globally best path passes through state `s` at time `t`. The *future* cost of that path depends only on `s` (the memory is the state) and the future received bits, **not on how we got to `s`**. If some other prefix reached `s` with a smaller metric, swapping it in gives a strictly smaller total — contradicting "best". So the best path's prefix must be the best path to `s`: all other arrivals at `s` can be thrown away immediately. This is **optimal substructure**, the defining property of DP (§3.5).

**Cost:** per step `4 states × 2 branches = 8` add-compare operations — constant. Total `O(n · 2^(K−1))` instead of `O(2^n)`. (Real K=7 decoders: 64 states.)

**Tie-breaking.** Two paths into a state can have equal metrics, and the decoder may legitimately pick either; for repeatable tests, our rule is: previous states ascending, `u = 0` before `u = 1`, and a new candidate wins only if **strictly** smaller.

**Hard vs soft decisions.** Our decoder sees only bits (0/1) — *hard decision*. A real receiver can pass the *analog confidence* of each bit (soft decision) and use that as the branch metric; this typically buys roughly 2 dB in SNR (**VERIFY**). The algorithm is unchanged; only `bm` changes.

> **Misconceptions box 3.4**
> 1. *"Viterbi tries all 2^n paths."* It keeps only 4 survivors per step.
> 2. *"Viterbi greedily picks the best branch at each step."* No — it keeps both candidates into each state and only prunes *within a state*; the apparently worst branch now may win later (§4).
> 3. *"The decoder knows where the errors are."* It never identifies errors; it finds the closest valid path, and the number of errors falls out as the final metric.
> 4. *"Metric 0 means no error, so metric k means k errors."* Metric = distance to the *chosen* codeword: if more errors occurred than the code can fix, the decoder picks a wrong, closer codeword and reports a *smaller* metric than the real error count (Q5).

### 3.5 Viterbi ↔ dynamic programming (LeetCode cousins)

| | Viterbi | Minimum Falling Path Sum (LC 931) |
|---|---|---|
| stage | time step `t` | row `r` |
| state | one of 4 memory states | column `c` |
| value | best path metric so far | cheapest cost so far |
| transition | from 2 predecessor states, `+bm` | from up to 3 cells above, `+ cell` |
| answer | metric at S0 after the last step (+ traceback) | min of last row |

Recurrence: `dp[r][c] = m[r][c] + min(dp[r−1][c−1], dp[r−1][c], dp[r−1][c+1])` (in bounds). Example `{{2,1,3},{6,5,4},{7,8,9}}` → row 0: `2,1,3`; row 1: `6+min(2,1)=7, 5+min(2,1,3)=6, 4+min(1,3)=5`; row 2: `7+min(7,6)=13, 8+min(7,6,5)=13, 9+min(6,5)=14` → min = **13** ✓ (path 1→4→8: `1+4+8`). To recover the path, keep a back-pointer per cell — exactly Viterbi's survivor + traceback.
Other cousins (**VERIFY numbers**): LC 746 *Min Cost Climbing Stairs* (1 predecessor pair), LC 120 *Triangle*, LC 64 *Minimum Path Sum* (2D grid), LC 72 *Edit Distance* (Viterbi on an alignment lattice; also the Needleman–Wunsch/Levenshtein DP). **Interview tip:** if asked "how is Viterbi different from Dijkstra?" — same shortest-path idea, but the trellis is a layered DAG so a single forward sweep suffices.

### 3.6 Interleaving: turning a burst into scattered errors

**Problem.** Radio fading and interference produce **bursts**: several *consecutive* bit errors. A convolutional code (and the Hamming/Golay codes of Pack 05) correct a *few* errors per window; a burst puts them all in one window. Example from §4/Q7 with our test message: a burst of 4 consecutive errors in the coded stream defeats the decoder at **every** start position (tested).

**Block interleaver (rows × cols).** Write the coded bits into a grid **row by row**, read it out **column by column**. Receiver does the inverse. Concrete 4 × 7 grid (28 coded bits, positions numbered by write order 0..27):

```
         col0 col1 col2 col3 col4 col5 col6
row0:      0    1    2    3    4    5    6
row1:      7    8    9   10   11   12   13
row2:     14   15   16   17   18   19   20
row3:     21   22   23   24   25   26   27
transmit order (column by column):  0 7 14 21 | 1 8 15 22 | 2 9 16 23 | ...
```
A burst of 4 consecutive channel bits (e.g. transmit slots 8..11 = column 2) hits coded bits **2, 9, 16, 23**: seven apart. After deinterleaving, the decoder sees **four isolated single errors**, spaced more than the encoder's memory span — each is corrected independently: the final metric is 4, the data are intact (tested at all 25 burst start offsets).
Formula: transmit index `j = c·rows + r` ⇄ coded index `i = r·cols + c`.

**Cost:** delay (you must buffer a full block before sending/before decoding) and memory. Interleaver depth is a latency vs burst-length trade-off.

**Failure if ignored:** a perfectly good rate-1/2 code is defeated by a short burst. **Failure if overdone:** 100 ms of interleaving ruins push-to-talk latency.

> **Misconceptions box 3.6**
> 1. *"Interleaving adds protection."* It adds **no** redundancy; it only changes *which* errors the decoder sees.
> 2. *"Interleave then FEC."* Order at the transmitter: **FEC → interleave → channel**; receiver: deinterleave → FEC decode.
> 3. *"Interleaving helps against random errors."* No effect on uniformly random errors; only helps for bursts.

### 3.7 The LFSR, derived sample by sample

A **linear-feedback shift register** is a shift register whose new input bit is the XOR of selected earlier bits. Polynomial `x⁷ + x⁴ + 1` (the 802.11 scrambler polynomial — **VERIFY**, IEEE 802.11 §17.3.5.5) means: new bit = (the bit from 7 steps ago) ⊕ (the bit from 4 steps ago):

`ks[n] = ks[n−4] ⊕ ks[n−7]`

**Concrete: seed with a single 1.** Pretend the bit produced one step ago was 1 and the six before that 0 (our `seed7 = 0x01`):

| n | ks[n−4] | ks[n−7] | ks[n] | note |
|---|---|---|---|---|
| 0 | 0 | 0 | 0 | |
| 1 | 0 | 0 | 0 | |
| 2 | 0 | 0 | 0 | |
| 3 | ks[−1] = 1 | 0 | **1** | the seed's 1 first reaches a tap |
| 4 | ks[0] = 0 | 0 | 0 | |
| 5 | ks[1] = 0 | 0 | 0 | |
| 6 | ks[2] = 0 | ks[−1] = 1 | **1** | |
| 7 | ks[3] = 1 | ks[0] = 0 | **1** | |
| 8 | ks[4] = 0 | ks[1] = 0 | 0 | |
| 9 | ks[5] = 0 | ks[2] = 0 | 0 | |
| 10 | ks[6] = 1 | ks[3] = 1 | 1⊕1 = **0** | |
| 11 | ks[7] = 1 | ks[4] = 0 | **1** | |

Sequence: `000100110001…` (matches `lfsr_keystream(12, 0x01)`).

**Why the register's state is just the last 7 outputs.** `ks[n]` is computed from `ks[n−4]` and `ks[n−7]`, so only the last 7 outputs matter: state = those 7 bits. `s` bit `(k−1)` = the output from `k` steps ago; each step: `fb = s6 ⊕ s3` (7 and 4 steps ago), shift left, put `fb` in bit 0. That's the 4-line C++ in the solution.

**Facts that must be true.**
- An LFSR is deterministic and the state space is finite (`2⁷ = 128` states), so the sequence is **periodic**.
- The all-zero state maps to itself (`0 ⊕ 0 = 0`) — **seed 0 is a trap** (all-zero output forever, i.e. *no scrambling*).
- If the polynomial is **primitive** (x⁷+x⁴+1 is), the other 127 states form a single cycle: **period 2⁷ − 1 = 127**, containing 64 ones and 63 zeros, the longest run of ones is 7 and of zeros is 6 (all tested).
- With seed all-ones (`0x7F`) the output begins `0000 1110 1111 0010…` (first byte `0x0E`) — the well-known 802.11 sequence (**VERIFY** against the standard's Table).

**Cross-link: LFSR ↔ CRC (Pack 04).** The CRC register *is* an LFSR whose polynomial is the CRC generator. Two layouts compute the **same sequence** from the same polynomial: **Fibonacci** (XOR taps together, feed the result into one end — this pack) and **Galois** (shift, and XOR the output bit into several internal taps — the CRC's `if (msb) crc ^= poly`). Different internal states, same output stream. Difference in purpose: scrambler/PRBS polynomials are chosen **primitive** for maximal period; CRC polynomials for burst-detection properties.

> **Misconceptions box 3.7**
> 1. *"An LFSR is random."* It is fully deterministic and periodic; it is *statistically* flat.
> 2. *"Any seed works."* Seed 0 is stuck; the *start* of the sequence changes with the seed but the cycle is the same.
> 3. *"The LFSR is secure."* Linear: knowing 2·7 = 14 consecutive output bits reveals the state (Berlekamp–Massey). Scrambling is **not encryption**.
> 4. *"Fibonacci and Galois give different sequences."* Same polynomial ⇒ same sequence set; only the internal state encoding differs.

### 3.8 Additive (synchronous) scrambler — why descrambling works

`tx = data ⊕ ks`. Receiver computes `tx ⊕ ks = (data ⊕ ks) ⊕ ks = data ⊕ (ks ⊕ ks) = data ⊕ 0 = data`, because **XOR is its own inverse**: any bit XORed with itself is 0, and `x ⊕ 0 = x`. Descrambling is literally **the same function** — `descramble = scramble`.

**What it needs:** the receiver's keystream must be **aligned with the transmitter's**: same polynomial, same seed, same bit position. If the seed is wrong, or one bit is slipped, the output is garbage (every following bit). In 802.11 the transmitter's seed is sent *implicitly*: the first 7 data bits (SERVICE field) are zero, so the first 7 received bits **are** the keystream = the LFSR state, and the receiver loads them (**VERIFY**, §17.3.5.5).

**What it does to a channel error:** a flipped bit in `tx` flips exactly the same bit in `data`: **one error stays one error** (no multiplication) but sync loss is catastrophic (needs a frame start / re-seed).

**Why bother?** Long runs of identical bits give no transitions for the receiver's clock recovery and put a spectral line (DC) in the signal. XOR with a 127-bit pseudo-random sequence breaks runs (max run 7).

> **Misconceptions box 3.8**
> 1. *"Scrambling protects against eavesdropping."* The sequence is public and linear; it is whitening, not encryption.
> 2. *"Scrambling adds redundancy/bits."* Output length = input length.
> 3. *"Scramble after the CRC."* Apply it consistently on both ends; in 802.11 the scrambler runs on the data *before* convolutional coding (**VERIFY**), and the receiver descrambles after Viterbi.

### 3.9 Self-synchronising scrambler

Instead of running its own LFSR, the scrambler feeds its **own output back**:
`out[n] = in[n] ⊕ out[n−4] ⊕ out[n−7]`  (a division of the input stream by `1 + D⁴ + D⁷`).
The **descrambler** uses the *received* bits as history:
`dec[n] = rx[n] ⊕ rx[n−4] ⊕ rx[n−7]`  (a *multiplication* by `1 + D⁴ + D⁷`, an FIR filter — no feedback).
If `rx = out`: `dec[n] = out[n] ⊕ out[n−4] ⊕ out[n−7] = in[n]` by the scrambler's own equation ✓.

**Self-synchronisation (derive).** `dec[n]` depends only on `rx[n], rx[n−4], rx[n−7]` — the last 7 received bits, nothing else. So after 7 *correct* received bits, the descrambler's state matches the true one **whatever it started as**: no seed to agree on, and after a slip or a burst it recovers on its own (tested: mismatches only in the first 7 output bits).

**The price: error multiplication.** A single flipped channel bit `rx[k]` appears in `dec[k]` (as `rx[n]`), `dec[k+4]` (as `rx[n−4]`) and `dec[k+7]` (as `rx[n−7]`): **1 error → 3 errors**, at offsets 0, +4, +7 (tested: with `k = 20` the output errors are at 20, 24, 27). It equals the number of nonzero terms in the polynomial. This is why self-sync scramblers sit in fibre/Ethernet links with very low BER, and additive scramblers sit where bursts are common and frames re-sync.

**Cross-link (Pack 04).** The self-synchronising scrambler divides the stream by a polynomial — the *quotient* is the output; a CRC keeps the *remainder*. Same shift-and-XOR hardware.

> **Misconceptions box 3.9**
> 1. *"Self-sync means no error propagation."* The opposite: it trades sync-robustness for multiplied errors (1 → #taps).
> 2. *"Descrambler needs the transmitter's seed."* No, after 7 bits it's locked.
> 3. *"Additive and self-sync are interchangeable."* Different failure modes: additive = lose sync on a slip; self-sync = triple the errors.

---

## §4 Fully worked example — decode with one error, then a burst (synthetic)

**Encode** `1011` → `11 10 00 01 01 11` (§3.2). **Channel** flips the 2nd bit of pair 2 (`00 → 10`): received `rx = 11 10 10 01 01 11`.

**Viterbi.** `∞` = unreachable. Entries are path metrics after consuming that step's pair.

| after step (rx pair) | S0 | S1 | S2 | S3 |
|---|---|---|---|---|
| start | 0 | ∞ | ∞ | ∞ |
| t0 (11) | 2 | ∞ | 0 | ∞ |
| t1 (10) | 3 | 0 | 3 | 2 |
| t2 (10) | 1 | 3 | 1 | 2 |
| t3 (01) | 2 | 2 | 2 | 1 |
| t4 (01) | 3 | 1 | 3 | 2 |
| t5 (11) | **1** | 3 | 3 | 3 |

Sample cell, `t2` into S0: candidates `from S0, u=0`: `3 + bm(00⊕10 = 1) = 4`; `from S1, u=0`: `0 + bm(11 ⊕ 10 = 1) = 1` → select **1** (from S1).
Survivors (predecessor, `u`) for the traceback:

| t | S0 ← | S1 ← | S2 ← | S3 ← |
|---|---|---|---|---|
| 0 | (S0,0) | – | (S0,1) | – |
| 1 | (S0,0) | (S2,0) | (S0,1) | (S2,1) |
| 2 | (S1,0) | (S2,0) | (S1,1) | (S3,1) |
| 3 | (S0,0) | (S3,0) | (S0,1) | (S2,1) |
| 4 | (S0,0) | (S3,0) | (S0,1) | (S2,1) |
| 5 | (S1,0) | (S3,0) | (S0,1) | (S3,1) |

Traceback from S0 at t5: `(S1,u=0)` → t4 S1: `(S3,0)` → t3 S3: `(S2,1)` → t2 S2: `(S1,1)` → t1 S1: `(S2,0)` → t0 S2: `(S0,1)`. Reading `u` forward from t0: `1, 0, 1, 1, 0, 0`. Drop the 2 tail bits: **`1011`** ✓, final metric **1** = the one flipped bit.
(Notice that at t1 S0 holds 3 while S1 holds 0: the "locally bad" paths in S0 are not what wins — but nothing was greedily discarded across states.)

**Burst without interleaving.** Data `101100111010` (12 bits) → 28 coded bits `1110000101111101100100101100`. A burst of 4 flips starting at coded position 10 decodes to `101110111010` (wrong, metric 3 — a closer wrong codeword). The same happens at all 25 possible start positions (tested).

**With interleaving 4 × 7.** The transmitter sends `interleave(coded, 4, 7)` = `1100101111100101010101100100`. The same 4-bit burst at transmit slots 8..11 hits coded bits 2, 9, 16, 23. After deinterleaving each error is isolated → Viterbi decodes `101100111010` ✓ with metric **4**. All 25 burst offsets are corrected (tested).

---

## §5 Questions

Each has a **type**, a **context line**, and an answer in `answers.md`.

**Q1 — Explain.** *Context: LMR data link.* What does it mean that a convolutional encoder "has memory"? What is the state of the (7,5) encoder, how many states, and why does one input bit affect more than one output pair? How does this differ from a block code (Pack 05)?

**Q2 — Calculate.** *Context: hand-checking a test vector.* Encode `1101` with the (7,5) K=3 encoder including the 2-bit tail, in a table like §3.2. Then verify your two output streams with the polynomial method.

**Q3 — Calculate.** *Context: linearity and distance.* (a) Encode `0110` (with tail) and show `encode(1011) ⊕ encode(1101) = encode(0110)`. (b) Find `d_free` and explain how many errors are always correctable. (c) Why is `d_free` found from the encoding of a single 1?

**Q4 — Trace.** *Context: gateway debugging.* Run Viterbi on `rx = 11 01 00 00 10 11` (this is `encode(1101)` with one bit flipped). Fill in the metric table (S0..S3 per step), the survivor table, and the traceback. Give the decoded bits and metric.

**Q5 — Predict.** *Context: limits of the code.* (a) Why is any pair of errors in a terminated `encode(1011)` always corrected? (b) For 3 errors, 108 of 220 patterns on this 12-bit codeword decode wrongly. Predict what the decoder returns for errors at coded positions 0, 1, 2 (*hint: the answer is "0011"*), and explain why its reported metric is 2, *less* than the 3 real errors.

**Q6 — Calculate.** *Context: why this is practical at all.* A terminated message has 100 data bits. (a) How many candidate input sequences are there? (b) How many branch evaluations does Viterbi do for the K=3 code? (c) For K=7 (64 states)? (d) Expressed as a DP, what are stage, state and recurrence?

**Q7 — Predict.** *Context: bursty fading channel.* Coded stream of 28 bits is interleaved with a 4 × 7 block interleaver. A burst hits transmit slots 10, 11, 12, 13. Which coded positions are corrupted? What does the decoder see? What if the burst were 5 long?

**Q8 — Find the bug.** *Context: code review of a Viterbi decoder.*
```cpp
ViterbiResult decode(const Bits& rx) {
  const size_t steps = rx.size() / 2;
  std::array<unsigned, 4> metric{0, 0, 0, 0};                        // (a)
  std::vector<std::array<uint8_t, 4>> prev(steps), inp(steps);
  for (size_t t = 0; t < steps; ++t) {
    unsigned r = rx[2*t] | (rx[2*t + 1] << 1);                       // (b)
    std::array<unsigned, 4> next{0, 0, 0, 0};                        // (c)
    for (unsigned ps = 0; ps < 4; ++ps)
      for (unsigned u = 0; u < 2; ++u) {
        unsigned ns   = (u << 1) | (ps >> 1);
        unsigned cost = metric[ps] + (expected_pair(ps, u) ^ r);     // (d)
        if (cost < next[ns]) { next[ns] = cost; prev[t][ns] = ps; inp[t][ns] = u; }
      }
    metric = next;
  }
  unsigned s = 0; Bits out;
  for (size_t t = steps; t-- > 0; ) { out.push_back(inp[t][s]); s = prev[t][s]; }   // (e)
  return { out, metric[0] };
}
```
(Assume `expected_pair` returns `(out0 << 1) | out1` as in the reference.) Find at least five bugs.

**Q9 — Calculate.** *Context: PRBS / scrambler self-test.* By hand with the recurrence `ks[n] = ks[n−4] ⊕ ks[n−7]` and seed `0x01` (ks[−1] = 1, ks[−2..−7] = 0), compute the first 12 keystream bits. Then compute the scrambled version of data `1111 0000 1111` and descramble it again.

**Q10 — Explain.** *Context: air-interface scrambler.* (a) Prove that descrambling the additive scrambler is the same operation as scrambling. (b) What goes wrong with the wrong seed or one slipped bit? (c) 802.11 sets the first 7 SERVICE bits to zero: how does the receiver recover the seed? (d) What does a single channel bit error cause?

**Q11 — Calculate.** *Context: self-sync link on fibre.* Using the scrambler `out[n] = in[n] ⊕ out[n−4] ⊕ out[n−7]`: (a) where do the output errors appear if channel bit 20 is flipped? (b) after how many bits does a descrambler started in the wrong state lock? (c) how does that compare with the additive scrambler?

**Q12 — Connect.** *Context: interview "have you seen DP in the wild?"* (a) Fill the DP table for LC 931 on `{{2,1,3},{6,5,4},{7,8,9}}`, recover the path, and map each element to Viterbi (stage/state/metric/survivor). (b) Which other LeetCode problems share this structure (VERIFY numbers)? (c) What changes if the weights are on the edges rather than the nodes?

**Q13 — Implement.** *Context: gateway FEC library.* Implement all stubs in `code/include/viterbi.h` (encode, Viterbi, interleaver, LFSR, both scramblers, falling-path DP). `ctest` all green; sanitizers silent.

**Q14 — Connect.** *Context: whole module.* (a) CRC ↔ LFSR: describe the Galois and Fibonacci forms and show they produce the same sequence for `x⁷+x⁴+1`; (b) a scrambler as polynomial division vs a CRC as division remainder; (c) where do Pack 04 (CRC), Pack 05 (block FEC) and this pack sit in a TX chain, in what order?

**Q15 — Interview.** *Context: phone screen.* "Explain the Viterbi algorithm to a software engineer who knows dynamic programming. Why is it correct? Complexity? Hard vs soft decisions?"

**Q16 — Interview.** *Context: system design.* "Design the bit pipeline for a narrowband data link with fading bursts, a 16-bit header that must be very reliable, and a requirement that the receiver can resync mid-stream. Choose and order: CRC, block FEC, convolutional FEC + Viterbi, interleaver, scrambler (additive vs self-sync). Say what each failure looks like."

---

## §6 Self-assessment

- [ ] I can encode a stream with the (7,5) code by hand (table) and with the polynomial method.
- [ ] I can write the 4-state table (state, u → next, output) from memory by deriving it.
- [ ] I can run Viterbi on a 6-step trellis with a metric table and survivor table, then trace back.
- [ ] I can explain why dropping non-best arrivals into a state is safe (optimal substructure).
- [ ] I can say how the algorithm relates to LC 931 / 746 / 120 / 64 / 72.
- [ ] I can interleave/deinterleave a block, and say what a burst becomes.
- [ ] I can hand-generate an LFSR sequence from a seed, and state period 127 and the seed-0 trap.
- [ ] I can prove descrambling works and compare additive vs self-sync scramblers (error behaviour).
- [ ] I can state how a CRC register relates to an LFSR (Galois vs Fibonacci).
- [ ] `ctest`: all `solution.*` and (after Q13) `student.*` tests pass; sanitizers silent.

**Rapid fire (key in `answers.md`):** (1) Rate of a (7,5) code with a 100-bit message and 2-bit tail? (2) Number of states for K=7? (3) Period of a maximal 7-bit LFSR? (4) 1 channel error through a 3-tap self-sync descrambler → how many? (5) Order at the TX: FEC, interleave, scramble?

**VERIFY list:** IEEE 802.11 §17.3.5.5 scrambler `x⁷+x⁴+1` and the 0x0E start for the all-ones seed; 802.11 convolutional code K=7 (133, 171 octal) and puncturing; GSM K=5; CCSDS K=7; P25 (TIA-102.BAAA) and DMR (ETSI TS 102 361-1) trellis rates and interleavers; Ethernet 64b/66b self-sync scrambler `x⁵⁸+x³⁹+1` (IEEE 802.3 cl. 49); ~2 dB soft-decision gain; LeetCode problem numbers 931, 746, 120, 64, 72; Viterbi 1967 (IEEE Trans. Inf. Theory) and Forney 1973 (Proc. IEEE) references.
