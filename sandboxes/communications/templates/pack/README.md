# {{title}}

**Phase:** {{phase}} · **Tier:** {{tier}} · **Est. hours:** {{est_hours}}  
**Status:** see `pack.yaml`

## Objectives

- …

## Prerequisites

- …

## Twins

- …

## How to work this pack

1. Attempt `worksheet.md` (or `.ipynb`) without opening answers.
2. Implement stubs in `code/include/` until tests pass.
3. Only then read `answers.md` / `code/solutions/`.
4. Fill `cheatsheet.md` from memory; compare.

```bash
cmake -S code -B code/build
cmake --build code/build
ctest --test-dir code/build --output-on-failure
# Reference solutions (spoiler):
cmake -S code -B code/build -DUSE_SOLUTIONS=ON && cmake --build code/build && ctest --test-dir code/build
```
