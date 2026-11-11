#pragma once
// SOLUTION - do not read before attempting Q18.
namespace es::toolchain {

inline int cxx_standard() {
#if defined(_MSVC_LANG)
    constexpr long lang = _MSVC_LANG;   // MSVC keeps __cplusplus at 199711L by default
#else
    constexpr long lang = __cplusplus;
#endif
    return lang >= 202002L ? 20 : 17;
}

inline bool asan_enabled() {
#if defined(__SANITIZE_ADDRESS__)
    return true;                         // GCC and MSVC
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
    return true;                         // clang
#else
    return false;
#endif
#else
    return false;
#endif
}

inline bool assertions_enabled() {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

}  // namespace es::toolchain
