# Pack 01 - Toolchain: CMake, GoogleTest, sanitizers, gdb

**Time:** 2-4 hours. **Code:** `code/` (stubs in `include/`, reference in `solutions/`, buggy programs in `labs/`).
**Answers:** `answers.md` (attempt first). **Recall sheet:** `cheatsheet.md`.

```bash
cd code
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```

At the start almost every test FAILS. That is correct. By the end they pass, and they pass *under sanitizers*.

---

## 1. Why this matters

Etherstack ships protocol stacks (P25, DMR, TETRA) that run on embedded radios, on Linux gateways bridging to 4G/5G MCPTT servers, and in test rigs. Three facts shape everything:

1. **One codebase, several targets.** The same encoder is compiled for an ARM radio with a cross-compiler and for x86 on a developer laptop. A build system that cannot express "same sources, different toolchain" becomes a pile of shell scripts nobody dares touch.
2. **Protocol code must be bit-exact.** One wrong byte in a frame and the far-end radio silently drops the call. The only practical defence is automated tests that compare against hand-computed bytes.
3. **Memory bugs surface weeks later.** A one-byte buffer overflow in a parser may corrupt the *next* allocation and crash a gateway at 3 a.m. Sanitizers turn "weird crash next Tuesday" into "red test now, with the exact line".

An interviewer who sees you reach for `cmake`, a failing GoogleTest, and an ASan report - instead of `printf` and guessing - learns you can work in a real team.

> **Safety:** every byte in this pack is synthetic or hand-computed. Never feed any tool, test or lab live public-safety traffic.

---

## 2. Prerequisites check

This is the first pack; the prerequisites are basic literacy. If any answer is not obvious, read the C++ cheat sheet in this repo first: `../../../../../documentation/c++/C++_CHEAT_SHEET.md` (path from this folder), and do pack 02 in parallel.

- **P1.** What is the difference between *declaring* and *defining* a function? Which one does a header normally contain? *(Answer key: `answers.md`, P1.)*
- **P2.** In `int main(int argc, char** argv)`, what is `argc` when you run `./prog` with no arguments? *(P2)*
- **P3.** What does `#include <cstdint>` literally do to the text of your file? *(P3)*
- **P4.** What is the value range of `std::uint8_t`? If you are not sure about arithmetic on it, that is pack 02 (`../02-types-basics/`). *(P4)*

---

## 3. Concept build-up

### 3.1 From `.cpp` to a running program

**Concrete first.** Two files:

```cpp
// util.cpp
int twice(int x) { return 2 * x; }

// main.cpp
int twice(int x);                 // declaration: "trust me, it exists elsewhere"
int main() { return twice(21); }  // exit code 42
```

```bash
g++ -c main.cpp -o main.o     # compile: main.o contains a *call to an unresolved symbol* twice(int)
g++ -c util.cpp -o util.o     # compile: util.o *defines* twice(int)
g++ main.o util.o -o prog     # link: connects the call to the definition
./prog; echo $?               # 42
```

Four stages: **preprocess** (paste `#include`s, expand macros) -> **compile** (C++ text to machine code in an object file, one *translation unit* at a time) -> *(assemble)* -> **link** (join object files, resolve symbols).

**Derive why compile and link are separate.** A gateway has hundreds of `.cpp` files. If compile and link were one step, editing one line would recompile everything. Splitting them lets the build system recompile only the changed `.o` and relink.

**Failure if ignored.** Link only `main.o`:

```
undefined reference to `twice(int)'
```

That message is from the *linker*; the compiler was happy because the declaration was enough. Likewise, defining a non-`inline` function in a header that two `.cpp` files include gives `multiple definition of ...` at link time - the One Definition Rule (ODR). That is why the headers in this pack use `inline` for functions defined in headers.

**Warnings as bug reports.** `-Wall -Wextra` ask the compiler to point at suspicious code (unused parameter, signed/unsigned compare). They are free, so every pack uses them. We do not use `-Werror` in a learning pack because the stubs deliberately have unused parameters; a production CI would.

> **Common misconceptions**
> - *"A header is compiled."* No: it is pasted into each `.cpp` that includes it. That is why heavy headers slow builds.
> - *"The compiler checks that the function exists."* It only checks the declaration; the linker checks existence.
> - *"`inline` means fast."* Its binding meaning is "multiple identical definitions are allowed (ODR exception)".

### 3.2 CMake: describe *what*, let a generator decide *how*

**Concrete first.** Smallest useful `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(demo LANGUAGES CXX)
add_executable(prog main.cpp util.cpp)
target_compile_features(prog PRIVATE cxx_std_20)
```

```bash
cmake -S . -B build        # CONFIGURE+GENERATE: reads CMakeLists, writes Makefiles/Ninja files into build/
cmake --build build        # BUILD: runs the generated build tool (make, ninja, msbuild...)
```

**Derive why it exists.** Raw Makefiles hard-code the compiler, flags and OS. A radio team needs *the same source description* to produce: a Linux x86 gateway binary, an ARM cross-build (a *toolchain file* swaps the compiler), and a test build with sanitizers. CMake separates "what are my targets and their requirements" from "which compiler and generator", so only the second changes per target.

**Targets with usage requirements.** The modern idiom is to attach properties to a *target*:

```cmake
target_include_directories(pack01_tests PRIVATE ${ES_HEADER_DIR})
target_compile_options(pack01_tests PRIVATE -Wall -Wextra)
target_link_libraries(pack01_tests PRIVATE GTest::gtest_main)
```

`PRIVATE` = needed to build this target only; `PUBLIC` = also needed by anything that links it; `INTERFACE` = only by consumers. **Why not one global `include_directories()`?** Because it applies to every target in the directory, so a include path meant for the test leaks into the firmware target and one day a test-only header shadows a real one. Per-target scoping makes dependencies explicit.

**Out-of-source builds** (`-B build`): the source tree stays clean; `rm -rf build` is a safe full reset. **Build type:** `Debug` = `-g -O0` (debugger-friendly), `Release` = `-O3 -DNDEBUG`. `NDEBUG` disables `assert()` - so a Release build can behave differently from the Debug one you tested (see Q20).

**Pulling in GoogleTest without installing it** - `FetchContent`: CMake downloads a pinned tag at configure time into `build/_deps`. Why pinned: reproducibility. A moving `main` branch could break your CI on a day you changed nothing. This is exactly what `code/CMakeLists.txt` does, and `USE_SOLUTIONS` just switches which header directory is on the include path - the tests do not know whether they test a stub or a solution.

**Failure if ignored.** Forget `target_link_libraries(... GTest::gtest_main)`: you get `undefined reference to testing::Test::Test()` or `undefined reference to main` (Q13).

> **Common misconceptions**
> - *"cmake builds my code."* It *generates* build files; the generator's tool builds.
> - *"Editing CMakeLists.txt needs manual re-run."* The generated build reruns CMake automatically when CMakeLists changes.
> - *"`PRIVATE` is about security."* It is about *who sees the property*.
> - *"Changing `-DCMAKE_BUILD_TYPE` after the first configure always works."* The value is cached; use a fresh build dir per configuration.

### 3.3 GoogleTest: executable specifications

**Concrete first.**

```cpp
TEST(XorChecksum, HandComputedVector) {            // TEST(SuiteName, TestName)
    const std::uint8_t b[] = {0x12, 0x34, 0x56};
    EXPECT_EQ(xor_checksum(b, 3), 0x70);           // 0x12^0x34=0x26 ; 0x26^0x56=0x70
}
```

**Derive the vocabulary.** A test = "given this input, expect that output". For protocol code the expected output is computed *by hand from the standard*, not by running the code - otherwise you only test that the code agrees with itself ("golden vector" testing). Write the test first and watch it fail ("red") - that proves the test *can* fail. Then implement ("green").

`EXPECT_*` records a failure and **continues**; `ASSERT_*` records and **returns from the current function**. **Why two?** If `ASSERT_EQ(frame_encode(...), 6u)` fails, indexing `out[5]` next is meaningless or unsafe, so stop. If you are checking five independent fields, `EXPECT` shows all five failures at once. Consequence: `ASSERT_*` only compiles in functions returning `void` (Q14).

Other pieces used here: *fixtures* (`TEST_F` with shared `SetUp`), `ctest` (runs every discovered test as a process, with a timeout so a deadlocked test cannot hang CI), `--gtest_filter=Suite.Name` to run one test.

**Failure if ignored.** A test that never fails (e.g. compares a variable to itself) gives false confidence. Always see red first.

> **Common misconceptions**
> - *"More asserts = better."* One behaviour per test; a failing name should tell you what broke.
> - *"Use ASSERT everywhere to be safe."* It hides later failures in the same test.
> - *"Tests only check the happy path."* Protocol code is mostly rejecting bad input (see `FrameDecode.Rejects*`).

### 3.4 Sanitizers: making silent bugs loud

**Concrete first.** `labs/lab_heap_overflow.cpp` writes `buf[8]` into an 8-byte array. Without tooling it often "works". With `-fsanitize=address`:

```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000018 ...
WRITE of size 1 at 0x602000000018 thread T0
    #0 0x... in main labs/lab_heap_overflow.cpp:11
0x602000000018 is located 0 bytes after 8-byte region [0x602000000010,0x602000000018)
allocated by thread T0 here:
    #0 operator new[](unsigned long)
    #1 main labs/lab_heap_overflow.cpp:9
```

(abridged; addresses vary run to run.) Read it bottom-up: *where it was allocated*, *how far past the region*, *which line wrote*.

**Derive how ASan can know.** Idea: keep a second memory area (**shadow memory**) that records, for every 8 bytes of program memory, whether those bytes are legal to touch. 1 shadow byte per 8 app bytes means the shadow costs 1/8 of the memory you track. Shadow byte meaning: `0` = all 8 accessible; `1..7` = only the first k bytes accessible; a negative value = poisoned (redzone, freed, etc.; the legend ASan prints names each code - read it, do not memorise).

The compiler rewrites every load/store to first do (simplified):

```
shadow_addr = (addr >> 3) + OFFSET          // OFFSET is a platform constant (0x7fff8000 on x86-64 Linux)
s = *(int8_t*)shadow_addr
if (s != 0 && (addr & 7) + access_size - 1 >= s)  report_error()
```

The allocator puts **redzones** (poisoned padding) around every heap block and poisons freed memory, so overflow and use-after-free both hit poisoned shadow. *Taken on trust:* the exact offsets/redzone sizes and shadow byte codes, which vary by platform and version; see the ASan legend in your own output.

**UBSan** (`-fsanitize=undefined`) inserts checks for operations the standard calls *undefined behaviour*: signed overflow, shift by too many bits, null deref, misaligned access (pack 05 goes deep). `labs/lab_signed_overflow.cpp` prints:

```
runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
```

We add `-fno-sanitize-recover=undefined` so UB *aborts* (a test run then fails) instead of printing and carrying on.

**LeakSanitizer** (part of ASan on Linux) reports allocations never freed when the process exits (`labs/lab_leak.cpp`).

**Costs and limits (why not on the radio).** Roughly 2x CPU and several x memory (VERIFY against your toolchain docs; order of magnitude is what matters). A small embedded target often has no room for shadow memory nor an OS runtime to host it. So sanitizers run on **host builds of the same code** in CI - another reason for the single-source-many-targets design from 3.2.

**What they do not catch:** bugs on paths no test executes; logic errors (a wrong but in-bounds byte); data races (that is **TSan**, `-fsanitize=thread`, a different tool; ASan and TSan cannot be combined because both claim control of the memory mapping and instrument the same accesses - pack 06).

**Windows note (relevant for your machine).** MinGW-w64 GCC has no `libasan`. Options: WSL2 Ubuntu (recommended), clang with ASan runtimes, or MSVC `/fsanitize=address` (ASan only; no UBSan in MSVC). Configure with `-DENABLE_SANITIZERS=OFF` to just build, but then do the sanitizer exercises in WSL2.

> **Common misconceptions**
> - *"ASan clean = no bugs."* It only reports bugs on executed paths and only memory-error classes.
> - *"The error is at the reported line."* The *access* is; the *cause* (wrong length computation) may be elsewhere.
> - *"UB just gives a wrong number."* Compilers assume UB never happens and may delete code (pack 05).
> - *"Sanitizers are slow so skip them."* Slow in CI is cheap; a field crash is not.

### 3.5 gdb: stopping time and looking

**Concrete first.** `labs/lab_gdb_crash.cpp` (built without sanitizers):

```
$ gdb ./lab_gdb_crash
(gdb) run
Program received signal SIGSEGV, Segmentation fault.
sum_list (n=0x0) at labs/lab_gdb_crash.cpp:14
14	    while (n->value >= 0) {
(gdb) bt
#0  sum_list (n=0x0) at labs/lab_gdb_crash.cpp:14
#1  0x... in main () at labs/lab_gdb_crash.cpp:25
(gdb) print n
$1 = (const Node *) 0x0
(gdb) frame 1
(gdb) info locals
```

**Derive the workflow.** A crash tells you *where the CPU gave up*, not *why*. `bt` (backtrace) shows the chain of calls; `frame N` + `print`/`info locals` show state at that call; `break file:line` + `run` + `next`/`step` lets you advance one line at a time; `watch var` stops when a variable changes; `x/16xb ptr` dumps 16 raw bytes (use it to check a frame you built); `finish` runs until the current function returns.

**Why `-g -O0`.** `-g` embeds a map from machine code back to source lines and variable names. At `-O2` the compiler reorders, merges and removes variables, so `print n` may say `<optimized out>` and stepping jumps around. CMake's `Debug` build gives `-g -O0`.

**Post-mortem.** A core dump (`ulimit -c unlimited`, then `gdb ./prog core`) lets you inspect a crash that happened in the field. You must keep the *exact* unstripped binary (or separate debug symbols) that produced it, matched by build ID - otherwise addresses cannot be mapped to lines.

> **Common misconceptions**
> - *"Crashes happen where the bug is."* The null pointer was *created* earlier; the crash is where it was *used*.
> - *"I can debug the Release binary the same way."* Optimised builds hide variables.
> - *"gdb fixes bugs."* It shows state; you still form the hypothesis.

---

## 4. Fully worked example: red -> green -> sanitizer-clean

Goal: add `count_equal(const uint8_t* p, size_t n, uint8_t v)` (how many bytes equal `v`) - used to count `0x7E` start markers in a capture buffer.

**Step 1 - write the test first, with hand-computed answers.**

```cpp
TEST(CountEqual, Basic) {
    const std::uint8_t b[] = {0x7E, 0x01, 0x7E, 0x7E};
    EXPECT_EQ(count_equal(b, 4, 0x7E), 3u);   // positions 0,2,3
    EXPECT_EQ(count_equal(b, 4, 0x02), 0u);
    EXPECT_EQ(count_equal(nullptr, 0, 0x7E), 0u);   // empty input must be safe
}
```

**Step 2 - stub that compiles, run, see red.** `return 99;` -> all three fail: *good, the test can fail*.

**Step 3 - implement.**

```cpp
inline std::size_t count_equal(const std::uint8_t* p, std::size_t n, std::uint8_t v) {
    std::size_t c = 0;
    for (std::size_t i = 0; i < n; ++i) if (p[i] == v) ++c;
    return c;
}
```

Trace the first assertion: i=0: 0x7E==0x7E -> c=1; i=1: 0x01 -> no; i=2: c=2; i=3: c=3. Returns 3. Green.

**Step 4 - see how a bug looks.** Change the loop to `i <= n`. On the empty input `n==0`, it reads `p[0]` with `p == nullptr` -> UBSan: `runtime error: load of null pointer`. On `b[4]` it reads one past the array -> ASan `stack-buffer-overflow` ... and *without sanitizers the test might still pass*, which is the whole point of the tooling.

**Step 5 - gdb if you cannot see it.** `break count_equal`, `run`, `next` x3, `print i`, `print n`, `print p[i]` - the loop variable exceeds `n-1` on the last step.

---

## 5. Questions

Numbering is continuous; `answers.md` mirrors it. Attempt before looking.

### 5.1 Explain why

**Q1.** *Context: the gateway has ~400 `.cpp` files; a developer edits one protocol parser.* Explain why compile and link are separate steps, and what a build system saves by exploiting that. Then say what error you get if you forget to link the object file that defines a function.

**Q2.** *Context: the test target needs `include/` of stubs, the firmware target must never see test headers.* Why is `target_include_directories(t PRIVATE ...)` preferred over global `include_directories()`? Give the failure that the global form eventually causes.

**Q3.** *Context: CI wants one job to run both ASan and TSan to save time.* Why can't they be combined in one binary, and why do we not ship sanitizer builds to the radios themselves?

### 5.2 Calculate

**Q4.** *Context: an ASan report says the heap block starts at `0x602000000010` (8 bytes).* Using `shadow = (addr >> 3) + 0x7fff8000`, compute the shadow address of that first byte, in hex. What is the shadow byte value for the block if all 8 bytes are valid? How many shadow bytes does a 1 MiB buffer need?

**Q5.** *Context: sizing a buffer for the toy debug frame.* The frame is `0x7E, N, payload[N], checksum`. Give the total size for N=0, N=3, N=255. For N=3 what fraction of the frame is overhead?

**Q6.** *Context: a trivial XOR integrity byte on a debug frame.* Compute the XOR checksum of `{0x12,0x34,0x56}` by hand, in binary. Then compute it for `{0x01,0x02}` and `{0x02,0x01}`. What does the result tell you about the detection quality?

### 5.3 Predict

**Q7.** *Context: the team's incremental builds feel slow.* In the build: `frame.hpp` includes `checksum.hpp`; `test_frame.cpp` includes `frame.hpp`; `test_checksum.cpp` includes `checksum.hpp`; `test_toolchain_info.cpp` includes only `toolchain_info.hpp`. You edit `checksum.hpp`. Which `.cpp` files are recompiled? Which are not? Is a relink needed?

**Q8.** *Context: reviewing lab A before a demo.* Predict what `lab_heap_overflow` prints/does under ASan for `./lab_heap_overflow 7`, `./lab_heap_overflow 8`, and `./lab_heap_overflow 9`. Explain each.

**Q9.** *Context: lab B.* Predict the behaviour of `lab_signed_overflow` (a) built with UBSan and `-fno-sanitize-recover`, (b) built without sanitizers at `-O0`, (c) at `-O2`. Which of these is *guaranteed* by the standard?

### 5.4 Trace by hand

**Q10.** *Context: debugging `lab_gdb_crash` in gdb.* Write the value of `n` (address of which node), `n->value` and `total` at the top of each loop iteration, up to the crash. State exactly what gdb's `bt` shows and which expression is the faulting dereference.

**Q11.** *Context: validating a debug frame on a gateway.* (a) Trace `frame_encode` for payload `{0x01,0x02,0x04}` into a 6-byte buffer: list `out[0..5]`. (b) Trace `frame_decode` for `{0x7E,0x02,0x0A,0x0B,0x01}` with `in_len = 5`: which checks run in which order and what is returned? (c) Same for `{0x7E,0x02,0x0A,0x0B,0x02}`.

### 5.5 Find the bug

**Q12.** *Context: code review of the frame encoder.* Find the bug and say what ASan prints:

```cpp
std::size_t frame_encode(const uint8_t* p, std::size_t n, uint8_t* out, std::size_t cap) {
    if (n > 255 || cap < n + 2) return 0;
    out[0] = 0x7E; out[1] = (uint8_t)n;
    for (std::size_t i = 0; i < n; ++i) out[2 + i] = p[i];
    out[2 + n] = xor_checksum(p, n);
    return n + 3;
}
```

**Q13.** *Context: a new engineer's CMake for a test target fails at link time.*

```cmake
add_executable(t test_a.cpp)
target_include_directories(t PRIVATE include)
target_link_libraries(t PRIVATE GTest::gtest)
```

It fails with `undefined reference to 'main'`. Why, and what is the one-word fix? What else could cause `undefined reference to testing::...`?

**Q14.** *Context: tidying repeated checks in a test file.* Explain why this does not compile and give two correct fixes:

```cpp
int decoded_len(const uint8_t* f, size_t n, uint8_t* out, size_t cap) {
    size_t r = frame_decode(f, n, out, cap);
    ASSERT_NE(r, kDecodeError);
    return static_cast<int>(r);
}
```

### 5.6 Implement

(Open `code/include/es_toolchain/`. Run `ctest` after each.)

**Q15.** *Context: integrity byte for the debug frame.* Implement `xor_checksum` so that `test_checksum.cpp` passes. Explain why `acc ^= data[i]` works but may produce a warning with `-Wconversion`.

**Q16.** *Context: a gateway debug channel emits framed bytes into a fixed buffer.* Implement `frame_encode`. It must never write beyond `out_cap` and must return 0 (writing nothing) on any failure.

**Q17.** *Context: the receiving side must treat the wire as hostile.* Implement `frame_decode`. Order the checks so that no byte is read before its existence is proven.

**Q18.** *Context: CI logs should state exactly how a failing binary was built.* Implement `cxx_standard()`, `asan_enabled()`, `assertions_enabled()` in `toolchain_info.hpp`. Then build twice: with `-DENABLE_SANITIZERS=OFF` and `ON`, and confirm the test adapts.

### 5.7 Connect

**Q19.** *Context: pack 02 (`../02-types-basics/`) covers integer promotion.* In `xor_checksum`, what is the type of the expression `acc ^ data[i]`, why do we write an explicit `static_cast<std::uint8_t>` around it, and why does that cast never change the numeric result here?

**Q20.** *Context: pack 05 (`../05-ub-layout-bit/`) and pack 06 (`../06-concurrency-rt/`).* You get three field reports: (a) crash after a large frame, (b) rare wrong output only in Release builds, (c) corrupted counter under load. Which tool (ASan, UBSan, TSan, gdb core, `NDEBUG` difference) is your first move for each, and why?

### 5.8 Interview-style

**Q21.** "Walk me through exactly what happens between typing `cmake --build build` and having a test executable, mentioning at least four distinct stages and the artefacts between them."

**Q22.** "A radio crashed in the field. You have a core file and no way to reproduce. What do you need, and what do you do?"

**Q23.** "How would you unit-test a P25/DMR frame encoder when the real transmit hardware is not available - and without ever touching a live channel?"

**Q24.** "ASan and UBSan are clean on all tests. Your manager says the memory-safety work is done. What do you say?"

---

## 6. Self-assessment

- [ ] I can explain compile vs link and recognise `undefined reference` and `multiple definition`.
- [ ] I can write a `CMakeLists.txt` with targets, per-target include dirs, and FetchContent.
- [ ] I know EXPECT vs ASSERT and why ASSERT needs a `void` function.
- [ ] I can state how ASan's shadow memory detects an out-of-bounds write (Q4).
- [ ] I can read an ASan report bottom-up.
- [ ] I can run gdb to a crash, use `bt`, `frame`, `print`, and explain why `-O0 -g`.
- [ ] All tests pass with `-DUSE_SOLUTIONS=OFF` after *my* implementations, with sanitizers on.
- [ ] I can answer Q21-Q24 aloud in under two minutes each.
