# Pack 05 cheat sheet - every line points back to the worksheet

**Undefined behaviour**
- UB = no requirements at all; compilers optimise assuming it never happens - §3.1
- signed overflow UB (`INT_MAX + 1`), unsigned wraps mod 2^N - §3.1 (Q1)
- `x + 1 < x` is "always false" at `-O2` (lab A) - §3.1, Q8
- check BEFORE the operation: `a > MAX - b` for `b > 0`, `a < MIN - b` for `b < 0` - §3.1, Q16
- `abs(INT_MIN)` is UB -> `checked_abs` - §3.1
- `-fwrapv` defines signed wrap (non-portable stop-gap); UBSan finds UB in tests - §3.1
- other UB: shift >= width, aliasing, misalignment, OOB/dangling, data race, uninit read - §3.1

**Shifts**
- shift count must be `0 <= n < width of PROMOTED left operand` - §3.2
- barrel shifter = 5 stages (1,2,4,8,16) -> x86 masks count to 5 bits: `1u<<32 == 1` - §3.2 (Q2)
- AArch32 register shift gives 0 for count >= 32 (VERIFY) -> different CPUs differ -> UB - §3.2 (Q9)
- `mask(32)`: `(1u<<32)-1` wrong -> special-case width 32/0 - §3.2, Q14
- safe rotate: `(v << (n&31)) | (v >> (-n & 31))`; `std::rotl` in C++20 - §3.2, Q11
- widen BEFORE shifting (`uint32_t{b} << 24`) - §3.2, Q21

**Aliasing / punning**
- access an object only via its own type (or `char`/`unsigned char`/`std::byte`) - §3.3
- compiler assumes unrelated-type pointers don't overlap (lab C: 1 vs 1073741824) - §3.3, Q10
- legal punning: `memcpy` or `std::bit_cast`; not `reinterpret_cast`, not union (C++) - §3.3, Q27
- alignment is a separate rule from aliasing - §3.3, §3.4

**Layout**
- each field at `align_up(prev_end, alignof(field))` - §3.4
- `sizeof = align_up(end, max alignment)` so arrays stay aligned - §3.4 (Q4, Q5)
- `align_up(x,a) = (x + a - 1) & ~(a - 1)`, `a` must be a power of two - §3.4, Q6
- `is_pow2(x) = x != 0 && (x & (x-1)) == 0` - §3.4
- order fields largest-first to cut padding - §3.4
- never `memcpy`/cast a struct as a wire format (padding leak, endianness, ABI) - §3.4, Q15
- packed structs fix padding, not endianness/alignment hazards; bit-fields are implementation-defined - §3.4

**Bits and endianness**
- `v & (v-1)` clears the lowest set bit -> Kernighan popcount - §3.5
- Hamming distance = `popcount(a ^ b)`; parity by xor-folding - §3.5, Q7
- `0x11223344` LE bytes `44 33 22 11`, BE `11 22 33 44`; network order = BE - §3.5
- portable BE read: shifts from bytes (widen first) or `memcpy` + conditional byteswap - §3.5, Q25
- `1.0f = 0x3F800000`, `-2.0f = 0xC0000000`, `0.5f = 0x3F000000` - §3.5, Q7
- `extract = (v >> lo) & mask(w)`, `insert = (v & ~(mask<<lo)) | ((f & mask) << lo)` - §3.5
- `index & (N-1)` == `index % N` iff N is a power of two - §3.4, Q23
