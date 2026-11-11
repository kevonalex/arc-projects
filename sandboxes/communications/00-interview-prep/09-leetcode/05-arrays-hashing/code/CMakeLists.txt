cmake_minimum_required(VERSION 3.20)
project(leetcode_pack LANGUAGES CXX)

# Identical in every pack. Layout:
#   headers/    declarations ("stubs") + small helpers -- the interface you code against
#   solutions/  reference implementations (*.cpp)
#   tests/      GoogleTest suites (*.cpp)
#
# Practise workflow (blank-page mode):
#   mkdir attempts && copy solutions\*.cpp attempts\ && empty the function bodies
#   cmake -S . -B build -DIMPL_DIR=attempts
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(IMPL_DIR "solutions" CACHE STRING "Directory (relative to this file) holding the implementation .cpp files")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # MSVC: match the CRT of our targets
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB IMPL_SRC  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${IMPL_DIR}/*.cpp")
file(GLOB TEST_SRC  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
if(NOT IMPL_SRC)
  message(FATAL_ERROR "No .cpp files found in ${IMPL_DIR}/ -- check -DIMPL_DIR")
endif()

# Warnings + sanitizers for OUR code only (GoogleTest is built with its own flags).
add_library(pack_flags INTERFACE)
if(MSVC)
  target_compile_options(pack_flags INTERFACE /W4 /fsanitize=address)
else()
  target_compile_options(pack_flags INTERFACE
    -Wall -Wextra -g -fno-omit-frame-pointer
    -fsanitize=address,undefined -fno-sanitize-recover=undefined)
  target_link_options(pack_flags INTERFACE -fsanitize=address,undefined)
endif()

add_library(pack_impl STATIC ${IMPL_SRC})
target_include_directories(pack_impl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/headers")
target_link_libraries(pack_impl PUBLIC pack_flags)

add_executable(pack_tests ${TEST_SRC})
target_link_libraries(pack_tests PRIVATE pack_impl GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(pack_tests)
