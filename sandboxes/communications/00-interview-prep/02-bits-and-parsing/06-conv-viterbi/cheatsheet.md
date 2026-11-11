# Pack 06 — Cheatsheet (every line cites a worksheet section)

Use *after* the worksheet. If a line doesn't make sense, jump to the § and re-derive it.

## Convolutional encoder (7,5), K=3, rate 1/2
- State = `(u[n−1], u[n−2])`; 4 states; the current input picks the branch. → §3.2, §3.3
- `out0 = u ⊕ u1 ⊕ u2` (gen 111 = 7); `out1 = u ⊕ u2` (gen 101 = 5); pair order out0 then out1. → §3.2
- Next state = `(u, u1)`; code: `next = ((u<<1)|(s>>1)) & 3`. → §3.3
- Tail = two 0 inputs → ends in S0 (traceback start). Costs rate, not protection. → §3.2
- Test vectors: `1011 → 11 10 00 01 01 11`; `1 → 11 10 11`; `1101 → 11 01 01 00 10 11`; `0110 → 00 11 01 01 11 00`. → §3.2, §4, Q2, Q3
- Encoder = polynomial multiplication (convolution) over GF(2): `out0 = u·(1+D+D²)`, `out1 = u·(1+D²)`. → §3.2
- Linear ⇒ `d_free` = weight of the encoding of `1 0 0` = **5** ⇒ any 2 errors fixed. → §3.2, Q3

## Viterbi (hard decision)
- Branch metric `bm = popcount(expected ⊕ received)` (0..2); path metric = sum. → §3.4
- Add–compare–select per state; keep survivor `(prev, u)`; strict `<`; tie rule: prev ascending, `u=0` first. → §3.4
- Start `metric[S0]=0`, others ∞; end at S0; traceback, reverse, drop 2 tail bits. → §3.4, Q8
- Why pruning is safe: future cost depends only on the state (optimal substructure). → §3.4
- Cost `O(T·2^(K−1))` (8 ops/step for K=3; 128 for K=7) vs `2ⁿ` paths. → §3.3, §3.4, Q6
- Metric = distance to the *chosen* codeword, not the number of real errors; ≥3 errors may miscorrect silently (`0,1,2 → 0011`). → §3.4, Q5
- Soft decision: analog branch metric, ~2 dB gain (VERIFY). → §3.4
- Worked table: rx `11 10 10 01 01 11` → final S0 metric 1 → `1011`. → §4

## Viterbi ↔ DP (LeetCode)
- Stage = row/time, state = column/memory state, `dp` = path metric, back-pointer = survivor. → §3.5
- LC 931: `dp[r][c] = m[r][c] + min(up-left, up, up-right)`; `{{2,1,3},{6,5,4},{7,8,9}} → 13`. → §3.5, Q12
- Cousins: LC 746, 120, 64, 72 (VERIFY numbers); greedy fails. → §3.5, Q12

## Interleaving
- Write row-wise, read column-wise; `j = c·rows + r` (transmit) ⇄ `i = r·cols + c` (coded). → §3.6
- Burst of `rows` consecutive channel bits → scattered errors `cols` apart (4×7: positions 2, 9, 16, 23). → §3.6, §4, Q7
- Adds **no** redundancy; needs `rows ≥ burst length`; costs latency/memory. → §3.6
- Order: FEC → interleave → channel; reverse at RX. → §3.6, Q14
- Without it: a 4-bit burst defeats the K=3 code at all 25 offsets (tested message). → §4

## LFSR
- `x⁷+x⁴+1`: `ks[n] = ks[n−4] ⊕ ks[n−7]`. → §3.7
- Code: `fb=((s>>6)^(s>>3))&1; s=((s<<1)|fb)&0x7F; out fb`; state = last 7 outputs; bit k−1 = k steps ago. → §3.7
- Seed `0x7F` → `0000 1110 1111 0010…` (first byte `0x0E`); seed `0x01` → `000100110001…`. → §3.7, Q9
- Period 127, 64 ones/63 zeros, max run 7 (ones) / 6 (zeros); seed 0 = stuck at 0. → §3.7
- Not secure: 14 output bits reveal the state. → §3.7
- Fibonacci vs Galois: same polynomial, same cycle, different state (Galois right-shift mask `0x48`). → §3.7, Q14

## Scramblers
- Additive: `tx = d ⊕ ks`; descramble = scramble since `(d⊕k)⊕k = d`. → §3.8, Q10
- Needs same seed/alignment; one slip ruins the rest; 1 channel error → 1 error. → §3.8
- 802.11: first 7 SERVICE bits are 0 so received bits = keystream = state (VERIFY). → §3.8, Q10
- Self-sync: `out[n] = in[n] ⊕ out[n−4] ⊕ out[n−7]`; descrambler `dec[n] = rx[n] ⊕ rx[n−4] ⊕ rx[n−7]`. → §3.9
- Locks after 7 bits; 1 channel error → 3 errors at `n, n+4, n+7`. → §3.9, Q11
- Scrambling = whitening (clock recovery, spectrum), **not** encryption. → §3.7, §3.8

## Cross-links and rules of thumb
- CRC register = Galois LFSR; self-sync scrambler = division (quotient), CRC = remainder. → §3.7, §3.9, Q14
- Always put a CRC outside FEC to catch miscorrection. → §3.4, Q5, Q16
- Unequal error protection: strong block code for headers, convolutional for payload. → Q16
- Synthetic data only; never live public-safety RX. → header
