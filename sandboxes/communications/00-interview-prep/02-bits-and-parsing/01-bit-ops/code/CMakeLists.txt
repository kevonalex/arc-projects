# Pack 01-bit-ops  --  build & test
#
#   cmake -S . -B build                      # C++17 (default)
#   cmake -S . -B build -DCMAKE_CXX_STANDARD=20
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Two test binaries are built from the SAME tests/test_bitops.cpp:
#   bitops_tests_student   -> includes include/bitops.h   (your stubs; fail until you implement them)
#   bitops_tests_solution  -> includes solutions/bitops.h (reference answers; must all pass)
#
# Sanitizers (-fsanitize=address,undefined) need GCC/Clang on Linux/macOS/WSL (or clang-cl).
# On MinGW-w64 they are unavailable: configure with -DENABLE_SANITIZERS=OFF.
cmake_minimum_required(VERSION 3.14)
project(pack01_bit_ops LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard (17 or 20)")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ENABLE_SANITIZERS "Build with -fsanitize=address,undefined" ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

function(configure_pack_target target inc_dir)
  target_include_directories(${target} PRIVATE ${inc_dir})
  target_link_libraries(${target} PRIVATE GTest::gtest_main)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    if(ENABLE_SANITIZERS)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
endfunction()

add_executable(bitops_tests_student tests/test_bitops.cpp)
configure_pack_target(bitops_tests_student ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(bitops_tests_solution tests/test_bitops.cpp)
configure_pack_target(bitops_tests_solution ${CMAKE_CURRENT_SOURCE_DIR}/solutions)

gtest_discover_tests(bitops_tests_student  TEST_PREFIX student.)
gtest_discover_tests(bitops_tests_solution TEST_PREFIX solution.)
