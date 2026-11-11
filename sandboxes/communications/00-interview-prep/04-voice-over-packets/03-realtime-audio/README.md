# realtime audio

**Phase:** 0 · **Tier:** 1 · **Est. hours:** see `pack.yaml`

## Objectives

Complete the worksheet and `code/` exercises for this pack.

## Prerequisites

See the Prerequisites section in `worksheet.md` / `worksheet.ipynb`.

## Twins

Listed in `pack.yaml` when linked.

## How to work this pack

1. Attempt the worksheet without opening answers.
2. Implement stubs in `code/include/` (or `code/headers/`) until tests pass.
3. Only then read `answers.md` / `code/solutions/`.
4. Fill `cheatsheet.md` from memory.

```bash
cmake -S code -B code/build && cmake --build code/build && ctest --test-dir code/build --output-on-failure
# Spoiler / check reference:
cmake -S code -B code/build -DUSE_SOLUTIONS=ON && cmake --build code/build && ctest --test-dir code/build --output-on-failure
```
