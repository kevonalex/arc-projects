#!/bin/sh
# Helper: configure+build+ctest every pack through CMake (FetchContent downloads GoogleTest v1.14.0).
# usage: ./.verify_cmake.sh [build-root]     (default /tmp/lc_cmake)
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-/tmp/lc_cmake}"
for d in "$ROOT"/[0-9][0-9]-*; do
  p="$(basename "$d")"
  [ -d "$d/code" ] || continue
  cmake -S "$d/code" -B "$OUT/$p" >"$OUT.$p.cfg.log" 2>&1 || { echo "$p: CONFIGURE FAILED"; continue; }
  cmake --build "$OUT/$p" -j4 >"$OUT.$p.build.log" 2>&1 || { echo "$p: BUILD FAILED"; continue; }
  warns=$(grep -c "warning:" "$OUT.$p.build.log")
  res=$(ctest --test-dir "$OUT/$p" 2>&1 | grep -E "tests passed|tests failed")
  echo "$p: $res (compiler warnings: $warns)"
done
