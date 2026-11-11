# Pack 01 - Answers

Format per question: **Reasoning -> Answer -> Common wrong answers** (why tempting / why wrong). Try first.

---

## Prerequisites

**P1.** A *declaration* tells the compiler a name and its type/signature (`int twice(int);`). A *definition* provides the body/storage. Headers normally hold declarations (plus `inline`/template/`constexpr` definitions). *Wrong:* "they are the same" - tempting because for a local `int x;` they coincide; wrong because functions and globals are declared in many TUs but defined once (ODR).

**P2.** `argc == 1`: `argv[0]` is the program name. *Wrong:* 0 - tempting because "no arguments"; wrong because the program name counts.

**P3.** The preprocessor replaces the line with the full text of the header (recursively). *Wrong:* "links the library" - tempting because of the word "include"; wrong because linking is a later stage and a standard header may contribute only declarations.

**P4.** 0..255 (8 bits unsigned, exactly, on every platform where the type exists). Arithmetic on it is done as `int` - pack 02.

---

## 5.1 Explain why

**Q1.** *Reasoning:* compile turns one translation unit into one object file; link joins object files. If they were fused, every edit recompiles all 400 files. Separate steps let `make`/`ninja` compare timestamps and recompile only the `.cpp` files whose inputs changed, then relink (cheap). *Answer:* separate compilation = incremental builds. Missing the defining object gives `undefined reference to 'f(...)'` from the **linker**.
*Wrong answers:*
- "Linking is optional for executables" - tempting because `-c` is familiar; wrong, an executable needs resolved symbols.
- "The compiler reports the undefined reference" - tempting because the message appears in the same terminal; wrong, the compiler only needs the declaration.

**Q2.** *Reasoning:* global `include_directories()` adds the path to *every* target in that directory (and subdirectories). Then firmware inherits test include paths; a test-only `config.hpp` can shadow the real one and the firmware compiles against test settings, silently. `PRIVATE` on a target states exactly which target needs the path. *Answer:* scoping makes dependencies explicit and prevents include-path leakage / shadowing.
*Wrong:* "It is faster" - tempting because CMake is perceived as about speed; wrong, it is about correctness of dependencies. "`PRIVATE` hides symbols in the binary" - wrong, it is a CMake propagation setting.

**Q3.** *Reasoning:* ASan keeps shadow memory for addresses and replaces `malloc`/`free`; TSan keeps different shadow state (per-memory-location access history and vector clocks) at its own address layout, and instruments each access differently. Two runtimes cannot both own the shadow region and allocator interposition. Run them as separate CI jobs. Radios: overhead (CPU ~2x or more, memory several x - VERIFY exact for your toolchain) breaks real-time deadlines and often exceeds RAM, and a bare-metal/RTOS target may not have the sanitizer runtime at all. *Answer:* mutually exclusive runtimes; use host builds of the same code for sanitization.
*Wrong:* "TSan is just an extension of ASan" - tempting since the flags look alike; wrong, different tools. "Radios are too slow to compile it" - wrong reason; compile is on a host, the issue is runtime cost and support.

---

## 5.2 Calculate

**Q4.** *Reasoning:* `0x602000000010 >> 3`: `0x6020` = 24608; /8 = 3076 = `0xC04`, and the low part `0x10 >> 3 = 2`, giving `0x0C0400000002`. Add `0x7fff8000`: low 32 bits `0x00000002 + 0x7FFF8000 = 0x7FFF8002`, upper bits `0x0C04` unchanged. *Answer:* shadow address `0x0C047FFF8002`; shadow byte `0x00` (all 8 valid). 1 MiB = 2^20 bytes -> 2^20/8 = 2^17 = 131072 bytes = 128 KiB.
*Wrong:* "`addr >> 8`" - tempting because "8" appears; wrong: 8 *bytes* means shift by 3 bits. "Shadow byte is 8" - tempting for "8 bytes"; wrong: 0 means all accessible. "1 MiB / 8 = 8 KiB" - arithmetic slip (1 MiB is 1,048,576).

**Q5.** *Reasoning:* size = N + 3. *Answer:* 3, 6, 258. For N=3: 3/6 = **50%** overhead. *Wrong:* 2 bytes overhead (forgetting the checksum), or `N+1` (forgetting marker) - the frame has three fixed bytes: start, length, checksum.

**Q6.** *Reasoning:* `0x12 = 0001 0010`, `0x34 = 0011 0100` -> XOR `0010 0110 = 0x26`; `0x56 = 0101 0110` -> XOR `0111 0000 = 0x70`. `{01,02}` -> `0x03`; `{02,01}` -> `0x03`. *Answer:* `0x70`; both orderings give `0x03`. XOR cannot detect reordering (it is commutative), nor two equal bit flips in the same bit position of different bytes. This is why real air interfaces use CRCs (position-sensitive polynomial division) - exact P25/DMR/TETRA CRC polynomials: VERIFY in the standards.
*Wrong:* "0x70 means data is intact if it matches" - tempting because matching feels like proof; wrong, collisions are easy to construct.

---

## 5.3 Predict

**Q7.** *Reasoning:* a `.cpp` is recompiled if it directly or transitively includes a changed header. `test_checksum.cpp` includes `checksum.hpp` (direct); `test_frame.cpp` includes `frame.hpp` which includes `checksum.hpp` (transitive); `test_toolchain_info.cpp` does not. *Answer:* two recompiled, one not; relink needed (the single executable changed). *Wrong:* "only test_checksum.cpp" - misses the transitive include; "all three" - assumes any header change rebuilds everything.

**Q8.** *Reasoning:* buffer has indices 0..7. `n=7`: loop `i<=7` writes 0..7, all legal; prints `last=7`, exit 0, no leak (deleted). `n=8`: at `i=8` writes `buf[8]` - one past the end - lands in the right redzone: ASan aborts with `heap-buffer-overflow ... WRITE of size 1 ... 0 bytes after 8-byte region`. `n=9`: still fails at the first illegal write, `i=8`; the report is the same (the `buf[9]` write is never reached). *Wrong:* "n=9 reports 1 byte after" - tempting because 9 is "one more"; wrong because the first violation stops the program.

**Q9.** *Reasoning:* (a) UBSan with no-recover prints `runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'` and aborts - nonzero exit. (b) at `-O0` you will most likely see `y=-2147483648` because the CPU's `add` wraps in two's complement. (c) at `-O2` the compiler is allowed to assume no overflow; here the result is probably the same wrap, but it may differ in other code (e.g. deleting `x+1 < x` checks). *Answer:* only (a)'s *detection* is deterministic; **nothing** about (b)/(c)'s output is guaranteed by the standard.
*Wrong:* "-2147483648 is guaranteed" - tempting because hardware wraps; wrong because the compiler, not the hardware, is the one the standard constrains (pack 05).

---

## 5.4 Trace by hand

**Q10.** Top of each iteration:

| iter | `n` points to | `n->value` | `total` before |
|------|---------------|------------|----------------|
| 1 | `&a` | 3 | 0 |
| 2 | `&b` | 5 | 3 |
| 3 | `&c` | 7 | 8 |
| 4 | `nullptr` | *fault* | 15 |

After the 3rd body `total = 15` and `n = c.next = nullptr`; evaluating `n->value` in the `while` condition dereferences null -> SIGSEGV. `bt`: `#0 sum_list (n=0x0) at ...:14`, `#1 main () at ...:25`. Faulting expression: `n->value` (condition). *Wrong:* "crash at `n = n->next`" - tempting since that line makes `n` null; wrong, assigning null is legal; the *use* faults on the next condition check.

**Q11.** (a) `out = 7E 03 01 02 04 07` (checksum `1^2^4 = 7`). (b) Checks in order: `in_len >= 3` (5, ok) -> `in[0]==0x7E` ok -> `n = in[1] = 2` -> `in_len == n+3 = 5` ok -> `out_cap >= n` -> checksum `0x0A ^ 0x0B = 0x01 == in[4]` ok -> copy -> return **2**. (c) same until checksum: `0x01 != 0x02` -> `kDecodeError`. *Wrong:* reading `in[1]` before the length check - harmless here but a crash/UB on a 1-byte input.

---

## 5.5 Find the bug

**Q12.** *Reasoning:* the frame needs `n+3` bytes but the guard checks `n+2`. With `cap == n+2` the guard passes and `out[2+n]` is index `cap` - one past the end. *Answer:* off-by-one. ASan (heap buffer): `heap-buffer-overflow ... WRITE of size 1 ... 0 bytes after N+2-byte region`; stack buffer: `stack-buffer-overflow`. The test `TooSmallOutputReturnsZeroAndWritesNothing` (cap 5 for n=3) catches it. Fix: `cap < n + 3`. *Wrong:* "`n > 255` should be `>=`" - tempting for off-by-one pattern spotting; wrong, 255 fits in one length byte. "`return n+3` is wrong" - it is correct.

**Q13.** *Reasoning:* `GTest::gtest` is the library without `main`; `GTest::gtest_main` adds a `main()` that runs all tests. *Answer:* `gtest_main`. `undefined reference to testing::...` arises from not linking gtest at all, from `include(GoogleTest)`/FetchContent not being made available before the target, or (with raw `-l` flags) wrong link order; mixing sanitizer flags only on some objects can also give link errors. *Wrong:* "add `#include <gtest/gtest.h>`" - tempting since gtest is named; wrong, header is not the problem.

**Q14.** *Reasoning:* `ASSERT_*` expands to roughly `if (!cond) { record failure; return; }` - a bare `return;` is illegal in a function returning `int`. *Answer fixes:* (1) make the helper `void` and return the value through an out-parameter, then call it from the test with `ASSERT_NO_FATAL_FAILURE(helper(...))`; (2) use `EXPECT_NE` and return a value normally (execution continues); (3) put the check in the test body. *Wrong:* "return 0 after the macro" - the macro already contains `return;`, the problem is the type.

---

## 5.6 Implement

Reference: `code/solutions/es_toolchain/`. Key points a good answer shows:

**Q15.** Loop with `acc = static_cast<std::uint8_t>(acc ^ data[i]);`. `acc ^ data[i]` has type `int` (both operands promote - pack 02); the compound `acc ^= data[i]` also computes in `int` then narrows; some compilers warn under `-Wconversion`. The empty input `(nullptr, 0)` returns 0 because the loop body never runs, so no dereference. *Wrong:* returning on `data == nullptr` as an error - changes the contract (the test requires 0).

**Q16.** Check `n <= 255` and `cap >= n+3` *before* any write; write marker, length, payload, checksum; return `n+3`. Common wrong answers: `<=` instead of `<` (rejects exact fit; `Max255PayloadFitsExactly` fails); writing the marker before the size check (violates "writes nothing"); `std::uint8_t len = n` without range check first (silent truncation at 256 -> length 0).

**Q17.** Order: `in_len >= 3` -> marker -> read `n=in[1]` -> `in_len == n+3` -> `out_cap >= n` -> checksum -> copy. Wrong: checking checksum before length (reads past end); `in_len >= n+3` instead of `==` (accepts trailing garbage - sometimes legitimate in a stream, but here the frame function receives exactly one frame).

**Q18.** See `solutions/es_toolchain/toolchain_info.hpp`: `_MSVC_LANG` for MSVC; `__SANITIZE_ADDRESS__` (GCC/MSVC) or `__has_feature(address_sanitizer)` (clang); `NDEBUG` for assertions. Wrong: `#ifdef __SANITIZE_ADDRESS__` only - misses clang. `__cplusplus >= 202002L` only - gives 17 on MSVC even when compiling as C++20.

---

## 5.7 Connect

**Q19.** `int`. Integer promotion: operands narrower than `int` are converted to `int` before the operation (why: `int` is the machine's natural arithmetic width, so arithmetic is done there; pack 02 section 3.1). The cast is explicit narrowing back to `uint8_t`; it silences `-Wconversion`, documents intent, and never changes the value because both inputs are in `[0,255]` and bitwise XOR of two 8-bit values has no bits above bit 7. *Wrong:* "XOR on uint8_t is 8-bit hardware XOR" - tempting from the C++ type name; wrong, the language model is `int`.

**Q20.** (a) Crash after a large frame -> ASan first (heap/stack overflow; the report names the line), gdb on the core for context. (b) Wrong output only in Release -> UBSan first (UB that the optimiser exploited), then diff behaviour with `NDEBUG` (assert with side effects, or code that relied on assert to guard bounds). (c) Corrupted counter under load -> TSan (data race), not ASan. *Wrong:* "always ASan" - tempting because it is the famous one; wrong, it does not see data races or most UB.

---

## 5.8 Interview-style

**Q21.** Model answer: (1) CMake already generated Ninja/Make files; the build tool reads the dependency graph. (2) For each changed `.cpp`: preprocess (headers pasted) -> compile -> `.o` (object file with machine code and unresolved symbols). GoogleTest was fetched earlier and built to static libs. (3) Link: the linker resolves symbols across `.o` files and libs (`gtest_main`) and writes the executable. (4) With sanitizers, the instrumented compile and the sanitizer runtime at link. (5) `ctest` then runs it. Weak answers skip the object-file stage or call the whole thing "compiling".

**Q22.** Need: the core file, the *exact* binary (or its debug symbols) with matching build ID, and the sources at that revision. Steps: `gdb ./binary core`; `bt` (all threads: `thread apply all bt`); `frame N`, `info locals`, inspect pointers (null? dangling?); reason backwards to who produced the bad state; write a failing test reproducing it; fix; run sanitizers. For future: ship symbols separately (not stripped from CI archive), log version/build ID. *Wrong:* "add printfs and wait for it to happen again" - slow and changes timing; "rebuild and debug the new binary" - addresses differ, the core is useless.

**Q23.** Separate the *pure transformation* (bytes in -> bytes out; no I/O) from the radio driver. Test the encoder with hand-computed vectors from the standard and with round-trip/negative tests; use synthetic frames or recorded *test* vectors released by the standard bodies/vendors; mock the radio interface behind an abstract class; run on the host under sanitizers. Never tune a test rig to live channels - public-safety traffic is sensitive and licensed. *Wrong:* "capture real traffic off the air" - exactly what the safety rule forbids.

**Q24.** Say: sanitizers only prove the *executed* paths are free of the *detectable classes* of error. Gaps: untested paths (add fuzzing / more negative tests), logic errors, data races (need TSan), uninitialised reads (MSan, clang), UB not covered by the enabled checks, bugs only in the cross-compiled target (e.g. endianness, alignment). Add coverage measurement, fuzz the decoder, and run TSan in a separate job. *Wrong:* "Agree - clean is clean" - the classic over-trust.
