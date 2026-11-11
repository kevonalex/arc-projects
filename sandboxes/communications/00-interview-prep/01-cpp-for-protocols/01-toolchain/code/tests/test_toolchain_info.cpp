#include <gtest/gtest.h>

#include <es_toolchain/toolchain_info.hpp>

using namespace es::toolchain;

TEST(ToolchainInfo, StandardIs17Or20) {
    const int s = cxx_standard();
    EXPECT_TRUE(s == 17 || s == 20) << "got " << s;
}

// EXPECT_ASAN is injected by CMake: 1 when ENABLE_SANITIZERS=ON, else 0.
TEST(ToolchainInfo, AsanMatchesBuildConfiguration) {
    EXPECT_EQ(asan_enabled(), EXPECT_ASAN == 1);
}

TEST(ToolchainInfo, AssertionsFollowNdebug) {
#ifdef NDEBUG
    EXPECT_FALSE(assertions_enabled());
#else
    EXPECT_TRUE(assertions_enabled());
#endif
}
