#pragma once
// STUB - worksheet Q18. Ask the compiler about itself.
namespace es::toolchain {

// Return 17 or 20 (the language standard this translation unit was compiled as).
// Hint: __cplusplus is 201703L / 202002L, but MSVC reports 199711L unless
// /Zc:__cplusplus; use _MSVC_LANG there.
inline int cxx_standard() {
    return 0;  // TODO
}

// True iff AddressSanitizer instrumentation is compiled into this TU.
// GCC defines __SANITIZE_ADDRESS__; clang answers __has_feature(address_sanitizer).
inline bool asan_enabled() {
    return false;  // TODO
}

// True iff this is NOT an NDEBUG (release-style) build.
inline bool assertions_enabled() {
    return false;  // TODO
}

}  // namespace es::toolchain
