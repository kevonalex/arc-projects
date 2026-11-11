# Pack 05 — Cheatsheet (every line cites a worksheet section)

Use *after* the worksheet. If a line doesn't make sense, jump to the § and re-derive it.

## Parity
- Even parity bit = XOR of data bits = `popcount(x) & 1`; whole word has an even count of 1s. → §3.2
- XOR fold `x^=x>>16; >>8; >>4; >>2; >>1; x&1` works because XOR is associative. → §3.2
- Detects **odd** numbers of errors, misses all even; cannot correct. → §3.2, Q1

## Distance and spheres
- `d(a,b) = popcount(a ^ b)`; code distance `d` = min over distinct codeword pairs. → §3.3
- Linear code: `d` = minimum **nonzero weight** (XOR of codewords is a codeword). → §3.3, Q2
- Correct `t = ⌊(d−1)/2⌋`; detect-only `d−1`; correct `t` + detect `s` needs `d ≥ t+s+1`. → §3.3, Q3, Q11
- Radius-`t` balls disjoint ⟺ `d ≥ 2t+1`; beyond `t` errors the decoder may **miscorrect silently**. → §3.3, Q4
- Perfect code: `2^k · Σ_{i≤t} C(n,i) = 2^n`; no gaps ⇒ nothing detected beyond `t`. → §3.3, §3.6, Q10

## Hamming(7,4)
- Need `2^r ≥ k + r + 1` ⇒ `n = 2^r − 1`: (7,4), (15,11), (31,26). → §3.4, Q8
- Check *i* covers positions with bit *i* set in the binary index: G1 {1,3,5,7} = `0x55`, G2 {2,3,6,7} = `0x66`, G4 {4,5,6,7} = `0x78`. → §3.4
- Check bits at 1, 2, 4 (powers of two): each is in one group only ⇒ solved independently. → §3.4
- Syndrome `(s4 s2 s1)` = **position of the error** (0 = none); it equals XOR of positions of all 1-bits. → §3.4, Q6
- Position `k` = bit `k−1`; data nibble bit0..3 → positions 3,5,6,7. → §2, §3.4
- Nibble `0xB` → `0x55`; `6` → `0x33`; `9` → `0x4C`; `7` → `0x34`. → §3.4, §4, Q5, Q6
- Two errors ⇒ **miscorrection** (`0x56` → data `0xA`). → §3.4, Q7
- Syndrome depends only on the error pattern, never on the data. → §3.4
- Weights: 1×0, 7×3, 7×4, 1×7 ⇒ `d = 3`, rate 4/7. → §3.4

## Extended Hamming / SECDED
- Append overall even parity ⇒ (8,4,4). → §3.5
- `(s≠0, P=1)` correct one; `(s=0, P=1)` parity bit flipped; `(s≠0, P=0)` two errors → **detect only**. → §3.5, Q9
- Why: one flip changes overall parity, two don't. → §3.5

## Golay
- (23,12,7), `g(x) = 0xC75` (also `0xAE3`), cyclic; encode = `(d<<11) | ((d<<11) mod g)` (Pack 04 division). → §3.6
- Perfect: `1+23+253+1771 = 2048 = 2¹¹`, `2¹²·2¹¹ = 2²³`; `t = 3`; ≥4 errors miscorrect silently. → §3.6, Q10
- Extended (24,12,8): correct 3 + detect 4. Shortened (18,6,8) keeps `d`. → §3.6
- Weights: 0:1, 7:253, 8:506, 11:1288, 12:1288, 15:506, 16:253, 23:1. → §3.6
- Test vectors: `encode(1) = 0xC75`, `encode(0xABC) = 0x55E11E`. → §3.6, §4

## BCH
- Hamming is `t=1` BCH; BCH bound: roots `α..α^{2t}` ⇒ `d ≥ 2t+1`; `n = 2^m − 1`; ~`m·t` check bits. → §3.7
- P25 NID: BCH(63,16,23) (+1 parity), `t = 11`, 47 check bits (**VERIFY**). → §3.7, §1
- Gateway rewrites NAC ⇒ **recompute BCH**; stale checks make the decoder "correct" the edit away (9 ≤ 11). → §4, Q14
- BCH(31,16,7): `t=3`, 31 bits for 16 data (**VERIFY**). → Q16

## Reed–Solomon
- `k` points fix a degree `k−1` polynomial; send `n > k` evaluations. → §3.8
- `d = n − k + 1` (MDS); `2·errors + erasures ≤ n − k`. → §3.8
- Symbols not bits: a burst inside one symbol = one error. → §3.8
- RS(255,223) corrects 16 symbol errors. → §6 (rapid fire)
- Toy GF(5): `(3,1) → 3,4,0,1`; recover from any 2 symbols. → §3.8, §4

## Rules of thumb
- Decoder confidence ≠ correctness: add an outer CRC (Pack 04) or use SECDED/extended codes. → §3.3, §4, Q16
- Bursts break block codes: interleave (Pack 06). → §3.4, Q7, Q14
- FEC vs ARQ: FEC = fixed overhead, no back-channel; ARQ = delay + ACK path. → §1, Q16
- Never use real radio captures: synthetic words only. → header
