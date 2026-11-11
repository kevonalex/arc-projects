# Etherstack interview prep — LeetCode pattern packs

Thirteen **pattern** packs (not problem dumps). Each pack teaches *why* a technique works (invariants, proof sketches of correctness and complexity), then applies it to 3–12 problems that each carry a radio/protocol-flavoured context line.

| Pack | Pattern | Problems |
|------|---------|----------|
| [01-bits](01-bits) | bit manipulation, XOR algebra, two's complement | 191 190 338 136 137 268 461 477 371 201 89 393 |
| [02-parsing](02-parsing) | DFAs, total parsers, framing | 8 468 271 443 65 |
| [03-queues-buffers](03-queues-buffers) | ring buffers, monotonic deque | 622 641 232 239 346 |
| [04-design](04-design) | map + list/array compositions | 146 359 362 705 706 155 |
| [05-arrays-hashing](05-arrays-hashing) | lookup, canonical keys, prefix/suffix | 1 49 238 128 41 |
| [06-sliding-window](06-sliding-window) | monotone predicates, two pointers | 3 424 76 11 |
| [07-linked-lists](07-linked-lists) | pointer invariants, Floyd | 206 21 141 19 |
| [08-heaps](08-heaps) | bounded heap, k-way merge, two heaps | 215 23 295 |
| [09-binary-search](09-binary-search) | monotone predicate, rotated arrays | 704 33 153 |
| [10-trees](10-trees) | structural induction, BFS levels | 104 226 102 98 297 |
| [11-graphs](11-graphs) | BFS/DFS, Kahn, Dijkstra, clone | 200 207 743 133 |
| [12-intervals](12-intervals) | sort + sweep, exchange argument | 56 57 435 |
| [13-dp](13-dp) | optimal substructure, trellis/Viterbi link | 70 198 322 300 1143 |

## Layout of every pack
```
NN-name/
  worksheet.md    theory (why it works) + problems with blanks for the 6-step method + misconceptions
  answers.md      full write-ups: clarify -> brute force -> optimise -> code -> hand-trace -> final complexity
  cheatsheet.md   one-page recap + templates + gotchas + "say it out loud" script
  code/
    CMakeLists.txt   identical in all packs; GoogleTest v1.14.0 via FetchContent
    headers/         declarations / stubs (+ small helpers such as ListArena, TreeArena, NodeArena)
    solutions/       reference implementations
    tests/           GoogleTest suites (normal + edge cases, brute-force cross-checks where cheap)
```

## The method (used in every problem)
**Clarify → Brute force + complexity → Optimise → Code → Hand-trace (normal + edge) → Final complexity.**

## Build and run one pack
```
cd 01-bits/code
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
Compiler flags (set in `CMakeLists.txt`): C++20, `-Wall -Wextra -fsanitize=address,undefined` on GCC/Clang (`/W4 /fsanitize=address` on MSVC). GoogleTest is downloaded at configure time (network needed once).

## Practise in "blank page" mode
```
cd 07-linked-lists/code
mkdir attempts
copy solutions\*.cpp attempts\      (cp on Linux/macOS)   # then empty the function bodies
cmake -S . -B build -DIMPL_DIR=attempts
cmake --build build && ctest --test-dir build --output-on-failure
```
The headers are the stable interface; only the `.cpp` in `IMPL_DIR` changes.

## Helper scripts (optional)
* `.verify.sh <pack>` — compile + run one pack with plain `g++` against a locally built `libgtest.a` (no CMake).
* `.verify_cmake.sh [build-root]` — configure/build/ctest **every** pack through CMake.

Ownership note: in the pointer-heavy packs (07, 08, 10, 11) the functions never `delete` nodes; test arenas own everything, so ASan/LSan stay quiet even for cyclic structures. `clone_graph` and `deserialize` therefore take the arena that will own the new nodes (a documented deviation from LeetCode's signatures).
