# 06-cellular/04-mcptt-iwf  --  build & test
#
#   cmake -S . -B build                      # C++17 (default); add -DCMAKE_CXX_STANDARD=20 for C++20
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# The test file(s) are built twice:
#   student_tests   -> include/     (your stubs; fail until implemented)
#   solution_tests  -> solutions/   (reference; must pass)
# Flags: -Wall -Wextra -fsanitize=address,undefined (GCC/Clang). MinGW-w64: -DENABLE_SANITIZERS=OFF.
# Offline? Point FetchContent at a local googletest checkout:
#   cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest-1.14.0
cmake_minimum_required(VERSION 3.14)
project(pack06_04_mcptt LANGUAGES CXX)

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
find_package(Threads REQUIRED)

enable_testing()
include(GoogleTest)

function(configure_pack_target target inc_dir)
  target_include_directories(${target} PRIVATE ${inc_dir})
  target_link_libraries(${target} PRIVATE GTest::gtest_main Threads::Threads)
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

add_executable(student_tests tests/test_mcptt.cpp)
configure_pack_target(student_tests ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(solution_tests tests/test_mcptt.cpp)
configure_pack_target(solution_tests ${CMAKE_CURRENT_SOURCE_DIR}/solutions)

gtest_discover_tests(student_tests  TEST_PREFIX student.)
gtest_discover_tests(solution_tests TEST_PREFIX solution.)

# Demo program built against the REFERENCE solution (not part of ctest).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/demo/nrcalc.cpp)
  add_executable(nrcalc demo/nrcalc.cpp)
  configure_pack_target(nrcalc ${CMAKE_CURRENT_SOURCE_DIR}/solutions)
endif()
