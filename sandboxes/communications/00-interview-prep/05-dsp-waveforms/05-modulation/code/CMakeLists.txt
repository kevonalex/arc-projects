cmake_minimum_required(VERSION 3.16)
project(pack05_modulation LANGUAGES CXX)

# Build:   cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
# Stubs (default) FAIL on purpose.  Reference solutions:  cmake -S . -B build -DUSE_SOLUTIONS=ON
# Sanitizers (-fsanitize=address,undefined) need GCC/Clang on Linux/macOS/WSL (not MinGW).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_SOLUTIONS "Build against solutions/ instead of stubs/" OFF)
option(ENABLE_SANITIZERS "Enable ASan + UBSan" ON)

if(USE_SOLUTIONS)
  set(IMPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/solutions)
else()
  set(IMPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stubs)
endif()

file(GLOB IMPL_SRC CONFIGURE_DEPENDS ${IMPL_DIR}/*.cpp)
add_library(pack_impl STATIC ${IMPL_SRC})
target_include_directories(pack_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
  target_compile_options(pack_impl PUBLIC /W4)
  if(ENABLE_SANITIZERS)
    target_compile_options(pack_impl PUBLIC /fsanitize=address)
  endif()
else()
  target_compile_options(pack_impl PUBLIC -Wall -Wextra)
  if(ENABLE_SANITIZERS)
    if(MINGW)
      message(WARNING "MinGW has no ASan/UBSan runtime; sanitizers disabled. Use WSL/Linux/clang.")
    else()
      target_compile_options(pack_impl PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(pack_impl PUBLIC -fsanitize=address,undefined)
    endif()
  endif()
endif()

include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
file(GLOB TEST_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
add_executable(run_tests ${TEST_SRC})
target_link_libraries(run_tests PRIVATE pack_impl GTest::gtest_main)
add_test(NAME all_tests COMMAND run_tests)
