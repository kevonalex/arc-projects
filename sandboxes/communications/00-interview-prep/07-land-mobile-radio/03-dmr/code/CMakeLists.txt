cmake_minimum_required(VERSION 3.18)
project(lmr_dmr LANGUAGES CXX)

# Etherstack interview pack 03 - DMR (ETSI TS 102 361)
# SYNTHETIC FRAMES ONLY. Nothing in this project receives live radio traffic.
#
# Default build : tests compile against the STUBS in include/  (tests FAIL until you implement them)
# Reference run : cmake -S . -B build -DLMR_USE_SOLUTIONS=ON    (tests compile against solutions/)
# Sanitizers need gcc/clang on Linux, macOS or WSL (MinGW has no ASan).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LMR_USE_SOLUTIONS "Build tests against solutions/ instead of the stubs in include/" OFF)
if(LMR_USE_SOLUTIONS)
  set(LMR_IMPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/solutions)
else()
  set(LMR_IMPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
message(STATUS "Implementation headers: ${LMR_IMPL_DIR}")

# Build flags required by the brief: -Wall -Wextra -fsanitize=address,undefined
add_library(lmr_flags INTERFACE)
if(MSVC)
  target_compile_options(lmr_flags INTERFACE /W4)
else()
  target_compile_options(lmr_flags INTERFACE -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(lmr_flags INTERFACE -fsanitize=address,undefined)
endif()

add_library(lmr_impl INTERFACE)
target_include_directories(lmr_impl INTERFACE ${LMR_IMPL_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_link_libraries(lmr_impl INTERFACE lmr_flags)

include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(test_dmr
  tests/test_burst.cpp
  tests/test_slot_type.cpp
  tests/test_superframe.cpp
  tests/test_bptc.cpp)
target_link_libraries(test_dmr PRIVATE lmr_impl GTest::gtest_main)
gtest_discover_tests(test_dmr DISCOVERY_MODE PRE_TEST)
