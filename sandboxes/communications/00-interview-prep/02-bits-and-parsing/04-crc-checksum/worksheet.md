# Pack 04 — CRC and the Internet Checksum (Etherstack interview worksheet)

> **Audience:** from zero; C++17/20. **Rule:** do the long division by hand *before* looking at any code. Every parameter of a CRC is derived or demonstrated by a failure, never just stated.
> **Data rule:** all messages, packets and test vectors here are **synthetic** (`"123456789"`, made-up headers). No live public-safety RX.
> **Companions:** [`answers.md`](answers.md) · [`cheatsheet.md`](cheatsheet.md) · `code/` (stubs `code/include/crcsum.h`, tests, reference in `code/solutions/`).
> **Cross-links:** Pack 01 (XOR, shifts, reflect) · Pack 03 (IPv4/UDP headers carry these checksums) · **Pack 05** (the CRC-3 below *is* the (7,4) cyclic Hamming code) · **Pack 06** (a CRC register *is* an LFSR).

Sections (the cheatsheet cites these): **§1** Why · **§2** Prerequisites · **§3** Concept build-up (3.1–3.8) · **§4** Worked example · **§5** Questions Q1–Q16 · **§6** Self-assessment.

---

## §1 Why this matters

Noise flips bits. A receiver needs a cheap, strong way to say "this frame is damaged, throw it away (or ask again)".

| Where | What it is used for |
|---|---|
| **LMR air interfaces** | Data headers/blocks and packet data are protected with CRCs (P25 TIA-102.BAAA/BAAB and DMR ETSI TS 102 361 define which polynomial protects which field — **VERIFY the exact polynomial/init/xorout per field in the spec you are given**) |
| **Gateway / network** | Ethernet's frame check sequence is CRC-32; IPv4 header and UDP/TCP use the Internet checksum (Pack 03) |
| **Embedded** | Flash image integrity, bootloader checks, serial protocols (CRC-16 variants), EEPROM records |
| **Interview** | "Implement CRC-32", "why is the table-driven version the same?", "why do init and final-XOR exist?", "CRC vs checksum?" |

**You will be able to:** divide polynomials over GF(2) by hand; explain what poly/init/reflect/xorout each do and show the failure when one is wrong; derive the table-driven algorithm from the bitwise one; reproduce the check value of `"123456789"`; and explain why the Internet checksum doesn't care about byte order.

**Failure if ignored:** a "CRC" that matches your own code but nobody else's (wrong init/reflection) passes loopback tests and fails the first interop with a real radio or router.

---

## §2 Prerequisites

- XOR (Pack 01 §2): `a ^ a = 0`, `a ^ 0 = a`, XOR is its own inverse.
- Shifts: `x << 1` doubles, `x >> 1` halves (unsigned). Reflect = reverse the bits of a value (`reflect(0b0001,4) = 0b1000`).
- Long division of decimal numbers (you remember it from school; we will do the same with bits).
- A polynomial is a sum of powers of *x*: `x³ + x + 1`. We only need its **coefficient list**.
- Check-values: standard test message is the 9 ASCII characters `"123456789"` = bytes `31 32 33 34 35 36 37 38 39`.

---

## §3 Concept build-up

### 3.1 Concrete first: "make the total divisible, then test the remainder"

Decimal toy. Divisor 7. Message `12`.
1. Make room: `12 → 1200` (shift left by two decimal digits).
2. Remainder of the shifted message: `1200 mod 7`: `7 × 171 = 1197`, remainder **3**.
3. Send `1200 − 3 = 1197` (now divisible by 7: `1197 = 7 × 171`).
4. The receiver divides by 7: remainder 0 ⇒ "looks fine". Any change like `1198` gives remainder 1 ⇒ "damaged".

CRC is *exactly this* with **binary polynomials**, where "subtract" is XOR (so no `−3`, just add the remainder back), and "divisor 7" is the **generator polynomial**.

### 3.2 GF(2) polynomial division by hand

**Bits as polynomials.** Read a bit string as coefficients: `1101` = `1·x³ + 1·x² + 0·x + 1` = `x³ + x² + 1`. Multiplying by *x* appends a 0 (shift left). Multiplying by *x³* appends three 0s.

**Why XOR is subtraction (derive).** Arithmetic in GF(2) is arithmetic **mod 2**, one bit per column, no carries between columns:

| a | b | a + b mod 2 | a − b mod 2 | a XOR b |
|---|---|---|---|---|
| 0 | 0 | 0 | 0 | 0 |
| 0 | 1 | 1 | 1 (since −1 ≡ 1) | 1 |
| 1 | 0 | 1 | 1 | 1 |
| 1 | 1 | 0 | 0 | 0 |

Columns 3 and 4 equal column 5. So **add = subtract = XOR**, and there is **never a borrow** — that is what makes CRC hardware-cheap.

**Worked division.** Generator `g(x) = x³ + x + 1 = 1011` (degree r = 3). Message `1101`. Step 1: multiply by xʳ: append r zeros → `1101000` (7 bits). Divide by `1011`:

```
        1101000          <- dividend (message + 000)
   XOR  1011             <- top bit is 1: subtract (XOR) divisor, aligned under it
        -------
        0110000
   XOR   1011            <- top bit now at position of the next 1
        -------
        0011100
   XOR    1011
        -------
        0001010
   XOR      1011
        -------
        0000001          <- remainder (the last 3 bits): 001
```
Rules: look at the leading 1 of what remains; XOR the divisor aligned with it; repeat until fewer than r+1 bits remain. If the leading bit is 0, just move on.

**CRC = remainder = `001`.** The transmitted **codeword** is message followed by remainder: `1101 001`.

**Why the codeword is divisible (derive).** Codeword `= M·xʳ + R`, where `R = (M·xʳ) mod g`. So `M·xʳ = Q·g + R`. Then `codeword mod g = (Q·g + R + R) mod g = 0`, because `R + R = 0` in GF(2) (that is the "XOR as subtraction" paying off). ∎ Check by hand: `1101001 ÷ 1011` → remainder `000`.

**What errors are caught (derive).** Received `r(x) = codeword(x) + e(x)`, where *e* marks the flipped bits. Remainder of the received word `= e(x) mod g` (the codeword part vanishes). The error is **missed only if g divides e(x)**.
- **Single-bit error** `e = xⁱ`. `g` has constant term 1, so `x` does not divide `g`, and `g` (degree ≥ 1, constant term 1) can never divide `xⁱ`. ⇒ *every* single-bit error is detected. Concretely for our `g`: `xⁱ mod g` for i = 0..7 is `001 010 100 011 110 111 101 001` — never 0, and the sequence repeats after 7. (We use this table again in Pack 05 and Pack 06.)
- **Burst error of length ≤ r** `e = xⁱ·b(x)` with deg b < r and b(0)=1: `g` is coprime to `xⁱ` and *cannot divide* `b`, since deg b < deg g and b ≠ 0 ⇒ detected. **Any burst up to r bits long is caught** — which is *why CRCs beat additive checksums on noisy radio links*, where errors come in bursts.
- **Odd number of errors:** if `g` contains the factor `(x+1)`, then every codeword has an even number of 1s (because `g(1) = 0`), so any odd-weight error is caught. CRC-16/CCITT (`x¹⁶+x¹²+x⁵+1`: 4 terms ⇒ `g(1)=0`) has this property.
- **Failure example (a generator with no constant term):** take `g = x³ + x² = 1100 = x²(x+1)`. The error `e = x³ + x²` (two *adjacent* flipped bits, a burst only 2 bits long, well within r = 3) equals `g` itself, so `e mod g = 0` and it is **undetected**. The proofs above needed `g` to be coprime to `xⁱ`, which requires a constant term 1; always give `g` one.

> **Misconceptions box 3.2**
> 1. *"CRC is a fancy sum."* A sum adds integers with carries; a CRC divides polynomials with *no* carries. Reordering bytes changes a CRC but not a plain sum (§3.8).
> 2. *"The polynomial 0x1021 is 16 bits so the CRC is 15 bits."* The leading `x¹⁶` term is **implicit**: `0x1021` stands for `x¹⁶ + x¹² + x⁵ + 1`; the remainder has 16 bits.
> 3. *"CRC protects against malicious change."* CRC is linear: an attacker can adjust other bytes to restore the same CRC. It's for accidents, not security (use a MAC/AEAD).
> 4. *"A longer message weakens the CRC."* It never *breaks* burst detection (≤ r bits) but the guarantee for multi-bit random errors depends on the polynomial and the message length (Hamming distance drops with length).

### 3.3 Systematic encoding, in one line

Codeword = `(M << r) | ((M << r) mod g)`. The message bits travel unchanged; the r check bits ride at the end. The receiver can either (a) recompute the CRC of the message and compare, or (b) run the CRC over message **+** CRC and test for the constant *residue* (§3.4).

### 3.4 What each parameter does (and the failure when it is wrong)

The **Rocksoft model** describes any CRC by: `width`, `poly`, `init`, `refin`, `refout`, `xorout`, plus a `check` value. All the "different CRCs" with the same polynomial differ only in these.

| Parameter | Role | Failure if wrong / missing |
|---|---|---|
| **width** | Remainder size r (8/16/32). Burst length guaranteed detected = r | Wrong width → different result length |
| **poly** (normal form, no leading term) | Which divisor; determines detection strength. 0x1021 (CCITT), 0x04C11DB7 (CRC-32), 0x8005 (ARC) | Different polynomial → different (and differently strong) code |
| **init** | Register start value | **init = 0 cannot see leading zero bytes**: with init 0 the register stays 0 through zero bytes. Demo: CRC-16/XMODEM of `{00 01}` and `{01}` are **both `0x1021`** — a message with an extra leading `00` is undetected. CCITT-FALSE (init `FFFF`): `{00 01}` → `0x0D2E`, `{01}` → `0xF1D1` (different) |
| **refin** | Reflect each input byte before processing | Serial links (UART, Ethernet) send each byte **LSB first**; reflection lets software process bytes LSB-first with right shifts. Mismatch ⇒ the other camp's answer (XMODEM `0x31C3` vs KERMIT `0x2189` for `"123456789"`, same poly/init) |
| **refout** | Reflect the final register | Same: the CRC value is transmitted LSB-first, so it must be reflected back |
| **xorout** | XOR into the final value | (1) gives the empty message a non-zero CRC (CRC-32: init `FFFFFFFF` ^ xorout `FFFFFFFF` = `0` for empty — a curiosity, not a weakness); (2) **residue is a non-zero constant**, so *stray appended zero bytes are detected*. Demo: CCITT-FALSE has `xorout = 0`; run it over message+CRC (big-endian) and the result is `0x0000` — and still `0x0000` after **another** `00` byte is appended: undetected. CRC-32 over message+CRC (CRC little-endian) always gives the constant residue `0x2144DF1C`; one extra `00` changes it |
| **check** | Value for `"123456789"` | The unit test that tells you the other six are right |

**Reflection, derived.** If bits go on the wire LSB-first, then within each byte **bit 0 is the first bit sent**, i.e. the *highest-degree* coefficient. To feed bytes into an MSB-first divider you would reflect each byte. Instead, **reflect everything once** (polynomial, register, shift direction): the divider shifts **right** and tests the **LSB**, and the polynomial becomes `reflect(poly)`. Check: `reflect(0x04C11DB7, 32) = 0xEDB88320`; `reflect(0x1021, 16) = 0x8408` (0001 0000 0010 0001 → 1000 0100 0000 1000).

**Check values for `"123456789"`** (all verified against an independent bit-level computation; VERIFY against the RevEng catalogue):

| Name | width | poly | init | refin | refout | xorout | check |
|---|---|---|---|---|---|---|---|
| CRC-16/CCITT-FALSE (also IBM-3740) | 16 | 0x1021 | 0xFFFF | no | no | 0x0000 | **0x29B1** |
| CRC-16/XMODEM | 16 | 0x1021 | 0x0000 | no | no | 0x0000 | **0x31C3** |
| CRC-16/KERMIT (CCITT-TRUE) | 16 | 0x1021 | 0x0000 | yes | yes | 0x0000 | **0x2189** |
| CRC-16/X-25 (IBM-SDLC) | 16 | 0x1021 | 0xFFFF | yes | yes | 0xFFFF | **0x906E** |
| CRC-16/ARC | 16 | 0x8005 | 0x0000 | yes | yes | 0x0000 | **0xBB3D** |
| CRC-32 (ISO-HDLC, Ethernet, zlib) | 32 | 0x04C11DB7 | 0xFFFFFFFF | yes | yes | 0xFFFFFFFF | **0xCBF43926** |
| CRC-32C (Castagnoli, iSCSI) | 32 | 0x1EDC6F41 | 0xFFFFFFFF | yes | yes | 0xFFFFFFFF | **0xE3069283** |

Residues when the CRC is appended in the way the algorithm expects (computed): CCITT-FALSE (BE append) `0x0000`; X-25 (LE append) `0x0F47`; CRC-32 (LE append) `0x2144DF1C`.

> **Misconceptions box 3.4**
> 1. *"CRC-CCITT is one algorithm."* Four (at least) common variants share `0x1021` and differ in init/reflect/xorout (above). "CCITT" in a spec without parameters is ambiguous — **always get all six parameters** (or a check value).
> 2. *"Reflection is a different polynomial."* It is the same code with a different bit order on the wire.
> 3. *"xorout is cosmetic."* It determines the residue and whether stray trailing zeros are caught.
> 4. *"If my CRC matches my own receiver it is correct."* Only the check value proves interop.

### 3.5 Bitwise ⇒ table-driven (proof they are the same)

**Bitwise CRC-16/CCITT-FALSE** (MSB-first, poly 0x1021):

```cpp
crc = 0xFFFF;
for each byte b:
  crc ^= b << 8;                           // feed the byte into the TOP of the register
  repeat 8 times:
    crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);   // shift; if a 1 fell out, XOR poly
```
This *is* the long division of §3.2, folded into a register: the top bit of the register is the "leading 1"; XOR-ing the poly is subtracting the divisor.

**Why a table is allowed.** Let `crc` have top byte `T` and low byte `L`. After XOR-ing the input byte, the top byte is `t = T ^ b`, so the register is `(t << 8) ^ L`. Eight bit-steps are applied. Two facts:
1. **Linearity:** shifting and XOR-ing a constant are linear over XOR, so `steps(A ^ B) = steps(A) ^ steps(B)`.
2. **`L` cannot influence any decision** in these 8 steps: each step looks at bit 15, and `L`'s bit 7 is at position 7 and needs 8 shifts to reach bit 15, i.e. it is looked at only at step 9.
Therefore `steps((t << 8) ^ L) = steps(t << 8) ^ (L << 8)`. Define `table[t] = steps(t << 8)`. Then:

```cpp
crc = (crc << 8) ^ table[(crc >> 8) ^ b];  // one lookup instead of 8 shift/test/xor steps
```
**Derive table[1] by hand:** register `0x0100`: 7 shifts → `0x8000`; the 8th shift pushes the 1 out and XORs: `0x0000 ^ 0x1021 = 0x1021`. `table[2]`: `0x0200` → 6 shifts → `0x8000`; 7th → `0x1021`; 8th → `0x1021 << 1 = 0x2042` (top bit 0, no XOR). By linearity `table[3] = table[1] ^ table[2] = 0x3063`. Also `table[0x10] = 0x1231`, `table[0x80] = 0x9188`, `table[0xFF] = 0x1EF0`.

**Reflected CRC-32** is the mirror image: `crc = table[(crc ^ b) & 0xFF] ^ (crc >> 8)`, with `table[i]` = 8 right-shift steps of `i` using `0xEDB88320`: `table[1] = 0x77073096`, `table[128] = 0xEDB88320` (the polynomial itself, since a lone 1 at the top ends up XOR-ing it once), `table[255] = 0x2D02EF8D`.

> **Misconceptions box 3.5**
> 1. *"The table-driven CRC is an approximation."* It is mathematically identical (proof above). Our tests compare both on hundreds of random buffers.
> 2. *"The table depends on the message."* It depends only on the polynomial (and direction); build once.
> 3. *"Bigger tables are always better."* Slicing-by-4/8/16 tables trade cache for speed; on an MCU a 256×2-byte table is usually the sweet spot, a bit-at-a-time loop the smallest.

### 3.6 Verify against `"123456789"`

The nine ASCII digits are the universal CRC smoke test because they are short, printable and *every* catalogue lists the result. Use it like a unit test: compute `crc_generic(params, "123456789")` and compare with the table in §3.4. **Diagnosis when it fails:**

| Symptom | Likely cause |
|---|---|
| Result is the byte-swapped value (e.g. `0xB129` instead of `0x29B1`) | Output byte order (you wrote the CRC little-endian instead of big-endian when printing) |
| Result is bit-reversed | refin/refout mismatch |
| Result differs for *every* input, same polynomial | init or xorout wrong |
| Result right for `"123456789"` but wrong for 1-byte messages | Table/off-by-one bug in the last byte |
| Matches for all inputs except leading `00` | init = 0 where it should be `FFFF` |

Hand check of a tiny case: CRC-16/XMODEM of the single byte `0x01` is `table[1] = 0x1021` (init 0 ⇒ `crc = 0 ^ 0x0100` ⇒ 8 steps = table[1]). `{01 02}` → first byte gives `0x1021`; second: index `(0x10) ^ 0x02 = 0x12`, `crc = (0x1021 << 8 & 0xFFFF) ^ table[0x12] = 0x2100 ^ 0x3273 = 0x1373`.

### 3.7 CRC ↔ LFSR (cross-link to Pack 06)

The bitwise loop is a **Galois LFSR**: a shift register where, when the bit shifted out is 1, the polynomial's tap positions are XOR-ed in. Hardware: a row of flip-flops with XOR gates at the taps of `g`; data bits enter and the register ends up holding the remainder. Feed *zeros* instead of data and the register just walks through **powers of x mod g** — our table `001 → 010 → 100 → 011 → 110 → 111 → 101 → 001` is exactly the state sequence of the 3-bit LFSR with `g = x³ + x + 1`, period 7 = 2³ − 1 (a primitive polynomial). Pack 06 uses the same structure, in the Fibonacci form, to whiten (scramble) radio data.

And our CRC-3 code `{1101001, ...}` with `g = 1011` **is** the (7,4) cyclic Hamming code of Pack 05: the remainder of a single-bit error `xⁱ` is the unique syndrome naming position *i*.

### 3.8 The Internet checksum (RFC 1071)

**Concrete first (RFC 1071 §3 example).** Bytes `00 01 f2 03 f4 f5 f6 f7` → 16-bit big-endian words `0001 f203 f4f5 f6f7`.
```
 0001 + f203 = f204
 f204 + f4f5 = 1e6f9      (17 bits: a carry out of bit 15)
 1e6f9 + f6f7 = 2ddf0     (18 bits)
 fold carries back in:  ddf0 + 2 = ddf2        <-- the "sum"
 checksum = ~ddf2 = 220d                       <-- stored in the header
```
Receiver: sum all words **including** the checksum: `ddf2 + 220d = ffff` ⇒ OK (all-ones = "zero" in one's complement).

**Why the carries are added back ("end-around carry").** `0x10000` (carry out of bit 15) should be *worth* what? In arithmetic mod `0xFFFF` (65535), `0x10000 = 65536 ≡ 1`. So adding the carry back as `+1` is exactly "reduce modulo 65535". One's-complement addition **is** addition mod 65535.

**Byte-order independence (derive).** A word is `hi·256 + lo`. Byte-swapped: `lo·256 + hi`. Multiply the original by 256: `hi·65536 + lo·256 ≡ hi·1 + lo·256 (mod 65535)` — equal to the swapped word. So **swapping the bytes of a word = multiplying it by 256 mod 65535**. Sum of swapped words = 256 × (sum of words) mod 65535 = **swapped sum**. Hence the same code produces a correct checksum on big- and little-endian machines as long as you consistently treat the buffer as 16-bit words (RFC 1071 §2(B)). Check: `12 34 56 78` → `0x68AC`; swapped bytes `34 12 78 56` → `0xAC68`, which is `0x68AC` byte-swapped.

**IPv4 header by hand (Pack 03 example, checksum field zeroed):** words `4500 0073 0000 4000 4011 0000 c0a8 0001 c0a8 00c7`:
`4500+0073 = 4573; +4000 = 8573; +4011 = c584; +c0a8 = 1862c → 862d; +0001 = 862e; +c0a8 = 146d6 → 46d7; +00c7 = 479e`. Checksum `~479e = b861` ✓. Verify: put `b861` in and re-sum → `ffff`.

**Odd length:** pad one zero **byte after** the last byte (so the lone byte is the *high* byte of the last word): `01 02 03` → words `0102`, `0300` → `0402`, checksum `fbfd`.

**Properties & weaknesses.** Detects any single corrupted word (one word changes ⇒ sum changes). Cannot detect: **reordering of 16-bit words** (addition commutes: `0001 0002` vs `0002 0001`), insertion of zero words, or two compensating changes. Fine for headers (cheap, software-friendly, 1981-era hardware) because the link layer's CRC protects the frame.

**UDP detail:** a computed checksum of `0x0000` is transmitted as `0xFFFF` (since `0x0000` in the field means "no checksum" in IPv4). In one's complement, 0x0000 and 0xFFFF are both "zero" so the substitution is safe (RFC 768). The UDP checksum covers a pseudo-header (source IP, destination IP, zero, protocol, UDP length) + UDP header + data.

**Incremental update (rewriting gateway):** when you change one 16-bit word `m → m'`, `HC' = ~(~HC + ~m + m')` in one's-complement arithmetic (RFC 1624 — VERIFY the exact formula, it corrects an edge case of RFC 1141).

> **Misconceptions box 3.8**
> 1. *"The Internet checksum is a CRC."* No — an additive checksum; much weaker (no burst/ordering guarantees).
> 2. *"I must `ntohs` every word first."* Not needed (box above), as long as the fold/complement are done consistently.
> 3. *"The checksum is the sum."* It's the **complement** of the folded sum.
> 4. *"Odd length: pad at the front."* Pad a zero byte at the **end** (the odd byte is the high half of the last word).
> 5. *"A zero UDP checksum is just `0`."* Transmit `0xFFFF` if the computation yields 0.

---

## §4 Fully worked example

**A. CRC-3 (g = x³ + x + 1 = 1011), message `1010`.** Append 000 → `1010000`:
```
1010000
^1011      (leading 1 at bit 6)   -> 0001000
   ^1011   (next 1 at bit 3)      -> 0000011       remainder = 011
```
Codeword `1010 011`. Check: `1010011`: bit 6 → `0001011`; bit 3 → `0000000` ✓.
Flip bit 4 → `1000011`: `^1011000 → 0011011`, `^0010110 → 0001101`, `^0001011 → 0000110` = `110` = `x⁴ mod g` ✓ — the syndrome of a single-bit error at position 4 is `x⁴ mod g`.

**B. CRC-16/XMODEM of byte `0x01` via the register:** `0x0100 → 0x0200 → 0x0400 → 0x0800 → 0x1000 → 0x2000 → 0x4000 → 0x8000 → (top bit out, XOR 1021) 0x1021`.

**C. Table method on `{01 02}`** (XMODEM): `0x1021`, then `0x2100 ^ table[0x12] (0x3273) = 0x1373`; compare the bitwise `crc_generic` result `0x1373` (computed independently).

**D. Check value.** `crc_generic(kCrc16CcittFalse, "123456789")` must equal `0x29B1`; `kCrc32` → `0xCBF43926`.

**E. IPv4 header checksum** computed in §3.8: `b861`.

---

## §5 Questions

Each question has a **type**, a **context line**, and an answer in `answers.md`.

**Q1 — Explain.** *Context: reasoning about CRC hardware.* Show with a 4-row truth table why subtraction in GF(2) equals XOR, and say why that means long division never needs a borrow.

**Q2 — Calculate.** *Context: toy link protecting 4-bit control words.* With `g = x³ + x + 1` (`1011`), compute the 3-bit CRC and the 7-bit codeword for messages (a) `1010` (b) `0110` (c) `1111`. Check each codeword is divisible. Verify (c) using linearity from (a) and `0101` (CRC `100`).

**Q3 — Predict.** *Context: single-bit error on the toy link.* For codeword `1010011` flip each of the 7 bits in turn. What is the syndrome (remainder) for each? Which bit position gives `110`? Why are all seven syndromes different?

**Q4 — Calculate.** *Context: CRC-16/XMODEM in a bootloader.* Using `table[1] = 0x1021` and `table[2] = 0x2042`, find `table[3]` by linearity. Then find the XMODEM CRC of the single bytes `0x01`, `0x02`, `0x03`; and of `{00 01}`. What do you notice?

**Q5 — Explain.** *Context: porting a CRC from a spec to code.* Why do CRC specs have "reflect in / reflect out"? Compute `reflect(0x1021,16)` by hand. Name two protocols/technologies where bytes are sent LSB-first.

**Q6 — Trace.** *Context: CRC register by hand.* Trace the 8 bit-steps of CRC-16 (poly 0x1021, init 0) for the single byte `0x01`, and then the table-driven computation for `{01 02}`.

**Q7 — Explain.** *Context: interview — "why is the table version the same?"* Prove it using linearity and the "low byte can't reach bit 15 in 8 steps" argument.

**Q8 — Find the bug.** *Context: embedded firmware review.* Find all bugs (relative to CCITT-FALSE):
```cpp
uint16_t crc16(const uint8_t* p, size_t n) {
  uint16_t crc = 0;
  for (size_t i = 0; i < n; ++i) {
    crc ^= p[i];
    for (int k = 0; k < 8; ++k)
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}
```
Also: what would a correct CRC-32 omit if it returned `crc` instead of `~crc`?

**Q9 — Calculate.** *Context: IPv4 header in a gateway.* Compute the Internet checksum of `45 00 00 73 00 00 40 00 40 11 00 00 c0 a8 00 01 c0 a8 00 c7` by hand, then verify the filled-in header sums to `ffff`.

**Q10 — Explain.** *Context: why end-around carry.* Show that one's-complement addition equals addition mod 65535. Compute `ffff + 0001` both ways.

**Q11 — Predict.** *Context: big-endian vs little-endian CPU checksum code.* Sum of words for bytes `12 34 56 78`; sum for `34 12 78 56`; relate them. Why does the checksum code not need `htons`?

**Q12 — Find the bug.** *Context: UDP send path in a gateway.* Bugs in:
```cpp
uint16_t udp_checksum(const uint8_t* seg, size_t len) {   // seg = UDP header + data, checksum field NOT zeroed
  uint32_t sum = 0;
  for (size_t i = 0; i < len; i += 2) sum += (seg[i] << 8) | seg[i+1];
  sum = (sum & 0xFFFF) + (sum >> 16);
  return ~sum;
}
```
(Find ≥ 5.)

**Q13 — Implement.** *Context: gateway utility library.* Implement every stub in `code/include/crcsum.h`; `student.*` tests green and sanitizer-clean.

**Q14 — Connect.** *Context: whole module.* (a) Why is the CRC register an LFSR (Pack 06)? (b) How does this pack's CRC-3 relate to Hamming (7,4) (Pack 05)? (c) In a gateway that decrements TTL, which checksum must be updated and how?

**Q15 — Interview.** *Context: phone screen.* "Implement CRC-32 and explain why the table-driven version works. What do init and final XOR do?"

**Q16 — Interview.** *Context: design review.* "CRC vs checksum vs cryptographic hash vs MAC: what does each detect? Would you use a CRC to authenticate a firmware update?"

---

## §6 Self-assessment

- [ ] I can do GF(2) long division by hand and explain why it has no borrows.
- [ ] I can prove codeword ≡ 0 (mod g) and explain what error patterns are caught.
- [ ] I can describe each of poly/init/refin/refout/xorout and give a failure demo for init and xorout.
- [ ] I can derive the table-driven algorithm (linearity + "low byte can't affect 8 steps").
- [ ] I can reproduce the check value of `"123456789"` for at least CCITT-FALSE and CRC-32.
- [ ] I can compute the Internet checksum by hand and explain end-around carry and byte-order independence.
- [ ] I know the weaknesses of the Internet checksum (reordering) and the UDP zero rule.
- [ ] `ctest`: all `solution.*` and (after Q13) `student.*` tests pass, sanitizers silent.

**Rapid fire (key in `answers.md`):** (1) `0x1021` means which polynomial? (2) Check value CCITT-FALSE? (3) Why does init=0 miss leading zeros? (4) `ffff + 0001` in one's complement? (5) Which checksum cannot see word reordering?

**VERIFY list:** which polynomial/init/xorout P25 (TIA-102.BAAA/BAAB) and DMR (ETSI TS 102 361-x) use per field; RevEng/Rocksoft catalogue values (https://reveng.sourceforge.io/crc-catalogue/); Rocksoft model paper (Ross Williams, "A Painless Guide to CRC Error Detection Algorithms"); RFC 1071 (§2(B), §3 example), RFC 1624 incremental-update formula, RFC 768 (zero checksum rule), RFC 1141 (obsolete); CRC-32 residue constants.
