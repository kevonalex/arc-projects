// LAB B (worksheet 3.2 / Q9). Deliberate UB: shift count == width of the type.
// Run with no arguments: argc == 1, so n == 32.
// x86-64 and AArch64 (VERIFY on your CPU): the hardware masks the count to 5 bits -> 1u << 32 behaves as 1u << 0 -> prints 1.
// 32-bit ARM register-shift (VERIFY): counts >= 32 give 0.  A compiler may also constant-fold differently.
// => The same source gives different answers on different CPUs: that is exactly why the standard calls it undefined.
// Compile this file with -fsanitize=undefined to see the report:
//   runtime error: shift exponent 32 is too large for 32-bit type 'unsigned int'
#include <cstdio>

int main(int argc, char**) {
    unsigned n = 31u + static_cast<unsigned>(argc);  // 32
    unsigned r = 1u << n;                            // BUG: n >= 32
    std::printf("1u << %u = %u\n", n, r);
    return 0;
}
