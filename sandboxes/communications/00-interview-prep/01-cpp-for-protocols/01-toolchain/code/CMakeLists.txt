cmake_minimum_required(VERSION 3.16)
project(es_pack01_toolchain LANGUAGES CXX)

# ---- options ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard (17 or 20)")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)   # -g -O0: gdb friendly
endif()

option(USE_SOLUTIONS     "Build against solutions/ instead of the stubs in include/" OFF)
option(ENABLE_SANITIZERS "Compile with -fsanitize=address,undefined (MSVC: /fsanitize=address)" ON)
option(BUILD_LABS        "Build the deliberately buggy lab programs" ON)

if(USE_SOLUTIONS)
  set(ES_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/solutions)
else()
  set(ES_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
message(STATUS "Headers from: ${ES_HEADER_DIR}")

# ---- GoogleTest (self-contained) --------------------------------------------
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# ---- shared per-target configuration ----------------------------------------
function(es_configure target)
  target_include_directories(${target} PRIVATE ${ES_HEADER_DIR})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(ENABLE_SANITIZERS)
      target_compile_options(${target} PRIVATE /fsanitize=address)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -g -fno-omit-frame-pointer)
    if(ENABLE_SANITIZERS)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
endfunction()

# ---- tests --------------------------------------------------------------------
add_executable(pack01_tests
  tests/test_checksum.cpp
  tests/test_frame.cpp
  tests/test_toolchain_info.cpp)
es_configure(pack01_tests)
target_link_libraries(pack01_tests PRIVATE GTest::gtest_main)
if(ENABLE_SANITIZERS)
  target_compile_definitions(pack01_tests PRIVATE EXPECT_ASAN=1)
else()
  target_compile_definitions(pack01_tests PRIVATE EXPECT_ASAN=0)
endif()
gtest_discover_tests(pack01_tests PROPERTIES TIMEOUT 30)

# ---- labs (NOT tests: run them by hand, see worksheet section 3.4 / 3.5) --------
if(BUILD_LABS)
  foreach(lab lab_heap_overflow lab_signed_overflow lab_leak)
    add_executable(${lab} labs/${lab}.cpp)
    es_configure(${lab})
  endforeach()
  # The gdb lab must crash with a plain SIGSEGV, so no sanitizers here.
  add_executable(lab_gdb_crash labs/lab_gdb_crash.cpp)
  if(MSVC)
    target_compile_options(lab_gdb_crash PRIVATE /W4)
  else()
    target_compile_options(lab_gdb_crash PRIVATE -Wall -Wextra -g -O0)
  endif()
endif()
