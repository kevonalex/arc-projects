#!/bin/sh
# Helper: compile+run one pack without CMake, using a locally built libgtest.a.
# usage: ./.verify.sh 01-bits
set -e
P="$(dirname "$0")/$1/code"
GT=/tmp/googletest/googletest
g++ -std=c++20 -Wall -Wextra -g -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -I "$P/headers" -I "$GT/include" "$P"/solutions/*.cpp "$P"/tests/*.cpp /tmp/gt/libgtest.a -pthread -o "/tmp/lc_$1"
"/tmp/lc_$1" --gtest_brief=1
