cmake_minimum_required(VERSION 3.16)
project(es_pack07_sm_builds LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard (17 or 20)")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(USE_SOLUTIONS     "Build against solutions/ instead of the stubs in include/" OFF)
option(ENABLE_SANITIZERS "Compile with -fsanitize=address,undefined (MSVC: /fsanitize=address)" ON)
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

add_executable(pack07_tests
  tests/test_call_state.cpp
  tests/test_msg_visit.cpp
  tests/test_ring.cpp
  tests/test_spsc.cpp
  tests/test_fixed_vector.cpp
  tests/test_integration.cpp)

target_include_directories(pack07_tests PRIVATE ${ES_HEADER_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(pack07_tests PRIVATE GTest::gtest_main Threads::Threads)

if(MSVC)
  target_compile_options(pack07_tests PRIVATE /W4 /wd4324)  # 4324: padding due to alignas (intended)
  if(ENABLE_SANITIZERS AND NOT ENABLE_TSAN)
    target_compile_options(pack07_tests PRIVATE /fsanitize=address)
  endif()
else()
  target_compile_options(pack07_tests PRIVATE -Wall -Wextra -g -fno-omit-frame-pointer)
  if(ENABLE_TSAN)
    target_compile_options(pack07_tests PRIVATE -fsanitize=thread)
    target_link_options(pack07_tests PRIVATE -fsanitize=thread)
  elseif(ENABLE_SANITIZERS)
    target_compile_options(pack07_tests PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined)
    target_link_options(pack07_tests PRIVATE -fsanitize=address,undefined)
  endif()
endif()

# 60 s: a deadlocked/looping test must FAIL, never hang CI.
gtest_discover_tests(pack07_tests PROPERTIES TIMEOUT 60)
