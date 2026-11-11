# Pack 04 — Cheatsheet (every line points to its derivation in `worksheet.md`)

Re-derive, don't memorise: open the cited section when a line is not obvious.

| Fact | Where derived |
|---|---|
| Idea: make `message·xʳ` divisible by `g`, send the remainder; receiver checks remainder = 0 | W §3.1 |
| GF(2): add = subtract = XOR; no carries, no borrows | W §3.2 |
| Long division: if leading bit is 1, XOR divisor aligned under it; repeat | W §3.2 |
| CRC = `(M << r) mod g`; codeword = `(M << r) \| CRC`; divisible because `R + R = 0` | W §3.2, §3.3 |
| Received remainder = `e(x) mod g`; missed only if `g \| e` | W §3.2 |
| `g` with constant term 1 ⇒ all single-bit errors caught; all bursts ≤ r bits caught | W §3.2 |
| `(x+1) \| g` ⇒ all odd-weight errors caught (CCITT: 4 terms) | W §3.2 |
| `0x1021` stands for `x¹⁶+x¹²+x⁵+1` (leading term implicit) | W §3.2 (box), §3.4 |
| CRC is linear ⇒ forgeable; for accidents, not security | W §3.2 (box), Q16 |
| Six parameters define a CRC: width, poly, init, refin, refout, xorout (+ check value) | W §3.4 |
| init = 0 cannot see leading zeros (`{00 01}` = `{01}` = `0x1021` in XMODEM) | W §3.4 |
| xorout ≠ 0 makes the residue a non-zero constant; CCITT-FALSE residue 0 misses a trailing `00` | W §3.4 |
| Reflection = LSB-first transmission; `reflect(0x1021)=0x8408`, `reflect(0x04C11DB7)=0xEDB88320` | W §3.4 |
| Check values for `"123456789"`: CCITT-FALSE `29B1`, XMODEM `31C3`, KERMIT `2189`, X-25 `906E`, ARC `BB3D`, CRC-32 `CBF43926`, CRC-32C `E3069283` (VERIFY catalogue) | W §3.4, §3.6 |
| Bitwise CRC-16: `crc ^= b<<8; 8× (top bit ? (crc<<1)^0x1021 : crc<<1)` | W §3.5 |
| Table method is exact: linearity + low byte can't reach bit 15 in 8 steps | W §3.5 |
| `crc = (crc<<8) ^ table[(crc>>8) ^ b]` (MSB-first); `crc = table[(crc^b)&0xFF] ^ (crc>>8)` (reflected) | W §3.5 |
| `table[1]=0x1021`, `table[2]=0x2042`, `table[3]=table[1]^table[2]` | W §3.5 |
| Check failure diagnosis: byte-swapped ⇒ output order; bit-reversed ⇒ reflection; always wrong ⇒ init/xorout | W §3.6 |
| CRC register = Galois LFSR; zero input walks powers `xⁱ mod g` (period 7 for `x³+x+1`) | W §3.7 |
| CRC-3 `g=1011` is the cyclic Hamming (7,4) code; syndrome of error at `i` is `xⁱ mod g` | W §3.7, §4 |
| Internet checksum: sum 16-bit BE words, fold carries (mod 65535), complement | W §3.8 |
| `0x10000 ≡ 1 (mod 0xFFFF)` ⇒ end-around carry | W §3.8 |
| Swapping a word's bytes = ×256 mod 65535 ⇒ checksum is byte-order independent | W §3.8 |
| Verify: sum including checksum field = `0xFFFF` | W §3.8 |
| Odd length: pad a zero byte at the END; UDP zero result is sent as `0xFFFF` | W §3.8 |
| Internet checksum is blind to word reordering; CRC is not | W §3.8, Q16 |
| RFC 1624 incremental update: `HC' = ~(~HC + ~m + m')` (VERIFY) | W §3.8 |

**VERIFY:** per-field CRC parameters in P25 (TIA-102.BAAA/BAAB) and DMR (ETSI TS 102 361); RevEng catalogue; RFC 1071, 1624, 768 (see W §1, §3.4, §3.8, §6).
