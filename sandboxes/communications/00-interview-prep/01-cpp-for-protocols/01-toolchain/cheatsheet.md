# Pack 01 cheat sheet - every line points back to the worksheet

**Pipeline**
- preprocess -> compile (per `.cpp` -> `.o`) -> link; compile errors vs `undefined reference` (linker) - worksheet §3.1
- `multiple definition` = ODR violation; fix with `inline`/one `.cpp` - §3.1
- `-Wall -Wextra` = free bug reports; `-Werror` in CI, not in the learning stubs - §3.1

**CMake**
- `cmake -S . -B build` = configure+generate; `cmake --build build` = build - §3.2
- `target_*` commands + `PRIVATE/PUBLIC/INTERFACE` instead of global flags - §3.2
- `FetchContent` pins GoogleTest -> reproducible, no installs - §3.2
- Debug = `-g -O0`; Release = `-O3 -DNDEBUG` (asserts vanish!) - §3.2 (Q20)
- `-DUSE_SOLUTIONS=ON` swaps header dir only; tests unchanged - §3.2
- link `GTest::gtest_main` (has `main`), not `GTest::gtest` - §3.2 (Q13)

**GoogleTest**
- `TEST(Suite, Name)`; hand-compute expected bytes first; see red before green - §3.3, §4
- `EXPECT_*` continues, `ASSERT_*` returns -> `void` functions only - §3.3 (Q14)
- `ctest --output-on-failure`, `--gtest_filter=Suite.Name` - §3.3

**Sanitizers**
- ASan: shadow byte per 8 app bytes; `shadow=(addr>>3)+OFFSET` - §3.4 (Q4)
- shadow `0` = all 8 OK, `1..7` = first k OK, negative = poisoned - §3.4
- redzones around heap blocks catch overflow; freed memory poisoned catches use-after-free - §3.4
- read ASan report bottom-up: allocation site -> distance -> access line - §3.4
- UBSan + `-fno-sanitize-recover=undefined` = UB aborts the test - §3.4
- LeakSanitizer reports at exit - §3.4
- ASan + TSan cannot be combined; run separately - §3.4 (Q3)
- Clean sanitizer run != no bugs (only executed paths, detectable classes) - §3.4 (Q24)
- MinGW has no ASan -> WSL2 / clang / MSVC `/fsanitize=address` - §3.4

**gdb**
- build `-g -O0`; `run`, `bt`, `frame N`, `print x`, `info locals` - §3.5
- `break f` / `break file:line`, `next`, `step`, `finish`, `watch v`, `x/16xb p` - §3.5
- Crash site = where used, not where made null - §3.5 (Q10)
- Core dump needs matching unstripped binary + build ID - §3.5 (Q22)

**Frame toy format**
- `0x7E | N | payload[N] | xor(payload)`; size `N+3`; max 255 - §4, Q5
- check lengths before reading bytes - Q17
- XOR checksum is order-blind (weak) -> real stacks use CRC (VERIFY polynomials) - Q6
