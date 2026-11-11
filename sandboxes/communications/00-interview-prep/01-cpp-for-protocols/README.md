# Etherstack C++ interview packs (module 01-cpp)

Seven self-contained worksheet packs. Each pack = 2-4 hours. Do them in order; later packs link back to earlier ones.

| Pack | Topic | Core idea |
|------|-------|-----------|
| `01-toolchain` | CMake, GoogleTest, gdb, sanitizers | Make bugs loud and reproducible |
| `02-types-basics` | fixed-width ints, promotion, enum class, refs/pointers/const/constexpr | Know what the compiler *really* computes |
| `03-lifetime-move` | RAII, rule of 0/3/5, move, smart pointers | Who owns it, who frees it, when |
| `04-containers-templates` | array, vector, span, string_view, optional, variant, templates | Safe views over protocol bytes |
| `05-ub-layout-bit` | UB, aliasing, padding, bit tricks | What hardware does vs. what C++ promises |
| `06-concurrency-rt` | threads, mutex, condvar, atomics, real-time rules | Hot paths: no heap, no locks, no throw |
| `07-sm-builds` | state machines, RingBuffer, SpscQueue, FixedVector | Put it all together |

## Per-pack layout

```
NN-name/
  worksheet.md    <- read and work through this
  answers.md      <- check AFTER you attempt a question
  cheatsheet.md   <- one-page recall, every line points back to a worksheet section
  code/
    CMakeLists.txt
    include/<es_pack>/*.hpp     <- STUBS: compile, tests FAIL until you implement
    tests/*.cpp                 <- GoogleTest
    solutions/<es_pack>/*.hpp   <- reference solutions (do not peek first)
    labs/                       <- (pack 01) deliberately buggy programs
```

## Build and run

```bash
cd NN-name/code
cmake -S . -B build                      # downloads GoogleTest via FetchContent (needs internet once)
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Check your reference-solution understanding (all tests must pass):

```bash
cmake -S . -B build-sol -DUSE_SOLUTIONS=ON
cmake --build build-sol -j && ctest --test-dir build-sol --output-on-failure
```

Options: `-DENABLE_SANITIZERS=OFF`, `-DCMAKE_CXX_STANDARD=17` (pack 04/05 need C++20 for `std::span` / `<bit>`; the worksheet says what to do in C++17).
Flags used: `-Wall -Wextra -fsanitize=address,undefined` (MSVC: `/W4 /fsanitize=address`).

**Packs 06 and 07 (threads):** also run them under ThreadSanitizer, which cannot be combined with ASan: `cmake -S . -B build-tsan -DENABLE_TSAN=ON` (GCC/Clang on Linux/macOS/WSL2 only). On x86, wrong memory orders often pass at run time; TSan checks the happens-before edges you wrote. Pack 06 has a deliberately racy demo (`labs/lab_lost_update.cpp`, built without sanitizers) and an allocation-counting test executable (`pack06_rt_tests`, replaces global `operator new`). Pack 07 builds with C++17 (`-DCMAKE_CXX_STANDARD=20` also works). All thread tests have deadlines and a 60 s ctest timeout, so a wrong implementation fails instead of hanging.

**Status of the code:** the packs were written on a machine without a C++ compiler or CMake, so the code has been reviewed by hand but **not compiled or run**. Expect to fix small compile errors on first build, and please report any test that disagrees with its worksheet.

**Windows note:** MinGW-w64 GCC does not ship ASan/UBSan. Use WSL2 (Ubuntu), clang with sanitizer runtimes, or MSVC (ASan only). Pack 01 explains this.

## Safety

All exercises use synthetic or hand-computed bytes. Never point any test or tool at live public-safety radio traffic (P25/DMR/TETRA). Protocol figures that the author is not certain of are tagged **VERIFY** - check them against the current standard (TIA-102, ETSI TS 102 361, ETSI EN 300 392) before relying on them.
