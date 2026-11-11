cmake_minimum_required(VERSION 3.16)
project(es_pack06_concurrency_rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard (17 or 20)")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(USE_SOLUTIONS     "Build against solutions/ instead of the stubs in include/" OFF)
option(ENABLE_SANITIZERS "Compile with -fsanitize=address,undefined (MSVC: /fsanitize=address)" ON)
option(BUILD_LABS        "Build labs/ (deliberately racy demo, NO sanitizers)" ON)
option(ENABLE_TSAN       "ThreadSanitizer instead of ASan/UBSan (the two cannot be combined; GCC/Clang only)" OFF)

if(USE_SOLUTIONS)
  set(ES_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/solutions)
else()
  set(ES_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
message(STATUS "Headers from: ${ES_HEADER_DIR}")

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Threads REQUIRED)

enable_testing()
include(GoogleTest)

function(es_configure target)
  target_include_directories(${target} PRIVATE ${ES_HEADER_DIR})
  target_link_libraries(${target} PRIVATE GTest::gtest_main Threads::Threads)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(ENABLE_SANITIZERS AND NOT ENABLE_TSAN)
      target_compile_options(${target} PRIVATE /fsanitize=address)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -g -fno-omit-frame-pointer)
    if(ENABLE_TSAN)
      target_compile_options(${target} PRIVATE -fsanitize=thread)
      target_link_options(${target} PRIVATE -fsanitize=thread)
    elseif(ENABLE_SANITIZERS)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
  # 60 s: a deadlocked test must FAIL, never hang CI.
  gtest_discover_tests(${target} PROPERTIES TIMEOUT 60)
endfunction()

add_executable(pack06_tests
  tests/test_queue.cpp
  tests/test_atomics.cpp
  tests/test_pool.cpp)
es_configure(pack06_tests)

# Separate executable: it REPLACES global operator new/delete to count heap allocations.
add_executable(pack06_rt_tests tests/test_rt_alloc.cpp)
es_configure(pack06_rt_tests)

if(BUILD_LABS)
  # Deliberately racy: no sanitizers, so we can SEE the lost updates (worksheet 3.1).
  add_executable(lab_lost_update labs/lab_lost_update.cpp)
  target_link_libraries(lab_lost_update PRIVATE Threads::Threads)
  if(MSVC)
    target_compile_options(lab_lost_update PRIVATE /W4)
  else()
    target_compile_options(lab_lost_update PRIVATE -Wall -Wextra -O1)
  endif()
endif()
