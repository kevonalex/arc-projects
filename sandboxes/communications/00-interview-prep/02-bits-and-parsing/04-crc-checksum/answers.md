# Pack 04 — Answers (CRC & Internet checksum)

Each answer: **reasoning** → **common wrong answers** (*why tempting* / *why wrong*). `§` = [`worksheet.md`](worksheet.md) section. All data synthetic. Numeric CRC values were cross-checked with an independent bit-level computation; VERIFY against the RevEng catalogue before quoting them externally.

---

## Q1 — XOR is subtraction (Explain)

| a | b | a − b mod 2 | a XOR b |
|---|---|---|---|
| 0 | 0 | 0 | 0 |
| 0 | 1 | −1 ≡ 1 | 1 |
| 1 | 0 | 1 | 1 |
| 1 | 1 | 0 | 0 |
The last two columns are identical in all four rows, so subtraction mod 2 *is* XOR. In ordinary subtraction `0 − 1` needs a borrow from the next column; mod 2 it simply gives 1 with **nothing carried or borrowed**. So every column is independent and polynomial long division reduces to "if the leading bit is 1, XOR the divisor under it, shift". (§3.2)

**Common wrong answers.**
- *"XOR is just a convenient approximation of subtraction."* — Tempting: arithmetic habit. Wrong: in GF(2) it is *exactly* subtraction (and addition).
- *"There is a borrow, it's just discarded."* — Wrong: discarding a carry/borrow is arithmetic mod 2ⁿ (integer wrap); GF(2) polynomial arithmetic has no inter-column interaction at all.

---

## Q2 — Toy CRC-3 (Calculate)

Use `m(x)·x³ mod g`. Using the powers table `x⁰..x⁶ mod g = 001, 010, 100, 011, 110, 111, 101`:
- (a) `1010` → `1010000` = `x⁶ + x⁴` → `101 ^ 110 = 011`. By division: `1010000 ^ 1011000 = 0001000`; `0001000 ^ 0001011 = 0000011` ✓. **CRC = 011**, codeword `1010011`; check: `1010011 → 0001011 → 0` ✓.
- (b) `0110` → `0110000` = `x⁵ + x⁴` → `111 ^ 110 = 001`. By division: `0110000 ^ 0101100 = 0011100`; `^ 0010110 = 0001010`; `^ 0001011 = 0000001` ✓. **CRC = 001**, codeword `0110001`.
- (c) `1111` → `x⁶+x⁵+x⁴+x³` → `101^111^110^011 = 111`. By division: `1111000 ^ 1011000 = 0100000`; `^ 0101100 = 0001100`; `^ 0001011 = 0000111` ✓. **CRC = 111**, codeword `1111111`.
Linearity check: CRC(0101) = `0101000 ^ 0101100 = 0000100` → `100`; CRC(1010) ^ CRC(0101) = `011 ^ 100 = 111` = CRC(1111) ✓ (since `1010 ^ 0101 = 1111`).

**Common wrong answers.**
- *CRC(1010) = 000 (stopping after the first XOR, reading the last three bits of `0001000`).* — Tempting: the first XOR removed the top bit and the low three bits look "done". Wrong: a 1 remains at bit 3, which is still at or above the divisor's top position; it must be cleared too (second XOR) before the last 3 bits are the remainder.
- *Using the 4-bit message without appending three zeros.* — Tempting: "divide the message". Wrong: the check bits must live *below* the message; you divide `m·x³` (§3.2).
- *Forgetting leading bit 0 lines are skipped, XOR-ing anyway.* — Wrong: XOR the divisor only when the current leading bit is 1.

---

## Q3 — Single-bit errors (Predict)

The syndrome of an error at bit position *i* (bit 0 = rightmost) is `xⁱ mod g`: pos 0 → `001`, 1 → `010`, 2 → `100`, 3 → `011`, 4 → `110`, 5 → `111`, 6 → `101`. **Position 4 gives `110`.** Example: `1010011 ^ 0010000 = 1000011` → remainder `110` (worked in §4). All seven are different and non-zero because `x⁰…x⁶` are the seven distinct nonzero 3-bit values (g is primitive: `x` has order 7 mod g) — that distinctness is what lets a CRC *locate* a single error, and it is exactly the Hamming (7,4) decoder of Pack 05.

**Common wrong answers.**
- *"Syndrome 110 means position 6."* — Tempting: reading `110` as binary 6. Wrong *here*: for the cyclic code the syndrome is `xⁱ mod g` (position 4 ↔ `110`), not the binary value of the position. (In Pack 05's *non-cyclic* Hamming layout the syndrome *is* the position — a different arrangement of the same code.)
- *"Some single-bit errors give 000."* — Wrong: `g` has a nonzero constant term, so `g ∤ xⁱ`.

---

## Q4 — XMODEM by table and linearity (Calculate)

`table[3] = table[1] ^ table[2] = 0x1021 ^ 0x2042 = 0x3063`. With init 0, a single byte `b` gives `crc = table[b]`: `0x01 → 0x1021`, `0x02 → 0x2042`, `0x03 → 0x3063`. For `{00 01}`: after `0x00` the register is `table[0] = 0`; then `0x01` → `0x1021`: **identical to the CRC of `{01}`**. Observation: init 0 cannot see leading zero bytes (§3.4).

**Common wrong answers.**
- *`table[3] = 0x1021 + 0x2042 = 0x3063` "by luck of addition".* — The value matches here because no bits overlap; the correct justification is XOR (linearity). Try `table[0x12] = table[0x10] ^ table[0x02]` (`0x1231 ^ 0x2042 = 0x3273`) — addition would give `0x3273` here too but fails whenever bits overlap.
- *`{00 01}` has a different CRC "because it is longer".* — Tempting from intuition. Wrong for init = 0.

---

## Q5 — Reflection (Explain)

Serial links (UART, Ethernet, HDLC/PPP, many radio data links) send each byte **least-significant bit first**. Dividing in *transmission order* means the first bit on the wire is the highest-degree coefficient, i.e. bit 0 of each byte plays the role of an MSB. Instead of reversing each byte in software, you reflect the whole algorithm once: shift **right**, test the **LSB**, use the reflected polynomial. And the CRC itself is transmitted LSB-first, so the final register is reflected back (`refout`). `reflect(0x1021,16)`: `0001 0000 0010 0001 → 1000 0100 0000 1000 = 0x8408`.

**Common wrong answers.**
- *"Reflection makes the CRC stronger."* — Tempting: extra operation. Wrong: same code, different bit order.
- *`reflect(0x1021) = 0x2110` (byte swap).* — Tempting: confusing reflection with endian swap. Wrong: reflection reverses **bits**, not bytes.

---

## Q6 — Traces (Trace)

Bitwise, init 0, byte `0x01`: `crc = 0x0100`. Steps: `0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000`, then step 8: top bit set → `(0x8000<<1)=0x0000 ^ 0x1021 = 0x1021`. Table-driven `{01 02}`: byte 1: `idx = (0 >> 8) ^ 0x01 = 0x01`; `crc = (0 << 8) ^ table[1] = 0x1021`. Byte 2: `idx = (0x1021 >> 8) ^ 0x02 = 0x10 ^ 0x02 = 0x12`; `crc = (0x1021 << 8) & 0xFFFF = 0x2100`; `^ table[0x12]`; `table[0x12] = table[0x10] ^ table[0x02] = 0x1231 ^ 0x2042 = 0x3273` → `0x2100 ^ 0x3273 = 0x1373`.

**Common wrong answers.**
- *Truncating at 7 steps: result 0x8000.* — Tempting: stopping when the top bit is first set. Wrong: you always do **8** steps; the 8th is the one that XORs the polynomial.
- *Index `(crc >> 8) + b`.* — Wrong: XOR, not add.

---

## Q7 — Why the table method equals the bitwise one (Explain)

After `crc ^= b << 8` the register is `(t << 8) ^ L` with `t = (crc >> 8) ^ b` and `L = crc & 0xFF`. The 8 steps are *linear* (shift, XOR constant), so `steps(X ^ Y) = steps(X) ^ steps(Y)`. And `L` never influences a decision in these 8 steps: each step tests bit 15, `L`'s top bit sits at bit 7 and reaches bit 15 only after 8 shifts. Hence `steps((t<<8) ^ L) = steps(t<<8) ^ (L<<8)`. `steps(t<<8)` depends only on the 8-bit value `t`: precompute `table[t]`. The new register is `table[t] ^ (L << 8)`; since the top byte of `crc << 8` is `L`, that's `(crc << 8) ^ table[t]` (16-bit). ∎

**Common wrong answers.**
- *"It's a speed-up approximation; results may differ for long messages."* — Wrong: exact, our tests compare both on 200+ random buffers.
- *"Because the table is precomputed from the message."* — Wrong: it depends only on the polynomial.

---

## Q8 — Bugs in `crc16` (Find the bug)

1. `init = 0` (this is XMODEM's init, not CCITT-FALSE's `0xFFFF`): leading zero bytes become invisible.
2. `crc ^= p[i];` feeds the byte into the **low** byte; MSB-first CRC needs `crc ^= (uint16_t)p[i] << 8` so the byte lines up with the bit tested (`0x8000`). Result is a different code.
3. `crc << 1` and `(crc << 1) ^ 0x1021` are `int`s; assignment to `uint16_t` truncates (fine in effect) but `-Wconversion` warns; a cleaner `static_cast<uint16_t>(...)` is advised.
For `"123456789"` the correct value is `0x29B1`; this buggy code returns something else (compare with the check value to prove it).
If a CRC-32 returned `crc` instead of `~crc`, the final-XOR step is missing (xorout = 0 instead of `0xFFFFFFFF`): results won't match any peer, and the empty message would give `0xFFFFFFFF` instead of 0.

**Common wrong answers.**
- *"Only the `<< 8` is wrong."* — Tempting: it's the "hard" one. Wrong: init is also wrong against CCITT-FALSE; the spec's six parameters must match.
- *"`0x8000` should be `0x80`."* — Wrong: the register is 16 bits wide, the top bit is `0x8000`.

---

## Q9 — IPv4 checksum by hand (Calculate)

Words: `4500, 0073, 0000, 4000, 4011, 0000, c0a8, 0001, c0a8, 00c7`.
`4500+0073 = 4573`; `+0000 = 4573`; `+4000 = 8573`; `+4011 = c584`; `+0000 = c584`; `+c0a8 = 1862c` → fold `862c+1 = 862d`; `+0001 = 862e`; `+c0a8 = 146d6` → fold `46d6+1 = 46d7`; `+00c7 = 479e`. Checksum = `~479e = b861`. Verify: with `b861` in place the sum is `479e + b861 = ffff` ✓.

**Common wrong answers.**
- *`479e`.* — Forgetting the final complement.
- *Not zeroing the checksum field first.* — Gives `~(479e + b861) = ~ffff = 0000`... i.e. a meaningless value.
- *Adding the carry only once at the very end.* — Carries must be folded until none remain (here one fold suffices after each, but a long header can need two).

---

## Q10 — End-around carry (Explain)

One's-complement addition is addition modulo 65535 (2¹⁶ − 1) because `65536 ≡ 1 (mod 65535)`: a carry out of bit 15 (worth 65536) is worth `1` mod 65535 and is added back at the bottom. `ffff + 0001 = 0x10000` → fold → `0001`; modulo: `(65535 + 1) mod 65535 = 1` ✓ both give `0x0001`. (And `ffff` itself ≡ 0 mod 65535, so `0000` and `ffff` are two spellings of zero — "negative zero".)

**Common wrong answers.**
- *"The carry is thrown away, as in `uint16_t` overflow."* — Tempting from C. Wrong: that would be mod 65536; Internet checksum wraps *around*.
- *`ffff + 0001 = 0000`.* — Wrong for the same reason.

---

## Q11 — Byte-order independence (Predict)

`12 34 56 78` → `0x1234 + 0x5678 = 0x68AC`. `34 12 78 56` → `0x3412 + 0x7856 = 0xAC68`, which is `0x68AC` byte-swapped. Because swapping a word's bytes = ×256 mod 65535 and the sum is linear, swapping every word swaps the sum (§3.8). So an implementation that reads native-endian 16-bit words (little-endian CPU) computes the byte-swapped sum, and storing the result in native order puts the right bytes on the wire — no `htons` required if applied consistently.

**Common wrong answers.**
- *"Little-endian machines get a different checksum."* — Wrong: the *bytes on the wire* come out identical when used consistently.
- *"The sums are equal."* — Wrong: they are byte-swapped copies of each other.

---

## Q12 — Bugs in `udp_checksum` (Find the bug)

1. **Checksum field not zeroed** before summing (as the comment admits): computes `sum + old_checksum`.
2. **Odd length:** `seg[i+1]` reads **past the buffer** on the last iteration when `len` is odd (UB / ASan). Must pad: high byte `seg[len-1]`, low byte 0.
3. **Single fold:** `(sum & 0xFFFF) + (sum >> 16)` may itself carry; fold in a `while (sum >> 16)` loop.
4. **`return ~sum`** with `uint32_t sum` returns a 32-bit complement truncated to 16 — OK numerically but `~` on the promoted value is easy to mis-handle; mask explicitly `~sum & 0xFFFF`.
5. **Missing pseudo-header** (src IP, dst IP, zero, protocol 17, UDP length): the UDP checksum covers it.
6. **Zero result must be sent as `0xFFFF`** (RFC 768): `0x0000` means "no checksum" in IPv4.
7. 32-bit accumulator can overflow for > 128 KiB buffers — irrelevant for UDP (≤ 65,535) but use 64-bit in generic code.

**Common wrong answers.**
- *"The loop is fine: UDP segments are always even length."* — Tempting: audio frames are even. Wrong: arbitrary payloads exist; and fuzzers will find it.
- *"Zero checksum is fine, just send 0."* — Wrong: the receiver reads it as "no checksum", silently disabling protection.

---

## Q13 — Implement (Implement)

Reference: `code/solutions/crcsum.h`. Test → likely bug:
- `Gf2.WorkedExampleCrc3` — dividing without aligning the divisor under the leading 1, or not appending the zeros.
- `Generic.CatalogueCheckValues` — one of init/refin/refout/xorout wrong; the table in §3.6 diagnoses which.
- `Roles.*` — mixing LE/BE when appending the CRC.
- `Crc16Ccitt.TableDrivenEqualsBitwise` — table indexed with `crc` instead of `(crc>>8)^b`, or forgetting to mask to 16 bits.
- `InetChecksum.OddLength…` — padding the wrong byte lane.
- `InetChecksum.ByteOrderIndependence` — calling `ntohs` on words *and* using native loads.

**Common wrong answers.** *"Use `uint32_t crc` for 16-bit CRCs without masking."* — High bits from `<< 1` pollute the top-bit test; mask after every shift.

---

## Q14 — Connections (Connect)

**(a)** The bitwise CRC loop is a Galois LFSR: shift register + XOR of the polynomial taps when the outgoing bit is 1. With zero input it cycles through `xⁱ mod g` (our 7-state table). Pack 06's additive scrambler is the same machinery used to generate a pseudo-random keystream (Fibonacci form).
**(b)** `g = x³+x+1` generates the cyclic (7,4) Hamming code. The syndrome of a single error at position *i* is `xⁱ mod g`, unique for each *i*: a CRC that corrects instead of merely detects, when the message is short enough.
**(c)** TTL changes ⇒ the IPv4 header checksum must be updated. Recompute over the header, or use the RFC 1624 incremental formula `HC' = ~(~HC + ~m + m')` (VERIFY). The UDP/TCP checksums are unaffected by TTL (not in the pseudo-header) but *are* affected by address rewrites (NAT).

**Common wrong answers.** *"The payload checksum must change because the header changed."* — Wrong: only fields in the pseudo-header affect it.

---

## Q15 — Interview: CRC-32 (Interview)

```cpp
uint32_t crc32(const uint8_t* p, size_t n) {
  static uint32_t t[256]; static bool init = false;
  if (!init) { for (uint32_t i=0;i<256;++i){ uint32_t c=i; for(int k=0;k<8;++k) c=(c&1)?(c>>1)^0xEDB88320u:(c>>1); t[i]=c; } init=true; }
  uint32_t crc = 0xFFFFFFFFu;
  while (n--) crc = t[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
  return ~crc;
}
```
Why it works: §3.5 (linearity + low byte cannot affect the 8 steps; table computed by running the 8 steps on each byte value). `init = 0xFFFFFFFF` makes leading zeros visible; final `~` (xorout) gives a constant residue and detects trailing-zero tampering. Check: `"123456789"` → `0xCBF43926`. Mention thread-safe table initialisation (`static const` initialised via a function) and that hardware `crc32` instructions (SSE4.2 is CRC-32C, *not* Ethernet CRC-32) exist.

**Common wrong answers.**
- *"SSE4.2 `crc32` computes the standard CRC-32."* — Tempting: same name. Wrong: it is CRC-32C (Castagnoli, `0x1EDC6F41`) — a different polynomial.
- *"init and xorout don't matter."* — Wrong: interop breaks and leading-zero detection is lost (§3.4).

---

## Q16 — Interview: CRC vs checksum vs hash vs MAC (Interview)

- **Internet checksum:** 16-bit additive; catches most random single-word damage; blind to word reordering/compensating pairs; cheap. For headers.
- **CRC:** polynomial division; guaranteed to catch all bursts ≤ r bits, all odd-weight errors if `(x+1) | g`, and (depending on the polynomial and length) all errors up to a given weight; linear ⇒ **trivially forgeable**.
- **Cryptographic hash (SHA-256):** detects any change with overwhelming probability, but anyone can recompute it ⇒ no authentication.
- **MAC / AEAD (HMAC, AES-GCM, signature):** needs a secret/key ⇒ authenticates.
Firmware update: use a signature (e.g. Ed25519/ECDSA) or at least HMAC with a protected key; CRC only for *accident* detection (flash corruption) in addition.

**Common wrong answers.**
- *"A 32-bit CRC is as good as a hash."* — Tempting: both are 32-bit digests. Wrong: a hash resists engineered collisions; CRC does not.
- *"CRC-32 detects all errors."* — Wrong: it misses some multi-bit patterns (any multiple of `g`), probability ≈ 2⁻³² for random corruption.

---

## §6 Self-assessment key

(1) `0x1021` = `x¹⁶ + x¹² + x⁵ + 1` (leading `x¹⁶` implicit). (2) `0x29B1`. (3) With init 0 the register stays 0 over zero bytes, so `{00 01}` and `{01}` give the same CRC. (4) `0x0001` (end-around carry). (5) The Internet checksum (addition commutes).

**VERIFY (this pack):** see the list at the end of the worksheet (P25/DMR CRC fields, RevEng catalogue, RFC 1071/1624/768/1141, residue constants, SSE4.2 = CRC-32C).
