cmake_minimum_required(VERSION 3.16)
project(pack_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_SOLUTIONS "Build against solutions/ instead of include/ stubs" OFF)
option(ENABLE_SANITIZERS "Address/UB sanitizers (non-MSVC)" ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(USE_SOLUTIONS)
  set(PACK_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/solutions)
else()
  set(PACK_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

add_executable(pack_tests tests/test_placeholder.cpp)
target_include_directories(pack_tests PRIVATE ${PACK_HEADER_DIR})
target_link_libraries(pack_tests PRIVATE GTest::gtest_main)
target_compile_options(pack_tests PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
  $<$<AND:$<BOOL:${ENABLE_SANITIZERS}>,$<CXX_COMPILER_ID:GNU,Clang>>:-fsanitize=address,undefined>
)
target_link_options(pack_tests PRIVATE
  $<$<AND:$<BOOL:${ENABLE_SANITIZERS}>,$<CXX_COMPILER_ID:GNU,Clang>>:-fsanitize=address,undefined>
)

include(GoogleTest)
gtest_discover_tests(pack_tests PROPERTIES LABELS "exercise")
