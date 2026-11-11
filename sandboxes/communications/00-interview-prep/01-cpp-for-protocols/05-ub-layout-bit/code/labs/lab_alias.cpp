// LAB C (worksheet 3.3 / Q10). Deliberate strict-aliasing violation: an int object accessed through a float lvalue.
// Typical GCC/Clang: -O0 prints 1073741824 (0x40000000, the bits of 2.0f - the write really reached the int)
//                    -O2 prints 1            (the compiler assumes *i and *f cannot overlap and returns the cached 1)
// Neither is guaranteed. The fix is std::memcpy / std::bit_cast (pack 05 Q20 / bits.hpp).
#include <cstdio>

static int set_and_read(int* i, float* f) {
    *i = 1;
    *f = 2.0f;   // as far as the compiler is concerned, cannot modify an int object
    return *i;   // so it may return 1 without re-reading memory
}

int main() {
    int x = 0;
    float* fp = reinterpret_cast<float*>(&x);  // BUG: type punning through an incompatible pointer
    std::printf("%d\n", set_and_read(&x, fp));
    return 0;
}
